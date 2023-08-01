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
#include "comm.h"
#include "strings.h"

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#include <windows.h>
#include <winsock.h>
#include <stdint.h>
#include <io.h>
#define CLOSE_SOCKET closesocket
#define SOCKLEN int
#else
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#define CLOSE_SOCKET close
#define SOCKLEN socklen_t
#define SOCKET int
#ifndef _XOPEN_CRYPT
#include <crypt.h>
#endif
#endif

// Global variables.
DESCRIPTOR_DATA* descriptor_list;   // All open descriptors
DESCRIPTOR_DATA* d_next;            // Next descriptor in loop
FILE* fpReserve;                    // Reserved file handle
bool god;                           // All new chars are gods!
bool merc_down = false;             // Shutdown
bool wizlock;                       // Game is wizlocked
bool newlock;                       // Game is newlocked
char str_boot_time[MAX_INPUT_LENGTH];
time_t current_time;                // time of this pulse

bool rt_opt_benchmark = false;
bool rt_opt_noloop = false;
char area_dir[256] = DEFAULT_AREA_DIR;

void game_loop(SOCKET control);

#ifdef _MSC_VER
int gettimeofday(struct timeval* tp, struct timezone* tzp);
#endif

int main(int argc, char** argv)
{
    struct timeval now_time = { 0 };
    int port;

    SOCKET control;

    /*
 * Get the command line arguments.
 */
    port = 4000;
    char* port_str = NULL;
    char* area_dir_str = NULL;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (is_number(argv[i])) {
                port = atoi(argv[i]);
            }
            else if (!strcmp(argv[i], "-p")) {
                if (++i < argc) {
                    port_str = argv[i];
                }
            }
            else if (!strncmp(argv[i], "--port=", 7)) {
                port_str = argv[i] + 7;
            }
            else if (!strcmp(argv[i], "-a")) {
                if (++i < argc) {
                    area_dir_str = argv[i];
                }
            }
            else if (!strncmp(argv[i], "--area-dir=", 7)) {
                area_dir_str = argv[i] + 11;
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

    if (port_str) {
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

    if (area_dir_str) {
        size_t len = strlen(area_dir_str);
        if (area_dir_str[len - 1] != '/' && area_dir_str[len - 1] != '\\')
            sprintf(area_dir, "%s/", area_dir_str);
        else
            sprintf(area_dir, "%s", area_dir_str);
    }

#ifdef _MSC_VER
    if (!rt_opt_noloop) {
        WORD wVersionRequested;
        WSADATA wsaData;
        int err;

        /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
        wVersionRequested = MAKEWORD(2, 2);

        err = WSAStartup(wVersionRequested, &wsaData);
        if (err != 0) {
            /* Tell the user that we could not find a usable */
            /* Winsock DLL.                                  */
            printf("WSAStartup failed with error: %d\n", err);
            exit(1);
        }
    }
#endif

    // Init time.
    gettimeofday(&now_time, NULL);
    current_time = (time_t)now_time.tv_sec;
    strcpy(str_boot_time, ctime(&current_time));

    // Reserve one channel for our use.
    if ((fpReserve = fopen(NULL_FILE, "r")) == NULL) {
        perror(NULL_FILE);
        exit(1);
    }

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
    }

    if (!rt_opt_noloop) {
        control = init_socket(port);
        sprintf(log_buf, "ROM is ready to rock on port %d.", port);
        log_string(log_buf);
        game_loop(control);
        CLOSE_SOCKET(control);

#ifdef _MSC_VER
        WSACleanup();
#endif
    }

    /*
     * That's all, folks.
     */
    log_string("Normal termination of game.");
    exit(0);
    return 0;
}

#ifdef _MSC_VER
////////////////////////////////////////////////////////////////////////////////
// This implementation taken from StackOverflow user Michaelangel007's example:
//    https://stackoverflow.com/a/26085827
//    "Here is a free implementation:"
////////////////////////////////////////////////////////////////////////////////
int gettimeofday(struct timeval* tp, struct timezone* tzp)
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


void game_loop(SOCKET control)
{
    static struct timeval null_time;
    struct timeval last_time;

#ifndef _MSC_VER
    signal(SIGPIPE, SIG_IGN);
#endif
    gettimeofday(&last_time, NULL);
    current_time = (time_t)last_time.tv_sec;

    /* Main loop */
    while (!merc_down) {
        fd_set in_set = { 0 };
        fd_set out_set = { 0 };
        fd_set exc_set = { 0 };
        SOCKET maxdesc;
        DESCRIPTOR_DATA* d;

#if defined(MALLOC_DEBUG)
        if (malloc_verify() != 1) abort();
#endif

        /*
         * Poll all active descriptors.
         */
        FD_ZERO(&in_set);
        FD_ZERO(&out_set);
        FD_ZERO(&exc_set);
        FD_SET(control, &in_set);
        maxdesc = control;
        for (d = descriptor_list; d; d = d->next) {
            maxdesc = UMAX(maxdesc, d->descriptor);
            FD_SET(d->descriptor, &in_set);
            FD_SET(d->descriptor, &out_set);
            FD_SET(d->descriptor, &exc_set);
        }

        if (select((int)maxdesc + 1, &in_set, &out_set, &exc_set, &null_time) < 0) {
            perror("Game_loop: select: poll");
#ifdef _MSC_VER
            PrintLastWinSockError();
#endif
            CLOSE_SOCKET(control);
            exit(1);
        }

        /*
         * New connection?
         */
        if (FD_ISSET(control, &in_set)) init_descriptor(control);

        /*
         * Kick out the freaky folks.
         */
        for (d = descriptor_list; d != NULL; d = d_next) {
            d_next = d->next;
            if (FD_ISSET(d->descriptor, &exc_set)) {
                FD_CLR(d->descriptor, &in_set);
                FD_CLR(d->descriptor, &out_set);
                if (d->character && d->connected == CON_PLAYING)
                    save_char_obj(d->character);
                d->outtop = 0;
                close_socket(d);
            }
        }

        /*
         * Process input.
         */
        for (d = descriptor_list; d != NULL; d = d_next) {
            d_next = d->next;
            d->fcommand = false;

            if (FD_ISSET(d->descriptor, &in_set)) {
                if (d->character != NULL) d->character->timer = 0;
                if (!read_from_descriptor(d)) {
                    FD_CLR(d->descriptor, &out_set);
                    if (d->character != NULL && d->connected == CON_PLAYING)
                        save_char_obj(d->character);
                    d->outtop = 0;
                    close_socket(d);
                    continue;
                }
            }

            if (d->character != NULL && d->character->daze > 0)
                --d->character->daze;

            if (d->character != NULL && d->character->wait > 0) {
                --d->character->wait;
                continue;
            }

            read_from_buffer(d);
            if (d->incomm[0] != '\0') {
                d->fcommand = true;
                stop_idling(d->character);

                if (d->showstr_point && *d->showstr_point != '\0')
                    show_string(d, d->incomm);
                else if (d->connected == CON_PLAYING)
                    substitute_alias(d, d->incomm);
                else
                    nanny(d, d->incomm);

                d->incomm[0] = '\0';
            }
        }

        /*
         * Autonomous game motion.
         */
        update_handler();

        /*
         * Output.
         */
        for (d = descriptor_list; d != NULL; d = d_next) {
            d_next = d->next;

            if ((d->fcommand || d->outtop > 0)
                && FD_ISSET(d->descriptor, &out_set)) {
                if (!process_output(d, true)) {
                    if (d->character != NULL && d->connected == CON_PLAYING)
                        save_char_obj(d->character);
                    d->outtop = 0;
                    close_socket(d);
                }
            }
        }

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
