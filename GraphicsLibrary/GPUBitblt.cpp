//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include <stdio.h>
#include <algorithm>
#include "jett.h"
#include "GPUBitblt.h"
#include "Image.h"
#include <math.h>
#include "transform.h"

// Here is the OpenCL program
#include "Colour.cl.h"

namespace
{
    size_t get_bitblt_local_y(size_t xyz_size, size_t desired_local_y, size_t kernel_workgroup_size)
    {
        size_t max_local_y = kernel_workgroup_size / std::max<size_t>(xyz_size, 1);
        max_local_y = std::max<size_t>(max_local_y, 1);
        return std::min(desired_local_y, max_local_y);
    }
}


CGPUBitblt::CGPUBitblt(CGPUProcessor* cl, int src_dimensions, bool alpha )
: m_cl(cl)
{
    m_alpha = alpha;
    m_dimensions_in = src_dimensions;    
    
    // We only support 1,3 or 4 dimensions in
	switch (m_dimensions_in)
	{
        case 1:
            m_workgroup_factor = 64;
            break;
        case 3:
            m_workgroup_factor = 32;
            break;
        case 4:
            m_workgroup_factor = 32;
            break;
        default:
            throw jett_exception( JETT_INVALID_PROFILE, 0, "Invalid input colour profile, unsupported number of colours" );
	}
    
    // Now build the program
    char buffer[ 256 ];
    
    stringCollection defs;    
    
    sprintf_s( buffer, "%d", m_workgroup_factor );
    defs[ "WORKGROUP_FACTOR" ] = buffer;
    
    sprintf_s( buffer, "%d", m_dimensions_in );
    defs[ "SRC_DIMENSIONS" ] = buffer;

    sprintf_s( buffer, "%d", m_alpha );
    defs[ "ALPHA_BLEND" ] = buffer;
    
    sprintf_s( buffer, "%d", TRANSFORM_N );
    defs[ "N" ] = buffer;

    sprintf_s( buffer, "%d", TRANSFORM_N+TRANSFORM_FP_PRECISION );
    defs[ "Np" ] = buffer;
    
    sprintf_s( buffer, "%d", (1<<(TRANSFORM_N+TRANSFORM_FP_PRECISION-1))-1 );
    defs[ "NROUND" ] = buffer;
    
    
    m_program.create( *m_cl, (unsigned char*)Colour_cl, sizeof( Colour_cl), &defs );
    
    // Now create the individual kernels
    m_bitblt_kernel.create( m_program, "bitblt" );
    m_bitblt_matrix_kernel.create( m_program, "bitblt_matrix" );
    
	if (m_dimensions_in == 1)
	{
		m_bitblt_glyph_kernel.create( m_program, "bitblt_glyph" );
	}
    
    // Calculate the opta table and upload as a shared resource
    // The dimensions calculation
    int dst_dimensions[] = { 1, 3, 4 };
    for (int i = 0; i < 3; ++ i)
    {
        char opta_name[ 64 ];
        _snprintf_s( opta_name, sizeof(opta_name), "transform_%d_%d_%d_opta", TRANSFORM_AXIS_SIZE, m_dimensions_in, dst_dimensions[i] );
        // Shared resources
        cl_mem opta_cl = m_cl->get_shared_resource( opta_name, NULL, 0 );
        if (!opta_cl)
        {
            int *opta = new int[ m_dimensions_in ];
            opta[0] = dst_dimensions[i];
            for (int i = 1; i < m_dimensions_in; ++i)
            {
                opta[i] = opta[i-1] * TRANSFORM_AXIS_SIZE;
            }
            opta_cl = m_cl->get_shared_resource( opta_name, opta, sizeof( opta[0] ) * m_dimensions_in );
            delete [] opta;
        }
    }
}

CGPUBitblt::~CGPUBitblt()
{
}


