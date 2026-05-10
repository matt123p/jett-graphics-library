//
//  gdi_font_sampler.cpp
//  GraphicsLibrary
//
//  Created by Matt Pyne on 26/06/2012.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "StdAfx.h"
#include "gdi_font_sampler.h"
#include "Image.h"



gdi_font_sampler::gdi_font_sampler()
{
    m_font = NULL;
    m_cell_height = 0;
    m_cell_width = 0;

	m_bitmap_dc = NULL;
	m_bitmap = NULL;
	m_bits = NULL;

}

gdi_font_sampler::~gdi_font_sampler()
{
	if (m_bitmap)
	{
		DeleteObject( m_bitmap );
	}
}

void gdi_font_sampler::destroy()
{
    delete this;
}


// Open a font file
void gdi_font_sampler::openFont(  HFONT font  )
{
	m_font = font;

	HDC dc = CreateDC(_T("DISPLAY"),NULL,NULL,NULL);
	if (!dc)
	{
		throw jett_exception(JETT_GDI_FAILURE,GetLastError(),"Cannot get font size");
	}
	HFONT old_font = (HFONT)SelectObject( dc, m_font );

	TEXTMETRIC tm;
	if (GetTextMetrics( dc, &tm ) == 0)
	{
		SelectObject( dc, old_font );
		DeleteDC( dc );
		throw jett_exception(JETT_GDI_FAILURE,GetLastError(),"Cannot get font size");
	}

    
    // Get the cell size, we add a small pixel margin due to hinting sometimes pushing
    // the glyph bigger than the predicted size
	m_cell_height = tm.tmHeight;
	m_cell_width = tm.tmMaxCharWidth;

	m_cell_width = ((m_cell_width + 3) / 4) * 4;

	// Create a bitmap to buffer in to
	struct SIBITMAPINFO
	{
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD bmiColor[256];  // exactly 256 colors in the palette
	};
	
	SIBITMAPINFO bi = {0};
	bi.bmiHeader.biSize				= sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth			= m_cell_width;
	bi.bmiHeader.biHeight			= -m_cell_height;
	bi.bmiHeader.biPlanes			= 1;
	bi.bmiHeader.biBitCount			= 8;
	bi.bmiHeader.biCompression		= BI_RGB;
	bi.bmiHeader.biClrUsed			= 256;

	for (int i = 0; i < 256; ++ i)
	{
		bi.bmiColor[ i ].rgbRed			= i;
		bi.bmiColor[ i ].rgbGreen		= i;
		bi.bmiColor[ i ].rgbBlue		= i;
		bi.bmiColor[ i ].rgbReserved	= 0;
	}
	
	// Create the bitmap
	m_bitmap = ::CreateDIBSection( dc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&m_bits, 0, 0 );
	
	// Now get the kerning pairs
	int number_of_pairs = GetKerningPairs( dc, -1, NULL );
	if (number_of_pairs > 0)
	{
		KERNINGPAIR *pairs = new KERNINGPAIR[ number_of_pairs ];
		number_of_pairs = GetKerningPairs( dc, number_of_pairs, pairs );

		// Now populate the map
		for (int i = 0; i < number_of_pairs; ++ i)
		{
			m_kerning_data.insert( pairs[i] );
		}

		delete[] pairs;
	}
	SelectObject( dc, old_font );
	DeleteDC( dc );
}


// Hand over a character (charcode is a utf16 code)
void gdi_font_sampler::get_glyph( int charcode, jett_image& target, glyph_t &glyph )
{
	// Draw the character in to the bitmap
	HDC dc = CreateDC(_T("DISPLAY"),NULL,NULL,NULL);
	if (!dc)
	{
		throw jett_exception(JETT_GDI_FAILURE,GetLastError(),"Cannot render glyph");
	}

	HDC mem_dc = CreateCompatibleDC( dc );
	if (!mem_dc)
	{
		DeleteDC( dc );
		throw jett_exception(JETT_GDI_FAILURE,GetLastError(),"Cannot render glyph");
	}

	HBITMAP old_bitmap = (HBITMAP)SelectObject( mem_dc, m_bitmap );
	HFONT old_font = (HFONT)SelectObject( mem_dc, m_font );

	// Erase the bitmap
	memset( m_bits, 255, m_cell_width * m_cell_height );

	// Render the glyph
	SetTextAlign( mem_dc, TA_LEFT | TA_TOP );
	SetTextColor( mem_dc, RGB(0,0,0) );
	SetBkColor( mem_dc, RGB(255,255,255) );
	TCHAR buffer[1];
	buffer[0] = charcode;
	TextOut( mem_dc, 0,0, buffer, 1 ); 

	// The information about the char
	SIZE size;
	GetTextExtentPoint32( mem_dc, buffer, 1, &size ); 

	// Delete the DCs
	SelectObject( mem_dc, old_font );
	SelectObject( mem_dc, old_bitmap );
	DeleteDC( mem_dc );
	DeleteDC( dc );

	// Now set the data..
	target.copy_bitmap( size.cx, size.cy, image_mono, image_mode_gpu_copy, m_bits, m_cell_width, 8, false );

    glyph.m_width = size.cx;
    glyph.m_height = size.cy;
    glyph.m_left = 0;
    glyph.m_top = size.cy;    
    glyph.m_advance_x = static_cast<float>(size.cx);
    glyph.m_advance_y = 0;
}


// Set a transform matrix
void gdi_font_sampler::set_matrix( const jett_matrix &matrix )
{
    if (!matrix.is_ident())
    {
        throw jett_exception( JETT_UNSUPPORTED_FEATURE, 0, "The GDI font engine does not support matrix operations");
    }
}


// Get the kerning for a pair of glyphs
float gdi_font_sampler::get_kerning( int charcode_left, int charcode_right )
{
	KERNINGPAIR p;
	p.wFirst = charcode_left;
	p.wSecond = charcode_right;
	kerningCollection::iterator i = m_kerning_data.find( p );
	if (i != m_kerning_data.end())
	{
		return static_cast<float>(i->iKernAmount);
	}
	return 0;
}

// Determine the bounding box for all of the characters
int gdi_font_sampler::get_cell_width()
{
    return m_cell_width;
}

int gdi_font_sampler::get_cell_height()
{
    return m_cell_height;
}
