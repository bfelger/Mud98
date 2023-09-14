////////////////////////////////////////////////////////////////////////////////
// object_data.h
// Utilities for handling in-game objects
////////////////////////////////////////////////////////////////////////////////

typedef struct object_prototype_t ObjectPrototype;
typedef struct object_data_t ObjectData;

#pragma once
#ifndef MUD98__ENTITIES__OBJECT_DATA_H
#define MUD98__ENTITIES__OBJECT_DATA_H

#include "merc.h"

#include "affect_data.h"
#include "area_data.h"
#include "char_data.h"
#include "extra_desc.h"
#include "room_data.h"

#include "data/item.h"

#include <stdio.h>

typedef struct object_prototype_t {
    ObjectPrototype* next;
    ExtraDesc* extra_desc;
    AffectData* affected;
    AreaData* area;        // OLC
    char* name;
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
    bool new_format;
} ObjectPrototype;

typedef struct object_data_t {
    ObjectData* next;
    ObjectData* next_content;
    ObjectData* contains;
    ObjectData* in_obj;
    ObjectData* on;
    CharData* carried_by;
    ExtraDesc* extra_desc;
    AffectData* affected;
    ObjectPrototype* prototype;
    RoomData* in_room;
    char* owner;
    char* name;
    char* short_descr;
    char* description;
    char* material;
    int value[5];
    int cost;
    FLAGS extra_flags;
    FLAGS wear_flags;
    LEVEL level;
    int16_t condition;
    int16_t weight;
    int16_t timer;
    ItemType item_type;
    WearLocation wear_loc;
    bool enchanted;
    bool valid;
} ObjectData;

#define CAN_WEAR(obj, part)       (IS_SET((obj)->wear_flags, (part)))
#define IS_OBJ_STAT(obj, stat)    (IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj, stat) (IS_SET((obj)->value[4], (stat)))
#define WEIGHT_MULT(obj)                                                       \
    ((obj)->item_type == ITEM_CONTAINER ? (obj)->value[4] : 100)

void clone_object(ObjectData* parent, ObjectData* clone);
void convert_object(ObjectPrototype* obj_proto);
void convert_objects();
ObjectData* create_object(ObjectPrototype* obj_proto, LEVEL level);
void free_object(ObjectData* obj);
void free_object_prototype(ObjectPrototype* pObj);
ObjectPrototype* get_object_prototype(VNUM vnum);
void load_objects(FILE* fp);
ObjectData* new_object();
ObjectPrototype* new_object_prototype();

extern ObjectPrototype* object_prototype_hash[];
extern VNUM top_vnum_obj;
extern int top_object_prototype;
extern int newobjs;
extern ObjectData* object_free;
extern ObjectData* object_list;

#endif // !MUD98__ENTITIES__OBJECT_DATA_H
