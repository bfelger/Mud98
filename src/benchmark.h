////////////////////////////////////////////////////////////////////////////////
// benchmark.h
//
// Utilities for gathering metrics to benchmark code changes
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef ROM__BENCHMARK_H
#define ROM__BENCHMARK_H

#include <stdbool.h>
#include <time.h>

typedef struct {
    struct timespec start;
    struct timespec stop;
    bool running;
} Timer;

struct timespec elapsed(Timer* timer);
void start_timer(Timer* timer);
void stop_timer(Timer* timer);

#endif // !ROM__BENCHMARK_H
