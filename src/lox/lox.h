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

#include "lox/object.h"
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
} ExecContext;

extern CompileContext compile_context;
extern ExecContext exec_context;

void add_global(const char* name, Value val);
InterpretResult call_function(const char* fn_name, int count, ...);
Value create_room_value(Room* room);
ObjClass* find_class(const char* class_name);
void free_vm();
void init_vm();
InterpretResult interpret_code(const char* source);
Value pop();
void push(Value value);
void runtime_error(const char* format, ...);

#define SET_NATIVE_FIELD(inst, src, tgt, TYPE)                                 \
{                                                                              \
    Value tgt##_value = WRAP_##TYPE(src);                                      \
    push(tgt##_value);                                                         \
    char* tgt##_str = "_" #tgt;                                                \
    ObjString* tgt##_fld = copy_string(tgt##_str, (int)strlen(tgt##_str));     \
    push(OBJ_VAL(tgt##_fld));                                                  \
    table_set(&(inst)->fields, tgt##_fld, tgt##_value);                        \
    pop();                                                                     \
    pop();                                                                     \
}

#endif // !LOX__LOX_H
