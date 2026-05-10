//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GenericOrdering_CLKernel_h
#define GenericOrdering_CLKernel_h

#include "CLProgram.h"

//
// This class wraps an OpenCL Kernel object
// 

class CLKernel
{
private:
    cl_kernel   m_kernel;                  // compute kernel
    
    // The maximum size of a workgroup
    size_t      m_workgroup_size;


public:
    
    // Construction/destruction
    CLKernel();
    ~CLKernel();
    
    // Create a kernel
    void create( CLProgram &program, const char *name );
    
    // Destroy this kernel
    void free();
    
    // Get the underlying native handle
    cl_kernel get_handle() { return m_kernel; }
    
    // Set a parameter on this kernel
    void setKernelArg(  cl_uint      arg_index,
                   size_t       arg_size,
                 const void * arg_value);
    
    // Get the max size of the workgroup
    size_t  get_workgroup_size() const { return m_workgroup_size; }
};

#endif
