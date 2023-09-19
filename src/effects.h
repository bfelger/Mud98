////////////////////////////////////////////////////////////////////////////////
// effects.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__EFFECTS_H
#define MUD98__EFFECTS_H

#include "merc.h"

#include "data/spell.h"

void acid_effect(void* vo, LEVEL level, int dam, SpellTarget target);
void cold_effect(void* vo, LEVEL level, int dam, SpellTarget target);
void fire_effect(void* vo, LEVEL level, int dam, SpellTarget target);
void poison_effect(void* vo, LEVEL level, int dam, SpellTarget target);
void shock_effect(void* vo, LEVEL level, int dam, SpellTarget target);

#endif // !MUD98__EFFECTS_H
