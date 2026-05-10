//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#ifndef GenericOrdering_timer_h
#define GenericOrdering_timer_h

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif


class CTimer
{
private:

#ifdef __MACH__
    uint64_t        m_start;
    clock_serv_t    m_cclock;
    mach_timespec_t m_mts_start;
#else
	LARGE_INTEGER	m_start;
#endif

public:
    
    // Construction
    CTimer();

    // Destruction
    ~CTimer();

    // Start the timer
    void start();
    
    // Return elapsed time in micro seconds
    uint64_t elapsed();
};

#endif
