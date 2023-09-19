////////////////////////////////////////////////////////////////////////////////
// item.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__ITEM_H
#define MUD98__DATA__ITEM_H

#include "merc.h"

////////////////////////////////////////////////////////////////////////////////
// Generic items
////////////////////////////////////////////////////////////////////////////////

typedef enum item_flags_t {
    ITEM_GLOW           = BIT(0),
    ITEM_HUM            = BIT(1),
    ITEM_DARK           = BIT(2),
    ITEM_LOCK           = BIT(3),
    ITEM_EVIL           = BIT(4),
    ITEM_INVIS          = BIT(5),
    ITEM_MAGIC          = BIT(6),
    ITEM_NODROP         = BIT(7),
    ITEM_BLESS          = BIT(8),
    ITEM_ANTI_GOOD      = BIT(9),
    ITEM_ANTI_EVIL      = BIT(10),
    ITEM_ANTI_NEUTRAL   = BIT(11),
    ITEM_NOREMOVE       = BIT(12),
    ITEM_INVENTORY      = BIT(13),
    ITEM_NOPURGE        = BIT(14),
    ITEM_ROT_DEATH      = BIT(15),
    ITEM_VIS_DEATH      = BIT(16),
    ITEM_NONMETAL       = BIT(18),
    ITEM_NOLOCATE       = BIT(19),
    ITEM_MELT_DROP      = BIT(20),
    ITEM_HAD_TIMER      = BIT(21),
    ITEM_SELL_EXTRACT   = BIT(22),
    ITEM_BURN_PROOF     = BIT(24),
    ITEM_NOUNCURSE      = BIT(25),
} ItemFlags;

typedef enum item_type_t {
    ITEM_NONE           = 0,
    ITEM_LIGHT          = 1,
    ITEM_SCROLL         = 2,
    ITEM_WAND           = 3,
    ITEM_STAFF          = 4,
    ITEM_WEAPON         = 5,
    ITEM_TREASURE       = 8,
    ITEM_ARMOR          = 9,
    ITEM_POTION         = 10,
    ITEM_CLOTHING       = 11,
    ITEM_FURNITURE      = 12,
    ITEM_TRASH          = 13,
    ITEM_CONTAINER      = 15,
    ITEM_DRINK_CON      = 17,
    ITEM_KEY            = 18,
    ITEM_FOOD           = 19,
    ITEM_MONEY          = 20,
    ITEM_BOAT           = 22,
    ITEM_CORPSE_NPC     = 23,
    ITEM_CORPSE_PC      = 24,
    ITEM_FOUNTAIN       = 25,
    ITEM_PILL           = 26,
    ITEM_PROTECT        = 27,
    ITEM_MAP            = 28,
    ITEM_PORTAL         = 29,
    ITEM_WARP_STONE     = 30,
    ITEM_ROOM_KEY       = 31,
    ITEM_GEM            = 32,
    ITEM_JEWELRY        = 33,
    ITEM_JUKEBOX        = 34,
} ItemType;

#define ITEM_TYPE_COUNT 35

typedef struct item_info_t {
    const ItemType type;
    const char* name;
} ItemInfo;

extern const ItemInfo item_table[ITEM_TYPE_COUNT];

////////////////////////////////////////////////////////////////////////////////
// Containers
////////////////////////////////////////////////////////////////////////////////

typedef enum container_flags_t {
    CONT_CLOSEABLE      = BIT(0),
    CONT_PICKPROOF      = BIT(1),
    CONT_CLOSED         = BIT(2),
    CONT_LOCKED         = BIT(3),
    CONT_PUT_ON         = BIT(4),
} ContainerFlags;

////////////////////////////////////////////////////////////////////////////////
// Furniture
////////////////////////////////////////////////////////////////////////////////

typedef enum furniture_flags_t {
    STAND_AT            = BIT(0),
    STAND_ON            = BIT(1),
    STAND_IN            = BIT(2),
    SIT_AT              = BIT(3),
    SIT_ON              = BIT(4),
    SIT_IN              = BIT(5),
    REST_AT             = BIT(6),
    REST_ON             = BIT(7),
    REST_IN             = BIT(8),
    SLEEP_AT            = BIT(9),
    SLEEP_ON            = BIT(10),
    SLEEP_IN            = BIT(11),
    PUT_AT              = BIT(12),
    PUT_ON              = BIT(13),
    PUT_IN              = BIT(14),
    PUT_INSIDE          = BIT(15),
} FurnitureFlags;

////////////////////////////////////////////////////////////////////////////////
// Liquids
////////////////////////////////////////////////////////////////////////////////

#define LIQ_WATER 0
#define LIQ_COUNT 36

typedef struct liquid_info_t {
    const char* name;
    const char* color;
    const int16_t proof;
    const int16_t full;
    const int16_t thirst;
    const int16_t food;
    const int16_t sip_size;
} LiquidInfo;

extern const LiquidInfo liquid_table[LIQ_COUNT];

////////////////////////////////////////////////////////////////////////////////
// Portals
////////////////////////////////////////////////////////////////////////////////

typedef enum portal_flags_t {
    PORTAL_NORMAL_EXIT  = BIT(0),
    PORTAL_NOCURSE      = BIT(1),
    PORTAL_GOWITH       = BIT(2),
    PORTAL_BUGGY        = BIT(3),
    PORTAL_RANDOM       = BIT(4),
} PortalFlags;

////////////////////////////////////////////////////////////////////////////////
// Weapons
////////////////////////////////////////////////////////////////////////////////

