//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_OpenCLRuntime_h
#define GraphicsLibrary_OpenCLRuntime_h

#ifndef __MACH__

#include <string>

namespace jett_opencl
{
    bool ensure_loaded(std::string* error_message = NULL);

    cl_int clGetPlatformIDs(cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms);
    cl_int clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices);
    cl_int clGetDeviceInfo(cl_device_id device, cl_device_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);
    cl_context clCreateContext(const cl_context_properties* properties, cl_uint num_devices, const cl_device_id* devices, void (CL_CALLBACK* pfn_notify)(const char*, const void*, size_t, void*), void* user_data, cl_int* errcode_ret);
    cl_command_queue clCreateCommandQueue(cl_context context, cl_device_id device, cl_command_queue_properties properties, cl_int* errcode_ret);
    cl_program clCreateProgramWithSource(cl_context context, cl_uint count, const char** strings, const size_t* lengths, cl_int* errcode_ret);
    cl_program clCreateProgramWithBinary(cl_context context, cl_uint num_devices, const cl_device_id* device_list, const size_t* lengths, const unsigned char** binaries, cl_int* binary_status, cl_int* errcode_ret);
    cl_int clBuildProgram(cl_program program, cl_uint num_devices, const cl_device_id* device_list, const char* options, void (CL_CALLBACK* pfn_notify)(cl_program, void*), void* user_data);
    cl_kernel clCreateKernel(cl_program program, const char* kernel_name, cl_int* errcode_ret);
    cl_mem clCreateBuffer(cl_context context, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret);
    cl_mem clCreateSubBuffer(cl_mem buffer, cl_mem_flags flags, cl_buffer_create_type buffer_create_type, const void* buffer_create_info, cl_int* errcode_ret);
    cl_int clEnqueueWriteBuffer(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, size_t offset, size_t cb, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);
    cl_int clEnqueueReadBuffer(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, size_t offset, size_t cb, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);
    cl_int clEnqueueNDRangeKernel(cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);
    cl_int clGetKernelWorkGroupInfo(cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);
    cl_int clGetProgramInfo(cl_program program, cl_program_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);
    cl_int clGetProgramBuildInfo(cl_program program, cl_device_id device, cl_program_build_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);
    cl_int clSetKernelArg(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value);
    cl_int clReleaseProgram(cl_program program);
    cl_int clReleaseKernel(cl_kernel kernel);
    cl_int clReleaseMemObject(cl_mem memobj);
    cl_int clReleaseCommandQueue(cl_command_queue command_queue);
    cl_int clReleaseContext(cl_context context);
    cl_int clFinish(cl_command_queue command_queue);
}

#ifndef jett_OPENCL_RUNTIME_NO_ALIASES
#define clGetPlatformIDs ::jett_opencl::clGetPlatformIDs
#define clGetDeviceIDs ::jett_opencl::clGetDeviceIDs
#define clGetDeviceInfo ::jett_opencl::clGetDeviceInfo
#define clCreateContext ::jett_opencl::clCreateContext
#define clCreateCommandQueue ::jett_opencl::clCreateCommandQueue
#define clCreateProgramWithSource ::jett_opencl::clCreateProgramWithSource
#define clCreateProgramWithBinary ::jett_opencl::clCreateProgramWithBinary
#define clBuildProgram ::jett_opencl::clBuildProgram
#define clCreateKernel ::jett_opencl::clCreateKernel
#define clCreateBuffer ::jett_opencl::clCreateBuffer
#define clCreateSubBuffer ::jett_opencl::clCreateSubBuffer
#define clEnqueueWriteBuffer ::jett_opencl::clEnqueueWriteBuffer
#define clEnqueueReadBuffer ::jett_opencl::clEnqueueReadBuffer
#define clEnqueueNDRangeKernel ::jett_opencl::clEnqueueNDRangeKernel
#define clGetKernelWorkGroupInfo ::jett_opencl::clGetKernelWorkGroupInfo
#define clGetProgramInfo ::jett_opencl::clGetProgramInfo
#define clGetProgramBuildInfo ::jett_opencl::clGetProgramBuildInfo
#define clSetKernelArg ::jett_opencl::clSetKernelArg
#define clReleaseProgram ::jett_opencl::clReleaseProgram
#define clReleaseKernel ::jett_opencl::clReleaseKernel
#define clReleaseMemObject ::jett_opencl::clReleaseMemObject
#define clReleaseCommandQueue ::jett_opencl::clReleaseCommandQueue
#define clReleaseContext ::jett_opencl::clReleaseContext
#define clFinish ::jett_opencl::clFinish
#endif

#endif

#endif