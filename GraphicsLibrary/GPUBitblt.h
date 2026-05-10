//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#ifndef GraphicsLibrary_bitblt_h
#define GraphicsLibrary_bitblt_h


#include "CLKernel.h"
#include "GPUProcessor.h"
#include "CLProgram.h"

class CImage;
class CTransform;



// This class is the wrapper for pre-computed colour management
// transform tables
class CGPUBitblt : public CBitblt
{
private:
    
    // Here is the OpenCL device we are using
    CGPUProcessor*   m_cl;
    
    // Here is the colour lookup table in CPU memory
    int         m_dimensions_in;
    bool        m_alpha;
    
    // Here is the GPU program
    CLProgram   m_program;
    CLKernel    m_bitblt_kernel;
    CLKernel    m_bitblt_glyph_kernel;
    CLKernel    m_bitblt_matrix_kernel;
    int         m_workgroup_factor;
        
    
public:
    
    CGPUBitblt(CGPUProcessor* cl, int src_dimensions, bool alpha );
    virtual ~CGPUBitblt();
        
    // Convert a bitmap using the GPU
    virtual void copy( CTransform *trans, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap );
    virtual void copy_glyph( unsigned int colour, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap );
	virtual void copy_matrix( CTransform *trans, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap );
};


#endif
