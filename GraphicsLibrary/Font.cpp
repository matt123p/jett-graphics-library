//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "Processor.h"
#include <algorithm>
#include <set>
#include "Font.h"
#include "GPUProcessor.h"
#include "CPUBitblt.h"
#include "Image.h"
#include "jett.h"

CFont::CFont()
{
    m_cl = NULL;
    m_cell_width = 0;
    m_cell_height = 0;
    m_free_cells = 0;
    m_font = NULL;
    m_cache_size = 1;
}

CFont::~CFont()
{
    clear( false );
}

// Drop the entire cache from memory
void CFont::clear( bool keep_sampler )
{
    m_cache.discard();
    m_cell_width = 0;
    m_cell_height = 0;
    m_free_cells = 0;
    
    m_glyphs.erase( m_glyphs.begin(), m_glyphs.end() );
    
    // Destroy the sampler
    if (m_font && !keep_sampler)
    {
        m_font->destroy();
        m_font = NULL;
    }
}

// Distort this font by using a matrix
void CFont::setMatrix( const jett_matrix& matrix )
{
    // Take a copy of the matrix
    m_matrix = matrix;
    
    // Drop the existing font cache
    clear( true );
    
    // Do we have a font, if so we must attach it again...
    if (m_font)
    {
        setupFont();
    }
}

void CFont::attachFont( CProcessor* cl, font_sampler* font, int cache_size )
{
    clear( false );

	m_cl = cl;
    m_font = font;
    m_cache_size = cache_size;
    
    setupFont();
}

void CFont::setupFont()
{
    // Determine the cell size (remember to apply the matrix!)
    float height = static_cast<float>(m_font->get_cell_height());
    float width = static_cast<float>(m_font->get_cell_width());
    jett_point a = m_matrix.apply( jett_point( 0, 0 ));
    jett_point c = m_matrix.apply( jett_point( width, 0 ));
    jett_point b = m_matrix.apply( jett_point( width, height ));
    jett_point d = m_matrix.apply( jett_point( 0, height ));

    float x1 = std::min(a.x,std::min(b.x,std::min(c.x,d.x)));
    float y1 = std::min(a.y,std::min(b.y,std::min(c.y,d.y)));
    float x2 = std::max(a.x,std::max(b.x,std::max(c.x,d.x)));
    float y2 = std::max(a.y,std::max(b.y,std::max(c.y,d.y)));
    
    m_cell_height = static_cast<int>(y2 - y1 + 0.5f);
    m_cell_width = static_cast<int>(x2 - x1 + 0.5f);
    int cell_size = static_cast<int>(m_cell_height * m_font->get_cell_width());
    
    // Round the cache to the nearest cell size
    m_free_cells = std::max(1,(m_cache_size / cell_size));
    
    // Ok, first create the memory block to work with
    m_cache.createImage(
        static_cast<int>(m_free_cells * m_cell_width), static_cast<int>(m_cell_height), image_mono, false, image_mode_gpu_ro );
    
    m_cache.erase();
    
    // Now apply the transformation matrix to the font
    // (but do not apply the translation as we do that!)
    jett_matrix matrix = m_matrix;
    matrix.e = 0.0f;
    matrix.f = 0.0f;
    m_font->set_matrix( m_matrix );
}


