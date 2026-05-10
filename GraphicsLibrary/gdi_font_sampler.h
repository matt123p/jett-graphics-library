//
//  gdi_font_sampler.h
//  GraphicsLibrary
//
//  Created by Matt Pyne on 26/06/2012.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef GraphicsLibrary_gdi_font_sampler_h
#define GraphicsLibrary_gdi_font_sampler_h

#include "Font.h"
#include <set>

inline bool operator<( const KERNINGPAIR &a, const KERNINGPAIR &b )
{
	if (a.wFirst == b.wFirst)
	{
		return a.wSecond < b.wSecond;
	}
	return a.wFirst < b.wFirst;
}

class gdi_font_sampler : public font_sampler
{
private:
    HFONT	    m_font;
    
	// The cell size
    int         m_cell_width;
    int         m_cell_height;

	// Here is the bitmap we are drawing to
	HDC				m_bitmap_dc;
	HBITMAP			m_bitmap;
	unsigned char *	m_bits;

	typedef std::set<KERNINGPAIR> kerningCollection;
	kerningCollection	m_kerning_data;
    
public:
    gdi_font_sampler();
    ~gdi_font_sampler();
    
    // Open a font file
    void openFont( HFONT font );
    
	// Set a transform matrix
    virtual void set_matrix( const jett_matrix &matrix );

	// Hand over a character (charcode is a utf16 code)
    virtual void get_glyph( int charcode, jett_image& target, glyph_t &glyph );
 
	// Get the kerning for a pair of glyphs
    virtual float get_kerning( int charcode_left, int charcode_right );

    // Determine the bounding box for all of the characters
    virtual int get_cell_width();
    virtual int get_cell_height();

    
    // This is called by the internals - do not destroy the
    // font yourself
    virtual void destroy();
};



#endif
