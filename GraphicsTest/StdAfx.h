//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsTest_StdAfx_h
#define GraphicsTest_StdAfx_h

#include "../GraphicsLibrary/StdAfx.h"

#ifdef _WIN32
#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")
#else
#include <cstddef>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <type_traits>

#ifndef LPTSTR
#define LPTSTR TCHAR*
#endif

#ifndef LPCTSTR
#define LPCTSTR const TCHAR*
#endif

template <size_t N>
inline int _tcscpy_s(TCHAR (&destination)[N], const TCHAR *source)
{
	if (!source)
	{
		destination[0] = 0;
		return 0;
	}

	std::strncpy(destination, source, N - 1);
	destination[N - 1] = 0;
	return 0;
}

template <size_t N>
inline int _tcscat_s(TCHAR (&destination)[N], const TCHAR *source)
{
	if (!source)
	{
		return 0;
	}

	size_t used = std::strlen(destination);
	if (used >= N)
	{
		destination[N - 1] = 0;
		return 0;
	}

	std::strncpy(destination + used, source, N - used - 1);
	destination[N - 1] = 0;
	return 0;
}

template <size_t N>
inline int _stprintf(TCHAR (&destination)[N], const TCHAR *format, ...)
{
	va_list args;
	va_start(args, format);
	int result = std::vsnprintf(destination, N, format, args);
	va_end(args);
	if (result < 0 || static_cast<size_t>(result) >= N)
	{
		destination[N - 1] = 0;
	}
	return result;
}

#define _tprintf printf
#define _stprintf_s _stprintf
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#endif

