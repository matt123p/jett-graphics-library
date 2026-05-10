//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_OrderedScreenCollection_h
#define GraphicsLibrary_OrderedScreenCollection_h

#include "jett.h"
#include "OrderedScreen.h"

class CGPUProcessor;


class COrderedScreenCollection
{
private:
	// Here is the OpenCL device we are using
    CGPUProcessor*     m_cl;

	typedef std::vector<COrderedScreen> screenCollection;
	screenCollection	m_screens;

	// The openCL data
    cl_mem      m_screen_cl;

	// The file header
	static const char* m_file_header;

public:
	COrderedScreenCollection(void);
	~COrderedScreenCollection(void);

	// Generate the set of screens
	void createScreens( int planes, const int *size, double sigma, int output_levels, jett_progress_callback callback );

	// How many screens are there in this collection?
	size_t getPlanes() const { return m_screens.size(); }

	// Get the screen data
	const int* get_screen( int screen ) const;
	int getWidth( int screen ) const;
	int getHeight( int screen ) const;
	double getQuantizer( int screen ) const;

	// Get the screen data as a bundle
	cl_mem get_screen_cl( CGPUProcessor* cl);

	// Load/save to file
	void saveToFile( const TCHAR *filename );
	void loadFromFile( const TCHAR *filename );
};

#endif

