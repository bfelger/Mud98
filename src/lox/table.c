////////////////////////////////////////////////////////////////////////////////
// table.h
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>

#include "lox/memory.h"
#include "lox/object.h"
#include "lox/table.h"
#include "lox/value.h"

#define TABLE_MAX_LOAD 0.75

Table* new_obj_table()
{
    Table* table = ALLOCATE_OBJ(Table, OBJ_TABLE);
    init_table(table);
    return table;
}

void init_table(Table* table)
{
    table->obj.type = OBJ_TABLE;
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_table(Table* table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    init_table(table);
}

static Entry* find_entry(Entry* entries, int capacity, ObjString* key)
{
    uint32_t index = key->hash & (capacity - 1);
    Entry* tombstone = NULL;
    for (;;) {
        Entry* entry = &entries[index];
        if (entry->key == NIL_VAL) {
            if (IS_NIL(entry->value)) {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            }
            else {
                // We found a tombstone.
                if (tombstone == NULL)
                    tombstone = entry;
            }
        }
        else if (IS_STRING(entry->key) && AS_STRING(entry->key) == key) {
            // We found the key.
            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}

static Entry* find_entry_vnum(Entry* entries, int capacity, int32_t key)
{
    uint32_t index = (uint32_t)key & (capacity - 1);

    Entry* tombstone = NULL;
    for (;;) {
        Entry* entry = &entries[index];
        if (entry->key == NIL_VAL) {
            if (IS_NIL(entry->value)) {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            }
            else {
                // We found a tombstone.
                if (tombstone == NULL)
                    tombstone = entry;
            }
        }
        else if (IS_INT(entry->key) && AS_INT(entry->key) == key) {
            // We found the key.
            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}

bool table_get_entry(Table* table, ObjString* key, Entry** out_entry)
{
    if (table->count == 0)
        return false;

    Entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NIL_VAL)
        return false;

    *out_entry = entry;
    return true;
}

bool table_get(Table* table, ObjString* key, Value* value)
{
    if (table->count == 0)
        return false;

    Entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NIL_VAL)
        return false;

    *value = entry->value;
    return true;
}

bool table_get_vnum(Table* table, int32_t key, Value* value)
{
    if (table->count == 0)
        return false;

    Entry* entry = find_entry_vnum(table->entries, table->capacity, key);
    if (entry->key == NIL_VAL)
        return false;

    *value = entry->value;
    return true;
}

static void adjust_capacity(Table* table, int capacity)
{
    Entry* entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NIL_VAL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key == NIL_VAL) 
            continue;

        Entry* dest;
        if (IS_STRING(entry->key)) {
            dest = find_entry(entries, capacity, AS_STRING(entry->key));
            dest->key = entry->key;
        }
        else {
            dest = find_entry_vnum(entries, capacity, AS_INT(entry->key));
            dest->key = entry->key;
        }
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool table_set(Table* table, ObjString* key, Value value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        // Because memory comes from Mud98's resource bucket, we need to enforce
        // an early GC is the strings table gets too big.
        if (GROW_CAPACITY(table->capacity) * sizeof(Entry) > 65536 * 4)
            collect_garbage();
    }

    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }

    Entry* entry = find_entry(table->entries, table->capacity, key);
    bool is_new_key = IS_NIL(entry->key);
    if (is_new_key && IS_NIL(entry->value))
        table->count++;

    entry->key = OBJ_VAL(key);
    entry->value = value;
    return is_new_key;
}

bool table_set_vnum(Table* table, int32_t key, Value value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    // Because memory comes from Mud98's resource bucket, we need to enforce
    // an early GC is the strings table gets too big.
        if (GROW_CAPACITY(table->capacity) * sizeof(Entry) >= 65536 * 4)
            collect_garbage();
    }

    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }

    Entry* entry = find_entry_vnum(table->entries, table->capacity, key);
    bool is_new_key = IS_NIL(entry->key);
    if (is_new_key && IS_NIL(entry->value))
        table->count++;

    entry->key = INT_VAL(key);
    entry->value = value;
    return is_new_key;
}

bool table_delete(Table* table, ObjString* key)
{
    if (table->count == 0)
        return false;

    // Find the entry.
    Entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NIL_VAL)
        return false;

    // Place a tombstone in the entry.
    entry->key = NIL_VAL;
    entry->value = BOOL_VAL(true);
    return true;
}

bool table_delete_vnum(Table* table, int32_t key)
{
    if (table->count == 0) 
        return false;

    // Find the entry.
    Entry* entry = find_entry_vnum(table->entries, table->capacity, key);
    if (entry->key == NIL_VAL) 
        return false;

    // Place a tombstone in the entry.
    entry->key = NIL_VAL;
    entry->value = BOOL_VAL(true);
    return true;
}

void table_add_all(Table* from, Table* to)
{
    for (int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (IS_STRING(entry->key)) {
            table_set(to, AS_STRING(entry->key), entry->value);
        }
        else if (IS_INT(entry->key)) {
            table_set_vnum(to, AS_INT(entry->key), entry->value);
        }
    }
}

ObjString* table_find_string(Table* table, const char* chars, int length,
    uint32_t hash)
{
    if (table->count == 0)
        return NULL;

    uint32_t index = hash & (table->capacity - 1);
    for (;;) {
        Entry* entry = &table->entries[index];
        if (entry->key == NIL_VAL) {
          // Stop if we find an empty non-tombstone entry.
            if (IS_NIL(entry->value))
                return NULL;
        }
        else if (IS_STRING(entry->key)) {
            ObjString* key_str = AS_STRING(entry->key);
            if (key_str->length == length &&
                key_str->hash == hash &&
                memcmp(key_str->chars, chars, length) == 0) {
                // We found it.
                return key_str;
            }
        }

        index = (index + 1) & (table->capacity - 1);
    }
}

void table_remove_white(Table* table)
{
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (IS_STRING(entry->key)) {
            ObjString* str = AS_STRING(entry->key);
            if (!str->obj.is_marked && !IS_PERM_STRING(str->chars)) {
                table_delete(table, AS_STRING(entry->key));
            }
        }
    }
}

void mark_table(Table* table)
{
    mark_object((Obj*)table);
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (IS_STRING(entry->key))
            mark_object(AS_OBJ(entry->key));
        mark_value(entry->value);
    }
}
