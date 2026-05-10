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
#ifdef __MACH__
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &m_cclock);    
#endif
}

// Destruction
CTimer::~CTimer()
{
#ifdef __MACH__
    mach_port_deallocate(mach_task_self(), m_cclock);
#endif
}


void CTimer::start()
{
#ifdef __MACH__
    clock_get_time(m_cclock, &m_mts_start);
#else
	::QueryPerformanceCounter( &m_start );
#endif
}

uint64_t CTimer::elapsed()
{
 #ifdef __MACH__
	mach_timespec_t mts_end;
    clock_get_time(m_cclock, &mts_end);
    
    // Convert from seconds to nano seconds
    uint64_t r = (mts_end.tv_sec - m_mts_start.tv_sec);
    r = r * 1000000000l;
    r += mts_end.tv_nsec - m_mts_start.tv_nsec;
    
    return r / 1000;
#else
	LARGE_INTEGER end,f;
	QueryPerformanceCounter( &end );
	__int64 d = end.QuadPart - m_start.QuadPart;
	QueryPerformanceFrequency( &f );
	d = (d * 1000000) / f.QuadPart;

	return d;
#endif
}