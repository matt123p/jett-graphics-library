//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

//
// Dither in-place
//
__kernel void dither_in_place(  
                     __global unsigned char *src_bitmap,
                     int src_stride,
                     int src_pixel_stride,
                     int src_width,
					 int src_height,
					 __constant int *screen_data,
#ifdef LINEARIZE
                     __constant unsigned char *lin_data,
#endif
					 float screen_quantizer,
					 int dst_scale )
{    
	int base = get_local_id(2) * 3;
#ifdef LINEARIZE
    __constant unsigned char *lin = lin_data + 256 * get_local_id(2);
#endif
	__constant int* screen = screen_data + screen_data[base];
	int screen_width = screen_data[base + 1];
	int screen_height = screen_data[base + 2];

    for (int y = get_global_id(1); y < src_height; y += get_global_size(1) )
    {         
		for (int x = get_global_id(0); x < src_width; x += get_global_size(0) )
		{       
			__global unsigned char *p_src_pixel = src_bitmap + y * src_stride + x * src_pixel_stride + get_local_id(2);

			int rank = screen[ mad24( y % screen_height, screen_width, x % screen_width ) ];
            
#ifndef LINEARIZE
			rank += *p_src_pixel;
#else
			rank += lin[ *p_src_pixel  ];
#endif
            
			*p_src_pixel = ((unsigned char)( rank / screen_quantizer)) * dst_scale;
        }
    }
}

//
// Dither to separated images (8bpp)
//
__kernel void dither_sep8(  
                     __global unsigned char *src_bitmap,
                     int src_stride,
                     int src_pixel_stride,
                     int src_width,
					 int src_height,
					 __constant int *screen_data,
#ifdef LINEARIZE
                     __constant unsigned char *lin_data,
#endif
					 float screen_quantizer,
					 int dst_scale,
					 int dst_stride,
					__global unsigned char *dst_bitmap1,
					__global unsigned char *dst_bitmap2,
					__global unsigned char *dst_bitmap3,
					__global unsigned char *dst_bitmap4 )
{    
	int base = get_local_id(2) * 3;
#ifdef LINEARIZE
    __constant unsigned char *lin = lin_data + 256 * get_local_id(2);
#endif

	__constant int* screen = screen_data + screen_data[base];
	int screen_width = screen_data[base + 1];
	int screen_height = screen_data[base + 2];
	__global unsigned char*dst_bitmap;

	switch (get_local_id(2))
	{
	case 0:
		dst_bitmap = dst_bitmap1;
		break;
	case 1:
		dst_bitmap = dst_bitmap2;
		break;
	case 2:
		dst_bitmap = dst_bitmap3;
		break;
	case 3:
		dst_bitmap = dst_bitmap4;
		break;
	}

    for (int y = get_global_id(1); y < src_height; y += get_global_size(1) )
    {         
		for (int x = get_global_id(0); x < src_width; x += get_global_size(0) )
		{       
			__global unsigned char *p_src_pixel = src_bitmap + y * src_stride + x * src_pixel_stride + get_local_id(2);
			__global unsigned char *p_dst_pixel = dst_bitmap + y * dst_stride + x;

			int rank = screen[ mad24( y % screen_height, screen_width, x % screen_width ) ];
            
#ifndef LINEARIZE
			rank += *p_src_pixel;
#else
			rank += lin[ *p_src_pixel ];
#endif

			*p_dst_pixel = ((unsigned char)( rank / screen_quantizer)) * dst_scale;
		}
    }
}


//
// Dither to separated images (4bpp)
//
__kernel void dither_sep4(  
                     __global unsigned char *src_bitmap,
                     int src_stride,
                     int src_pixel_stride,
                     int src_width,
					 int src_height,
					 __constant int *screen_data,
#ifdef LINEARIZE
                     __constant unsigned char *lin_data,
#endif
					 float screen_quantizer,
					 int dst_scale,
					 int dst_stride,
					__global unsigned char *dst_bitmap1,
					__global unsigned char *dst_bitmap2,
					__global unsigned char *dst_bitmap3,
					__global unsigned char *dst_bitmap4,
					__local  int *workspace )
{    
	int base = get_local_id(2) * 3;
#ifdef LINEARIZE
    __constant unsigned char *lin = lin_data + 256 * get_local_id(2);
#endif

	__constant int* screen = screen_data + screen_data[base];
	int screen_width = screen_data[base + 1];
	int screen_height = screen_data[base + 2];
	__global unsigned char*dst_bitmap;
	switch (get_local_id(2))
	{
	case 0:
		dst_bitmap = dst_bitmap1;
		break;
	case 1:
		dst_bitmap = dst_bitmap2;
		break;
	case 2:
		dst_bitmap = dst_bitmap3;
		break;
	case 3:
		dst_bitmap = dst_bitmap4;
		break;
	}

	__local int * store = workspace + get_local_id(0) 
								    + get_local_id(1) * 8 + get_local_id(2) * 64;

    for (int y = get_global_id(1); y < src_height; y += get_global_size(1) )
    {         
		for (int x = get_global_id(0); x < src_width; x += get_global_size(0) )
		{       
			__global unsigned char *p_src_pixel = src_bitmap + y * src_stride + x * src_pixel_stride + get_local_id(2);
			__global unsigned char *p_dst_pixel = dst_bitmap + y * dst_stride + (x>>1);

			int rank = screen[ mad24( y % screen_height, screen_width, x % screen_width ) ];
#ifndef LINEARIZE
			rank += *p_src_pixel;
#else
			rank += lin[ *p_src_pixel ];
#endif

			 *store = ((unsigned char)( rank / screen_quantizer)) * dst_scale;

			 // Wait..
			 barrier(CLK_LOCAL_MEM_FENCE);

			 if ((x&1)==0)
			 {
				*p_dst_pixel = store[0] << 4 | store[1];
			 }

			 barrier(CLK_LOCAL_MEM_FENCE);
        }
    }
}