// Get a glyph, if it's not in the
// cache, put it there...
void CFont::getGlyph( int charcode, glyph_t& glyph, int flags )
{
    // Is this already in the font cache?
    auto index = m_glyphs.find( charcode );
    if (index != m_glyphs.end())
    {
        ++ index->second.m_uses;
        if (index->second.m_cell == -1)
        {
            // We have a size, but no actual bitmap
            // do we need the bitmap?
            if ((flags & glyph_no_render) != 0 && (flags & glyph_size) != 0)
            {
                glyph = index->second;
                return;
            }
        }
        else
        {
            // We have both bitmap and size, so any caller
            // will be happy!
            glyph = index->second;
            return;
        }
    }
    
    // Have we been asked not to render the
    // glyph, and user is not interested in the size?
    if ((flags & glyph_no_render) != 0 && (flags & glyph_size) == 0)
    {
        // Indicate invalid glyph
        glyph.m_advance_x = 0;
        glyph.m_advance_y = 0;
        glyph.m_lock = false;
        return;
    }
    
    // Have we got the glyph size, but not the glyph?
    
    // No, so we must add it - are there any
    // free cells?
    int cell_location = 0;
    if (m_free_cells == 0)
    {
        // No so create some space by picking
        // the lowest use glyph that isn't locked
        int lowest_use = 0xffffff;
        auto pick = m_glyphs.end();
        for (auto i = m_glyphs.begin(); i != m_glyphs.end(); ++ i)
        {
            if (!i->second.m_lock && i->second.m_cell != -1)
            {
                if (i->second.m_uses < lowest_use)
                {
                    pick = i;
                    lowest_use = i->second.m_uses;
                }
            }
        }
        
        if (pick == m_glyphs.end())
        {
            // There is no unlocked space in the cache to insert
            // this glyph
            if ((flags & glyph_size) == 0)
            {
                // There is no space in the cace to insert
                // a glyph (as they are all locked)
                glyph.m_advance_x = 0;
                glyph.m_advance_y = 0;
                glyph.m_lock = false;
                return;
            }
            else
            {
                // The caller doesn't require the glyph's bitmap
                // to be added to the cache, they just require
                // the size of the glyph
                cell_location = -1;
            }
        }
        else
        {        
            // Ok, discard this glyph
            cell_location = pick->second.m_cell;
            pick->second.m_cell = -1;
        }
    }
    else
    {
        // Yes, so use the next location
        cell_location = static_cast<int>((m_free_cells-1) * m_cell_width);
        -- m_free_cells;
    }
    
    // Fetch the new glyph
    jett_image img;
    m_font->get_glyph( charcode, img, glyph );
    if (index != m_glyphs.end())
    {
        // Copy across the existing data
        glyph.m_uses = index->second.m_uses;
    }
    else
    {
        glyph.m_uses = 1;
    }
    glyph.m_stride = img.getStride();
    glyph.m_width = img.getWidth();
    glyph.m_height = img.getHeight();
    glyph.m_lock = false;
    
    
    // Upload to the GPU memory
    if (img.getHeight() > m_cell_height || img.getWidth() > m_cell_width)
    {
        // Won't fit!
        throw jett_exception(JETT_FREETYPE_FAILURE, 0, "Glyph returned by font_sampler too big to fit in to cache's cell");
    }
    
    // Upload to cache
    if (cell_location != -1)
    {
        bitblt_params params;
        params.flags = perform_no_cm;
        params.src_x = 0;
        params.src_y = 0;
        params.dst_x1 = cell_location;
        params.dst_x2 = img.getWidth() + params.dst_x1 - 1;
        params.dst_y1 = 0;
        params.dst_y2 = img.getHeight() - 1;
        params.scale_src_x = 1.f;
        params.scale_src_y = 1.f;
        
        CCPUBitblt bl(NULL, 1, false );
        bl.copy(NULL, params, img, &m_cache);
    }
    
    // Push in to the cache
    glyph.m_cell = cell_location;
    m_glyphs[ charcode ] = glyph;
}

// Paint a glyph to an image at a specific position
void CFont::paintGlyph( unsigned int colour, CImage& target, int x, int y, float advance_x, float advance_y, const glyph_t& glyph, int bitblt_flags )
{
    bitblt_params params;
    
    int adv_x = static_cast<int>( advance_x + 0.5f );
    int adv_y = static_cast<int>( advance_y + 0.5f );

    params.flags = bitblt_flags;
    params.src_x = glyph.m_cell;
    params.src_y = 0;
    params.scale_src_x = 1.0f;
    params.scale_src_y = 1.0f;
    params.dst_x1 = x + glyph.m_left;
    params.dst_y1 = y - glyph.m_top;
    
    if ( (bitblt_flags & perform_rotate90) != 0 )
    {
        params.dst_x1 = x - glyph.m_height + glyph.m_top + adv_y;
        params.dst_y1 = y + glyph.m_left + adv_x;
        params.dst_x2 = params.dst_x1 + glyph.m_height - 1;
        params.dst_y2 = params.dst_y1 + glyph.m_width - 1;
    }
    else if ( (bitblt_flags & perform_rotate270) != 0)
    {
        params.dst_x1 = x - glyph.m_top - adv_y;
        params.dst_y1 = y - glyph.m_width - glyph.m_left - adv_x;
        params.dst_x2 = params.dst_x1 + glyph.m_height - 1;
        params.dst_y2 = params.dst_y1 + glyph.m_width - 1;
    }
    else if ( (bitblt_flags & perform_rotate180) != 0)
    {
        params.dst_x1 = x - glyph.m_width - glyph.m_left - adv_x;
        params.dst_y1 = y - glyph.m_height + glyph.m_top + adv_y;
        params.dst_x2 = params.dst_x1 + glyph.m_width - 1;
        params.dst_y2 = params.dst_y1 + glyph.m_height - 1;
    }    
    else
    {
        params.dst_x1 = x + glyph.m_left + adv_x;
        params.dst_y1 = y - glyph.m_top - adv_y;
        params.dst_x2 = params.dst_x1 + glyph.m_width - 1;
        params.dst_y2 = params.dst_y1 + glyph.m_height - 1;
    }

    if (!CBitblt::clip( params, &m_cache, &target, true ))
	{
		return;
	}

    m_cl->bitblt(1)->copy_glyph( colour, params, &m_cache, &target );
}

