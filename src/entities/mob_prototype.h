////////////////////////////////////////////////////////////////////////////////
// mob_prototype.h
// Prototype data for mobile (NPC) CharData
////////////////////////////////////////////////////////////////////////////////

typedef struct mob_prototype_t MobPrototype;

#pragma once
#ifndef MUD98__ENTITIES__MOB_PROTOTYPE_H
#define MUD98__ENTITIES__MOB_PROTOTYPE_H

#include "merc.h"

#include "interp.h"
#include "mob_prog.h"

#include "area_data.h"
#include "shop_data.h"

#include "data/damage.h"
#include "data/mobile.h"

#include <stdbool.h>

typedef struct mob_prototype_t {
    MobPrototype* next;
    SpecFunc* spec_fun;
    ShopData* pShop;
    MobProg* mprogs;
    AreaData* area;
    char* name;
    char* short_descr;
    char* long_descr;
    char* description;
    char* material;
    FLAGS act_flags;
    FLAGS affect_flags;
    FLAGS atk_flags;
    FLAGS imm_flags;
    FLAGS res_flags;
    FLAGS vuln_flags;
    FLAGS form;
    FLAGS parts;
    FLAGS mprog_flags;
    VNUM vnum;
    int wealth;
    int16_t hit[3];
    int16_t mana[3];
    int16_t damage[3];
    int16_t ac[AC_COUNT];
    int16_t group;
    int16_t count;
    int16_t killed;
    int16_t alignment;
    int16_t level;
    int16_t hitroll;
    int16_t dam_type;
    Position start_pos;
    Position default_pos;
    Sex sex;
    int16_t race;
    MobSize size;
    int16_t reset_num;
    bool new_format;
} MobPrototype;

// Well-known mob IDs
#define MOB_VNUM_FIDO           3090
#define MOB_VNUM_CITYGUARD      3060
#define MOB_VNUM_VAMPIRE        3404
#define MOB_VNUM_PATROLMAN      2106
#define GROUP_VNUM_TROLLS       2100
#define GROUP_VNUM_OGRES        2101

void convert_mobile(MobPrototype* p_mob_proto);
CharData* create_mobile(MobPrototype* p_mob_proto);
void free_mob_prototype(MobPrototype* p_mob_proto);
long get_mob_id();
MobPrototype* get_mob_prototype(VNUM vnum);
void load_mobiles(FILE* fp);
void load_old_mob(FILE* fp);
MobPrototype* new_mob_prototype();
void recalc(MobPrototype* pMob);

extern MobPrototype* mob_prototype_hash[];
extern MobPrototype* mob_prototype_free;

extern int top_mob_prototype;
extern int newmobs;
extern VNUM top_vnum_mob;      // OLC

#endif // !MUD98__ENTITIES__MOB_PROTOTYPE_H
