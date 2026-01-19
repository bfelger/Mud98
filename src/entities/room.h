////////////////////////////////////////////////////////////////////////////////
// entities/room.h
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ENTITIES__ROOM_H
#define MUD98__ENTITIES__ROOM_H

#include <merc.h>

#include <data/direction.h>

#include "daycycle_period.h"
#include "entity.h"
#include "extra_desc.h"
#include "mobile.h"
#include "room_exit.h"
#include "object.h"
#include "reset.h"

#include <lox/lox.h>
#include <lox/list.h>
#include <lox/ordered_table.h>
#include <lox/table.h>

typedef struct room_t Room;
typedef struct room_data_t RoomData;
typedef struct area_data_t AreaData;

// Static room VNUMs
#define ROOM_VNUM_LIMBO         2
#define ROOM_VNUM_CHAT          1200
//#define ROOM_VNUM_TEMPLE      3001    -- Use ROOM_RECALL flag instead
#define ROOM_VNUM_ALTAR         3054
//#define ROOM_VNUM_SCHOOL      3700    -- Set by config/race/class
#define ROOM_VNUM_BALANCE       4500
#define ROOM_VNUM_CIRCLE        4400
#define ROOM_VNUM_DEMISE        4201
#define ROOM_VNUM_HONOR         4300

#define ROOM_VNUM_PETSHOP       9621
#define ROOM_VNUM_PETSHOP_INV   9706

// Room flags
typedef enum room_flags_t {
    ROOM_DARK           = BIT(0),
    ROOM_NO_MOB         = BIT(2),
    ROOM_INDOORS        = BIT(3),
    ROOM_PRIVATE        = BIT(9),
    ROOM_SAFE           = BIT(10),
    ROOM_SOLITARY       = BIT(11),
    ROOM_PET_SHOP       = BIT(12),
    ROOM_NO_RECALL      = BIT(13),
    ROOM_IMP_ONLY       = BIT(14),
    ROOM_GODS_ONLY      = BIT(15),
    ROOM_HEROES_ONLY    = BIT(16),
    ROOM_NEWBIES_ONLY   = BIT(17),
    ROOM_LAW            = BIT(18),
    ROOM_NOWHERE        = BIT(19),
    ROOM_RECALL         = BIT(20),
} RoomFlags;

typedef struct room_t {
    Entity header;
    Room* next;
    List mobiles;
    List objects;
    List inbound_exits;  // RoomExit* pointing TO this room
    RoomData* data;
    Area* area;
    RoomExit* exit[DIR_MAX];
    int16_t light;
} Room;

typedef struct room_data_t {
    Entity header;
    RoomData* next;
    List instances;
    ExtraDesc* extra_desc;
    DayCyclePeriod* periods;
    AreaData* area_data;
    RoomExitData* exit_data[DIR_MAX];
    Reset* reset_first;
    Reset* reset_last;
    char* description;
    FLAGS room_flags;
    Sector sector_type;
    int16_t reset_num;
    bool suppress_daycycle_messages;
} RoomData;

