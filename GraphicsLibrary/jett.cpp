//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#include "StdAfx.h"
#include "Image.h"
#include "GPUProcessor.h"
#include "CPUProcessor.h"
#include "jett.h"
#include "transform.h"
#include "lcms_transform_sampler.h"
#include "k_only_transform_sampler.h"
#include "Font.h"
#include "Line.h"
#include "freetype_font_sampler.h"
#ifndef __MACH__
#include "gdi_font_sampler.h"
#endif
#include "OrderedScreenCollection.h"
#include "CPUDither.h"
#include "LinearizationCollection.h"


#ifndef __MACH__
wString to_unicode( const char *txt )
{
	wString r;
	int size_in = strlen( txt );
	int size_out = MultiByteToWideChar( CP_ACP, 0, txt, size_in, NULL, 0 );
	r.resize( size_out );
	MultiByteToWideChar( CP_ACP, 0, txt, size_in, &r.at(0), size_out );
	return r;
}

#endif


/*!
 * \brief Construct a new graphics library entry point.
 *
 * Use this object as the entry point in to the graphics library.  If you wish to use both the CPU and OpenCL 
 * accleration then you will require two of these objects.  An application would not normally have more than two
 * of these objects.  Before this object can be used init() must be called.
 *
 * \sa init()
 */
jett::jett()
{
    m_cl = NULL;

}

jett::~jett()
{
    delete m_cl;
}

/*!
 * \brief Initialise the library prior to use.
 *
 * \param use_gpu Set to true if you wish to use OpenCL acceleration or false to use the CPU.
 *
 * This function is used to initialise the library.  It should only be called once immediately after construction and before
 * any other function of the library is called.  
 *
 * A two stage initialisation process is used because it is possible for this call to throw an exception - for example 
 * when there is no OpenCL support.
 *
 */
void jett::init( bool use_gpu )
{
    if (use_gpu)
    {
        m_cl = new CGPUProcessor;
    }
    else
    {
        m_cl = new CCPUProcessor;
    }

	// Open our connection to OpenCL (if we are using
    // openCL)
    m_cl->Open();
}


/** @name Painting functions
 *  These functions are used to draw on images
 */
///@{

/*!
 * \brief Copy an image or part of an image to another image.
 *
 * \param trans         The colour transform profile.  This can be NULL if source and destination images have compatible colourspaces (e.g. RGB->RGB).
 * \param src_bitmap    The bitmap the image is copied from
 * \param src_x1        The left edge of the source rectangle
 * \param src_y1        The top edge of the source rectangle
 * \param src_width     The width of the source rectangle
 * \param src_height    The height of the source rectangle
 * \param dst_bitmap    The bitmap that the image is copied to
 * \param dst_x1        The left edge of the destination rectangle
 * \param dst_y1        The top edge of the destination rectangle
 * \param dst_width     The width of the destination rectangle
 * \param dst_height    The height of the destination rectangle
 * \param flags         OR together zero or more \ref bitblt_flags 
 *
 * \sa bitblt_flags
 *
 * This function copies all or part of an image from one image to another.  At the same time it also performs
 * colour transformation (e.g. RGB -> CMYK), resizing and rotation as required.
 *
 * If the source or destination bitmap is not in the correct memory (e.g. if it not on the Graphics Card but is
 * instead in the CPU main memory) then it will be automatically moved or copied as determined by the image's 
 * image_mode_t.
 *
 * If the source and destination images are compatible colour spaces (e.g. RGB->RGB or RGBA->RGB) then trans can be NULL.  However, if they are not compatible (e.g. RGB->CMYK) then a trans must be specified.
 *
 * If the source or destination rectangle extend outside of the size of their image then the copy is automatically clipped to the size of the image.
 *
 * If the source and desintation rectangles are the same size then the copy is automatically accerated and will be faster.  If they are not the same then either a nearest neighbour or bi-cubic interpolation will be used as specified by the flags.
 *
 * A 90, 180 or 270 degree rotation can be performed and is set by using the flags.
 *
 * If the source image is RGBA then the "A" or alpha component can be used.  This will cause the image to not obscure the image in the destination, but instead the two will be blended together, controlled by the Alpha channel.  This mode must be turned on using the flags otherwise the alpha channel will be ignored.
 *
 * If the source image is Monochrome, then it is possible to use the white background pixels as transparent.  This is controlled by the flags.
 *
 * \sa \ref page_images
 *
 */
