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

//typedef struct descriptor_data DESCRIPTOR_DATA;

// Descriptor (channel) structure.
typedef struct descriptor_data {
    struct descriptor_data* next;
    struct descriptor_data* snoop_by;
    CHAR_DATA* character;
    CHAR_DATA* original;
    bool valid;
    char* host;
    SOCKET descriptor;
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

void init_descriptor(SOCKET control);
SOCKET init_socket(int port);
void nanny(DESCRIPTOR_DATA* d, char* argument);
bool process_output(DESCRIPTOR_DATA* d, bool fPrompt);
void read_from_buffer(DESCRIPTOR_DATA* d);
bool read_from_descriptor(DESCRIPTOR_DATA* d);
void stop_idling(CHAR_DATA* ch);
bool write_to_descriptor(DESCRIPTOR_DATA* d, char* txt, size_t length);

#ifdef _MSC_VER
void PrintLastWinSockError();
#endif

#endif // ROM__COMM_H
