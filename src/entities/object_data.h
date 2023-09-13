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

// Well-known object IDs
#define OBJ_VNUM_SILVER_ONE     1
#define OBJ_VNUM_GOLD_ONE       2
#define OBJ_VNUM_GOLD_SOME      3
#define OBJ_VNUM_SILVER_SOME    4
#define OBJ_VNUM_COINS          5

#define OBJ_VNUM_CORPSE_NPC     10
#define OBJ_VNUM_CORPSE_PC      11
#define OBJ_VNUM_SEVERED_HEAD   12
#define OBJ_VNUM_TORN_HEART     13
#define OBJ_VNUM_SLICED_ARM     14
#define OBJ_VNUM_SLICED_LEG     15
#define OBJ_VNUM_GUTS           16
#define OBJ_VNUM_BRAINS         17

#define OBJ_VNUM_MUSHROOM       20
#define OBJ_VNUM_LIGHT_BALL     21
#define OBJ_VNUM_SPRING         22
#define OBJ_VNUM_DISC           23
#define OBJ_VNUM_PORTAL         25

#define OBJ_VNUM_DUMMY          30

#define OBJ_VNUM_ROSE           1001

#define OBJ_VNUM_PIT            3010

#define OBJ_VNUM_SCHOOL_MACE    3700
#define OBJ_VNUM_SCHOOL_DAGGER  3701
#define OBJ_VNUM_SCHOOL_SWORD   3702
#define OBJ_VNUM_SCHOOL_SPEAR   3717
#define OBJ_VNUM_SCHOOL_STAFF   3718
#define OBJ_VNUM_SCHOOL_AXE     3719
#define OBJ_VNUM_SCHOOL_FLAIL   3720
#define OBJ_VNUM_SCHOOL_WHIP    3721
#define OBJ_VNUM_SCHOOL_POLEARM 3722

#define OBJ_VNUM_SCHOOL_VEST    3703
#define OBJ_VNUM_SCHOOL_SHIELD  3704
#define OBJ_VNUM_SCHOOL_BANNER  3716
#define OBJ_VNUM_MAP            3162

#define OBJ_VNUM_WHISTLE        2116

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