void jett::bitblt( jett_transform trans, jett_image& src_bitmap,
                 int src_x1, int src_y1, int src_width, int src_height,
                 jett_image& dst_bitmap,
                 int dst_x1, int dst_y1, int dst_width, int dst_height, int flags)
{
    bitblt_params params;
    
    // The start/end co-ordinates
    params.src_x = src_x1;
    params.src_y = src_y1;
    params.dst_x1 = dst_x1;
    params.dst_y1 = dst_y1;
    params.dst_x2 = dst_x1 + dst_width - 1;
    params.dst_y2 = dst_y1 + dst_height - 1;
        
	// Test the flags
	if ((flags & bitblt_rotate_90) != 0)
	{
		std::swap( dst_width, dst_height );
		params.flags |= perform_rotate90;
	}
	else if ((flags & bitblt_rotate_180) != 0)
	{
		params.flags |= perform_rotate180;
	}
	else if ((flags & bitblt_rotate_270) != 0)
	{
		std::swap( dst_width, dst_height );
		params.flags |= perform_rotate270;
	}
    
    if ((flags & bitblt_white_is_transparent) != 0)
    {
        if (src_bitmap.getType() != image_mono)
        {
            // Unsupport source components
            throw jett_exception( JETT_INVALID_ARGUMENT, 0, "Invalid input image, white_is_transparent only supported with Mono8 images" );
        }
        params.flags |= perform_white_is_trans;
    }
    
    size_t src = src_bitmap.getComponents();
    if ((flags & bitblt_use_alpha) != 0)
    {
        if (src_bitmap.getType() != image_rgba && src_bitmap.getType() != image_bgra)
        {
            // Unsupport source components
            throw jett_exception( JETT_INVALID_ARGUMENT, 0, "Invalid input image, alpha only supported with RGBA images" );
        }
        src = 0;
    }
    else if (src != 1 && src != 3 && src != 4)
    {
        // Unsupport source components
        throw jett_exception( JETT_INVALID_ARGUMENT, 0, "Invalid input image, unsupported number of colours" );
    }
    
    // Scaling factors (for the source)
    if ( dst_width != src_width || dst_height != src_height)
    {
        params.scale_src_x = static_cast<float>( src_width ) / static_cast<float>( dst_width );
        params.scale_src_y = static_cast<float>( src_height ) / static_cast<float>( dst_height );
        
		// How did the user want this resize performed?
		if ((flags & bitblt_cubic_scaling) != 0)
		{
	        params.flags |= perform_resize_complx;
		}
		else
		{
	        params.flags |= perform_resize_simple;
		}
    }
    
	if (!CBitblt::clip( params, src_bitmap, dst_bitmap ))
	{
		return;
	}
 
        
    // Call the generic function
	m_cl->bitblt( src )->copy( trans, params, src_bitmap, dst_bitmap );
}


/*!
 * \brief Copy an entire image image to another image.
 *
 * \param trans         The colour transform profile.  This can be NULL if source and destination images have compatible colourspaces (e.g. RGB->RGB).
 * \param src_bitmap    The bitmap the image is copied from
 * \param dst_bitmap    The bitmap that the image is copied to
 * \param dst_x1        The left edge of the destination rectangle
 * \param dst_y1        The top edge of the destination rectangle
 * \param flags         OR together zero or more \ref bitblt_flags
 *
 * \sa bitblt_flags
 *
 * This function copies all of an image from one image to another.  At the same time it also performs
 * colour transformation (e.g. RGB -> CMYK), resizing and rotation as required.
 *
 * If the source or destination bitmap is not in the correct memory (e.g. if it not on the Graphics Card but is
 * instead in the CPU main memory) then it will be automatically moved or copied as determined by the image's
 * image_mode_t.
 *
 * If the source and destination images are compatible colour spaces (e.g. RGB->RGB or RGBA->RGB) then trans can be NULL.  However, if they are not compatible (e.g. RGB->CMYK) then a trans must be specified.
 *
 * If the source or destination rectangle extend outside of the size of their image then the copy is automatically clipped to the size of the image.
 *
 * A 90, 180 or 270 degree rotation can be performed and is set by using the flags.
 *
 * If the source image is RGBA then the "A" or alpha component can be used.  This will cause the image to not obscure the image in the destination, but instead the two will be blended together, controlled by the Alpha channel.  This mode must be turned on using the flags otherwise the alpha channel will be ignored.
 *
 * If the source image is Monochrome, then it is possible to use the white background pixels as transparent.  This is controlled by the flags.
 *
 * \sa \ref page_images
 *
 */
void jett::bitblt( jett_transform trans, jett_image& src_bitmap,
            jett_image& dst_bitmap, int dst_x1, int dst_y1, int flags )
{
    bitblt( trans, src_bitmap, 0,0, src_bitmap.getWidth(), src_bitmap.getHeight(),
           dst_bitmap, dst_x1, dst_y1, src_bitmap.getWidth(), src_bitmap.getHeight(), flags );
}

/*!
 * \brief Copy an image or part of an image to another image without resizing
 *
 * \param trans         The colour transform profile.  This can be NULL if source and destination images have compatible colourspaces (e.g. RGB->RGB).
 * \param src_bitmap    The bitmap the image is copied from
 * \param src_x1        The left edge of the source rectangle
 * \param src_y1        The top edge of the source rectangle
 * \param src_width     The width of the source rectangle
 * \param src_height    The height of the source rectangle
 * \param dst_bitmap    The bitmap that the image is copied to
 * \param dst_x1        The left edge of the destination rectangle
 * \param dst_y1        The top edge of the destination rectangle
 * \param flags         OR together zero or more \ref bitblt_flags
 *
 * \sa bitblt_flags
 *
 * This function copies all or part of an image from one image to another.  At the same time it also performs
 * colour transformation (e.g. RGB -> CMYK), resizing and rotation as required.
 *
 * If the source or destination bitmap is not in the correct memory (e.g. if it not on the Graphics Card but is
 * instead in the CPU main memory) then it will be automatically moved or copied as determined by the image's
 * image_mode_t.
 *
 * If the source and destination images are compatible colour spaces (e.g. RGB->RGB or RGBA->RGB) then trans can be NULL.  However, if they are not compatible (e.g. RGB->CMYK) then a trans must be specified.
 *
 * If the source or destination rectangle extend outside of the size of their image then the copy is automatically clipped to the size of the image.
 *
 * A 90, 180 or 270 degree rotation can be performed and is set by using the flags.
 *
 * If the source image is RGBA then the "A" or alpha component can be used.  This will cause the image to not obscure the image in the destination, but instead the two will be blended together, controlled by the Alpha channel.  This mode must be turned on using the flags otherwise the alpha channel will be ignored.
 *
 * If the source image is Monochrome, then it is possible to use the white background pixels as transparent.  This is controlled by the flags.
 *
 * \sa \ref page_images
 *
 */
