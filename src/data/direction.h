////////////////////////////////////////////////////////////////////////////////
// data/direction.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__DIRECTION_H
#define MUD98__DATA__DIRECTION_H

#include <stdint.h>

typedef enum direction_t {
    DIR_NORTH           = 0,
    DIR_EAST            = 1,
    DIR_SOUTH           = 2,
    DIR_WEST            = 3,
    DIR_UP              = 4,
    DIR_DOWN            = 5,
} Direction;

#define DIR_MAX         6

typedef struct direction_info_t {
    const Direction dir;
    const Direction rev_dir;
    const char* name;
    const char* name_abbr;
} DirInfo;

extern const DirInfo dir_list[DIR_MAX];

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
    SECT_UNDERGROUND    = 8,
    SECT_AIR            = 9,
    SECT_DESERT         = 10,
} Sector;

#define SECT_MAX        11

Direction get_direction(const char* dir_name);

extern const int16_t movement_loss[SECT_MAX];

#endif // !MUD98__DATA__DIRECTION_H
