//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#ifdef __MACH__
#include <stdint.h>
#include <OpenCL/OpenCL.h>
#else
#include <windows.h>
#include <algorithm>

#include <tchar.h>
typedef unsigned __int64 uint64_t;
typedef __int16 int16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned char uint8_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;


#endif

#include <stdio.h>

#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")

#ifdef _OPENMP
#include <omp.h>
#endif

// We use the standard library version of these functions
#ifndef __MACH__
#undef min
#undef max
#define strcasecmp stricmp
#endif

