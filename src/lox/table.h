////////////////////////////////////////////////////////////////////////////////
// table.h
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_table_h
#define clox_table_h

#include "lox/common.h"
#include "lox/object.h"
#include "lox/value.h"

typedef struct {
    Value key;
    Value value;
} Entry;

typedef struct {
    Obj obj;
    int count;
    int capacity;
    Entry* entries;
} Table;

void init_table(Table* table);
void free_table(Table* table);
bool table_get(Table* table, ObjString* key, Value* value);
bool table_get_entry(Table* table, ObjString* key, Entry** out_entry);
bool table_get_vnum(Table* table, int32_t key, Value* value);
bool table_set(Table* table, ObjString* key, Value value);
bool table_set_vnum(Table* table, int32_t key, Value value);
bool table_delete(Table* table, ObjString* key);
bool table_delete_vnum(Table* table, int32_t key);
void table_add_all(Table* from, Table* to);
ObjString* table_find_string(Table* table, const char* chars, int length, 
    uint32_t hash);
void table_remove_white(Table* table);
void mark_table(Table* table);

#endif
