////////////////////////////////////////////////////////////////////////////////
// item.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__ITEM_H
#define MUD98__DATA__ITEM_H

#include "merc.h"

typedef enum item_type_t {
    ITEM_LIGHT              = 1,
    ITEM_SCROLL             = 2,
    ITEM_WAND               = 3,
    ITEM_STAFF              = 4,
    ITEM_WEAPON             = 5,
    ITEM_TREASURE           = 8,
    ITEM_ARMOR              = 9,
    ITEM_POTION             = 10,
    ITEM_CLOTHING           = 11,
    ITEM_FURNITURE          = 12,
    ITEM_TRASH              = 13,
    ITEM_CONTAINER          = 15,
    ITEM_DRINK_CON          = 17,
    ITEM_KEY                = 18,
    ITEM_FOOD               = 19,
    ITEM_MONEY              = 20,
    ITEM_BOAT               = 22,
    ITEM_CORPSE_NPC         = 23,
    ITEM_CORPSE_PC          = 24,
    ITEM_FOUNTAIN           = 25,
    ITEM_PILL               = 26,
    ITEM_PROTECT            = 27,
    ITEM_MAP                = 28,
    ITEM_PORTAL             = 29,
    ITEM_WARP_STONE         = 30,
    ITEM_ROOM_KEY           = 31,
    ITEM_GEM                = 32,
    ITEM_JEWELRY            = 33,
    ITEM_JUKEBOX            = 34,
} ItemType;

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
    WEAPON_EXOTIC = 0,
    WEAPON_SWORD = 1,
    WEAPON_DAGGER = 2,
    WEAPON_SPEAR = 3,
    WEAPON_MACE = 4,
    WEAPON_AXE = 5,
    WEAPON_FLAIL = 6,
    WEAPON_WHIP = 7,
    WEAPON_POLEARM = 8,
} WeaponType;

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
} WearFlags;

typedef enum wear_location_t {
    WEAR_NONE = -1,
    WEAR_LIGHT = 0,
    WEAR_FINGER_L = 1,
    WEAR_FINGER_R = 2,
    WEAR_NECK_1 = 3,
    WEAR_NECK_2 = 4,
    WEAR_BODY = 5,
    WEAR_HEAD = 6,
    WEAR_LEGS = 7,
    WEAR_FEET = 8,
    WEAR_HANDS = 9,
    WEAR_ARMS = 10,
    WEAR_SHIELD = 11,
    WEAR_ABOUT = 12,
    WEAR_WAIST = 13,
    WEAR_WRIST_L = 14,
    WEAR_WRIST_R = 15,
    WEAR_WIELD = 16,
    WEAR_HOLD = 17,
    WEAR_FLOAT = 18,
    WEAR_MAX
} WearLocation;

#endif // !MUD98__DATA__ITEM_H
