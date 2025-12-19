////////////////////////////////////////////////////////////////////////////////
// benchmarks/benchmarks.c
////////////////////////////////////////////////////////////////////////////////

#include "benchmarks.h"

#include <merc.h>

#include <string.h>
#include <time.h>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <winnt.h>
#define CLOCK_MONOTONIC		        1
#define CLOCK_PROCESS_CPUTIME_ID    2
#endif

#define MS_PER_SEC      1000ULL     // MS = milliseconds
#define US_PER_MS       1000ULL     // US = microseconds
#define HNS_PER_US      10ULL       // HNS = hundred-nanoseconds (e.g., 1 hns = 100 ns)
#define NS_PER_US       1000ULL

#define HNS_PER_SEC     (MS_PER_SEC * US_PER_MS * HNS_PER_US)
#define NS_PER_HNS      (100ULL)    // NS = nanoseconds
#define NS_PER_SEC      (MS_PER_SEC * US_PER_MS * NS_PER_US)

#ifdef _MSC_VER
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

    if (timer->running)
        return temp;

    temp.tv_sec = timer->stop.tv_sec - timer->start.tv_sec;
    temp.tv_nsec = timer->stop.tv_nsec - timer->start.tv_nsec;

    if (temp.tv_nsec < 0) {
        temp.tv_sec -= 1;
        temp.tv_nsec += NS_PER_SEC;
    }

    return temp;
}

void reset_timer(Timer* timer)
{
    memset(timer, 0, sizeof(Timer));
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

static const BenchmarkEntry benchmark_entries[] = {
    { "containers", benchmark_containers },
    { "formatting", benchmark_formatting },
};

const BenchmarkEntry* benchmark_registry(size_t* count)
{
    if (count) {
        *count = sizeof(benchmark_entries) / sizeof(benchmark_entries[0]);
    }
    return benchmark_entries;
}

bool run_benchmark_by_name(const char* name)
{
    size_t count = 0;
    const BenchmarkEntry* entries = benchmark_registry(&count);

    for (size_t i = 0; i < count; i++) {
        if (!strcmp(entries[i].name, name)) {
            entries[i].fn();
            return true;
        }
    }

    return false;
}

void run_benchmarks()
{
    size_t count = 0;
    const BenchmarkEntry* entries = benchmark_registry(&count);

    for (size_t i = 0; i < count; i++) {
        entries[i].fn();
    }
}
