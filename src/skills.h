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

typedef enum spell_target_t {
    TARGET_CHAR         = 0,
    TARGET_OBJ          = 1,
    TARGET_ROOM         = 2,
    TARGET_NONE         = 3,
} SpellTarget;

typedef enum skill_target_t {
    TAR_IGNORE          = 0,
    TAR_CHAR_OFFENSIVE  = 1,
    TAR_CHAR_DEFENSIVE  = 2,
    TAR_CHAR_SELF       = 3,
    TAR_OBJ_INV         = 4,
    TAR_OBJ_CHAR_DEF    = 5,
    TAR_OBJ_CHAR_OFF    = 6,
} SkillTarget;

struct group_type {
    char* name;
    int16_t rating[ARCH_COUNT];
    char* spells[MAX_IN_GROUP];
};

struct skill_type {
    char* name;                     /* Name of skill		*/
    LEVEL skill_level[ARCH_COUNT];  /* Level needed by class	*/
    int16_t rating[ARCH_COUNT];     /* How hard it is to learn	*/
    SpellFunc* spell_fun;           /* Spell pointer (for spells)	*/
    SkillTarget target;             /* Legal targets		*/
    Position minimum_position;      /* Position for caster / user	*/
    SKNUM* pgsn;                    /* Pointer to associated gsn	*/
    int slot;                       /* Slot for #OBJECT loading	*/
    int min_mana;                   /* Minimum mana used		*/
    int beats;                      /* Waiting time after use	*/
    char* noun_damage;              /* Damage message		*/
    char* msg_off;                  /* Wear off message		*/
    char* msg_obj;                  /* Wear off message for obects	*/
};

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

extern struct group_type* group_table;
extern struct skill_type* skill_table;

extern SKNUM max_skill;
extern int max_group;

#endif // !MUD98__SKILLS_H