void jett::bitblt( jett_transform trans, jett_image& src_bitmap,
            int src_x1, int src_y1, int src_width, int src_height,
            jett_image& dst_bitmap, int dst_x1, int dst_y1, int flags )
{
    bitblt( trans, src_bitmap, src_x1,src_y1, src_width, src_height,
           dst_bitmap, dst_x1, dst_y1, src_width, src_height, flags );    
}


/*!
 * \brief Copy an image or part of an image to another image.
 *
 * \param trans         The colour transform profile.  This can be NULL if source and destination images have compatible colourspaces (e.g. RGB->RGB).
 * \param src_bitmap    The bitmap the image is copied from
 * \param src_x1        The left edge of the source rectangle
 * \param src_y1        The top edge of the source rectangle
 * \param src_width     The width of the source rectangle
 * \param src_height    The height of the source rectangle
 * \param dst_bitmap    The bitmap that the image is copied to
 * \param matrix        The matrix to be applied to the source rectangle to get the destination rectangle
 * \param flags         OR together zero or more \ref bitblt_flags 
 *
 * \sa bitblt_flags
 *
 * This function copies all or part of an image from one image to another.  At the same time it also performs
 * colour transformation (e.g. RGB -> CMYK), resizing and rotation as required.
 *
 * If the source or destination bitmap is not in the correct memory (e.g. if it not on the Graphics Card but is
 * instead in the CPU main memory) then it will be automatically moved or copied as determined by the image's 
 * image_mode_t.
 *
 * If the source and destination images are compatible colour spaces (e.g. RGB->RGB or RGBA->RGB) then trans can be NULL.  However, if they are not compatible (e.g. RGB->CMYK) then a trans must be specified.
 *
 * If the source or destination rectangle extend outside of the size of their image then the copy is automatically clipped to the size of the image.
 *
 * If the source and desintation rectangles are the same size then the copy is automatically accerated and will be faster.  If they are not the same then either a nearest neighbour or bi-cubic interpolation will be used as specified by the flags.
 *
 * A any rotation can be performed by using the matrix.
 *
 * If the source image is RGBA then the "A" or alpha component can be used.  This will cause the image to not obscure the image in the destination, but instead the two will be blended together, controlled by the Alpha channel.  This mode must be turned on using the flags otherwise the alpha channel will be ignored.
 *
 * If the source image is Monochrome, then it is possible to use the white background pixels as transparent.  This is controlled by the flags.
 *
 * \sa \ref page_images
 *
 */
void jett::bitblt( jett_transform trans, jett_image& src_bitmap,
            int src_x1, int src_y1, int src_width, int src_height,
            jett_image& dst_bitmap,
            const jett_matrix& matrix, int flags )
{
    bitblt_params params;
    
    // The start/end co-ordinates
    params.src_x = src_x1;
    params.src_y = src_y1;
    params.src_x2 = src_x1 + src_width - 1;
    params.src_y2 = src_y1 + src_height - 1;
    params.matrix = matrix.invert();
	params.flags = perform_matrix;
        
	// Test the flags    
    if ((flags & bitblt_white_is_transparent) != 0)
    {
        if (src_bitmap.getType() != image_mono)
        {
            // Unsupport source components
            throw jett_exception( JETT_INVALID_ARGUMENT, 0, "Invalid input image, white_is_transparent only supported with Mono8 images" );
        }
        params.flags |= perform_white_is_trans;
    }
    
    size_t src = src_bitmap.getComponents();
    if ((flags & bitblt_use_alpha) != 0)
    {
        if (src_bitmap.getType() != image_rgba && src_bitmap.getType() != image_bgra)
        {
            // Unsupport source components
            throw jett_exception( JETT_INVALID_ARGUMENT, 0, "Invalid input image, alpha only supported with RGBA images" );
        }
        src = 0;
    }
    else if (src != 1 && src != 3 && src != 4)
    {
        // Unsupport source components
        throw jett_exception( JETT_INVALID_ARGUMENT, 0, "Invalid input image, unsupported number of colours" );
    }
    
	// How did the user want this resize performed?
	if ((flags & bitblt_cubic_scaling) != 0)
	{
	    params.flags |= perform_resize_complx;
	}
	else
	{
	    params.flags |= perform_resize_simple;
	}
    
    // Now determine the output area
    jett_point a = matrix.apply( jett_point( src_x1, src_y1 ));
    jett_point c = matrix.apply( jett_point( src_x1 + src_width, src_y1 ));
    jett_point b = matrix.apply( jett_point( src_x1 + src_width, src_y1 + src_height ));
    jett_point d = matrix.apply( jett_point( src_x1, src_y1 + src_height ));
    params.dst_x1 = static_cast<int>(std::min(a.x,std::min(b.x,std::min(c.x,d.x))));
    params.dst_y1 = static_cast<int>(std::min(a.y,std::min(b.y,std::min(c.y,d.y))));
    params.dst_x2 = static_cast<int>(std::max(a.x,std::max(b.x,std::max(c.x,d.x))));
    params.dst_y2 = static_cast<int>(std::max(a.y,std::max(b.y,std::max(c.y,d.y))));
    
    
    //
	// Get the clipping rectangle
	//
	int clip_x1, clip_y1, clip_x2, clip_y2;
	((CImage*)dst_bitmap)->clipping( clip_x1, clip_y1, clip_x2, clip_y2 );

    if (params.dst_x1 < clip_x1)
    {
        params.dst_x1 = clip_x1;
    }
    if (params.dst_x1 > clip_x2)
    {
        params.dst_x1 = clip_x2;
    }

    if (params.dst_x2 < clip_x1)
    {
        params.dst_x2 = clip_x1;
    }
    if (params.dst_x2 > clip_x2)
    {
        params.dst_x2 = clip_x2;
    }
    
    if (params.dst_y1 < clip_y1)
    {
        params.dst_y1 = clip_y1;
    }
    if (params.dst_y1 > clip_y2)
    {
        params.dst_y1 = clip_y2;
    }

    if (params.dst_y2 < clip_y1)
    {
        params.dst_y2 = clip_y1;
    }
    if (params.dst_y2 > clip_y2)
    {
        params.dst_y2 = clip_y2;
    }

    // Call the generic function
    m_cl->bitblt( src )->copy_matrix( trans, params, src_bitmap, dst_bitmap );
}

