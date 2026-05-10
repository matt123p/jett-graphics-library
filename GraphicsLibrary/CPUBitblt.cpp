//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"

#include "CPUBitblt.h"
#include "Image.h"
#include "transform.h"

CCPUBitblt::CCPUBitblt(CCPUProcessor* cl, int src_dimensions, bool alpha )
{
    m_dimensions_in = src_dimensions;
    m_alpha = alpha;
}

CCPUBitblt::~CCPUBitblt()
{
    
}




//////////////////////////////////////////////////////////////////
//
// For cubic resize
//
static float cubic_spline_fit ( float  dx,
                                float  pt0,
                                float  pt1,
                                float  pt2,
                                float  pt3)
{
    return static_cast<float>(((( ( -pt0 + 3.0f * pt1 - 3.0f * pt2 + pt3 ) *   dx +
              ( 2.0f * pt0 - 5.0f * pt1 + 4.0f * pt2 - pt3 ) ) * dx +
             ( -pt0 + pt2 ) ) * dx + (pt1 + pt1) ) / 2.0f);
}

template<class src_t, class dst_t, int src_max> void _copy( CTransform *trans, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap, bool alpha )
{    
    if (src_bitmap->isBGR())
    {
        params.flags |= flip_src_components;
    }
    
    if (!trans)
    {
        params.flags |= perform_no_cm;
    }
    else
    {
        if (trans->get_dimensions_out() != dst_bitmap->getComponents())
        {
            throw jett_exception( JETT_INVALID_PROFILE, 0, "This transform is not for this destination image (number of components)" );
        }
    }
    
    if (dst_bitmap->isBGR())
    {
        params.flags |= flip_dst_components;
    }
    
    // The src/dst bitmaps
    src_t* src_bitmap_p = (src_t*)src_bitmap->lockData(true);
    int src_stride = static_cast<int>(src_bitmap->getStride() / sizeof(src_t));
    int src_pixel_stride = static_cast<int>(src_bitmap->getPixelStride());
    dst_t* dst_bitmap_p = (dst_t*)dst_bitmap->lockData( false );
    int dst_stride = static_cast<int>(dst_bitmap->getStride() / sizeof(dst_t));
    int dst_pixel_stride = static_cast<int>(dst_bitmap->getPixelStride());
    int dst_dimensions = static_cast<int>(dst_bitmap->getComponents());

    // Perform the copy
#ifdef _OPENMP
    bool use_openmp = (params.dst_y2 - params.dst_y1) > OPENMP_CUTOFF;
#endif
#pragma omp parallel for if (use_openmp)
	for (int px_y = params.dst_y1; px_y <= params.dst_y2; ++ px_y)
    {
        src_t *src_pixel = NULL;
        dst_t *dst_pixel = NULL;
        src_t src_hold[ 8 ];
        dst_t dst_hold[ 8 ];

        for (int px_x = params.dst_x1; px_x <= params.dst_x2; ++ px_x )
        {
            int _x;
            int _y;
            
            if ((params.flags & perform_rotate90) != 0)
            {
                // 90 degree rotation clockwise
                _x = (px_y - params.dst_y1);
                _y = (params.dst_x2 - px_x);
            }
            else if ((params.flags & perform_rotate180) != 0)
            {
                // 180 degree rotation clockwise
                _x = params.dst_x2 - px_x;
                _y = params.dst_y2 - px_y;
            }
            else if ((params.flags & perform_rotate270) != 0)
            {
                // 270 degree rotation clockwise
                _x = (params.dst_y2 - px_y);
                _y = (px_x - params.dst_x1);
            }
            else
            {
                // No rotation
                _x = (px_x - params.dst_x1);
                _y = (px_y - params.dst_y1);
            }
            
            
            if ((params.flags & perform_resize_simple) != 0)
            {
                // Nearest neighbour resize
                _x = (int)(_x * params.scale_src_x) + params.src_x;
                _y = (int)(_y * params.scale_src_y) + params.src_y;
                
                int pixel_index = _y * src_stride + _x * src_pixel_stride;
                src_pixel = src_bitmap_p + pixel_index;
            }
            else if ((params.flags & perform_resize_complx) != 0)
            {
                // Cubic fit resize
                //
                float dx = _x * params.scale_src_x + params.src_x;
                float dy = _y * params.scale_src_y + params.src_y;
                _x = (int)(dx);
                _y = (int)(dy);
                
                bool near_the_edge = (_y < 2 || _x < 2
                                      || _x >= src_bitmap->getWidth() - 2
                                      || _y >= src_bitmap->getHeight() - 2);
                
                
                if (!near_the_edge)
                {
                    dx = dx - (int)dx;
                    dy = dy - (int)dy;
                    
                    float workspace_float[ 32 ];
                    for (int y_offset = 0; y_offset < 4; ++ y_offset)
                    {
                        for (int col=0; col < src_pixel_stride; ++ col)
                        {
                            int pixel_index = (_y-2 + y_offset) * src_stride + (_x-2) * src_pixel_stride + col;
                            workspace_float[ y_offset + col*4  ] = cubic_spline_fit( dx,
                                                                                    src_bitmap_p[ pixel_index ],
                                                                                    src_bitmap_p[ pixel_index + src_pixel_stride ],
                                                                                    src_bitmap_p[ pixel_index + src_pixel_stride*2 ],
                                                                                    src_bitmap_p[ pixel_index + src_pixel_stride*3 ] );
                            
                        }
                    }
                    
                    for (int col=0; col < src_pixel_stride; ++ col)
                    {
                        float pixel = cubic_spline_fit( dy,
                                                       workspace_float[ col * 4],
                                                       workspace_float[ col * 4 + 1 ],
                                                       workspace_float[ col * 4 + 2 ],
                                                       workspace_float[ col * 4 + 3 ] );
                        if (pixel < 0)
                        {
                            pixel = 0;
                        }
                        if (pixel > src_max)
                        {
                            pixel = src_max;
                        }
                        src_hold[col] = static_cast<int>(pixel);
                    }
                    src_pixel = src_hold;
                }
                else
                {
                    int pixel_index = _y * src_stride + _x * src_pixel_stride;
                    src_pixel = src_bitmap_p + pixel_index;
                }
            }
            else
            {
                // No re-size
                _x = _x + params.src_x;
                _y = _y + params.src_y;
                
                int pixel_index = _y * src_stride + _x * src_pixel_stride;
                src_pixel = src_bitmap_p + pixel_index;
            }
            
            
            // Now determine the destination pixel location
            int pixel_index = px_y * dst_stride + px_x * dst_pixel_stride;
            dst_pixel = dst_bitmap_p + pixel_index;
            
            // Perform the conversion
            if ((params.flags & perform_no_cm) != 0)
            {
                pixel_convert<dst_t,src_t>(dst_pixel, src_pixel, dst_pixel_stride);
            }
            else if ((params.flags & perform_white_is_trans) != 0)
            {
                unsigned char c = 0;
                trans->convert(&c, dst_hold );
                src_t alpha = src_pixel[ 0 ];
                src_t ralpha = src_max - src_pixel[ 3 ];

                for (int col = 0; col < dst_dimensions; ++ col)
                {
                    int px = ( dst_pixel[col] * alpha ) + ( dst_hold[col] * ralpha );
                    dst_pixel[col] = px / src_max;
                }
            }
            else if (alpha)
            {
                trans->convert(src_pixel, dst_hold );
                src_t alpha = src_pixel[ 3 ];
                src_t ralpha = src_max - src_pixel[ 3 ];
                for (int col = 0; col < dst_dimensions; ++ col)
                {
                    int px = ( dst_pixel[col] * ralpha ) + ( dst_hold[col] * alpha );
                    dst_pixel[col] = px / src_max;
                }
            }
            else
            {
                trans->convert(src_pixel, dst_pixel);
            }
        }
    }
    
    
    // Unlock the bitmaps
    src_bitmap->unlockData();
    dst_bitmap->unlockData();
}

