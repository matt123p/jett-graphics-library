//
//  CPULine.h
//  GraphicsLibrary
//
//  Created by Matt Pyne on 02/09/2013.
//
//

#ifndef GraphicsLibrary_CPULine_h
#define GraphicsLibrary_CPULine_h

#include "CPUProcessor.h"
#include "jett.h"

// This class is the wrapper for pre-computed colour management
// transform tables
class CCPULine : public CLine
{
private:
    
    void lines_best( CImage *dst_image, unsigned char *colour, float width, jett_point *points, int n, bool close, int flags );
    void lines_fast( CImage *dst_image, unsigned char *colour, float width, jett_point *points, int n, bool close, int flags );
    
    
public:
    // Construction/destruction
    CCPULine();
    virtual ~CCPULine();
    
    //
    // Draw a series of lines
    //
    virtual void lines( CImage *dst_image, unsigned char *colour, float width, jett_point *points, int n, bool close, int flags );
    
};



#endif
