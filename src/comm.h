////////////////////////////////////////////////////////////////////////////////
// comm.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef ROM__COMM_H
#define ROM__COMM_H

#include "merc.h"

#ifdef _MSC_VER
#include <winsock.h>
#else
#define SOCKET int
#endif

typedef struct sock_server_t {
    SOCKET control;
} SockServer;

typedef struct sock_client_t {
    SOCKET fd;
} SockClient;

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
    CHAR_DATA* character;
    CHAR_DATA* original;
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
} DESCRIPTOR_DATA;

bool can_write(DESCRIPTOR_DATA* d, PollData* poll_data);
void close_server(SockServer* server);
bool has_new_conn(SockServer* server, PollData* poll_data);
void init_descriptor(SockServer* server);
void init_server(SockServer* server, int port);
void nanny(DESCRIPTOR_DATA* d, char* argument);
void poll_server(SockServer* server, PollData* poll_data);
void process_client_input(SockServer* server, PollData* poll_data);
void process_client_output(PollData* poll_data);
bool process_descriptor_output(DESCRIPTOR_DATA* d, bool fPrompt);
void read_from_buffer(DESCRIPTOR_DATA* d);
bool read_from_descriptor(DESCRIPTOR_DATA* d);
void stop_idling(CHAR_DATA* ch);
bool write_to_descriptor(DESCRIPTOR_DATA* d, char* txt, size_t length);

#ifdef _MSC_VER
void PrintLastWinSockError();
#endif

#endif // ROM__COMM_H
