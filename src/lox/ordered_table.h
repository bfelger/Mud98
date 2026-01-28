////////////////////////////////////////////////////////////////////////////////
// ordered_table.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__LOX__ORDERED_TABLE_H
#define MUD98__LOX__ORDERED_TABLE_H

#include <merc.h>

#include "table.h"
#include "value.h"

#include <stdint.h>

typedef struct {
    VNUM key;
    Value value;
} OrderedEntry;

typedef struct {
    Table table;
    OrderedEntry* ordered;
    int ordered_count;
    int ordered_capacity;
} OrderedTable;

typedef struct {
    const OrderedTable* table;
    int index;
} OrderedTableIter;

void ordered_table_init(OrderedTable* ordered);
void ordered_table_free(OrderedTable* ordered);

bool ordered_table_get_vnum(OrderedTable* ordered, VNUM key, Value* value);
bool ordered_table_set_vnum(OrderedTable* ordered, VNUM key, Value value);
bool ordered_table_delete_vnum(OrderedTable* ordered, VNUM key);
bool ordered_table_contains_vnum(OrderedTable* ordered, VNUM key);
int ordered_table_count(const OrderedTable* ordered);

OrderedTableIter ordered_table_iter(const OrderedTable* ordered);
bool ordered_table_iter_next(OrderedTableIter* iter, VNUM* key, Value* value);

void mark_ordered_table(OrderedTable* ordered);

#endif // !MUD98__LOX__ORDERED_TABLE_H
