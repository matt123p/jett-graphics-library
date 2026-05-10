//
//  Linearization.cpp
//  GraphicsLibrary
//
//  Created by Matt Pyne on 13/02/2013.
//
//

#include "StdAfx.h"
#include "jett.h"
#include "Linearization.h"

// Default constructor (makes a identity linearization)
CLinearization::CLinearization()
{
	// Make a 1:1 mapping for the linearization curve
	for (int i = 0; i < 256; ++i)
	{
		m_linearization[i] = i;
	}
}

// Import linearization from a set of points
//
// If we have "filter" set to true then we will apply noise filtering
// to the curve.
//
// If we have "measurement" set to true, then this is assumed to be a
// measurement curve rather than the correction curve and it will
// be inverted to converted it to a linearization curve.
//
void CLinearization::Import(jett_point* curve, size_t points, int options)
{
	typedef std::map< float, float > pointMap;
	pointMap point;

	jett_point first;
	jett_point last;

	if (points < 2)
	{
		throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Invalid curve, there must be at least 2 points in the curve");
	}

	//  Get the points and normalize
	for (size_t i = 0; i < points; ++i)
	{
		// Is this "x" already in the map?
		if (point.find(curve[i].x) != point.end())
		{
			// Error!
			throw jett_exception(JETT_INVALID_LINEARIZATION, 0, "Invalid curve, at least 2 points share an X co-ordinate");
		}

		// Is the x < 0 or x > 255 ?
		if (curve[i].x < 0 && curve[i].x > 255)
		{
			// Error!
			throw jett_exception(JETT_INVALID_LINEARIZATION, 0, "Invalid curve, at least one point has a X co-ordinate less than 0 or greater than 255");
		}

		// Ok, we can insert the point
		point[curve[i].x] = curve[i].y;
	}

	// Make sure we have a valid curve
	if (point.begin()->first != 0)
	{
		throw jett_exception(JETT_INVALID_LINEARIZATION, 0, "Invalid curve, the first point must have a x-cordinate of 0");
	}
	if (point.rbegin()->first != 255)
	{
		throw jett_exception(JETT_INVALID_LINEARIZATION, 0, "Invalid curve, the last point must have a x-cordinate of 255");
	}

	// Extract the first and last point, then we normalize
	double first_y = point.begin()->second;
	double last_y = point.rbegin()->second;
	double norm = 255.0 / (last_y - first_y);

	// Normalize so the y values run from 0..255
	for (pointMap::iterator i = point.begin(); i != point.end(); ++i)
	{
		i->second = static_cast<float>((i->second - first_y) * norm);
	}

	// We can now use linear interpolation to make the curve
	for (int i = 0; i < 256; ++i)
	{
		// Find the closest value (that is bigger or the same as this index)
		pointMap::iterator u = point.lower_bound(static_cast<float>(i));
		pointMap::iterator l = u;
		if (l != point.begin())
		{
			--l;
		}

		double x1 = l->first;
		double y1 = l->second;
		double dx = u->first - l->first;
		double dy = u->second - l->second;

		m_linearization[i] = static_cast<unsigned char>(y1 + ((i - x1) / dx) * dy);
		// printf( "%d\t%d\n", i, m_linearization[i]);
	}
}

// Return the actual data
unsigned char* CLinearization::get_linearization()
{
	return m_linearization;
}