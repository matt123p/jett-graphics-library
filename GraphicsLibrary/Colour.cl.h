static const unsigned char Colour_cl[] =
R"CLC(
//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

//////////////////////////////////////////////////////////////////
//
// Flags
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
#define     perform_black_is_trans  512

//////////////////////////////////////////////////////////////////
//
// For cubic resize
//
float cubic_spline_fit ( float  dx,
                   float  pt0,
                   float  pt1,
                   float  pt2,
                   float  pt3)
{
    return ((( ( -pt0 + 3.0f * pt1 - 3.0f * pt2 + pt3 ) *   dx +
                        ( 2.0f * pt0 - 5.0f * pt1 + 4.0f * pt2 - pt3 ) ) * dx +
                       ( -pt0 + pt2 ) ) * dx + (pt1 + pt1) ) / 2.0f;
}

//////////////////////////////////////////////////////////////////
//
// colour management defines
//

// Here is how we share the calculated values
#define     local_X0    0
#define     local_X1    1
#define     local_Y0    2
#define     local_Y1    3
#define     local_Z0    4
#define     local_Z1    5
#define     local_K0    6
#define     local_K1    7
#define     local_rx    8
#define     local_ry    9
#define     local_rz    10
#define     local_rk    11
#define     alpha_pixel 12

//
// Look up a value in the LUT
//
#define px( px_pixel_in, px_n ) ((px_pixel_in>>px_n)&1)

#if SRC_DIMENSIONS == 1
#define     lut_fetch(n,x,y,z,k) (lut[ workspace_block[ local_X0+px(x,n) ] + xyz_index])
#elif SRC_DIMENSIONS == 4
#define     lut_fetch(n,x,y,z,k) (lut[ \
          workspace_block[ local_X0+px(x,n) ] \
        + workspace_block[ local_Y0+px(y,n) ] \
        + workspace_block[ local_Z0+px(z,n) ] \
        + workspace_block[ local_K0+px(k,n) ] \
        + xyz_index ])
#else
#define     lut_fetch(n,x,y,z,k) (lut[ \
          workspace_block[ local_X0+px(x,n) ] \
        + workspace_block[ local_Y0+px(y,n) ] \
        + workspace_block[ local_Z0+px(z,n) ] \
        + xyz_index ])
#endif

