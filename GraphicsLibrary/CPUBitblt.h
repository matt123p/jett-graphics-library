//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_CPUBitblt_h
#define GraphicsLibrary_CPUBitblt_h

#include "Processor.h"
#include "CPUProcessor.h"

// This class is the wrapper for pre-computed colour management
// transform tables
class CCPUBitblt : public CBitblt
{
private:
    
    // Here is the options this object was created with
    int         m_dimensions_in;
    bool        m_alpha;
    
public:
    
    CCPUBitblt(CCPUProcessor* cl, int src_dimensions, bool alpha );
    virtual ~CCPUBitblt();
    
    // Convert a bitmap using the GPU
    virtual void copy( CTransform *trans, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap );
	virtual void copy_glyph( unsigned int colour, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap );
	virtual void copy_matrix( CTransform *trans, bitblt_params &params, CImage* src_bitmap, CImage* dst_bitmap );
};


#endif
