/***************************************************************************
 *  Mud98 Benchmark Runner                                                 *
 *                                                                         *
 *  Standalone benchmark executable that measures performance of          *
 *  critical game systems without requiring the full server.              *
 ***************************************************************************/

#include "merc.h"
#include "config.h"
#include "db.h"
#include "fileutils.h"
#include "stringutils.h"

#include <benchmarks/benchmarks.h>

#include <ctype.h>

#ifdef _MSC_VER
#include <stdint.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#else
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#endif

// Global variables (defined in globals.c)
extern bool merc_down;
extern bool wizlock;
extern bool newlock;
extern char str_boot_time[MAX_INPUT_LENGTH];
extern time_t boot_time;
extern time_t current_time;
extern bool events_enabled;

#ifdef _MSC_VER
struct timezone {
    int dummy;
};

int gettimeofday(struct timeval* tp, struct timezone* unused);
#endif

#define MAX_SELECTED_BENCHMARKS 32
#define MAX_BENCHMARK_LISTS 32

#ifdef _MSC_VER
static char* dup_arg(const char* value)
{
    return _strdup(value);
}
#else
static char* dup_arg(const char* value)
{
    return strdup(value);
}
#endif

static char* trim_space(char* value)
{
    char* start = value;
    char* end = NULL;

    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';

    return start;
}

static void print_available_benchmarks(void)
{
    size_t count = 0;
    const BenchmarkEntry* entries = benchmark_registry(&count);

    fprintf(stderr, "Available benchmarks:");
    for (size_t i = 0; i < count; i++) {
        fprintf(stderr, " %s", entries[i].name);
    }
    fprintf(stderr, "\n");
}

static void print_usage(const char* argv0)
{
    fprintf(stderr, "Usage: %s [-d|--dir=<rundir>] [-a|--area-dir=<areadir>] [-b|--bench=<list>]\n", argv0);
    fprintf(stderr, "       %s [--bench <list>]\n", argv0);
    fprintf(stderr, "  <list> is a comma-separated list of benchmark names.\n");
    print_available_benchmarks();
}

static bool benchmark_exists(const char* name)
{
    size_t count = 0;
    const BenchmarkEntry* entries = benchmark_registry(&count);

    for (size_t i = 0; i < count; i++) {
        if (!strcmp(entries[i].name, name)) {
            return true;
        }
    }

    return false;
}

static void add_benchmark_list(const char* list,
                               char** selected,
                               size_t* selected_count,
                               char** buffers,
                               size_t* buffer_count)
{
    if (*buffer_count >= MAX_BENCHMARK_LISTS) {
        fprintf(stderr, "Too many --bench arguments.\n");
        exit(1);
    }

    char* copy = dup_arg(list);
    if (!copy) {
        fprintf(stderr, "Out of memory while parsing benchmark list.\n");
        exit(1);
    }
    buffers[(*buffer_count)++] = copy;

    char* token = strtok(copy, ",");
    while (token) {
        char* trimmed = trim_space(token);
        if (*trimmed != '\0') {
            if (*selected_count >= MAX_SELECTED_BENCHMARKS) {
                fprintf(stderr, "Too many benchmark selections (max %d).\n", MAX_SELECTED_BENCHMARKS);
                exit(1);
            }
            selected[(*selected_count)++] = trimmed;
        }
        token = strtok(NULL, ",");
    }
}

