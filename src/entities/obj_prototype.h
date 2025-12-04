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
