//
//  LinearizationCollection.h
//  GraphicsLibrary
//
//  Created by Matt Pyne on 13/02/2013.
//
//

#ifndef __GraphicsLibrary__LinearizationCollection__
#define __GraphicsLibrary__LinearizationCollection__

#include "Linearization.h"


class CGPUProcessor;

class CLinearizationCollection
{
private:
	// Here is the OpenCL device we are using
    CGPUProcessor*     m_cl;

    typedef std::vector<CLinearization> linearizationCollection;
	linearizationCollection	m_linearizations;
    
	// The openCL data
    cl_mem      m_linearization_cl;

public:
    // Construction
    CLinearizationCollection();
    
    // Add a new linearization file
    void push_back( const CLinearization &lin );
    
    // Get the linearization data as a bundle
	cl_mem* get_linearization_cl( CGPUProcessor* cl);
    
    // Get a single linearization
    unsigned char* get_linearization( int index );
    
    // How many linearization in this collection?
    size_t size() const { return m_linearizations.size(); }

};

#endif /* defined(__GraphicsLibrary__LinearizationCollection__) */