#define FOR_EACH_ROOM_INST(inst, room_data) \
    if ((room_data)->instances.front == NULL) \
        inst = NULL; \
    else if ((inst = AS_ROOM((room_data)->instances.front->value)) != NULL) \
        for (struct { Node* node; Node* next; } inst##_loop = { (room_data)->instances.front, (room_data)->instances.front->next }; \
            inst##_loop.node != NULL; \
            inst##_loop.node = inst##_loop.next, \
                inst##_loop.next = inst##_loop.next ? inst##_loop.next->next : NULL, \
                inst = inst##_loop.node != NULL ? AS_ROOM(inst##_loop.node->value) : NULL) \
            if (inst != NULL)

#define FOR_EACH_GLOBAL_ROOM(r) \
    for (GlobalRoomIter r##_iter = make_global_room_iter(); \
        (r = global_room_iter_next(&r##_iter)) != NULL; )

#define FOR_EACH_AREA_ROOM(r, a) \
    for (int r##_idx = 0, r##_l_count = 0; r##_l_count < a->rooms.count; ++r##_idx) \
        if (!IS_NIL((&a->rooms.entries[r##_idx])->key) \
            && !IS_NIL((&a->rooms.entries[r##_idx])->value) \
            && IS_ROOM((&a->rooms.entries[r##_idx])->value) \
            && (r = AS_ROOM(a->rooms.entries[r##_idx].value)) != NULL \
            && ++r##_l_count)

#define FOR_EACH_ROOM_OBJ(content, room) \
    if ((room)->objects.front == NULL) \
        content = NULL; \
    else if ((content = AS_OBJECT((room)->objects.front->value)) != NULL) \
        for (struct { Node* node; Node* next; } content##_loop = { (room)->objects.front, (room)->objects.front->next }; \
            content##_loop.node != NULL; \
            content##_loop.node = content##_loop.next, \
                content##_loop.next = content##_loop.next ? content##_loop.next->next : NULL, \
                content = content##_loop.node != NULL ? AS_OBJECT(content##_loop.node->value) : NULL) \
            if (content != NULL)

#define FOR_EACH_ROOM_MOB(mob, room) \
    if ((room)->mobiles.front == NULL) \
        mob = NULL; \
    else if ((mob = AS_MOBILE((room)->mobiles.front->value)) != NULL) \
        for (struct { Node* node; Node* next; } mob##_loop = { (room)->mobiles.front, (room)->mobiles.front->next }; \
            mob##_loop.node != NULL; \
            mob##_loop.node = mob##_loop.next, \
                mob##_loop.next = mob##_loop.next ? mob##_loop.next->next : NULL, \
                mob = mob##_loop.node != NULL ? AS_MOBILE(mob##_loop.node->value) : NULL) \
            if (mob != NULL)

#define ROOM_HAS_MOBS(room) \
    ((room)->mobiles.count > 0)

#define ROOM_HAS_OBJS(room) \
    ((room)->objects.count > 0)

Room* new_room(RoomData* room_data, Area* area);
void free_room(Room* room);
Room* get_room(Area* search_context, VNUM vnum);
Room* get_room_for_player(Mobile* ch, VNUM vnum);

void free_room_data(RoomData* pRoom);
RoomData* get_room_data(VNUM vnum);
RoomData* new_room_data();

void load_rooms(FILE* fp);

DayCyclePeriod* room_daycycle_period_add(RoomData* room, const char* name, int start_hour, int end_hour);
DayCyclePeriod* room_daycycle_period_find(RoomData* room, const char* name);
bool room_daycycle_period_remove(RoomData* room, const char* name);
void room_daycycle_period_clear(RoomData* room);
DayCyclePeriod* room_daycycle_period_clone(const DayCyclePeriod* head);
DayCyclePeriod* area_daycycle_period_add(AreaData* area, const char* name, int start_hour, int end_hour);
DayCyclePeriod* area_daycycle_period_find(AreaData* area, const char* name);
bool area_daycycle_period_remove(AreaData* area, const char* name);
void area_daycycle_period_clear(AreaData* area);
const char* room_description_for_hour(const RoomData* room, int hour);
bool room_suppresses_daycycle_messages(const RoomData* room);
bool room_has_period_message_transition(const RoomData* room, int old_hour, int new_hour);
void broadcast_room_period_messages(int old_hour, int new_hour);
void broadcast_area_period_messages(int old_hour, int new_hour);

extern int room_count;
extern int room_perm_count;
extern int room_data_count;
extern int room_data_perm_count;

extern Room* room_free;
extern RoomData* room_data_free;

typedef struct {
    OrderedTableIter iter;
} GlobalRoomIter;

void init_global_rooms(void);
void free_global_rooms(void);
RoomData* global_room_get(VNUM vnum);
bool global_room_set(RoomData* room_data);
bool global_room_remove(VNUM vnum);
int global_room_count(void);
GlobalRoomIter make_global_room_iter(void);
RoomData* global_room_iter_next(GlobalRoomIter* iter);
OrderedTable snapshot_global_rooms(void);
void restore_global_rooms(OrderedTable snapshot);
void mark_global_rooms(void);

extern VNUM top_vnum_room;

#endif // !MUD98__ENTITIES__ROOM_H
