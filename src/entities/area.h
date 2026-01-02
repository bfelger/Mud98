////////////////////////////////////////////////////////////////////////////////
// entities/area.h
// Utilities to handle area
////////////////////////////////////////////////////////////////////////////////

typedef struct area_t Area;
typedef struct area_data_t AreaData;
typedef struct story_beat_t StoryBeat;
typedef struct checklist_item_t ChecklistItem;

#pragma once
#ifndef MUD98__ENTITIES__AREA_H
#define MUD98__ENTITIES__AREA_H

#include <merc.h>

#include <data/direction.h>
#include <data/quest.h>

#include "daycycle_period.h"
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

typedef enum checklist_status_t {
    CHECK_TODO = 0,
    CHECK_IN_PROGRESS = 1,
    CHECK_DONE = 2,
} ChecklistStatus;

struct story_beat_t {
    StoryBeat* next;
    char* title;
    char* description;
};

struct checklist_item_t {
    ChecklistItem* next;
    char* title;
    char* description;
    ChecklistStatus status;
};

typedef struct area_t {
    Entity header;
    Area* next;
    AreaData* data;
    Table rooms;
    char* owner_list;
    int16_t reset_timer;
    int16_t empty_timer;    // Time since nplayer became 0 (for instance cleanup grace period)
    int nplayer;
    bool empty;
    bool teardown_in_progress;  // Skip inbound exit cleanup during bulk teardown
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
    StoryBeat* story_beats;
    ChecklistItem* checklist;
    DayCyclePeriod* periods;
    char* loot_table;           // Default loot table name for mobs in this area
    bool suppress_daycycle_messages;
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

Area* new_area(AreaData* area_data);
void free_area(Area* area);
AreaData* new_area_data();
Area* create_area_instance(AreaData* area_data, bool create_exits);
void create_instance_exits(Area* area);
void save_area(AreaData* area);
Area* get_area_for_player(Mobile* ch, AreaData* area_data);

void load_area(FILE* fp);
void load_story_beats(FILE* fp);
void load_checklist(FILE* fp);
void load_area_daycycle(FILE* fp);
StoryBeat* add_story_beat(AreaData* area_data, const char* title, const char* description);
ChecklistItem* add_checklist_item(AreaData* area_data, const char* title, const char* description, ChecklistStatus status);
void free_story_beats(StoryBeat* head);
void free_checklist(ChecklistItem* head);

extern int area_count;
extern int area_perm_count;
extern int area_data_perm_count;
extern int area_data_count;
extern AreaData* area_data_free;

extern ValueArray global_areas;

#endif // !MUD98__ENTITIES__AREA_H
