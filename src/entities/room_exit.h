////////////////////////////////////////////////////////////////////////////////
// room_exit.h
// Utilities to handle room exits
////////////////////////////////////////////////////////////////////////////////

typedef struct room_exit_t RoomExit;

#pragma once
#ifndef MUD98__ENTITIES__ROOM_EXIT_H
#define MUD98__ENTITIES__ROOM_EXIT_H

#include "merc.h"

#include "room.h"

#include "data/direction.h"

#include <stdint.h>

typedef enum exit_flags_t {
    EX_ISDOOR           = BIT(0),
    EX_CLOSED           = BIT(1),
    EX_LOCKED           = BIT(2),
    EX_PICKPROOF        = BIT(5),
    EX_NOPASS           = BIT(6),
    EX_EASY             = BIT(7),
    EX_HARD             = BIT(8),
    EX_INFURIATING      = BIT(9),
    EX_NOCLOSE          = BIT(10),
    EX_NOLOCK           = BIT(11),
} ExitFlags;

typedef struct room_exit_t {
    RoomExit* next;
    Room* to_room;
    VNUM to_vnum;
    char* keyword;
    char* description;
    Direction orig_dir;
    SHORT_FLAGS exit_reset_flags;
    SHORT_FLAGS exit_flags;
    int16_t key;
} RoomExit;

void free_room_exit(RoomExit* room_exit);
RoomExit* new_room_exit();

#define ADD_EXIT_DESC(t, ed)                                                   \
    if (!t->extra_desc) {                                                      \
        t->extra_desc = ed;                                                    \
    }                                                                          \
    else {                                                                     \
        ExtraDesc* i = t->extra_desc;                                          \
        while (i->next != NULL)                                                \
            NEXT_LINK(i);                                                      \
        i->next = ed;                                                          \
    }                                                                          \
    ed->next = NULL;

extern int room_exit_count;

#endif // !MUD98__ENTITIES__ROOM_EXIT_H
