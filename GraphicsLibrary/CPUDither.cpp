//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "CPUDither.h"
#include "OrderedScreenCollection.h"
#include "Image.h"
#include "LinearizationCollection.h"


CCPUDither::CCPUDither(void)
{
}


CCPUDither::~CCPUDither(void)
{
}

void CCPUDither::dither( CImage* src_image, CImage** dst_images, COrderedScreenCollection& screens, CLinearizationCollection* linearization, int scale )
{
	// Checks
	size_t planes = screens.getPlanes();
	if (planes != src_image->getComponents())
	{
		throw jett_exception(JETT_INVALID_ARGUMENT,0,"The planes parameter must be the same as the number of colour components in the source image");
	}

	// Is each image of the correct type?
	for (size_t i = 0; i < screens.getPlanes(); ++ i)
	{
		switch (dst_images[i]->getType())
		{
		case image_mono1:
		case image_mono2:
		case image_mono4:
		case image_mono:
			break;
		default:
			throw jett_exception(JETT_INVALID_ARGUMENT,0,"The dst_images must be of type image_mono1, image_mono2, image_mono4 or image_mono8");
		}

		if (dst_images[i]->getType() != dst_images[0]->getType())
		{
			throw jett_exception(JETT_INVALID_ARGUMENT,0,"The dst_images must be all of the same type (e.g. all image_mono1)");
		}

		if (	dst_images[i]->getWidth() != src_image->getWidth()
			||  dst_images[i]->getHeight() != src_image->getHeight())
		{
			throw jett_exception(JETT_INVALID_ARGUMENT,0,"The dst_images must be the same size as src_image");
		}
	}

    
    CLinearizationCollection defaultCollection;
    if (linearization)
    {
        if (linearization->size() != src_image->getComponents())
        {
            throw jett_exception(JETT_INVALID_ARGUMENT,0,"The linearization parameter must have the same the number of curves as the colour components in the source image");
        }
    }
    else
    {
        // Build the default collection
        linearization = &defaultCollection;
        CLinearization l;
        for (size_t i = 0; i < planes; ++ i)
        {
            defaultCollection.push_back(l);
        }
    }
    
    unsigned char *linearizations[4];
    for (size_t i = 0;i < planes; ++ i)
    {
        linearizations[i] = linearization->get_linearization(i);
    }

	//
	// Dither this bitmap using the ordered screen
	//

	// Get the output bits per pixel
	int max_scale = 255;
	int bpp = 8;
	int interlude = 1;
	switch (dst_images[0]->getType())
	{
	case image_mono1:
		max_scale = 1;
		bpp = 1;
		interlude = 8;
		break;
	case image_mono2:
		max_scale = 3;
		bpp = 2;
		interlude = 4;
		break;
	case image_mono4:
		max_scale = 15;
		bpp = 4;
		interlude = 2;
		break;
	case image_mono:
		max_scale = 255;
		bpp = 8;
		interlude = 1;
		break;
	}

	if (scale > max_scale || scale < 1)
	{
        throw jett_exception( JETT_INVALID_ARGUMENT, 0, "The parameter scale is too large or less than 1" );
	}


	// Extract the screen data
	const int* screen_data[4];
	int screen_width[4];
	int screen_height[4];
	double screen_quantizer[4];
	unsigned char *p_dst[4];
	for (size_t i = 0; i< planes; ++i)
	{
		screen_data[i] = screens.get_screen(i);
		screen_width[i]  = screens.getWidth(i);
		screen_height[i] = screens.getHeight(i);
		screen_quantizer[i] = screens.getQuantizer(i);

		p_dst[i] = dst_images[i]->lockData( false );
	}


	//
	// Perform the dithering
	//
	int height = src_image->getHeight();
	int width = (src_image->getWidth() + interlude -1) / interlude * interlude;
	bool skip = src_image->getType() == image_bgra 
			 || src_image->getType() == image_rgba; 
	const unsigned char *p_src = src_image->lockData( true );

#pragma omp parallel for
	for (int y = 0; y < height; ++ y)
	{
		const unsigned char *p_src_pixel = p_src + y * src_image->getStride();
		unsigned char *p_dst_pixel[4];
		int px_acc[4];
		for (size_t plane = 0; plane < planes; ++ plane)
		{
			p_dst_pixel[plane] = p_dst[plane] + y * dst_images[0]->getStride();
			px_acc[plane] = 0;
		}

		for (int x = 0; x < width; ++ x)
		{
			for (size_t plane = 0; plane < planes; ++ plane)
			{
				int rank = screen_data[plane][ (y % screen_height[plane]) 
					* screen_width[plane] + (x % screen_width[plane]) ];

                unsigned char pixel = linearizations[plane][*p_src_pixel];

				px_acc[plane] <<= bpp;
				px_acc[plane] |= static_cast<int>( (rank + pixel) / screen_quantizer[plane] ) * scale;

				if ((x % interlude) == (interlude-1))
				{
					*p_dst_pixel[plane] =px_acc[plane];
					px_acc[plane] = 0;
					++ p_dst_pixel[plane];
				}

				++ p_src_pixel;
			}
			if (skip)
			{
				++ p_src_pixel;
			}
		}
	}

	// Release the bitmaps
	src_image->unlockData();
	for (size_t i = 0; i< planes; ++i)
	{
		dst_images[i]->unlockData();
	}
}