/*!
 * \brief Draw a filled polygon
 *
 * \param dst_bitmap    The image that the polygon will be drawn in to
 * \param colour        The colour of the polygon
 * \param points        The points that make up the verticies of the polygon
 * \param n             The number of verticies in the "points" array
 * \param flags         The drawing options
 *
 * \sa polygon_flags
 *
 * This function draws a filled polygon.  Polygons are drawn a single colour.  They can be optionally anti-aliased.
 * Anti-aliasing a polygon does cause it to be drawn significantly slower than without.
 *
 * \sa \ref page_polygons
 */
void jett::polygon( jett_image& dst_bitmap, unsigned char *colour, jett_point *points, int n, int flags )
{
    int j = 0;
    if ((flags & polygon_best) != 0)
    {
        j = 1;
    }
    
    m_cl->polygon(j)->fill(dst_bitmap, colour, points, n );
}


/*!
 * \brief Draw a line
 *
 * \param dst_bitmap    The image that the lines will be drawn in to
 * \param colour        The colour of the lines
 * \param width         The width of the lines
 * \param points        The points that make up the lines
 * \param n             The number of entries in the "points" array
 * \param close         Complete the shape by drawing a line from the last point to the first point
 * \param flags         The drawing options
 *
 * \sa line_flags
 *
 * This function draws a set of joined lines. Lines are drawn a single colour.  They can be optionally anti-aliased.
 * Anti-aliasing a lines does cause it to be drawn significantly slower than without.
 *
 * \sa \ref page_lines
 *
 */
void jett::lines( jett_image& dst_bitmap, unsigned char *colour, float width, jett_point *points, int n, bool close, int flags )
{
    int j = 0;
    if ((flags & line_best) != 0)
    {
        j = 1;
    }
    
    CLineToPoly l;
    l.paint(m_cl->polygon(j), dst_bitmap, colour, width, points, n, close, flags);
}



/*!
 * \brief Draw a rectangle
 *
 * \param dst_bitmap    The image that the lines will be drawn in to
 * \param colour        The colour of the lines
 * \param dst_x1        The left edge of the rectangle
 * \param dst_y1        The top edge of the rectangle
 * \param dst_width     The width of the rectangle
 * \param dst_height    The height of the rectangle
 *
 * This function draws a filled rectanlge is a single solid colour.
 *
 * \sa \ref page_polygons
 *
 */
void jett::rectangle( jett_image& dst_bitmap, unsigned char *colour, int dst_x1, int dst_y1, int dst_width, int dst_height )
{
    jett_point rect[4];
    rect[0] = jett_point( dst_x1, dst_y1 );
    rect[1] = jett_point( dst_x1 + dst_width, dst_y1 );
    rect[2] = jett_point( dst_x1 + dst_width, dst_y1 + dst_height );
    rect[3] = jett_point( dst_x1, dst_y1 + dst_height );
    
    polygon(dst_bitmap, colour, rect, 4, polygon_fast );
}


/*!
 * \brief Draw text
 *
 * \param f             The font that the text will be drawn with
 * \param colour        The colour to paint the font
 * \param dst_bitmap    The bitmap the text will be drawn on
 * \param x             The co-ordinate of left edge of the text
 * \param y             The co-ordinate of baseline of the text
 * \param str           The text to be drawn
 * \param flags         The flags for this drawing.
 *
 * \sa string_flags
 *
 * This function draws text.  Before this function can be called the font to draw with must be created.
 *
 * Text can be drawn anti-aliased.  This is an option specified when the font is created.
 *
 * Text can be drawn at 0, 90, 180 or 270 degrees to the normal.
 *
 * \sa \ref page_text
 *
 */
void jett::text( jett_font f, const unsigned char* colour, jett_image& dst_bitmap, int x, int y, const TCHAR * str, int flags )
{
	// Build the colour
	int col = 0;
	for (int i = 0; i < dst_bitmap.getComponents(); ++ i)
	{
		col |= colour[i] << (i*8);
	}
	f->paintString( col, *dst_bitmap, x, y, str, flags );
}

#ifndef __MACH__
void jett::text( jett_font f, const unsigned char* colour, jett_image& dst_bitmap, int x, int y, const char * str, int flags )
{
	text( f, colour, dst_bitmap, x, y, to_unicode(str).c_str(), flags );
}
#endif

/*!
 * \brief Find out the size of text
 *
 * \param f             The font that the text will be measured with
 * \param str           The text to be measured
 * \param flags         The flags for this drawing.
 *
 * \sa string_flags
 *
 * This function draws text.  Before this function can be called the font to draw with must be created.
 *
 * Text can be drawn anti-aliased.  This is an option specified when the font is created.
 *
 * Text can be measured at 0, 90, 180 or 270 degrees to the normal.
 *
 * \sa \ref page_text
 *
 */
