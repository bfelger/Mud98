////////////////////////////////////////////////////////////////////////////////
// effects.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__EFFECTS_H
#define MUD98__EFFECTS_H

#include "merc.h"

void acid_effect(void* vo, LEVEL level, int dam, int target);
void cold_effect(void* vo, LEVEL level, int dam, int target);
void fire_effect(void* vo, LEVEL level, int dam, int target);
void poison_effect(void* vo, LEVEL level, int dam, int target);
void shock_effect(void* vo, LEVEL level, int dam, int target);

#endif // !MUD98__EFFECTS_H
