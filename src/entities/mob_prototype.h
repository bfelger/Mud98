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
#include <lox/ordered_table.h>

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
    VNUM group;
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

#define FOR_EACH_MOB_PROTO(m) \
    for (GlobalMobProtoIter m##_iter = make_global_mob_proto_iter(); \
        (m = global_mob_proto_iter_next(&m##_iter)) != NULL; )

MobPrototype* new_mob_prototype();
void free_mob_prototype(MobPrototype* p_mob_proto);
MobPrototype* get_mob_prototype(VNUM vnum);
void load_mobiles(FILE* fp);
void recalc(MobPrototype* pMob);

typedef struct {
    OrderedTableIter iter;
} GlobalMobProtoIter;

void init_global_mob_protos(void);
void free_global_mob_protos(void);
MobPrototype* global_mob_proto_get(VNUM vnum);
bool global_mob_proto_set(MobPrototype* proto);
bool global_mob_proto_remove(VNUM vnum);
int global_mob_proto_count(void);
GlobalMobProtoIter make_global_mob_proto_iter(void);
MobPrototype* global_mob_proto_iter_next(GlobalMobProtoIter* iter);
OrderedTable snapshot_global_mob_protos(void);
void restore_global_mob_protos(OrderedTable snapshot);
void mark_global_mob_protos(void);
extern MobPrototype* mob_prototype_free;

extern int mob_proto_count;
extern int mob_proto_perm_count;

extern VNUM top_vnum_mob;      // OLC

#endif // !MUD98__ENTITIES__MOB_PROTOTYPE_H