//
// Dither to separated images (2bpp)
//
__kernel void dither_sep2(  
                     __global unsigned char *src_bitmap,
                     int src_stride,
                     int src_pixel_stride,
                     int src_width,
					 int src_height,
					 __constant int *screen_data,
#ifdef LINEARIZE
                     __constant unsigned char *lin_data,
#endif
                    float screen_quantizer,
					 int dst_scale,
					 int dst_stride,
					__global unsigned char *dst_bitmap1,
					__global unsigned char *dst_bitmap2,
					__global unsigned char *dst_bitmap3,
					__global unsigned char *dst_bitmap4,
					__local  int *workspace )
{    
	int base = get_local_id(2) * 3;
#ifdef LINEARIZE
    __constant unsigned char *lin = lin_data + 256 * get_local_id(2);
#endif

	__constant int* screen = screen_data + screen_data[base];
	int screen_width = screen_data[base + 1];
	int screen_height = screen_data[base + 2];
	__global unsigned char*dst_bitmap;
	switch (get_local_id(2))
	{
	case 0:
		dst_bitmap = dst_bitmap1;
		break;
	case 1:
		dst_bitmap = dst_bitmap2;
		break;
	case 2:
		dst_bitmap = dst_bitmap3;
		break;
	case 3:
		dst_bitmap = dst_bitmap4;
		break;
	}

	__local int * store = workspace + get_local_id(0) 
								    + get_local_id(1) * 8 + get_local_id(2) * 64;

    for (int y = get_global_id(1); y < src_height; y += get_global_size(1) )
    {         
		for (int x = get_global_id(0); x < src_width; x += get_global_size(0) )
		{       
			__global unsigned char *p_src_pixel = src_bitmap + y * src_stride + x * src_pixel_stride + get_local_id(2);
			__global unsigned char *p_dst_pixel = dst_bitmap + y * dst_stride + (x>>2);

			int rank = screen[ mad24( y % screen_height, screen_width, x % screen_width ) ];
#ifndef LINEARIZE
			rank += *p_src_pixel;
#else
			rank += lin[ *p_src_pixel ];
#endif

			 *store = ((unsigned char)( rank / screen_quantizer)) * dst_scale;

			 // Wait..
			 barrier(CLK_LOCAL_MEM_FENCE);

			 if ((x&3)==0)
			 {
				*p_dst_pixel = (store[0] << 6) | (store[1] << 4) | (store[2] << 2) | store[3];
			 }

			 barrier(CLK_LOCAL_MEM_FENCE);
        }
    }
}



//
// Dither to separated images (1bpp)
//
__kernel void dither_sep1(  
                     __global unsigned char *src_bitmap,
                     int src_stride,
                     int src_pixel_stride,
                     int src_width,
					 int src_height,
					 __constant int *screen_data,
#ifdef LINEARIZE
                     __constant unsigned char *lin_data,
#endif
					 float screen_quantizer,
					 int dst_scale,
					 int dst_stride,
					__global unsigned char *dst_bitmap1,
					__global unsigned char *dst_bitmap2,
					__global unsigned char *dst_bitmap3,
					__global unsigned char *dst_bitmap4,
					__local  int *workspace )
{    
	int base = get_local_id(2) * 3;
#ifdef LINEARIZE
    __constant unsigned char *lin = lin_data + 256 * get_local_id(2);
#endif

	__constant int* screen = screen_data + screen_data[base];
	int screen_width = screen_data[base + 1];
	int screen_height = screen_data[base + 2];
	__global unsigned char*dst_bitmap;
	switch (get_local_id(2))
	{
	case 0:
		dst_bitmap = dst_bitmap1;
		break;
	case 1:
		dst_bitmap = dst_bitmap2;
		break;
	case 2:
		dst_bitmap = dst_bitmap3;
		break;
	case 3:
		dst_bitmap = dst_bitmap4;
		break;
	}

	__local int * store = workspace + get_local_id(0) 
								    + get_local_id(1) * 8 + get_local_id(2) * 64;

    for (int y = get_global_id(1); y < src_height; y += get_global_size(1) )
    {         
		for (int x = get_global_id(0); x < src_width; x += get_global_size(0) )
		{       
			__global unsigned char *p_src_pixel = src_bitmap + y * src_stride + x * src_pixel_stride + get_local_id(2);
			__global unsigned char *p_dst_pixel = dst_bitmap + y * dst_stride + (x>>3);

			int rank = screen[ mad24( y % screen_height, screen_width, x % screen_width ) ];
#ifndef LINEARIZE
			rank += *p_src_pixel;
#else
			rank += lin[ *p_src_pixel ];
#endif

			 *store = ((unsigned char)( rank / screen_quantizer)) * dst_scale;

			 // Wait..
			 barrier(CLK_LOCAL_MEM_FENCE);

			 if ((x&7)==0)
			 {
				*p_dst_pixel = (store[0] << 7) | (store[1] << 6) | (store[2] << 5) | (store[3] << 4)
					| (store[4] << 3) | (store[5] << 2) | (store[6] << 1) | store[7];
			 }

			 barrier(CLK_LOCAL_MEM_FENCE);
        }
    }
}