jett_point jett::size_text( jett_font f, const TCHAR * str )
{
    return f->sizeString( str );
}

#ifndef __MACH__
jett_point jett::size_text( jett_font f, const char * str )
{
    return f->sizeString( to_unicode(str).c_str() );
}
#endif

/*!
 * \brief Finish all graphics operations
 *
 * Any drawing operation may continue after the function has returned, this is particulary true for the GPU.  Calling this
 * function ensures all outstanding graphics operations have completed.  This function should be called before the bitmap
 * is accessed by a non-graphics library function.
 *
 */
void jett::flush()
{
    m_cl->finish();
}


///@}


/** @name Linearization
 *  These functions are used to create a linearization curves for use
 *  with the dithering functions.
 */
///@{



/*!
 * \brief Create an empty linearization set
 *
 * \sa page_linearization
 *
 * This creates an empty linearization set.  You must call import_linearization for each of the colours
 * in the output.  For example form a CMYK image you must call import_linearization 4 times for each
 * of the C, M, Y and K colours.
 *
 * When you have finished with the linearization set you must call destroy_linearization.
 *
 * \sa destroy_linearization()
 *
 */
jett_linearization jett::create_linearization()
{
    return new CLinearizationCollection;
}

/*!
 * \brief Create a linearization curve from measurement data
 *
 * \param lin           The linearization set to add this curve to
 * \param curve         Points of the curve to import
 * \param points        The number of points in "curve"
 * \param options       How the data is to be interpreted
 *
 * \sa page_linearization
 *
 * This creates a linearization curve for use with the dither functions.
 *
 * When you have finished with a linearization call build_linearization to release the resources
 * associated with it.
 *
 * \sa destroy_linearization()
 *
 */
void jett::import_linearization( jett_linearization lin, jett_point* curve, size_t points, int options )
{
    CLinearization l;
    l.Import(curve, points, options );
    lin->push_back(l);
}


/*!
 * \brief Apply a linearization to an image in place
 *
 * \param src_image				The source image to dither and separate
 * \param screens				The screens for the dithering (one for each colour)
 * \param scale					Multiply the output pixels by this value.  If left at zero, then the output will be full-scale (i.e. 0-255).
 *
 * This function takes an image and linearizes it.  If the image also requires dithering, then the dither
 * functions can be used instead which dithers and linearizes the image.
 *
 * There must be the same number of linearizations in the linearization set object as there are
 * components in the image, that is 3 for an RGB image and 4 for a CMYK image.
 *
 * \sa \ref page_linearization
 */
void jett::linearize( jett_image& src_image, jett_linearization linearization )
{
	// TODO: 
}


/*!
 * \brief Delete a linearization
 *
 * \param lin   The linearization to destroy
 *
 * When you have finished with a linearization call the this function to release
 * the resources it is using.
 *
 */
void jett::destroy_linearization( jett_linearization lin )
{
    delete lin;
}

///@}

/** @name Colour Transform
 *  These functions are used to create a colour transform from ICC colour profiles,
 *  profiles embedded in images and from user defined functions.
 */
///@{

/*!
 * \brief Create a transform from one colourspace to another
 *
 * \param file_in   The path to an ICC colour profile to be the source profile
 * \param file_out  The path to an ICC colour profile to be the destination profile
 * \param intent    The rendering intent to use
 *
 * \sa rendering_intents
 *
 * This creates a colour transform for use with the bitblt functions.
 *
 * When you have finished with a colourspace transform call destroy_transform to release the resources
 * associated with it.
 *
 * The file_in and file_out must reference a valid ICC profile file.  You can also reference an internal built-in profile.
 * These are accessed using the special filenames:
 *
 *  ":srgb"     - The standard sRGB profile
 *
 *  ":mono"     - A monochrome grey profile with a gamma of 2.2
 *
 *  ":lab"      - A profile compatible with L*a*b* images
 *
 *
 * \sa build_k_transform(), destroy_transform()
 *
 */
jett_transform jett::build_transform( const TCHAR *file_in, const TCHAR *file_out, int intent )
{
    lcms_transform_sampler sampler;
    sampler.open( file_in, file_out, intent, 0 );
    return build_transform(&sampler);
}

#ifndef __MACH__
jett_transform jett::build_transform( const char *file_in, const char *file_out, int intent )
{
	return build_transform( to_unicode( file_in ).c_str(), to_unicode( file_out ).c_str(), intent );
}
#endif

/*!
 * \brief Create a transform from one colourspace to another
 *
 * \param image_src An image with an embedded ICC colour profile to be use as the source profile.
 * \param file_out  The path to an ICC colour profile to be the destination profile
 * \param intent    The rendering intent to use
 *
 * \sa rendering_intents
 *
 * This creates a colour transform for use with the bitblt functions.
 *
 * When you have finished with a colourspace transform call destroy_transform to release the resources
 * associated with it.
 *
 * The file_out must reference a valid ICC profile file.  You can also reference an internal built-in profile.
 * These are accessed using the special filenames:
 *
 *  ":srgb"     - The standard sRGB profile
 *
 *  ":mono"     - A monochrome grey profile with a gamma of 2.2
 *
 *  ":lab"      - A profile compatible with L*a*b* images
 *
 *
 * \sa build_k_transform(), destroy_transform()
 *
 */
