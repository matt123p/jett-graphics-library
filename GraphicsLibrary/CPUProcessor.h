//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_CPUProcessor_h
#define GraphicsLibrary_CPUProcessor_h

#include "Processor.h"
#include "CPUDither.h"

class CCPUProcessor : public CProcessor
{
private:
    
    // The bitblt pre-compiled kernels we can call on
    // (each one for different source dimensions)
    CBitblt*  m_bitblt[5];
    
    // The polygon fill kernels
    CPolygon* m_polygon[2];

	// The dither kernel
	CCPUDither	  m_dither;
    
public:
    CCPUProcessor();
    virtual ~CCPUProcessor();
    
    // Find and open the OpenCL device.
    // This throws an exception if there is a problem
    virtual void Open();
    
    // Access the bitblt kernels
    virtual CBitblt* bitblt( size_t index );
    
    // Access the polygon kernels
    virtual CPolygon* polygon(size_t index);

	// Access the dither kernels
	virtual CDither* dither();
    
    // Wait for a kernel execution to finish
    virtual void finish();
    
    // Do we have an attached CLDevice (i.e. a GPU?)
    virtual CGPUProcessor*  get_cl_device();
};

#endif
