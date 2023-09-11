////////////////////////////////////////////////////////////////////////////////
// magic.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__MAGIC_H
#define MUD98__MAGIC_H

#include "merc.h"

#include "entities/char_data.h"
#include "entities/object_data.h"

SKNUM find_spell(CharData* ch, const char* name);
int mana_cost(CharData* ch, int min_mana, LEVEL level);
SKNUM skill_lookup(const char* name);
SKNUM skill_slot_lookup(int slot);
bool saves_spell(LEVEL level, CharData* victim, DamageType dam_type);
void obj_cast_spell(SKNUM sn, LEVEL level, CharData* ch, CharData* victim,
    ObjectData* obj);

#endif // !MUD98__MAGIC_H
