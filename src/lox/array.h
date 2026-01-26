////////////////////////////////////////////////////////////////////////////////
// array.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_array_h
#define clox_arrah_h

#include "object.h"
#include "value.h"

typedef struct {
    Obj obj;
    int capacity;
    int count;
    Value* values;
} ValueArray;

ValueArray* new_obj_array(void);
void init_value_array(ValueArray* array);
void write_value_array(ValueArray* array, Value value);
void free_value_array(ValueArray* array); 
bool value_array_contains(ValueArray* array, Value value);
void remove_array_value(ValueArray* array, Value value);
void remove_array_index(ValueArray* array, int index);

#endif // !clox_array_h
