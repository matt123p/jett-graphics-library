//
//  CPULine.cpp
//  GraphicsLibrary
//
//  Created by Matt Pyne on 02/09/2013.
//
//

#include "CPULine.h"


// Construction/destruction
CCPULine::CCPULine()
{
    
}

CCPULine::~CCPULine()
{
    
}


//
// Draw a series of lines
//
void CCPULine::lines( CImage *dst_image, unsigned char *colour, float width, jett_point *points, int n, bool close, int flags )
{
    // What type of line are we going to draw?
    if ((flags & line_best) != 0)
    {
        lines_best( dst_image, colour, width, points, n, close, flags );
    }
    else
    {
        lines_fast( dst_image, colour, width, points, n, close, flags );
    }
    
}


//
// Draw a series of lines with anti-aliasing
//
void CCPULine::lines_best( CImage *dst_image, unsigned char *colour, float width, jett_point *points, int n, bool close, int flags )
{
}

//
// Draw a series of lines without anti-aliasing
//
void CCPULine::lines_fast( CImage *dst_image, unsigned char *colour, float width, jett_point *points, int n, bool close, int flags )
{
    // Use Bresenham's line algorithm
    
}
