//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_OrderedBitmap_h
#define GraphicsLibrary_OrderedBitmap_h

#include "Image.h"
#include "OrderedScreen.h"

//
// This class is used to generate the void&cluster bitmap to generate
// the ordered screen.
//

class COrderedBitmap
{
private:

	CImage	m_inital_binary_pattern;
	CImage	m_binary_pattern;
	int		m_ones;
	int		m_rank;

	int		m_kernel_size;
	double*	m_kernel;

    // The callback
    jett_progress          m_progress;
    jett_progress_callback m_callback;
    
	// Test a pixel to get it's void or cluster value
	double measurePixel( int x, int y, bool cluster );

	// Find the largest void or cluster
	void findMaxCluster( int &x, int &y, bool cluster );

	// Make the bitmap have blue noise
	void redistrubute();

	// Copy the initial binary pattern in to the binary pattern
	void init_binary_pattern();
	
	// Invert the binary pattern
	void invert_binary_pattern();

	// Set a pixel in the binary pattern
	void set_pixel( int x, int y, unsigned char c );

public:
	COrderedBitmap(jett_progress_callback callback);
	~COrderedBitmap(void);

	//
	// Create a new randomly filled binary bitmap with the fill factor (in percent).
	//
	void make_initial_pattern( int width, int height, int fill, double sigma );

	//
	// Generate the ranking
	//
	void make_screen( COrderedScreen *screen );
};

#endif
