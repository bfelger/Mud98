////////////////////////////////////////////////////////////////////////////////
// obj_prototype.h
////////////////////////////////////////////////////////////////////////////////

typedef struct obj_prototype_t ObjPrototype;

#pragma once
#ifndef MUD98__ENTITIES__OBJ_PROTOTYPE_H
#define MUD98__ENTITIES__OBJ_PROTOTYPE_H

#include "merc.h"

#include "data/item.h"

#include "affect.h"
#include "area.h"
#include "extra_desc.h"

#include "lox/lox.h"

typedef struct obj_prototype_t {
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

void free_object_prototype(ObjPrototype* pObj);
ObjPrototype* get_object_prototype(VNUM vnum);
ObjPrototype* new_object_prototype();
void load_objects(FILE* fp);

extern ObjPrototype* obj_proto_hash[];
extern int obj_proto_count;
extern int obj_proto_perm_count;
extern VNUM top_vnum_obj;

#endif // !MUD98__ENTITIES__OBJ_PROTOTYPE_H
