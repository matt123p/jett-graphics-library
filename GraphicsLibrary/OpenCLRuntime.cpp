//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"

#ifndef __MACH__

#undef clGetPlatformIDs
#undef clGetDeviceIDs
#undef clGetDeviceInfo
#undef clCreateContext
#undef clCreateCommandQueue
#undef clCreateProgramWithSource
#undef clCreateProgramWithBinary
#undef clBuildProgram
#undef clCreateKernel
#undef clCreateBuffer
#undef clCreateSubBuffer
#undef clEnqueueWriteBuffer
#undef clEnqueueReadBuffer
#undef clEnqueueNDRangeKernel
#undef clGetKernelWorkGroupInfo
#undef clGetProgramInfo
#undef clGetProgramBuildInfo
#undef clSetKernelArg
#undef clReleaseProgram
#undef clReleaseKernel
#undef clReleaseMemObject
#undef clReleaseCommandQueue
#undef clReleaseContext
#undef clFinish

#include "OpenCLRuntime.h"
#include "jett.h"

#include <sstream>

namespace
{
    typedef cl_int (CL_API_CALL *clGetPlatformIDs_fn)(cl_uint, cl_platform_id*, cl_uint*);
    typedef cl_int (CL_API_CALL *clGetDeviceIDs_fn)(cl_platform_id, cl_device_type, cl_uint, cl_device_id*, cl_uint*);
    typedef cl_int (CL_API_CALL *clGetDeviceInfo_fn)(cl_device_id, cl_device_info, size_t, void*, size_t*);
    typedef cl_context (CL_API_CALL *clCreateContext_fn)(const cl_context_properties*, cl_uint, const cl_device_id*, void (CL_CALLBACK *)(const char*, const void*, size_t, void*), void*, cl_int*);
    typedef cl_command_queue (CL_API_CALL *clCreateCommandQueue_fn)(cl_context, cl_device_id, cl_command_queue_properties, cl_int*);
    typedef cl_program (CL_API_CALL *clCreateProgramWithSource_fn)(cl_context, cl_uint, const char**, const size_t*, cl_int*);
    typedef cl_program (CL_API_CALL *clCreateProgramWithBinary_fn)(cl_context, cl_uint, const cl_device_id*, const size_t*, const unsigned char**, cl_int*, cl_int*);
    typedef cl_int (CL_API_CALL *clBuildProgram_fn)(cl_program, cl_uint, const cl_device_id*, const char*, void (CL_CALLBACK *)(cl_program, void*), void*);
    typedef cl_kernel (CL_API_CALL *clCreateKernel_fn)(cl_program, const char*, cl_int*);
    typedef cl_mem (CL_API_CALL *clCreateBuffer_fn)(cl_context, cl_mem_flags, size_t, void*, cl_int*);
    typedef cl_mem (CL_API_CALL *clCreateSubBuffer_fn)(cl_mem, cl_mem_flags, cl_buffer_create_type, const void*, cl_int*);
    typedef cl_int (CL_API_CALL *clEnqueueWriteBuffer_fn)(cl_command_queue, cl_mem, cl_bool, size_t, size_t, const void*, cl_uint, const cl_event*, cl_event*);
    typedef cl_int (CL_API_CALL *clEnqueueReadBuffer_fn)(cl_command_queue, cl_mem, cl_bool, size_t, size_t, void*, cl_uint, const cl_event*, cl_event*);
    typedef cl_int (CL_API_CALL *clEnqueueNDRangeKernel_fn)(cl_command_queue, cl_kernel, cl_uint, const size_t*, const size_t*, const size_t*, cl_uint, const cl_event*, cl_event*);
    typedef cl_int (CL_API_CALL *clGetKernelWorkGroupInfo_fn)(cl_kernel, cl_device_id, cl_kernel_work_group_info, size_t, void*, size_t*);
    typedef cl_int (CL_API_CALL *clGetProgramInfo_fn)(cl_program, cl_program_info, size_t, void*, size_t*);
    typedef cl_int (CL_API_CALL *clGetProgramBuildInfo_fn)(cl_program, cl_device_id, cl_program_build_info, size_t, void*, size_t*);
    typedef cl_int (CL_API_CALL *clSetKernelArg_fn)(cl_kernel, cl_uint, size_t, const void*);
    typedef cl_int (CL_API_CALL *clReleaseProgram_fn)(cl_program);
    typedef cl_int (CL_API_CALL *clReleaseKernel_fn)(cl_kernel);
    typedef cl_int (CL_API_CALL *clReleaseMemObject_fn)(cl_mem);
    typedef cl_int (CL_API_CALL *clReleaseCommandQueue_fn)(cl_command_queue);
    typedef cl_int (CL_API_CALL *clReleaseContext_fn)(cl_context);
    typedef cl_int (CL_API_CALL *clFinish_fn)(cl_command_queue);

