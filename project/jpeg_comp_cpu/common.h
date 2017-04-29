#ifndef COMMON_H
#define COMMON_H

// Min Max
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

///////////////////////////////////////////////////////////////////////////////
// TIMERS
///////////////////////////////////////////////////////////////////////////////
// OS-DEPENDENT SUPPORT
#if (defined(_WIN32) || defined(_WIN64))
#include <tchar.h>
#include <windows.h>

#define TIME_TYPE LARGE_INTEGER

#define OS_TIMER_START(X)  QueryPerformanceCounter(&X)
#define OS_TIMER_STOP(X)   QueryPerformanceCounter(&X)
#define OS_TIMER_CALC(A,B,V)                                   \
{                                                              \
   LARGE_INTEGER frequency;                                    \
   QueryPerformanceFrequency(&frequency);                      \
   V = (1.0*(B.QuadPart - A.QuadPart) / frequency.QuadPart);   \
}

#else

#define TIME_TYPE struct timespec

// call this function to start a nanosecond-resolution timer
struct timespec get_time();

// call this function to end a timer, returning microseconds
double timer_calc(struct timespec start_time, struct timespec end_time);

#define OS_TIMER_START(X)     X = get_time() 
#define OS_TIMER_STOP(X)      X = get_time() 
#define OS_TIMER_CALC(A,B,V)  V = timer_calc(A,B)
#endif

#endif /* COMMON_H */
