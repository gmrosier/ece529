#include "common.h"
#include <time.h>

#if !(defined(_WIN32) || defined(_WIN64))

struct timespec get_time()
{
   struct timespec start_time;
   clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
   return start_time;
}

// call this function to end a timer, returning microseconds
double timer_calc(struct timespec start_time, struct timespec end_time)
{
   long diffInSec = end_time.tv_sec - start_time.tv_sec;
   long diffInNanos = end_time.tv_nsec - start_time.tv_nsec;
   double deltaNanos = 1.0e9*diffInSec + 1.0*diffInNanos;
   return deltaNanos*1.0e-9; // convert to seconds
}

#endif
