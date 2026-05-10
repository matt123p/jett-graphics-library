//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#include "StdAfx.h"
#include "CPUProcessor.h"
#include "CPUPolygon.h"
#include "CPUBitblt.h"


CCPUProcessor::CCPUProcessor()
{
        // Create helper objects
    //
    // Pre-compile the bitblt operators
    m_bitblt[0] = new CCPUBitblt(this, 3, true  );     // RGB + Alpha support
    m_bitblt[1] = new CCPUBitblt(this, 1, false );     // Monochrome
    m_bitblt[2] = NULL;											   // Unused
    m_bitblt[3] = new CCPUBitblt(this, 3, false );     // RGB
    m_bitblt[4] = new CCPUBitblt(this, 4, false );     // CMYK
    
    // Pre-compile polygon operators
    m_polygon[0]  = new CCPUPolygon(this, 0);
    m_polygon[1]  = new CCPUPolygon(this, 4);
}

CCPUProcessor::~CCPUProcessor()
{
    // Destroy the helper objects
    for (int i = 0; i < sizeof(m_bitblt) / sizeof(m_bitblt[0]); ++ i)
    {
        if (m_bitblt[i])
        {
            delete m_bitblt[i];
        }
    }
    for (int i = 0; i < sizeof(m_polygon) / sizeof(m_polygon[0]); ++ i)
    {
        if (m_polygon[i])
        {
            delete m_polygon[i];
        }
    }

}

// Find and open the OpenCL device.
// This throws an exception if there is a problem
void CCPUProcessor::Open()
{
    
}

CBitblt* CCPUProcessor::bitblt( size_t index )
{
    return m_bitblt[index];
}

// Access the polygon kernels
CPolygon* CCPUProcessor::polygon(size_t index)
{
    return m_polygon[index];
}

// Access the dither kernels
CDither* CCPUProcessor::dither()
{
	return &m_dither;
}


// Wait for a kernel execution to finish
void CCPUProcessor::finish()
{
    
}

// Do we have an attached CLDevice (i.e. a GPU?)
CGPUProcessor*  CCPUProcessor::get_cl_device()
{
    return NULL;
}
