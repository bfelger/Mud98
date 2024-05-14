////////////////////////////////////////////////////////////////////////////////
// obj_prototype.h
////////////////////////////////////////////////////////////////////////////////

typedef struct obj_prototype_t ObjPrototype;

#pragma once
#ifndef MUD98__ENTITIES__OBJ_PROTOTYPE_H
#define MUD98__ENTITIES__OBJ_PROTOTYPE_H

#include "merc.h"

#include "data/item.h"

#include "entities/affect.h"
#include "entities/area.h"
#include "entities/entity.h"
#include "entities/extra_desc.h"

#include "lox/lox.h"

typedef struct obj_prototype_t {
    EntityHeader header;
    ObjPrototype* next;
    ExtraDesc* extra_desc;
    Affect* affected;
    AreaData* area;
    String* name;
    char* short_descr;
    char* description;
    char* material;
    int value[5];
    int cost;
    FLAGS extra_flags;
    FLAGS wear_flags;
    VNUM vnum;
    LEVEL level;
    int16_t reset_num;
    int16_t condition;
    int16_t count;
    int16_t weight;
    ItemType item_type;
} ObjPrototype;

#define FOR_EACH_OBJ_PROTO(p) \
    for (int p##_hash_idx = 0; p##_hash_idx < MAX_KEY_HASH; ++p##_hash_idx) \
        FOR_EACH(p, obj_proto_hash[p##_hash_idx])

void free_object_prototype(ObjPrototype* pObj);
ObjPrototype* get_object_prototype(VNUM vnum);
ObjPrototype* new_object_prototype();
void load_objects(FILE* fp);

extern ObjPrototype* obj_proto_hash[];
extern int obj_proto_count;
extern int obj_proto_perm_count;
extern VNUM top_vnum_obj;

#endif // !MUD98__ENTITIES__OBJ_PROTOTYPE_H
