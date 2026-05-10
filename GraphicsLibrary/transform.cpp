//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "Processor.h"
#include <stdio.h>
#include <algorithm>
#include "transform.h"
#include "Image.h"
#include "GPUProcessor.h"
#include <math.h>
#include "timer.h"
#include "lcms_transform_sampler.h"


CTransform::CTransform()
{
    m_cl = NULL;
    m_lut_cl = NULL;
    m_dimensions_in = 0;
    m_dimensions_out = 0;
    m_lut = NULL;
}

CTransform::~CTransform()
{
    if (m_lut_cl)
    {
        m_cl->releaseMemObject( m_lut_cl );
    }
    delete [] m_lut;
}
    
//
// Create this object from a transform_sampler
//
void CTransform::build_transform( transform_sampler* pSampler )
{
    // Find out the dimensions
    m_dimensions_in = pSampler->dimensions_in();
    m_dimensions_out = pSampler->dimensions_out();
    
    // Make the colour lookup table
    size_t total_points = static_cast<int>(pow((double)TRANSFORM_AXIS_SIZE,m_dimensions_in));
    m_lut = new uint16_t[ total_points * m_dimensions_out ];
    
    // Perform sampling
    int sample_p[ 4 ];
    float sample_in[ 4 ];
    float sample_out[ 4 ];
    memset( sample_p, 0, sizeof(sample_p[0]) * m_dimensions_in );
    memset( sample_in, 0, sizeof(sample_in[0]) * m_dimensions_in );
    
    try 
    {
        float fp_precision = 1 << TRANSFORM_FP_PRECISION;
        for (size_t p = 0; p < total_points; ++ p)
        {
            for (int i = 0; i < m_dimensions_in; ++i)
            {
                if (sample_p[i] == TRANSFORM_AXIS_SIZE-1)
                {
                    sample_in[i] = 1.0f;
                }
                else
                {
                    sample_in[i] = (sample_p[i] << TRANSFORM_N) / (255.0f);
                }
            }
            
            // Perform the conversion
            pSampler->sample(sample_in, sample_out );
            
            for (int i = 0; i < m_dimensions_out; ++i)
            {
                // We add 8 more bits of precision (hence the mul * 256)
                m_lut[ p * m_dimensions_out + i ] = static_cast<int>(sample_out[i] * 255.0f * fp_precision);
            }
            
            // Increment the sample point
            for (int i = 0; i < m_dimensions_in; ++i)
            {
                ++sample_p[i];
                if (sample_p[i] > TRANSFORM_AXIS_SIZE-1)
                {
                    sample_p[i] = 0;
                }
                else
                {
                    break;
                }
            }
        }
    } 
    catch (...)
    {
        throw;
    }
    
}

cl_mem* CTransform::get_lut_cl( CGPUProcessor* cl)
{
    m_cl = cl;
    
    if (m_cl && !m_lut_cl)
    {
        size_t total_points = static_cast<int>(pow((double)TRANSFORM_AXIS_SIZE,m_dimensions_in));
        m_lut_cl = m_cl->createBuffer(CL_MEM_READ_ONLY,  sizeof(uint16_t) * total_points * m_dimensions_out, NULL );
        m_cl->enqueueWriteBuffer(m_lut_cl, CL_TRUE, 0, sizeof(uint16_t) * total_points * m_dimensions_out, m_lut );
    }
    
    return &m_lut_cl;
}

template<class src_t, class dst_t, int _SRC_SHIFT, int _DST_SHIFT, int _TRANSFORM_FP_ROUND, int _DST_MAX> void _convert(const src_t* src_pixel, dst_t* dst_pixel, int dimensions_in, int dimensions_out, uint16_t* lut )
{

    // Extract the base location of the cube
    int P[8] = {0};
    int pixel[4] = {0};

    int f = dimensions_out;
    for (int i = 0; i < dimensions_in; ++ i)
    {
        pixel[i] = src_pixel[i];
        
        P[i*2] = f * (pixel[i] >> _SRC_SHIFT);
        P[i*2+1] = P[i*2] + f;
        
        f = f * TRANSFORM_AXIS_SIZE;
    }


    
    for (int col = 0; col < dimensions_out; ++ col)
    {
        int sum = lut[ P[0] + P[2] + P[4] + P[6] + col ];
        for (int n = 0; n < TRANSFORM_N; ++ n)
        {
            int v = lut[ P[ (pixel[0] >> n) & 1] + P[ 2 + ((pixel[1] >> n) & 1) ] + P[ 4 + ((pixel[2] >> n) & 1) ] + P[ 6 + ((pixel[3] >> n) & 1) ] + col ];

            sum += v << n;
        }
        
        // The answer has 8 more bits of precision than we need
        sum = (sum+_TRANSFORM_FP_ROUND) >> (TRANSFORM_FP_PRECISION+_DST_SHIFT);
        
        if (sum > _DST_MAX)
        {
            sum = _DST_MAX;
        }
        if (sum < 0)
        {
            sum = 0;
        }
        
        dst_pixel[ col ] = sum;
    }
}


void CTransform::convert(const unsigned char*  src_pixel, unsigned char*  dst_pixel )
{
    _convert<unsigned char, unsigned char , TRANSFORM_N, TRANSFORM_N, TRANSFORM_FP_ROUND, 255>(src_pixel, dst_pixel, m_dimensions_in, m_dimensions_out, m_lut );
}

void CTransform::convert(const unsigned short* src_pixel, unsigned short* dst_pixel )
{
    _convert<unsigned short, unsigned short , TRANSFORM_N+8, TRANSFORM_N-8, TRANSFORM_FP_ROUND16, 65535>(src_pixel, dst_pixel, m_dimensions_in, m_dimensions_out, m_lut );
}

void CTransform::convert(const unsigned short* src_pixel, unsigned char*  dst_pixel )
{
    _convert<unsigned short, unsigned char , TRANSFORM_N+8, TRANSFORM_N, TRANSFORM_FP_ROUND, 255>(src_pixel, dst_pixel, m_dimensions_in, m_dimensions_out, m_lut );
}

void CTransform::convert(const unsigned char*  src_pixel, unsigned short* dst_pixel )
{
    _convert<unsigned char, unsigned short, TRANSFORM_N, TRANSFORM_N-8, TRANSFORM_FP_ROUND16, 65535>(src_pixel, dst_pixel, m_dimensions_in, m_dimensions_out, m_lut );
}

