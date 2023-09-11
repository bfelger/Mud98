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

typedef struct area_data_t {
    AreaData* next;
    HelpArea* helps;
    char* file_name;
    char* name;
    char* credits;
    int16_t age;
    int nplayer;
    LEVEL low_range;
    LEVEL high_range;
    VNUM min_vnum;
    VNUM max_vnum;
    bool empty;
    char* builders;     // OLC
    VNUM vnum;          // OLC
    int area_flags;     // OLC
    int security;       // OLC Value 1-9
} AreaData;

AreaData* new_area();

extern int top_area;
extern AreaData* area_first;
extern AreaData* area_last;

#endif // !MUD98__ENTITIES__AREA_DATA_H
