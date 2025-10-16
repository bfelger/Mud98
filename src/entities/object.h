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
#include "lox/list.h"

#include <stdio.h>

typedef struct object_t {
    Entity header;
    Node* obj_list_node;
    List objects;
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

#define CAN_WEAR(obj, part)       (IS_SET((obj)->wear_flags, (part)))
#define IS_OBJ_STAT(obj, stat)    (IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj, stat) (IS_SET((obj)->value[4], (stat)))
#define WEIGHT_MULT(obj)                                                       \
    ((obj)->item_type == ITEM_CONTAINER ? (obj)->value[4] : 100)

#define FOR_EACH_GLOBAL_OBJ(obj) \
    if (obj_list.front == NULL) \
        obj = NULL; \
    else if ((obj = AS_OBJECT(obj_list.front->value)) != NULL) \
        for (struct { Node* node; Node* next; } obj##_loop = { obj_list.front, obj_list.front->next }; \
            obj##_loop.node != NULL; \
            obj##_loop.node = obj##_loop.next, \
                obj##_loop.next = obj##_loop.next ? obj##_loop.next->next : NULL, \
                obj = obj##_loop.node != NULL ? AS_OBJECT(obj##_loop.node->value) : NULL) \
            if (obj != NULL)

#define OBJ_HAS_OBJS(obj) \
    ((obj)->objects.count > 0)

#define FOR_EACH_OBJ_CONTENT(content, obj) \
    if ((obj)->objects.front == NULL) \
        content = NULL; \
    else if ((content = AS_OBJECT((obj)->objects.front->value)) != NULL) \
        for (struct { Node* node; Node* next; } content##_loop = { (obj)->objects.front, (obj)->objects.front->next }; \
            content##_loop.node != NULL; \
            content##_loop.node = content##_loop.next, \
                content##_loop.next = content##_loop.next ? content##_loop.next->next : NULL, \
                content = content##_loop.node != NULL ? AS_OBJECT(content##_loop.node->value) : NULL) \
            if (content != NULL)

void clone_object(Object* parent, Object* clone);
Object* create_object(ObjPrototype* obj_proto, LEVEL level);
void free_object(Object* obj);
Object* new_object();

extern List obj_free;
extern List obj_list;

////////////////////////////////////////////////////////////////////////////////
// Lox implementation
////////////////////////////////////////////////////////////////////////////////

//void init_object_class();
//Value create_object_value(Object* object);

#endif // !MUD98__ENTITIES__OBJECT_H
