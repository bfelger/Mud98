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

struct group_type {
    char* name;
    int rating[MAX_CLASS];
    char* spells[MAX_IN_GROUP];
};

struct skill_type {
    char* name; /* Name of skill		*/
    LEVEL skill_level[MAX_CLASS]; /* Level needed by class	*/
    int rating[MAX_CLASS]; /* How hard it is to learn	*/
    SpellFunc* spell_fun; /* Spell pointer (for spells)	*/
    int target; /* Legal targets		*/
    int minimum_position; /* Position for caster / user	*/
    SKNUM* pgsn; /* Pointer to associated gsn	*/
    int slot; /* Slot for #OBJECT loading	*/
    int min_mana; /* Minimum mana used		*/
    int beats; /* Waiting time after use	*/
    char* noun_damage; /* Damage message		*/
    char* msg_off; /* Wear off message		*/
    char* msg_obj; /* Wear off message for obects	*/
};

typedef struct skill_hash_t {
    SkillHash* next;
    SKNUM sn;
} SkillHash;

bool parse_gen_groups(CharData* ch, char* argument);
void list_group_costs(CharData* ch);
int exp_per_level(CharData* ch, int points);
void check_improve(CharData* ch, int sn, bool success, int multiplier);
int group_lookup(const char* name);
void gn_add(CharData* ch, int gn);
void gn_remove(CharData* ch, int gn);
void group_add(CharData* ch, const char* name, bool deduct);
void group_remove(CharData* ch, const char* name);

extern struct group_type* group_table;
extern struct skill_type* skill_table;

extern SKNUM max_skill;
extern int max_group;

#endif // !MUD98__SKILLS_H