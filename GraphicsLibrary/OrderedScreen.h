//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_OrderedScreen_h
#define GraphicsLibrary_OrderedScreen_h

//
// This is an screen for an ordered dither
//

class CGPUProcessor;

class COrderedScreen
{
private:
	// The screen parameters
	int		m_width;
	int		m_height;
	int*	m_screen;

	// The calculated quantizer
	double	m_quantizer;

	// The the number of output levels
	void normalize( int input_levels, int output_levels );

public:
	COrderedScreen(void);
	~COrderedScreen(void);

	// Generate a screen
	void create( int width, int height, double sigma, int output_levels, jett_progress_callback callback );
	void set_rank( int x, int y, int rank );

	// Save/Load a screen embedded in a file
	void save( FILE *fout );
	void load( FILE *fin );

	// Debug
	void dump();
	
	// Access the raw screen data
	const int* get_screen() const;

	// Get the parameters associated with the screen
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	double getQuantizer() const { return m_quantizer; }
};

#endif
