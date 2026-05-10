//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H

#include "freetype_font_sampler.h"
#include "Image.h"


FT_Library g_ft_library = NULL;


freetype_font_sampler::freetype_font_sampler()
{
    m_face = NULL;
    m_anti_alias = false;
    m_cell_height = 0;
    m_cell_width = 0;
	m_block = NULL;
    m_kerning_data = false;
}

freetype_font_sampler::~freetype_font_sampler()
{
	if (m_face)
	{
	    FT_Done_Face( m_face );
	}

	delete m_block;
}

void freetype_font_sampler::destroy()
{
    delete this;
}


// Open a font file
void freetype_font_sampler::openFont( const TCHAR *filename, int width, int height, bool anti_alias )
{
    FT_Error error;
    
    m_anti_alias = anti_alias;

    if (!g_ft_library)
    {
        error = FT_Init_FreeType( &g_ft_library );
        if ( error )
        {
            throw jett_exception(JETT_FREETYPE_FAILURE,error,"Unable initialise freetype library");
        }
    }
        
    // Load the font file
#ifdef UNICODE
	char file_name_ansi[ MAX_PATH ];
	BOOL use_default_char = FALSE;

	if (WideCharToMultiByte( CP_ACP, 0, filename, -1, file_name_ansi, sizeof( file_name_ansi ), " ", &use_default_char ) == 0 
		|| use_default_char)
	{
		// Filename conversion error
        throw jett_exception(JETT_FREETYPE_FAILURE,error,"The font file name cannot be converted to an ANSI string");
	}
    error = FT_New_Face( g_ft_library,
                        file_name_ansi, 0, &m_face );
#else
    error = FT_New_Face( g_ft_library,
                        filename, 0, &m_face );
#endif
    if ( error == FT_Err_Unknown_File_Format )
    {
        throw jett_exception(JETT_FREETYPE_FAILURE,error,"The font file is in an unsupported format");
    }
    else if ( error )
    {
        throw jett_exception(JETT_FREETYPE_FAILURE,error,"The font file cannot be read");
    }
    
    // Set the size of the face
    error = FT_Set_Pixel_Sizes( m_face, width, height );   
    if ( error )
    {
        throw jett_exception(JETT_FREETYPE_FAILURE,error,"Cannot set font size");
    }
    
    // Get the cell size, we add a small pixel margin due to hinting sometimes pushing
    // the glyph bigger than the predicted size
    m_cell_height = (static_cast<int>(m_face->bbox.yMax - m_face->bbox.yMin) >> 5) + 2;
    m_cell_width = (static_cast<int>(m_face->bbox.xMax - m_face->bbox.xMin) >> 5) + 2;

    // Do we have kerning information?
    m_kerning_data = FT_HAS_KERNING( m_face ) != 0;
}


// Open a font from memory
void freetype_font_sampler::openFont( char *block, size_t size, int width, int height, bool anti_alias )
{
    FT_Error error;
    
    m_anti_alias = anti_alias;
	m_block = block;

    if (!g_ft_library)
    {
        error = FT_Init_FreeType( &g_ft_library );
        if ( error )
        {
            throw jett_exception(JETT_FREETYPE_FAILURE,error,"Unable initialise freetype library");
        }
    }
        
    // Load the font from memory
	error = FT_New_Memory_Face( g_ft_library,
                        (const FT_Byte*)block, size, 0, &m_face );
    if ( error == FT_Err_Unknown_File_Format )
    {
        throw jett_exception(JETT_FREETYPE_FAILURE,error,"The font file is in an unsupported format");
    }
    else if ( error )
    {
        throw jett_exception(JETT_FREETYPE_FAILURE,error,"The font file cannot be read");
    }
    
    // Set the size of the face
    error = FT_Set_Pixel_Sizes( m_face, width, height );   
    if ( error )
    {
        throw jett_exception(JETT_FREETYPE_FAILURE,error,"Cannot set font size");
    }
    
    // Get the cell size, we add a small pixel margin due to hinting sometimes pushing
    // the glyph bigger than the predicted size
    m_cell_height = (static_cast<int>(m_face->bbox.yMax - m_face->bbox.yMin) >> 5) + 2;
    m_cell_width = (static_cast<int>(m_face->bbox.xMax - m_face->bbox.xMin) >> 5) + 2;
    
    // Do we have kerning information?
    m_kerning_data = FT_HAS_KERNING( m_face ) != 0;
}

