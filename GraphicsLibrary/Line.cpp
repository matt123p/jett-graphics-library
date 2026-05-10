//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#include "StdAfx.h"
#include "Line.h"
#include "Processor.h"
#include <math.h>



// Construction/destruction
CLineToPoly::CLineToPoly()
{
    m_miter_limit = 10;
    m_dx_adjust = 1.0f;
}

CLineToPoly::~CLineToPoly()
{
    
}

// Perform the conversion
void CLineToPoly::paint( CPolygon* p, jett_image& dst_bitmap, unsigned char *colour, float width, jett_point *points, int n, bool close, int flags )
{
    // We must halve the width
    width = std::max(0.5f,width / 2.0f );
    
    // The line join mode
    join_t jm = join_bevel;

    if ((flags & line_join_none) != 0)
    {
        jm = join_none;
    }
    else if ((flags & line_join_bevel) != 0)
    {
        jm = join_bevel;
    }
    else if ((flags & line_join_miter) != 0)
    {
        jm = join_miter;
    }
    
    // The buffers to keep track of the
    // the previous and next rectangles
    polygon_t b1;
    polygon_t b2;

    polygon_t *r1 = &b1;
    polygon_t *r2 = &b2;
    
    // We have one less line if we don't close
    int np = n;
    if (!close)
    {
        -- np;
    }
    
    // Convert the first line to a rectangle
    lineToPoly(width, points[0], points[1], *r1);
    
    // Now do the other lines
    int p1_i = 1;
    int p2_i = 2;
    for (int i = 1; i <= np; ++i)
    {
        // Wrap around the the beginning for closed
        // line sets
        lineToPoly(width, points[p1_i % n], points[p2_i % n], *r2 );
        
        // Ok, modify the previous rectangle (r1) to merge with the next
        // rectangle (r2)
        // We must find the two points on the rectangles that are
        // inside the other rectangle and the two points that are outside
        if (close || p1_i != n-1)
        {
            switch (jm)
            {
                case join_none:
                    break;
                case join_bevel:
                    mergePolyFlat( *r1, *r2 );
                    break;
                case join_miter:
                    mergePolyMiter( *r1, *r2 );
                    break;
            }
        }
        
        // Paint the first rectangle (the 2nd one is held for the next)
        // go
        p->fill(dst_bitmap, colour, r1->p, r1->nx);
        
        // Roll the buffers
        std::swap( r1, r2 );
        ++ p1_i;
        ++ p2_i;
    }

}


// Convert a single line to a rectangle
void CLineToPoly::lineToPoly( float width, jett_point p1, jett_point p2, polygon_t& r)
{
    // Determine dx and dy from the end-points
    double dx = 0;
    double dy = 0;    
    
	p1.x += 0.5f;
	p2.x += 0.5f;

	float dx_adjust = m_dx_adjust;

    // Is this a simple case (horizontal or vertical)?
    if (p2.x == p1.x)
    {
        // Vertical
        dx = width;
        dy = 0;
    }
    else if (p2.y == p1.y)
    {
        // Horizontal
        dx = 0;
        dy = width;
		dx_adjust = 0;
    }
    else
    {
        // First we convert the line to a parametric form:
        //  Y = aX + b
        double a = ( p2.y - p1.y) / ( p2.x - p1.x );
        // Don't need: double b = p1.y - p1.x * a;
        
        // The perpendicular gradient of the line is -1/a
        double pa = -1.0/a;
        // Don't need: double pb = p1.y - p1.x * pa;
        
        // Determine the distance based on width
        dx = sqrt( (width*width)/(1+pa*pa));
        dy = pa * dx;
		dx_adjust = dx_adjust / sqrt( 1+pa*pa );
    }

    // Now we can calculate the 4 points of our rectangle
	/*
	bool psx = p2.x > p1.x;
	bool psy = p2.y > p1.y;

	if (psx && psy)
	{
	}
	else if (!psx && psy)
	{
		// dy = -dy;
	}
	else if (psx && !psy)
	{
		// dx = -dx;
	}
	else // if (!psx && !psy)
	{
	}
	*/

	r.p[0] = jett_point( p1.x - dx, p1.y - dy);
	r.p[1] = jett_point( p1.x + dx, p1.y + dy);
	r.p[2] = jett_point( p2.x + dx, p2.y + dy);
	r.p[3] = jett_point( p2.x - dx, p2.y - dy);
    r.nx = 4;

	if (dx > 0)
	{
		r.p[1].x -= dx_adjust;
		r.p[2].x -= dx_adjust;
	}
	else
	{
		r.p[0].x -= dx_adjust;
		r.p[3].x -= dx_adjust;
	}

    r.p0 = 0;
    r.p1 = 1;
    r.p2 = 2;
    r.p3 = 3;
}

