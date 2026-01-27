////////////////////////////////////////////////////////////////////////////////
// ordered_table.c
////////////////////////////////////////////////////////////////////////////////

#include "ordered_table.h"

#include "memory.h"

#include <string.h>

static int find_index(const OrderedTable* ordered, VNUM key, bool* found)
{
    int lo = 0;
    int hi = ordered->ordered_count - 1;

    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        VNUM mid_key = ordered->ordered[mid].key;
        if (mid_key == key) {
            *found = true;
            return mid;
        }
        if (key < mid_key) {
            hi = mid - 1;
        }
        else {
            lo = mid + 1;
        }
    }

    *found = false;
    return lo;
}

static bool ensure_capacity(OrderedTable* ordered, int needed)
{
    if (needed <= ordered->ordered_capacity)
        return true;

    int old_capacity = ordered->ordered_capacity;
    int new_capacity = GROW_CAPACITY(old_capacity);
    while (new_capacity < needed) {
        new_capacity = GROW_CAPACITY(new_capacity);
    }

    OrderedEntry* resized = GROW_ARRAY(OrderedEntry, ordered->ordered,
        ordered->ordered_capacity, new_capacity);
    if (resized == NULL)
        return false;

    ordered->ordered = resized;
    ordered->ordered_capacity = new_capacity;
    return true;
}

void ordered_table_init(OrderedTable* ordered)
{
    init_table(&ordered->table);
    ordered->ordered = NULL;
    ordered->ordered_count = 0;
    ordered->ordered_capacity = 0;
}

void ordered_table_free(OrderedTable* ordered)
{
    free_table(&ordered->table);
    FREE_ARRAY(OrderedEntry, ordered->ordered, ordered->ordered_capacity);
    ordered->ordered = NULL;
    ordered->ordered_capacity = 0;
    ordered->ordered_count = 0;
}

bool ordered_table_get_vnum(OrderedTable* ordered, VNUM key, Value* value)
{
    return table_get_vnum(&ordered->table, key, value);
}

bool ordered_table_contains_vnum(OrderedTable* ordered, VNUM key)
{
    Value dummy;
    return ordered_table_get_vnum(ordered, key, &dummy);
}

bool ordered_table_set_vnum(OrderedTable* ordered, VNUM key, Value value)
{
    bool found;
    int index = find_index(ordered, key, &found);

    if (!found) {
        if (!ensure_capacity(ordered, ordered->ordered_count + 1))
            return false;
    }

    bool inserted = table_set_vnum(&ordered->table, key, value);

    if (found) {
        ordered->ordered[index].value = value;
    }
    else {
        if (index < ordered->ordered_count) {
            memmove(&ordered->ordered[index + 1], &ordered->ordered[index],
                (size_t)(ordered->ordered_count - index) * sizeof(OrderedEntry));
        }

        ordered->ordered[index].key = key;
        ordered->ordered[index].value = value;
        ordered->ordered_count++;
    }

    return inserted;
}

bool ordered_table_delete_vnum(OrderedTable* ordered, VNUM key)
{
    bool found;
    int index = find_index(ordered, key, &found);
    if (!found)
        return false;

    if (!table_delete_vnum(&ordered->table, key))
        return false;

    if (index < ordered->ordered_count - 1) {
        memmove(&ordered->ordered[index], &ordered->ordered[index + 1],
            (size_t)(ordered->ordered_count - index - 1) * sizeof(OrderedEntry));
    }
    ordered->ordered_count--;
    ordered->ordered[ordered->ordered_count].key = 0;
    ordered->ordered[ordered->ordered_count].value = NIL_VAL;
    return true;
}

int ordered_table_count(const OrderedTable* ordered)
{
    return ordered->ordered_count;
}

OrderedTableIter ordered_table_iter(const OrderedTable* ordered)
{
    OrderedTableIter iter;
    iter.table = ordered;
    iter.index = 0;
    return iter;
}

bool ordered_table_iter_next(OrderedTableIter* iter, VNUM* key, Value* value)
{
    if (iter->table == NULL)
        return false;

    if (iter->index >= iter->table->ordered_count)
        return false;

    const OrderedEntry* entry = &iter->table->ordered[iter->index++];
    if (key)
        *key = entry->key;
    if (value)
        *value = entry->value;
    return true;
}

void mark_ordered_table(OrderedTable* ordered)
{
    if (ordered == NULL)
        return;

    mark_table(&ordered->table);
    for (int i = 0; i < ordered->ordered_count; i++) {
        mark_value(ordered->ordered[i].value);
    }
}
