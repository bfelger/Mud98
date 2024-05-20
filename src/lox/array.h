////////////////////////////////////////////////////////////////////////////////
// array.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_array_h
#define clox_arrah_h

#include "lox/object.h"
#include "lox/value.h"

typedef struct {
    Obj obj;
    int capacity;
    int count;
    Value* values;
} ValueArray;

ValueArray* new_obj_array();
void init_value_array(ValueArray* array);
void write_value_array(ValueArray* array, Value value);
void free_value_array(ValueArray* array);
void remove_array_value(ValueArray* array, Value value);
void remove_array_index(ValueArray* array, int index);

#endif // !clox_array_h