// Lock the glyph in the cache to stop it being
// discarded by a another glyph
void CFont::lockGlyph( int charcode )
{
    auto i = m_glyphs.find( charcode );
    if (i != m_glyphs.end())
    {
        i->second.m_lock = true;
        ++ i->second.m_uses;
    }

}

// Unlock the glyph
void CFont::unlockGlyphs()
{
    for (auto i = m_glyphs.begin(); i != m_glyphs.end(); ++ i)
    {
        i->second.m_lock = false;
    }
}


// Write out a string to a specific location on an image
void CFont::paintString( unsigned int colour, CImage &target, int x, int y, const TCHAR * str, int flags )
{
    int bitblt_flags = perform_white_is_trans;
    
    if ((flags & string_rotate_90) != 0)
    {
        bitblt_flags |= perform_rotate90;
    }
    else if ((flags & string_rotate_180) != 0)
    {
        bitblt_flags |= perform_rotate180;
    }
    else if ((flags & string_rotate_270) != 0)
    {
        bitblt_flags |= perform_rotate270;
    }    
    
    size_t len = _tcslen(str);

    std::set<int> painted;
    int number_painted = 0;
    do
    {
        float advance_x = 0;
        float advance_y = 0;

        //
        // Phase 1:
        //
        // First we attempt to build the cache
        // with as many of the glyphs as possible
        std::vector<int> cache_misses;
        for (size_t i = 0; i < len; ++i )
        {
            int charcode = str[i];
            int flags = 0;
            glyph_t g;
            
            if (painted.find( charcode ) != painted.end())
            {
                // We have already painted this character
                continue;
            }
            
            // If we have run out of space,
            // then don't render any new glyphs
            // until we have locked the ones we need
            if (m_free_cells == 0)
            {
                flags = glyph_no_render;
            }
            
            getGlyph( charcode, g, flags );
            
            if ((g.m_advance_x == 0 && g.m_advance_y == 0))
            {
                // This is a cache miss
                cache_misses.push_back(charcode);
            }
            else
            {
                // Cache hit, so lock it
                lockGlyph(charcode);
            }
        }
        
        //
        // Phase 2:
        //
        // Now load the cache with as many
        // of the misses as possible, this is
        // will result in some of the existing
        // glyphs being ejected from the cache
        for (auto i = cache_misses.begin(); i != cache_misses.end(); ++ i)
        {
            int charcode = *i;
            if (painted.find( charcode ) != painted.end())
            {
                // We have already painted this character
                continue;
            }

            glyph_t g;
            getGlyph( charcode, g, 0 );
            
            if (g.m_advance_x != 0 || g.m_advance_y != 0)
            {
                // Cache hit, so lock it
                lockGlyph(charcode);
            }
        }
        
        //
        // Phase 3:
        //
        // Now render as much as the string as possible until
        // we hit a cache miss.
        //
        for (size_t p = 0; p < len; ++ p )
        {
            glyph_t g;
            int charcode = str[p];
            
            getGlyph( charcode, g, glyph_no_render | glyph_size);
            
            if (g.m_lock)
            {
                // Only paint glyphs that have been locked, so which indicates
                // they are identified as needing painting in the previous 2 loops
                paintGlyph( colour, target, x, y, advance_x, advance_y, g, bitblt_flags );
                painted.insert( charcode );
                ++ number_painted;
            }
            
            advance_x += g.m_advance_x;
            advance_y += g.m_advance_y;
            if (p < len-1)
            {
                advance_x += m_font->get_kerning(str[p], str[p+1]);
            }
        }
        
        //
        // Phase 4:
        //
        // Unlock the glyphs we have been using ready for the next pass
        unlockGlyphs();
    }
    while (number_painted < len);
}

// Measure a string
jett_point CFont::sizeString( const TCHAR * str )
{
    jett_point r;
    
    size_t len = _tcslen(str);
    float advance_x = 0.0f;
    float advance_y = 0.0f;
    for (size_t p = 0; p < len; ++ p )
    {
        glyph_t g;
        getGlyph( str[p], g, glyph_no_render | glyph_size );
        
        advance_x += g.m_advance_x;
        advance_y += g.m_advance_y;
        if (p < len-1)
        {
            advance_x += m_font->get_kerning(str[p], str[p+1]);
        }
        r.y = std::max( r.y, static_cast<float>(g.m_height) );
    }
    

    r.x = advance_x;
    r.y = advance_y;
    return r;
}