// Set a transform matrix
void freetype_font_sampler::set_matrix( const jett_matrix &matrix )
{
    // Convert the matrix to Freetype format
    FT_Matrix  ft_matrix;
    FT_Vector  ft_delta;
    
    ft_matrix.xx = static_cast<FT_Fixed>(matrix.a * 0x10000L);
    ft_matrix.xy = static_cast<FT_Fixed>(matrix.b * 0x10000L);
    ft_matrix.yx = static_cast<FT_Fixed>(matrix.c * 0x10000L);
    ft_matrix.yy = static_cast<FT_Fixed>(matrix.d * 0x10000L);
    ft_delta.x = static_cast<FT_Pos>(matrix.e * 0x10000L);
    ft_delta.y = static_cast<FT_Pos>(matrix.f * 0x10000L);
    
    // Apply this matrix
    FT_Set_Transform( m_face, &ft_matrix, &ft_delta );
}

static float fixedToFloat( FT_Pos fixed )
{
    return fixed / 64.0f;
}

// Hand over a character (charcode is a utf16 code)
void freetype_font_sampler::get_glyph( int charcode, jett_image& target, glyph_t &glyph )
{
    FT_Error error;
    FT_GlyphSlot  slot = m_face->glyph;
    
    // load glyph image into the slot (erase previous one)
    int flags;
    if (m_anti_alias)
    {
        flags = FT_LOAD_RENDER;
    }
    else
    {
        flags = FT_LOAD_RENDER | FT_LOAD_TARGET_MONO;        
    }
    error = FT_Load_Char( m_face, charcode, flags );
    
    if ( !error )
    {
        
        // Create the target bitmap
        
        // Now copy it over
        if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
        {
            target.copy_bitmap( slot->bitmap.width, slot->bitmap.rows, image_mono, image_mode_gpu_copy, slot->bitmap.buffer, slot->bitmap.pitch, 8, true );
        }
        else if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
        {
            target.copy_bitmap( slot->bitmap.width, slot->bitmap.rows, image_mono, image_mode_gpu_copy, slot->bitmap.buffer, slot->bitmap.pitch, 1, true );
        }
        else
        {
            // Humph, unsupported output bitmap
            target.erase();
        }
        
        glyph.m_width = slot->bitmap.width;
        glyph.m_height = slot->bitmap.rows;
        glyph.m_left = slot->bitmap_left;
        glyph.m_top = slot->bitmap_top;
        glyph.m_advance_x = fixedToFloat(slot->advance.x);
        glyph.m_advance_y = fixedToFloat(slot->advance.y);
    }
    else
    {
        throw jett_exception(JETT_FREETYPE_FAILURE,error,"Cannot render glyph");
    }
}

// Get the kerning for a pair of glyphs
float freetype_font_sampler::get_kerning( int charcode_left, int charcode_right )
{
    if (!m_kerning_data)
    {
        return 0;
    }
    
    FT_Vector  delta;
    
    int a = FT_Get_Char_Index(m_face, charcode_left );
    int b = FT_Get_Char_Index(m_face, charcode_right );
    
    FT_Get_Kerning( m_face, a, b,
                   FT_KERNING_DEFAULT, &delta );
 
    return fixedToFloat( delta.x );
}



// Determine the bounding box for all of the characters
int freetype_font_sampler::get_cell_width()
{
    return m_cell_width;
}

int freetype_font_sampler::get_cell_height()
{
    return m_cell_height;
}
