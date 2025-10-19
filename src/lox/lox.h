////////////////////////////////////////////////////////////////////////////////
// lox.h
// Intended to be the one header to include if you are calling Lox code from
// native C.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef LOX__LOX_H
#define LOX__LOX_H

#include <stdarg.h>

typedef struct entity_t Entity;
typedef struct room_t Room;
typedef struct mobile_t Mobile;
typedef struct object_t Object;

#include "array.h"
#include "function.h"
#include "list.h"
#include "object.h"
#include "table.h"
#include "value.h"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

typedef struct {
    Mobile* me;
    Entity* this_;
} CompileContext;

typedef struct {
    Mobile* me;
    bool is_repl;
} ExecContext;

typedef ObjString String;

void add_global(const char* name, Value val);
InterpretResult call_function(const char* fn_name, int count, ...);
InterpretResult invoke_closure(ObjClosure* closure, int count, ...);
ObjClass* find_class(const char* class_name);
void free_vm();
void init_const_natives();
void init_world_natives();
void init_vm();
InterpretResult interpret_code(const char* source);
Value pop();
void push(Value value);
void runtime_error(const char* format, ...);

bool lox_streq(ObjString* a, ObjString* b);

extern char str_empty[1];
extern ObjString* lox_empty_string;

extern CompileContext compile_context;
extern ExecContext exec_context;

#endif // !LOX__LOX_H
