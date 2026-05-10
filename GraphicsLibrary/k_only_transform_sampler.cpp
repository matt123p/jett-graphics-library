//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "k_only_transform_sampler.h"


k_only_transform_sampler::k_only_transform_sampler(  )
{
    m_dimensions_out = 4;
    m_colour[0] = 0;        // C
    m_colour[1] = 0;        // M
    m_colour[2] = 0;        // Y
    m_colour[3] = 1.0f;     // K
}

k_only_transform_sampler::k_only_transform_sampler( int dimensions_out, const unsigned char* colour )
{
    m_dimensions_out = dimensions_out;
    for (int i = 0; i < m_dimensions_out; ++ i)
    {
        m_colour[i] = colour[i] / 255.0f;
    }
}


k_only_transform_sampler::~k_only_transform_sampler()
{
    
}

//
// Determine the transform properties
//
int k_only_transform_sampler::dimensions_in()
{
    // Fixed monochrome in
    return 1;
}

int k_only_transform_sampler::dimensions_out()
{
    return m_dimensions_out;
}

// 
// Perform a colour management operation
// 
void k_only_transform_sampler::sample( float *sample_in, float *sample_out )
{
    float f  = 1.0f - sample_in[0];
    
    for (int i = 0; i < m_dimensions_out; ++ i)
    {
        sample_out[i] = f * m_colour[i];
    }
}
