//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//



#include "StdAfx.h"
#include <stdio.h>
#include <algorithm>
#include "jett.h"
#include "GPUPolygon.h"
#include "Image.h"
#include <math.h>

// Here is the OpenCL program
#include "Polygon.cl.h"

CGPUPolygon::CGPUPolygon(CGPUProcessor* cl, int oversample)
: m_cl(cl)
{
    m_oversample = std::max(1,oversample);
    
    // The maximum number of nodes
    m_max_nodes = 1024;

    // The cache for these nodes to upload in to
    m_polyX_cl = m_cl->createBuffer( CL_MEM_READ_ONLY, sizeof(float) * m_max_nodes, NULL );
    m_polyY_cl = m_cl->createBuffer( CL_MEM_READ_ONLY, sizeof(float) * m_max_nodes, NULL );

    
    // Now build the program
    char buffer[ 256 ];
    
    stringCollection defs;    
    _snprintf_s( buffer, sizeof(buffer), "%d", m_oversample );
    defs[ "OVERSAMPLE" ] = buffer;
    
    _snprintf_s( buffer, sizeof(buffer), "%d", m_oversample*m_oversample );
    defs[ "OVERSAMPLE2" ] = buffer;

	_snprintf_s( buffer, sizeof(buffer), "%d", std::max(0,m_oversample-1) );
    defs[ "ADJUST" ] = buffer;

    // Compile the program
    m_program.create( *m_cl, (unsigned char*)Polygon_cl, sizeof( Polygon_cl), &defs, "-cl-fp32-correctly-rounded-divide-sqrt" );

    // Now create the individual kernels
    if (m_oversample > 1)
    {
        m_poly_kernel.create( m_program, "fill_aa" );
    }
    else
    {
        m_poly_kernel.create( m_program, "fill" );        
    }
}

CGPUPolygon::~CGPUPolygon()
{
    m_cl->releaseMemObject( m_polyX_cl );
    m_cl->releaseMemObject( m_polyY_cl );   
}

void CGPUPolygon::fill( CImage *dst_image, unsigned char *colour, jett_point *points, int number_of_corners )
{
    // Have we a big enough node cache?
    if (number_of_corners > m_max_nodes)
    {
        throw jett_exception( JETT_INVALID_ARGUMENT, 0, "This Polygon has too many corners" );
    }
        
    // Setup the kernel
    cl_mem dst_bitmap_cl = dst_image->lockCLData( m_cl, false );
    cl_int dst_stride = static_cast<cl_int>(dst_image->getStride());
    cl_int dst_pixel_stride = static_cast<cl_int>(dst_image->getPixelStride());
    cl_int dst_x1 = 0;
    cl_int dst_x2 = 0;
	cl_int image_top = dst_image->getHeight() - 1;
    cl_int image_bottom = 0;
	cl_int image_left = dst_image->getWidth() - 1;
	cl_int image_right = 0;
    cl_int top_edge_y = 0;
    cl_uint colour_cl = 0;
    cl_int dst_width = dst_image->getWidth();
    cl_int number_of_corners_cl = number_of_corners;
    
    for (int p = 0; p < dst_image->getComponents(); ++ p)
    {
        colour_cl = colour_cl | (colour[p] << (8*p));
    }
    
    // Do we need to pre-parse the polygon data?
    float *px = new float[ number_of_corners ];
    float *py = new float[ number_of_corners ];
    for (int i = 0; i < number_of_corners; ++ i)
    {
        px[i] = points[i].x * m_oversample;
        py[i] = points[i].y * m_oversample;
        image_top = std::min( image_top, static_cast<int>(py[i]));
        if (m_oversample > 1)
        {
            image_bottom = std::max( image_bottom, static_cast<int>(py[i]));
        }
        else
        {
            image_bottom = std::max( image_bottom, static_cast<int>(py[i] + 0.999f) + 1);
        }
        image_left = std::min( image_left, static_cast<int>(px[i]));
        image_right = std::max( image_right, static_cast<int>(px[i]));
    }
    

    // Apply the clipping rect
    int clip_x1, clip_y1, clip_x2, clip_y2;
    dst_image->clipping( clip_x1, clip_y1, clip_x2, clip_y2 );
    
    if (m_oversample > 1)
    {
        top_edge_y = std::max( clip_y1 * m_oversample, image_top );
        image_top = (std::max( clip_y1 * m_oversample, image_top ) / m_oversample) * m_oversample;
        image_bottom = (std::min( clip_y2 * m_oversample, image_bottom ) / m_oversample) * m_oversample + (m_oversample-1);
    }
    else
    {
        image_top = std::max( clip_y1, image_top );
        image_bottom = std::min( image_bottom, clip_y2 );
        top_edge_y = image_top;
    }
    if (m_oversample > 1)
    {
        dst_x1 = (std::max( clip_x1 * m_oversample, image_left ) / m_oversample) * m_oversample;
        dst_x2 = (std::min( clip_x2 * m_oversample + (m_oversample - 1), image_right ) / m_oversample) * m_oversample + (m_oversample - 1);
    }
    else
    {
        dst_x1 = clip_x1 * m_oversample;
        dst_x2 = clip_x2 * m_oversample;
    }

    // Upload the polygon data
    m_cl->enqueueWriteBuffer(m_polyX_cl,CL_TRUE,0,sizeof(float) * number_of_corners,px );
    m_cl->enqueueWriteBuffer(m_polyY_cl,CL_TRUE,0,sizeof(float) * number_of_corners,py );

    delete [] px;
    delete [] py;
    int workgroup_factor = 32;
    
    m_poly_kernel.setKernelArg(0,  sizeof(cl_mem), &dst_bitmap_cl);
    m_poly_kernel.setKernelArg(1,  sizeof(cl_int), &dst_stride);
    m_poly_kernel.setKernelArg(2,  sizeof(cl_int), &dst_pixel_stride);
    m_poly_kernel.setKernelArg(3,  sizeof(cl_int), &dst_x1);
    m_poly_kernel.setKernelArg(4,  sizeof(cl_int), &image_top);
    m_poly_kernel.setKernelArg(5,  sizeof(cl_int), &dst_x2);
    m_poly_kernel.setKernelArg(6,  sizeof(cl_int), &image_bottom);
    m_poly_kernel.setKernelArg(7,  sizeof(cl_int), &top_edge_y);
    m_poly_kernel.setKernelArg(8,  sizeof(cl_uint), &colour_cl);
    m_poly_kernel.setKernelArg(9,  sizeof(cl_int), &dst_width);
    m_poly_kernel.setKernelArg(10,  sizeof(cl_int), &number_of_corners_cl);
    m_poly_kernel.setKernelArg(11,  sizeof(cl_mem), &m_polyX_cl);
    m_poly_kernel.setKernelArg(12, sizeof(cl_mem), &m_polyY_cl);
    m_poly_kernel.setKernelArg(13, 32 * sizeof(int) * workgroup_factor, 0 );

    if (m_oversample > 1)
    {
        m_poly_kernel.setKernelArg(14, sizeof(cl_int) * workgroup_factor * m_oversample, 0 );
    }
    
    int dims = 1;
    size_t local[1] = { static_cast<size_t>(workgroup_factor) };
    size_t global[1] = { static_cast<size_t>(512 * workgroup_factor) };
    
    m_cl->enqueueNDRangeKernel( m_poly_kernel, dims, NULL, global, local );
    m_cl->finish();
    
    dst_image->unlockCLData(m_cl);
}

