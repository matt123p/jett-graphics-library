//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#ifndef GraphicsLibrary_transform_h
#define GraphicsLibrary_transform_h

// The algorithms depend on these being constant across all
// transforms

// This is the number of bits we reduce the precision by in the LUT table
#define TRANSFORM_N             3
// This is the resulting axis size
#define TRANSFORM_AXIS_SIZE     33
// This is the addition precision in the table (i.e. the amount of fixed point precision)
#define TRANSFORM_FP_PRECISION  8
// This is the value of (1<<(TRANSFORM_N+TRANSFORM_FP_PRECISION-1))-1 which is used to round the fixed point number
#define TRANSFORM_FP_ROUND      1023
// This is the value of (1<<(TRANSFORM_N+TRANSFORM_FP_PRECISION-9))-1 which is used to round the fixed point number
#define TRANSFORM_FP_ROUND16    3

#include "jett.h"
class CGPUProcessor;


//////////////////////////////////////////////////////////////////
//
// 8bit/16bit conversions
//

inline void int_convert( unsigned char& to, unsigned char from)
{
    to = from;
}

inline void int_convert( unsigned short& to, unsigned short from)
{
    to = from;
}

inline void int_convert( unsigned char& to, unsigned short from)
{
    to = from / 257;
}

inline void int_convert( unsigned short& to, unsigned char from)
{
    to = from * 257;
}

template< class dst_t, class src_t> void pixel_convert( dst_t* to, const src_t *from, int N)
{
    for (int i = 0; i < N; ++ i)
    {
        int_convert(to[i], from[i]);
    }
}



// This class is the wrapper for pre-computed colour management
// transform tables
class CTransform 
{
private:
 
    // Here is the OpenCL device we are using
    CGPUProcessor*   m_cl;
    
    // Here is the colour lookup table in CPU memory
    int         m_dimensions_in;
    int         m_dimensions_out;
    uint16_t*   m_lut;
    
    // The openCL data
    cl_mem      m_lut_cl;
    
public:
    
    CTransform();
    virtual ~CTransform();
        
    //
    // Create this object from a transform_sampler
    //
    void build_transform( transform_sampler* pSampler );

    // Access the lut table
    cl_mem* get_lut_cl( CGPUProcessor* cl);
    int get_dimensions_in() const { return m_dimensions_in; }
    int get_dimensions_out() const { return m_dimensions_out; }
    
    // Convert a single pixel
    void convert(const unsigned char*  src_pixel, unsigned char*  dst_pixel );
    void convert(const unsigned short* src_pixel, unsigned short* dst_pixel );
    void convert(const unsigned short* src_pixel, unsigned char*  dst_pixel );
    void convert(const unsigned char*  src_pixel, unsigned short* dst_pixel );
};

#endif