void CCPUBitblt::copy( CTransform *trans, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap )
{
    // Check some assumptions
    if (src_bitmap->getComponents() != m_dimensions_in)
    {
        throw jett_exception( JETT_INTERNAL_ERROR, 0, "This CBitblt object was not built for this source image (number of components)" );
    }
    
    if (!trans)
    {
        if (src_bitmap->getComponents() != dst_bitmap->getComponents())
        {
            throw jett_exception( JETT_INVALID_PROFILE, 0, "Colour management cannot be disabled because the source and destination bitmaps are not compatible" );
        }
    }
    else
    {
        if (trans->get_dimensions_in() != src_bitmap->getComponents())
        {
            throw jett_exception( JETT_INVALID_PROFILE, 0, "This transform is not for this source image (number of components)" );
        }
    }

    
    //
    // Use templates to perform automatic 16bit / 8bit conversions
    //
    if      (src_bitmap->is16bpp() && dst_bitmap->is16bpp())
    {
        _copy<unsigned short, unsigned short,65535>( trans, params, src_bitmap, dst_bitmap, m_alpha );
    }
    else if (!src_bitmap->is16bpp() && dst_bitmap->is16bpp())
    {
        _copy<unsigned char, unsigned short,255>( trans, params, src_bitmap, dst_bitmap, m_alpha );
    }
    else if (src_bitmap->is16bpp() && !dst_bitmap->is16bpp())
    {
        _copy<unsigned short, unsigned char,65535>( trans, params, src_bitmap, dst_bitmap, m_alpha );
    }
    else // if (!src_bitmap->is16bpp() && !dst_bitmap->is16bpp())
    {
        _copy<unsigned char, unsigned char,255>( trans, params, src_bitmap, dst_bitmap, m_alpha );
    }
}


