////////////////////////////////////////////////////////////////////////////////
// area.h
// Utilities to handle area
////////////////////////////////////////////////////////////////////////////////

typedef struct area_t Area;

#pragma once
#ifndef MUD98__ENTITIES__AREA_H
#define MUD98__ENTITIES__AREA_H

#include "merc.h"

#include <stdint.h>
#include <stdio.h>

#include "help_data.h"
#include "room.h"

#include "data/direction.h"
#include "data/quest.h"

typedef enum area_flags_t {
    AREA_NONE       = BIT(0),
    AREA_CHANGED    = BIT(1),	// Area has been modified.
    AREA_ADDED      = BIT(2),	// Area has been added to.
    AREA_LOADING    = BIT(3),	// Used for counting in db.c
} AreaFlags;

typedef struct area_t {
    Area* next;
    HelpArea* helps;
    Quest* quests;
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
    char* builders;  
    VNUM vnum;
    Sector sector;
    FLAGS area_flags;
    int16_t reset_thresh;
    int16_t reset_timer;
    bool always_reset;
} Area;

Area* new_area();

extern int area_count;
extern Area* area_first;
extern Area* area_last;

#endif // !MUD98__ENTITIES__AREA_H
