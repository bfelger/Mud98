////////////////////////////////////////////////////////////////////////////////
// lox.h
// Intended to be the one header to include if you are calling Lox code from
// native C.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef LOX__LOX_H
#define LOX__LOX_H

#include <stdarg.h>

typedef struct room_t Room;

#include "lox/object.h"
#include "lox/value.h"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

InterpretResult call_function(const char* fn_name, int count, ...);
Value create_room_value(Room* room);
void free_vm();
void init_vm();
InterpretResult interpret_code(const char* source);
Value pop();
void push(Value value);

#endif // !LOX__LOX_H
