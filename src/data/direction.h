////////////////////////////////////////////////////////////////////////////////
// direction.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__DIRECTION_H
#define MUD98__DATA__DIRECTION_H

typedef enum direction_t {
    DIR_NORTH       = 0,
    DIR_EAST        = 1,
    DIR_SOUTH       = 2,
    DIR_WEST        = 3,
    DIR_UP          = 4,
    DIR_DOWN        = 5,
    DIR_MAX
} Direction;

typedef struct direction_info_t {
    const Direction dir;
    const Direction rev_dir;
    const char* name;
    const char* name_abbr;
} DirInfo;

extern const DirInfo dir_list[DIR_MAX];

#endif // !MUD98__DATA__DIRECTION_H
