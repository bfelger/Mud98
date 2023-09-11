////////////////////////////////////////////////////////////////////////////////
// object_data.h
// Utilities for handling in-game objects
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ENTITIES__OBJECT_DATA_H
#define MUD98__ENTITIES__OBJECT_DATA_H

typedef struct object_prototype_t ObjectPrototype;
typedef struct object_data_t ObjectData;

#include "merc.h"

#include "char_data.h"
#include "room_data.h"

/*
 * Prototype for an object.
 */
typedef struct object_prototype_t {
    ObjectPrototype* next;
    EXTRA_DESCR_DATA* extra_descr;
    AFFECT_DATA* affected;
    AREA_DATA* area;        // OLC
    char* name;
    char* short_descr;
    char* description;
    char* material;
    int value[5];
    int cost;
    int extra_flags;
    int wear_flags;
    VNUM vnum;
    LEVEL level;
    int16_t item_type;
    int16_t reset_num;
    int16_t condition;
    int16_t count;
    int16_t weight;
    bool new_format;
} ObjectPrototype;

typedef struct object_data_t {
    ObjectData* next;
    ObjectData* next_content;
    ObjectData* contains;
    ObjectData* in_obj;
    ObjectData* on;
    CharData* carried_by;
    EXTRA_DESCR_DATA* extra_descr;
    AFFECT_DATA* affected;
    ObjectPrototype* pIndexData;
    RoomData* in_room;
    char* owner;
    char* name;
    char* short_descr;
    char* description;
    char* material;
    int value[5];
    int cost;
    int extra_flags;
    int wear_flags;
    LEVEL level;
    int16_t condition;
    int16_t wear_loc;
    int16_t weight;
    int16_t timer;
    int16_t item_type;
    bool enchanted;
    bool valid;
} ObjectData;

void clone_object(ObjectData* parent, ObjectData* clone);
void convert_object(ObjectPrototype* p_object_prototype);
void convert_objects();
ObjectData* create_object(ObjectPrototype* p_object_prototype, LEVEL level);
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
