////////////////////////////////////////////////////////////////////////////////
// exit_data.h
// Utilities to handle room exits
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ENTITIES__ExitData_H
#define MUD98__ENTITIES__ExitData_H

typedef struct exit_data_t ExitData;

#include "merc.h"

#include "room_data.h"

#include <stdint.h>

typedef struct exit_data_t {
    union {
        RoomData* to_room;
        VNUM vnum;
    } u1;
    char* keyword;
    char* description;
    ExitData* next;
    int orig_door;
    int16_t rs_flags;
    int16_t exit_info;
    int16_t key;
} ExitData;

void free_exit(ExitData* pExit);
ExitData* new_exit();

extern int top_exit;

#endif // !MUD98__ENTITIES__ExitData_H
