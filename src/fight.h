////////////////////////////////////////////////////////////////////////////////
// fight.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__FIGHT_H
#define MUD98__FIGHT_H

#include "merc.h"

#include "entities/char_data.h"

bool is_safe(CharData* ch, CharData* victim);
bool is_safe_spell(CharData* ch, CharData* victim, bool area);
void violence_update(void);
void multi_hit(CharData* ch, CharData* victim, SKNUM dt);
bool damage(CharData* ch, CharData* victim, int dam, SKNUM dt,
    DamageType dam_type, bool show);
void update_pos(CharData* victim);
void stop_fighting(CharData* ch, bool fBoth);
void check_killer(CharData* ch, CharData* victim);

#endif // !MUD98__FIGHT_H
