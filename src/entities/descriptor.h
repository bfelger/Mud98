////////////////////////////////////////////////////////////////////////////////
// descriptor.h
////////////////////////////////////////////////////////////////////////////////

typedef enum connection_state_t ConnectionState;
typedef struct descriptor_t Descriptor;

#pragma once
#ifndef MUD98__ENTITIES__DESCRIPTOR_H
#define MUD98__ENTITIES__DESCRIPTOR_H

#include "merc.h"

#include "socket.h"

#include "char_data.h"

#include <stddef.h>
#include <stdint.h>

typedef enum connection_state_t {
    CON_PLAYING                 = 0,
    CON_GET_NAME                = 1,
    CON_GET_OLD_PASSWORD        = 2,
    CON_CONFIRM_NEW_NAME        = 3,
    CON_GET_NEW_PASSWORD        = 4,
    CON_CONFIRM_NEW_PASSWORD    = 5,
    CON_GET_NEW_RACE            = 6,
    CON_GET_NEW_SEX             = 7,
    CON_GET_NEW_CLASS           = 8,
    CON_GET_ALIGNMENT           = 9,
    CON_DEFAULT_CHOICE          = 10,
    CON_GEN_GROUPS              = 11,
    CON_PICK_WEAPON             = 12,
    CON_READ_IMOTD              = 13,
    CON_READ_MOTD               = 14,
    CON_BREAK_CONNECT           = 15,
} ConnectionState;

typedef struct descriptor_t {
    Descriptor* next;
    Descriptor* snoop_by;
    CharData* character;
    CharData* original;
    char* host;
    SockClient client;
    ConnectionState connected;
    char inbuf[4 * MAX_INPUT_LENGTH];
    char incomm[MAX_INPUT_LENGTH];
    char inlast[MAX_INPUT_LENGTH];
    char* outbuf;
    size_t outsize;
    ptrdiff_t outtop;
    char* showstr_head;
    char* showstr_point;
    char** pString;     // OLC
    uintptr_t pEdit;    // OLC
    char* screenmap;
    char* oldscreenmap;
    int repeat;
    int16_t editor;     // OLC
    int16_t page;
    bool fcommand;
    bool valid;
} Descriptor;

void free_descriptor(Descriptor* d);
Descriptor* new_descriptor();

extern Descriptor* descriptor_list;
extern Descriptor* descriptor_free;

#endif // !MUD98__ENTITIES__DESCRIPTOR_H
