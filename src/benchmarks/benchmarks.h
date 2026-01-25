////////////////////////////////////////////////////////////////////////////////
// benchmarks/benchmarks.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__TESTS__BENCHMARKS_H
#define MUD98__TESTS__BENCHMARKS_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

typedef struct {
    struct timespec start;
    struct timespec stop;
    bool running;
} Timer;

typedef void (*BenchmarkFn)(void);

typedef struct {
    const char* name;
    BenchmarkFn fn;
} BenchmarkEntry;

struct timespec elapsed(Timer* timer);
void reset_timer(Timer* timer);
void start_timer(Timer* timer);
void stop_timer(Timer* timer);

void benchmark_containers(void);
void benchmark_formatting(void);

const BenchmarkEntry* benchmark_registry(size_t* count);
bool run_benchmark_by_name(const char* name);
void run_benchmarks(void);

#endif // !MUD98__TESTS__BENCHMARKS_H
