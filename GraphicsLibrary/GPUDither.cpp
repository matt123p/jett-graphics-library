//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "GPUDither.h"
#include "OrderedScreenCollection.h"
#include "Image.h"
#include "GPUProcessor.h"
#include "LinearizationCollection.h"
#include "Linearization.h"

#include "Dither.cl.h"

namespace
{
	size_t round_up_work_size(cl_int value, size_t multiple)
	{
		size_t size = static_cast<size_t>(std::max<cl_int>(1, value));
		return ((size + multiple - 1) / multiple) * multiple;
	}
}

CGPUDither::CGPUDither(CGPUProcessor* cl)
	: m_cl( cl )
{
    // Do we support linearization?
    m_linearization = true;
    
    // Compile the program
	stringCollection defs;

    // Turn on linearization support
    if (m_linearization)
    {
        defs[ "LINEARIZE" ] = "1";
    }
    
	m_program.create( *m_cl, (unsigned char*)Dither_cl, sizeof( Dither_cl ), &defs, "-cl-fp32-correctly-rounded-divide-sqrt" );

    // Now create the individual kernel
    m_dither_in_place_kernel.create( m_program, "dither_in_place" );
	m_dither_sep8_kernel.create( m_program, "dither_sep8" );
	m_dither_sep4_kernel.create( m_program, "dither_sep4" );
	m_dither_sep2_kernel.create( m_program, "dither_sep2" );
	m_dither_sep1_kernel.create( m_program, "dither_sep1" );

}


CGPUDither::~CGPUDither(void)
{
}

void CGPUDither::dither( CImage* src_image, CImage** dst_images, COrderedScreenCollection& screens, CLinearizationCollection* linearization, int scale )
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

	//
	// Dither this bitmap using the ordered screen
	//

	// Get the output bits per pixel
	cl_int bpp = 8;
	int max_scale = 255;
	CLKernel* k = &m_dither_sep8_kernel;
	switch (dst_images[0]->getType())
	{
	case image_mono1:
		max_scale = 1;
		bpp = 1;
		k = &m_dither_sep1_kernel;
		break;
	case image_mono2:
		max_scale = 3;
		bpp = 2;
		k = &m_dither_sep2_kernel;
		break;
	case image_mono4:
		max_scale = 15;
		bpp = 4;
		k = &m_dither_sep4_kernel;
		break;
	case image_mono:
		max_scale = 255;
		bpp = 8;
		k = &m_dither_sep8_kernel;
		break;
	}

	if (scale > max_scale || scale < 1)
	{
        throw jett_exception( JETT_INVALID_ARGUMENT, 0, "The parameter scale is too large or less than 1" );
	}


	// Extract the bitmap data
	cl_mem p_dst[4]= { NULL };
	for (size_t i = 0; i< planes; ++i)
	{
		p_dst[i] = dst_images[i]->lockCLData( m_cl, false );
	}
	
	// Extract the screen data
	cl_mem screen_data = screens.get_screen_cl( m_cl );
	cl_float screen_quantizer = static_cast<float>(screens.getQuantizer(0));

	//
	// Perform the dithering
	//
	cl_int height = src_image->getHeight();
	cl_int width = src_image->getWidth();

	cl_mem src_bitmap_cl = src_image->lockCLData( m_cl, true );
	cl_int src_stride = static_cast<cl_int>(src_image->getStride());
	cl_int src_pixel_stride = static_cast<cl_int>(src_image->getPixelStride());
	cl_int dst_scale = scale;

	cl_int dst_stride = static_cast<cl_int>(dst_images[0]->getStride());

    k->setKernelArg(0,  sizeof(cl_mem), &src_bitmap_cl);
    k->setKernelArg(1,  sizeof(cl_int), &src_stride);
    k->setKernelArg(2,  sizeof(cl_int), &src_pixel_stride);
    k->setKernelArg(3,  sizeof(cl_int), &width);
    k->setKernelArg(4,  sizeof(cl_int), &height);
    k->setKernelArg(5,  sizeof(cl_mem), &screen_data);
    
    int a = 0;
	CLinearizationCollection defaultCollection;
    if (m_linearization)
    {
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

        k->setKernelArg(6,  sizeof(cl_mem), linearization->get_linearization_cl(m_cl));
        a = 1;
    }
    
	k->setKernelArg(6 + a,  sizeof(cl_float), &screen_quantizer);
	k->setKernelArg(7 + a,  sizeof(cl_int), &dst_scale);
    k->setKernelArg(8 + a,  sizeof(cl_int), &dst_stride);
	k->setKernelArg(9 + a,  sizeof(cl_mem), &p_dst[0]);
	k->setKernelArg(10 + a, sizeof(cl_mem), &p_dst[1]);
	k->setKernelArg(11 + a, sizeof(cl_mem), &p_dst[2]);
	k->setKernelArg(12 + a, sizeof(cl_mem), &p_dst[3]);

	if (bpp != 8)
	{
		k->setKernelArg(13 + a, 64 * planes * sizeof(int), 0 );
	}

	int dims = 3;
    size_t local[3] = { 8, 8, planes };
    size_t global[3] = {
		round_up_work_size(width, local[0]),
		round_up_work_size(height, local[1]),
		planes
	};
    
    m_cl->enqueueNDRangeKernel( *k, dims, NULL, global, local );
    m_cl->finish();


	// Unlock the data
	src_image->unlockCLData( m_cl );
	for (size_t i = 0; i< planes; ++i)
	{
		dst_images[i]->unlockCLData( m_cl );
	}

}


