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

#include "comm.h"

#include "act_info.h"
#include "alias.h"
#include "act_move.h"
#include "act_wiz.h"
#include "ban.h"
#include "color.h"
#include "config.h"
#include "db.h"
#include "digest.h"
#include "handler.h"
#include "interp.h"
#include "lookup.h"
#include "mob_prog.h"
#include "note.h"
#include "recycle.h"
#include "save.h"
#include "skills.h"
#include "stringutils.h"
#include "tables.h"
#include "telnet.h"
#include "vt.h"

#include <olc/olc.h>
#include <olc/screen.h>
#include <olc/string_edit.h>

#include <entities/area.h>
#include <entities/descriptor.h>
#include <entities/object.h>
#include <entities/player_data.h>

#include <data/class.h>
#include <data/mobile_data.h>
#include <data/player.h>
#include <data/race.h>

#include <lox/lox.h>
#include <lox/object.h>
#include <lox/vm.h>

#include <mth/mth.h>

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifndef NO_OPENSSL
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#endif

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#include <windows.h>
#include <winsock.h>
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
#endif

const unsigned char echo_off_str[4] = { IAC, WILL, TELOPT_ECHO, 0 };
const unsigned char echo_on_str[4] = { IAC, WONT, TELOPT_ECHO, 0 };
const unsigned char go_ahead_str[3] = { IAC, GA, 0 };

Descriptor* d_next;     // Next descriptor in loop

typedef enum {
    THREAD_STATUS_NONE,
    THREAD_STATUS_STARTED,
    THREAD_STATUS_RUNNING,
    THREAD_STATUS_FINISHED,
} ThreadStatus;

#ifdef _MSC_VER
#define INIT_DESC_RET DWORD WINAPI
#define INIT_DESC_PARAM LPVOID
#define THREAD_ERR (DWORD)-1
#define THREAD_RET_T DWORD
#else
#define INIT_DESC_RET void*
#define INIT_DESC_PARAM void*
#define THREAD_ERR (void*)-1
#define THREAD_RET_T void*
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

void bust_a_prompt(Mobile* ch);
bool check_parse_name(char* name);
bool check_playing(Descriptor* d, char* name);
bool check_reconnect(Descriptor* d, bool fConn);
void send_to_desc(const char* txt, Descriptor* desc);

extern bool test_output_enabled;

static int running_servers = 0;

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

    printf("Error: %s\n", msgbuf);
}
#endif

