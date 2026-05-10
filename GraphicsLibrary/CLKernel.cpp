//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#include "StdAfx.h"
#include "jett.h"

#include "CLKernel.h"
#include "GPUProcessor.h"

// Construction/destruction
CLKernel::CLKernel()
{
    m_kernel = NULL;
    m_workgroup_size = 1;
}

CLKernel::~CLKernel()
{
    free();
}


// Create a kernel
void CLKernel::create( CLProgram &program, const char *name )
{
    m_kernel = program.createKernel( name );    
    
    // Get the maximum work group size for executing the kernel on the device
    //
    int err = clGetKernelWorkGroupInfo(m_kernel, program.getDevice()->get_handle(), CL_KERNEL_WORK_GROUP_SIZE, sizeof(m_workgroup_size), &m_workgroup_size, NULL);
    if (err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to retrieve kernel work group info!");
    }
}

// Destroy this kernel
void CLKernel::free()
{
    if (m_kernel)
    {
        clReleaseKernel(m_kernel);
    }
    m_kernel = NULL;
}


// Set a parameter on this kernel
void CLKernel::setKernelArg(  cl_uint      arg_index,
                  size_t       arg_size,
                  const void * arg_value)
{
    int err = clSetKernelArg(m_kernel, arg_index, arg_size, arg_value );
    
    if (err != CL_SUCCESS)
    {
        throw jett_exception(JETT_OPENCL_FAILURE, err, "Failed to set an argument on a kernel");
    }

}