void CGPUDither::dither( CImage* src_image, COrderedScreenCollection& screens, CLinearizationCollection* linearization, int scale )
{
	// Checks
	size_t planes = screens.getPlanes();
	if (screens.getPlanes() != src_image->getComponents())
	{
		throw jett_exception(JETT_INVALID_ARGUMENT,0,"The planes parameter must be the same as the number of colour components in the source image");
	}

	if (scale > 255 || scale < 1)
	{
        throw jett_exception( JETT_INVALID_ARGUMENT, 0, "The parameter scale is too large or less than 1" );
	}

	// Extract the screen data
	cl_mem screen_data = screens.get_screen_cl( m_cl );
	cl_float screen_quantizer = static_cast<float>(screens.getQuantizer(0));

	//
	// Perform the dithering
	//
	cl_int height = src_image->getHeight();
	cl_int width = src_image->getWidth();

	cl_mem src_bitmap_cl = src_image->lockCLData( m_cl, false );
	cl_int src_stride = static_cast<cl_int>(src_image->getStride());
    cl_int src_pixel_stride = static_cast<cl_int>(src_image->getPixelStride());
	cl_int dst_scale = scale;

    m_dither_in_place_kernel.setKernelArg(0,  sizeof(cl_mem), &src_bitmap_cl);
    m_dither_in_place_kernel.setKernelArg(1,  sizeof(cl_int), &src_stride);
    m_dither_in_place_kernel.setKernelArg(2,  sizeof(cl_int), &src_pixel_stride);
    m_dither_in_place_kernel.setKernelArg(3,  sizeof(cl_int), &width);
    m_dither_in_place_kernel.setKernelArg(4,  sizeof(cl_int), &height);
    m_dither_in_place_kernel.setKernelArg(5,  sizeof(cl_mem), &screen_data);
    
    int a = 0;
	CLinearizationCollection defaultCollection;
    if (m_linearization)
    {
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
        
        m_dither_in_place_kernel.setKernelArg(6,  sizeof(cl_mem), linearization->get_linearization_cl(m_cl));
        a = 1;
    }

	m_dither_in_place_kernel.setKernelArg(6 + a,  sizeof(cl_float), &screen_quantizer);
	m_dither_in_place_kernel.setKernelArg(7 + a,  sizeof(cl_int), &dst_scale);

	int dims = 3;
    size_t local[3] = { 8, 8, planes };
    size_t global[3] = {
		round_up_work_size(width, local[0]),
		round_up_work_size(height, local[1]),
		planes
	};
    
    m_cl->enqueueNDRangeKernel( m_dither_in_place_kernel, dims, NULL, global, local );
    m_cl->finish();
    
    src_image->unlockCLData(m_cl);
}