    struct OpenCLFunctionTable
    {
        clGetPlatformIDs_fn pGetPlatformIDs;
        clGetDeviceIDs_fn pGetDeviceIDs;
        clGetDeviceInfo_fn pGetDeviceInfo;
        clCreateContext_fn pCreateContext;
        clCreateCommandQueue_fn pCreateCommandQueue;
        clCreateProgramWithSource_fn pCreateProgramWithSource;
        clCreateProgramWithBinary_fn pCreateProgramWithBinary;
        clBuildProgram_fn pBuildProgram;
        clCreateKernel_fn pCreateKernel;
        clCreateBuffer_fn pCreateBuffer;
        clCreateSubBuffer_fn pCreateSubBuffer;
        clEnqueueWriteBuffer_fn pEnqueueWriteBuffer;
        clEnqueueReadBuffer_fn pEnqueueReadBuffer;
        clEnqueueNDRangeKernel_fn pEnqueueNDRangeKernel;
        clGetKernelWorkGroupInfo_fn pGetKernelWorkGroupInfo;
        clGetProgramInfo_fn pGetProgramInfo;
        clGetProgramBuildInfo_fn pGetProgramBuildInfo;
        clSetKernelArg_fn pSetKernelArg;
        clReleaseProgram_fn pReleaseProgram;
        clReleaseKernel_fn pReleaseKernel;
        clReleaseMemObject_fn pReleaseMemObject;
        clReleaseCommandQueue_fn pReleaseCommandQueue;
        clReleaseContext_fn pReleaseContext;
        clFinish_fn pFinish;
    };

    OpenCLFunctionTable g_opencl = {};
    HMODULE g_opencl_module = NULL;
    bool g_opencl_load_attempted = false;
    std::string g_opencl_load_error;

    std::string format_win32_error(DWORD error_code)
    {
        if (error_code == 0)
        {
            return std::string();
        }

        LPSTR message_buffer = NULL;
        DWORD size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error_code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPSTR>(&message_buffer),
            0,
            NULL);

        std::string message;
        if (size && message_buffer)
        {
            message.assign(message_buffer, size);
            while (!message.empty() && (message[message.size() - 1] == '\r' || message[message.size() - 1] == '\n'))
            {
                message.erase(message.size() - 1);
            }
            LocalFree(message_buffer);
        }

        std::ostringstream text;
        text << "Win32 error " << error_code;
        if (!message.empty())
        {
            text << ": " << message;
        }