// Convert a bitmap on GPU (generic version)
void CGPUBitblt::copy( CTransform *trans, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap )
{
    // Check some assumptions
    if (src_bitmap->getComponents() != m_dimensions_in)
    {
        throw jett_exception( JETT_INTERNAL_ERROR,0, "This CBitblt object was not built for this source image (number of components)" );
    }
    
    if (!trans)
    {
        if (src_bitmap->getComponents() != dst_bitmap->getComponents())
        {
            throw jett_exception( JETT_INVALID_PROFILE,0, "Colour management cannot be disabled because the source and destination bitmaps are not compatible" );
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
    cl_mem src_bitmap_cl = src_bitmap->lockCLData(m_cl, true );
    cl_int src_stride = static_cast<cl_int>(src_bitmap->getStride());
    cl_int src_pixel_stride = static_cast<cl_int>(src_bitmap->getPixelStride());
    cl_int src_width = static_cast<cl_int>(src_bitmap->getWidth());
    cl_int src_height = static_cast<cl_int>(src_bitmap->getHeight());
    cl_mem dst_bitmap_cl = dst_bitmap->lockCLData( m_cl, false );
    cl_int dst_stride = static_cast<cl_int>(dst_bitmap->getStride());
    cl_int dst_pixel_stride = static_cast<cl_int>(dst_bitmap->getPixelStride());
    cl_int dst_dimensions = static_cast<cl_int>(dst_bitmap->getComponents());
    
    char opta_name[ 64 ];
    _snprintf_s( opta_name, sizeof(opta_name), "transform_%d_%d_%d_opta", TRANSFORM_AXIS_SIZE, m_dimensions_in, dst_dimensions );
    // Shared resources
    cl_mem opta_cl = m_cl->get_shared_resource( opta_name, NULL, 0 );

    
    size_t xyz_size = std::max(dst_dimensions, m_dimensions_in);
    size_t local_y = get_bitblt_local_y(xyz_size, static_cast<size_t>(m_workgroup_factor), m_bitblt_kernel.get_workgroup_size());

    // How much local memory do we need?
    int mem_factor = 15;
    if ((params.flags & perform_resize_complx) != 0)
    {
        mem_factor = 60;
    }
    
	// Setup the kernel
    m_bitblt_kernel.setKernelArg(0,  sizeof(cl_int), &params.flags);
    m_bitblt_kernel.setKernelArg(1,  sizeof(cl_mem), &src_bitmap_cl);
    m_bitblt_kernel.setKernelArg(2,  sizeof(cl_int), &src_stride);
    m_bitblt_kernel.setKernelArg(3,  sizeof(cl_int), &src_pixel_stride);
    m_bitblt_kernel.setKernelArg(4,  sizeof(cl_int), &params.src_x);
    m_bitblt_kernel.setKernelArg(5,  sizeof(cl_int), &params.src_y);
    m_bitblt_kernel.setKernelArg(6,  sizeof(cl_int), &src_width);
    m_bitblt_kernel.setKernelArg(7,  sizeof(cl_int), &src_height);
    m_bitblt_kernel.setKernelArg(8,  sizeof(cl_float), &params.scale_src_x);
    m_bitblt_kernel.setKernelArg(9,  sizeof(cl_float), &params.scale_src_y);
    m_bitblt_kernel.setKernelArg(10, sizeof(cl_mem), &dst_bitmap_cl);
    m_bitblt_kernel.setKernelArg(11, sizeof(cl_int), &dst_stride);
    m_bitblt_kernel.setKernelArg(12, sizeof(cl_int), &dst_pixel_stride);
    m_bitblt_kernel.setKernelArg(13, sizeof(cl_int), &dst_dimensions);
    m_bitblt_kernel.setKernelArg(14, sizeof(cl_int), &params.dst_x1);
    m_bitblt_kernel.setKernelArg(15, sizeof(cl_int), &params.dst_y1);
    m_bitblt_kernel.setKernelArg(16, sizeof(cl_int), &params.dst_x2);
    m_bitblt_kernel.setKernelArg(17, sizeof(cl_int), &params.dst_y2);
    m_bitblt_kernel.setKernelArg(18, sizeof(cl_mem), trans ? trans->get_lut_cl( m_cl ) : NULL );
    m_bitblt_kernel.setKernelArg(19, sizeof(cl_mem), &opta_cl);
    m_bitblt_kernel.setKernelArg(20, local_y * mem_factor * sizeof(int), 0 );

    size_t local[2] = { xyz_size, local_y };
    size_t global[2] = { xyz_size * 512, local_y * 8 };
    
    m_cl->enqueueNDRangeKernel( m_bitblt_kernel, 2, NULL, global, local );
    m_cl->finish();
    
    // Release the bitmap
    src_bitmap->unlockCLData(m_cl);
    dst_bitmap->unlockCLData(m_cl);
}


// Convert a bitmap on GPU (generic version)
void CGPUBitblt::copy_glyph( unsigned int colour, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap )
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
        
    if (dst_bitmap->isBGR())
    {
        params.flags |= flip_dst_components;
    }

    // The src/dst bitmaps
    cl_mem src_bitmap_cl = src_bitmap->lockCLData(m_cl, true );
    cl_int src_stride = static_cast<cl_int>(src_bitmap->getStride());
    cl_mem dst_bitmap_cl = dst_bitmap->lockCLData( m_cl, false );
    cl_int dst_stride = static_cast<cl_int>(dst_bitmap->getStride());
    cl_int dst_pixel_stride = static_cast<cl_int>(dst_bitmap->getPixelStride());
    cl_int dst_dimensions = static_cast<cl_int>(dst_bitmap->getComponents());
            
	// Setup the kernel
    m_bitblt_glyph_kernel.setKernelArg(0,  sizeof(cl_int), &params.flags);
    m_bitblt_glyph_kernel.setKernelArg(1,  sizeof(cl_mem), &src_bitmap_cl);
    m_bitblt_glyph_kernel.setKernelArg(2,  sizeof(cl_int), &src_stride);
    m_bitblt_glyph_kernel.setKernelArg(3,  sizeof(cl_int), &params.src_x);
    m_bitblt_glyph_kernel.setKernelArg(4,  sizeof(cl_int), &params.src_y);
    m_bitblt_glyph_kernel.setKernelArg(5, sizeof(cl_mem), &dst_bitmap_cl);
    m_bitblt_glyph_kernel.setKernelArg(6, sizeof(cl_int), &dst_stride);
    m_bitblt_glyph_kernel.setKernelArg(7, sizeof(cl_int), &dst_pixel_stride);
    m_bitblt_glyph_kernel.setKernelArg(8, sizeof(cl_int), &dst_dimensions);
    m_bitblt_glyph_kernel.setKernelArg(9, sizeof(cl_int), &params.dst_x1);
    m_bitblt_glyph_kernel.setKernelArg(10, sizeof(cl_int), &params.dst_y1);
    m_bitblt_glyph_kernel.setKernelArg(11, sizeof(cl_int), &params.dst_x2);
    m_bitblt_glyph_kernel.setKernelArg(12, sizeof(cl_int), &params.dst_y2);
    m_bitblt_glyph_kernel.setKernelArg(13, sizeof(cl_uint), &colour);
        
    size_t xyz_size = std::max(dst_dimensions, m_dimensions_in);
    size_t local_y = get_bitblt_local_y(xyz_size, static_cast<size_t>(m_workgroup_factor), m_bitblt_glyph_kernel.get_workgroup_size());

    size_t local[2] = { xyz_size, local_y };
    size_t global[2] = { xyz_size * 512, local_y * 8 };
    
    m_cl->enqueueNDRangeKernel( m_bitblt_glyph_kernel, 2, NULL, global, local );
    
    // Release the bitmap
    src_bitmap->unlockCLData(m_cl);
    dst_bitmap->unlockCLData(m_cl);
}


// Convert a bitmap on GPU (generic version)
void CGPUBitblt::copy_matrix( CTransform *trans, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap )
{
    // Check some assumptions
    if (src_bitmap->getComponents() != m_dimensions_in)
    {
        throw jett_exception( JETT_INTERNAL_ERROR,0, "This CBitblt object was not built for this source image (number of components)" );
    }
    
    if (!trans)
    {
        if (src_bitmap->getComponents() != dst_bitmap->getComponents())
        {
            throw jett_exception( JETT_INVALID_PROFILE,0, "Colour management cannot be disabled because the source and destination bitmaps are not compatible" );
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
    cl_mem src_bitmap_cl = src_bitmap->lockCLData(m_cl, true );
    cl_int src_stride = static_cast<cl_int>(src_bitmap->getStride());
    cl_int src_pixel_stride = static_cast<cl_int>(src_bitmap->getPixelStride());
    cl_mem dst_bitmap_cl = dst_bitmap->lockCLData( m_cl, false );
    cl_int dst_stride = static_cast<cl_int>(dst_bitmap->getStride());
    cl_int dst_pixel_stride = static_cast<cl_int>(dst_bitmap->getPixelStride());
    cl_int dst_dimensions = static_cast<cl_int>(dst_bitmap->getComponents());
    
    char opta_name[ 64 ];
    _snprintf_s( opta_name, sizeof(opta_name), "transform_%d_%d_%d_opta", TRANSFORM_AXIS_SIZE, m_dimensions_in, dst_dimensions );
    // Shared resources
    cl_mem opta_cl = m_cl->get_shared_resource( opta_name, NULL, 0 );

    
    int dimensions_in = m_dimensions_in;
    if (m_alpha)
    {
        dimensions_in = 4;
    }

    size_t xyz_size = std::max(dst_dimensions, dimensions_in);
    size_t local_y = get_bitblt_local_y(xyz_size, static_cast<size_t>(m_workgroup_factor), m_bitblt_matrix_kernel.get_workgroup_size());

    // How much local memory do we need?
    int mem_factor = 15;
    if ((params.flags & perform_resize_complx) != 0)
    {
        mem_factor = 60;
    }
    
	// Setup the kernel
    m_bitblt_matrix_kernel.setKernelArg(0,  sizeof(cl_int), &params.flags);
    m_bitblt_matrix_kernel.setKernelArg(1,  sizeof(cl_mem), &src_bitmap_cl);
    m_bitblt_matrix_kernel.setKernelArg(2,  sizeof(cl_int), &src_stride);
    m_bitblt_matrix_kernel.setKernelArg(3,  sizeof(cl_int), &src_pixel_stride);
    m_bitblt_matrix_kernel.setKernelArg(4,  sizeof(cl_int), &params.src_x);
    m_bitblt_matrix_kernel.setKernelArg(5,  sizeof(cl_int), &params.src_y);
    m_bitblt_matrix_kernel.setKernelArg(6,  sizeof(cl_int), &params.src_x2);
    m_bitblt_matrix_kernel.setKernelArg(7,  sizeof(cl_int), &params.src_y2);
    m_bitblt_matrix_kernel.setKernelArg(8, sizeof(cl_mem), &dst_bitmap_cl);
    m_bitblt_matrix_kernel.setKernelArg(9, sizeof(cl_int), &dst_stride);
    m_bitblt_matrix_kernel.setKernelArg(10, sizeof(cl_int), &dst_pixel_stride);
    m_bitblt_matrix_kernel.setKernelArg(11, sizeof(cl_int), &dst_dimensions);
    m_bitblt_matrix_kernel.setKernelArg(12, sizeof(cl_int), &params.dst_x1);
    m_bitblt_matrix_kernel.setKernelArg(13, sizeof(cl_int), &params.dst_y1);
    m_bitblt_matrix_kernel.setKernelArg(14, sizeof(cl_int), &params.dst_x2);
    m_bitblt_matrix_kernel.setKernelArg(15, sizeof(cl_int), &params.dst_y2);
    m_bitblt_matrix_kernel.setKernelArg(16, sizeof(cl_mem), trans ? trans->get_lut_cl( m_cl ) : NULL );
    m_bitblt_matrix_kernel.setKernelArg(17, sizeof(cl_mem), &opta_cl);
    m_bitblt_matrix_kernel.setKernelArg(18, local_y * mem_factor * sizeof(int), 0 );
    m_bitblt_matrix_kernel.setKernelArg(19,  sizeof(cl_float), &params.matrix.a);
    m_bitblt_matrix_kernel.setKernelArg(20,  sizeof(cl_float), &params.matrix.b);
    m_bitblt_matrix_kernel.setKernelArg(21,  sizeof(cl_float), &params.matrix.c);
    m_bitblt_matrix_kernel.setKernelArg(22,  sizeof(cl_float), &params.matrix.d);
    m_bitblt_matrix_kernel.setKernelArg(23,  sizeof(cl_float), &params.matrix.e);
    m_bitblt_matrix_kernel.setKernelArg(24,  sizeof(cl_float), &params.matrix.f);
    
    
    size_t local[2] = { xyz_size, local_y };
    size_t global[2] = { xyz_size * 512, local_y * 8 };
    
    m_cl->enqueueNDRangeKernel( m_bitblt_matrix_kernel, 2, NULL, global, local );
    m_cl->finish();
    
    // Release the bitmap
    src_bitmap->unlockCLData(m_cl);
    dst_bitmap->unlockCLData(m_cl);
}

