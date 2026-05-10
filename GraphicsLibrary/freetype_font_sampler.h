//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_freetype_font_sampler_h
#define GraphicsLibrary_freetype_font_sampler_h

#include "Font.h"

struct FT_FaceRec_;
typedef struct FT_FaceRec_*  FT_Face;


class freetype_font_sampler : public font_sampler
{
private:
    FT_Face     m_face;
    bool        m_anti_alias;
    
    int         m_cell_width;
    int         m_cell_height;
    
    // Do we have a kerning table?
    bool        m_kerning_data;

	// If this was opened from memory, here
	// is the memory block (which we will delete on exit)
	char*		m_block;
    
public:
    freetype_font_sampler();
    ~freetype_font_sampler();
    
    // Open a font file
    // width & height are in pixels
    void openFont( const TCHAR *filename, int width, int height, bool anti_alias );

    // Open a font from a memory block
    // width & height are in pixels
	void openFont( char *block, size_t size, int width, int height, bool anti_alias );
    
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