void CCPUDither::dither( CImage* src_image, COrderedScreenCollection& screens, CLinearizationCollection* linearization, int scale )
{
	// Checks
	size_t planes = screens.getPlanes();
	if (planes != src_image->getComponents())
	{
		throw jett_exception(JETT_INVALID_ARGUMENT,0,"The planes parameter must be the same as the number of colour components in the source image");
	}

	if (scale > 255 || scale < 1)
	{
        throw jett_exception( JETT_INVALID_ARGUMENT, 0, "The parameter scale is too large or less than 1" );
	}
    
    CLinearizationCollection defaultCollection;
    if (linearization)
    {
        if (linearization->size() != src_image->getComponents())
        {
            throw jett_exception(JETT_INVALID_ARGUMENT,0,"The linearization parameter must have the same the number of curves as the colour components in the source image");
        }
    }
    else
    {
        // Build the default collection
        linearization = &defaultCollection;
        CLinearization l;
        for (size_t i = 0; i < planes; ++ i)
        {
            defaultCollection.push_back(l);
        }
    }
    
    unsigned char *linearizations[4];
    for (size_t i = 0;i < planes; ++ i)
    {
        linearizations[i] = linearization->get_linearization(i);
    }

	// Extract the screen data
	const int* screen_data[4];
	int screen_width[4];
	int screen_height[4];
	double screen_quantizer[4];
	for (size_t i = 0; i< planes; ++i)
	{
		screen_data[i] = screens.get_screen(i);
		screen_width[i]  = screens.getWidth(i);
		screen_height[i] = screens.getHeight(i);
		screen_quantizer[i] = screens.getQuantizer(i);
	}


	//
	// Perform the dithering
	//
	int height = src_image->getHeight();
	int width = src_image->getWidth();
	unsigned char *p_src = src_image->lockData( false );
	bool skip = src_image->getType() == image_bgra 
			|| src_image->getType() == image_rgba; 

#pragma omp parallel for
	for (int y = 0; y < height; ++ y)
	{
		unsigned char *p_src_pixel = p_src + y * src_image->getStride();
		for (int x = 0; x < width; ++ x)
		{
			for (size_t plane = 0; plane < planes; ++ plane)
			{
				int rank = screen_data[plane][ (y % screen_height[plane]) 
					* screen_width[plane] + (x % screen_width[plane]) ];

                unsigned char pixel = linearizations[plane][*p_src_pixel];
				*p_src_pixel = static_cast<int>( (rank + pixel) / screen_quantizer[plane]) * scale;

				++ p_src_pixel;
			}
			if (skip)
			{
				++ p_src_pixel;
			}
		}
	}

	// Release the bitmaps
	src_image->unlockData();
}
