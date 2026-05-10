//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#ifndef CGPUProcessor_h
#define CGPUProcessor_h

#include <string>
#include "Processor.h"
#include "GPUDither.h"


class CLKernel;
class CLProgram;

class CBitblt;
class CPolygon;
class CImage;

// This class wraps the main interface with the OpenCL library.
// There should be only one version of this class in the program.

class CGPUProcessor : public CProcessor
{
private:

    cl_device_id        m_device_id;        // compute device id 
    cl_context          m_context;          // compute context
    cl_command_queue    m_commands;         // compute command queue

    typedef std::map<String, cl_mem>    staticMemCollection;
    staticMemCollection m_static_resources;
    
    // The bitblt pre-compiled kernels we can call on
    // (each one for different source dimensions)
    CBitblt*  m_bitblt[5];
    
    // The polygon fill kernels
    CPolygon* m_polygon[2];

	// The dither kernel
	CGPUDither*	  m_dither;

public:
    
    // Construction/destruction
    CGPUProcessor();
    ~CGPUProcessor();
    
    // Find and open the OpenCL device.
    // This throws an exception if there is a problem
    virtual void Open();
    
    // Access the bitblt kernels
    virtual CBitblt* bitblt( size_t index ) { return m_bitblt[ index ]; }
    
    // Access the polygon kernels
    virtual CPolygon* polygon(size_t index);
    
    // Do we have an attached CGPUProcessor (i.e. a GPU?)
    virtual CGPUProcessor*  get_cl_device() { return this; }

	// Access the dither kernels
	virtual CDither* dither();

	// Push an image in to the GPU
	virtual void cache_image( CImage *image, bool read_only );

    // Get the device id
    cl_device_id get_handle() { return m_device_id; }
    
    // Compile a program from text
    cl_program createProgram( const unsigned char* src, const char* build_options = NULL );

	// Create a program from a previously saved compiled binary
    cl_program createProgramFromBinary (	
 			cl_uint num_devices,
 			const cl_device_id *device_list,
 			const size_t *lengths,
 			const unsigned char **binaries,
 			cl_int *binary_status );
    
    // Get a shared static resource
    cl_mem get_shared_resource( const char *name, const void* p, size_t size );
    
    // Create a memory buffer
    virtual cl_mem createBuffer(cl_mem_flags flags, size_t size, void *host_ptr );
    
    // Write to a memory buffer
    virtual void enqueueWriteBuffer(
                         cl_mem             buffer, 
                         cl_bool            blocking_write, 
                         size_t             offset, 
                         size_t             cb, 
                         const void *       ptr );
    
    // Read a memory buffer
    virtual void enqueueReadBuffer(
                        cl_mem              buffer,
                        cl_bool             blocking_read,
                        size_t              offset,
                        size_t              cb, 
                        void *              ptr );

    // Create sub-buffer from a buffer
    virtual cl_mem createSubBuffer(cl_mem              buffer,
                      cl_mem_flags             flags,
                      cl_buffer_create_type    buffer_create_type,
                      const void *             buffer_create_info );

    
    // Release a memory buffer
    virtual void releaseMemObject( cl_mem buffer );

    // Wait for a kernel execution to finish
    virtual void finish();
    
    // Run a kernel
    void enqueueNDRangeKernel(
                           CLKernel&        kernel,
                           cl_uint          work_dim,
                           const size_t *   global_work_offset,
                           const size_t *   global_work_size,
                           const size_t *   local_work_size );

};

#endif
