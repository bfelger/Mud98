/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@hypercube.org)                                *
 *      Gabrielle Taylor (gtaylor@hypercube.org)                           *
 *      Brian Moore (zump@rom.org)                                         *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#include "merc.h"

#include "benchmark.h"
#include "config.h"
#include "comm.h"
#include "db.h"
#include "file.h"
#include "strings.h"
#include "tests.h"
#include "update.h"

#include "entities/descriptor.h"

#ifdef _MSC_VER
#include <stdint.h>
#include <io.h>
#else
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#endif

// Global variables.
bool merc_down = false;             // Shutdown
bool wizlock;                       // Game is wizlocked
bool newlock;                       // Game is newlocked
char str_boot_time[MAX_INPUT_LENGTH];
time_t current_time;                // time of this pulse
bool MOBtrigger = true;             // act() switch

void game_loop(SockServer* telnet_server, TlsServer* tls_server);

#ifdef _MSC_VER
struct timezone {
    int  dummy;
};

int gettimeofday(struct timeval* tp, struct timezone* unused);
#endif

int main(int argc, char** argv)
{
    struct timeval now_time = { 0 };
    int port;

    SockServer* telnet_server = NULL;
    TlsServer* tls_server = NULL;

    // Get the command line arguments.
    port = 4000;
    char port_str[256] = { 0 };
    char run_dir[256] = { 0 };
    char area_dir[256] = { 0 };
    bool rt_opt_benchmark = false;
    bool rt_opt_noloop = false;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (is_number(argv[i])) {
                port = atoi(argv[i]);
            }
            else if (!strcmp(argv[i], "-p")) {
                if (++i < argc) {
                    strcpy(port_str, argv[i]);
                }
            }
            else if (!strncmp(argv[i], "--port=", 7)) {
                strcpy(port_str, argv[i] + 7);
            }
            else if (!strncmp(argv[i], "--dir=", 6)) {
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
            else if (!strcmp(argv[i], "--benchmark")) {
                rt_opt_benchmark = true;
            }
            else if (!strcmp(argv[i], "--benchmark-only")) {
                rt_opt_benchmark = true;
                rt_opt_noloop = true;
            }
            else if (argv[i][0] == '-') {
                char* opt = argv[i] + 1;
                while (*opt != '\0') {
                    switch (*opt) {
                    case 'b':
                        rt_opt_benchmark = true;
                        break;
                    case 'B':
                        rt_opt_benchmark = true;
                        rt_opt_noloop = true;
                        break;
                    default:
                        fprintf(stderr, "Unknown option '-%c'.\n", *opt);
                        exit(1);
                    }
                    opt++;
                }
            }
            else {
                fprintf(stderr, "Unknown argument '%s'.\n", argv[i]);
                exit(1);
            }
        }
    }

    if (port_str[0]) {
        if (is_number(port_str)) {
            port = atoi(port_str);
        }
        else {
            fprintf(stderr, "Must specify a number for port.\n");
            exit(1);
        }
    }

    if (port <= 1024) {
        fprintf(stderr, "Port number must be above 1024.\n");
        exit(1);
    }

    if (area_dir[0]) {
        cfg_set_area_dir(area_dir);
    }

    if (run_dir[0])
        cfg_set_base_dir(run_dir);

    // Init time.
    gettimeofday(&now_time, NULL);
    current_time = (time_t)now_time.tv_sec;
    strcpy(str_boot_time, ctime(&current_time));

    open_reserve_file();

    load_config();

    if (port_str)
        cfg_set_telnet_port(port);

    /*
     * Run the game.
     */
    Timer boot_timer = { 0 };
    if (rt_opt_benchmark)
        start_timer(&boot_timer);

    boot_db();
    if (rt_opt_benchmark) {
        stop_timer(&boot_timer);
        struct timespec timer_res = elapsed(&boot_timer);
        sprintf(log_buf, "Boot time: "TIME_FMT"s, %ldns.", timer_res.tv_sec, timer_res.tv_nsec);
        log_string(log_buf);

        run_unit_tests();
    }

    if (!rt_opt_noloop) {
        bool telnet = cfg_get_telnet_enabled();
        bool tls = cfg_get_tls_enabled();
        int telnet_port = cfg_get_telnet_port();
        int tls_port = cfg_get_tls_port();

        if (telnet) {
            telnet_server = (SockServer*)alloc_mem(sizeof(SockServer));
            memset(telnet_server, 0, sizeof(SockServer));
            telnet_server->type = SOCK_TELNET;
            init_server(telnet_server, telnet_port);
        }
        if (tls) {
            tls_server = (TlsServer*)alloc_mem(sizeof(TlsServer));
            memset(tls_server, 0, sizeof(TlsServer));
            tls_server->type = SOCK_TLS;
            init_server((SockServer*)tls_server, tls_port);
        }

        if (telnet && tls) {
            sprintf(log_buf, MUD_NAME " is ready to rock on ports %d (telnet) "
                "& %d (tls).", telnet_port, tls_port);
        }
        else if (telnet) {
            sprintf(log_buf, MUD_NAME " is ready to rock on port %d (telnet). ",
                telnet_port);
        }
        else if (tls) {
            sprintf(log_buf, MUD_NAME " is ready to rock on port %d (tls). ",
                tls_port);
        }
        else {
            sprintf(log_buf, "You must enable either telnet or TLS in mud98.cfg.");
        }
        log_string(log_buf);
        game_loop(telnet_server, tls_server);

        if (telnet_server)
            close_server(telnet_server);
        if (tls_server)
            close_server((SockServer*)tls_server);
    }

    /*
     * That's all, folks.
     */
    log_string("Normal termination of game.");
    exit(0);
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