template<class dst_t> void _copy_glyph( unsigned int colour, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap )
{        
    if (dst_bitmap->isBGR())
    {
        params.flags |= flip_dst_components;
    }
    
    // The src/dst bitmaps
    unsigned char* src_bitmap_p = src_bitmap->lockData(true);
    int src_stride = static_cast<int>(src_bitmap->getStride());
    int src_pixel_stride = static_cast<int>(src_bitmap->getPixelStride());
    dst_t* dst_bitmap_p = (dst_t*)dst_bitmap->lockData( false );
    int dst_stride = static_cast<int>(dst_bitmap->getStride() / sizeof(dst_t));
    int dst_pixel_stride = static_cast<int>(dst_bitmap->getPixelStride());
    int dst_dimensions = static_cast<int>(dst_bitmap->getComponents());
    
	// Decode the colour
	dst_t dst_hold[ 8 ];
	for (int i = 0; i < dst_dimensions; ++i)
	{
		int_convert(dst_hold[i], static_cast<unsigned char>((colour >> (i*8) ) & 0xff));
	}

    // Perform the copy
#ifdef _OPENMP
    bool use_openmp = (params.dst_y2 - params.dst_y1) > OPENMP_CUTOFF;
#endif
#pragma omp parallel for if (use_openmp)
	for (int px_y = params.dst_y1; px_y <= params.dst_y2; ++ px_y)
    {
        unsigned char *src_pixel = NULL;
        dst_t *dst_pixel = NULL;

        for (int px_x = params.dst_x1; px_x <= params.dst_x2; ++ px_x )
        {
            int _x;
            int _y;
            
            if ((params.flags & perform_rotate90) != 0)
            {
                // 90 degree rotation clockwise
                _x = (px_y - params.dst_y1);
                _y = (params.dst_x2 - px_x);
            }
            else if ((params.flags & perform_rotate180) != 0)
            {
                // 180 degree rotation clockwise
                _x = params.dst_x2 - px_x;
                _y = params.dst_y2 - px_y;
            }
            else if ((params.flags & perform_rotate270) != 0)
            {
                // 270 degree rotation clockwise
                _x = (params.dst_y2 - px_y);
                _y = (px_x - params.dst_x1);
            }
            else
            {
                // No rotation
                _x = (px_x - params.dst_x1);
                _y = (px_y - params.dst_y1);
            }
            
            // No re-size
			{
				_x = _x + params.src_x;
				_y = _y + params.src_y;
                
				int pixel_index = _y * src_stride + _x * src_pixel_stride;
				src_pixel = src_bitmap_p + pixel_index;
			}            
            
            // Now determine the destination pixel location
            int pixel_index = px_y * dst_stride + px_x * dst_pixel_stride;
            dst_pixel = dst_bitmap_p + pixel_index;
            
            // Perform the conversion
            int alpha = src_pixel[ 0 ];
            int ralpha = 255 - src_pixel[ 0 ];
            for (int col = 0; col < dst_dimensions; ++ col)
            {
                int px = ( dst_pixel[col] * alpha ) + ( dst_hold[col] * ralpha );
                dst_pixel[col] = px / 255;
            }
        }
    }
    
    
    // Unlock the bitmaps
    src_bitmap->unlockData();
    dst_bitmap->unlockData();
}

