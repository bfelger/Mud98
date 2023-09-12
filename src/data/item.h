////////////////////////////////////////////////////////////////////////////////
// item.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__ITEM_H
#define MUD98__DATA__ITEM_H

typedef enum item_type_t {
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

typedef enum wear_location_t {
    WEAR_NONE           = -1,
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
    WEAR_MAX
} WearLocation;

#endif // !MUD98__DATA__ITEM_H
