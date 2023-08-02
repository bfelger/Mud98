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

#include "merc.h"

#include "comm.h"
#include "interp.h"
#include "recycle.h"
#include "strings.h"
#include "tables.h"
#include "telnet.h"

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifndef USE_RAW_SOCKETS
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#endif

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
#include <pthread.h>
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

const char echo_off_str[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const char echo_on_str[] = { IAC, WONT, TELOPT_ECHO, '\0' };
const char go_ahead_str[] = { IAC, GA, '\0' };

/*
 * Malloc debugging stuff.
 */
#if defined(MALLOC_DEBUG)
#include <malloc.h>
extern int malloc_debug args((int));
extern int malloc_verify args((void));
#endif

DESCRIPTOR_DATA* descriptor_list;           // All open descriptors
DESCRIPTOR_DATA* d_next;                    // Next descriptor in loop

typedef enum {
    THREAD_STATUS_NONE,
    THREAD_STATUS_STARTED,
    THREAD_STATUS_RUNNING, 
    THREAD_STATUS_FINISHED,
} ThreadStatus;

#ifdef _MSC_VER
#define INIT_DESC_RET DWORD WINAPI
#define INIT_DESC_PARAM LPVOID
#define THREAD_ERR -1
#else
#define INIT_DESC_RET void*
#define INIT_DESC_PARAM void*
#define THREAD_ERR (void*)-1
#endif

typedef struct new_conn_thread_t {
#ifdef _MSC_VER
    HANDLE thread;
    DWORD id;
#else
    pthread_t id;
#endif
    ThreadStatus status;
} NewConnThread;

typedef struct thread_data_t {
    SockServer* server;
    int index;
} ThreadData;

NewConnThread new_conn_threads[MAX_HANDSHAKES] = { 0 };
ThreadData thread_data[MAX_HANDSHAKES] = { 0 };

extern bool merc_down;                      // Shutdown
extern bool wizlock;                        // Game is wizlocked
extern bool newlock;                        // Game is newlocked

void bust_a_prompt args((CHAR_DATA* ch));
bool check_parse_name args((char* name));
bool check_playing args((DESCRIPTOR_DATA* d, char* name));
bool check_reconnect args((DESCRIPTOR_DATA* d, char* name, bool fConn));

#ifdef _MSC_VER
void PrintLastWinSockError()
{
    char msgbuf[256] = "";
    int err = WSAGetLastError();

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, // flags
        NULL,           // lpsource
        err,            // message id
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // languageid
        msgbuf,         // output buffer
        sizeof(msgbuf), // size of msgbuf, bytes
        NULL);          // va_list of arguments

    if (!*msgbuf)
        sprintf(msgbuf, "%d", err);

    printf("Error: %s", msgbuf);
}
#endif

