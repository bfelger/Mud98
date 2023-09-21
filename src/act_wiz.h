////////////////////////////////////////////////////////////////////////////////
// act_owiz.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ACT_WIZ_H
#define MUD98__ACT_WIZ_H

#include "merc.h"

#include "entities/char_data.h"
#include "entities/object_data.h"

typedef struct wiznet_t {
    char* name;
    FLAGS flag;
    LEVEL level;
} WizNet;

extern const WizNet wiznet_table[];

RoomData* find_location(CharData* ch, char* arg);
void wiznet(char* string, CharData* ch, ObjectData* obj, FLAGS flag, 
    FLAGS flag_skip, LEVEL min_level);

#endif // !MUD98__ACT_WIZ_H