// Merge two rectangles to take into account their
// line join options
void CLineToPoly::mergePolyFlat( polygon_t& r1,  polygon_t& r2 )
{
    // First we must determine which of the next line points
    // is inside the previous line's rectangle
    
    // We are comparing the right-hand end of r1 with the
    // left-hand end of r2
    
    if (isPointInPoly(r1, r2.p[r2.p0] ))
    {
        // The first point is inside the rectangle
        // so we must extend the second point to
        // make the join
        r1.p[ r1.nx ] = r1.p[ r1.p3 ];
        r1.p[ r1.p3 ] = r2.p[ r2.p1 ];
        r1.p3 = r1.nx;
        ++ r1.nx;
    }
    else
    {
        // The second point must be inside the rectangle
        r1.p[ r1.nx ] = r1.p[ r1.p3 ];
        r1.p[ r1.p3 ] = r2.p[ r2.p0 ];
        r1.p3 = r1.nx;
        ++ r1.nx;
    }
    
}

void CLineToPoly::mergePolyMiter( polygon_t& r1,  polygon_t& r2 )
{
    // First we must determine which of the next line points
    // is inside the previous line's rectangle
    

    // Which of the two lines of the first rectangle is inside the second
    // rectangle?
    jett_point fp1;
    jett_point l1[2];
    if (isPointInPoly(r2, r1.p[ r1.p3 ]))
    {
        // The first point is inside the rectangle
        // so we make the miter with the other line
        l1[0] = r1.p[ r1.p1 ];
        l1[1] = r1.p[ r1.p2 ];
        fp1 = r1.p[ r1.p1 ];

    }
    else
    {
        // otherwise make the miter with the first line
        l1[0] = r1.p[ r1.p0 ];
        l1[1] = r1.p[ r1.p3 ];
        fp1 = r1.p[ r1.p0 ];
    }

    // Now look at the second rectangle inside the first
    jett_point fp2;
    jett_point l2[2];
    if (isPointInPoly(r1, r2.p[ r2.p0 ]))
    {
        // The first point is inside the rectangle
        // so we make the miter with the other line
        l2[0] = r2.p[ r2.p1 ];
        l2[1] = r2.p[ r2.p2 ];
        fp2 = r2.p[ r2.p1 ];
    }
    else
    {
        // otherwise make the miter with the first line
        l2[0] = r2.p[ r2.p0 ];
        l2[1] = r2.p[ r2.p3 ];
        fp2 = r2.p[ r2.p0 ];
    }

    bool use_flat_join = false;

    // Now make the miter
    jett_point p;
    double angle = 0;
    if (!intersection( l1, l2, p, angle ))
    {
        // There is no intersection, we must use a flat join
        use_flat_join = true;
    }
    else
    {
        // Check the miter limit
        if (angle < m_miter_limit || angle > (360-m_miter_limit))
        {
            use_flat_join = true;
        }
        
    }
    
    // if p is too far away use a flat join
    if (fabs(p.x - fp1.x) > 250 || fabs(p.y - fp1.y)>250 )
    {
        use_flat_join = true;
    }

    if (use_flat_join)
    {
        // We can't use a miter join, so do a normal
        // flat join instead
        mergePolyFlat( r1, r2 );
    }
    else
    {
        // We can use a miter join, so do it!
        //
        r1.p[ r1.nx ] = r1.p[ r1.p3 ];
        r1.p[ r1.p3 ] = p;
        r1.p3 = r1.nx;
        ++ r1.nx;
        
        // First shift all the points up
        for (int i = r2.nx; i > 1; --i)
        {
            r2.p[i] = r2.p[i-1];
        }
        r2.p1 ++;
        r2.p2 ++;
        r2.p3 ++;
        r2.p[ 1 ] = p;
        ++ r2.nx;
    }
}


