//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GenericOrdering_CLProgram_h
#define GenericOrdering_CLProgram_h

typedef std::string String;
typedef std::map<String,String> stringCollection;

class CGPUProcessor;

// This class wraps an OpenCL program object
//

class CLProgram
{
private:
    CGPUProcessor*		m_pDevice;
    cl_program			m_program;                 // compute program

public:

    // Construction/destruction
    CLProgram();
    ~CLProgram();

    // Create this program
    void create(  CGPUProcessor &cl, const unsigned char *program, size_t len, const stringCollection *defs = NULL, const char* build_options = NULL );

    // Destroy this program
    void free();
    
    // Create a kernel
    cl_kernel createKernel( const char *kernel_name );
    
    // Get the device
    CGPUProcessor* getDevice() { return m_pDevice; }

	// Load/Save the compiled kernel
	void saveKernelBinary( TCHAR *filename );
	void loadKernelBinary( TCHAR *filename );
};



#endif
