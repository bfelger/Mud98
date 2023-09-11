////////////////////////////////////////////////////////////////////////////////
// room_data.h
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ENTITIES__ROOM_DATA_H
#define MUD98__ENTITIES__ROOM_DATA_H

typedef struct room_data_t RoomData;

#include "merc.h"

#include "room_data.h"
#include "char_data.h"
#include "exit_data.h"
#include "object_data.h"
#include "reset_data.h"

typedef struct room_data_t {
    RoomData* next;
    CharData* people;
    ObjectData* contents;
    EXTRA_DESCR_DATA* extra_descr;
    AreaData* area;
    ExitData* exit[6];
    ResetData* reset_first;    // OLC
    ResetData* reset_last;     // OLC
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
RoomData* get_room_data(VNUM vnum);
RoomData* new_room_index();

extern RoomData* room_index_hash[MAX_KEY_HASH];

extern int top_room;
extern VNUM top_vnum_room;

#endif // !MUD98__ENTITIES__ROOM_DATA_H