int main(int argc, char** argv)
{
    struct timeval now_time = { 0 };
    char run_dir[256] = { 0 };
    char area_dir[256] = { 0 };
    char* selected_benchmarks[MAX_SELECTED_BENCHMARKS] = { 0 };
    char* benchmark_buffers[MAX_BENCHMARK_LISTS] = { 0 };
    size_t selected_count = 0;
    size_t buffer_count = 0;

    // Parse command line arguments
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (!strncmp(argv[i], "--dir=", 6)) {
                strcpy(run_dir, argv[i] + 6);
            }
            else if (!strncmp(argv[i], "--rundir=", 9)) {
                strcpy(run_dir, argv[i] + 9);
            }
            else if (!strcmp(argv[i], "-d")) {
                if (++i < argc) {
                    strcpy(run_dir, argv[i]);
                }
            }
            else if (!strncmp(argv[i], "--area-dir=", 11)) {
                strcpy(area_dir, argv[i] + 11);
            }
            else if (!strcmp(argv[i], "-a")) {
                if (++i < argc) {
                    strcpy(area_dir, argv[i]);
                }
            }
            else if (!strncmp(argv[i], "--bench=", 8)) {
                if (argv[i][8] == '\0') {
                    fprintf(stderr, "Empty benchmark list provided.\n");
                    print_usage(argv[0]);
                    exit(1);
                }
                add_benchmark_list(argv[i] + 8, selected_benchmarks, &selected_count, benchmark_buffers, &buffer_count);
            }
            else if (!strcmp(argv[i], "--bench") || !strcmp(argv[i], "-b")) {
                if (++i < argc && argv[i][0] != '\0') {
                    add_benchmark_list(argv[i], selected_benchmarks, &selected_count, benchmark_buffers, &buffer_count);
                }
                else {
                    fprintf(stderr, "Missing benchmark list after %s.\n", argv[i - 1]);
                    print_usage(argv[0]);
                    exit(1);
                }
            }
            else {
                fprintf(stderr, "Unknown argument '%s'.\n", argv[i]);
                print_usage(argv[0]);
                exit(1);
            }
        }
    }

    if (selected_count > 0) {
        size_t missing = 0;
        for (size_t i = 0; i < selected_count; i++) {
            if (!benchmark_exists(selected_benchmarks[i])) {
                fprintf(stderr, "Unknown benchmark '%s'.\n", selected_benchmarks[i]);
                missing++;
            }
        }
        if (missing > 0) {
            print_available_benchmarks();
            for (size_t i = 0; i < buffer_count; i++) {
                free(benchmark_buffers[i]);
            }
            return 1;
        }
    }

    if (area_dir[0]) {
        cfg_set_area_dir(area_dir);
    }

    if (run_dir[0]) {
        cfg_set_base_dir(run_dir);
    }

    // Initialize time
    gettimeofday(&now_time, NULL);
    boot_time = (time_t)now_time.tv_sec;
    current_time = boot_time;
    strcpy(str_boot_time, ctime(&boot_time));

    open_reserve_file();

    // Boot database with timing
    Timer boot_timer = { 0 };
    start_timer(&boot_timer);
    boot_db();
    stop_timer(&boot_timer);
    
    struct timespec timer_res = elapsed(&boot_timer);
    sprintf(log_buf, "Boot time: "TIME_FMT"s, %ldns.", timer_res.tv_sec, timer_res.tv_nsec);
    log_string(log_buf);

    print_memory();

    // Run benchmarks (optionally filtered)
    if (selected_count > 0) {
        for (size_t i = 0; i < selected_count; i++) {
            run_benchmark_by_name(selected_benchmarks[i]);
        }
    }
    else {
        run_benchmarks();
    }

    for (size_t i = 0; i < buffer_count; i++) {
        free(benchmark_buffers[i]);
    }

    // Cleanup
    free_vm();

    return 0;
}

#ifdef _MSC_VER
////////////////////////////////////////////////////////////////////////////////
// This implementation taken from StackOverflow user Michaelangel007's example:
//    https://stackoverflow.com/a/26085827
//    "Here is a free implementation:"
////////////////////////////////////////////////////////////////////////////////
int gettimeofday(struct timeval* tp, struct timezone* unused)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch
    // has 9 trailing zero's. This magic number is the number of 100 nanosecond 
    // intervals since January 1, 1601 (UTC) until 00:00:00 January 1, 1970.
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
#endif
