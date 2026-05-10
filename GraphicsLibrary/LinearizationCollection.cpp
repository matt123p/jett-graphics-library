//
//  LinearizationCollection.cpp
//  GraphicsLibrary
//
//  Created by Matt Pyne on 13/02/2013.
//
//

#include "StdAfx.h"

#include "jett.h"
#include "GPUProcessor.h"
#include "LinearizationCollection.h"


CLinearizationCollection::CLinearizationCollection()
{
    m_linearization_cl = NULL;
    m_cl = NULL;
}

void CLinearizationCollection::push_back( const CLinearization &lin )
{
    m_linearizations.push_back( lin );
}

cl_mem* CLinearizationCollection::get_linearization_cl( CGPUProcessor* cl)
{
    m_cl = cl;
    
	// Create a new buffer
    if (m_cl && !m_linearization_cl)
    {
		size_t total_size = m_linearizations.size() * 256;
        
		unsigned char* buffer = new unsigned char[ total_size ];
		for (size_t i = 0; i < m_linearizations.size(); ++ i)
		{
			memcpy( buffer + i * 256 , get_linearization( i ), 256 );
		}
        
        m_linearization_cl = m_cl->createBuffer(CL_MEM_READ_ONLY, total_size, NULL );
        m_cl->enqueueWriteBuffer(m_linearization_cl, CL_TRUE, 0, total_size, buffer );
		delete[] buffer;
    }
    
    return &m_linearization_cl;
}


unsigned char* CLinearizationCollection::get_linearization( int index )
{
    return m_linearizations[ index ].get_linearization();
}

