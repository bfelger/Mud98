////////////////////////////////////////////////////////////////////////////////
// value.c
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>

#include "lox/object.h"
#include "lox/memory.h"
#include "lox/value.h"

void init_value_array(ValueArray* array)
{
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

char* string_value(Value value)
{
    // Don't get recursive.
    static char buf[1024];

    if (IS_BOOL(value)) {
        sprintf(buf, "%s", AS_BOOL(value) ? "true" : "false");
    }
    else if (IS_NIL(value)) {
        sprintf(buf, "nil");
    }
    else if (IS_NUMBER(value)) {
        sprintf(buf, "%g", AS_NUMBER(value));
    }
    else if (IS_STRING(value)) {
        sprintf(buf, "%s", AS_STRING(value)->chars);
    }
    else 
        sprintf(buf, "(object)");

    return buf;
}

void print_value(Value value)
{
#ifdef NAN_BOXING
    if (IS_BOOL(value)) {
        printf("%s", AS_BOOL(value) ? "true" : "false");
    }
    else if (IS_NIL(value)) {
        printf("nil");
    }
    else if (IS_NUMBER(value)) {
        printf("%g", AS_NUMBER(value));
    }
    else if (IS_OBJ(value)) {
        print_object(value);
    }
#else
    switch (value.type) {
    case VAL_BOOL:
        printf("%s", AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL: printf("nil"); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
    case VAL_OBJ: print_object(value); break;
    }
#endif
}

bool values_equal(Value a, Value b)
{
#ifdef NAN_BOXING
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        return AS_NUMBER(a) == AS_NUMBER(b);
    }
    return a == b;
#else
    if (a.type != b.type) return false;
    switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);
    default:         return false; // Unreachable.
    }
#endif
}
