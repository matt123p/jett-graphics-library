//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "OrderedBitmap.h"

#define _USE_MATH_DEFINES 
#include <math.h>


COrderedBitmap::COrderedBitmap(jett_progress_callback callback )
{
    m_callback = callback;
	m_kernel_size = 0;
	m_kernel = NULL;
	m_ones = 0;
	m_rank = 0;
    
    m_progress.m_phase = 0;
    m_progress.m_iteration = 0;
    m_progress.m_total = 0;
}


COrderedBitmap::~COrderedBitmap(void)
{
	delete m_kernel;
}


//
// Create a new randomly filled binary bitmap with the fill factor (in percent).
//
void COrderedBitmap::make_initial_pattern( int width, int height, int fill, double sigma )
{
	// Create the image
	m_binary_pattern.createImage( width, height, image_mono, false, image_mode_default );
	m_inital_binary_pattern.createImage( width, height, image_mono, false, image_mode_default );
	m_binary_pattern.erase();

    // Callback
    m_progress.m_phase = 1;
    m_progress.m_iteration = 0;
    m_progress.m_total = width*height/2;
    if (m_callback)
    {
        m_callback( m_progress );
    }
    
    
	// Now fill it
	unsigned char *data = m_binary_pattern.lockData( false );
	m_ones = (width * height * fill ) / 100;
	int pixels = m_ones;

	while (pixels > 0)
	{
		int x = rand() % width;
		int y = rand() % height;

		unsigned char *p = data + m_binary_pattern.getStride() * y + x;
		if (*p != 0)
		{
			*p = 0;
			-- pixels;
		}
	}

	// Save a copy for inspection
#ifdef DUMP
	m_binary_pattern.saveToFile( _T("c:\\temp\\random.bmp") );
#endif

	// Create the gaussian blur matrix
	m_kernel_size = static_cast<int>(ceil(sigma))*2 + 3;
	m_kernel = new double[ m_kernel_size ];
	int mean = m_kernel_size / 2; 
	double factor = sqrt(2 * M_PI) * sigma;
	double sigsq = 2 * sigma * sigma;

	double sum = 0;
	for (int x = 0; x < m_kernel_size; ++x)
	{
		m_kernel[x] = exp( -(x-mean) * (x-mean) / sigsq ) / factor;
		sum += m_kernel[x];
	}

	// Now normalise the kernel
	for (int x = 0; x < m_kernel_size; ++x)
	{
		m_kernel[x] = m_kernel[x] / sum;
	}

	// Now redistribute to make a blue noise 
	// bitmap
	redistrubute();

	// Finally copy the binary pattern in to the initial binary pattern
	unsigned char* src = m_binary_pattern.lockData( true );
	unsigned char* dst = m_inital_binary_pattern.lockData( false );
	memcpy( dst, src, m_binary_pattern.getHeight() * m_binary_pattern.getStride() );
	m_binary_pattern.unlockData();
	m_inital_binary_pattern.unlockData();
}



//
// Generate the ranking
//
void COrderedBitmap::make_screen( COrderedScreen *screen )
{
	int mn = m_inital_binary_pattern.getWidth() * m_inital_binary_pattern.getHeight();

	// PHASE I
	init_binary_pattern();

#ifdef DUMP
	m_binary_pattern.saveToFile( _T("c:\\temp\\1.bmp") );
#endif
   
    // Callback
    m_progress.m_phase = 2;
    m_progress.m_iteration = 0;
    m_progress.m_total = mn;
    if (m_callback)
    {
        m_callback( m_progress );
    }

	int rank = m_ones - 1;
	while (rank >= 0)
	{
		// Find the largest cluster
		int cx,cy;
		findMaxCluster( cx,cy, true );

		// Make the pixel white
		set_pixel( cx,cy, 255 );

		screen->set_rank( cx,cy,rank );

        // Callback
        ++ m_progress.m_iteration;
        if (m_callback)
        {
            m_callback( m_progress );
        }
        
		-- rank;
	}

#ifdef DUMP
	m_binary_pattern.saveToFile( _T("c:\\temp\\2.bmp") );
#endif

	// Phase II
	init_binary_pattern();

#ifdef DUMP
	m_binary_pattern.saveToFile( _T("c:\\temp\\3.bmp") );
#endif

	rank = m_ones;
	while (rank < mn / 2)
	{
		// Find the largest void
		int vx,vy;
		findMaxCluster( vx,vy, false );

		// Make the pixel black
		set_pixel( vx,vy, 0 );

		screen->set_rank( vx,vy,rank );

        // Callback
        ++ m_progress.m_iteration;
        if (m_callback)
        {
            m_callback( m_progress );
        }
        

		++ rank;
	}

	// Phase III
#ifdef DUMP
	m_binary_pattern.saveToFile( _T("c:\\temp\\4.bmp") );
#endif

	invert_binary_pattern();

#ifdef DUMP
	m_binary_pattern.saveToFile( _T("c:\\temp\\5.bmp") );
#endif

	while (rank < mn)
	{
        // Find the largest cluster
		int cx,cy;
		findMaxCluster( cx,cy, true );

		// Make the pixel white
		set_pixel( cx,cy, 255 );

		screen->set_rank( cx,cy,rank );

        // Callback
        ++ m_progress.m_iteration;
        if (m_callback)
        {
            m_callback( m_progress );
        }
        

		++ rank;
	}

#ifdef DUMP
	m_binary_pattern.saveToFile( _T("c:\\temp\\6.bmp") );
#endif
}