        return text.str();
    }

    template<typename T>
    bool load_symbol(T& target, const char* name)
    {
        target = reinterpret_cast<T>(GetProcAddress(g_opencl_module, name));
        if (!target)
        {
            std::ostringstream text;
            text << "OpenCL.dll is missing required entry point '" << name << "'";
            g_opencl_load_error = text.str();
            return false;
        }

        return true;
    }

    bool load_opencl_runtime()
    {
        if (g_opencl_load_attempted)
        {
            return g_opencl_module != NULL;
        }

        g_opencl_load_attempted = true;
        g_opencl_module = LoadLibraryW(L"OpenCL.dll");
        if (!g_opencl_module)
        {
            g_opencl_load_error = "Failed to load OpenCL.dll: " + format_win32_error(GetLastError());
            return false;
        }

        if (!load_symbol(g_opencl.pGetPlatformIDs, "clGetPlatformIDs")
            || !load_symbol(g_opencl.pGetDeviceIDs, "clGetDeviceIDs")
            || !load_symbol(g_opencl.pGetDeviceInfo, "clGetDeviceInfo")
            || !load_symbol(g_opencl.pCreateContext, "clCreateContext")
            || !load_symbol(g_opencl.pCreateCommandQueue, "clCreateCommandQueue")
            || !load_symbol(g_opencl.pCreateProgramWithSource, "clCreateProgramWithSource")
            || !load_symbol(g_opencl.pCreateProgramWithBinary, "clCreateProgramWithBinary")
            || !load_symbol(g_opencl.pBuildProgram, "clBuildProgram")
            || !load_symbol(g_opencl.pCreateKernel, "clCreateKernel")
            || !load_symbol(g_opencl.pCreateBuffer, "clCreateBuffer")
            || !load_symbol(g_opencl.pCreateSubBuffer, "clCreateSubBuffer")
            || !load_symbol(g_opencl.pEnqueueWriteBuffer, "clEnqueueWriteBuffer")
            || !load_symbol(g_opencl.pEnqueueReadBuffer, "clEnqueueReadBuffer")
            || !load_symbol(g_opencl.pEnqueueNDRangeKernel, "clEnqueueNDRangeKernel")
            || !load_symbol(g_opencl.pGetKernelWorkGroupInfo, "clGetKernelWorkGroupInfo")
            || !load_symbol(g_opencl.pGetProgramInfo, "clGetProgramInfo")
            || !load_symbol(g_opencl.pGetProgramBuildInfo, "clGetProgramBuildInfo")
            || !load_symbol(g_opencl.pSetKernelArg, "clSetKernelArg")
            || !load_symbol(g_opencl.pReleaseProgram, "clReleaseProgram")
            || !load_symbol(g_opencl.pReleaseKernel, "clReleaseKernel")
            || !load_symbol(g_opencl.pReleaseMemObject, "clReleaseMemObject")
            || !load_symbol(g_opencl.pReleaseCommandQueue, "clReleaseCommandQueue")
            || !load_symbol(g_opencl.pReleaseContext, "clReleaseContext")
            || !load_symbol(g_opencl.pFinish, "clFinish"))
        {
            FreeLibrary(g_opencl_module);
            g_opencl_module = NULL;
            ZeroMemory(&g_opencl, sizeof(g_opencl));
            return false;
        }

        return true;
    }

    void throw_opencl_load_failure()
    {
        throw jett_exception(JETT_OPENCL_FAILURE, 0, g_opencl_load_error.c_str());
    }

    template<typename T>
    T require_symbol(T symbol)
    {
        if (!load_opencl_runtime())
        {
            throw_opencl_load_failure();
        }

        return symbol;
    }
}

namespace jett_opencl
{
    bool ensure_loaded(std::string* error_message)
    {
        if (load_opencl_runtime())
        {
            return true;
        }

        if (error_message)
        {
            *error_message = g_opencl_load_error;
        }

        return false;
    }

    cl_int clGetPlatformIDs(cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms)
    {
        return require_symbol(g_opencl.pGetPlatformIDs)(num_entries, platforms, num_platforms);
    }

