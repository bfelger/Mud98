////////////////////////////////////////////////////////////////////////////////
// entities/reset.h
// Utilities to handle area resets of mobs and items
////////////////////////////////////////////////////////////////////////////////

typedef struct reset_t Reset;
typedef struct reset_counter_t ResetCounter;

#pragma once
#ifndef MUD98__ENTITIES__RESET_H
#define MUD98__ENTITIES__RESET_H

#include "area.h"

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

Reset* new_reset(void);
void free_reset(Reset* reset);

void reset_area(Area* area);

void load_resets(FILE* fp);

extern int reset_count;
extern int reset_perm_count;


#endif // !MUD98__ENTITIES__RESET_H
