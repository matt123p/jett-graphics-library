//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#include "StdAfx.h"

#include "GPUProcessor.h"
#include "CLKernel.h"

#include "GPUBitblt.h"
#include "GPUPolygon.h"
#include "Image.h"


#ifndef __MACH__
// Trace function for debugging
void _trace( const char *fmt, ... )
{
	va_list args;
	int len;
	char * buffer;

	va_start( args, fmt );
	len = _vscprintf( fmt, args ) + 1; // terminating '\0'
	buffer = new char[ len ];
	vsprintf_s( buffer,len, fmt, args );
	OutputDebugStringA( buffer );
    delete [] buffer;
}
#else
#define _trace printf
#endif

// Construction/destruction
CGPUProcessor::CGPUProcessor()
{
    m_device_id = NULL;        // compute device id 
    m_context = NULL;          // compute context
    m_commands = NULL;         // compute command queue
    
    memset( m_bitblt, 0, sizeof(m_bitblt) );
    memset( m_polygon, 0, sizeof(m_polygon) );
	m_dither = NULL;
}

// clGetPlatformIDs

CGPUProcessor::~CGPUProcessor()
{
    // Destroy the pre-compiled kernels
    for (int i = 0; i < sizeof(m_bitblt) / sizeof(m_bitblt[0]); ++ i)
    {
        if (m_bitblt[i])
        {
            delete m_bitblt[i];
        }
    }
    for (int i = 0; i < sizeof(m_polygon) / sizeof(m_polygon[0]); ++ i)
    {
        if (m_polygon[i])
        {
            delete m_polygon[i];
        }
    }
    delete m_dither;

    // Release the shared resources
    staticMemCollection::iterator i = m_static_resources.begin();
    while (i != m_static_resources.end())
    {
        releaseMemObject(i->second);
        ++ i;
    }

    
    if (m_commands)
    {
        clReleaseCommandQueue(m_commands);
    }
    
    if (m_context)
    {
        clReleaseContext(m_context);
    }
}


// Open a new OpenCL context for talking to
// the OpenCL device.  This only needs to be called once
void CGPUProcessor::Open()
{
    int err;

    std::string opencl_load_error;
    if (!jett_opencl::ensure_loaded(&opencl_load_error))
    {
        throw jett_exception(JETT_OPENCL_FAILURE, 0, opencl_load_error.c_str());
    }

	// Enumerate the platforms
	cl_uint num_platforms;
	cl_platform_id platforms[ 8 ];
	err = clGetPlatformIDs(sizeof(platforms) / sizeof(platforms[0]), platforms, &num_platforms);
    if (err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to enumerate platforms");
    }

    
    // Connect to a compute device
    //
	bool have_device = false;
	for (int t = 0; t < 3 && !have_device; ++ t)
	{
		for (size_t i = 0; i < num_platforms; ++i)
		{
			// We search for OpenCL devices in this order
			int tp = 0;
			switch (t)
			{
			case 0:
				tp = CL_DEVICE_TYPE_GPU;
				break;
			case 1:
				tp = CL_DEVICE_TYPE_DEFAULT;
				break;
			case 2:
				tp = CL_DEVICE_TYPE_ACCELERATOR;
				break;
			}

			err = clGetDeviceIDs(platforms[i], tp, 1, &m_device_id, NULL);
			if (err == CL_SUCCESS)
			{
				have_device = true;
				break;
			}
		}
	}
	if (!have_device)
	{
		throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to connect to a GPU device");
	}

	{
		// Print the device's information
		char name[ 256 ];
		clGetDeviceInfo( m_device_id, CL_DEVICE_NAME, sizeof( name ), name, NULL );
		_trace( "jett: Using %s\n", name );
	}

    {
        cl_int p;
        clGetDeviceInfo( m_device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof( p ), &p, NULL );
        _trace( "jett: Max compute units: %d\n", p );
    }
    
    {
        cl_uint p;
        clGetDeviceInfo( m_device_id, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof( p ), &p, NULL );
        _trace( "jett: Max clock frequency: %uMHz\n", p );
    }

    {
        cl_ulong p;
        clGetDeviceInfo( m_device_id, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof( p ), &p, NULL );
        _trace( "jett: Max memory allocation: %lluMbytes\n", p / (1024*1024) );
    }

    
    // Create a compute context 
    //
    m_context = clCreateContext(0, 1, &m_device_id, NULL, NULL, &err);
    if (!m_context)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to create a compute context");
    }
    
    // Create a command queue
    //
    m_commands = clCreateCommandQueue(m_context, m_device_id, 0, &err);
    if (!m_commands)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to create a command queue");
    }
    
    // Create helper objects
    //
    // Pre-compile the bitblt operators
    m_bitblt[0] = new CGPUBitblt(this, 3, true  );     // RGB + Alpha support
    m_bitblt[1] = new CGPUBitblt(this, 1, false );     // Monochrome
    m_bitblt[3] = new CGPUBitblt(this, 3, false );     // RGB
    m_bitblt[4] = new CGPUBitblt(this, 4, false );     // CMYK
    
    // Don't pre-compile polygon operators until required
    m_polygon[0]  = NULL;
    m_polygon[1]  = NULL;

	// Don't pre-compile dither operators until required
	m_dither = NULL;
}


//
// Push an image in to the GPU
//
void CGPUProcessor::cache_image( CImage *image, bool read_only ) 
{ 
	image->lockCLData( this, read_only );
	image->unlockCLData( this );
} 