// Is a point inside a rectangle?
bool CLineToPoly::isPointInPoly(  polygon_t& poly, jett_point& p )
{
    // Note, the rectangle may not be perpendicular to
    // the axis, hence a slightly more advanced
    // algorithm than you might first think
    
    // See: http://mathforum.org/library/drmath/view/54386.html
    
    int pos = 0;
    int neg = 0;
    
    int p0_i = poly.nx - 1;
    int p1_i = 0;

    for (int i = 0; i < poly.nx; ++ i)
    {
        double x0 = poly.p[ p0_i ].x;
        double y0 = poly.p[ p0_i ].y;
        double x1 = poly.p[ p1_i ].x;
        double y1 = poly.p[ p1_i ].y;
        double x2 = p.x;
        double y2 = p.y;
        
        double A = (0.5)*(x1*y2 - y1*x2 -x0*y2 + y0*x2 + x0*y1 - y0*x1);
        
        if (A > 0)
        {
            ++ pos;
        }
        if (A < 0)
        {
            ++ neg;
        }
        
        p0_i = p1_i;
        ++ p1_i;
    }

    // They must all be the same sense - i.e.
    // they must all not be positive or all not negative
    return pos == 0 || neg == 0;
}

// What is the intersection between lines?
bool CLineToPoly::intersection( jett_point* l1, jett_point *l2, jett_point &p, double& angle )
{
    // Find the intersection of the top of each
    // lines
    //  Y = aX + b

    bool l1_vert = false;
    double a1 = 0;
    if ( l1[1].x - l1[0].x == 0 )
    {
        // Line 1 is vertical
        l1_vert = true;
    }
    else
    {
        a1 = ( l1[1].y - l1[0].y) / ( l1[1].x - l1[0].x );
    }
    
    double b1 = l1[0].y - l1[0].x * a1;

    bool l2_vert = false;
    double a2 = 0;
    if ( l2[1].x - l2[0].x == 0 )
    {
        // Line 2 is vertical
        l2_vert = true;
    }
    else
    {
        a2 = ( l2[1].y - l2[0].y) / ( l2[1].x - l2[0].x );
    }
    double b2 = l2[0].y - l2[0].x * a2;
    
    // Ok, are the lines parallel?
    if (l1_vert && l2_vert)
    {
        // They don't intersect
        angle = 0;
        return false;
    }
    else if (l1_vert)
    {
        // They intersect when l2 has the same X as l1
        p.x = l1[0].x;
        p.y = static_cast<float>(a2 * p.x + b2);
    }
    else if (l2_vert)
    {
        // They intersect when l1 has the same X as l2
        p.x = l2[0].x;
        p.y = static_cast<float>(a1 * p.x + b1);
    }
    else if (a1 == a2)
    {
        // They don't intersect
        angle = 0;
        return false;        
    }
    else
    {
        
        // Now find the intersection
        // See: http://www.mathopenref.com/coordintersection.html
        
        // Y1 = a1X + b1 = a2X + b2
        // X = (b2 - b1) / (a1 - a2)
        p.x = static_cast<float>((b2 - b1) / (a1 - a2));
        p.y = static_cast<float>(a1 * p.x + b1);
    }
    
    // Find the angle between the lines at the
    // intersection point
    // We are looking at the 3 points:
    // l1[0], p, l2[0]
    // Use cosine rule: cos C = (a^2 + b^2 - c^2) / (2ab)
    {
        double a = distance( l1, &p);
        double b = distance( l2, &p);
        double c = distance( l1, l2);
        
        if (a == 0 && b == 0)
        {
            angle = 0;
        }
        else
        {
            const double pi = 3.14159265359;
            double cs = (a*a + b*b - c*c)/(2*a*b);
            angle = acos( cs ) * 360.0 / (2*pi);
        }
    }
    
    
    return true;
}



// What is the distance between two points
double CLineToPoly::distance( jett_point *p1, jett_point *p2 )
{
    double dx = p1->x - p2->x;
    double dy = p1->y - p2->y;
    return sqrt( dx*dx + dy*dy );
}