    cl_int clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices)
    {
        return require_symbol(g_opencl.pGetDeviceIDs)(platform, device_type, num_entries, devices, num_devices);
    }

    cl_int clGetDeviceInfo(cl_device_id device, cl_device_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    {
        return require_symbol(g_opencl.pGetDeviceInfo)(device, param_name, param_value_size, param_value, param_value_size_ret);
    }

    cl_context clCreateContext(const cl_context_properties* properties, cl_uint num_devices, const cl_device_id* devices, void (CL_CALLBACK* pfn_notify)(const char*, const void*, size_t, void*), void* user_data, cl_int* errcode_ret)
    {
        return require_symbol(g_opencl.pCreateContext)(properties, num_devices, devices, pfn_notify, user_data, errcode_ret);
    }

    cl_command_queue clCreateCommandQueue(cl_context context, cl_device_id device, cl_command_queue_properties properties, cl_int* errcode_ret)
    {
        return require_symbol(g_opencl.pCreateCommandQueue)(context, device, properties, errcode_ret);
    }

    cl_program clCreateProgramWithSource(cl_context context, cl_uint count, const char** strings, const size_t* lengths, cl_int* errcode_ret)
    {
        return require_symbol(g_opencl.pCreateProgramWithSource)(context, count, strings, lengths, errcode_ret);
    }

    cl_program clCreateProgramWithBinary(cl_context context, cl_uint num_devices, const cl_device_id* device_list, const size_t* lengths, const unsigned char** binaries, cl_int* binary_status, cl_int* errcode_ret)
    {
        return require_symbol(g_opencl.pCreateProgramWithBinary)(context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret);
    }

    cl_int clBuildProgram(cl_program program, cl_uint num_devices, const cl_device_id* device_list, const char* options, void (CL_CALLBACK* pfn_notify)(cl_program, void*), void* user_data)
    {
        return require_symbol(g_opencl.pBuildProgram)(program, num_devices, device_list, options, pfn_notify, user_data);
    }

    cl_kernel clCreateKernel(cl_program program, const char* kernel_name, cl_int* errcode_ret)
    {
        return require_symbol(g_opencl.pCreateKernel)(program, kernel_name, errcode_ret);
    }

    cl_mem clCreateBuffer(cl_context context, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret)
    {
        return require_symbol(g_opencl.pCreateBuffer)(context, flags, size, host_ptr, errcode_ret);
    }

    cl_mem clCreateSubBuffer(cl_mem buffer, cl_mem_flags flags, cl_buffer_create_type buffer_create_type, const void* buffer_create_info, cl_int* errcode_ret)
    {
        return require_symbol(g_opencl.pCreateSubBuffer)(buffer, flags, buffer_create_type, buffer_create_info, errcode_ret);
    }

    cl_int clEnqueueWriteBuffer(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, size_t offset, size_t cb, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event)
    {
        return require_symbol(g_opencl.pEnqueueWriteBuffer)(command_queue, buffer, blocking_write, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event);
    }

    cl_int clEnqueueReadBuffer(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, size_t offset, size_t cb, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event)
    {
        return require_symbol(g_opencl.pEnqueueReadBuffer)(command_queue, buffer, blocking_read, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event);
    }

    cl_int clEnqueueNDRangeKernel(cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event)
    {
        return require_symbol(g_opencl.pEnqueueNDRangeKernel)(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event);
    }

    cl_int clGetKernelWorkGroupInfo(cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    {
        return require_symbol(g_opencl.pGetKernelWorkGroupInfo)(kernel, device, param_name, param_value_size, param_value, param_value_size_ret);
    }

    cl_int clGetProgramInfo(cl_program program, cl_program_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    {
        return require_symbol(g_opencl.pGetProgramInfo)(program, param_name, param_value_size, param_value, param_value_size_ret);
    }

    cl_int clGetProgramBuildInfo(cl_program program, cl_device_id device, cl_program_build_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret)
    {
        return require_symbol(g_opencl.pGetProgramBuildInfo)(program, device, param_name, param_value_size, param_value, param_value_size_ret);
    }

    cl_int clSetKernelArg(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value)
    {
        return require_symbol(g_opencl.pSetKernelArg)(kernel, arg_index, arg_size, arg_value);
    }

    cl_int clReleaseProgram(cl_program program)
    {
        return require_symbol(g_opencl.pReleaseProgram)(program);
    }

    cl_int clReleaseKernel(cl_kernel kernel)
    {
        return require_symbol(g_opencl.pReleaseKernel)(kernel);
    }

    cl_int clReleaseMemObject(cl_mem memobj)
    {
        return require_symbol(g_opencl.pReleaseMemObject)(memobj);
    }

    cl_int clReleaseCommandQueue(cl_command_queue command_queue)
    {
        return require_symbol(g_opencl.pReleaseCommandQueue)(command_queue);
    }

    cl_int clReleaseContext(cl_context context)
    {
        return require_symbol(g_opencl.pReleaseContext)(context);
    }

    cl_int clFinish(cl_command_queue command_queue)
    {
        return require_symbol(g_opencl.pFinish)(command_queue);
    }
}

#endif