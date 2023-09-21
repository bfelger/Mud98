////////////////////////////////////////////////////////////////////////////////
// skills.h
////////////////////////////////////////////////////////////////////////////////

typedef struct skill_hash_t SkillHash;

#pragma once
#ifndef MUD98__SKILLS_H
#define MUD98__SKILLS_H

#include "merc.h"

#include "gsn.h"
#include "interp.h"

#include "entities/char_data.h"

#include "data/class.h"

typedef struct skill_hash_t {
    SkillHash* next;
    SKNUM sn;
} SkillHash;

bool parse_gen_groups(CharData* ch, char* argument);
void list_group_costs(CharData* ch);
int exp_per_level(CharData* ch, int points);
void check_improve(CharData* ch, SKNUM sn, bool success, int multiplier);
SKNUM group_lookup(const char* name);
void gn_add(CharData* ch, SKNUM gn);
void gn_remove(CharData* ch, SKNUM gn);
void group_add(CharData* ch, const char* name, bool deduct);
void group_remove(CharData* ch, const char* name);

#endif // !MUD98__SKILLS_H