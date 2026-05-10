//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#ifndef GraphicsLibrary_k_only_transform_sampler_h
#define GraphicsLibrary_k_only_transform_sampler_h



#include "transform.h"

class k_only_transform_sampler : public transform_sampler
{
    // Up to CMYK
    int   m_dimensions_out;
    float m_colour[4];
    
public:
    
    k_only_transform_sampler();
    k_only_transform_sampler( int dimensions_out, const unsigned char* colour );
    virtual ~k_only_transform_sampler();
    
    //
    // Determine the transform properties
    //
    virtual int dimensions_in();
    virtual int dimensions_out();
    
    // 
    // Perform a colour management operation
    // 
    virtual void sample( float *sample_in, float *sample_out );
    
};

#endif
