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

#include <db.h>

void printf_to_char(Mobile*, const char*, ...);

extern bool test_output_enabled;
extern Value test_output_buffer;

void lox_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buf[4608];

    if (test_output_enabled) {
        vsprintf(buf, format, args);
        if (IS_NIL(test_output_buffer)) {
            test_output_buffer = OBJ_VAL(copy_string(buf, (int)strlen(buf)));
        }
        else {
            ObjString* a = AS_STRING(test_output_buffer);

            size_t length = a->length + strlen(buf);
            char* chars = ALLOCATE(char, length + 1);
            memcpy(chars, a->chars, a->length);
            memcpy(chars + a->length, buf, strlen(buf));
            chars[length] = '\0';

            ObjString* result = take_string(chars, (int)length);

            test_output_buffer = OBJ_VAL(result);
        }
    }
    else if (exec_context.me != NULL) {
        vsprintf(buf, format, args);
        printf_to_char(exec_context.me, "%s", buf);
    }
    else {
        vprintf(format, args);
    }

    va_end(args);
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
    else if (IS_DOUBLE(value)) {
        sprintf(buf, "%g", AS_DOUBLE(value));
    }
    else if (IS_INT(value)) {
        sprintf(buf, "%d", AS_INT(value));
    }
    else if (IS_STRING(value)) {
        sprintf(buf, "%s", AS_STRING(value)->chars);
    }
    else if (IS_RAW_PTR(value)) {
        ObjRawPtr* raw = AS_RAW_PTR(value);
        if (raw->type == RAW_STR) {
            sprintf(buf, "%s", *((char**)raw->addr));
        }
        else {
            sprintf(buf, "(raw_ptr [%d])", raw->type);
        }
    }
    else if (IS_OBJ(value)) {
        sprintf(buf, "(object [%d])", AS_OBJ(value)->type);
    }

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
    else if (IS_DOUBLE(value)) {
        lox_printf("%g", AS_DOUBLE(value));
    }
    else if (IS_INT(value)) {
        lox_printf("%d", AS_INT(value));
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

void print_value_debug(Value value)
{
#ifdef NAN_BOXING
    if (IS_BOOL(value)) {
        lox_printf("%s <bool>", AS_BOOL(value) ? "true" : "false");
    }
    else if (IS_NIL(value)) {
        lox_printf("nil");
    }
    else if (IS_DOUBLE(value)) {
        lox_printf("%g <double>", AS_DOUBLE(value));
    }
    else if (IS_INT(value)) {
        lox_printf("%d <int>", AS_INT(value));
    }
    else if (IS_STRING(value)) {
        lox_printf("\"%s\"", AS_STRING(value)->chars);
    }
    else if (IS_OBJ(value)) {
        print_object(value);
    }
    else {
        lox_printf("Unknown value type: 0x%016llx", (uint64_t)value);
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
    if (IS_DOUBLE(a) && IS_DOUBLE(b)) {
        return AS_DOUBLE(a) == AS_DOUBLE(b);
    }
    else if (IS_INT(a) && IS_INT(b)) {
        return AS_INT(a) == AS_INT(b);
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