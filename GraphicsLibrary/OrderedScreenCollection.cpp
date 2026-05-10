//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "jett.h"
#include "OrderedScreenCollection.h"
#include "GPUProcessor.h"

// The file header
const char* COrderedScreenCollection::m_file_header = "jett_screen";

COrderedScreenCollection::COrderedScreenCollection(void)
{
	m_cl = NULL;
	m_screen_cl = NULL;
}


COrderedScreenCollection::~COrderedScreenCollection(void)
{
	if (m_screen_cl)
    {
        m_cl->releaseMemObject( m_screen_cl );
    }
}

void COrderedScreenCollection::createScreens( int planes, const int *size, double sigma, int output_levels, jett_progress_callback callback )
{
	// Discard old screens
	m_screens.erase( m_screens.begin(), m_screens.end() );

	// Generate the new screens
	m_screens.resize( planes );

	for (int i = 0; i < planes; ++ i)
	{
		m_screens[i].create( size[i], size[i], sigma, output_levels, callback );
	}
}


const int* COrderedScreenCollection::get_screen( int screen ) const
{
	return m_screens[ screen ].get_screen();
}

int COrderedScreenCollection::getWidth( int screen ) const
{
	return m_screens[ screen ].getWidth();
}

int COrderedScreenCollection::getHeight( int screen ) const
{
	return m_screens[ screen ].getHeight();
}

double COrderedScreenCollection::getQuantizer( int screen ) const
{
	return m_screens[ screen ].getQuantizer();
}

cl_mem COrderedScreenCollection::get_screen_cl( CGPUProcessor* cl)
{
    m_cl = cl;

	// Create a new buffer
    if (m_cl && !m_screen_cl)
    {
		size_t header_size = getPlanes() * 3;
		size_t data_size = 0;
		for (size_t i = 0; i < getPlanes(); ++ i)
		{
			data_size += m_screens[i].getWidth() * m_screens[i].getHeight();
		}

		size_t total_size = header_size + data_size;
		int* buffer = new int[ total_size ];
		int data_location = static_cast<int>(header_size);
		for (size_t i = 0; i < getPlanes(); ++ i)
		{
			buffer[ i * 3     ] = data_location;
			buffer[ i * 3 + 1 ] = m_screens[i].getWidth();
			buffer[ i * 3 + 2 ] = m_screens[i].getWidth();
			int data_size = m_screens[i].getWidth() * m_screens[i].getHeight();

			memcpy( buffer + data_location, m_screens[i].get_screen(), data_size * sizeof(int) );
			data_location += data_size;
		}

        m_screen_cl = m_cl->createBuffer(CL_MEM_READ_ONLY, total_size * sizeof(int), NULL );
        m_cl->enqueueWriteBuffer(m_screen_cl, CL_TRUE, 0, total_size * sizeof(int), buffer );
		delete[] buffer;
    }
    
    return m_screen_cl;
}

// Save to file
void COrderedScreenCollection::saveToFile( const TCHAR *filename )
{
	// Open the file
    FILE *outfile = NULL;
	_tfopen_s( &outfile, filename,_T("wb"));
	if ( !outfile )
	{
		throw jett_exception(JETT_INVALID_ARGUMENT,0,"Cannot open file for writing");
	}

	// Write out the header
	fwrite( m_file_header, 1, strlen( m_file_header ), outfile );

	int32_t size = static_cast<int>(m_screens.size());
	fwrite( &size, sizeof(size), 1, outfile );

	// Now write out the screens
	for (int i = 0;i < size; ++ i)
	{
		m_screens[i].save( outfile );
	}

	// Done!
	fclose( outfile );
}

// Load from file
void COrderedScreenCollection::loadFromFile( const TCHAR *filename )
{
	// Open the file
    FILE *infile = NULL;
	_tfopen_s( &infile, filename,_T("rb"));
	if ( !infile )
	{
		throw jett_exception(JETT_INVALID_ARGUMENT,0,"Cannot open file for reading");
	}

	// Read in the header
	char buffer[ 16 ];
	if (fread( buffer, 1, strlen(m_file_header), infile) != strlen(m_file_header))
	{
		throw jett_exception(JETT_UNSUPPORTED_SCREEN,0,"Not a valid ordered screen file (invalid header)");
	}

	int32_t planes = 0;
	if (fread( &planes, sizeof(int32_t), 1, infile) != 1 || planes < 1 || planes > 4)
	{
		throw jett_exception(JETT_UNSUPPORTED_SCREEN,0,"Not a valid ordered screen file (invalid planes)");
	}


	// Discard old screens
	m_screens.erase( m_screens.begin(), m_screens.end() );

	// Generate the new screens
	m_screens.resize( planes );

	// Now read in the screens
	for (int i = 0;i < planes; ++ i)
	{
		m_screens[i].load( infile );
	}

	// Done!
	fclose( infile );
}

