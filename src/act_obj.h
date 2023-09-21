////////////////////////////////////////////////////////////////////////////////
// act_obj.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ACT_OBJ_H
#define MUD98__ACT_OBJ_H

#include "entities/char_data.h"
#include "entities/object_data.h"

bool can_loot(CharData* ch, ObjectData* obj);
void wear_obj(CharData* ch, ObjectData* obj, bool fReplace);
void get_obj(CharData* ch, ObjectData* obj, ObjectData* container);

#endif // !MUD98__ACT_OBJ_H
