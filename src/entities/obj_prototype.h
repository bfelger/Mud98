////////////////////////////////////////////////////////////////////////////////
// entities/obj_prototype.h
////////////////////////////////////////////////////////////////////////////////

typedef struct obj_prototype_t ObjPrototype;

#pragma once
#ifndef MUD98__ENTITIES__OBJ_PROTOTYPE_H
#define MUD98__ENTITIES__OBJ_PROTOTYPE_H

#include <merc.h>

#include <data/item.h>

#include <entities/affect.h>
#include <entities/area.h>
#include <entities/entity.h>
#include <entities/extra_desc.h>

#include <lox/lox.h>
#include <lox/ordered_table.h>

typedef struct obj_prototype_t {
    Entity header;
    ObjPrototype* next;
    ExtraDesc* extra_desc;
    Affect* affected;
    AreaData* area;
    char* short_descr;
    char* description;
    char* material;
    
    // Type-specific value semantics via anonymous union
    // All members share the same 20-byte storage (5 * int32_t)
    // (Identical to Object union - see entities/object.h for full documentation)
    union {
        int value[5];  // Generic access (backward compatible)
        struct { int unused0; int unused1; int hours; int unused3; int unused4; } light;
        struct { int level; int spell1; int spell2; int spell3; int unused4; } scroll;
        struct { int level; int max_charges; int charges; int spell; int unused4; } wand;
        struct { int level; int max_charges; int charges; int spell; int unused4; } staff;
        struct { int unused0; int unused1; int unused2; int weapon_type; int flags; } weapon;
        struct { int ac_pierce; int ac_bash; int ac_slash; int ac_exotic; int unused4; } armor;
        struct { int capacity; int flags; int key_vnum; int max_item_weight; int weight_mult; } container;
        struct { int capacity; int current; int liquid_type; int poisoned; int unused4; } drink_con;
        struct { int hours_full; int hours_hunger; int unused2; int poisoned; int unused4; } food;
        struct { int copper; int silver; int gold; int unused3; int unused4; } money;
        struct { int capacity; int current; int liquid_type; int unused3; int unused4; } fountain;
        struct { int level; int spell1; int spell2; int spell3; int unused4; } pill;
        struct { int level; int spell1; int spell2; int spell3; int unused4; } potion;
        struct { int max_people; int max_weight; int flags; int heal_rate; int mana_rate; } furniture;
        struct { int charges; int exit_flags; int gate_flags; int destination; int unused4; } portal;
    };
    
    int cost;
    FLAGS extra_flags;
    FLAGS wear_flags;
    LEVEL level;
    int16_t reset_num;
    int16_t condition;
    int16_t count;
    int16_t weight;
    ItemType item_type;
} ObjPrototype;

#define FOR_EACH_OBJ_PROTO(o) \
    for (GlobalObjProtoIter o##_iter = make_global_obj_proto_iter(); \
        (o = global_obj_proto_iter_next(&o##_iter)) != NULL; )

void free_object_prototype(ObjPrototype* pObj);
ObjPrototype* get_object_prototype(VNUM vnum);
ObjPrototype* new_object_prototype();
void load_objects(FILE* fp);

typedef struct {
    OrderedTableIter iter;
} GlobalObjProtoIter;

void init_global_obj_protos(void);
void free_global_obj_protos(void);
ObjPrototype* global_obj_proto_get(VNUM vnum);
bool global_obj_proto_set(ObjPrototype* proto);
bool global_obj_proto_remove(VNUM vnum);
int global_obj_proto_count(void);
GlobalObjProtoIter make_global_obj_proto_iter(void);
ObjPrototype* global_obj_proto_iter_next(GlobalObjProtoIter* iter);
OrderedTable snapshot_global_obj_protos(void);
void restore_global_obj_protos(OrderedTable snapshot);
void mark_global_obj_protos(void);
extern int obj_proto_count;
extern int obj_proto_perm_count;
extern VNUM top_vnum_obj;

#endif // !MUD98__ENTITIES__OBJ_PROTOTYPE_H
