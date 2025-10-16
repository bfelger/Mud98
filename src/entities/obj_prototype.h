////////////////////////////////////////////////////////////////////////////////
// obj_prototype.h
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

typedef struct obj_prototype_t {
    Entity header;
    ObjPrototype* next;
    ExtraDesc* extra_desc;
    Affect* affected;
    AreaData* area;
    char* short_descr;
    char* description;
    char* material;
    int value[5];
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
    for (int o##_idx = 0, o##_l_count = 0; o##_l_count < obj_protos.count; ++o##_idx) \
        if (!IS_NIL((&obj_protos.entries[o##_idx])->key) \
            && !IS_NIL((&obj_protos.entries[o##_idx])->value) \
            && IS_OBJ_PROTO((&obj_protos.entries[o##_idx])->value) \
            && (o = AS_OBJ_PROTO(obj_protos.entries[o##_idx].value)) != NULL \
            && ++o##_l_count)

void free_object_prototype(ObjPrototype* pObj);
ObjPrototype* get_object_prototype(VNUM vnum);
ObjPrototype* new_object_prototype();
void load_objects(FILE* fp);

extern Table obj_protos;
extern int obj_proto_count;
extern int obj_proto_perm_count;
extern VNUM top_vnum_obj;

#endif // !MUD98__ENTITIES__OBJ_PROTOTYPE_H
