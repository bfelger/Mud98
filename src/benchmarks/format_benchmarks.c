////////////////////////////////////////////////////////////////////////////////
// benchmarks/format_benchmarks.c
////////////////////////////////////////////////////////////////////////////////

#include "benchmarks.h"

#include <olc/string_edit.h>

#include <color.h>
#include <db.h>
#include <format.h>
#include <match.h>

char* _OLD_format_string(char* oldstring);

#define ITERATIONS 100000

static const char* lorum_ipsum = "At vero eos et accusamus et iusto odio dignissimos "
"ducimus qui blanditiis praesentium voluptatum deleniti atque corrupti "
"quos dolores et quas molestias excepturi sint occaecati cupiditate non "
"provident, similique sunt in culpa qui officia deserunt mollitia animi, "
"id est laborum et dolorum fuga.\n\nEt harum quidem rerum facilis est et "
"expedita distinctio.Nam libero tempore, cum soluta nobis est eligendi "
"optio cumque nihil impedit quo minus id quod maxime placeat facere "
"possimus, omnis voluptas assumenda est, omnis dolor repellendus.\n\n"
"Temporibus autem quibusdam et aut officiis debitis aut rerum "
"necessitatibus saepe eveniet ut et voluptates repudiandae sint et "
"molestiae non recusandae.\n\nItaque earum rerum hic tenetur a sapiente "
"delectus, ut aut reiciendis voluptatibus maiores alias consequatur aut "
"perferendis doloribus asperiores repellat.";

long alloc_time;

static void benchmark_string_allocs()
{
    Timer timer = { 0 };
    start_timer(&timer);

    for (int i = 0; i < ITERATIONS; i++) {
        char* s = str_dup(lorum_ipsum);
        free_string(s);
    }

    stop_timer(&timer);
    struct timespec timer_res = elapsed(&timer);
    alloc_time = timer_res.tv_nsec;
}

static void benchmark_old_string_format()
{
    Timer timer = { 0 };
    start_timer(&timer);

    for (int i = 0; i < ITERATIONS; i++) {
        char* s = str_dup(lorum_ipsum);
        s = _OLD_format_string(s);
        free_string(s);
    }

    stop_timer(&timer);
    struct timespec timer_res = elapsed(&timer);
    printf(": %12ldns\n", timer_res.tv_nsec);
}

static void benchmark_new_string_format()
{
    Timer timer = { 0 };
    start_timer(&timer);

    for (int i = 0; i < ITERATIONS; i++) {
        char* s = format_string2(lorum_ipsum);
        free_string(s);
    }

    stop_timer(&timer);
    struct timespec timer_res = elapsed(&timer);
    printf(": %12ldns\n", timer_res.tv_nsec);
}

void benchmark_formatting()
{
    benchmark_string_allocs();
    // Do it again to get the cache warm-up:
    benchmark_string_allocs();

    printf("string_format() Benchmarks:\n");
    printf("         Old ");
    benchmark_old_string_format();
    printf("         New ");
    benchmark_new_string_format();
#ifdef BENCHMARK_FMT_PROCS
    printf("        Norm : %12ldns\n", norm_elapsed);
    printf("        Wrap : %12ldns\n", wrap_elapsed);
#endif
}