////////////////////////////////////////////////////////////////////////////////
// reset_data.h
// Utilities to handle area resets of mobs and items
////////////////////////////////////////////////////////////////////////////////

typedef struct reset_data_t ResetData;

#pragma once
#ifndef MUD98__ENTITIES__RESET_DATA_H
#define MUD98__ENTITIES__RESET_DATA_H

#include "merc.h"

#include <stdint.h>

/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
typedef struct reset_data_t {
    ResetData* next;
    char command;
    VNUM arg1;
    int16_t arg2;
    VNUM arg3;
    int16_t arg4;
} ResetData;

void free_reset_data(ResetData* pReset);
ResetData* new_reset_data();

//#define ADD_AFF_DATA(t, aff)                                                   \
//    if (!t->affected) {                                                        \
//        t->affected = aff;                                                     \
//    }                                                                          \
//    else {                                                                     \
//        AffectData* i = t->affected;                                           \
//        while (i->next != NULL)                                                \
//            NEXT_LINK(i);                                                       \
//        i->next = aff;                                                         \
//    }                                                                          \
//    aff->next = NULL;

extern int top_reset;

#endif // !MUD98__ENTITIES__RESET_DATA_H
