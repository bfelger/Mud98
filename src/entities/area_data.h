////////////////////////////////////////////////////////////////////////////////
// area_data.h
// Utilities to handle area
////////////////////////////////////////////////////////////////////////////////

typedef struct area_data_t AreaData;

#pragma once
#ifndef MUD98__ENTITIES__AREA_DATA_H
#define MUD98__ENTITIES__AREA_DATA_H

#include "merc.h"

#include <stdint.h>
#include <stdio.h>

#include "help_data.h"

typedef enum area_flags_t {
    AREA_NONE       = BIT(0),
    AREA_CHANGED    = BIT(1),	// Area has been modified.
    AREA_ADDED      = BIT(2),	// Area has been added to.
    AREA_LOADING    = BIT(3),	// Used for counting in db.c
} AreaFlags;

typedef struct area_data_t {
    AreaData* next;
    HelpArea* helps;
    char* file_name;
    char* name;
    char* credits;
    int nplayer;
    int security;       // OLC Value 1-9
    LEVEL low_range;
    LEVEL high_range;
    VNUM min_vnum;
    VNUM max_vnum;
    bool empty;
    char* builders;     // OLC
    VNUM vnum;          // OLC
    FLAGS area_flags;   // OLC
    int16_t age;
} AreaData;

AreaData* new_area();

extern int area_count;
extern AreaData* area_first;
extern AreaData* area_last;

#endif // !MUD98__ENTITIES__AREA_DATA_H
