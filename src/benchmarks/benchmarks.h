////////////////////////////////////////////////////////////////////////////////
// benchmarks/benchmarks.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__TESTS__BENCHMARKS_H
#define MUD98__TESTS__BENCHMARKS_H

#include <stdbool.h>
#include <time.h>

typedef struct {
    struct timespec start;
    struct timespec stop;
    bool running;
} Timer;

struct timespec elapsed(Timer* timer);
void reset_timer(Timer* timer);
void start_timer(Timer* timer);
void stop_timer(Timer* timer);

void benchmark_containers();
void benchmark_formatting();

void run_benchmarks();

#endif // !MUD98__TESTS__BENCHMARKS_H
