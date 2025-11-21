////////////////////////////////////////////////////////////////////////////////
// entities/mob_prototype.h
// Prototype data for mobile (NPC) Mobile
////////////////////////////////////////////////////////////////////////////////

typedef struct mob_prototype_t MobPrototype;

#pragma once
#ifndef MUD98__ENTITIES__MOB_PROTOTYPE_H
#define MUD98__ENTITIES__MOB_PROTOTYPE_H

#include "area.h"
#include "entity.h"
#include "shop_data.h"

#include <interp.h>
#include <mob_prog.h>

#include <data/damage.h>
#include <data/mobile_data.h>

#include <lox/lox.h>

#include <stdbool.h>

typedef struct mob_prototype_t {
    Entity header;
    MobPrototype* next;
    SpecFunc* spec_fun;
    ShopData* pShop;
    MobProg* mprogs;
    AreaData* area;
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
    int wealth;
    int16_t hit[3];
    int16_t mana[3];
    int16_t damage[3];
    int16_t ac[AC_COUNT];
    VNUM faction_vnum;
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
} MobPrototype;

// Well-known mob IDs
#define MOB_VNUM_FIDO           3090
#define MOB_VNUM_CITYGUARD      3060
#define MOB_VNUM_VAMPIRE        3404
#define MOB_VNUM_PATROLMAN      2106
#define GROUP_VNUM_TROLLS       2100
#define GROUP_VNUM_OGRES        2101

#define FOR_EACH_MOB_PROTO(m) \
    for (int m##_idx = 0, m##_l_count = 0; m##_l_count < mob_protos.count; ++m##_idx) \
        if (!IS_NIL((&mob_protos.entries[m##_idx])->key) \
            && !IS_NIL((&mob_protos.entries[m##_idx])->value) \
            && IS_MOB_PROTO((&mob_protos.entries[m##_idx])->value) \
            && (m = AS_MOB_PROTO(mob_protos.entries[m##_idx].value)) != NULL \
            && ++m##_l_count)

MobPrototype* new_mob_prototype();
void free_mob_prototype(MobPrototype* p_mob_proto);
MobPrototype* get_mob_prototype(VNUM vnum);
void load_mobiles(FILE* fp);
void recalc(MobPrototype* pMob);

extern Table mob_protos;
extern MobPrototype* mob_prototype_free;

extern int mob_proto_count;
extern int mob_proto_perm_count;

extern VNUM top_vnum_mob;      // OLC

#endif // !MUD98__ENTITIES__MOB_PROTOTYPE_H
