////////////////////////////////////////////////////////////////////////////////
// array.c
////////////////////////////////////////////////////////////////////////////////

#include "lox/common.h"

#include "lox/array.h"
#include "lox/object.h"
#include "lox/memory.h"
#include "lox/value.h"

ValueArray* new_obj_array()
{
    ValueArray* array_ = ALLOCATE_OBJ(ValueArray, OBJ_ARRAY);
    init_value_array(array_);
    return array_;
}

void init_value_array(ValueArray* array)
{
    array->obj.type = OBJ_ARRAY;
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void write_value_array(ValueArray* array, Value value)
{
    if (array->capacity < array->count + 1) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(Value, array->values, old_capacity,
            array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void free_value_array(ValueArray* array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    init_value_array(array);
}

bool value_array_contains(ValueArray* array, Value value)
{
    for (int i = 0; i < array->count; i++) {
        if (values_equal(array->values[i], value))
            return true;
    }

    return false;
}

void remove_array_value(ValueArray* array, Value value)
{
    int found = 0;

    for (int i = 0; i < array->count; i++) {
        if (values_equal(array->values[i], value))
            found++;
        else if (found > 0) {
            array->values[i - found] = array->values[i];
            if (i >= array->count - found)
                array->values[i] = NIL_VAL;
        }
    }

    array->count -= found;
}

void remove_array_index(ValueArray* array, int index)
{
    if (index >= array->count)
        return;

    for (int i = index + 1; i < array->count; i++) {
        array->values[i - 1] = array->values[i];
        if (i == array->count - 1)
            array->values[i] = NIL_VAL;
    }

    array->count--;
}