#ifndef USE_RAW_SOCKETS
void init_ssl_server(SockServer* server)
{
    log_string("Initializing SSL server:");

    SSL_library_init();
    OpenSSL_add_all_algorithms();

    if ((server->ssl_ctx = SSL_CTX_new(TLS_server_method())) == NULL) {
        perror("! Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    else {
        printf("* Context created.");
    }

    char cert_file[256];
    sprintf(cert_file, "%s%s", area_dir, CERT_FILE);

    char pkey_file[256];
    sprintf(pkey_file, "%s%s", area_dir, PKEY_FILE);

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(server->ssl_ctx, cert_file, 
            SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "! Failed to open SSL certificate file %s", cert_file);
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    else {
        printf("* Certificate %s loaded.", cert_file);
    }

    if (SSL_CTX_use_PrivateKey_file(server->ssl_ctx, pkey_file, 
            SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "! Failed to open SSL private key file %s", pkey_file);
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    else {
        printf("* Private key %s loaded.", cert_file);
    }

    if (SSL_CTX_check_private_key(server->ssl_ctx) <= 0) {
        perror("! Certificate/pkey validation failed.");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    else {
        printf("* Cert/pkey validated.");
    }

    // Only allow TLS
    SSL_CTX_set_options(server->ssl_ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 
        | SSL_OP_NO_SSLv3);
}
#endif

void init_server(SockServer* server, int port)
{
    static struct sockaddr_in sa_zero;
    struct sockaddr_in sa;
    int x = 1;
    int errno;

#ifndef USE_RAW_SOCKETS
    init_ssl_server(server);
#endif

#ifndef _MSC_VER
    signal(SIGPIPE, SIG_IGN);
#else
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    // Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        // Tell the user that we could not find a usable Winsock DLL.
        printf("WSAStartup failed with error: %d\n", err);
        exit(1);
    }
#endif

    if ((server->control = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Init_socket: socket");
#ifdef _MSC_VER
        PrintLastWinSockError();
#endif
        exit(1);
    }

    if (setsockopt(server->control, SOL_SOCKET, SO_REUSEADDR, (char*)&x, sizeof(x)) < 0) {
        perror("Init_socket: SO_REUSEADDR");
#ifdef _MSC_VER
        PrintLastWinSockError();
#endif
        close_server(server);
        exit(1);
    }

#ifdef SO_DONTLINGER
    {
        struct linger ld = { 0 };

        ld.l_onoff = 1;
        ld.l_linger = 1000;

        if (setsockopt(server->control, SOL_SOCKET, SO_DONTLINGER, (char*)&ld, sizeof(ld)) 
            < 0) {
            perror("Init_socket: SO_DONTLINGER");
#ifdef _MSC_VER
            PrintLastWinSockError();
#endif
            close_server(server);
            exit(1);
        }
    }
#endif

    sa = sa_zero;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

    if (bind(server->control, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        perror("Init socket: bind");
#ifdef _MSC_VER
        PrintLastWinSockError();
#endif
        close_server(server);
        exit(1);
    }

    if (listen(server->control, 3) < 0) {
        perror("Init socket: listen");
#ifdef _MSC_VER
        PrintLastWinSockError();
#endif
        close_server(server);
        exit(1);
    }
}

bool can_write(DESCRIPTOR_DATA* d, PollData* poll_data)
{
    return FD_ISSET(d->client.fd, &poll_data->out_set);
}

void close_server(SockServer* server)
{
    CLOSE_SOCKET(server->control);

#ifndef USE_RAW_SOCKETS
    SSL_CTX_free(server->ssl_ctx);
#endif

#ifdef _MSC_VER
    WSACleanup();
#endif
}

void close_client(SockClient* client)
{
#ifndef USE_RAW_SOCKETS
    if (client->ssl) {
        SSL_shutdown(client->ssl);
        SSL_free(client->ssl);
    }
#endif
    CLOSE_SOCKET(client->fd);
}

bool has_new_conn(SockServer* server, PollData* poll_data)
{
    return FD_ISSET(server->control, &poll_data->in_set);
}

static INIT_DESC_RET init_descriptor(INIT_DESC_PARAM lp_data)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA* dnew;
    struct sockaddr_in sock = { 0 };
    struct hostent* from;
    SOCKLEN size;
    SockClient client = { 0 };

    ThreadData* data = (ThreadData*)lp_data;
    SockServer* server = data->server;

    size = sizeof(sock);
    getsockname(server->control, (struct sockaddr*)&sock, &size);

    new_conn_threads[thread_data->index].status = THREAD_STATUS_RUNNING;
    if ((client.fd = accept(server->control, (struct sockaddr*)&sock, &size)) < 0) {
        perror("New_descriptor: accept");
        return THREAD_ERR;
    }

#ifndef USE_RAW_SOCKETS
    client.ssl = SSL_new(server->ssl_ctx);
    SSL_set_fd(client.ssl, client.fd);

    if (SSL_accept(client.ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        close_client(&client);
        return THREAD_ERR;
    }
#endif

#ifndef _MSC_VER
#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

    if (fcntl(client.fd, F_SETFL, FNDELAY) == -1) {
        perror("New_descriptor: fcntl: FNDELAY");
        return THREAD_ERR;
    }
#endif

    // Cons a new descriptor.
    dnew = new_descriptor();

    dnew->client = client;
    dnew->connected = CON_GET_NAME;
    dnew->showstr_head = NULL;
    dnew->showstr_point = NULL;
    dnew->outsize = 2000;
    dnew->outbuf = alloc_mem(dnew->outsize);

    size = sizeof(sock);
    if (getpeername(dnew->client.fd, (struct sockaddr*)&sock, &size) < 0) {
        perror("New_descriptor: getpeername");
        dnew->host = str_dup("(unknown)");
    }
    else {
        /*
         * Would be nice to use inet_ntoa here but it takes a struct arg,
         * which ain't very compatible between gcc and system libraries.
         */
        int addr;

        addr = ntohl(sock.sin_addr.s_addr);
        sprintf(buf, "%d.%d.%d.%d",
            (addr >> 24) & 0xFF, (addr >> 16) & 0xFF,
            (addr >> 8) & 0xFF, (addr)&0xFF
        );
        sprintf(log_buf, "Sock.sinaddr:  %s", buf);
        log_string(log_buf);
        from = gethostbyaddr((char*)&sock.sin_addr,
            sizeof(sock.sin_addr), AF_INET);
        dnew->host = str_dup(from ? from->h_name : buf);
    }

    /*
     * Swiftest: I added the following to ban sites.  I don't
     * endorse banning of sites, but Copper has few descriptors now
     * and some people from certain sites keep abusing access by
     * using automated 'autodialers' and leaving connections hanging.
     *
     * Furey: added suffix check by request of Nickel of HiddenWorlds.
     */
    if (check_ban(dnew->host, BAN_ALL)) {
        write_to_descriptor(dnew, "Your site has been banned from this mud.\n\r", 0);
        close_client(&dnew->client);
        free_descriptor(dnew);
        return THREAD_ERR;
    }
    /*
     * Init descriptor data.
     */
    dnew->next = descriptor_list;
    descriptor_list = dnew;

    /*
     * Send the greeting.
     */
    {
        extern char* help_greeting;
        if (help_greeting[0] == '.')
            write_to_buffer(dnew, help_greeting + 1, 0);
        else
            write_to_buffer(dnew, help_greeting, 0);
    }

    new_conn_threads[thread_data->index].status = THREAD_STATUS_FINISHED;

#ifndef _MSC_VER
    pthread_exit(NULL);
#endif

    return 0;
}

void handle_new_connection(SockServer* server) {
    int i;
    int new_thread_ix = -1;

    for (i = 0; i < MAX_HANDSHAKES; i++) {
        if (new_conn_threads[i].status == THREAD_STATUS_STARTED) {
            // We are still processing a handshake. Do nothing.
            new_thread_ix = -2;
        }

        if (new_conn_threads[i].status == THREAD_STATUS_FINISHED) {
#ifdef _MSC_VER
            CloseHandle(new_conn_threads[i].thread);
#endif
            new_conn_threads[i].id = 0;
            new_conn_threads[i].status = THREAD_STATUS_NONE;
        }

        if (new_conn_threads[i].status == THREAD_STATUS_NONE
            && new_thread_ix == -1)
            new_thread_ix = i;
    }

    if (new_thread_ix == -1) {
        // New threads are maxed out.
        perror("Not ready to receive new connections, yet.");
        return;
    }
    else if (new_thread_ix == -2) {
        // Still spinning up a previous thread.
        return;
    }

    thread_data[new_thread_ix].index = new_thread_ix;
    thread_data[new_thread_ix].server = server;

    new_conn_threads[new_thread_ix].status = THREAD_STATUS_STARTED;
#ifdef _MSC_VER
    new_conn_threads[new_thread_ix].thread = CreateThread(
        NULL,                                   // default security attributes
        0,                                      // use default stack size  
        init_descriptor,                        // thread function name
        &thread_data[new_thread_ix],            // argument to thread function 
        0,                                      // use default creation flags 
        &(new_conn_threads[new_thread_ix].id)   // returns the thread identifier 
    );

    if (new_conn_threads[new_thread_ix].thread == NULL) {
        perror("handle_new_connection()");
        return;
    }
#else
    if (pthread_create(&(new_conn_threads[new_thread_ix].id), NULL, 
            init_descriptor, &thread_data[new_thread_ix]) != 0) {
        perror("handle_new_connection(): pthread_create()");
    }
#endif
}

void close_socket(DESCRIPTOR_DATA* dclose)
{
    if (dclose == NULL) {
        log_string("close_socket: NULL descriptor!");
        return;
    }

    if (descriptor_list == NULL) {
        log_string("close_socket: Descriptor not NULL but descriptor list is?");
        return;
    }

    CHAR_DATA* ch;

    if (dclose->outtop > 0)
        process_descriptor_output(dclose, false);

    if (dclose->snoop_by != NULL) {
        write_to_buffer(dclose->snoop_by, "Your victim has left the game.\n\r", 
            0);
    }

    {
        DESCRIPTOR_DATA* d;

        for (d = descriptor_list; d != NULL; d = d->next) {
            if (d->snoop_by == dclose) d->snoop_by = NULL;
        }
    }

    if ((ch = dclose->character) != NULL) {
        sprintf(log_buf, "Closing link to %s.", ch->name);
        log_string(log_buf);
        /* cut down on wiznet spam when rebooting */
        if (dclose->connected == CON_PLAYING && !merc_down) {
            act("$n has lost $s link.", ch, NULL, NULL, TO_ROOM);
            wiznet("Net death has claimed $N.", ch, NULL, WIZ_LINKS, 0, 0);
            ch->desc = NULL;
        }
        else {
            free_char(dclose->original ? dclose->original : dclose->character);
        }
    }

    if (d_next == dclose) 
        d_next = d_next->next;

    if (dclose == descriptor_list) {
        descriptor_list = descriptor_list->next;
    }
    else {
        DESCRIPTOR_DATA* d;

        for (d = descriptor_list; d && d->next != dclose; d = d->next)
            ;
        if (d != NULL)
            d->next = dclose->next;
        else
            bug("Close_socket: dclose not found.", 0);
    }

    close_client(&dclose->client);
    free_descriptor(dclose);
    return;
}

bool read_from_descriptor(DESCRIPTOR_DATA* d)
{
#ifndef USE_RAW_SOCKETS
    int ssl_err;
#endif

    /* Hold horses if pending command already. */
    if (d->incomm[0] != '\0') return true;

    /* Check for overflow. */
    size_t iStart = strlen(d->inbuf);
    if (iStart >= sizeof(d->inbuf) - 10) {
        sprintf(log_buf, "%s input overflow!", d->host);
        log_string(log_buf);
        write_to_descriptor(d, "\n\r*** PUT A LID ON IT!!! ***\n\r", 0);
        return false;
    }

    /* Snarf input. */
    for (;;) {
        size_t nRead;

#ifndef USE_RAW_SOCKETS
        if ((ssl_err = SSL_read_ex(d->client.ssl, d->inbuf + iStart,
                sizeof(d->inbuf) - 10 - iStart, &nRead)) <= 0) {
            ERR_print_errors_fp(stderr);
            return false;
        }
#else
#ifdef _MSC_VER
        nRead = recv(d->client.fd, d->inbuf + iStart, 
            (int)(sizeof(d->inbuf) - 10 - iStart), 0);
#else
        nRead = read(d->client.fd, d->inbuf + iStart,
            sizeof(d->inbuf) - 10 - iStart);
#endif
#endif // !USE_RAW_SOCKETS
        if (nRead > 0) {
            iStart += nRead;
            if (d->inbuf[iStart - 1] == '\n' || d->inbuf[iStart - 1] == '\r')
                break;
        }
        else if (nRead == 0) {
            log_string("EOF encountered on read.");
            return false;
        }
        else if (errno == EWOULDBLOCK)
            break;
        else {
            perror("Read_from_descriptor");
            return false;
        }
    }

    d->inbuf[iStart] = '\0';
    return true;
}

void process_client_input(SockServer* server, PollData* poll_data)
{
    // Kick out the freaky folks.
    for (DESCRIPTOR_DATA* d = descriptor_list; d != NULL; d = d_next) {
        d_next = d->next;
        if (FD_ISSET(d->client.fd, &poll_data->exc_set)) {
            FD_CLR(d->client.fd, &poll_data->in_set);
            FD_CLR(d->client.fd, &poll_data->out_set);
            if (d->character && d->connected == CON_PLAYING)
                save_char_obj(d->character);
            d->outtop = 0;
            close_socket(d);
        }
    }

    // Process input.
    for (DESCRIPTOR_DATA* d = descriptor_list; d != NULL; d = d_next) {
        d_next = d->next;
        d->fcommand = false;

        if (FD_ISSET(d->client.fd, &poll_data->in_set)) {
            if (d->character != NULL) d->character->timer = 0;
            if (!read_from_descriptor(d)) {
                FD_CLR(d->client.fd, &poll_data->out_set);
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
}

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer(DESCRIPTOR_DATA* d)
{
    int i, j, k;

    /*
     * Hold horses if pending command already.
     */
    if (d->incomm[0] != '\0') return;

    /*
     * Look for at least one new line.
     */
    for (i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++) {
        if (d->inbuf[i] == '\0') return;
    }

    /*
     * Canonical input processing.
     */
    for (i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++) {
        if (k >= MAX_INPUT_LENGTH - 2) {
            write_to_descriptor(d, "Line too long.\n\r", 0);

            /* skip the rest of the line */
            for (; d->inbuf[i] != '\0'; i++) {
                if (d->inbuf[i] == '\n' || d->inbuf[i] == '\r') break;
            }
            d->inbuf[i] = '\n';
            d->inbuf[i + 1] = '\0';
            break;
        }

        if (d->inbuf[i] == '\b' && k > 0)
            --k;
        else if (ISASCII(d->inbuf[i]) && ISPRINT(d->inbuf[i]))
            d->incomm[k++] = d->inbuf[i];
    }

    /*
     * Finish off the line.
     */
    if (k == 0) d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    /*
     * Deal with bozos with #repeat 1000 ...
     */

    if (k > 1 || d->incomm[0] == '!') {
        if (d->incomm[0] != '!' && strcmp(d->incomm, d->inlast)) {
            d->repeat = 0;
        }
        else {
            if (++d->repeat >= 25 && d->character
                && d->connected == CON_PLAYING) {
                sprintf(log_buf, "%s input spamming!", d->host);
                log_string(log_buf);
                wiznet("Spam spam spam $N spam spam spam spam spam!",
                    d->character, NULL, WIZ_SPAM, 0, 
                    get_trust(d->character));
                if (d->incomm[0] == '!')
                    wiznet(d->inlast, d->character, NULL, WIZ_SPAM, 0,
                        get_trust(d->character));
                else
                    wiznet(d->incomm, d->character, NULL, WIZ_SPAM, 0,
                        get_trust(d->character));

                d->repeat = 0;
            }
        }
    }

    /*
     * Do '!' substitution.
     */
    if (d->incomm[0] == '!')
        strcpy(d->incomm, d->inlast);
    else
        strcpy(d->inlast, d->incomm);

    /*
     * Shift the input buffer.
     */
    while (d->inbuf[i] == '\n' || d->inbuf[i] == '\r') i++;
    for (j = 0; (d->inbuf[j] = d->inbuf[i + j]) != '\0'; j++)
        ;
    return;
}

/*
 * Low level output function.
 */
bool process_descriptor_output(DESCRIPTOR_DATA* d, bool fPrompt)
{
    extern bool merc_down;

    /*
     * Bust a prompt.
     */
    if (!merc_down && d->showstr_point && *d->showstr_point != '\0')
        write_to_buffer(d, "[Hit Return to continue]\n\r", 0);
    else if (fPrompt && !merc_down && d->connected == CON_PLAYING) {
        CHAR_DATA* ch;
        CHAR_DATA* victim;

        ch = d->character;

        /* battle prompt */
        if ((victim = ch->fighting) != NULL && can_see(ch, victim)) {
            int percent;
            char wound[100];
            char buf[MAX_STRING_LENGTH];

            if (victim->max_hit > 0)
                percent = victim->hit * 100 / victim->max_hit;
            else
                percent = -1;

            if (percent >= 100)
                sprintf(wound, "is in excellent condition.");
            else if (percent >= 90)
                sprintf(wound, "has a few scratches.");
            else if (percent >= 75)
                sprintf(wound, "has some small wounds and bruises.");
            else if (percent >= 50)
                sprintf(wound, "has quite a few wounds.");
            else if (percent >= 30)
                sprintf(wound, "has some big nasty wounds and scratches.");
            else if (percent >= 15)
                sprintf(wound, "looks pretty hurt.");
            else if (percent >= 0)
                sprintf(wound, "is in awful condition.");
            else
                sprintf(wound, "is bleeding to death.");

            sprintf(buf, "%s %s \n\r",
                IS_NPC(victim) ? victim->short_descr : victim->name, wound);
            buf[0] = UPPER(buf[0]);
            write_to_buffer(d, buf, 0);
        }

        ch = d->original ? d->original : d->character;
        if (!IS_SET(ch->comm, COMM_COMPACT))
            write_to_buffer(d, "\n\r", 2);

        if (IS_SET(ch->comm, COMM_PROMPT))
            bust_a_prompt(d->character);

        if (IS_SET(ch->comm, COMM_TELNET_GA))
            write_to_buffer(d, go_ahead_str, 0);
    }

    /*
     * Short-circuit if nothing to write.
     */
    if (d->outtop == 0) return true;

    /*
     * Snoop-o-rama.
     */
    if (d->snoop_by != NULL) {
        if (d->character != NULL)
            write_to_buffer(d->snoop_by, d->character->name, 0);
        write_to_buffer(d->snoop_by, "> ", 2);
        write_to_buffer(d->snoop_by, d->outbuf, d->outtop);
    }

    /*
     * OS-dependent output.
     */
    if (!write_to_descriptor(d, d->outbuf, d->outtop)) {
        d->outtop = 0;
        return false;
    }
    else {
        d->outtop = 0;
        return true;
    }
}

/*
 * Bust a prompt (player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void bust_a_prompt(CHAR_DATA* ch)
{
    INIT_BUF(temp1, MAX_STRING_LENGTH);
    INIT_BUF(temp2, MAX_STRING_LENGTH);
    INIT_BUF(temp3, MAX_STRING_LENGTH * 2);

    const char* str;
    const char* i;
    char* point;
    char* pbuff;
    char doors[MAX_INPUT_LENGTH] = "";
    EXIT_DATA* pexit;
    bool found;
    const char* dir_name[] = {"N", "E", "S", "W", "U", "D"};
    int door;

    point = BUF(temp1);
    str = ch->prompt;
    if (str == NULL || str[0] == '\0') {
        sprintf(BUF(temp1), "{p<%dhp %dm %dmv>{x %s", ch->hit, ch->mana, ch->move, 
            ch->prefix);
        send_to_char(BUF(temp1), ch);
        goto bust_a_prompt_cleanup;
    }

    if (IS_SET(ch->comm, COMM_AFK)) {
        send_to_char("{p<AFK>{x ", ch);
        goto bust_a_prompt_cleanup;
    }

    while (*str != '\0') {
        if (*str != '%') {
            *point++ = *str++;
            continue;
        }
        ++str;
        switch (*str) {
        default:
            i = " ";
            break;
        case 'e':
            found = false;
            doors[0] = '\0';
            for (door = 0; door < 6; door++) {
                if ((pexit = ch->in_room->exit[door]) != NULL
                    && pexit->u1.to_room != NULL
                    && (can_see_room(ch, pexit->u1.to_room)
                        || (IS_AFFECTED(ch, AFF_INFRARED)
                            && !IS_AFFECTED(ch, AFF_BLIND)))
                    && !IS_SET(pexit->exit_info, EX_CLOSED)) {
                    found = true;
                    strcat(doors, dir_name[door]);
                }
            }
            if (!found) strcat(BUF(temp1), "none");
            sprintf(BUF(temp2), "%s", doors);
            i = BUF(temp2);
            break;
        case 'c':
            sprintf(BUF(temp2), "%s", "\n\r");
            i = BUF(temp2); 
            break;
        case 'h':
            sprintf(BUF(temp2), "%d", ch->hit);
            i = BUF(temp2);
            break;
        case 'H':
            sprintf(BUF(temp2), "%d", ch->max_hit);
            i = BUF(temp2);
            break;
        case 'm':
            sprintf(BUF(temp2), "%d", ch->mana);
            i = BUF(temp2);
            break;
        case 'M':
            sprintf(BUF(temp2), "%d", ch->max_mana);
            i = BUF(temp2);
            break;
        case 'v':
            sprintf(BUF(temp2), "%d", ch->move);
            i = BUF(temp2);
            break;
        case 'V':
            sprintf(BUF(temp2), "%d", ch->max_move);
            i = BUF(temp2);
            break;
        case 'x':
            sprintf(BUF(temp2), "%d", ch->exp);
            i = BUF(temp2);
            break;
        case 'X':
            sprintf(BUF(temp2), "%d",
                IS_NPC(ch) ? 0
                : (ch->level + 1)
                * exp_per_level(ch, ch->pcdata->points)
                - ch->exp);
            i = BUF(temp2);
            break;
        case 'g':
            sprintf(BUF(temp2), "%ld", ch->gold);
            i = BUF(temp2);
            break;
        case 's':
            sprintf(BUF(temp2), "%ld", ch->silver);
            i = BUF(temp2);
            break;
        case 'a':
            if (ch->level > 9)
                sprintf(BUF(temp2), "%d", ch->alignment);
            else
                sprintf(BUF(temp2), "%s",
                    IS_GOOD(ch)   ? "good"
                    : IS_EVIL(ch) ? "evil"
                    : "neutral");
            i = BUF(temp2);
            break;
        case 'r':
            if (ch->in_room != NULL)
                sprintf(BUF(temp2), "%s",
                    ((!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
                        || (!IS_AFFECTED(ch, AFF_BLIND)
                            && !room_is_dark(ch->in_room)))
                    ? ch->in_room->name
                    : "darkness");
            else
                sprintf(BUF(temp2), " ");
            i = BUF(temp2);
            break;
        case 'R':
            if (IS_IMMORTAL(ch) && ch->in_room != NULL)
                sprintf(BUF(temp2), "%d", ch->in_room->vnum);
            else
                sprintf(BUF(temp2), " ");
            i = BUF(temp2);
            break;
        case 'z':
            if (IS_IMMORTAL(ch) && ch->in_room != NULL)
                sprintf(BUF(temp2), "%s", ch->in_room->area->name);
            else
                sprintf(BUF(temp2), " ");
            i = BUF(temp2);
            break;
        case '%':
            sprintf(BUF(temp2), "%%");
            i = BUF(temp2);
            break;
        }
        ++str;
        while ((*point = *i) != '\0') ++point, ++i;
    }
    *point = '\0';
    pbuff = BUF(temp3);
    colourconv(pbuff, BUF(temp1), ch);
    send_to_char("{p", ch);
    write_to_buffer(ch->desc, BUF(temp3), 0);
    send_to_char("{x", ch);

    if (ch->prefix[0] != '\0') 
        write_to_buffer(ch->desc, ch->prefix, 0);

bust_a_prompt_cleanup:
    free_buf(temp1);
    free_buf(temp2);
    free_buf(temp3);
}

// Append onto an output buffer.
void write_to_buffer(DESCRIPTOR_DATA* d, const char* txt, size_t length)
{
    // Find length in case caller didn't.
    if (length <= 0) length = strlen(txt);

    // Initial \n\r if needed.
    if (d->outtop == 0 && !d->fcommand) {
        d->outbuf[0] = '\n';
        d->outbuf[1] = '\r';
        d->outtop = 2;
    }

    // Expand the buffer as needed.
    while (d->outtop + length >= d->outsize) {
        char* outbuf;

        if (d->outsize >= 32000) {
            bug("Buffer overflow. Closing.\n\r", 0);
            close_socket(d);
            return;
        }
        outbuf = alloc_mem(2 * d->outsize);
        strncpy(outbuf, d->outbuf, d->outtop);
        outbuf[d->outtop] = '\0';
        free_mem(d->outbuf, d->outsize);
        d->outbuf = outbuf;
        d->outsize *= 2;
    }

    // Copy.
    strncpy(d->outbuf + d->outtop, txt, length);
    d->outtop += length;
    return;
}

/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor(DESCRIPTOR_DATA* d, char* txt, size_t length)
{
    size_t nWrite = 0;
    int nBlock;
#ifndef USE_RAW_SOCKETS
    int ssl_err;
#endif

    if (length <= 0)
        length = strlen(txt);

    for (size_t iStart = 0; iStart < length; iStart += nWrite) {
        nBlock = (int)UMIN(length - iStart, 4096);
#ifndef USE_RAW_SOCKETS
        if ((ssl_err = SSL_write_ex(d->client.ssl, txt + iStart, nBlock, &nWrite)) <= 0) {
            ERR_print_errors_fp(stderr);
            return false;
        }
#else
#ifdef _MSC_VER
        if ((nWrite = send(d->client.fd, txt + iStart, nBlock, 0)) < 0) {
            PrintLastWinSockError();
#else
        if ((nWrite = write(d->client.fd, txt + iStart, nBlock)) < 0) {
#endif
            perror("Write_to_descriptor");
            return false;
        }
#endif // !USE_RAW_SOCKETS
    }

    return true;
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny(DESCRIPTOR_DATA* d, char* argument)
{
    DESCRIPTOR_DATA* d_old = NULL;
    DESCRIPTOR_DATA* d_next = NULL;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA* ch;
    char* pwdnew;
    char* p;
    int iClass, race, i, weapon;
    bool fOld;

    while (ISSPACE(*argument)) argument++;

    ch = d->character;

    switch (d->connected) {
    default:
        bug("Nanny: bad d->connected %d.", d->connected);
        close_socket(d);
        return;

    case CON_GET_NAME:
        if (argument[0] == '\0') {
            close_socket(d);
            return;
        }

        argument[0] = UPPER(argument[0]);
        if (!check_parse_name(argument)) {
            write_to_buffer(d, "Illegal name, try another.\n\rName: ", 0);
            return;
        }

        fOld = load_char_obj(d, argument);
        ch = d->character;

        if (IS_SET(ch->act, PLR_DENY)) {
            sprintf(log_buf, "Denying access to %s@%s.", argument, d->host);
            log_string(log_buf);
            write_to_buffer(d, "You are denied access.\n\r", 0);
            close_socket(d);
            return;
        }

        if (check_ban(d->host, BAN_PERMIT) && !IS_SET(ch->act, PLR_PERMIT)) {
            write_to_buffer(d, "Your site has been banned from this mud.\n\r", 0);
            close_socket(d);
            return;
        }

        if (check_reconnect(d, argument, false)) {
            fOld = true;
        }
        else {
            if (wizlock && !IS_IMMORTAL(ch)) {
                write_to_buffer(d, "The game is wizlocked.\n\r", 0);
                close_socket(d);
                return;
            }
        }

        if (fOld) {
            /* Old player */
            write_to_buffer(d, "Password: ", 0);
            write_to_buffer(d, echo_off_str, 0);
            d->connected = CON_GET_OLD_PASSWORD;
            return;
        }
        else {
            /* New player */
            if (newlock) {
                write_to_buffer(d, "The game is newlocked.\n\r", 0);
                close_socket(d);
                return;
            }

            if (check_ban(d->host, BAN_NEWBIES)) {
                write_to_buffer(d,
                    "New players are not allowed from your site.\n\r", 0);
                close_socket(d);
                return;
            }

            sprintf(buf, "Did I get that right, %s (Y/N)? ", argument);
            write_to_buffer(d, buf, 0);
            d->connected = CON_CONFIRM_NEW_NAME;
            return;
        }
        break;

    case CON_GET_OLD_PASSWORD:
        write_to_buffer(d, "\n\r", 2);

        if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd)) {
            write_to_buffer(d, "Wrong password.\n\r", 0);
            close_socket(d);
            return;
        }

        write_to_buffer(d, echo_on_str, 0);

        if (check_playing(d, ch->name)) return;

        if (check_reconnect(d, ch->name, true)) return;

        sprintf(log_buf, "%s@%s has connected.", ch->name, d->host);
        log_string(log_buf);
        wiznet(log_buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

        if (IS_IMMORTAL(ch)) {
            do_function(ch, &do_help, "imotd");
            d->connected = CON_READ_IMOTD;
        }
        else {
            do_function(ch, &do_help, "motd");
            d->connected = CON_READ_MOTD;
        }
        break;

        /* RT code for breaking link */

    case CON_BREAK_CONNECT:
        switch (*argument) {
        case 'y':
        case 'Y':
            for (d_old = descriptor_list; d_old != NULL; d_old = d_next) {
                d_next = d_old->next;
                if (d_old == d || d_old->character == NULL) continue;

                if (str_cmp(ch->name, d_old->original ? d_old->original->name
                    : d_old->character->name))
                    continue;

                close_socket(d_old);
            }
            if (check_reconnect(d, ch->name, true)) return;
            write_to_buffer(d, "Reconnect attempt failed.\n\rName: ", 0);
            if (d->character != NULL) {
                free_char(d->character);
                d->character = NULL;
            }
            d->connected = CON_GET_NAME;
            break;

        case 'n':
        case 'N':
            write_to_buffer(d, "Name: ", 0);
            if (d->character != NULL) {
                free_char(d->character);
                d->character = NULL;
            }
            d->connected = CON_GET_NAME;
            break;

        default:
            write_to_buffer(d, "Please type Y or N? ", 0);
            break;
        }
        break;

    case CON_CONFIRM_NEW_NAME:
        switch (*argument) {
        case 'y':
        case 'Y':
            sprintf(buf, "New character.\n\rGive me a password for %s: %s",
                ch->name, echo_off_str);
            write_to_buffer(d, buf, 0);
            d->connected = CON_GET_NEW_PASSWORD;
            break;

        case 'n':
        case 'N':
            write_to_buffer(d, "Ok, what IS it, then? ", 0);
            free_char(d->character);
            d->character = NULL;
            d->connected = CON_GET_NAME;
            break;

        default:
            write_to_buffer(d, "Please type Yes or No? ", 0);
            break;
        }
        break;

    case CON_GET_NEW_PASSWORD:
        write_to_buffer(d, "\n\r", 2);

        if (strlen(argument) < 5) {
            write_to_buffer(
                d,
                "Password must be at least five characters long.\n\rPassword: ",
                0);
            return;
        }

        pwdnew = crypt(argument, ch->name);
        for (p = pwdnew; *p != '\0'; p++) {
            if (*p == '~') {
                write_to_buffer(
                    d,
                    "New password not acceptable, try again.\n\rPassword: ", 0);
                return;
            }
        }

        free_string(ch->pcdata->pwd);
        ch->pcdata->pwd = str_dup(pwdnew);
        write_to_buffer(d, "Please retype password: ", 0);
        d->connected = CON_CONFIRM_NEW_PASSWORD;
        break;

    case CON_CONFIRM_NEW_PASSWORD:
        write_to_buffer(d, "\n\r", 2);

        if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd)) {
            write_to_buffer(d,
                "Passwords don't match.\n\rRetype password: ", 0);
            d->connected = CON_GET_NEW_PASSWORD;
            return;
        }

        write_to_buffer(d, echo_on_str, 0);
        write_to_buffer(d, "The following races are available:\n\r  ", 0);
        for (race = 1; race_table[race].name != NULL; race++) {
            if (!race_table[race].pc_race) break;
            write_to_buffer(d, race_table[race].name, 0);
            write_to_buffer(d, " ", 1);
        }
        write_to_buffer(d, "\n\r", 0);
        write_to_buffer(d, "What is your race (help for more information)? ",
            0);
        d->connected = CON_GET_NEW_RACE;
        break;

    case CON_GET_NEW_RACE:
        one_argument(argument, arg);

        if (!strcmp(arg, "help")) {
            argument = one_argument(argument, arg);
            if (argument[0] == '\0')
                do_function(ch, &do_help, "race help");
            else
                do_function(ch, &do_help, argument);
            write_to_buffer(
                d, "What is your race (help for more information)? ", 0);
            break;
        }

        race = race_lookup(argument);

        if (race == 0 || !race_table[race].pc_race) {
            write_to_buffer(d, "That is not a valid race.\n\r", 0);
            write_to_buffer(d, "The following races are available:\n\r  ", 0);
            for (race = 1; race_table[race].name != NULL; race++) {
                if (!race_table[race].pc_race) break;
                write_to_buffer(d, race_table[race].name, 0);
                write_to_buffer(d, " ", 1);
            }
            write_to_buffer(d, "\n\r", 0);
            write_to_buffer(
                d, "What is your race? (help for more information) ", 0);
            break;
        }

        ch->race = race;
        /* initialize stats */
        for (i = 0; i < MAX_STATS; i++)
            ch->perm_stat[i] = pc_race_table[race].stats[i];
        ch->affected_by = ch->affected_by | race_table[race].aff;
        ch->imm_flags = ch->imm_flags | race_table[race].imm;
        ch->res_flags = ch->res_flags | race_table[race].res;
        ch->vuln_flags = ch->vuln_flags | race_table[race].vuln;
        ch->form = race_table[race].form;
        ch->parts = race_table[race].parts;

        /* add skills */
        for (i = 0; i < 5; i++) {
            if (pc_race_table[race].skills[i] == NULL) break;
            group_add(ch, pc_race_table[race].skills[i], false);
        }
        /* add cost */
        ch->pcdata->points = pc_race_table[race].points;
        ch->size = pc_race_table[race].size;

        write_to_buffer(d, "What is your sex (M/F)? ", 0);
        d->connected = CON_GET_NEW_SEX;
        break;

    case CON_GET_NEW_SEX:
        switch (argument[0]) {
        case 'm':
        case 'M':
            ch->sex = SEX_MALE;
            ch->pcdata->true_sex = SEX_MALE;
            break;
        case 'f':
        case 'F':
            ch->sex = SEX_FEMALE;
            ch->pcdata->true_sex = SEX_FEMALE;
            break;
        default:
            write_to_buffer(d, "That's not a sex.\n\rWhat IS your sex? ", 0);
            return;
        }

        strcpy(buf, "Select a class [");
        for (iClass = 0; iClass < MAX_CLASS; iClass++) {
            if (iClass > 0) strcat(buf, " ");
            strcat(buf, class_table[iClass].name);
        }
        strcat(buf, "]: ");
        write_to_buffer(d, buf, 0);
        d->connected = CON_GET_NEW_CLASS;
        break;

    case CON_GET_NEW_CLASS:
        iClass = class_lookup(argument);

        if (iClass == -1) {
            write_to_buffer(d, "That's not a class.\n\rWhat IS your class? ",
                0);
            return;
        }

        ch->ch_class = iClass;

        sprintf(log_buf, "%s@%s new player.", ch->name, d->host);
        log_string(log_buf);
        wiznet("Newbie alert!  $N sighted.", ch, NULL, WIZ_NEWBIE, 0, 0);
        wiznet(log_buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

        write_to_buffer(d, "\n\r", 2);
        write_to_buffer(d, "You may be good, neutral, or evil.\n\r", 0);
        write_to_buffer(d, "Which alignment (G/N/E)? ", 0);
        d->connected = CON_GET_ALIGNMENT;
        break;

    case CON_GET_ALIGNMENT:
        switch (argument[0]) {
        case 'g':
        case 'G':
            ch->alignment = 750;
            break;
        case 'n':
        case 'N':
            ch->alignment = 0;
            break;
        case 'e':
        case 'E':
            ch->alignment = -750;
            break;
        default:
            write_to_buffer(d, "That's not a valid alignment.\n\r", 0);
            write_to_buffer(d, "Which alignment (G/N/E)? ", 0);
            return;
        }

        write_to_buffer(d, "\n\r", 0);

        group_add(ch, "rom basics", false);
        group_add(ch, class_table[ch->ch_class].base_group, false);
        ch->pcdata->learned[gsn_recall] = 50;
        write_to_buffer(d, "Do you wish to customize this character?\n\r", 0);
        write_to_buffer(d,
            "Customization takes time, but allows a wider range of "
            "skills and abilities.\n\r",
            0);
        write_to_buffer(d, "Customize (Y/N)? ", 0);
        d->connected = CON_DEFAULT_CHOICE;
        break;

    case CON_DEFAULT_CHOICE:
        write_to_buffer(d, "\n\r", 2);
        switch (argument[0]) {
        case 'y':
        case 'Y':
            ch->gen_data = new_gen_data();
            ch->gen_data->points_chosen = ch->pcdata->points;
            do_function(ch, &do_help, "group header");
            list_group_costs(ch);
            write_to_buffer(d, "You already have the following skills:\n\r", 0);
            do_function(ch, &do_skills, "");
            do_function(ch, &do_help, "menu choice");
            d->connected = CON_GEN_GROUPS;
            break;
        case 'n':
        case 'N':
            group_add(ch, class_table[ch->ch_class].default_group, true);
            write_to_buffer(d, "\n\r", 2);
            write_to_buffer(
                d, "Please pick a weapon from the following choices:\n\r", 0);
            buf[0] = '\0';
            for (i = 0; weapon_table[i].name != NULL; i++)
                if (ch->pcdata->learned[*weapon_table[i].gsn] > 0) {
                    strcat(buf, weapon_table[i].name);
                    strcat(buf, " ");
                }
            strcat(buf, "\n\rYour choice? ");
            write_to_buffer(d, buf, 0);
            d->connected = CON_PICK_WEAPON;
            break;
        default:
            write_to_buffer(d, "Please answer (Y/N)? ", 0);
            return;
        }
        break;

    case CON_PICK_WEAPON:
        write_to_buffer(d, "\n\r", 2);
        weapon = weapon_lookup(argument);
        if (weapon == -1
            || ch->pcdata->learned[*weapon_table[weapon].gsn] <= 0) {
            write_to_buffer(d, "That's not a valid selection. Choices are:\n\r",
                0);
            buf[0] = '\0';
            for (i = 0; weapon_table[i].name != NULL; i++)
                if (ch->pcdata->learned[*weapon_table[i].gsn] > 0) {
                    strcat(buf, weapon_table[i].name);
                    strcat(buf, " ");
                }
            strcat(buf, "\n\rYour choice? ");
            write_to_buffer(d, buf, 0);
            return;
        }

        ch->pcdata->learned[*weapon_table[weapon].gsn] = 40;
        write_to_buffer(d, "\n\r", 2);
        do_function(ch, &do_help, "motd");
        d->connected = CON_READ_MOTD;
        break;

    case CON_GEN_GROUPS:
        send_to_char("\n\r", ch);

        if (!str_cmp(argument, "done")) {
            if (ch->pcdata->points == pc_race_table[ch->race].points) {
                send_to_char("You didn't pick anything.\n\r", ch);
                break;
            }

            if (ch->pcdata->points <= 40 + pc_race_table[ch->race].points) {
                sprintf(buf,
                    "You must take at least %d points of skills and groups",
                    40 + pc_race_table[ch->race].points);
                send_to_char(buf, ch);
                break;
            }

            sprintf(buf, "Creation points: %d\n\r", ch->pcdata->points);
            send_to_char(buf, ch);
            sprintf(buf, "Experience per level: %d\n\r",
                exp_per_level(ch, ch->gen_data->points_chosen));
            if (ch->pcdata->points < 40)
                ch->train = (40 - ch->pcdata->points + 1) / 2;
            free_gen_data(ch->gen_data);
            ch->gen_data = NULL;
            send_to_char(buf, ch);
            write_to_buffer(d, "\n\r", 2);
            write_to_buffer(
                d, "Please pick a weapon from the following choices:\n\r", 0);
            buf[0] = '\0';
            for (i = 0; weapon_table[i].name != NULL; i++)
                if (ch->pcdata->learned[*weapon_table[i].gsn] > 0) {
                    strcat(buf, weapon_table[i].name);
                    strcat(buf, " ");
                }
            strcat(buf, "\n\rYour choice? ");
            write_to_buffer(d, buf, 0);
            d->connected = CON_PICK_WEAPON;
            break;
        }

        if (!parse_gen_groups(ch, argument))
            send_to_char(
                "Choices are: list,learned,premise,add,drop,info,help, and "
                "done.\n\r",
                ch);

        do_function(ch, &do_help, "menu choice");
        break;

    case CON_READ_IMOTD:
        write_to_buffer(d, "\n\r", 2);
        do_function(ch, &do_help, "motd");
        d->connected = CON_READ_MOTD;
        break;

    case CON_READ_MOTD:
        if (ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0') {
            write_to_buffer(d, "Warning! Null password!\n\r", 0);
            write_to_buffer(d, "Please report old password with bug.\n\r", 0);
            write_to_buffer(
                d, "Type 'password null <new password>' to fix.\n\r", 0);
        }

        write_to_buffer(
            d, "\n\rWelcome to ROM 2.4.  Please do not feed the mobiles.\n\r",
            0);
        ch->next = char_list;
        char_list = ch;
        d->connected = CON_PLAYING;
        reset_char(ch);

        if (ch->level == 0) {
            ch->perm_stat[class_table[ch->ch_class].attr_prime] += 3;

            ch->level = 1;
            ch->exp = exp_per_level(ch, ch->pcdata->points);
            ch->hit = ch->max_hit;
            ch->mana = ch->max_mana;
            ch->move = ch->max_move;
            ch->train = 3;
            ch->practice = 5;
            sprintf(buf, "the %s",
                title_table[ch->ch_class][ch->level]
                [ch->sex == SEX_FEMALE ? 1 : 0]);
            set_title(ch, buf);

            do_function(ch, &do_outfit, "");
            obj_to_char(create_object(get_obj_index(OBJ_VNUM_MAP), 0), ch);

            char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL));
            send_to_char("\n\r", ch);
            do_function(ch, &do_help, "newbie info");
            send_to_char("\n\r", ch);
        }
        else if (ch->in_room != NULL) {
            char_to_room(ch, ch->in_room);
        }
        else if (IS_IMMORTAL(ch)) {
            char_to_room(ch, get_room_index(ROOM_VNUM_CHAT));
        }
        else {
            char_to_room(ch, get_room_index(ROOM_VNUM_TEMPLE));
        }

        act("$n has entered the game.", ch, NULL, NULL, TO_ROOM);
        do_function(ch, &do_look, "auto");

        wiznet("$N has left real life behind.", ch, NULL, WIZ_LOGINS, WIZ_SITES,
            get_trust(ch));

        if (ch->pet != NULL) {
            char_to_room(ch->pet, ch->in_room);
            act("$n has entered the game.", ch->pet, NULL, NULL, TO_ROOM);
        }

        do_function(ch, &do_unread, "");
        break;
    }

    return;
}