// Copy the initial binary pattern in to the binary pattern
void COrderedBitmap::init_binary_pattern()
{
	unsigned char* src = m_inital_binary_pattern.lockData( true );
	unsigned char* dst = m_binary_pattern.lockData( false );
	memcpy( dst, src, m_binary_pattern.getHeight() * m_binary_pattern.getStride() );
	m_binary_pattern.unlockData();
	m_inital_binary_pattern.unlockData();
}

// Invert the binary pattern
void COrderedBitmap::invert_binary_pattern()
{
	unsigned char *data = m_binary_pattern.lockData( false );
	
	for (int x = 0; x < m_binary_pattern.getWidth(); ++ x)
	{
		for (int y = 0; y < m_binary_pattern.getHeight(); ++ y)
		{

			unsigned char *p = data + m_binary_pattern.getStride() * y + x;
			if (*p != 0)
			{
				*p = 0;
			}
			else
			{
				*p = 255;
			}
		}
	}

	m_binary_pattern.unlockData();
}

// Set a pixel in the binary pattern
void COrderedBitmap::set_pixel( int x, int y, unsigned char c )
{
	unsigned char* data = m_binary_pattern.lockData( true );
	unsigned char* p = data + m_binary_pattern.getStride() * y + x;
	*p = c;
	m_binary_pattern.unlockData();
}


//
// Test a pixel to get it's void or cluster value
//
double COrderedBitmap::measurePixel( int x, int y, bool cluster )
{
	// We blur horizontally and vertically independently.
	// We need an output buffer to accumulate the results in to.
	//
	int df = m_kernel_size / 2;
	unsigned char *data = m_binary_pattern.lockData( true );

	// Is this the correct type of pixel?
	unsigned char *p = data + m_binary_pattern.getStride() * y + x;
	if (   (*p != 0 && cluster)
		|| (*p == 0 && !cluster) )
	{
		m_binary_pattern.unlockData();
		return 0;
	}

	double* buffer = new double[ m_kernel_size ];

	//
	// Vertical
	//
#pragma omp parallel for
	for (int dx = 0; dx < m_kernel_size; ++ dx )
	{
		buffer[ dx ] = 0;

		for (int dy = 0; dy < m_kernel_size; ++ dy )
		{
			// Determine the pixel to look at
			int sample_x = x + dx - df;
			int sample_y = y + dy - df;

			// Wrap around the tile
			if (sample_x < 0)
			{
				sample_x += m_binary_pattern.getWidth();
			}
			if (sample_x >= m_binary_pattern.getWidth())
			{
				sample_x -= m_binary_pattern.getWidth();
			}
			if (sample_y < 0)
			{
				sample_y += m_binary_pattern.getHeight();
			}
			if (sample_y >= m_binary_pattern.getHeight())
			{
				sample_y -= m_binary_pattern.getHeight();
			}

			// Now sample the pixel
			double pixel = data[ sample_x + sample_y * m_binary_pattern.getStride() ];

			if (cluster)
			{
				// Count up black (0) pixels
				pixel = pixel != 0 ? 0 : 1;
			}
			else
			{
				// Count up while (non-zero) pixels
				pixel = pixel == 0 ? 0 : 1;
			}
			buffer[dx] += m_kernel[dy] * pixel;
		}
	}


	//
	// Horizontal
	//
	double r = 0;
	for (int dx = 0; dx < m_kernel_size; ++ dx )
	{
		// Now apply the kernel
		r += buffer[dx] * m_kernel[dx];
	}
    
    delete[] buffer;

	m_binary_pattern.unlockData();

	return r;
}


//
// Find the largest void or cluster
//
void COrderedBitmap::findMaxCluster( int &x, int &y, bool cluster )
{
	x = 0;
	y = 0;
	double best = 0;

	typedef std::pair<int,int>	cord_t;
	typedef std::vector<cord_t>	cordCollection;
	cordCollection candidates;

	for (int tx = 0; tx < m_binary_pattern.getWidth(); ++ tx)
	{
		for (int ty = 0; ty < m_binary_pattern.getHeight(); ++ ty)
		{
			double r = measurePixel( tx,ty, cluster);
			if ( r > best )
			{
				candidates.erase( candidates.begin(), candidates.end() );
				best = r;
				candidates.push_back( cord_t( tx, ty ) );
			}
			else if (r == best)
			{
				candidates.push_back( cord_t( tx, ty ) );
			}
		}
	}

	// Select from the list of candidates
	int index = rand() % candidates.size();
	x = candidates[ index ].first;
	y = candidates[ index ].second;
}


//
// Make the bitmap have blue noise
//
void COrderedBitmap::redistrubute()
{
	// We find the largest cluster and move it to the largest void
	// until we run out of iterations or the void and cluster are
	// the same 
	for (int loop = 0; loop < m_binary_pattern.getWidth() * m_binary_pattern.getHeight() * 2; ++ loop)
	{
		// Find the largest cluster
		int cx,cy;
		findMaxCluster( cx,cy, true );

		// Make the pixel white
		set_pixel( cx,cy, 255 );

		// Find the largest void
		int vx,vy;
		findMaxCluster( vx,vy, false );

		// Make the pixel black
		set_pixel( vx,vy, 0 );

		// Is this the end?
		if (cx == vx && cy == vy)
		{
			break;
		}
        
        // Callback
        ++ m_progress.m_iteration;
        if (m_callback)
        {
            m_callback( m_progress );
        }
	}

#ifdef DUMP
	m_binary_pattern.saveToFile( _T("c:\\temp\\blue.bmp") );
#endif
}
