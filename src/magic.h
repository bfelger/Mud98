////////////////////////////////////////////////////////////////////////////////
// magic.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__MAGIC_H
#define MUD98__MAGIC_H

#include "merc.h"

#include "entities/mobile.h"
#include "entities/object.h"

#include "data/damage.h"

SKNUM find_spell(Mobile* ch, const char* name);
int mana_cost(Mobile* ch, int min_mana, LEVEL level);
SKNUM skill_lookup(const char* name);
SKNUM skill_slot_lookup(int slot);
bool saves_spell(LEVEL level, Mobile* victim, DamageType dam_type);
void obj_cast_spell(SKNUM sn, LEVEL level, Mobile* ch, Mobile* victim,
    Object* obj);

#endif // !MUD98__MAGIC_H