#ifndef NO_OPENSSL
void init_tls_server(TlsServer* server)
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
        printf("* Context created.\n");
    }

    char cert_file[256];
    sprintf(cert_file, "%s%s", cfg_get_keys_dir(), cfg_get_cert_file());

    char pkey_file[256];
    sprintf(pkey_file, "%s%s", cfg_get_keys_dir(), cfg_get_pkey_file());

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(server->ssl_ctx, cert_file,
        SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "! Failed to open SSL certificate file %s\n", cert_file);
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    else {
        printf("* Certificate %s loaded.\n", cert_file);
    }

    if (SSL_CTX_use_PrivateKey_file(server->ssl_ctx, pkey_file,
        SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "! Failed to open SSL private key file %s\n", pkey_file);
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    else {
        printf("* Private key %s loaded.\n", pkey_file);
    }

    if (SSL_CTX_check_private_key(server->ssl_ctx) <= 0) {
        perror("! Certificate/pkey validation failed.");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    else {
        printf("* Cert/pkey validated.\n");
    }

    // Only allow TLS
    SSL_CTX_set_options(server->ssl_ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2
        | SSL_OP_NO_SSLv3);
}
#endif

void init_server(SockServer* server, int port)
{
    struct sockaddr_in sa;
    int x = 1;

#ifndef NO_OPENSSL
    if (server->type == SOCK_TLS)
        init_tls_server((TlsServer*)server);
#endif

    if (running_servers++ == 0) {
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
    }

#ifdef _MSC_VER
    server->control = socket(AF_INET, SOCK_STREAM, 0);
#else
    if ((server->control = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Init_socket: socket");
        exit(1);
    }
#endif

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

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((u_short)port);

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

bool can_write(Descriptor* d, PollData* poll_data)
{
    return FD_ISSET(d->client->fd, &poll_data->out_set);
}

void close_server(SockServer* server)
{
    CLOSE_SOCKET(server->control);

#ifndef NO_OPENSSL
    if (server->type == SOCK_TLS) {
        TlsServer* tls = (TlsServer*)server;
        SSL_CTX_free(tls->ssl_ctx);
    }
#endif

    --running_servers;
#ifdef _MSC_VER
    if (running_servers == 0)
        WSACleanup();
#endif
}

void close_client(SockClient* client)
{
#ifndef NO_OPENSSL
    if (client->type == SOCK_TLS) {
        TlsClient* tls = (TlsClient*)client;
        if (tls->ssl) {
            SSL_shutdown(tls->ssl);
            SSL_free(tls->ssl);
        }
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
    Descriptor* dnew;
    struct sockaddr_in sock = { 0 };
    struct hostent* from;
    SOCKLEN size;
    THREAD_RET_T rc = THREAD_ERR;

    ThreadData* data = (ThreadData*)lp_data;
    SockServer* server = data->server;

    if (server == NULL) {
        perror("New_descriptor: null server");
        goto init_descriptor_finish;
    }

    SockClient* client = NULL;

#ifndef NO_OPENSSL
    TlsClient* tls_client = NULL;
    TlsServer* tls_server = (server->type == SOCK_TLS) ? (TlsServer*)server : NULL;

    if (tls_server) {
        tls_client = (TlsClient*)alloc_mem(sizeof(TlsClient));
        tls_client->type = SOCK_TLS;
        client = (SockClient*)tls_client;
    }
    else {
        client = (SockClient*)alloc_mem(sizeof(SockClient));
        client->type = SOCK_TELNET;
    }
#else
    client = (SockClient*)alloc_mem(sizeof(SockClient));
    client->type = SOCK_TELNET;
#endif

    if (client == NULL) {
        perror("New_descriptor: null client");
        goto init_descriptor_finish;
    }

    size = sizeof(sock);
    getsockname(server->control, (struct sockaddr*)&sock, &size);

    new_conn_threads[thread_data->index].status = THREAD_STATUS_RUNNING;
#ifdef _MSC_VER
    client->fd = accept(server->control, (struct sockaddr*)&sock, &size);
#else
    if ((client->fd = accept(server->control, (struct sockaddr*)&sock, &size)) < 0) {
        perror("New_descriptor: accept");
        goto init_descriptor_finish;
    }
#endif

#ifndef NO_OPENSSL
    if (server->type == SOCK_TLS) {
        tls_client->ssl = SSL_new(tls_server->ssl_ctx);
        SSL_set_fd(tls_client->ssl, (int)tls_client->fd);

        if (SSL_accept(tls_client->ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            goto init_descriptor_finish;
        }
    }
#endif

#ifndef _MSC_VER
#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

    if (fcntl(client->fd, F_SETFL, FNDELAY) == -1) {
        perror("New_descriptor: fcntl: FNDELAY");
        goto init_descriptor_finish;
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
    dnew->pEdit = 0;        // OLC
    dnew->pString = NULL;   // OLC
    dnew->editor = 0;       // OLC

    init_mth_socket(dnew);

    size = sizeof(sock);
    if (getpeername(dnew->client->fd, (struct sockaddr*)&sock, &size) < 0) {
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
            (addr >> 8) & 0xFF, (addr) & 0xFF
        );
        from = gethostbyaddr((char*)&sock.sin_addr,
            sizeof(sock.sin_addr), AF_INET);
        dnew->host = str_dup(from ? from->h_name : buf);
        if (from == NULL || from->h_name == NULL || from->h_name[0] == '\0')
            sprintf(log_buf, "New connection: %s", buf);
        else
            sprintf(log_buf, "New connection: %s (%s)", from->h_name, buf);
        log_string(log_buf);
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
        close_client(dnew->client);
        free_descriptor(dnew);
        goto init_descriptor_finish;
    }

    // Init descriptor data.
    dnew->next = descriptor_list;
    descriptor_list = dnew;

    // Send the greeting.
    {
        extern char* help_greeting;
        if (help_greeting[0] == '.')
            send_to_desc(help_greeting + 1, dnew);
        else
            send_to_desc(help_greeting, dnew);
    }

    rc = 0; // OK

init_descriptor_finish:
    new_conn_threads[thread_data->index].status = THREAD_STATUS_FINISHED;

#ifndef _MSC_VER
    pthread_exit(NULL);
#endif

    return rc;
}

void handle_new_connection(SockServer* server)
{
    int new_thread_ix = -1;

    for (int i = 0; i < MAX_HANDSHAKES; i++) {
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

void close_socket(Descriptor* dclose)
{
    if (dclose == NULL) {
        log_string("close_socket: NULL descriptor!");
        return;
    }

    if (descriptor_list == NULL) {
        log_string("close_socket: Descriptor not NULL but descriptor list is?");
        return;
    }

    Mobile* ch;

    if (dclose->outtop > 0)
        process_descriptor_output(dclose, false);

    if (dclose->snoop_by != NULL) {
        write_to_buffer(dclose->snoop_by, "Your victim has left the game.\n\r",
            0);
    }

    {
        Descriptor* d;

        FOR_EACH(d, descriptor_list) {
            if (d->snoop_by == dclose) 
                d->snoop_by = NULL;
        }
    }

    if ((ch = dclose->character) != NULL) {
        sprintf(log_buf, "Closing link to %s.", NAME_STR(ch));
        log_string(log_buf);
        /* cut down on wiznet spam when rebooting */
        if (dclose->connected == CON_PLAYING && !merc_down) {
            act("$n has lost $s link.", ch, NULL, NULL, TO_ROOM);
            wiznet("Net death has claimed $N.", ch, NULL, WIZ_LINKS, 0, 0);
            ch->desc = NULL;
        }
        else {
            free_mobile(dclose->original ? dclose->original : dclose->character);
        }
    }

    if (d_next == dclose)
        NEXT_LINK(d_next);

    if (dclose == descriptor_list) {
        NEXT_LINK(descriptor_list);
    }
    else {
        Descriptor* d;

        for (d = descriptor_list; d && d->next != dclose; NEXT_LINK(d))
            ;
        if (d != NULL)
            d->next = dclose->next;
        else
            bug("Close_socket: dclose not found.", 0);
    }

    uninit_mth_socket(dclose);

    close_client(dclose->client);
    free_descriptor(dclose);
    return;
}

bool read_from_descriptor(Descriptor* d)
{
    char bufin[INPUT_BUFFER_SIZE] = { 0 };

    if (!IS_VALID(d))
        return false;

    /* Hold horses if pending command already. */
    if (d->incomm[0] != '\0')
        return true;

    /* Check for overflow. */
    size_t start = strlen(d->inbuf);
    if (start >= sizeof(d->inbuf) - 10) {
        sprintf(log_buf, "%s input overflow!", d->host);
        log_string(log_buf);
        write_to_descriptor(d, "\n\r*** PUT A LID ON IT!!! ***\n\r", 0);
        return false;
    }

    /* Snarf input. */
    for (;;) {
        size_t s_read = 0;
#ifndef NO_OPENSSL
        if (d->client->type == SOCK_TLS) {
            int ssl_err;
            if ((ssl_err = SSL_read_ex(((TlsClient*)d->client)->ssl, bufin,
                sizeof(bufin) - 10 - start, &s_read)) <= 0) {
                ERR_print_errors_fp(stderr);
                return false;
            }
        }
        else {
#endif
            int i_read;
#ifdef _MSC_VER
            i_read = recv(d->client->fd, bufin,
                (int)(sizeof(bufin) - 10 - start), 0);
#else
            i_read = read(d->client->fd, bufin,
                sizeof(bufin) - 10 - start);
#endif
            if (i_read < 0) {
                perror("Read_from_descriptor");
                return false;
            }
            s_read = (size_t)i_read;
#ifndef NO_OPENSSL
        }
#endif
        if (errno == EWOULDBLOCK)
            break;
        else if (s_read > 0) {
            start += translate_telopts(d, (unsigned char*)bufin, s_read, (unsigned char*)d->inbuf, start);
            if (d->inbuf[start - 1] == '\n' || d->inbuf[start - 1] == '\r')
                break;
        }
        else if (s_read == 0) {
            log_string("EOF encountered on read.");
            d->valid = false;
            return false;
        }
    }

    d->inbuf[start] = '\0';
    return true;
}

void process_client_input(SockServer* server, PollData* poll_data)
{
    SockType type = server->type;
    // Kick out the freaky folks.
    for (Descriptor* d = descriptor_list; d != NULL; d = d_next) {
        d_next = d->next;
        if (d->client->type != type)
            continue;
        if (FD_ISSET(d->client->fd, &poll_data->exc_set)) {
            FD_CLR(d->client->fd, &poll_data->in_set);
            FD_CLR(d->client->fd, &poll_data->out_set);
            if (d->character && d->connected == CON_PLAYING)
                save_char_obj(d->character);
            d->outtop = 0;
            close_socket(d);
        }
    }

    // Process input.
    for (Descriptor* d = descriptor_list; d != NULL; d = d_next) {
        d_next = d->next;
        if (d->client->type != type)
            continue;
        d->fcommand = false;

        if (FD_ISSET(d->client->fd, &poll_data->in_set)) {
            if (d->character != NULL)
                d->character->timer = 0;
            if (!read_from_descriptor(d)) {
                FD_CLR(d->client->fd, &poll_data->out_set);
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
            else if (d->pString)
                string_add(d->character, d->incomm);    // OLC
            else if (d->connected == CON_PLAYING)
                substitute_alias(d, d->incomm);
            else
                nanny(d, d->incomm);

            d->incomm[0] = '\0';
        }
    }
}

// Transfer one line from input buffer to input line.
void read_from_buffer(Descriptor* d)
{
    int i, j, k;

    // Hold horses if pending command already.
    if (d->incomm[0] != '\0')
        return;

    // Look for at least one new line.
    for (i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++) {
        if (d->inbuf[i] == '\0')
            return;
    }

    // Canonical input processing.
    for (i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++) {
        if (k >= MAX_INPUT_LENGTH - 2) {
            write_to_descriptor(d, "Line too long.\n\r", 0);

            /* skip the rest of the line */
            for (; d->inbuf[i] != '\0'; i++) {
                if (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
                    break;
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

    // Finish off the line.
    if (k == 0) 
        d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    // Deal with bozos with #repeat 1000 ...

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

    // Do '!' substitution.
    if (d->incomm[0] == '!')
        strcpy(d->incomm, d->inlast);
    else
        strcpy(d->inlast, d->incomm);

    // Shift the input buffer.
    while (d->inbuf[i] == '\n' || d->inbuf[i] == '\r') i++;
    for (j = 0; (d->inbuf[j] = d->inbuf[i + j]) != '\0'; j++)
        ;
    return;
}

// Low level output function.
bool process_descriptor_output(Descriptor* d, bool fPrompt)
{
    // Bust a prompt.
    if (!merc_down) {
        if (d->showstr_point && *d->showstr_point != '\0')
            write_to_buffer(d, "[Hit Return to continue]\n\r", 0);
        else if (fPrompt && d->connected == CON_PLAYING) {
            if (d->pString)
                write_to_buffer(d, "> ", 2);    // OLC
            else {
                Mobile* ch;
                Mobile* victim;

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
                        IS_NPC(victim) ? victim->short_descr : NAME_STR(victim),
                        wound);
                    buf[0] = UPPER(buf[0]);
                    write_to_buffer(d, buf, 0);
                }

                ch = d->original ? d->original : d->character;
                if (!IS_SET(ch->comm_flags, COMM_COMPACT))
                    write_to_buffer(d, "\n\r", 2);

                if (IS_SET(ch->comm_flags, COMM_PROMPT))
                    bust_a_prompt(d->character);

                if (IS_SET(ch->comm_flags, COMM_TELNET_GA))
                    write_to_buffer(d, (const char*)go_ahead_str, 0);

                if (IS_SET(ch->comm_flags, COMM_OLCX)
                    && d->editor != ED_NONE
                    && d->pString == NULL)
                    UpdateOLCScreen(d);
            }
        }
    }

    // Short-circuit if nothing to write.
    if (d->outtop == 0)
        return true;

    // Snoop-o-rama.
    if (d->snoop_by != NULL) {
        if (d->character != NULL)
            write_to_buffer(d->snoop_by, NAME_STR(d->character), 0);
        write_to_buffer(d->snoop_by, "> ", 2);
        write_to_buffer(d->snoop_by, d->outbuf, d->outtop);
    }

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
void bust_a_prompt(Mobile* ch)
{
    INIT_BUF(temp1, MAX_STRING_LENGTH);
    INIT_BUF(temp2, MAX_STRING_LENGTH);
    INIT_BUF(temp3, MAX_STRING_LENGTH * 2);

    const char* str;
    const char* i;
    char* point;
    char* pbuff;
    char doors[MAX_INPUT_LENGTH] = "";
    RoomExitData* room_exit_data;
    bool found;
    int door;

    point = BUF(temp1);
    str = ch->prompt;
    if (str == NULL || str[0] == '\0') {
        sprintf(BUF(temp1), "{p<%dhp %dm %dmv>{x %s", ch->hit, ch->mana, ch->move,
            ch->prefix);
        send_to_char(BUF(temp1), ch);
        goto bust_a_prompt_cleanup;
    }

    if (IS_SET(ch->comm_flags, COMM_AFK)) {
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
            for (door = 0; door < DIR_MAX; door++) {
                if ((room_exit_data = ch->in_room->data->exit_data[door]) != NULL
                    && room_exit_data->to_room != NULL
                    && (can_see_room(ch, room_exit_data->to_room)
                        || (IS_AFFECTED(ch, AFF_INFRARED)
                            && !IS_AFFECTED(ch, AFF_BLIND)))
                    && !IS_SET(ch->in_room->exit[door]->exit_flags, EX_CLOSED)) {
                    found = true;
                    strcat(doors, dir_list[door].name_abbr);
                }
            }
            if (!found) 
                strcat(BUF(temp1), "none");
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
            sprintf(BUF(temp2), "%d", ch->gold);
            i = BUF(temp2);
            break;
        case 's':
            sprintf(BUF(temp2), "%d", ch->silver);
            i = BUF(temp2);
            break;
        case 'a':
            if (ch->level > 9)
                sprintf(BUF(temp2), "%d", ch->alignment);
            else
                sprintf(BUF(temp2), "%s",
                    IS_GOOD(ch) ? "good"
                    : IS_EVIL(ch) ? "evil"
                    : "neutral");
            i = BUF(temp2);
            break;
        case 'r':
            if (ch->in_room != NULL)
                sprintf(BUF(temp2), "%s",
                    ((!IS_NPC(ch) && IS_SET(ch->act_flags, PLR_HOLYLIGHT))
                        || (!IS_AFFECTED(ch, AFF_BLIND)
                            && !room_is_dark(ch->in_room)))
                    ? NAME_STR(ch->in_room)
                    : "darkness");
            else
                sprintf(BUF(temp2), " ");
            i = BUF(temp2);
            break;
        case 'R':
            if (IS_IMMORTAL(ch) && ch->in_room != NULL)
                sprintf(BUF(temp2), "%d", VNUM_FIELD(ch->in_room));
            else
                sprintf(BUF(temp2), " ");
            i = BUF(temp2);
            break;
        case 'z':
            if (IS_IMMORTAL(ch) && ch->in_room != NULL)
                sprintf(BUF(temp2), "%s", NAME_STR(ch->in_room->area));
            else
                sprintf(BUF(temp2), " ");
            i = BUF(temp2);
            break;
        case '%':
            sprintf(BUF(temp2), "%%");
            i = BUF(temp2);
            break;
        case 'o':
            sprintf(BUF(temp2), "%s", olc_ed_name(ch));
            i = BUF(temp2);
            break;
        case 'O':
            sprintf(BUF(temp2), "%s", olc_ed_vnum(ch));
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
void write_to_buffer(Descriptor* d, const char* txt, size_t length)
{
    // Find length in case caller didn't.
    if (length <= 0) 
        length = strlen(txt);

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

static void nanny_weapon_prompt(Descriptor* d, Mobile* ch)
{
    char buf[MAX_STRING_LENGTH] = "";
    write_to_buffer(d, "\n\r", 2);
    write_to_buffer(d, "Please pick a weapon from the following choices:\n\r", 0);
    // Skip exotic. No one is trained in it.
    for (int i = 1; i < WEAPON_TYPE_COUNT; i++)
        if (ch->pcdata->learned[*weapon_table[i].gsn] > 0) {
            strcat(buf, weapon_table[i].name);
            strcat(buf, " ");
        }
    strcat(buf, "\n\rYour choice? ");
    write_to_buffer(d, buf, 0);
}

// Deal with sockets that haven't logged in yet.
void nanny(Descriptor * d, char* argument)
{
    Descriptor* d_old = NULL;
    Descriptor* d_next_local = NULL;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Mobile* ch;
    int iClass, race, i, weapon;
    bool fOld = false;
    bool file_exists = false;

    while (ISSPACE(*argument))
        argument++;

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

        file_exists = load_char_obj(d, argument);
        ch = d->character;

        if (IS_SET(ch->act_flags, PLR_DENY)) {
            sprintf(log_buf, "Denying access to %s@%s.", argument, d->host);
            log_string(log_buf);
            write_to_buffer(d, "You are denied access.\n\r", 0);
            close_socket(d);
            return;
        }

        if (check_ban(d->host, BAN_PERMIT) && !IS_SET(ch->act_flags, PLR_PERMIT)) {
            write_to_buffer(d, "Your site has been banned from this mud.\n\r", 0);
            close_socket(d);
            return;
        }

        if (check_reconnect(d, false)) {
            fOld = true;
        }
        else {
            if (wizlock && !IS_IMMORTAL(ch)) {
                write_to_buffer(d, "The game is wizlocked.\n\r", 0);
                close_socket(d);
                return;
            }
        }

        if (fOld && file_exists) {
            /* Old player */
            write_to_buffer(d, "Password: ", 0);
            write_to_buffer(d, (const char*)echo_off_str, 0);
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

        if (!validate_password(argument, ch)) {
            write_to_buffer(d, "Wrong password.\n\r", 0);
            close_socket(d);
            return;
        }

        write_to_buffer(d, (const char*)echo_on_str, 0);

        if (check_playing(d, NAME_STR(ch))) 
            return;

        if (check_reconnect(d, true)) 
            return;

        sprintf(log_buf, "%s@%s has connected.", NAME_STR(ch), d->host);
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
            for (d_old = descriptor_list; d_old != NULL; d_old = d_next_local) {
                d_next_local = d_old->next;
                if (d_old == d || d_old->character == NULL) 
                    continue;

                if (!lox_streq(NAME_FIELD(ch), d_old->original 
                    ? NAME_FIELD(d_old->original)
                    : NAME_FIELD(d_old->character)))
                    continue;

                close_socket(d_old);
            }
            if (check_reconnect(d, true)) return;
            write_to_buffer(d, "Reconnect attempt failed.\n\rName: ", 0);
            if (d->character != NULL) {
                free_mobile(d->character);
                d->character = NULL;
            }
            d->connected = CON_GET_NAME;
            break;

        case 'n':
        case 'N':
            write_to_buffer(d, "Name: ", 0);
            if (d->character != NULL) {
                free_mobile(d->character);
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
                NAME_STR(ch), echo_off_str);
            write_to_buffer(d, buf, 0);
            d->connected = CON_GET_NEW_PASSWORD;
            break;

        case 'n':
        case 'N':
            write_to_buffer(d, "Ok, what IS it, then? ", 0);
            free_mobile(d->character);
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
            write_to_buffer(d, "Password must be at least five characters long."
                "\n\rPassword: ", 0);
            return;
        }

        if (!set_password(argument, ch)) {
            perror("nanny: Could not set password.");
            close_socket(d);
            return;
        }

        write_to_buffer(d, "Please retype password: ", 0);
        d->connected = CON_CONFIRM_NEW_PASSWORD;
        break;

    case CON_CONFIRM_NEW_PASSWORD:
        write_to_buffer(d, "\n\r", 2);

        if (!validate_password(argument, ch)) {
            write_to_buffer(d, "Passwords don't match.\n\r"
                "Retype password: ", 0);
            d->connected = CON_GET_NEW_PASSWORD;
            return;
        }

        write_to_buffer(d, (const char*)echo_on_str, 0);
        write_to_buffer(d, "The following races are available:\n\r  ", 0);
        for (race = 1; race_table[race].name != NULL; race++) {
            if (!race_table[race].pc_race) 
                break;
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
            READ_ARG(arg);
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

        ch->race = (int16_t)race;
        /* initialize stats */
        for (i = 0; i < STAT_COUNT; i++)
            ch->perm_stat[i] = race_table[race].stats[i];
        ch->affect_flags = ch->affect_flags | race_table[race].aff;
        ch->imm_flags = ch->imm_flags | race_table[race].imm;
        ch->res_flags = ch->res_flags | race_table[race].res;
        ch->vuln_flags = ch->vuln_flags | race_table[race].vuln;
        ch->form = race_table[race].form;
        ch->parts = race_table[race].parts;

        /* add skills */
        for (i = 0; i < RACE_NUM_SKILLS; i++) {
            if (race_table[race].skills[i] == NULL) 
                break;
            group_add(ch, race_table[race].skills[i], false);
        }
        
        /* add cost */
        ch->pcdata->points = race_table[race].points;
        ch->size = race_table[race].size;

        write_to_buffer(d, "What is your sex (Male/Female/None/Either)? ", 0);
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
        case 'n':
        case 'N':
            ch->sex = SEX_NEUTRAL;
            ch->pcdata->true_sex = SEX_NEUTRAL;
            break;
        case 'e':
        case 'E':
            ch->sex = SEX_EITHER;
            ch->pcdata->true_sex = SEX_EITHER;
            break;
        default:
            write_to_buffer(d, "That's not a sex.\n\rWhat IS your sex? ", 0);
            return;
        }

        strcpy(buf, "Select a class [");
        for (iClass = 0; iClass < class_count; iClass++) {
            if (iClass > 0) 
                strcat(buf, " ");
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

        ch->ch_class = (int16_t)iClass;

        sprintf(log_buf, "%s@%s new player.", NAME_STR(ch), d->host);
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

        if (cfg_get_chargen_custom()) {
            write_to_buffer(d, "Do you wish to customize this character?\n\r", 0);
            write_to_buffer(d,
                "Customization takes time, but allows a wider range of "
                "skills and abilities.\n\r",
                0);
            write_to_buffer(d, "Customize (Y/N)? ", 0);
            d->connected = CON_DEFAULT_CHOICE;
        }
        else {
            group_add(ch, class_table[ch->ch_class].default_group, true);
            nanny_weapon_prompt(d, ch);
            d->connected = CON_PICK_WEAPON;
        }
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
            nanny_weapon_prompt(d, ch);
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
        if (weapon == WEAPON_EXOTIC
            || ch->pcdata->learned[*weapon_table[weapon].gsn] <= 0) {
            write_to_buffer(d, "That's not a valid selection. Choices are:\n\r",
                0);
            buf[0] = '\0';
            for (i = 1; i < WEAPON_TYPE_COUNT; i++)
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
            if (ch->pcdata->points == race_table[ch->race].points) {
                send_to_char("You didn't pick anything.\n\r", ch);
                break;
            }

            if (ch->pcdata->points <= 40 + race_table[ch->race].points) {
                sprintf(buf,
                    "You must take at least %d points of skills and groups",
                    40 + race_table[ch->race].points);
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
            nanny_weapon_prompt(d, ch);
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
        sprintf(buf, "\n\rWelcome to %s. Please do not feed the mobiles.\n\r\n\r", cfg_get_mud_name());
        write_to_buffer(d, buf, 0);

        {
            PlayerData* pc;
            bool logged_in = false;
            FOR_EACH(pc, player_data_list) {
                if (pc == ch->pcdata) {
                    logged_in = true;
                    break;
                }
            }

            if (!logged_in) {
                ch->pcdata->next = player_data_list;
                player_data_list = ch->pcdata;
            }
        }

        d->connected = CON_PLAYING;
        reset_char(ch);


        if (IS_SET(ch->act_flags, PLR_TESTER))
            send_to_char("{*THIS IS A TESTER ACCOUNT.\n\r"
                "It comes with enhanced privileges, and an expectation that you"
                " will not abuse them.{x\n\r", ch);

        if (ch->level == 0) {
            ch->perm_stat[class_table[ch->ch_class].prime_stat] += 3;
            ch->level = 1;
            ch->exp = exp_per_level(ch, ch->pcdata->points);
            ch->hit = ch->max_hit;
            ch->mana = ch->max_mana;
            ch->move = ch->max_move;
            ch->train = 3;
            ch->practice = 5;
            sprintf(buf, "the %s",
                class_table[ch->ch_class].titles[ch->level][ch->sex == SEX_FEMALE ? 1 : 0]);
            set_title(ch, buf);

            do_function(ch, &do_outfit, "");
            obj_to_char(create_object(get_object_prototype(OBJ_VNUM_MAP), 0), ch);

            VNUM start_loc = 0;

            // Do we look at racial start locations?
            if (cfg_get_start_loc_by_race()) {
                Race* p_race = &race_table[ch->race];

                // Do we also look at classes?
                if (cfg_get_start_loc_by_class()) {
                    VNUM race_class_loc = GET_ELEM(&(p_race->class_start), ch->ch_class);
                    if (race_class_loc > 0)
                        start_loc = race_class_loc;
                }

                // Was there a special start location for the race/class combo?
                // If not, load racial default.
                if (start_loc == 0)
                    start_loc = p_race->start_loc;
            }
            else if (cfg_get_start_loc_by_class()) {
                start_loc = class_table[ch->ch_class].start_loc;
            }

            // No race or class start locations were available.
            if (start_loc == 0)
                start_loc = cfg_get_default_start_loc();

            send_to_char("\n\r", ch);
            do_function(ch, &do_help, "newbie info");
            send_to_char("\n\r", ch);

            ch->in_room = get_room_for_player(ch, start_loc);
            mob_to_room(ch, ch->in_room);

            // Fire any TRIG_LOGIN events on this room
            Event* event = get_event_by_trigger((Entity*)ch->in_room, TRIG_LOGIN);

            if (event == NULL) {
                bug("BUG: No TRIG_LOGIN found for room vnum #%"PRVNUM".\n", start_loc);
            }

            if (event) {
                // Get the closure for this event from the room
                ObjClosure* closure = get_event_closure((Entity*)ch->in_room, event);

                if (closure) {
                    // Invoke the closure with the room and character as parameters
                    invoke_method_closure(OBJ_VAL(ch->in_room), closure, 1, OBJ_VAL(ch));
                }
            }
        }
        else if (ch->in_room != NULL) {
            mob_to_room(ch, ch->in_room);
        }
        else if (IS_IMMORTAL(ch)) {
            mob_to_room(ch, get_room(NULL, ROOM_VNUM_CHAT));
        }
        else {
            mob_to_room(ch, get_room(NULL, ch->pcdata->recall));
        }

        act("$n has entered the game.", ch, NULL, NULL, TO_ROOM);
        do_function(ch, &do_look, "auto");

        wiznet("$N has left real life behind.", ch, NULL, WIZ_LOGINS, WIZ_SITES,
            get_trust(ch));

        if (ch->pet != NULL) {
            mob_to_room(ch->pet, ch->in_room);
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

    SockType type = server->type;

    // Poll all active descriptors.
    FD_ZERO(&poll_data->in_set);
    FD_ZERO(&poll_data->out_set);
    FD_ZERO(&poll_data->exc_set);
    FD_SET(server->control, &poll_data->in_set);
    poll_data->maxdesc = server->control;
    for (Descriptor* d = descriptor_list; d; NEXT_LINK(d)) {
        if (d->client->type != type)
            continue;
        poll_data->maxdesc = UMAX(poll_data->maxdesc, d->client->fd);
        FD_SET(d->client->fd, &poll_data->in_set);
        FD_SET(d->client->fd, &poll_data->out_set);
        FD_SET(d->client->fd, &poll_data->exc_set);
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

// Parse a name for acceptability.
bool check_parse_name(char* name)
{
    int clan;

    // Reserved words.
    if (is_exact_name(
        name, "all auto immortal self someone something the you loner none")) {
        return false;
    }

    /* check clans */
    for (clan = 0; clan < MAX_CLAN; clan++) {
        if (LOWER(name[0]) == LOWER(clan_table[clan].name[0])
            && !str_cmp(name, clan_table[clan].name))
            return false;
    }

    int len = (int)strlen(name);

    // Length restrictions.
    if (len < 2 || len > 12) 
        return false;

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

        if (fIll)
            return false;

        if (cleancaps || (total_caps > len / 2 && len < 3))
            return false;
    }

    // Prevent players from naming themselves after mobs.
    {
        MobPrototype* p_mob_proto;
        FOR_EACH_MOB_PROTO(p_mob_proto) {
            if (is_name(name, NAME_STR(p_mob_proto)))
                return false;
        }
    }

    return true;
}

// Look for link-dead player to reconnect.
bool check_reconnect(Descriptor * d, bool fConn)
{
    Mobile* ch;

    FOR_EACH_GLOBAL_MOB(ch) {
        if (!IS_NPC(ch) && (!fConn || ch->desc == NULL)
            && lox_streq(NAME_FIELD(d->character), NAME_FIELD(ch))) {
            if (fConn == false) {
                if (d->character->pcdata->pwd_digest != NULL)
                    free_digest(d->character->pcdata->pwd_digest);
#ifndef NO_OPENSSL
                d->character->pcdata->pwd_digest =
                    (unsigned char*)OPENSSL_memdup(ch->pcdata->pwd_digest,
                        ch->pcdata->pwd_digest_len);
#else
                d->character->pcdata->pwd_digest = (unsigned char*)malloc(
                    ch->pcdata->pwd_digest_len);
                memcpy(d->character->pcdata->pwd_digest, ch->pcdata->pwd_digest, 
                    ch->pcdata->pwd_digest_len);
#endif
                d->character->pcdata->pwd_digest_len =
                    ch->pcdata->pwd_digest_len;
            }
            else {
                free_mobile(d->character);
                d->character = ch;
                ch->desc = d;
                ch->timer = 0;
                send_to_char(
                    "Reconnecting. Type replay to see missed tells.\n\r", ch);
                act("$n has reconnected.", ch, NULL, NULL, TO_ROOM);

                sprintf(log_buf, "%s@%s reconnected.", NAME_STR(ch), d->host);
                log_string(log_buf);
                wiznet("$N groks the fullness of $S link.", ch, NULL,
                    WIZ_LINKS, 0, 0);
                d->connected = CON_PLAYING;
            }
            return true;
        }
    }

    return false;
}

// Check if already playing.
bool check_playing(Descriptor * d, char* name)
{
    Descriptor* dold;

    FOR_EACH(dold, descriptor_list) {
        if (dold != d && dold->character != NULL
            && dold->connected != CON_GET_NAME
            && dold->connected != CON_GET_OLD_PASSWORD
            && !str_cmp(name, dold->original ? NAME_STR(dold->original)
                : NAME_STR(dold->character))) {
            write_to_buffer(d, "That character is already playing.\n\r", 0);
            write_to_buffer(d, "Do you wish to connect anyway (Y/N)?", 0);
            d->connected = CON_BREAK_CONNECT;
            return true;
        }
    }

    return false;
}

void stop_idling(Mobile * ch)
{
    if (ch == NULL || ch->desc == NULL || ch->desc->connected != CON_PLAYING
        || ch->was_in_room == NULL
        || ch->in_room != get_room(NULL, ROOM_VNUM_LIMBO))
        return;

    ch->timer = 0;
    transfer_mob(ch, ch->was_in_room);
    ch->was_in_room = NULL;
    act("$n has returned from the void.", ch, NULL, NULL, TO_ROOM);
}

/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor(Descriptor* d, char* txt, size_t length)
{
    size_t s_bytes = 0;
    int block;

    if (!IS_VALID(d))
        return false;

    if (length <= 0)
        length = strlen(txt);

#ifndef NO_ZLIB
    if (d->mth->mccp2) {
        write_mccp2(d, txt, length);
        return true;
    }
#endif

    for (size_t start = 0; start < length; start += s_bytes) {
        block = (int)UMIN(length - start, 4096);
#ifndef NO_OPENSSL
        int ssl_err;
        if (d->client->type == SOCK_TLS) {
            if ((ssl_err = SSL_write_ex(((TlsClient*)d->client)->ssl, txt + start, block, &s_bytes)) <= 0) {
                ERR_print_errors_fp(stderr);
                return false;
            }
        }
        else {
#endif
            int i_bytes;
#ifdef _MSC_VER
            if ((i_bytes = send(d->client->fd, txt + start, block, 0)) < 0) {
                PrintLastWinSockError();
#else
            if ((i_bytes = write(d->client->fd, txt + start, block)) < 0) {
#endif
                perror("Write_to_descriptor");
                return false;
            }
            s_bytes += i_bytes;
#ifndef NO_OPENSSL
        }
#endif
    }

    return true;
}

void process_client_output(PollData * poll_data, SockType type)
{
    for (Descriptor* d = descriptor_list; d != NULL; d = d_next) {
        d_next = d->next;
        if (d->client->type != type)
            continue;
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

// So we can send the greeting in color
void send_to_desc(const char* txt, Descriptor* desc)
{
    const char* point;
    char* point2;
    char temp[MAX_STRING_LENGTH] = { 0 };
    size_t skip = 0;
    int len = 0;

    temp[0] = '\0';
    point2 = temp;
    if (txt && desc) {
        for (point = txt; *point; point++) {
            if (*point == '{') {
                point++;
                skip = colour(*point, NULL, point2);
                while (skip-- > 0) 
                    ++point2;
                continue;
            }
            *point2 = *point;
            *++point2 = '\0';
            if (len++ >= MAX_STRING_LENGTH - 100 /*arbitrary*/) {
                write_to_descriptor(desc, temp, point2 - temp);
                temp[0] = '\0';
                len = 0;
                point2 = temp;
            }
        }

        if (point2 != temp) {
            write_to_descriptor(desc, temp, point2 - temp);
        }
    };
}


// Write to one char.
void send_to_char_bw(const char* txt, Mobile * ch)
{
    if (txt != NULL && ch->desc != NULL)
        write_to_buffer(ch->desc, txt, strlen(txt));
    return;
}

// Write to one char, new colour version, by Lope.
void send_to_char(const char* txt, Mobile * ch)
{
    const char* point;
    char* point2;
    INIT_BUF(temp, MAX_STRING_LENGTH * 4);
    size_t skip = 0;

    BUF(temp)[0] = '\0';
    point2 = BUF(temp);
    if (txt && ch->desc) {
        if (IS_SET(ch->act_flags, PLR_COLOUR)) {
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

// Send a page to one char.
void page_to_char_bw(const char* txt, Mobile * ch)
{
    if (txt == NULL || ch->desc == NULL) 
        return;

    if (ch->lines == 0) {
        send_to_char_bw(txt, ch);
        return;
    }

    ch->desc->showstr_head = alloc_mem(strlen(txt) + 1);
    strcpy(ch->desc->showstr_head, txt);
    ch->desc->showstr_point = ch->desc->showstr_head;
    show_string(ch->desc, "");
}

// Page to one char, new colour version, by Lope.
void page_to_char(const char* txt, Mobile * ch)
{
    INIT_BUF(temp, MAX_STRING_LENGTH * 4);
    const char* point;
    char* point2;
    size_t skip = 0;

    BUF(temp)[0] = '\0';
    point2 = BUF(temp);
    if (txt && ch->desc) {
        if (IS_SET(ch->act_flags, PLR_COLOUR)) {
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
void show_string(Descriptor* d, char* input)
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
void fix_sex(Mobile * ch)
{
    if (ch->sex < 0 || ch->sex > SEX_PLR_MAX)
        ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
}

void act_pos_new(const char* format, Obj* target, Obj* arg1, Obj* arg2, 
    ActTarget type, Position min_pos)
{
    Mobile* ch = NULL;
    Mobile* to = NULL;
    Mobile* vch = NULL;
    Object* obj1 = NULL;
    Object* obj2 = NULL;
    Room* to_room = NULL;
    char* string1 = "";
    char* string2 = "";

    if (target == NULL)
        goto act_cleanup;

    if (target->type == OBJ_MOB)
        ch = (Mobile*)target;
    else if (target->type == OBJ_ROOM)
        to_room = (Room*)target;

    if (arg1 != NULL) {
        if (arg1->type == OBJ_MOB)
            to = (Mobile*)arg1;
        else if (arg1->type == OBJ_OBJ)
            obj1 = (Object*)arg1;
        else if (arg1->type == OBJ_ROOM)
            to_room = (Room*)arg1;
        else if (arg1->type == OBJ_STRING)
            string1 = ((String*)arg1)->chars;
    }

    if (arg2 != NULL) {
        if (arg2->type == OBJ_MOB)
            vch = (Mobile*)arg2;
        else if (arg2->type == OBJ_OBJ)
            obj2 = (Object*)arg2;
        else if (arg2->type == OBJ_STRING)
            string2 = ((String*)arg2)->chars;
    }

    const char* str;
    const char* i = NULL;
    char* point;
    char* pbuff;
    char buffer[MAX_STRING_LENGTH * 2] = "";
    char buf[MAX_STRING_LENGTH] = "";
    char fname[MAX_INPUT_LENGTH] = "";

    // Discard null and zero-length messages.
    if (!format || !*format)
        goto act_cleanup;

    /* discard null rooms and chars */
    if ((!ch || !ch->in_room) && to_room == NULL)
        goto act_cleanup;

    if (type == TO_VICT) {
        if (!vch) {
            bug("Act: null vch with TO_VICT.", 0);
            goto act_cleanup;
        }

        if (!vch->in_room)
            goto act_cleanup;

        to_room = vch->in_room;
    }
    else if (to_room == NULL) {
        to_room = ch->in_room;
    }

    FOR_EACH_ROOM_MOB(to, to_room) {
        if ((!IS_NPC(to) && to->desc == NULL)
            || (IS_NPC(to) && !HAS_TRIGGER(to, TRIG_ACT) && !HAS_EVENT_TRIGGER(to, TRIG_ACT))
            || to->position < min_pos)
            continue;

        if ((type == TO_CHAR) && to != ch) 
            continue;
        if (type == TO_VICT && (to != vch || to == ch)) 
            continue;
        if (type == TO_ROOM && to == ch) 
            continue;
        if (type == TO_NOTVICT && (to == ch || to == vch)) 
            continue;

        // Display "you" forms to victim.
        int vch_sex = (vch != NULL && to != vch) ? vch->sex : SEX_YOU;

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
                    i = string1;
                    break;
                case 'T':
                    i = string2;
                    break;
                case 'n':
                    i = PERS(ch, to);
                    break;
                case 'N':
                    i = PERS(vch, to);
                    break;
                case 'e':
                    i = sex_table[ch->sex].subj;
                    break;
                case 'E':
                    i = sex_table[vch_sex].subj;
                    break;
                case 'm':
                    i = sex_table[ch->sex].obj;
                    break;
                case 'M':
                    i = sex_table[vch_sex].obj;
                    break;
                case 's':
                    i = sex_table[ch->sex].poss;
                    break;
                case 'S':
                    i = sex_table[vch_sex].poss;
                    break;

                case 'p':
                    i = (obj1 != NULL && can_see_obj(to, obj1)) ? obj1->short_descr : "something";
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
            while ((*point = *i) != '\0') {
                ++point;
                ++i;
            }
        }

        *point++ = '\n';
        *point++ = '\r';
        *point = '\0';
        buf[0] = UPPER(buf[0]);

        if (test_output_enabled) {
            lox_printf("%s", buf);
        }
        else if (to->desc != NULL) {
            pbuff = buffer;
            colourconv(pbuff, buf, to);
            write_to_buffer(to->desc, pbuff, 0);
        }
        else if (events_enabled) {
            mp_act_trigger(buf, to, ch, arg1, arg2, TRIG_ACT);
        }
    }

act_cleanup:
    if (arg1 != NULL && arg1->type == OBJ_STRING)
        pop();
    if (arg2 != NULL && arg2->type == OBJ_STRING)
        pop();
}

size_t colour(char type, Mobile * ch, char* string)
{
    char code[50] = { 0 };
    bool xterm = ch->pcdata->theme_config.xterm;

    if (!ch || IS_NPC(ch) || ch->pcdata->current_theme == NULL)
        return 0;

    int slot = -1;
    int pal = -1;

    ColorTheme* theme = ch->pcdata->current_theme;

    LOOKUP_COLOR_SLOT_CODE(slot, type);

    if (slot < 0) {
        LOOKUP_PALETTE_CODE(pal, type);

        if (pal < 0) {
            switch (type) {
            case '!':
                sprintf(code, "%c", '\a');
                break;
            case '/':
                strcpy(code, "\n\r");
                break;
            case '-':
                sprintf(code, "%c", '~');
                break;
            case '{':
                sprintf(code, "%c", type);
                break;
            case 'x':
                sprintf(code, VT_NORMALT "%s%s",
                    color_to_str(theme, &theme->channels[SLOT_TEXT], xterm),
                    bg_color_to_str(theme, &theme->channels[SLOT_BACKGROUND], xterm)
                );
                break;
            default:
                break;
            }
        }
    }

    if (slot >= 0) {
        sprintf(code, "%s", color_to_str(ch->pcdata->current_theme, 
            &ch->pcdata->current_theme->channels[slot], xterm));
    }
    else if (pal >= 0) {
        sprintf(code, "%s", color_to_str(ch->pcdata->current_theme, 
            &ch->pcdata->current_theme->palette[pal], xterm));
    }

    char* p = code;
    while (*p != '\0') {
        *string++ = *p++;
    }
    *string = '\0';

    return (strlen(code));
}

int colourconv(char* buffer, const char* txt, Mobile * ch)
{
    const char* point;
    size_t skip = 0;

    char* start = buffer;

    if (ch->desc && txt) {
        if (IS_SET(ch->act_flags, PLR_COLOUR)) {
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
    return (int)(buffer - start);
}

// source: EOD, by John Booth <???> 
void printf_to_char(Mobile* ch, const char* fmt, ...)
{
    char buf[MAX_STRING_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    send_to_char(buf, ch);
}

void bugf(char* fmt, ...)
{
    char buf[2 * MSL];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    bug(buf, 0);
}

void printf_log(char* fmt, ...)
{
    char buf[2 * MSL];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    log_string(buf);
}
