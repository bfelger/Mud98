////////////////////////////////////////////////////////////////////////////////
// act_owiz.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ACT_WIZ_H
#define MUD98__ACT_WIZ_H

#include "entities/char_data.h"
#include "entities/object_data.h"

RoomData* find_location(CharData* ch, char* arg);
void wiznet(char* string, CharData* ch, ObjectData* obj, long flag, 
    long flag_skip, int min_level);

#endif // !MUD98__ACT_WIZ_H
