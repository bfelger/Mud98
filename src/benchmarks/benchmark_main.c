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

#ifdef _MSC_VER
#include <stdint.h>
#include <io.h>
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

int main(int argc, char** argv)
{
    struct timeval now_time = { 0 };
    char run_dir[256] = { 0 };
    char area_dir[256] = { 0 };

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
            else {
                fprintf(stderr, "Unknown argument '%s'.\n", argv[i]);
                fprintf(stderr, "Usage: %s [-d|--dir=<rundir>] [-a|--area-dir=<areadir>]\n", argv[0]);
                exit(1);
            }
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

    // Run all benchmarks
    run_benchmarks();

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
