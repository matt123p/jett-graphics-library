//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#ifndef GenericOrdering_timer_h
#define GenericOrdering_timer_h

#if defined(__APPLE__)
#include <mach/clock.h>
#include <mach/mach.h>
#elif !defined(_WIN32)
#include <time.h>
#endif


class CTimer
{
private:

#if defined(__APPLE__)
    uint64_t        m_start;
    clock_serv_t    m_cclock;
    mach_timespec_t m_mts_start;
#elif defined(_WIN32)
    LARGE_INTEGER   m_start;
#else
    timespec        m_start;
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
