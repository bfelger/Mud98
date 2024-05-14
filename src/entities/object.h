////////////////////////////////////////////////////////////////////////////////
// object.h
// Utilities for handling in-game objects
////////////////////////////////////////////////////////////////////////////////

typedef struct object_t Object;

#pragma once
#ifndef MUD98__ENTITIES__OBJECT_H
#define MUD98__ENTITIES__OBJECT_H

#include "merc.h"

#include "data/item.h"

#include "entities/affect.h"
#include "entities/area.h"
#include "entities/entity.h"
#include "entities/extra_desc.h"
#include "entities/mobile.h"
#include "entities/obj_prototype.h"
#include "entities/reset.h"
#include "entities/room.h"

#include "lox/lox.h"

#include <stdio.h>

typedef struct object_t {
    EntityHeader header;
    Object* next;
    Object* next_content;
    Object* contains;
    Object* in_obj;
    Object* on;
    Mobile* carried_by;
    ExtraDesc* extra_desc;
    Affect* affected;
    ObjPrototype* prototype;
    Room* in_room;
    String* owner;
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
} Object;

#define FOR_EACH_CONTENT(i, c) \
    for ((i) = (c); (i) != NULL; (i) = i->next_content)

#define CAN_WEAR(obj, part)       (IS_SET((obj)->wear_flags, (part)))
#define IS_OBJ_STAT(obj, stat)    (IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj, stat) (IS_SET((obj)->value[4], (stat)))
#define WEIGHT_MULT(obj)                                                       \
    ((obj)->item_type == ITEM_CONTAINER ? (obj)->value[4] : 100)

void clone_object(Object* parent, Object* clone);
Object* create_object(ObjPrototype* obj_proto, LEVEL level);
void free_object(Object* obj);
Object* new_object();

extern int obj_count;
extern int obj_perm_count;

extern Object* obj_free;
extern Object* obj_list;

////////////////////////////////////////////////////////////////////////////////
// Lox implementation
////////////////////////////////////////////////////////////////////////////////

//void init_object_class();
//Value create_object_value(Object* object);

#endif // !MUD98__ENTITIES__OBJECT_H
