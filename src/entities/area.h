////////////////////////////////////////////////////////////////////////////////
// area.h
// Utilities to handle area
////////////////////////////////////////////////////////////////////////////////

typedef struct area_t Area;
typedef struct area_data_t AreaData;

#pragma once
#ifndef MUD98__ENTITIES__AREA_H
#define MUD98__ENTITIES__AREA_H

#include "merc.h"

#include <stdint.h>
#include <stdio.h>

#include "help_data.h"
#include "reset.h"
#include "room.h"

#include "data/direction.h"
#include "data/quest.h"

#define AREA_ROOM_VNUM_HASH_SIZE    32

typedef enum area_flags_t {
    AREA_NONE       = BIT(0),
    AREA_CHANGED    = BIT(1),	// Area has been modified.
    AREA_ADDED      = BIT(2),	// Area has been added to.
    AREA_LOADING    = BIT(3),	// Used for counting in db.c
} AreaFlags;

typedef enum inst_type_t {
    AREA_INST_NONE  = 0,
    AREA_INST_MULTI = 1,        // Multiple instances, delete instead of reset
    //AREA_INST_WEEK  = 2,      // For future; weekly instance locks
} InstanceType;

typedef struct area_t {
    Area* next;
    AreaData* data;
    Room* rooms[AREA_ROOM_VNUM_HASH_SIZE];
    ResetCounter* mob_counts;
    ResetCounter* obj_counts;
    char* char_list;
    int16_t reset_timer;
    int nplayer;
    bool empty;
} Area;

typedef struct area_data_t {
    AreaData* next;
    Area* instances;
    HelpArea* helps;
    Quest* quests;
    char* file_name;
    char* name;
    char* credits;
    int security;       // OLC Value 1-9
    LEVEL low_range;
    LEVEL high_range;
    VNUM min_vnum;
    VNUM max_vnum;
    char* builders;  
    VNUM vnum;
    Sector sector;
    FLAGS area_flags;
    int16_t reset_thresh;
    bool always_reset;
    InstanceType inst_type;
} AreaData;

AreaData* new_area_data();
Area* create_area_instance(AreaData* area_data, bool create_exits);
void create_instance_exits(Area* area);

extern int area_count;
extern int area_perm_count;
extern int area_data_count;
extern int area_data_perm_count;

extern AreaData* area_data_list;
extern AreaData* area_data_last;

#endif // !MUD98__ENTITIES__AREA_H
