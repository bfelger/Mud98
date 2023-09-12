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

#include "area_data.h"

#include <stdbool.h>

typedef struct mob_prototype_t {
    MobPrototype* next;
    SpecFunc* spec_fun;
    SHOP_DATA* pShop;
    MPROG_LIST* mprogs;
    AreaData* area;
    char* name;
    char* short_descr;
    char* long_descr;
    char* description;
    char* material;
    int act;
    int affected_by;
    int off_flags;
    int imm_flags;
    int res_flags;
    int vuln_flags;
    int wealth;
    int form;
    int parts;
    int mprog_flags;
    VNUM vnum;
    int16_t hit[3];
    int16_t mana[3];
    int16_t damage[3];
    int16_t ac[4];
    int16_t group;
    int16_t count;
    int16_t killed;
    int16_t alignment;
    int16_t level;
    int16_t hitroll;
    int16_t dam_type;
    int16_t start_pos;
    int16_t default_pos;
    int16_t sex;
    int16_t race;
    int16_t size;
    int16_t reset_num;
    bool new_format;
} MobPrototype;

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
