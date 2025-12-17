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
void one_hit(Mobile* ch, Mobile* victim, int16_t dt, bool secondary);
bool damage(Mobile* ch, Mobile* victim, int dam, int16_t dt, DamageType dam_type, bool show);
void update_pos(Mobile* victim);
void stop_fighting(Mobile* ch, bool fBoth);
bool check_parry(Mobile* ch, Mobile* victim);
bool check_dodge(Mobile* ch, Mobile* victim);
bool check_shield_block(Mobile* ch, Mobile* victim);
void check_killer(Mobile* ch, Mobile* victim);
void make_corpse(Mobile* ch);

#endif // !MUD98__FIGHT_H