void poll_server(SockServer* server, PollData* poll_data)
{
    static struct timeval null_time = { 0 };

    // Poll all active descriptors.
    FD_ZERO(&poll_data->in_set);
    FD_ZERO(&poll_data->out_set);
    FD_ZERO(&poll_data->exc_set);
    FD_SET(server->control, &poll_data->in_set);
    poll_data->maxdesc = server->control;
    for (DESCRIPTOR_DATA* d = descriptor_list; d; d = d->next) {
        poll_data->maxdesc = UMAX(poll_data->maxdesc, d->client.fd);
        FD_SET(d->client.fd, &poll_data->in_set);
        FD_SET(d->client.fd, &poll_data->out_set);
        FD_SET(d->client.fd, &poll_data->exc_set);
    }

    if (select((int)poll_data->maxdesc + 1, &poll_data->in_set, 
            &poll_data->out_set, &poll_data->exc_set, &null_time) < 0) {
        perror("poll_server");
#ifdef _MSC_VER
        PrintLastWinSockError();
#endif
        close_server(server);
        exit(1);
    }
}

/*
 * Parse a name for acceptability.
 */
bool check_parse_name(char* name)
{
    int clan;

    /*
     * Reserved words.
     */
    if (is_exact_name(
        name, "all auto immortal self someone something the you loner")) {
        return false;
    }

    /* check clans */
    for (clan = 0; clan < MAX_CLAN; clan++) {
        if (LOWER(name[0]) == LOWER(clan_table[clan].name[0])
            && !str_cmp(name, clan_table[clan].name))
            return false;
    }

    if (str_cmp(capitalize(name), "Alander")
        && (!str_prefix("Alan", name) || !str_suffix("Alander", name)))
        return false;

    /*
     * Length restrictions.
     */
    if (strlen(name) < 2) return false;
    if (strlen(name) > 12) return false;

    /*
     * Alphanumerics only.
     * Lock out IllIll twits.
     */
    {
        char* pc;
        bool fIll, adjcaps = false, cleancaps = false;
        int total_caps = 0;

        fIll = true;
        for (pc = name; *pc != '\0'; pc++) {
            if (!ISALPHA(*pc)) return false;

            if (ISUPPER(*pc)) /* ugly anti-caps hack */
            {
                if (adjcaps) cleancaps = true;
                total_caps++;
                adjcaps = true;
            }
            else
                adjcaps = false;

            if (LOWER(*pc) != 'i' && LOWER(*pc) != 'l') fIll = false;
        }

        if (fIll) return false;

        if (cleancaps || (total_caps > (strlen(name)) / 2 && strlen(name) < 3))
            return false;
    }

    /*
     * Prevent players from naming themselves after mobs.
     */
    {
        extern MOB_INDEX_DATA* mob_index_hash[MAX_KEY_HASH];
        MOB_INDEX_DATA* pMobIndex;
        int iHash;

        for (iHash = 0; iHash < MAX_KEY_HASH; iHash++) {
            for (pMobIndex = mob_index_hash[iHash]; pMobIndex != NULL;
                pMobIndex = pMobIndex->next) {
                if (is_name(name, pMobIndex->player_name)) return false;
            }
        }
    }

    return true;
}

