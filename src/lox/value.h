////////////////////////////////////////////////////////////////////////////////
// value.h
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_value_h
#define clox_value_h

#include <string.h>

#include "lox/common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING
// See note in common.h

typedef uint64_t Value;

#define NANISH                  0x7ffc000000000000
#define NANISH_MASK             0xffff000000000000

#define BOOLEAN_MASK            0x7ffe000000000002
#define INTEGER_MASK            0x7ffc000000000000
#define OBJECT_MASK             0xfffc000000000000
#define STRING_MASK             0xfffe000000000000

#define NIL_VAL                 0x7ffe000000000000
#define TRUE_VAL                (BOOLEAN_MASK | 3)
#define FALSE_VAL               (BOOLEAN_MASK | 2)

#define IS_DOUBLE(v)            ((v & NANISH) != NANISH)
#define IS_OBJ(v)               ((v & NANISH_MASK) == OBJECT_MASK)
#define IS_NIL(v)               (v == NIL_VAL)
#define IS_BOOL(v)              ((v & BOOLEAN_MASK) == BOOLEAN_MASK)
#define IS_INT(v)               ((v & NANISH_MASK) == INTEGER_MASK)

#define AS_DOUBLE(v)            value_to_double(v)
#define AS_OBJ(v)               ((Obj*)(v & 0xFFFFFFFFFFFF))
#define AS_BOOL(v)              ((char)(v & 0x1))
#define AS_INT(v)               ((int32_t)(v))

#define BOOL_VAL(b)             ((b) ? TRUE_VAL : FALSE_VAL)
#define OBJ_VAL(p)              ((uint64_t)(p) | OBJECT_MASK)
#define INT_VAL(i)              ((uint64_t)(i) | INTEGER_MASK)
#define DOUBLE_VAL(d)           double_to_value(d)


static inline double value_to_double(Value value)
{
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

static inline Value double_to_value(double num)
{
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

#else

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NIL(value)       ((value).type == VAL_NIL)
#define IS_NUMBER(value)    ((value).type == VAL_NUMBER)
#define IS_OBJ(value)       ((value).type == VAL_OBJ)

#define AS_OBJ(value)       ((value).as.obj)
#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUMBER(value)    ((value).as.number)

#define BOOL_VAL(value)     ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL             ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value)   ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)     ((Value){VAL_OBJ, {.obj = (Obj*)object}})

#endif

bool values_equal(Value a, Value b);
char* string_value(Value value);
void print_value(Value value);

#define IS_PERM_STRING(str) \
    (str == &str_empty[0] || (str >= string_space && str < top_string))

extern char* string_space;
extern char* top_string;
extern char str_empty[1];

#endif
