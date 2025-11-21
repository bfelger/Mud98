////////////////////////////////////////////////////////////////////////////////
// fight.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__FIGHT_H
#define MUD98__FIGHT_H

#include "merc.h"

#include "entities/mobile.h"

#include "data/damage.h"

bool is_safe(Mobile* ch, Mobile* victim);
bool is_safe_spell(Mobile* ch, Mobile* victim, bool area);
void violence_update(void);
void multi_hit(Mobile* ch, Mobile* victim, int16_t dt);
bool damage(Mobile* ch, Mobile* victim, int dam, int16_t dt, DamageType dam_type, bool show);
void update_pos(Mobile* victim);
void stop_fighting(Mobile* ch, bool fBoth);
void check_killer(Mobile* ch, Mobile* victim);
void make_corpse(Mobile* ch);

#endif // !MUD98__FIGHT_H
