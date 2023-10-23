////////////////////////////////////////////////////////////////////////////////
// act_obj.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ACT_OBJ_H
#define MUD98__ACT_OBJ_H

#include "entities/mobile.h"
#include "entities/object.h"

bool can_loot(Mobile* ch, Object* obj);
void wear_obj(Mobile* ch, Object* obj, bool fReplace);
void get_obj(Mobile* ch, Object* obj, Object* container);

#endif // !MUD98__ACT_OBJ_H
