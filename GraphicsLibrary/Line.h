//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#ifndef __GraphicsLibrary__Line__
#define __GraphicsLibrary__Line__

#include "jett.h"
class CPolygon;

//
// This class converts a line in to a polygon
//
class CLineToPoly
{
private:
    
    enum join_t
    {
        join_none,
        join_bevel,
        join_miter
    };
    
    struct polygon_t
    {
        // The polygon
        jett_point p[ 64 ];
        // The number of verticies in the polygon
        int    nx;
        
        // Quick access to members of the polygon
        // associated with the original rectangle
        int    p0;
        int    p1;
        int    p2;
        int    p3;
        
        polygon_t()
        {
            nx = 0;
            p0 = 0;
            p1 = 1;
            p2 = 2;
            p3 = 3;
        }
    };
    
    // The miter limit
    double  m_miter_limit;

	// Adjustment for polygon drawing code
	float	m_dx_adjust;
    
    // Convert a single line to a rectangle
    void lineToPoly( float width, jett_point p1, jett_point p2, polygon_t& r);
    
    // Merge two rectangles to take into account their
    // line join options
    void mergePolyFlat( polygon_t& r1, polygon_t& r2 );
    void mergePolyMiter( polygon_t& r1, polygon_t& r2 );
    
    // Is a point inside a rectangle?
    static bool isPointInPoly( polygon_t& rect, jett_point &p );
    
    // What is the intersection between lines?
    static bool intersection( jett_point* l1, jett_point *l2, jett_point &p, double &angle );
    
    // What is the distance between two points
    static double distance( jett_point *p1, jett_point *p2 );

public:
    
    // Construction/destruction
    CLineToPoly();
    ~CLineToPoly();
    
    // Perform the conversion
    void paint( CPolygon* p, jett_image& dst_bitmap, unsigned char *colour, float width, jett_point *points, int n, bool close, int flags );
};

#endif /* defined(__GraphicsLibrary__Line__) */