jett_transform jett::build_transform( jett_image& src_image, const TCHAR *file_out, int intent )
{
    lcms_transform_sampler sampler;
    sampler.open( *src_image, file_out, intent, 0 );
    return build_transform(&sampler);
}

#ifndef __MACH__
jett_transform jett::build_transform( jett_image& src_image, const char *file_out, int intent )
{
	return build_transform( src_image, to_unicode( file_out ).c_str(), intent );
}
#endif

/*!
 * \brief Create a transform from one colourspace to another
 *
 * \param file_in   The path to an ICC colour profile to be the source profile
 * \param image_dst An image with an embedded ICC colour profile to be use as the destinaton profile
 * \param intent    The rendering intent to use
 *
 * \sa rendering_intents
 *
 * This creates a colour transform for use with the bitblt functions.
 *
 * The file_in must reference a valid ICC profile file.  You can also reference an internal built-in profile.
 * These are accessed using the special filenames:
 *
 *  ":srgb"     - The standard sRGB profile
 *
 *  ":mono"     - A monochrome grey profile with a gamma of 2.2
 *
 *  ":lab"      - A profile compatible with L*a*b* images
 *
 *
 * When you have finished with a colourspace transform call destroy_transform to release the resources
 * associated with it.
 *
 * \sa build_k_transform(), destroy_transform()
 *
 */
jett_transform jett::build_transform( const TCHAR *file_in, jett_image& dst_image, int intent )
{
    lcms_transform_sampler sampler;
    sampler.open( file_in, *dst_image, intent, 0 );
    return build_transform(&sampler);
}

#ifndef __MACH__
jett_transform jett::build_transform( const char *file_in, jett_image& dst_image, int intent )
{
	return build_transform( to_unicode( file_in ).c_str(), dst_image, intent );
}
#endif

/*!
 * \brief Create a transform from one colourspace to another
 *
 * \param image_src An image with an embedded ICC colour profile to be use as the source profile
 * \param image_dst An image with an embedded ICC colour profile to be use as the destinaton profile
 * \param intent    The rendering intent to use
 *
 * \sa rendering_intents
 *
 * This creates a colour transform for use with the bitblt functions.
 *
 * When you have finished with a colourspace transform call destroy_transform to release the resources
 * associated with it.
 *
 * \sa build_k_transform(), destroy_transform()
 *
 */
jett_transform jett::build_transform( jett_image& image_src, jett_image& image_dst, int intent )
{
    lcms_transform_sampler sampler;
    sampler.open( *image_src, *image_dst, intent, 0 );
    return build_transform(&sampler);
}

/*!
 * \brief Create a transform from one colourspace to another using a devicelink profile
 *
 * \param devicelink_file   The path to an ICC colour profile to be the source profile
 * \param intent    The rendering intent to use
 *
 * \sa rendering_intents
 *
 * This creates a colour transform for use with the bitblt, polygon and text functions but unlike the other
 * build transform functions the ICC colour profile is a devicelink profile.
 *
 * The devicelink_file must specify a valid ICC device-link profile.  It may also instead specify a built-in
 * device-link profile:
 *
 * ":mono_cymk" - A Monochrome to CMYK transform which puts the monochrome data in to only the K channel.
 *
 * ":mono_rgb" - A Monochrome to RGB transform which puts the monochrome data equally in to all 3 RGB channels.
 *
 *
 * When you have finished with a colourspace transform call destroy_transform to release the resources
 * associated with it.
 *
 * \sa build_k_transform(), destroy_transform()
 *
 */
jett_transform jett::build_transform( const TCHAR *devicelink_file, int intent )
{
    if (strcasecmp(devicelink_file,_T(":mono_cmyk")) == 0)
    {
        k_only_transform_sampler sampler;
        return build_transform(&sampler);
    }
    else if (strcasecmp(devicelink_file,_T(":mono_rgb")) == 0)
    {
        const unsigned char colour[3] = { 255,255,255 };
        k_only_transform_sampler sampler( 3, colour );
        return build_transform(&sampler);
    }
    else
    {
        lcms_transform_sampler sampler;
        sampler.open( devicelink_file, intent, 0 );
        return build_transform(&sampler);
    }
}

#ifndef __MACH__
jett_transform jett::build_transform( const char *devicelink_file, int intent )
{
	return build_transform( to_unicode( devicelink_file ).c_str(), intent );
}
#endif


/*!
 * \brief Create an arbitary transform using a user-defined function
 *
 * \param pSampler    The user-defined mapping function
 *
 * This creates a colour transform for use with the bitblt, polygon and text functions.  The transform
 * can be specified as a user defined function.  For more information see the transform_sampler class.
 *
 * The transform_sampler object can be deleted after this function has finished.
 *
 * When you have finished with a colourspace transform call destroy_transform to release the resources
 * associated with it.
 *
 * \sa build_k_transform(), destroy_transform()
 */
jett_transform jett::build_transform( transform_sampler* pSampler )
{
    jett_transform r = new CTransform;
    r->build_transform(pSampler);
    return r;
}

/*!
 * \brief Convert a single colour from one colourspace to another
 *
 * \param trans    The colour transform
 * \param src      The colour that is to be converted
 * \param dst      A buffer to receive the converted colour.
 *
 * Using the specified colour transform a single colour can be converted to another colourspace.
 *
 */
void jett::convert( jett_transform trans, const unsigned char *src, unsigned char *dst )
{
    trans->convert( src, dst );
}


/*!
 * \brief Deletes a colourspace that has been finished with
 *
 * \param trans    The colour transform
 *
 * When you have finished with a colourspace transform call this function to release the resources
 * associated with it.
 *
 */
