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

#include "char_data.h"

/*
 * Prototype for an object.
 */
typedef struct object_prototype_t {
    ObjectPrototype* next;
    EXTRA_DESCR_DATA* extra_descr;
    AFFECT_DATA* affected;
    AREA_DATA* area;        // OLC
    bool new_format;
    char* name;
    char* short_descr;
    char* description;
    VNUM vnum;
    int16_t reset_num;
    char* material;
    int16_t item_type;
    int extra_flags;
    int wear_flags;
    LEVEL level;
    int16_t condition;
    int16_t count;
    int16_t weight;
    int cost;
    int value[5];
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
    ROOM_INDEX_DATA* in_room;
    bool valid;
    bool enchanted;
    char* owner;
    char* name;
    char* short_descr;
    char* description;
    int16_t item_type;
    int extra_flags;
    int wear_flags;
    int16_t wear_loc;
    int16_t weight;
    int cost;
    LEVEL level;
    int16_t condition;
    char* material;
    int16_t timer;
    int value[5];
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
