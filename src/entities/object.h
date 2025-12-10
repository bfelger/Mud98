////////////////////////////////////////////////////////////////////////////////
// entities/object.h
// Utilities for handling in-game objects
////////////////////////////////////////////////////////////////////////////////

typedef struct object_t Object;

#pragma once
#ifndef MUD98__ENTITIES__OBJECT_H
#define MUD98__ENTITIES__OBJECT_H

#include "affect.h"
#include "area.h"
#include "entity.h"
#include "extra_desc.h"
#include "mobile.h"
#include "obj_prototype.h"
#include "reset.h"
#include "room.h"

#include <data/item.h>

#include <lox/lox.h>
#include <lox/list.h>

#include <assert.h>
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
    
    // Type-specific value semantics via anonymous union
    // All members share the same 20-byte storage (5 * int32_t)
    union {
        int value[5];  // Generic access (backward compatible)
        
        // ITEM_LIGHT (type 1)
        struct {
            int unused0;
            int unused1;
            int hours;  // Duration: 0=dead, -1=infinite
            int unused3;
            int unused4;
        } light;
        
        // ITEM_SCROLL (type 2)
        struct {
            int level;
            int spell1;
            int spell2;
            int spell3;
            int unused4;
        } scroll;
        
        // ITEM_WAND (type 3)
        struct {
            int level;
            int max_charges;
            int charges;
            int spell;
            int unused4;
        } wand;
        
        // ITEM_STAFF (type 4)
        struct {
            int level;
            int max_charges;
            int charges;
            int spell;
            int unused4;
        } staff;
        
        // ITEM_WEAPON (type 5)
        struct {
            int unused0;
            int unused1;
            int unused2;
            int weapon_type;  // WEAPON_* enum
            int flags;        // WEAPON_* flags
        } weapon;
        
        // ITEM_ARMOR (type 9)
        struct {
            int ac_pierce;
            int ac_bash;
            int ac_slash;
            int ac_exotic;
            int unused4;
        } armor;
        
        // ITEM_CONTAINER (type 15)
        struct {
            int capacity;         // Max weight in pounds
            int flags;            // CONT_* flags
            int key_vnum;         // Key object vnum
            int max_item_weight;  // Max single item weight in pounds
            int weight_mult;      // Weight multiplier percentage (usually 100)
        } container;
        
        // ITEM_DRINK_CON (type 17)
        struct {
            int capacity;     // Max liquid units
            int current;      // Current liquid units
            int liquid_type;  // Index into liquid_table
            int poisoned;     // Non-zero if poisoned
            int unused4;
        } drink_con;
        
        // ITEM_FOOD (type 19)
        struct {
            int hours_full;    // Hours of fullness
            int hours_hunger;  // Hours reduces hunger
            int unused2;
            int poisoned;      // Non-zero if poisoned
            int unused4;
        } food;
        
        // ITEM_MONEY (type 20)
        struct {
            int copper;   // value[0] - MONEY_VALUE_COPPER
            int silver;   // value[1] - MONEY_VALUE_SILVER
            int gold;     // value[2] - MONEY_VALUE_GOLD
            int unused3;
            int unused4;
        } money;
        
        // ITEM_FOUNTAIN (type 25)
        struct {
            int capacity;     // Max liquid units
            int current;      // Current liquid units
            int liquid_type;  // Index into liquid_table
            int unused3;
            int unused4;
        } fountain;
        
        // ITEM_PILL (type 26)
        struct {
            int level;
            int spell1;
            int spell2;
            int spell3;
            int unused4;
        } pill;
        
        // ITEM_POTION (type 10)
        struct {
            int level;
            int spell1;
            int spell2;
            int spell3;
            int unused4;
        } potion;
        
        // ITEM_FURNITURE (type 12)
        struct {
            int max_people;
            int max_weight;
            int flags;        // Furniture position flags
            int heal_rate;
            int mana_rate;
        } furniture;
        
        // ITEM_PORTAL (type 29)
        struct {
            int charges;
            int exit_flags;
            int gate_flags;   // PORTAL_* flags
            int destination;  // Target vnum
            int unused4;
        } portal;
    };
    
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

// Verify that all union members have the same size as the value array
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->light), 
              "light struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->scroll), 
              "scroll struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->wand), 
              "wand struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->staff), 
              "staff struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->weapon), 
              "weapon struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->armor), 
              "armor struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->container), 
              "container struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->drink_con), 
              "drink_con struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->food), 
              "food struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->money), 
              "money struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->fountain), 
              "fountain struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->pill), 
              "pill struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->potion), 
              "potion struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->furniture), 
              "furniture struct must match value[5] size");
static_assert(sizeof(((Object*)0)->value) == sizeof(((Object*)0)->portal), 
              "portal struct must match value[5] size");

// Legacy macros for money values (deprecated - use obj->money.gold etc.)
#define MONEY_VALUE_COPPER 0
#define MONEY_VALUE_SILVER 1
#define MONEY_VALUE_GOLD   2

#define CAN_WEAR(obj, part)       (IS_SET((obj)->wear_flags, (part)))
#define IS_OBJ_STAT(obj, stat)    (IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj, stat) (IS_SET((obj)->weapon.flags, (stat)))
#define WEIGHT_MULT(obj)                                                       \
    ((obj)->item_type == ITEM_CONTAINER ? (obj)->container.weight_mult : 100)

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

#endif // !MUD98__ENTITIES__OBJECT_H