/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect(DESCRIPTOR_DATA* d, char* name, bool fConn)
{
    CHAR_DATA* ch;

    for (ch = char_list; ch != NULL; ch = ch->next) {
        if (!IS_NPC(ch) && (!fConn || ch->desc == NULL)
            && !str_cmp(d->character->name, ch->name)) {
            if (fConn == false) {
                free_string(d->character->pcdata->pwd);
                d->character->pcdata->pwd = str_dup(ch->pcdata->pwd);
            }
            else {
                free_char(d->character);
                d->character = ch;
                ch->desc = d;
                ch->timer = 0;
                send_to_char(
                    "Reconnecting. Type replay to see missed tells.\n\r", ch);
                act("$n has reconnected.", ch, NULL, NULL, TO_ROOM);

                sprintf(log_buf, "%s@%s reconnected.", ch->name, d->host);
                log_string(log_buf);
                wiznet("$N groks the fullness of $S link.", ch, NULL, WIZ_LINKS,
                    0, 0);
                d->connected = CON_PLAYING;
            }
            return true;
        }
    }

    return false;
}

/*
 * Check if already playing.
 */
bool check_playing(DESCRIPTOR_DATA* d, char* name)
{
    DESCRIPTOR_DATA* dold;

    for (dold = descriptor_list; dold; dold = dold->next) {
        if (dold != d && dold->character != NULL
            && dold->connected != CON_GET_NAME
            && dold->connected != CON_GET_OLD_PASSWORD
            && !str_cmp(name, dold->original ? dold->original->name
                : dold->character->name)) {
            write_to_buffer(d, "That character is already playing.\n\r", 0);
            write_to_buffer(d, "Do you wish to connect anyway (Y/N)?", 0);
            d->connected = CON_BREAK_CONNECT;
            return true;
        }
    }

    return false;
}

