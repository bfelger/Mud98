////////////////////////////////////////////////////////////////////////////////
// act_owiz.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ACT_WIZ_H
#define MUD98__ACT_WIZ_H

#include "merc.h"

#include "entities/mobile.h"
#include "entities/object.h"

typedef struct wiznet_t {
    char* name;
    FLAGS flag;
    LEVEL level;
} WizNet;

extern const WizNet wiznet_table[];

RoomData* find_location(Mobile* ch, char* arg);
void wiznet(char* string, Mobile* ch, Object* obj, FLAGS flag, 
    FLAGS flag_skip, LEVEL min_level);

#endif // !MUD98__ACT_WIZ_H
