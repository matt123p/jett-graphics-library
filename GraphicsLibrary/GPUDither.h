//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_GPUDither_h
#define GraphicsLibrary_GPUDither_h

#include "Processor.h"
#include "CLKernel.h"
#include "CLProgram.h"

//
// Dither an image using the ordered screen
//

class COrderedScreenCollection;
class CImage;

class CGPUDither : public CDither
{
private:
    // Here is the OpenCL device we are using
    CGPUProcessor*   m_cl;
        
    // Here is the GPU program
    CLProgram   m_program;
    CLKernel    m_dither_in_place_kernel;
    CLKernel    m_dither_sep8_kernel;
    CLKernel    m_dither_sep4_kernel;
    CLKernel    m_dither_sep2_kernel;
    CLKernel    m_dither_sep1_kernel;
    
    // Support linearization
    bool        m_linearization;

public:
	CGPUDither(CGPUProcessor* cl);
	virtual ~CGPUDither(void);

	virtual void dither( CImage *src_image, COrderedScreenCollection& screens, CLinearizationCollection* linearization, int scale );
	virtual void dither( CImage *src_image, CImage** dst_images, COrderedScreenCollection& screens, CLinearizationCollection* linearization, int scale );
};

#endif


