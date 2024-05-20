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
typedef struct mobile_t Mobile;
typedef struct object_t Object;

#include "lox/array.h"
#include "lox/function.h"
#include "lox/list.h"
#include "lox/object.h"
#include "lox/table.h"
#include "lox/value.h"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

typedef struct {
    Mobile* me;
} CompileContext;

typedef struct {
    Room* here;
    Mobile* me;
    Object* this_;
    bool is_repl;
} ExecContext;

typedef ObjString String;

void add_global(const char* name, Value val);
InterpretResult call_function(const char* fn_name, int count, ...);
ObjClass* find_class(const char* class_name);
void free_vm();
void init_natives();
void init_vm();
InterpretResult interpret_code(const char* source);
Value pop();
void push(Value value);
void runtime_error(const char* format, ...);

bool lox_streq(ObjString* a, ObjString* b);

extern CompileContext compile_context;
extern ExecContext exec_context;

#endif // !LOX__LOX_H
