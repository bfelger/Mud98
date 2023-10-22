////////////////////////////////////////////////////////////////////////////////
// room_data.h
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

typedef struct room_data_t RoomData;

#pragma once
#ifndef MUD98__ENTITIES__ROOM_DATA_H
#define MUD98__ENTITIES__ROOM_DATA_H

#include "merc.h"

#include "mobile.h"
#include "exit_data.h"
#include "extra_desc.h"
#include "object.h"
#include "reset_data.h"

#include "data/direction.h"

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

typedef struct room_data_t {
    RoomData* next;
    Mobile* people;
    Object* contents;
    ExtraDesc* extra_desc;
    AreaData* area;
    ExitData* exit[DIR_MAX];
    ResetData* reset_first;    // OLC
    ResetData* reset_last;     // OLC
    char* name;
    char* description;
    char* owner;
    VNUM vnum;
    FLAGS room_flags;
    Sector sector_type;
    int16_t light;
    int16_t heal_rate;
    int16_t mana_rate;
    int16_t clan;
    int16_t reset_num;
} RoomData;

#define FOR_EACH_IN_ROOM(c, r) \
    for ((c) = (r); (c) != NULL; (c) = c->next_in_room)

void free_room_index(RoomData* pRoom);
RoomData* get_room_data(VNUM vnum);
RoomData* new_room_index();

extern RoomData* room_index_hash[MAX_KEY_HASH];

extern int room_count;
extern VNUM top_vnum_room;

#endif // !MUD98__ENTITIES__ROOM_DATA_H