void CCPUBitblt::copy_glyph( unsigned int colour, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap )
{
    // Check some assumptions
    if (1 != m_dimensions_in)
    {
        throw jett_exception( JETT_INTERNAL_ERROR,0, "This CBitblt object was not built for this source image (number of components)" );
    }
    if (src_bitmap->getComponents() != m_dimensions_in)
    {
        throw jett_exception( JETT_INTERNAL_ERROR,0, "This CBitblt object was not built for this source image (number of components)" );
    }
    if (src_bitmap->is16bpp())
    {
        throw jett_exception( JETT_INTERNAL_ERROR,0, "This font glyphs must be 8bpp" );
    }

    
    //
    // Use templates to perform automatic 16bit / 8bit conversions
    //
    if  (dst_bitmap->is16bpp())
    {
        _copy_glyph<unsigned short>(colour, params, src_bitmap, dst_bitmap );
    }
    else
    {
        _copy_glyph<unsigned char>(colour, params, src_bitmap, dst_bitmap );
    }

}


template<class src_t, class dst_t, int src_max> void _copy_matrix( CTransform *trans, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap, bool alpha )
{
    if (!trans)
    {
        if (src_bitmap->getComponents() != dst_bitmap->getComponents())
        {
            throw jett_exception( JETT_INVALID_PROFILE, 0, "Colour management cannot be disabled because the source and destination bitmaps are not compatible" );
        }
    }
    else
    {
        if (trans->get_dimensions_in() != src_bitmap->getComponents())
        {
            throw jett_exception( JETT_INVALID_PROFILE, 0, "This transform is not for this source image (number of components)" );
        }
    }
    
    if (src_bitmap->isBGR())
    {
        params.flags |= flip_src_components;
    }
    
    if (!trans)
    {
        params.flags |= perform_no_cm;
    }
    else
    {
        if (trans->get_dimensions_out() != dst_bitmap->getComponents())
        {
            throw jett_exception( JETT_INVALID_PROFILE, 0, "This transform is not for this destination image (number of components)" );
        }
    }
    
    if (dst_bitmap->isBGR())
    {
        params.flags |= flip_dst_components;
    }
    
    // The src/dst bitmaps
    src_t* src_bitmap_p = (src_t*)src_bitmap->lockData(true);
    int src_stride = static_cast<int>(src_bitmap->getStride() / sizeof(src_t));
    int src_pixel_stride = static_cast<int>(src_bitmap->getPixelStride());
    dst_t* dst_bitmap_p = (dst_t*)dst_bitmap->lockData( false );
    int dst_stride = static_cast<int>(dst_bitmap->getStride() / sizeof(dst_t));
    int dst_pixel_stride = static_cast<int>(dst_bitmap->getPixelStride());
    int dst_dimensions = static_cast<int>(dst_bitmap->getComponents());
    
    
    // Perform the copy
#ifdef _OPENMP
    bool use_openmp = (params.dst_y2 - params.dst_y1) > OPENMP_CUTOFF;
#endif
#pragma omp parallel for if (use_openmp)
	for (int px_y = params.dst_y1; px_y <= params.dst_y2; ++ px_y)
    {
        src_t *src_pixel = NULL;
        dst_t *dst_pixel = NULL;
        src_t src_hold[ 8 ];
        dst_t dst_hold[ 8 ];

        for (int px_x = params.dst_x1; px_x <= params.dst_x2; ++ px_x )
        {
            // Apply matrix
            jett_point p = params.matrix.apply( jett_point( px_x, px_y ) );
            
            if (   p.x < params.src_x || p.x > params.src_x2
                || p.y < params.src_y || p.y > params.src_y2)
            {
                // This outside of our source target area
                continue;
            }
            
            if ((params.flags & perform_resize_simple) != 0)
            {
                // Nearest neighbour resize
                int _x = static_cast<int>(p.x + 0.5f);
                int _y = static_cast<int>(p.y + 0.5f);

                int pixel_index = _y * src_stride + _x * src_pixel_stride;
                src_pixel = src_bitmap_p + pixel_index;
            }
            else if ((params.flags & perform_resize_complx) != 0)
            {
                // Cubic fit resize
                //
                float dx = p.x;
                float dy = p.y;
                int _x = (int)(dx);
                int _y = (int)(dy);
                
                bool near_the_edge = (_y < 2 || _x < 2
                                      || _x >= src_bitmap->getWidth() - 2
                                      || _y >= src_bitmap->getHeight() - 2);
                
                
                if (!near_the_edge)
                {
                    dx = dx - (int)dx;
                    dy = dy - (int)dy;
                    
                    float workspace_float[ 32 ];
                    for (int y_offset = 0; y_offset < 4; ++ y_offset)
                    {
                        for (int col=0; col < src_pixel_stride; ++ col)
                        {
                            int pixel_index = (_y-2 + y_offset) * src_stride + (_x-2) * src_pixel_stride + col;
                            workspace_float[ y_offset + col*4  ] = cubic_spline_fit( dx,
                                                                                    src_bitmap_p[ pixel_index ],
                                                                                    src_bitmap_p[ pixel_index + src_pixel_stride ],
                                                                                    src_bitmap_p[ pixel_index + src_pixel_stride*2 ],
                                                                                    src_bitmap_p[ pixel_index + src_pixel_stride*3 ] );
                            
                        }
                    }
                    
                    for (int col=0; col < src_pixel_stride; ++ col)
                    {
                        float pixel = cubic_spline_fit( dy,
                                                       workspace_float[ col * 4],
                                                       workspace_float[ col * 4 + 1 ],
                                                       workspace_float[ col * 4 + 2 ],
                                                       workspace_float[ col * 4 + 3 ] );
                        if (pixel < 0)
                        {
                            pixel = 0;
                        }
                        if (pixel > src_max)
                        {
                            pixel = src_max;
                        }
                        src_hold[col] = static_cast<int>(pixel);
                    }
                    src_pixel = src_hold;
                }
                else
                {
                    int pixel_index = _y * src_stride + _x * src_pixel_stride;
                    src_pixel = src_bitmap_p + pixel_index;
                }
            }
            else
            {
                // No re-size
                int _x = static_cast<int>(p.x + 0.5f);
                int _y = static_cast<int>(p.y + 0.5f);
                
                int pixel_index = _y * src_stride + _x * src_pixel_stride;
                src_pixel = src_bitmap_p + pixel_index;
            }
            
            
            // Now determine the destination pixel location
            int pixel_index = px_y * dst_stride + px_x * dst_pixel_stride;
            dst_pixel = dst_bitmap_p + pixel_index;
            
            // Perform the conversion
            if ((params.flags & perform_no_cm) != 0)
            {
                pixel_convert<dst_t,src_t>(dst_pixel, src_pixel, dst_pixel_stride);
            }
            else if ((params.flags & perform_white_is_trans) != 0)
            {
                unsigned char c = 0;
                trans->convert(&c, dst_hold );
                int alpha = src_pixel[ 0 ];
                int ralpha = src_max - src_pixel[ 0 ];
                for (int col = 0; col < dst_dimensions; ++ col)
                {
                    int px = ( dst_pixel[col] * alpha ) + ( dst_hold[col] * ralpha );
                    dst_pixel[col] = px / src_max;
                }
            }
            else if (alpha)
            {
                trans->convert(src_pixel, dst_hold );
                int alpha = src_pixel[ 3 ];
                int ralpha = src_max - src_pixel[ 3 ];
                for (int col = 0; col < dst_dimensions; ++ col)
                {
                    int px = ( dst_pixel[col] * ralpha ) + ( dst_hold[col] * alpha );
                    dst_pixel[col] = px / src_max;
                }
            }
            else
            {
                trans->convert(src_pixel, dst_pixel);
            }
        }
    }
    
    
    // Unlock the bitmaps
    src_bitmap->unlockData();
    dst_bitmap->unlockData();
}