__kernel void bitblt(
                        int flags,
                        __global const unsigned char *src_bitmap,
                        int src_stride,
                        int src_pixel_stride,
                        int src_x,
                        int src_y,
                        int src_width,
                        int src_height,
                        float scale_src_x,
                        float scale_src_y,
                        __global unsigned char *dst_bitmap,
                        int dst_stride,
                        int dst_pixel_stride,
                        int dst_dimensions,
                        int dst_x1,
                        int dst_y1,
                        int dst_x2,
                        int dst_y2,
                        __global const unsigned short *lut,
                        __constant int *strides,
                        __local  int *workspace_block_in )
{
	int xyz_index = get_local_id(0);
	int pixel_mod = get_local_id(1);
    bool do_cm = (flags & perform_no_cm) == 0;

    int src_pixel = xyz_index;
    if ((flags & flip_src_components) != 0 && src_pixel <= 2)
    {
        src_pixel = 2 - src_pixel;
    }

    int dst_pixel = xyz_index;
    if ((flags & flip_dst_components) != 0 && dst_pixel <= 2)
    {
        dst_pixel = 2 - dst_pixel;
    }
	int workspace_stride = ((flags & perform_resize_complx) != 0) ? 60 : 15;
	__local int   *workspace_block = workspace_block_in + (pixel_mod * workspace_stride);

	for (int px_y = get_group_id(0) + dst_y1; px_y <= dst_y2; px_y += get_global_size(0) / get_local_size(0))
    {
        for (int px_x_q = (get_group_id(1) * WORKGROUP_FACTOR) + dst_x1; px_x_q <= dst_x2; px_x_q += (get_global_size(1) / get_local_size(1)) * WORKGROUP_FACTOR )
        {
            int px_x = px_x_q + pixel_mod;

            //////////////////////////////////////////////////////////////////
            //
            // Fetch the pixel
            //
            int pixel = 0;

            if (xyz_index < SRC_DIMENSIONS && px_x <= dst_x2)
            {
				int _x;
				int _y;

				if ((flags & perform_rotate90) != 0)
				{
					// 90 degree rotation clockwise
					_x = (px_y - dst_y1);
					_y = (dst_x2 - px_x);
				}
				else if ((flags & perform_rotate180) != 0)
				{
					// 180 degree rotation clockwise
					_x = dst_x2 - px_x;
					_y = dst_y2 - px_y;
				}
				else if ((flags & perform_rotate270) != 0)
				{
					// 270 degree rotation clockwise
					_x = (dst_y2 - px_y);
					_y = (px_x - dst_x1);
				}
				else
				{
					// No rotation
					_x = (px_x - dst_x1);
					_y = (px_y - dst_y1);
				}

                if ((flags & perform_resize_simple) != 0)
                {
                    // Nearest neighbour resize
                    _x = (int)(_x * scale_src_x) + src_x;
                    _y = (int)(_y * scale_src_y) + src_y;
                    int pixel_index = mad24( _y, src_stride, _x * src_pixel_stride );
                    pixel = src_bitmap[ pixel_index + src_pixel ];
#if ALPHA_BLEND
                    if (src_pixel == 2)
                    {
                        workspace_block[ alpha_pixel ] = src_bitmap[ pixel_index + 3 ];
                    }
#endif
                }
                else if ((flags & perform_resize_complx) != 0)
                {
                    // Cubic fit resize
                    //
                    float dx = _x * scale_src_x;
                    float dy = _y * scale_src_y;
                    _x = (int)(dx) + src_x;
                    _y = (int)(dy) + src_y;
                    dx = dx - (int)dx;
                    dy = dy - (int)dy;

                    __local float *workspace_float = (__local float*)workspace_block;

                    bool near_the_edge = (_y < 2 || _x < 2
                                          || _x >= src_width- 2
                                          || _y >= src_height - 2);

                    if (!near_the_edge)
                    {
                        for (int y_offset = 0; y_offset < 4; ++ y_offset)
                        {
                            for (int x_offset = 0; x_offset < 4; ++ x_offset)
                            {
                                int pixel_index = mad24( _y-2 + y_offset, src_stride, (_x-2 + x_offset) * src_pixel_stride );
                                workspace_float[ x_offset + src_pixel * 8 + 4 ] = src_bitmap[ pixel_index + src_pixel ];
                            }
                            workspace_float[ src_pixel * 8 + y_offset ] = cubic_spline_fit( dx,
                                                                                               workspace_float[ src_pixel * 8 + 4],
                                                                                               workspace_float[ src_pixel * 8 + 5 ],
                                                                                               workspace_float[ src_pixel * 8 + 6 ],
                                                                                               workspace_float[ src_pixel * 8 + 7 ] );
                        }
                        pixel = clamp(cubic_spline_fit( dy,
                                                       workspace_float[ src_pixel * 8],
                                                       workspace_float[ src_pixel * 8 + 1 ],
                                                       workspace_float[ src_pixel * 8 + 2 ],
                                                       workspace_float[ src_pixel * 8 + 3 ] ),0.0f,255.0f);
                    }
                    else
                    {
                        int pixel_index = mad24( _y, src_stride, _x * src_pixel_stride );
                        pixel = src_bitmap[ pixel_index + src_pixel ];
                    }
#if ALPHA_BLEND
                    if (src_pixel == 2)
                    {
                        int pixel_index = mad24( _y, src_stride, _x * src_pixel_stride );
                        workspace_block[ alpha_pixel ] = src_bitmap[ pixel_index + 3 ];
                    }
#endif
                }
                else
                {
                    // No re-size
                    _x = _x + src_x;
                    _y = _y + src_y;
                    int pixel_index = mad24( _y, src_stride, _x * src_pixel_stride );
                    pixel = src_bitmap[ pixel_index + src_pixel ];
#if ALPHA_BLEND
                    if (src_pixel == 2)
                    {
                        workspace_block[ alpha_pixel ] = src_bitmap[ pixel_index + 3 ];
                    }
#endif
                }

#if SRC_DIMENSIONS == 1
                if ((flags & perform_white_is_trans) != 0)
                {
                    workspace_block[ alpha_pixel ] = (255 - pixel);
                    pixel = 0;
                }
#endif
            }

            //////////////////////////////////////////////////////////////////
            //
            // Determine it's position in the colour lookup table
            //

            if (do_cm && xyz_index < SRC_DIMENSIONS)
            {
                // Store the pixel for later use
                workspace_block[ local_rx + xyz_index ] = pixel;

                // Calculate X0, Y0, Z0, K0
                int s = strides[ xyz_index ];
                int a = mul24(s, (pixel >> N));
                workspace_block[ local_X0 + (xyz_index<<1) ] = a;

                // Calculate X1, Y1, Z1, K1
                workspace_block[ local_X1 + (xyz_index<<1) ] = a + s;
            }

            // wait for everyone in our local group to write to local memory
            barrier(CLK_LOCAL_MEM_FENCE);

            //////////////////////////////////////////////////////////////////
            //
            // Perform the colour interpolation
            //

            if (dst_pixel < dst_dimensions && px_x <= dst_x2)
            {
                if (do_cm)
                {
                    pixel = lut_fetch(0,0,0,0,0);

                    pixel += lut_fetch( 0, workspace_block[ local_rx ], workspace_block[ local_rx+1 ], workspace_block[ local_rx+2], workspace_block[ local_rx+3 ]);
#if N > 1
                    pixel += lut_fetch( 1, workspace_block[ local_rx ], workspace_block[ local_rx+1 ], workspace_block[ local_rx+2], workspace_block[ local_rx+3 ]) << 1;
#endif
#if N > 2
                    pixel += lut_fetch( 2, workspace_block[ local_rx ], workspace_block[ local_rx+1 ], workspace_block[ local_rx+2], workspace_block[ local_rx+3 ]) << 2;
#endif
                    pixel = clamp( (pixel+NROUND) >> Np, 0, 255);
                }

                int pixel_index = mad24( px_y, dst_stride, mul24(px_x,dst_pixel_stride)  );
#if ALPHA_BLEND
                pixel = pixel * workspace_block[ alpha_pixel ];
                dst_bitmap[ pixel_index + dst_pixel ] =
                mad24((255 - workspace_block[ alpha_pixel ]),dst_bitmap[ pixel_index + dst_pixel ],
                      pixel) >> 8;
#elif SRC_DIMENSIONS == 1
                if ((flags & perform_white_is_trans) != 0)
                {
                    pixel = pixel * workspace_block[ alpha_pixel ];
                    dst_bitmap[ pixel_index + dst_pixel ] =
                        mad24((255 - workspace_block[ alpha_pixel ]),dst_bitmap[ pixel_index + dst_pixel ],
                         pixel) >> 8;
                }
                else
                {
                    dst_bitmap[ pixel_index + dst_pixel ] = pixel;
                }
#else
                dst_bitmap[ pixel_index + dst_pixel ] = pixel;
#endif
            }
        }
    }
}

)CLC"
R"CLC(#if SRC_DIMENSIONS == 1

