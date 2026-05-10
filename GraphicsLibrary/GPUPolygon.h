//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_Polygon_h
#define GraphicsLibrary_Polygon_h

#include "CLKernel.h"
#include "GPUProcessor.h"
#include "CLProgram.h"
#include "jett.h"

// This class is the wrapper for pre-computed colour management
// transform tables
class CGPUPolygon : public CPolygon
{
private:
    
    // Here is the OpenCL device we are using
    CGPUProcessor*   m_cl;
        
    // Here is the GPU program
    CLProgram   m_program;
    CLKernel    m_poly_kernel;

    // How much do we oversample by (for anti-aliasing)
    int         m_oversample;
    
    // The nodes cache
    int         m_max_nodes;
    cl_mem      m_polyX_cl; 
    cl_mem      m_polyY_cl;

    
public:
    CGPUPolygon(CGPUProcessor* cl, int oversample);
    virtual ~CGPUPolygon();
        
    virtual void fill( CImage *dst_image, unsigned char *colour, jett_point *points, int n );

};

#endif
