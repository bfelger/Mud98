////////////////////////////////////////////////////////////////////////////////
// entities/room.h
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

typedef struct room_t Room;
typedef struct room_data_t RoomData;

#pragma once
#ifndef MUD98__ENTITIES__ROOM_H
#define MUD98__ENTITIES__ROOM_H

#include <merc.h>

#include <data/direction.h>

#include "entity.h"
#include "extra_desc.h"
#include "mobile.h"
#include "room_exit.h"
#include "object.h"
#include "reset.h"

#include <lox/lox.h>
#include <lox/list.h>
#include <lox/table.h>

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
    AreaData* area_data;
    RoomExitData* exit_data[DIR_MAX];
    Reset* reset_first;
    Reset* reset_last;
    char* description;
    char* owner;
    FLAGS room_flags;
    Sector sector_type;
    int16_t heal_rate;
    int16_t mana_rate;
    int16_t clan;
    int16_t reset_num;
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
    for (int r##_idx = 0, r##_l_count = 0; r##_l_count < global_rooms.count; ++r##_idx) \
        if (!IS_NIL((&global_rooms.entries[r##_idx])->key) \
            && !IS_NIL((&global_rooms.entries[r##_idx])->value) \
            && IS_ROOM_DATA((&global_rooms.entries[r##_idx])->value) \
            && (r = AS_ROOM_DATA(global_rooms.entries[r##_idx].value)) != NULL \
            && ++r##_l_count)

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

extern int room_count;
extern int room_perm_count;
extern int room_data_count;
extern int room_data_perm_count;

extern Room* room_free;
extern RoomData* room_data_free;

extern Table global_rooms;

extern VNUM top_vnum_room;

#endif // !MUD98__ENTITIES__ROOM_H
