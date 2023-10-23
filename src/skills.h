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

#include "entities/mobile.h"

#include "data/class.h"

typedef struct skill_hash_t {
    SkillHash* next;
    SKNUM sn;
} SkillHash;

bool parse_gen_groups(Mobile* ch, char* argument);
void list_group_costs(Mobile* ch);
int exp_per_level(Mobile* ch, int points);
void check_improve(Mobile* ch, SKNUM sn, bool success, int multiplier);
SKNUM group_lookup(const char* name);
void gn_add(Mobile* ch, SKNUM gn);
void gn_remove(Mobile* ch, SKNUM gn);
void group_add(Mobile* ch, const char* name, bool deduct);
void group_remove(Mobile* ch, const char* name);

#endif // !MUD98__SKILLS_H
