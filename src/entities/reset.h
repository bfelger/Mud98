////////////////////////////////////////////////////////////////////////////////
// reset.h
// Utilities to handle area resets of mobs and items
////////////////////////////////////////////////////////////////////////////////

typedef struct reset_t Reset;
typedef struct reset_counter_t ResetCounter;

#pragma once
#ifndef MUD98__ENTITIES__RESET_H
#define MUD98__ENTITIES__RESET_H

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

typedef struct reset_t {
    Reset* next;
    char command;
    VNUM arg1;
    int16_t arg2;
    VNUM arg3;
    int16_t arg4;
} Reset;

typedef struct reset_counter_t {
    ResetCounter* next;
    VNUM vnum;
    int count;
} ResetCounter;

Reset* new_reset();
void free_reset(Reset* reset);

ResetCounter* new_reset_counter();
void free_reset_counter(ResetCounter* reset_counter);

ResetCounter* find_reset_counter(ResetCounter* list, VNUM vnum);
void inc_reset_counter(ResetCounter** list, VNUM vnum);
void dec_reset_counter(ResetCounter* list, VNUM vnum);
int get_reset_count(ResetCounter* list, VNUM vnum);

extern int reset_count;
extern int reset_perm_count;

extern int reset_counter_count;
extern int reset_counter_perm_count;

#endif // !MUD98__ENTITIES__RESET_H
