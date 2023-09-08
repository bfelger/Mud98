////////////////////////////////////////////////////////////////////////////////
// benchmark.c
//
// Utilities for gathering metrics to benchmark code changes
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "benchmark.h"

#ifdef _MSC_VER
#include <windows.h>
#include <winsock.h>
#include <winnt.h>
#define CLOCK_MONOTONIC		        1
#define CLOCK_PROCESS_CPUTIME_ID    2

#define MS_PER_SEC      1000ULL     // MS = milliseconds
#define US_PER_MS       1000ULL     // US = microseconds
#define HNS_PER_US      10ULL       // HNS = hundred-nanoseconds (e.g., 1 hns = 100 ns)
#define NS_PER_US       1000ULL

#define HNS_PER_SEC     (MS_PER_SEC * US_PER_MS * HNS_PER_US)
#define NS_PER_HNS      (100ULL)    // NS = nanoseconds
#define NS_PER_SEC      (MS_PER_SEC * US_PER_MS * NS_PER_US)

////////////////////////////////////////////////////////////////////////////////
// This implementation taken from StackOverflow user jws's example:
//    https://stackoverflow.com/a/51974214
// I only implemented the CLOCK_MONOTONIC version.
////////////////////////////////////////////////////////////////////////////////
static int clock_gettime(int dummy, struct timespec* tv)
{
    static LARGE_INTEGER ticksPerSec;
    LARGE_INTEGER ticks;

    if (!ticksPerSec.QuadPart) {
        QueryPerformanceFrequency(&ticksPerSec);
        if (!ticksPerSec.QuadPart) {
            errno = ENOTSUP;
            return -1;
        }
    }

    QueryPerformanceCounter(&ticks);

    tv->tv_sec = (long)(ticks.QuadPart / ticksPerSec.QuadPart);
    tv->tv_nsec = (long)(((ticks.QuadPart % ticksPerSec.QuadPart) * NS_PER_SEC) / ticksPerSec.QuadPart);

    return 0;
}
////////////////////////////////////////////////////////////////////////////////
#endif

struct timespec elapsed(Timer* timer)
{
    struct timespec temp = { 0 };

    if (!timer->running)
        return temp;

    if ((timer->stop.tv_nsec - timer->start.tv_nsec) < 0) {
        temp.tv_sec = timer->stop.tv_sec - timer->start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + timer->stop.tv_nsec - timer->start.tv_nsec;
    }
    else {
        temp.tv_sec = timer->stop.tv_sec - timer->start.tv_sec;
        temp.tv_nsec = timer->stop.tv_nsec - timer->start.tv_nsec;
    }
    return temp;
}

void start_timer(Timer* timer)
{
    if (!timer->running) {
        clock_gettime(CLOCK_MONOTONIC, &(timer->start));
        timer->running = true;
    }
}

void stop_timer(Timer* timer)
{
    if (timer->running) {
        clock_gettime(CLOCK_MONOTONIC, &(timer->stop));
        timer->running = false;
    }
}
