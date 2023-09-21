////////////////////////////////////////////////////////////////////////////////
// room_data.h
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

typedef struct room_data_t RoomData;

#pragma once
#ifndef MUD98__ENTITIES__ROOM_DATA_H
#define MUD98__ENTITIES__ROOM_DATA_H

#include "merc.h"

#include "char_data.h"
#include "exit_data.h"
#include "extra_desc.h"
#include "object_data.h"
#include "reset_data.h"

#include "data/direction.h"

// Well-known room IDs
#define ROOM_VNUM_LIMBO         2
#define ROOM_VNUM_CHAT          1200
#define ROOM_VNUM_TEMPLE        3001
#define ROOM_VNUM_ALTAR         3054
#define ROOM_VNUM_SCHOOL        3700
#define ROOM_VNUM_BALANCE       4500
#define ROOM_VNUM_CIRCLE        4400
#define ROOM_VNUM_DEMISE        4201
#define ROOM_VNUM_HONOR         4300

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
} RoomFlags;

// Sector
typedef enum sector_t {
    SECT_INSIDE         = 0,
    SECT_CITY           = 1,
    SECT_FIELD          = 2,
    SECT_FOREST         = 3,
    SECT_HILLS          = 4,
    SECT_MOUNTAIN       = 5,
    SECT_WATER_SWIM     = 6,
    SECT_WATER_NOSWIM   = 7,
    SECT_UNUSED         = 8,
    SECT_AIR            = 9,
    SECT_DESERT         = 10,
    SECT_MAX
} Sector;

typedef struct room_data_t {
    RoomData* next;
    CharData* people;
    ObjectData* contents;
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

void free_room_index(RoomData* pRoom);
RoomData* get_room_data(VNUM vnum);
RoomData* new_room_index();

extern RoomData* room_index_hash[MAX_KEY_HASH];
extern const int16_t movement_loss[SECT_MAX];

extern int top_room;
extern VNUM top_vnum_room;

#endif // !MUD98__ENTITIES__ROOM_DATA_H