void jett::destroy_transform( jett_transform trans )
{
    delete trans;
}

///@}

/** @name OpenCL functions
 *  OpenCL operations (only work when initialised with OpenCL)
 */
///@{


/*!
 * \brief Cache an image in to the OpenCL buffer
 *
 * \param image		  A image
 *
 * This function forces an image to be put in to GPU memory.
 *
 */
void jett::cache_image( jett_image &image, bool read_only )
{
	m_cl->cache_image( image, read_only );
}


/*!
 * \brief Cache an image in to the OpenCL buffer and return a pointer to it
 *
 * \param image		  A image
 *
 * This function forces an image to be put in to GPU memory.
 *
 */
cl_mem jett::lockCLData( jett_image &image, bool read_only )
{
	CImage *pImage = image;
	return pImage->lockCLData( m_cl->get_cl_device(), read_only );
}

/*!
 * \brief Release a locked OpenCL object
 *
 * \param image		  A image
 *
 * Call this function after you have finished with manipulating the OpenCL memory.
 *
 */
void jett::unlockCLData( jett_image &image )
{
	CImage *pImage = image;
	pImage->unlockCLData( m_cl->get_cl_device() );
}

///@}


/** @name Font functions
 *  These functions are used to draw text on images
 */
///@{


/*!
 * \brief Create a font from a file
 *
 * \param filename    A font file to load
 * \param width       The width of the font (in pixels), if set to 0 the width will be set in proportion to the height
 * \param height      The height of the font (in pixels)
 * \param anti_alias  Draw the font using anti-alised edges
 *
 * This loads a font file.  When you have finished with the font call the destroy_font() function to release
 * the resources it is using.
 *
 * This function uses FreeType font rendering library and is operating system independent.  This function is useful
 * if you do not wish to have to install a font before using it.  For example, if the font was contained within
 * a design file.
 *
 */
jett_font jett::create_font( const TCHAR *filename, int width, int height, bool anti_alias )
{
	// Construct the font sampler around the font
	freetype_font_sampler *fs = new freetype_font_sampler();
	fs->openFont( filename, width, height, anti_alias );

	// Create a 1Mb font cache
	jett_font r = new CFont();
	r->attachFont( m_cl, fs, 1024 * 1024 );

	return r;
}

#ifndef __MACH__
jett_font jett::create_font( const char *filename, int width, int height, bool anti_alias )
{
	return create_font( to_unicode(filename).c_str(), width, height, anti_alias );
}
#endif


/*!
 * \brief Apply a matrix to a font
 *
 * \param f           The font object
 * \param matrix      The matrix to apply
 *
 * This applies the standard jett_matrix to the font to enable non-90 degree rotations
 * and other distortions.
 *
 */
void jett::set_font_matrix( jett_font f, const jett_matrix & matrix)
{
    f->setMatrix(matrix);
}


#ifndef __MACH__

/*!
 * \brief Create a font from a GDI font
 *
 * \param f			  A Windows GDI font object to use
 * \param width       The width of the font (in pixels), if set to 0 the width will be set in proportion to the height
 * \param height      The height of the font (in pixels)
 * \param anti_alias  Draw the font using anti-alised edges
 *
 * This function creates a FreeType font from a GDI font object.  After
 * this function returns, it is safe to destroy the GDI font object.
 *
 * Using this function enables the anti-aliasing of fonts.
 *
 */
jett_font  jett::create_font( HFONT f, int width, int height, bool anti_alias )
{

	// Construct the font sampler around the font
	HDC dc = CreateDC(_T("DISPLAY"),NULL,NULL,NULL);
	HFONT old_font = (HFONT)SelectObject( dc, f );

	int size = GetFontData( dc, 0, 0, NULL, 0 );
	if (size == GDI_ERROR)
	{
		throw jett_exception(JETT_GDI_FAILURE,GetLastError(),"The font is not a TrueType font");
	}
	void *block = new unsigned char[ size ];
	GetFontData( dc, 0, 0, block, size );

	SelectObject( dc, old_font );
	DeleteDC( dc );


	freetype_font_sampler *fs = new freetype_font_sampler();
	fs->openFont( (char*)block, size, width, height, anti_alias );

	// Create a 1Mb font cache
	jett_font r = new CFont();
	r->attachFont( m_cl, fs, 512 * 1024 );

	return r;
}

/*!
 * \brief Create a font from a GDI font
 *
 * \param f			  A Windows GDI font object to use
 *
 * This function maps a GDI font to a jett font object.  Do not destroy the HFONT
 * object until after this new font has been destroyed.
 *
 */jett_font  jett::create_font( HFONT f )
{
	// Construct the font sampler around the font
	gdi_font_sampler *fs = new gdi_font_sampler();
	fs->openFont( f );

	// Create a 1Mb font cache
	jett_font r = new CFont();
	r->attachFont( m_cl, fs, 512 * 1024 );

	return r;
}
#endif


/*!
 * \brief Delete a font
 *
 * When you have finished with a font call the this function to release
 * the resources it is using.
 *
 */
void jett::destroy_font( jett_font f )
{
    delete f;
}

///@}


/** @name Dither functions
 *  These functions are used to convert the image to a smaller number of grey-levels ready for printing
 */
///@{


