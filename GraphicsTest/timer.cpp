//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#include "StdAfx.h"
#include "timer.h"


// Construction
CTimer::CTimer()
{
#if defined(__APPLE__)
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &m_cclock);    
#endif
}

// Destruction
CTimer::~CTimer()
{
#if defined(__APPLE__)
    mach_port_deallocate(mach_task_self(), m_cclock);
#endif
}


void CTimer::start()
{
#if defined(__APPLE__)
    clock_get_time(m_cclock, &m_mts_start);
#elif defined(_WIN32)
    ::QueryPerformanceCounter( &m_start );
#else
    clock_gettime(CLOCK_MONOTONIC, &m_start);
#endif
}

uint64_t CTimer::elapsed()
{
	#if defined(__APPLE__)
	mach_timespec_t mts_end;
    clock_get_time(m_cclock, &mts_end);
    
    // Convert from seconds to nano seconds
    uint64_t r = (mts_end.tv_sec - m_mts_start.tv_sec);
    r = r * 1000000000l;
    r += mts_end.tv_nsec - m_mts_start.tv_nsec;
    
    return r / 1000;
    #elif defined(_WIN32)
	LARGE_INTEGER end,f;
	QueryPerformanceCounter( &end );
	__int64 d = end.QuadPart - m_start.QuadPart;
	QueryPerformanceFrequency( &f );
	d = (d * 1000000) / f.QuadPart;

	return d;
    #else
        timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);
        uint64_t seconds = static_cast<uint64_t>(end.tv_sec - m_start.tv_sec);
        int64_t nanoseconds = static_cast<int64_t>(end.tv_nsec) - static_cast<int64_t>(m_start.tv_nsec);
        return seconds * 1000000ULL + static_cast<uint64_t>(nanoseconds / 1000);
#endif
}