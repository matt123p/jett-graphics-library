//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#ifndef GraphicsLibrary_FontCache_h
#define GraphicsLibrary_FontCache_h

#include "jett.h"
#include "Image.h"

#include <map>

class CTransform;
class CImage;
class CGPUProcessor;

#define glyph_no_render     1           // If the glyph is not in the cache, do not render it
#define glyph_size          2           // We are only interested in the size of the glyph

/*
 * A font glyph
 */
struct glyph_t
{
    // Which cell is this glyph held in?
    int     m_cell;
    
    // The size of the glyph
    int     m_width;
    int     m_height;
    size_t  m_stride;
    
    // The attributes of the glyph
    int     m_left;
    int     m_top;
    float   m_advance_x;
    float   m_advance_y;
    
    // indicates the gylph has been locked
    bool     m_lock;
    
    // The number of times this glyph has been
    // used, since insertion, to indicate to the
    // cache how to discard glyphs
    int     m_uses;
};


/*
 * The user-defined font function
 */
class font_sampler
{
public:
    
    // destructor
    virtual ~font_sampler() { }
    
    // Set a transform matrix
    virtual void set_matrix( const jett_matrix &matrix ) = 0;

    // Hand over a character (charcode is a utf16 code)
    // This function might be called by more than a single thread, the
    // function must be re-entrant (even if this means a global lock)
    virtual void get_glyph( int charcode, jett_image& target, glyph_t &glyph ) = 0;
    
    // Get the kerning for a pair of glyphs
    virtual float get_kerning( int charcode_left, int charcode_right ) = 0;

    // Determine the bounding box for all of the characters
    virtual int get_cell_width() = 0;
    virtual int get_cell_height() = 0;    
    
    // This is called by the internals - do not destroy the
    // font yourself
    virtual void destroy() = 0;
    
};

class CFont
{
private:
    // Here is the OpenCL device we are using
    CProcessor*     m_cl;

    // The Image memory block that currently
    // holds this cache
    CImage        m_cache;
        
    // The cell size of the font
    int          m_cell_width;
    int          m_cell_height;
    
    // The matrix we are using to draw this font
    jett_matrix    m_matrix;
    
    // Here is the font we are connected to
    font_sampler* m_font;
    
    // Here is the user requested size of the font cache
    int           m_cache_size;
    
    // This is the number of free cells
    int            m_free_cells;
     
    // Here is the list of characters in
    // the cache and their location
    typedef std::map<int,glyph_t>   glyphCollection;
    glyphCollection m_glyphs;
    
    // Drop the entire cache from memory
    void clear( bool keep_sampler );
    
    // Read in the font data and create the font cache etc..
    void setupFont();

    // Get a glyph, and if it's not in the
    // cache, put it there...
    void getGlyph( int charcode, glyph_t& glyph, int flags );
    
    // Lock the glyph in the cache to stop it being
    // discarded by a another glyph
    void lockGlyph( int charcode );
    
    // Unlock all the glyphs
    void unlockGlyphs();
        
    
    // Paint a glyph to an image at a specific position
    void paintGlyph( unsigned int colour, CImage& target, int x, int y, float advance_x, float advance_y, const glyph_t& glyph, int bitblt_flags );
        
public:
    CFont();
    ~CFont();
    
    // Attach this cache to a font
    void attachFont( CProcessor* cl, font_sampler* font, int cache_size );
    
    // Write out a string to a specific location on an image
    void paintString( unsigned int colour, CImage &target, int x, int y, const TCHAR * str, int flags );

    // Measure a string
    jett_point sizeString( const TCHAR * str );
    
    // Distort this font by using a matrix
    void setMatrix( const jett_matrix& matrix );
};

#endif