__kernel void bitblt_glyph(
                        int flags,
                        __global const unsigned char *src_bitmap,
                        int src_stride,
                        int src_x,
                        int src_y,
                        __global unsigned char *dst_bitmap,
                        int dst_stride,
                        int dst_pixel_stride,
                        int dst_dimensions,
                        int dst_x1,
                        int dst_y1,
                        int dst_x2,
                        int dst_y2,
						unsigned int colour_in )
{
	int dst_pixel = get_local_id(0);
	int pixel_mod = get_local_id(1);

    if ((flags & flip_dst_components) != 0 && dst_pixel <= 2)
    {
        dst_pixel = 2 - dst_pixel;
    }

	int col = (colour_in >> (dst_pixel*8)) & 0xff;

	for (int px_y = get_group_id(0) + dst_y1; px_y <= dst_y2; px_y += get_global_size(0) / get_local_size(0))
    {
        for (int px_x_q = (get_group_id(1) * WORKGROUP_FACTOR) + dst_x1; px_x_q <= dst_x2; px_x_q += (get_global_size(1) / get_local_size(1)) * WORKGROUP_FACTOR )
        {
            int px_x = px_x_q + pixel_mod;

            //////////////////////////////////////////////////////////////////
            //
            // Fetch the pixel
            //
            int alpha = 0;

            if (px_x <= dst_x2)
            {
				int _x;
				int _y;

				if ((flags & perform_rotate90) != 0)
				{
					// 90 degree rotation clockwise
					_x = (px_y - dst_y1);
					_y = (dst_x2 - px_x);
				}
				else if ((flags & perform_rotate180) != 0)
				{
					// 180 degree rotation clockwise
					_x = dst_x2 - px_x;
					_y = dst_y2 - px_y;
				}
				else if ((flags & perform_rotate270) != 0)
				{
					// 270 degree rotation clockwise
					_x = (dst_y2 - px_y);
					_y = (px_x - dst_x1);
				}
				else
				{
					// No rotation
					_x = (px_x - dst_x1);
					_y = (px_y - dst_y1);
				}

                // No re-size
                _x = _x + src_x;
                _y = _y + src_y;
                int pixel_index = mad24( _y, src_stride, _x );
                alpha = src_bitmap[ pixel_index ];
			}

            //////////////////////////////////////////////////////////////////
            //
            // Perform the colour interpolation
            //

            if (dst_pixel < dst_dimensions && px_x <= dst_x2)
            {
                int pixel_index = mad24( px_y, dst_stride, mul24(px_x,dst_pixel_stride)  );
                int pixel = col * (255-alpha);
                dst_bitmap[ pixel_index + dst_pixel ] =
					mad24(alpha,dst_bitmap[ pixel_index + dst_pixel ], pixel) / 255;
            }
        }
    }
}