void stop_idling(CHAR_DATA* ch)
{
    if (ch == NULL || ch->desc == NULL || ch->desc->connected != CON_PLAYING
        || ch->was_in_room == NULL
        || ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
        return;

    ch->timer = 0;
    char_from_room(ch);
    char_to_room(ch, ch->was_in_room);
    ch->was_in_room = NULL;
    act("$n has returned from the void.", ch, NULL, NULL, TO_ROOM);
    return;
}

void process_client_output(PollData* poll_data)
{
    for (DESCRIPTOR_DATA* d = descriptor_list; d != NULL; d = d_next) {
        d_next = d->next;

        if ((d->fcommand || d->outtop > 0)
            && can_write(d, poll_data)) {
            if (!process_descriptor_output(d, true)) {
                if (d->character != NULL && d->connected == CON_PLAYING)
                    save_char_obj(d->character);
                d->outtop = 0;
                close_socket(d);
            }
        }
    }
}

/*
 * Write to one char.
 */
void send_to_char_bw(const char* txt, CHAR_DATA* ch)
{
    if (txt != NULL && ch->desc != NULL)
        write_to_buffer(ch->desc, txt, strlen(txt));
    return;
}

/*
 * Write to one char, new colour version, by Lope.
 */
void send_to_char(const char* txt, CHAR_DATA* ch)
{
    const char* point;
    char* point2;
    INIT_BUF(temp, MAX_STRING_LENGTH * 4);
    size_t skip = 0;

    BUF(temp)[0] = '\0';
    point2 = BUF(temp);
    if (txt && ch->desc) {
        if (IS_SET(ch->act, PLR_COLOUR)) {
            for (point = txt; *point; point++) {
                if (*point == '{') {
                    point++;
                    skip = colour(*point, ch, point2);
                    while (skip-- > 0) ++point2;
                    continue;
                }
                *point2 = *point;
                *++point2 = '\0';
            }
            *point2 = '\0';
            write_to_buffer(ch->desc, BUF(temp), point2 - BUF(temp));
        }
        else {
            for (point = txt; *point; point++) {
                if (*point == '{') {
                    point++;
                    continue;
                }
                *point2 = *point;
                *++point2 = '\0';
            }
            *point2 = '\0';
            write_to_buffer(ch->desc, BUF(temp), point2 - BUF(temp));
        }
    }

    free_buf(temp);
}

/*
 * Send a page to one char.
 */
void page_to_char_bw(const char* txt, CHAR_DATA* ch)
{
    if (txt == NULL || ch->desc == NULL) return;

    if (ch->lines == 0) {
        send_to_char_bw(txt, ch);
        return;
    }

    ch->desc->showstr_head = alloc_mem(strlen(txt) + 1);
    strcpy(ch->desc->showstr_head, txt);
    ch->desc->showstr_point = ch->desc->showstr_head;
    show_string(ch->desc, "");
}

/*
 * Page to one char, new colour version, by Lope.
 */
void page_to_char(const char* txt, CHAR_DATA* ch)
{
    INIT_BUF(temp, MAX_STRING_LENGTH * 4);
    const char* point;
    char* point2;
    size_t skip = 0;

    BUF(temp)[0] = '\0';
    point2 = BUF(temp);
    if (txt && ch->desc) {
        if (IS_SET(ch->act, PLR_COLOUR)) {
            for (point = txt; *point; point++) {
                if (*point == '{') {
                    point++;
                    skip = colour(*point, ch, point2);
                    while (skip-- > 0) ++point2;
                    continue;
                }
                *point2 = *point;
                *++point2 = '\0';
            }
            *point2 = '\0';
            ch->desc->showstr_head = alloc_mem(strlen(BUF(temp)) + 1);
            strcpy(ch->desc->showstr_head, BUF(temp));
            ch->desc->showstr_point = ch->desc->showstr_head;
            show_string(ch->desc, "");
        }
        else {
            for (point = txt; *point; point++) {
                if (*point == '{') {
                    point++;
                    continue;
                }
                *point2 = *point;
                *++point2 = '\0';
            }
            *point2 = '\0';
            ch->desc->showstr_head = alloc_mem(strlen(BUF(temp)) + 1);
            strcpy(ch->desc->showstr_head, BUF(temp));
            ch->desc->showstr_point = ch->desc->showstr_head;
            show_string(ch->desc, "");
        }
    }

    free_buf(temp);
}

/* string pager */
void show_string(struct descriptor_data* d, char* input)
{
    INIT_BUF(temp1, MAX_STRING_LENGTH * 4);
    INIT_BUF(temp2, MAX_STRING_LENGTH);
    register char* scan;
    register char* chk;
    int lines = 0, toggle = 1;
    int show_lines;

    one_argument(input, BUF(temp2));
    if (BUF(temp2)[0] != '\0') {
        if (d->showstr_head) {
            free_mem(d->showstr_head, strlen(d->showstr_head));
            d->showstr_head = 0;
        }
        d->showstr_point = 0;
        goto show_string_cleanup;
    }

    if (d->character)
        show_lines = d->character->lines;
    else
        show_lines = 0;

    for (scan = BUF(temp1);; scan++, d->showstr_point++) {
        if (((*scan = *d->showstr_point) == '\n' || *scan == '\r')
            && (toggle = -toggle) < 0)
            lines++;

        else if (!*scan || (show_lines > 0 && lines >= show_lines)) {
            *scan = '\0';
            write_to_buffer(d, BUF(temp1), strlen(BUF(temp1)));
            for (chk = d->showstr_point; ISSPACE(*chk); chk++)
                ;
            {
                if (!*chk) {
                    if (d->showstr_head) {
                        free_mem(d->showstr_head, strlen(d->showstr_head));
                        d->showstr_head = 0;
                    }
                    d->showstr_point = 0;
                }
            }
            goto show_string_cleanup;
        }
    }

show_string_cleanup:
    free_buf(temp1);
    free_buf(temp2);
}

/* quick sex fixer */
void fix_sex(CHAR_DATA* ch)
{
    if (ch->sex < 0 || ch->sex > 2)
        ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
}

void act_new(const char* format, CHAR_DATA* ch, const void* arg1,
    const void* arg2, int type, int min_pos)
{
    static char* const he_she[] = {"it", "he", "she"};
    static char* const him_her[] = {"it", "him", "her"};
    static char* const his_her[] = {"its", "his", "her"};

    CHAR_DATA* to;
    CHAR_DATA* vch = (CHAR_DATA*)arg2;
    OBJ_DATA* obj1 = (OBJ_DATA*)arg1;
    OBJ_DATA* obj2 = (OBJ_DATA*)arg2;
    const char* str;
    char* i = NULL;
    char* point;
    char* pbuff;
    char buffer[MAX_STRING_LENGTH * 2] = "";
    char buf[MAX_STRING_LENGTH] = "";
    char fname[MAX_INPUT_LENGTH] = "";

    /*
     * Discard null and zero-length messages.
     */
    if (!format || !*format) return;

    /* discard null rooms and chars */
    if (!ch || !ch->in_room) return;

    to = ch->in_room->people;
    if (type == TO_VICT) {
        if (!vch) {
            bug("Act: null vch with TO_VICT.", 0);
            return;
        }

        if (!vch->in_room) return;

        to = vch->in_room->people;
    }

    for (; to; to = to->next_in_room) {
        if (!to->desc || to->position < min_pos) continue;

        if ((type == TO_CHAR) && to != ch) continue;
        if (type == TO_VICT && (to != vch || to == ch)) continue;
        if (type == TO_ROOM && to == ch) continue;
        if (type == TO_NOTVICT && (to == ch || to == vch)) continue;

        point = buf;
        str = format;
        while (*str != '\0') {
            if (*str != '$') {
                *point++ = *str++;
                continue;
            }

            ++str;
            i = " <@@@> ";
            if (!arg2 && *str >= 'A' && *str <= 'Z') {
                bug("Act: missing arg2 for code %d.", *str);
                i = " <@@@> ";
            }
            else {
                switch (*str) {
                default:
                    bug("Act: bad code %d.", *str);
                    i = " <@@@> ";
                    break;
                    /* Thx alex for 't' idea */
                case 't':
                    i = (char*)arg1;
                    break;
                case 'T':
                    i = (char*)arg2;
                    break;
                case 'n':
                    i = PERS(ch, to);
                    break;
                case 'N':
                    i = PERS(vch, to);
                    break;
                case 'e':
                    i = he_she[URANGE(0, ch->sex, 2)];
                    break;
                case 'E':
                    i = he_she[URANGE(0, vch->sex, 2)];
                    break;
                case 'm':
                    i = him_her[URANGE(0, ch->sex, 2)];
                    break;
                case 'M':
                    i = him_her[URANGE(0, vch->sex, 2)];
                    break;
                case 's':
                    i = his_her[URANGE(0, ch->sex, 2)];
                    break;
                case 'S':
                    i = his_her[URANGE(0, vch->sex, 2)];
                    break;

                case 'p':
                    i = can_see_obj(to, obj1) ? obj1->short_descr : "something";
                    break;

                case 'P':
                    i = can_see_obj(to, obj2) ? obj2->short_descr : "something";
                    break;

                case 'd':
                    if (arg2 == NULL || ((char*)arg2)[0] == '\0') {
                        i = "door";
                    }
                    else {
                        one_argument((char*)arg2, fname);
                        i = fname;
                    }
                    break;
                }
            }

            ++str;
            while ((*point = *i) != '\0') ++point, ++i;
        }

        *point++ = '\n';
        *point++ = '\r';
        *point = '\0';
        buf[0] = UPPER(buf[0]);
        pbuff = buffer;
        colourconv(pbuff, buf, to);
        write_to_buffer(to->desc, buffer, 0);
    }

    return;
}

size_t colour(char type, CHAR_DATA* ch, char* string)
{
    PC_DATA* col;
    char code[20];

    if (IS_NPC(ch)) return (0);

    col = ch->pcdata;

    switch (type) {
    default:
        strcpy(code, CLEAR);
        break;
    case 'x':
        strcpy(code, CLEAR);
        break;
    case 'p':
        if (col->prompt[2])
            sprintf(code, "\033[%d;3%dm%c", col->prompt[0], col->prompt[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->prompt[0], col->prompt[1]);
        break;
    case 's':
        if (col->room_title[2])
            sprintf(code, "\033[%d;3%dm%c", col->room_title[0],
                col->room_title[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->room_title[0], col->room_title[1]);
        break;
    case 'S':
        if (col->room_text[2])
            sprintf(code, "\033[%d;3%dm%c", col->room_text[0], col->room_text[1],
                '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->room_text[0], col->room_text[1]);
        break;
    case 'd':
        if (col->gossip[2])
            sprintf(code, "\033[%d;3%dm%c", col->gossip[0], col->gossip[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->gossip[0], col->gossip[1]);
        break;
    case '9':
        if (col->gossip_text[2])
            sprintf(code, "\033[%d;3%dm%c", col->gossip_text[0],
                col->gossip_text[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->gossip_text[0],
                col->gossip_text[1]);
        break;
    case 'Z':
        if (col->wiznet[2])
            sprintf(code, "\033[%d;3%dm%c", col->wiznet[0], col->wiznet[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->wiznet[0], col->wiznet[1]);
        break;
    case 'o':
        if (col->room_exits[2])
            sprintf(code, "\033[%d;3%dm%c", col->room_exits[0],
                col->room_exits[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->room_exits[0], col->room_exits[1]);
        break;
    case 'O':
        if (col->room_things[2])
            sprintf(code, "\033[%d;3%dm%c", col->room_things[0],
                col->room_things[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->room_things[0],
                col->room_things[1]);
        break;
    case 'i':
        if (col->immtalk_text[2])
            sprintf(code, "\033[%d;3%dm%c", col->immtalk_text[0],
                col->immtalk_text[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->immtalk_text[0],
                col->immtalk_text[1]);
        break;
    case 'I':
        if (col->immtalk_type[2])
            sprintf(code, "\033[%d;3%dm%c", col->immtalk_type[0],
                col->immtalk_type[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->immtalk_type[0],
                col->immtalk_type[1]);
        break;
    case '2':
        if (col->fight_yhit[2])
            sprintf(code, "\033[%d;3%dm%c", col->fight_yhit[0],
                col->fight_yhit[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->fight_yhit[0], col->fight_yhit[1]);
        break;
    case '3':
        if (col->fight_ohit[2])
            sprintf(code, "\033[%d;3%dm%c", col->fight_ohit[0],
                col->fight_ohit[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->fight_ohit[0], col->fight_ohit[1]);
        break;
    case '4':
        if (col->fight_thit[2])
            sprintf(code, "\033[%d;3%dm%c", col->fight_thit[0],
                col->fight_thit[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->fight_thit[0], col->fight_thit[1]);
        break;
    case '5':
        if (col->fight_skill[2])
            sprintf(code, "\033[%d;3%dm%c", col->fight_skill[0],
                col->fight_skill[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->fight_skill[0],
                col->fight_skill[1]);
        break;
    case '1':
        if (col->fight_death[2])
            sprintf(code, "\033[%d;3%dm%c", col->fight_death[0],
                col->fight_death[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->fight_death[0],
                col->fight_death[1]);
        break;
    case '6':
        if (col->say[2])
            sprintf(code, "\033[%d;3%dm%c", col->say[0], col->say[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->say[0], col->say[1]);
        break;
    case '7':
        if (col->say_text[2])
            sprintf(code, "\033[%d;3%dm%c", col->say_text[0], col->say_text[1],
                '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->say_text[0], col->say_text[1]);
        break;
    case 'k':
        if (col->tell[2])
            sprintf(code, "\033[%d;3%dm%c", col->tell[0], col->tell[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->tell[0], col->tell[1]);
        break;
    case 'K':
        if (col->tell_text[2])
            sprintf(code, "\033[%d;3%dm%c", col->tell_text[0], col->tell_text[1],
                '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->tell_text[0], col->tell_text[1]);
        break;
    case 'l':
        if (col->reply[2])
            sprintf(code, "\033[%d;3%dm%c", col->reply[0], col->reply[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->reply[0], col->reply[1]);
        break;
    case 'L':
        if (col->reply_text[2])
            sprintf(code, "\033[%d;3%dm%c", col->reply_text[0],
                col->reply_text[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->reply_text[0], col->reply_text[1]);
        break;
    case 'n':
        if (col->gtell_text[2])
            sprintf(code, "\033[%d;3%dm%c", col->gtell_text[0],
                col->gtell_text[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->gtell_text[0], col->gtell_text[1]);
        break;
    case 'N':
        if (col->gtell_type[2])
            sprintf(code, "\033[%d;3%dm%c", col->gtell_type[0],
                col->gtell_type[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->gtell_type[0], col->gtell_type[1]);
        break;
    case 'a':
        if (col->auction[2])
            sprintf(code, "\033[%d;3%dm%c", col->auction[0], col->auction[1],
                '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->auction[0], col->auction[1]);
        break;
    case 'A':
        if (col->auction_text[2])
            sprintf(code, "\033[%d;3%dm%c", col->auction_text[0],
                col->auction_text[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->auction_text[0],
                col->auction_text[1]);
        break;
    case 'q':
        if (col->question[2])
            sprintf(code, "\033[%d;3%dm%c", col->question[0], col->question[1],
                '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->question[0], col->question[1]);
        break;
    case 'Q':
        if (col->question_text[2])
            sprintf(code, "\033[%d;3%dm%c", col->question_text[0],
                col->question_text[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->question_text[0],
                col->question_text[1]);
        break;
    case 'f':
        if (col->answer[2])
            sprintf(code, "\033[%d;3%dm%c", col->answer[0], col->answer[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->answer[0], col->answer[1]);
        break;
    case 'F':
        if (col->answer_text[2])
            sprintf(code, "\033[%d;3%dm%c", col->answer_text[0],
                col->answer_text[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->answer_text[0],
                col->answer_text[1]);
        break;
    case 'e':
        if (col->music[2])
            sprintf(code, "\033[%d;3%dm%c", col->music[0], col->music[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->music[0], col->music[1]);
        break;
    case 'E':
        if (col->music_text[2])
            sprintf(code, "\033[%d;3%dm%c", col->music_text[0],
                col->music_text[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->music_text[0], col->music_text[1]);
        break;
    case 'h':
        if (col->quote[2])
            sprintf(code, "\033[%d;3%dm%c", col->quote[0], col->quote[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->quote[0], col->quote[1]);
        break;
    case 'H':
        if (col->quote_text[2])
            sprintf(code, "\033[%d;3%dm%c", col->quote_text[0],
                col->quote_text[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->quote_text[0], col->quote_text[1]);
        break;
    case 'j':
        if (col->info[2])
            sprintf(code, "\033[%d;3%dm%c", col->info[0], col->info[1], '\a');
        else
            sprintf(code, "\033[%d;3%dm", col->info[0], col->info[1]);
        break;
    case 'b':
        strcpy(code, C_BLUE);
        break;
    case 'c':
        strcpy(code, C_CYAN);
        break;
    case 'g':
        strcpy(code, C_GREEN);
        break;
    case 'm':
        strcpy(code, C_MAGENTA);
        break;
    case 'r':
        strcpy(code, C_RED);
        break;
    case 'w':
        strcpy(code, C_WHITE);
        break;
    case 'y':
        strcpy(code, C_YELLOW);
        break;
    case 'B':
        strcpy(code, C_B_BLUE);
        break;
    case 'C':
        strcpy(code, C_B_CYAN);
        break;
    case 'G':
        strcpy(code, C_B_GREEN);
        break;
    case 'M':
        strcpy(code, C_B_MAGENTA);
        break;
    case 'R':
        strcpy(code, C_B_RED);
        break;
    case 'W':
        strcpy(code, C_B_WHITE);
        break;
    case 'Y':
        strcpy(code, C_B_YELLOW);
        break;
    case 'D':
        strcpy(code, C_D_GREY);
        break;
    case '*':
        sprintf(code, "%c", '\a');
        break;
    case '/':
        strcpy(code, "\n\r");
        break;
    case '-':
        sprintf(code, "%c", '~');
        break;
    case '{':
        sprintf(code, "%c", '{');
        break;
    }

    char* p = code;
    while (*p != '\0') {
        *string = *p++;
        *++string = '\0';
    }

    return (strlen(code));
}

void colourconv(char* buffer, const char* txt, CHAR_DATA* ch)
{
    const char* point;
    size_t skip = 0;

    if (ch->desc && txt) {
        if (IS_SET(ch->act, PLR_COLOUR)) {
            for (point = txt; *point; point++) {
                if (*point == '{') {
                    point++;
                    skip = colour(*point, ch, buffer);
                    while (skip-- > 0) ++buffer;
                    continue;
                }
                *buffer = *point;
                *++buffer = '\0';
            }
            *buffer = '\0';
        }
        else {
            for (point = txt; *point; point++) {
                if (*point == '{') {
                    point++;
                    continue;
                }
                *buffer = *point;
                *++buffer = '\0';
            }
            *buffer = '\0';
        }
    }
    return;
}
