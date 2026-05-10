//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#ifdef __MACH__
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <OpenCL/OpenCL.h>

#define _T(a) (a)
#define _tcslen strlen
#define _tcsrchr strrchr
#define vsprintf_s vsnprintf
#define _snprintf_s snprintf
#define sprintf_s sprintf
int _tfopen_s( FILE** pFile, const char *filename, const char *mode );

#else
#include "targetver.h"
#include <windows.h>
#include <algorithm>

#ifndef CL_TARGET_OPENCL_VERSION
#define CL_TARGET_OPENCL_VERSION 120
#endif

#ifndef CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#endif

#include <tchar.h>
typedef unsigned __int64 uint64_t;
typedef __int16 int16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned char uint8_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
#if defined(__has_include)
#if __has_include(<CL/OpenCL.h>)
#include <CL/OpenCL.h>
#define JETT_HAS_OPENCL 1
#elif __has_include(<CL/opencl.h>)
#include <CL/opencl.h>
#define JETT_HAS_OPENCL 1
#elif __has_include(<CL/cl.h>)
#include <CL/cl.h>
#define JETT_HAS_OPENCL 1
#elif __has_include(<OpenCL/opencl.h>)
#include <OpenCL/opencl.h>
#define JETT_HAS_OPENCL 1
#else
#error OpenCL headers not found. Install an OpenCL SDK or set CUDA_PATH so the compiler can find CL/cl.h.
#endif
#else
#include <CL/cl.h>
#define JETT_HAS_OPENCL 1
#endif

#include "OpenCLRuntime.h"

#define DLLEXPORT __declspec(dllexport)

#endif

#include <stdio.h>

// Do we have openmp support?
#ifdef _OPENMP
#include <omp.h>

// The number of lines in a job before we switch to openmp parallel operations
#define     OPENMP_CUTOFF   50
#endif

// We use the standard library version of these functions
#ifndef __MACH__
#undef min
#undef max
#define strcasecmp _tcsicmp
#endif

#include <string>
#include <map>
#include <vector>
