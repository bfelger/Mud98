////////////////////////////////////////////////////////////////////////////////
// value.c
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "lox/lox.h"
#include "lox/object.h"
#include "lox/memory.h"
#include "lox/value.h"

void printf_to_char(Mobile*, const char*, ...);

void lox_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    if (exec_context.me != NULL) {
        char buf[4608];
        vsprintf(buf, format, args);
        printf_to_char(exec_context.me, "%s", buf);
    }
    else {
        vprintf(format, args);
    }

    va_end(args);
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

void remove_value_array(ValueArray* array, int index)
{
    if (index > array->count || index < 0)
        return;

    for (int i = index; i < array->count - 1; ++i)
        array->values[i] = array->values[i + 1];
    array->values[array->count - 1] = NIL_VAL;
    --array->count;
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
        lox_printf("%s", AS_BOOL(value) ? "true" : "false");
    }
    else if (IS_NIL(value)) {
        lox_printf("nil");
    }
    else if (IS_NUMBER(value)) {
        lox_printf("%g", AS_NUMBER(value));
    }
    else if (IS_OBJ(value)) {
        print_object(value);
    }
#else
    switch (value.type) {
    case VAL_BOOL:
        lox_printf("%s", AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL: lox_printf("nil"); break;
    case VAL_NUMBER: lox_printf("%g", AS_NUMBER(value)); break;
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

bool lox_streq(ObjString* a, ObjString* b)
{
    if (a->hash != b->hash)
        return false;

    if (a->length != b->length)
        return false;

    return !strcmp(a->chars, b->chars);
}