// Access the polygon kernels
CPolygon*  CGPUProcessor::polygon(size_t index) 
{ 
	if (!m_polygon[0])
	{
	    m_polygon[0]  = new CGPUPolygon(this, 0);
	}

	if (!m_polygon[1])
	{
	    m_polygon[1]  = new CGPUPolygon(this, 4);
	}

	return m_polygon[ index ]; 
}


// Access the dither kernels
CDither* CGPUProcessor::dither()
{
	if (!m_dither)
	{
		m_dither = new CGPUDither( this );
	}

	return m_dither;
}

//
// In our program we have some shared static resources, the device
// object keeps track of them.
//
cl_mem CGPUProcessor::get_shared_resource( const char *name, const void* p, size_t size )
{
    staticMemCollection::iterator i = m_static_resources.find( name );
    if (i == m_static_resources.end())
    {
        if (p)
        {
            // We must create this in memory
            cl_mem r = createBuffer( CL_MEM_READ_ONLY, size, NULL );
            enqueueWriteBuffer(r,CL_TRUE,0,size,p);
            m_static_resources[ name ] = r;
            return r;
        }
        else
        {
            return NULL;
        }
    }
    
    return i->second;
}

// Compile a program from text
cl_program CGPUProcessor::createProgram( const unsigned char* src, const char* build_options )
{
    int err = CL_SUCCESS;
    
    cl_program program;
    const char *s = (const char*)src;
    program = clCreateProgramWithSource(m_context, 1, &s, NULL, &err);
    
    if (!program || err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to create compute program!\n");
    }

    
    // Build the program executable
    //
    err = clBuildProgram(program, 0, NULL, build_options, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        String message;
        size_t len;
        char *buffer = NULL;
        clGetProgramBuildInfo(program, m_device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
		buffer = new char[ len + 10 ];
        clGetProgramBuildInfo(program, m_device_id, CL_PROGRAM_BUILD_LOG, len+1, buffer, &len);


        /* DON'T DUMP PROGRAM as this leaks it to the end-user! */
#if 1
        message += buffer;
        if (buffer[0] == 0)
        {
            message += buffer+1;            
        }
		delete [] buffer;
        message += "\n";
        printf("Compiler error: %s\n", message.c_str() );
#endif
        
        throw jett_exception( JETT_OPENCL_FAILURE, err, "Failed to build program executable" );
        
    }
    
    return program;
}



// Create a program from a previously saved compiled binary
cl_program CGPUProcessor::createProgramFromBinary (	
 		cl_uint num_devices,
 		const cl_device_id *device_list,
 		const size_t *lengths,
 		const unsigned char **binaries,
 		cl_int *binary_status )
{
	int err = CL_SUCCESS;
    
    cl_program program;
    program = clCreateProgramWithBinary (	
 		m_context, num_devices, device_list, lengths, binaries,
 		binary_status, &err );

    if (!program || err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to create compute program!\n");
    }

    
    // Build the program executable
    //
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
		throw jett_exception( JETT_OPENCL_FAILURE, err, "Failed to build program executable" );
    }
    
    return program;
}

// Create a memory buffer
cl_mem CGPUProcessor::createBuffer(cl_mem_flags flags, size_t size, void *host_ptr )
{
    int err;
    
    // For some reason things don't work if the buffer is too
    // small
    size = std::max(size, static_cast<size_t>(512) );
    cl_mem r = clCreateBuffer( m_context, flags, size, host_ptr, &err );

    if (!r || err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to create a memory buffer");
    }
    
    return r;
}


// Create sub-buffer from a buffer
cl_mem CGPUProcessor::createSubBuffer(cl_mem     buffer,
                       cl_mem_flags             flags,
                       cl_buffer_create_type    buffer_create_type,
                       const void *             buffer_create_info )
{
    int err = 0;
    
    cl_mem r = clCreateSubBuffer(buffer, flags, buffer_create_type, buffer_create_info, &err );
    if (!r || err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to create a memory sub-buffer");
    }
    
    return r;
}


void CGPUProcessor::enqueueWriteBuffer(
                        cl_mem           buffer, 
                        cl_bool            blocking_write, 
                        size_t             offset, 
                        size_t             cb, 
                        const void *       ptr )
{
    int err = clEnqueueWriteBuffer( m_commands, buffer, blocking_write, offset, cb, ptr, 0, NULL, NULL); 
    if (err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to write to OpenCL buffer");
    }
}

// Read a memory buffer
void CGPUProcessor::enqueueReadBuffer(
                       cl_mem            buffer,
                       cl_bool             blocking_read,
                       size_t              offset,
                       size_t              cb, 
                       void *              ptr)
{
    int err = clEnqueueReadBuffer( m_commands, buffer, blocking_read, offset, cb, ptr, 0, NULL, NULL );
    if (err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to read from OpenCL buffer");
    }
}

void CGPUProcessor::releaseMemObject( cl_mem buffer )
{
    int err = clReleaseMemObject( buffer );
    if (err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to release OpenCL buffer");
    }
}

// Wait for a kernel execution to finish
void CGPUProcessor::finish()
{
    int err = clFinish(m_commands);
    
    if (err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to wait for command queue finish");
    }
}

// Run a kernel
void CGPUProcessor::enqueueNDRangeKernel(
                          CLKernel&        kernel,
                          cl_uint          work_dim,
                          const size_t *   global_work_offset,
                          const size_t *   global_work_size,
                          const size_t *   local_work_size )
{
    int err = clEnqueueNDRangeKernel(m_commands, kernel.get_handle(), work_dim, global_work_offset, global_work_size, local_work_size, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to run kernel");
    }
}