typedef enum weapon_flags_t {
    WEAPON_FLAMING      = BIT(0),
    WEAPON_FROST        = BIT(1),
    WEAPON_VAMPIRIC     = BIT(2),
    WEAPON_SHARP        = BIT(3),
    WEAPON_VORPAL       = BIT(4),
    WEAPON_TWO_HANDS    = BIT(5),
    WEAPON_SHOCKING     = BIT(6),
    WEAPON_POISON       = BIT(7),
} WeaponFlags;

typedef enum weapon_type_t {
    WEAPON_EXOTIC       = 0,
    WEAPON_SWORD        = 1,
    WEAPON_DAGGER       = 2,
    WEAPON_SPEAR        = 3,
    WEAPON_MACE         = 4,
    WEAPON_AXE          = 5,
    WEAPON_FLAIL        = 6,
    WEAPON_WHIP         = 7,
    WEAPON_POLEARM      = 8,
} WeaponType;

#define WEAPON_MAX 9

typedef struct weapon_info_t {
    const WeaponType type;
    const char* name;
    const SKNUM* gsn;
    const VNUM vnum;
} WeaponInfo;

extern const WeaponInfo weapon_table[WEAPON_MAX];

////////////////////////////////////////////////////////////////////////////////

typedef enum wear_flags_t {
    ITEM_TAKE           = BIT(0),
    ITEM_WEAR_FINGER    = BIT(1),
    ITEM_WEAR_NECK      = BIT(2),
    ITEM_WEAR_BODY      = BIT(3),
    ITEM_WEAR_HEAD      = BIT(4),
    ITEM_WEAR_LEGS      = BIT(5),
    ITEM_WEAR_FEET      = BIT(6),
    ITEM_WEAR_HANDS     = BIT(7),
    ITEM_WEAR_ARMS      = BIT(8),
    ITEM_WEAR_SHIELD    = BIT(9),
    ITEM_WEAR_ABOUT     = BIT(10),
    ITEM_WEAR_WAIST     = BIT(11),
    ITEM_WEAR_WRIST     = BIT(12),
    ITEM_WIELD          = BIT(13),
    ITEM_HOLD           = BIT(14),
    ITEM_NO_SAC         = BIT(15),
    ITEM_WEAR_FLOAT     = BIT(16),
    ITEM_WEAR_NONE      = NO_FLAG,
} WearFlags;

typedef enum wear_location_t {
    WEAR_UNHELD         = -1,
    WEAR_LIGHT          = 0,
    WEAR_FINGER_L       = 1,
    WEAR_FINGER_R       = 2,
    WEAR_NECK_1         = 3,
    WEAR_NECK_2         = 4,
    WEAR_BODY           = 5,
    WEAR_HEAD           = 6,
    WEAR_LEGS           = 7,
    WEAR_FEET           = 8,
    WEAR_HANDS          = 9,
    WEAR_ARMS           = 10,
    WEAR_SHIELD         = 11,
    WEAR_ABOUT          = 12,
    WEAR_WAIST          = 13,
    WEAR_WRIST_L        = 14,
    WEAR_WRIST_R        = 15,
    WEAR_WIELD          = 16,
    WEAR_HOLD           = 17,
    WEAR_FLOAT          = 18,
} WearLocation;

#define WEAR_COUNT 19

////////////////////////////////////////////////////////////////////////////////
// Static VNUMs
////////////////////////////////////////////////////////////////////////////////

#define OBJ_VNUM_SILVER_ONE     1
#define OBJ_VNUM_GOLD_ONE       2
#define OBJ_VNUM_GOLD_SOME      3
#define OBJ_VNUM_SILVER_SOME    4
#define OBJ_VNUM_COINS          5

#define OBJ_VNUM_CORPSE_NPC     10
#define OBJ_VNUM_CORPSE_PC      11
#define OBJ_VNUM_SEVERED_HEAD   12
#define OBJ_VNUM_TORN_HEART     13
#define OBJ_VNUM_SLICED_ARM     14
#define OBJ_VNUM_SLICED_LEG     15
#define OBJ_VNUM_GUTS           16
#define OBJ_VNUM_BRAINS         17

#define OBJ_VNUM_MUSHROOM       20
#define OBJ_VNUM_LIGHT_BALL     21
#define OBJ_VNUM_SPRING         22
#define OBJ_VNUM_DISC           23
#define OBJ_VNUM_PORTAL         25

#define OBJ_VNUM_DUMMY          30

#define OBJ_VNUM_ROSE           1001

#define OBJ_VNUM_PIT            3010

#define OBJ_VNUM_SCHOOL_MACE    3700
#define OBJ_VNUM_SCHOOL_DAGGER  3701
#define OBJ_VNUM_SCHOOL_SWORD   3702
#define OBJ_VNUM_SCHOOL_SPEAR   3717
#define OBJ_VNUM_SCHOOL_STAFF   3718
#define OBJ_VNUM_SCHOOL_AXE     3719
#define OBJ_VNUM_SCHOOL_FLAIL   3720
#define OBJ_VNUM_SCHOOL_WHIP    3721
#define OBJ_VNUM_SCHOOL_POLEARM 3722

#define OBJ_VNUM_SCHOOL_VEST    3703
#define OBJ_VNUM_SCHOOL_SHIELD  3704
#define OBJ_VNUM_SCHOOL_BANNER  3716
#define OBJ_VNUM_MAP            3162

#define OBJ_VNUM_WHISTLE        2116

#endif // !MUD98__DATA__ITEM_H