void CCPUBitblt::copy_matrix( CTransform *trans, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap )
{
    // Check some assumptions
    if (src_bitmap->getComponents() != m_dimensions_in)
    {
        throw jett_exception( JETT_INTERNAL_ERROR, 0, "This CBitblt object was not built for this source image (number of components)" );
    }
    
    //
    // Use templates to perform automatic 16bit / 8bit conversions
    //
    if      (src_bitmap->is16bpp() && dst_bitmap->is16bpp())
    {
        _copy_matrix<unsigned short, unsigned short,65535>(trans, params, src_bitmap, dst_bitmap, m_alpha);
    }
    else if (!src_bitmap->is16bpp() && dst_bitmap->is16bpp())
    {
        _copy_matrix<unsigned char, unsigned short,255>(trans, params, src_bitmap, dst_bitmap, m_alpha);
    }
    else if (src_bitmap->is16bpp() && !dst_bitmap->is16bpp())
    {
        _copy_matrix<unsigned short, unsigned char,65535>(trans, params, src_bitmap, dst_bitmap, m_alpha);
    }
    else // if (!src_bitmap->is16bpp() && !dst_bitmap->is16bpp())
    {
        _copy_matrix<unsigned char, unsigned char,255>(trans, params, src_bitmap, dst_bitmap, m_alpha);
    }
}


