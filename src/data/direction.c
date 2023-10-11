////////////////////////////////////////////////////////////////////////////////
// direction.c
////////////////////////////////////////////////////////////////////////////////

#include "direction.h"

const DirInfo dir_list[DIR_MAX] = {
    { DIR_NORTH,    DIR_SOUTH,      "north",    "N" },
    { DIR_EAST,     DIR_WEST,       "east",     "E" },
    { DIR_SOUTH,    DIR_NORTH,      "south",    "S" },
    { DIR_WEST,     DIR_EAST,       "west",     "W" },
    { DIR_UP,       DIR_DOWN,       "up",       "U" },
    { DIR_DOWN,     DIR_UP,         "down",     "D" },
};

const int16_t movement_loss[SECT_MAX] = { 1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6 };