/*!
 * \brief Create an ordered dither screen using the void and cluster technique
 *
 * \param planes			The number of screens to generate (specify 4 for CMYK and 3 for RGB)
 * \param size				The sizes of the screens in pixels
 * \param sigma				The sigma used to calculate the cluster function (1.5 would be a reasonable value)
 * \param output_levels     The number of output levels the screen is to target (2 for binary output)
 * \param callback          If this is non-NULL then this function will be called during the screen creation to register progress.
 *
 * This function generates a new set of ordered dither screens using the void and cluster technique.
 * The screens must be destroyed with destroy_screen().  These screens can then be used with the dither functions.
 *
 * Large screens (that is bigger than 100 pixels across) will take hours to generate, and really large screens (over 
 * 500 pixels across) will take days to create.  Use the progress callback to track it.  Once the screen has been
 * generated you can save it to file so that you need only create the screens once.
 *
 * The void and cluster technique was invented by Robert Ulichney and you can download a paper describing the 
 * technique in detail from here: <a href="http://www.hpl.hp.com/personal/Robert_Ulichney/publications.html">Robert Ulichney Publications</a>.
 * (Look for "The Void-and-Cluster Method for Generating Dither Arrays").
 *
 * \sa \ref page_dither
 */
jett_screens jett::create_screens( int planes, int* size, double sigma, int output_levels, jett_progress_callback callback )
{
	COrderedScreenCollection *s = new COrderedScreenCollection;
	try
	{
		s->createScreens(planes, size, sigma, output_levels, callback );
	}
	catch (...)
	{
		delete s;
		throw;
	}
	return s;
}

/*!
 * \brief Load a set of ordered dither screens from file
 *
 * \param filename			The name of the file to load
 *
 * This function loads a set of ordered screens from file
 *
 * \sa \ref page_dither
 */
jett_screens jett::load_screens( const TCHAR *filename )
{
	COrderedScreenCollection *s = new COrderedScreenCollection;
	try
	{
		s->loadFromFile( filename );
	}
	catch (...)
	{
		delete s;
		throw;
	}

	return s;
}

#ifndef __MACH__
jett_screens jett::load_screens( const char *filename )
{
	return load_screens( to_unicode( filename ).c_str() );
}
#endif


/*!
 * \brief Save a set of ordered screens to file
 *
 * \param filename			The name of the file to save to
 *
 * This function saves a set of ordered screens to file
 *
 * \sa \ref page_dither
 */
void jett::save_screens( jett_screens s, const TCHAR *filename )
{
	s->saveToFile( filename );
}

#ifndef __MACH__
void jett::save_screens( jett_screens s, const char *filename )
{
	s->saveToFile( to_unicode( filename ).c_str() );
}
#endif

/*!
 * \brief Destroy an ordered dither screen
 *
 * \param s				The screen to destroy
 *
 * This function deletes the screen object and frees resources associated with it.
 *
 */
void jett::destroy_screens( jett_screens s )
{
	delete s;
}

/*!
 * \brief Dither an image and separate it in to it's colour components
 *
 * \param src_image				The source image to dither and separate
 * \param dst_images			An array of images to separate in to
 * \param screens				The screens for the dithering (one for each separation)
 * \param linearization         The linearization set (or NULL if not required)
 * \param scale					Multiply the output pixels by this value.  If left at zero, then the output will be full-scale (i.e. 0-255).
 *
 * This function takes a source colour image and separates it in to seprate dithered bitmaps
 * ready for printing.
 *
 * All the destination bitmaps must be the same size as the source bitmap.  They must all
 * be the same type, which must be image_mono1, image_mono2, image_mono4 or image_mono8.
 *
 * The linearization set is optional, and if supplied it will be applied prior to dithering.
 *
 * There must be the same number of screens in the screens object as there are
 * components in the image, that is 3 for an RGB image and 4 for a CMYK image.
 *
 * This is also true for the linearization set (if supplied), there must be the same number 
 * of linearizations in the linearization set object as there are components in the image.
 *
 * \sa \ref page_dither
 */
void jett::dither( jett_image& src_image, jett_image* dst_images, jett_screens screens, jett_linearization linearization, int scale )
{
	// Create an array to copy over in to
	CImage* images[4];
	for (size_t i = 0; i < screens->getPlanes() && i < 4; ++i)
	{
		images[i] = dst_images[i];
	}

	if (scale == 0)
	{
		scale = static_cast<int>(screens->getQuantizer(0) + 0.5);
		switch (dst_images[0].getType())
		{
		case image_mono1:
			scale = 1;
			break;
		case image_mono2:
			scale = scale / 64;
			break;
		case image_mono4:
			scale = scale / 16;
			break;
		case image_mono:
			break;
		}
	}

	m_cl->dither()->dither( src_image, images, *screens, linearization, scale );
}

/*!
 * \brief Dither an image in place
 *
 * \param src_image				The source image to dither and separate
 * \param screens				The screens for the dithering (one for each colour)
 * \param linearization         The linearization set (or NULL if not required)
 * \param scale					Multiply the output pixels by this value.  If left at zero, then the output will be full-scale (i.e. 0-255).
 *
 * This function takes an image and dithers it ready for printing.
 *
 * The linearization set is optional, and if supplied it will be applied prior to dithering.
 *
 * There must be the same number of screens in the screens object as there are
 * components in the image, that is 3 for an RGB image and 4 for a CMYK image.
 *
 * This is also true for the linearization set (if supplied), there must be the same number
 * of linearizations in the linearization set object as there are components in the image.
 *
 * \sa \ref page_dither
 */
void jett::dither( jett_image& src_image, jett_screens screens, jett_linearization linearization, int scale )
{
	if (scale == 0)
	{
		scale = static_cast<int>(screens->getQuantizer(0) + 0.5);
	}

	m_cl->dither()->dither( src_image, *screens, linearization, scale );
}


///@}
