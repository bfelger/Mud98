////////////////////////////////////////////////////////////////////////////////
// skill.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD86__DATA__SKILL_H
#define MUD86__DATA__SKILL_H

typedef struct skill_t Skill;

#include "merc.h"

#include "class.h"
#include "mobile.h"



typedef enum spell_target_t {
    TARGET_CHAR = 0,
    TARGET_OBJ = 1,
    TARGET_ROOM = 2,
    TARGET_NONE = 3,
} SpellTarget;

typedef enum skill_target_t {
    TAR_IGNORE = 0,
    TAR_CHAR_OFFENSIVE = 1,
    TAR_CHAR_DEFENSIVE = 2,
    TAR_CHAR_SELF = 3,
    TAR_OBJ_INV = 4,
    TAR_OBJ_CHAR_DEF = 5,
    TAR_OBJ_CHAR_OFF = 6,
} SkillTarget;

typedef struct skill_t {
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
} Skill;

typedef struct skill_group_t {
    char* name;
    int16_t rating[ARCH_COUNT];
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
extern int max_skill;

extern SkillGroup* skill_group_table;
extern int max_skill_group;

#endif // !MUD86__DATA__SKILL_H