#endif

)CLC"
R"CLC(__kernel void bitblt_matrix(
                     int flags,
                     __global const unsigned char *src_bitmap,
                     int src_stride,
                     int src_pixel_stride,
                     int src_x,
                     int src_y,
                     int src_x2,
                     int src_y2,
                     __global unsigned char *dst_bitmap,
                     int dst_stride,
                     int dst_pixel_stride,
                     int dst_dimensions,
                     int dst_x1,
                     int dst_y1,
                     int dst_x2,
                     int dst_y2,
                     __global const unsigned short *lut,
                     __constant int *strides,
                     __local  int *workspace_block_in,
                     float m_a,
                     float m_b,
                     float m_c,
                     float m_d,
                     float m_e,
                     float m_f )
{
	int xyz_index = get_local_id(0);
	int pixel_mod = get_local_id(1);
    bool do_cm = (flags & perform_no_cm) == 0;

    int src_pixel = xyz_index;
    if ((flags & flip_src_components) != 0 && src_pixel <= 2)
    {
        src_pixel = 2 - src_pixel;
    }

    int dst_pixel = xyz_index;
    if ((flags & flip_dst_components) != 0 && dst_pixel <= 2)
    {
        dst_pixel = 2 - dst_pixel;
    }
	int workspace_stride = ((flags & perform_resize_complx) != 0) ? 60 : 15;
	__local int   *workspace_block = workspace_block_in + (pixel_mod * workspace_stride);

	for (int px_y = get_group_id(0) + dst_y1; px_y <= dst_y2; px_y += get_global_size(0) / get_local_size(0))
    {
        for (int px_x_q = (get_group_id(1) * WORKGROUP_FACTOR) + dst_x1; px_x_q <= dst_x2; px_x_q += (get_global_size(1) / get_local_size(1)) * WORKGROUP_FACTOR )
        {
            int px_x = px_x_q + pixel_mod;

            //////////////////////////////////////////////////////////////////
            //
            // Fetch the pixel
            //
            int pixel = -1;

            if (px_x <= dst_x2)
            {
				float dx = m_a * px_x + m_c * px_y + m_e;
				float dy = m_b * px_x + m_d * px_y + m_f;

                if (   dx < src_x || (dx+0.5) > src_x2
                    || dy < src_y || (dy+0.5) > src_y2)
                {
                    // This outside of our source target area
                    pixel = -1;
                }
#if ALPHA_BLEND
                else if (xyz_index >= 4)
#else
                else if (xyz_index >= SRC_DIMENSIONS)
#endif
                {
                    pixel = 0;
                }
                else if ((flags & perform_resize_simple) != 0)
                {
                    // Nearest neighbour resize
                    int _x = (int)(dx + 0.5f);
                    int _y = (int)(dy + 0.5f);
                    int pixel_index = mad24( _y, src_stride, _x * src_pixel_stride );
                    pixel = src_bitmap[ pixel_index + src_pixel ];
#if ALPHA_BLEND
                    if (src_pixel == 2)
                    {
                        workspace_block[ alpha_pixel ] = src_bitmap[ pixel_index + 3 ];
                    }
#endif
                }
                else if ((flags & perform_resize_complx) != 0)
                {
                    // Cubic fit resize
                    //
                    int _x = (int)(dx);
                    int _y = (int)(dy);
                    dx = dx - (int)dx;
                    dy = dy - (int)dy;

                    __local float *workspace_float = (__local float*)workspace_block;

                    bool near_the_edge = (_y < 2 || _x < 2
                                          || _x >= src_x2- 2
                                          || _y >= src_y2 - 2);

                    if (!near_the_edge)
                    {
                        for (int y_offset = 0; y_offset < 4; ++ y_offset)
                        {
                            for (int x_offset = 0; x_offset < 4; ++ x_offset)
                            {
                                int pixel_index = mad24( _y-2 + y_offset, src_stride, (_x-2 + x_offset) * src_pixel_stride );
                                workspace_float[ x_offset + src_pixel * 8 + 4 ] = src_bitmap[ pixel_index + src_pixel ];
                            }
                            workspace_float[ src_pixel * 8 + y_offset ] = cubic_spline_fit( dx,
                                                                                           workspace_float[ src_pixel * 8 + 4],
                                                                                           workspace_float[ src_pixel * 8 + 5 ],
                                                                                           workspace_float[ src_pixel * 8 + 6 ],
                                                                                           workspace_float[ src_pixel * 8 + 7 ] );
                        }
                        pixel = clamp(cubic_spline_fit( dy,
                                                       workspace_float[ src_pixel * 8],
                                                       workspace_float[ src_pixel * 8 + 1 ],
                                                       workspace_float[ src_pixel * 8 + 2 ],
                                                       workspace_float[ src_pixel * 8 + 3 ] ),0.0f,255.0f);
                    }
                    else
                    {
                        int pixel_index = mad24( _y, src_stride, _x * src_pixel_stride );
                        pixel = src_bitmap[ pixel_index + src_pixel ];
                    }
                }
                else
                {
                    // No re-size
                    int _x = (int)(dx + 0.5f);
                    int _y = (int)(dy + 0.5f);
                    int pixel_index = mad24( _y, src_stride, _x * src_pixel_stride );
                    pixel = src_bitmap[ pixel_index + src_pixel ];
                }

#if SRC_DIMENSIONS == 1
                if ((flags & perform_white_is_trans) != 0 && pixel >= 0)
                {
                    workspace_block[ alpha_pixel ] = (255 - pixel);
                    pixel = 0;
                }
#endif
            }

#if ALPHA_BLEND
            // wait for everyone in our local group to write to local memory
            barrier(CLK_LOCAL_MEM_FENCE);

            if (src_pixel == 3)
            {
                workspace_block[ alpha_pixel ] = pixel;
            }
#endif

            // wait for everyone in our local group to write to local memory
            barrier(CLK_LOCAL_MEM_FENCE);

)CLC"
R"CLC(            //////////////////////////////////////////////////////////////////
            //
            // Determine it's position in the colour lookup table
            //

            if (do_cm && xyz_index < SRC_DIMENSIONS && pixel >= 0)
            {
                // Store the pixel for later use
                workspace_block[ local_rx + xyz_index ] = pixel;

                // Calculate X0, Y0, Z0, K0
                int s = strides[ xyz_index ];
                int a = mul24(s, (pixel >> N));
                workspace_block[ local_X0 + (xyz_index<<1) ] = a;

                // Calculate X1, Y1, Z1, K1
                workspace_block[ local_X1 + (xyz_index<<1) ] = a + s;
            }

            // wait for everyone in our local group to write to local memory
            barrier(CLK_LOCAL_MEM_FENCE);

            //////////////////////////////////////////////////////////////////
            //
            // Perform the colour interpolation
            //

            if (dst_pixel < dst_dimensions && px_x <= dst_x2 && pixel >= 0)
            {
                if (do_cm)
                {
                    pixel = lut_fetch(0,0,0,0,0);

                    pixel += lut_fetch( 0, workspace_block[ local_rx ], workspace_block[ local_rx+1 ], workspace_block[ local_rx+2], workspace_block[ local_rx+3 ]);
#if N > 1
                    pixel += lut_fetch( 1, workspace_block[ local_rx ], workspace_block[ local_rx+1 ], workspace_block[ local_rx+2], workspace_block[ local_rx+3 ]) << 1;
#endif
#if N > 2
                    pixel += lut_fetch( 2, workspace_block[ local_rx ], workspace_block[ local_rx+1 ], workspace_block[ local_rx+2], workspace_block[ local_rx+3 ]) << 2;
#endif
                    pixel = clamp( (pixel+NROUND) >> Np, 0, 255);
                }

                int pixel_index = mad24( px_y, dst_stride, mul24(px_x,dst_pixel_stride)  );
#if ALPHA_BLEND
                pixel = pixel * workspace_block[ alpha_pixel ];
                dst_bitmap[ pixel_index + dst_pixel ] =
                    mad24((255 - workspace_block[ alpha_pixel ]),dst_bitmap[ pixel_index + dst_pixel ],
                    pixel) >> 8;
#elif SRC_DIMENSIONS == 1
                if ((flags & perform_white_is_trans) != 0)
                {
                    pixel = pixel * workspace_block[ alpha_pixel ];
                    dst_bitmap[ pixel_index + dst_pixel ] =
                    mad24((255 - workspace_block[ alpha_pixel ]),dst_bitmap[ pixel_index + dst_pixel ],
                          pixel) >> 8;
                }
                else
                {
                    dst_bitmap[ pixel_index + dst_pixel ] = pixel;
                }
#else
                dst_bitmap[ pixel_index + dst_pixel ] = pixel;
#endif
            }
        }
    }
}

)CLC";