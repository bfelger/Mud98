////////////////////////////////////////////////////////////////////////////////
// comm.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__COMM_H
#define MUD98__COMM_H

#define MAX_HANDSHAKES 10

#include "merc.h"

#include "entities/char_data.h"
#include "entities/descriptor.h"

#include "socket.h"

#include <stdint.h>
#include <stdbool.h>

bool can_write(Descriptor* d, PollData* poll_data);
void close_client(SockClient* client);
void close_server(SockServer* server);
bool has_new_conn(SockServer* server, PollData* poll_data);
void handle_new_connection(SockServer* server);
void init_server(SockServer* server, int port);
void nanny(Descriptor* d, char* argument);
void poll_server(SockServer* server, PollData* poll_data);
void process_client_input(SockServer* server, PollData* poll_data);
void process_client_output(PollData* poll_data);
bool process_descriptor_output(Descriptor* d, bool fPrompt);
void read_from_buffer(Descriptor* d);
bool read_from_descriptor(Descriptor* d);
void stop_idling(CharData* ch);
bool write_to_descriptor(Descriptor* d, char* txt, size_t length);
void show_string(Descriptor* d, char* input);
void close_socket(Descriptor* dclose);
void write_to_buffer(Descriptor* d, const char* txt, size_t length);
void send_to_char(const char* txt, CharData* ch);
void page_to_char(const char* txt, CharData* ch);

#define act(format, ch, arg1, arg2, type)                                      \
    act_new((format), (ch), (arg1), (arg2), (type), POS_RESTING)
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

#endif // MUD98__COMM_H
