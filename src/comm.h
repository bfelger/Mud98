////////////////////////////////////////////////////////////////////////////////
// comm.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__COMM_H
#define MUD98__COMM_H

#define MAX_HANDSHAKES 10

#include "merc.h"

#include "entities/char_data.h"

#ifndef USE_RAW_SOCKETS
#include <openssl/ssl.h>
#endif

#ifdef _MSC_VER
#include <winsock.h>
#else
#include <sys/select.h>
#define SOCKET int
#endif

typedef struct sock_client_t {
    SOCKET fd;
#ifndef USE_RAW_SOCKETS
    SSL* ssl;
#endif
} SockClient;

typedef struct sock_server_t {
    SOCKET control;
#ifndef USE_RAW_SOCKETS
    SSL_CTX* ssl_ctx;
#endif
} SockServer;

typedef struct poll_data_t {
    fd_set in_set;
    fd_set out_set;
    fd_set exc_set;
    SOCKET maxdesc;
} PollData;

// Descriptor (channel) structure.
typedef struct descriptor_data {
    struct descriptor_data* next;
    struct descriptor_data* snoop_by;
    CharData* character;
    CharData* original;
    bool valid;
    char* host;
    SockClient client;
    int16_t connected;
    bool fcommand;
    char inbuf[4 * MAX_INPUT_LENGTH];
    char incomm[MAX_INPUT_LENGTH];
    char inlast[MAX_INPUT_LENGTH];
    int repeat;
    char* outbuf;
    size_t outsize;
    ptrdiff_t outtop;
    char* showstr_head;
    char* showstr_point;
    uintptr_t pEdit;    // OLC
    char** pString;     // OLC
    int16_t editor;     // OLC
    int16_t page;
    char* screenmap;
    char* oldscreenmap;
} DESCRIPTOR_DATA;

bool can_write(DESCRIPTOR_DATA* d, PollData* poll_data);
void close_client(SockClient* client);
void close_server(SockServer* server);
bool has_new_conn(SockServer* server, PollData* poll_data);
void handle_new_connection(SockServer* server);
void init_server(SockServer* server, int port);
void nanny(DESCRIPTOR_DATA* d, char* argument);
void poll_server(SockServer* server, PollData* poll_data);
void process_client_input(SockServer* server, PollData* poll_data);
void process_client_output(PollData* poll_data);
bool process_descriptor_output(DESCRIPTOR_DATA* d, bool fPrompt);
void read_from_buffer(DESCRIPTOR_DATA* d);
bool read_from_descriptor(DESCRIPTOR_DATA* d);
void stop_idling(CharData* ch);
bool write_to_descriptor(DESCRIPTOR_DATA* d, char* txt, size_t length);
void show_string(struct descriptor_data* d, char* input);
void close_socket(DESCRIPTOR_DATA* dclose);
void write_to_buffer(DESCRIPTOR_DATA* d, const char* txt, size_t length);
void send_to_char(const char* txt, CharData* ch);
void page_to_char(const char* txt, CharData* ch);
void act_new(const char* format, CharData* ch, const void* arg1,
    const void* arg2, int type, int min_pos);
void printf_to_char(CharData*, char*, ...);
void bugf(char*, ...);
void flog(char*, ...);
size_t colour(char type, CharData* ch, char* string);
void colourconv(char* buffer, const char* txt, CharData* ch);
void send_to_char_bw(const char* txt, CharData* ch);
void page_to_char_bw(const char* txt, CharData* ch);

#ifdef _MSC_VER
void PrintLastWinSockError();
#endif

extern DESCRIPTOR_DATA* descriptor_list;

#endif // MUD98__COMM_H
