//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#ifndef GraphicsLibrary_Processor_h
#define GraphicsLibrary_Processor_h

#include "jett.h"

class CGPUProcessor;
class CTransform;
class CImage;
struct jett_point;
class COrderedScreenCollection;
class CLinearizationCollection;

typedef float cl_float;

//////////////////////////////////////////////////////////////////
//
// Bitblt Flags
//

#define     perform_resize_simple   1
#define     perform_resize_complx   2
#define     perform_rotate90        4
#define     perform_rotate180       8
#define     perform_rotate270       16
#define     perform_no_cm           32
#define     flip_src_components     64
#define     flip_dst_components     128
#define     perform_white_is_trans  256
#define		perform_matrix			512


// This structure is some of the parameters passed to the GPU
// bitblt function.  This makes it easier to make the different
// overloaded versions of the function
struct bitblt_params
{
    cl_int flags;
    cl_int src_x;
    cl_int src_y;
    
    cl_int dst_x1;
    cl_int dst_y1;
    cl_int dst_x2;
    cl_int dst_y2;

    // Only used by copy (not copy_glyph or copy_matrix)
    cl_float scale_src_x;
    cl_float scale_src_y;

    // Only used by copy_matrix
	jett_matrix	matrix;
    cl_int src_x2;
    cl_int src_y2;

    bitblt_params()
    {
        flags = 0;
        src_x = 0;
        src_y = 0;
        scale_src_x = 1.0f;
        scale_src_y = 1.0f;
        dst_x1 = 0;
        dst_y1 = 0;
        dst_x2 = 0;
        dst_y2 = 0;
        src_x2 = 0;
        src_y2 = 0;
    }
};


// This class is the wrapper for pre-computed colour management
// transform tables
class CBitblt 
{
public:
    CBitblt();
    virtual ~CBitblt();
    
    // Convert a bitmap using the GPU
    virtual void copy( CTransform *trans, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap ) = 0;
	virtual void copy_glyph( unsigned int colour, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap ) = 0;
	virtual void copy_matrix( CTransform *trans, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap ) = 0;

	// Utility functions
	static bool clip( bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap, bool ignore_src_dims = false ); 
};


// This class is the wrapper for the polygon fill routines
//
class CPolygon 
{
public:
    virtual ~CPolygon() { }
    
    virtual void fill( CImage *dst_image, unsigned char *colour, jett_point *points, int n ) = 0;
    
};

class CLine
{
public:
    virtual ~CLine() { }

    //
    // Draw a series of lines
    //
    virtual void lines( CImage *dst_image, unsigned char *colour, float width, jett_point *points, int n, bool close, int flags ) = 0;
};

// This class is the wrapper for the dither routines
class CDither
{
public:
	virtual ~CDither(void) {}

	virtual void dither( CImage *src_image, COrderedScreenCollection& screens, CLinearizationCollection* linearization, int scale ) = 0;
	virtual void dither( CImage *src_image, CImage** dst_images, COrderedScreenCollection& screens, CLinearizationCollection* linearization, int scale ) = 0;
};

// This help function converts from ASCII to UNICODE
#ifndef __MACH__
typedef std::basic_string<TCHAR> wString;
extern wString to_unicode( const char *txt );
#endif

class CProcessor
{
public:
    virtual ~CProcessor() { }
    // Find and open the OpenCL device.
    // This throws an exception if there is a problem
    virtual void Open() = 0;
    
    // Access the bitblt kernels
    virtual CBitblt* bitblt( size_t index ) = 0;
    
    // Access the polygon kernels
    virtual CPolygon* polygon(size_t index) = 0;
                
	// Access the dither kernels
	virtual CDither* dither() = 0;

    // Wait for a kernel execution to finish
    virtual void finish() = 0;
    
    // Do we have an attached CLDevice (i.e. a GPU?)
    virtual CGPUProcessor*  get_cl_device() = 0;

	// Push an image in to the GPU
	virtual void cache_image( CImage* image, bool read_only ) { } 
};


#endif
