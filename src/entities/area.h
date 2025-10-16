////////////////////////////////////////////////////////////////////////////////
// area.h
// Utilities to handle area
////////////////////////////////////////////////////////////////////////////////

typedef struct area_t Area;
typedef struct area_data_t AreaData;

#pragma once
#ifndef MUD98__ENTITIES__AREA_H
#define MUD98__ENTITIES__AREA_H

#include <merc.h>

#include <data/direction.h>
#include <data/quest.h>

#include "entity.h"
#include "help_data.h"
#include "reset.h"
#include "room.h"

#include <lox/lox.h>

#include <stdint.h>
#include <stdio.h>

#define AREA_ROOM_VNUM_HASH_SIZE    32

typedef enum area_flags_t {
    AREA_NONE       = BIT(0),
    AREA_CHANGED    = BIT(1),	// Area has been modified.
    AREA_ADDED      = BIT(2),	// Area has been added to.
    AREA_LOADING    = BIT(3),	// Used for counting in db.c
} AreaFlags;

typedef enum inst_type_t {
    AREA_INST_SINGLE  = 0,
    AREA_INST_MULTI = 1,        // Multiple instances, delete instead of reset
    //AREA_INST_WEEK  = 2,      // For future; weekly instance locks
} InstanceType;

typedef struct area_t {
    Entity header;
    Area* next;
    AreaData* data;
    Table rooms;
    char* owner_list;
    int16_t reset_timer;
    int nplayer;
    bool empty;
} Area;

typedef struct area_data_t {
    Entity header;
    AreaData* next;
    List instances;
    HelpArea* helps;
    Quest* quests;
    char* file_name;
    char* credits;
    int security;       // OLC Value 1-9
    LEVEL low_range;
    LEVEL high_range;
    VNUM min_vnum;
    VNUM max_vnum;
    char* builders;
    Sector sector;
    FLAGS area_flags;
    int16_t reset_thresh;
    bool always_reset;
    InstanceType inst_type;
} AreaData;

#define FOR_EACH_AREA(area)                                                    \
    for (int area##i = 0; area##i < global_areas.count; ++area##i)             \
        if ((area = AS_AREA_DATA(global_areas.values[area##i])) != NULL)

#define FOR_EACH_AREA_INST(inst, area) \
    if ((area)->instances.front == NULL) \
        inst = NULL; \
    else if ((inst = AS_AREA((area)->instances.front->value)) != NULL) \
        for (struct { Node* node; Node* next; } inst##_loop = { (area)->instances.front, (area)->instances.front->next }; \
            inst##_loop.node != NULL; \
            inst##_loop.node = inst##_loop.next, \
                inst##_loop.next = inst##_loop.next ? inst##_loop.next->next : NULL, \
                inst = inst##_loop.node != NULL ? AS_AREA(inst##_loop.node->value) : NULL) \
            if (inst != NULL)

#define GET_AREA_DATA(index)                                                   \
    AS_AREA_DATA(global_areas.values[index])

#define LAST_AREA_DATA                                                         \
    GET_AREA_DATA(global_areas.count - 1)

AreaData* new_area_data();
Area* create_area_instance(AreaData* area_data, bool create_exits);
void create_instance_exits(Area* area);
void save_area(AreaData* area);
Area* get_area_for_player(Mobile* ch, AreaData* area_data);

extern int area_count;
extern int area_perm_count;
extern int area_data_perm_count;

extern ValueArray global_areas;

#endif // !MUD98__ENTITIES__AREA_H
