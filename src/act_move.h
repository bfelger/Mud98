////////////////////////////////////////////////////////////////////////////////
// act_move.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ACT_MOVE_H
#define MUD98__ACT_MOVE_H

#include "entities/char_data.h"

void move_char(CharData* ch, int door, bool follow);

extern const char* dir_name[];
extern const char* dir_name_abbr[];
extern const int16_t rev_dir[];

#endif // !MUD98__ACT_MOVE_H
