//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_CPUPolygon_h
#define GraphicsLibrary_CPUPolygon_h


#include "CPUProcessor.h"
#include "jett.h"

// This class is the wrapper for pre-computed colour management
// transform tables
class CCPUPolygon : public CPolygon
{
private:
    
    // Here is the OpenCL device we are using
    CCPUProcessor*   m_cl;
        
    // How much do we oversample by (for anti-aliasing)
    int         m_oversample;
    
    void fill_quick( CImage *dst_image, unsigned char *colour, jett_point *points, int n );
    void fill_oversample( CImage *dst_image, unsigned char *colour, jett_point *points, int n );


public:
    CCPUPolygon(CCPUProcessor* cl, int oversample);
    virtual ~CCPUPolygon();
    
    virtual void fill( CImage *dst_image, unsigned char *colour, jett_point *points, int n );
    
};


#endif
