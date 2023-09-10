////////////////////////////////////////////////////////////////////////////////
// room_data.h
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

typedef struct room_data_t RoomData;

#pragma once
#ifndef MUD98__ENTITIES__ROOM_DATA_H
#define MUD98__ENTITIES__ROOM_DATA_H

#include "merc.h"

#include "char_data.h"
#include "object_data.h"

typedef struct room_data_t {
    RoomData* next;
    CharData* people;
    ObjectData* contents;
    EXTRA_DESCR_DATA* extra_descr;
    AREA_DATA* area;
    EXIT_DATA* exit[6];
    RESET_DATA* reset_first;    // OLC
    RESET_DATA* reset_last;     // OLC
    char* name;
    char* description;
    char* owner;
    VNUM vnum;
    int room_flags;
    int16_t light;
    int16_t sector_type;
    int16_t heal_rate;
    int16_t mana_rate;
    int16_t clan;
    int16_t reset_num;
} RoomData;

void free_room_index(RoomData* pRoom);
RoomData* new_room_index();

#endif // !MUD98__ENTITIES__ROOM_DATA_H
