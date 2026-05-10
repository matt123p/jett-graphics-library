#ifndef jett_OPENCL_STUB_H
#define jett_OPENCL_STUB_H

#include <stddef.h>

extern "C"
{
    typedef int cl_int;
    typedef unsigned int cl_uint;
    typedef cl_uint cl_bool;
    typedef unsigned long cl_bitfield;
    typedef cl_bitfield cl_device_type;
    typedef cl_bitfield cl_mem_flags;
    typedef cl_uint cl_buffer_create_type;
    typedef unsigned long long cl_ulong;
    typedef float cl_float;

    struct _cl_platform_id;
    struct _cl_device_id;
    struct _cl_context;
    struct _cl_command_queue;
    struct _cl_mem;
    struct _cl_program;
    struct _cl_kernel;

    typedef _cl_platform_id* cl_platform_id;
    typedef _cl_device_id* cl_device_id;
    typedef _cl_context* cl_context;
    typedef _cl_command_queue* cl_command_queue;
    typedef _cl_mem* cl_mem;
    typedef _cl_program* cl_program;
    typedef _cl_kernel* cl_kernel;

    static const cl_int CL_SUCCESS = 0;
    static const cl_int CL_DEVICE_NOT_FOUND = -1;
    static const cl_bool CL_TRUE = 1;

    static const cl_device_type CL_DEVICE_TYPE_DEFAULT = 1;
    static const cl_device_type CL_DEVICE_TYPE_GPU = 1 << 2;
    static const cl_device_type CL_DEVICE_TYPE_ACCELERATOR = 1 << 3;

    static const cl_mem_flags CL_MEM_READ_WRITE = 1 << 0;
    static const cl_mem_flags CL_MEM_READ_ONLY = 1 << 2;

    static const cl_uint CL_DEVICE_NAME = 0x102B;
    static const cl_uint CL_DEVICE_MAX_COMPUTE_UNITS = 0x1002;
    static const cl_uint CL_DEVICE_MAX_CLOCK_FREQUENCY = 0x100C;
    static const cl_uint CL_DEVICE_MAX_MEM_ALLOC_SIZE = 0x1010;
    static const cl_uint CL_KERNEL_WORK_GROUP_SIZE = 0x11B0;
    static const cl_uint CL_PROGRAM_BUILD_LOG = 0x1183;
    static const cl_uint CL_PROGRAM_NUM_DEVICES = 0x1162;
    static const cl_uint CL_PROGRAM_BINARY_SIZES = 0x1165;
    static const cl_uint CL_PROGRAM_BINARIES = 0x1166;

    static cl_int clGetPlatformIDs(cl_uint, cl_platform_id*, cl_uint *num_platforms)
    {
        if (num_platforms)
            *num_platforms = 0;
        return CL_DEVICE_NOT_FOUND;
    }

    static cl_int clGetDeviceIDs(cl_platform_id, cl_device_type, cl_uint, cl_device_id*, cl_uint *num_devices)
    {
        if (num_devices)
            *num_devices = 0;
        return CL_DEVICE_NOT_FOUND;
    }

    static cl_int clGetDeviceInfo(cl_device_id, cl_uint, size_t, void*, size_t *param_value_size_ret)
    {
        if (param_value_size_ret)
            *param_value_size_ret = 0;
        return CL_DEVICE_NOT_FOUND;
    }

    static cl_context clCreateContext(const void*, cl_uint, const cl_device_id*, void*, void*, cl_int *errcode_ret)
    {
        if (errcode_ret)
            *errcode_ret = CL_DEVICE_NOT_FOUND;
        return 0;
    }

    static cl_command_queue clCreateCommandQueue(cl_context, cl_device_id, cl_ulong, cl_int *errcode_ret)
    {
        if (errcode_ret)
            *errcode_ret = CL_DEVICE_NOT_FOUND;
        return 0;
    }

    static cl_program clCreateProgramWithSource(cl_context, cl_uint, const char **, const size_t*, cl_int *errcode_ret)
    {
        if (errcode_ret)
            *errcode_ret = CL_DEVICE_NOT_FOUND;
        return 0;
    }

    static cl_program clCreateProgramWithBinary(cl_context, cl_uint, const cl_device_id*, const size_t*, const unsigned char **, cl_int *binary_status, cl_int *errcode_ret)
    {
        if (binary_status)
            *binary_status = CL_DEVICE_NOT_FOUND;
        if (errcode_ret)
            *errcode_ret = CL_DEVICE_NOT_FOUND;
        return 0;
    }

    static cl_int clBuildProgram(cl_program, cl_uint, const cl_device_id*, const char*, void*, void*)
    {
        return CL_DEVICE_NOT_FOUND;
    }

    static cl_kernel clCreateKernel(cl_program, const char*, cl_int *errcode_ret)
    {
        if (errcode_ret)
            *errcode_ret = CL_DEVICE_NOT_FOUND;
        return 0;
    }

    static cl_mem clCreateBuffer(cl_context, cl_mem_flags, size_t, void*, cl_int *errcode_ret)
    {
        if (errcode_ret)
            *errcode_ret = CL_DEVICE_NOT_FOUND;
        return 0;
    }

    static cl_mem clCreateSubBuffer(cl_mem, cl_mem_flags, cl_buffer_create_type, const void*, cl_int *errcode_ret)
    {
        if (errcode_ret)
            *errcode_ret = CL_DEVICE_NOT_FOUND;
        return 0;
    }

    static cl_int clEnqueueWriteBuffer(cl_command_queue, cl_mem, cl_bool, size_t, size_t, const void*, cl_uint, const void*, void*)
    {
        return CL_DEVICE_NOT_FOUND;
    }

    static cl_int clEnqueueReadBuffer(cl_command_queue, cl_mem, cl_bool, size_t, size_t, void*, cl_uint, const void*, void*)
    {
        return CL_DEVICE_NOT_FOUND;
    }

    static cl_int clEnqueueNDRangeKernel(cl_command_queue, cl_kernel, cl_uint, const size_t*, const size_t*, const size_t*, cl_uint, const void*, void*)
    {
        return CL_DEVICE_NOT_FOUND;
    }

    static cl_int clGetKernelWorkGroupInfo(cl_kernel, cl_device_id, cl_uint, size_t, void *param_value, size_t *param_value_size_ret)
    {
        if (param_value_size_ret)
            *param_value_size_ret = 0;
        if (param_value)
            *(size_t*)param_value = 1;
        return CL_DEVICE_NOT_FOUND;
    }

    static cl_int clGetProgramInfo(cl_program, cl_uint, size_t, void*, size_t *param_value_size_ret)
    {
        if (param_value_size_ret)
            *param_value_size_ret = 0;
        return CL_DEVICE_NOT_FOUND;
    }

    static cl_int clGetProgramBuildInfo(cl_program, cl_device_id, cl_uint, size_t, void *param_value, size_t *param_value_size_ret)
    {
        if (param_value_size_ret)
            *param_value_size_ret = 0;
        if (param_value)
            ((char*)param_value)[0] = 0;
        return CL_DEVICE_NOT_FOUND;
    }

    static cl_int clSetKernelArg(cl_kernel, cl_uint, size_t, const void*)
    {
        return CL_DEVICE_NOT_FOUND;
    }

    static cl_int clReleaseProgram(cl_program)
    {
        return CL_SUCCESS;
    }

    static cl_int clReleaseKernel(cl_kernel)
    {
        return CL_SUCCESS;
    }

    static cl_int clReleaseMemObject(cl_mem)
    {
        return CL_SUCCESS;
    }

    static cl_int clReleaseCommandQueue(cl_command_queue)
    {
        return CL_SUCCESS;
    }

    static cl_int clReleaseContext(cl_context)
    {
        return CL_SUCCESS;
    }

    static cl_int clFinish(cl_command_queue)
    {
        return CL_DEVICE_NOT_FOUND;
    }

    static cl_int clFlush(cl_command_queue)
    {
        return CL_DEVICE_NOT_FOUND;
    }
}

#endif
