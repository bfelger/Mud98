////////////////////////////////////////////////////////////////////////////////
// room.h
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

typedef struct room_t Room;
typedef struct room_data_t RoomData;

#pragma once
#ifndef MUD98__ENTITIES__ROOM_H
#define MUD98__ENTITIES__ROOM_H

#include "merc.h"

#include "data/direction.h"

#include "entities/entity.h"
#include "entities/extra_desc.h"
#include "entities/mobile.h"
#include "entities/room_exit.h"
#include "entities/object.h"
#include "entities/reset.h"

#include "lox/lox.h"
#include "lox/table.h"

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
    EntityHeader header;
    Room* next;
    Room* next_instance;
    VNUM vnum;
    RoomData* data;
    Area* area;
    RoomExit* exit[DIR_MAX];
    Mobile* people;
    Object* contents;
    int16_t light;
} Room;

typedef struct room_data_t {
    EntityHeader header;
    RoomData* next;
    Room* instances;
    ExtraDesc* extra_desc;
    AreaData* area_data;
    RoomExitData* exit_data[DIR_MAX];
    Reset* reset_first;
    Reset* reset_last;
    String* name;
    char* description;
    char* owner;
    VNUM vnum;
    FLAGS room_flags;
    Sector sector_type;
    int16_t heal_rate;
    int16_t mana_rate;
    int16_t clan;
    int16_t reset_num;
} RoomData;

#define FOR_EACH_IN_ROOM(c, r) \
    for ((c) = (r); (c) != NULL; (c) = c->next_in_room)

#define FOR_EACH_INSTANCE(r, i) \
    for ((r) = (i); (r) != NULL; (r) = r->next_instance)

#define FOR_EACH_GLOBAL_ROOM_DATA(r) \
    for (int r##_hash = 0; r##_hash < MAX_KEY_HASH; ++r##_hash) \
        FOR_EACH(r, room_data_hash_table[r##_hash])

#define FOR_EACH_AREA_ROOM(r, a) \
    for (int r##_hash = 0; r##_hash < AREA_ROOM_VNUM_HASH_SIZE; ++r##_hash) \
        FOR_EACH(r, (a)->rooms[r##_hash])

Room* new_room(RoomData* room_data, Area* area);
void free_room(Room* room);
Room* get_room(Area* search_context, VNUM vnum);
Room* get_room_for_player(Mobile* ch, VNUM vnum);

void free_room_data(RoomData* pRoom);
RoomData* get_room_data(VNUM vnum);
RoomData* new_room_data();

extern int room_count;
extern int room_perm_count;
extern int room_data_count;
extern int room_data_perm_count;

extern RoomData* room_data_hash_table[MAX_KEY_HASH];

extern VNUM top_vnum_room;

////////////////////////////////////////////////////////////////////////////////
// Lox implementation
////////////////////////////////////////////////////////////////////////////////

//void init_room_class();
//Value create_room_value(Room* room);
//Value get_room_people_native(int arg_count, Value* args);
//Value get_room_native(int arg_count, Value* args);
//Value get_room_contents_native(int arg_count, Value* args);

#endif // !MUD98__ENTITIES__ROOM_H
