////////////////////////////////////////////////////////////////////////////////
// exit_data.h
// Utilities to handle room exits
////////////////////////////////////////////////////////////////////////////////

typedef struct exit_data_t ExitData;

#pragma once
#ifndef MUD98__ENTITIES__EXIT_DATA_H
#define MUD98__ENTITIES__EXIT_DATA_H

#include "merc.h"

#include "room_data.h"

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

typedef struct exit_data_t {
    union {
        RoomData* to_room;
        VNUM vnum;
    } u1;
    char* keyword;
    char* description;
    ExitData* next;
    Direction orig_dir;
    SHORT_FLAGS exit_reset_flags;
    SHORT_FLAGS exit_flags;
    int16_t key;
} ExitData;

void free_exit(ExitData* pExit);
ExitData* new_exit();

#define ADD_EXIT_DESC(t, ed)                                                  \
    if (!t->extra_desc) {                                             \
        t->extra_desc = ed;                                           \
    }                                                                          \
    else {                                                                     \
        ExtraDesc* i = t->extra_desc;                                 \
        while (i->next != NULL)                                                \
            i = i->next;                                                       \
        i->next = ed;                                                          \
    }                                                                          \
    ed->next = NULL;

extern int top_exit;

#endif // !MUD98__ENTITIES__EXIT_DATA_H