void game_loop(SockServer* telnet_server, TlsServer* tls_server)
{
    struct timeval last_time;
    gettimeofday(&last_time, NULL);
    current_time = (time_t)last_time.tv_sec;

    // Main loop
    while (!merc_down) {
        PollData telnet_poll_data = { 0 };
        PollData tls_poll_data = { 0 };

        if (telnet_server) {
            poll_server(telnet_server, &telnet_poll_data);

            // New connection?
            if (has_new_conn(telnet_server, &telnet_poll_data))
                handle_new_connection(telnet_server);

            process_client_input(telnet_server, &telnet_poll_data);
        }

        if (tls_server) {
            poll_server((SockServer*)tls_server, &tls_poll_data);

            // New connection?
            if (has_new_conn((SockServer*)tls_server, &tls_poll_data))
                handle_new_connection((SockServer*)tls_server);

            process_client_input((SockServer*)tls_server, &tls_poll_data);
        }

        // Autonomous game motion.
        update_handler();

        // Output.
        if (telnet_server)
            process_client_output(&telnet_poll_data, SOCK_TELNET);
        if (tls_server)
            process_client_output(&tls_poll_data, SOCK_TLS);

        /*
         * Synchronize to a clock.
         * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
         * Careful here of signed versus unsigned arithmetic.
         */
        {
            struct timeval now_time;
            long secDelta;
            long usecDelta;

            gettimeofday(&now_time, NULL);
            usecDelta = ((int)last_time.tv_usec) - ((int)now_time.tv_usec)
                + 1000000 / PULSE_PER_SECOND;
            secDelta = ((int)last_time.tv_sec) - ((int)now_time.tv_sec);
            while (usecDelta < 0) {
                usecDelta += 1000000;
                secDelta -= 1;
            }

            while (usecDelta >= 1000000) {
                usecDelta -= 1000000;
                secDelta += 1;
            }

            if (secDelta > 0 || (secDelta == 0 && usecDelta > 0)) {
#ifdef _MSC_VER
                long int mSeconds = (secDelta * 1000) + (usecDelta / 1000);
                if (mSeconds > 0) {
                    Sleep(mSeconds);
                }
#else
                struct timeval stall_time;

                stall_time.tv_usec = usecDelta;
                stall_time.tv_sec = secDelta;
                if (select(0, NULL, NULL, NULL, &stall_time) < 0) {
                    perror("Game_loop: select: stall");
                    exit(1);
                }
#endif
            }
        }

        gettimeofday(&last_time, NULL);
        current_time = (time_t)last_time.tv_sec;
    }

    return;
}
