////////////////////////////////////////////////////////////////////////////////
// skill.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD86__DATA__SKILL_H
#define MUD86__DATA__SKILL_H

#include "merc.h"

#include "array.h"
#include "class.h"
#include "mobile.h"
#include "spell.h"

#define DEFAULT_SKILL_RATING 0
#define DEFAULT_SKILL_LEVEL 53

typedef int16_t SkillRating;
DEFINE_ARRAY(SkillRating, DEFAULT_SKILL_RATING);
DEFINE_ARRAY(LEVEL, DEFAULT_SKILL_LEVEL);

#define SKILL_LEVEL(skill, ch) GET_ELEM(&skill_table[skill].skill_level, ch->ch_class)
#define SKILL_RATING(skill, ch) GET_ELEM(&skill_table[skill].rating, ch->ch_class)
#define SKILL_GROUP_RATING(skill, ch) GET_ELEM(&skill_group_table[skill].rating, ch->ch_class)

typedef struct skill_t {
    char* name;                     // Name of skill
    ARRAY(LEVEL) skill_level;       // Level needed by class
    ARRAY(SkillRating) rating;      // How hard it is to learn
    SpellFunc* spell_fun;           // Spell pointer (for spells)
    SkillTarget target;             // Legal targets
    Position minimum_position;      // Position for caster / user
    SKNUM* pgsn;                    // Pointer to associated gsn
    int slot;                       // Slot for #OBJECT loading
    int min_mana;                   // Minimum mana used
    int beats;                      // Waiting time after use
    char* noun_damage;              // Damage message
    char* msg_off;                  // Wear off message
    char* msg_obj;                  // Wear off message for obects
} Skill;

typedef struct skill_group_t {
    char* name;
    ARRAY(SkillRating) rating;      // How hard it is to learn
    char* skills[MAX_IN_GROUP];
} SkillGroup;

void load_skill_table();
void load_skill_group_table();
void save_skill_table();
void save_skill_group_table();
SKNUM skill_lookup(const char* name);
SKNUM* gsn_lookup(char* argument);
char* gsn_name(SKNUM* pgsn);

extern Skill* skill_table;
extern int skill_count;

extern SkillGroup* skill_group_table;
extern int skill_group_count;

#endif // !MUD86__DATA__SKILL_H
