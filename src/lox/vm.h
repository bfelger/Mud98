////////////////////////////////////////////////////////////////////////////////
// vm.h
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_vm_h
#define clox_vm_h

#include "lox.h"
#include "common.h"

#include "object.h"
#include "table.h"
#include "value.h"

#include <stdarg.h>

typedef struct entity_t Entity;

#define FRAMES_MAX 128
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frame_count;

    Value stack[STACK_MAX];
    Value* stack_top;
    Table globals;
    Table strings;
    ObjString* init_string;
    ObjUpvalue* open_upvalues;

    size_t bytes_allocated;
    size_t next_gc;
    Obj* objects;
    int gray_count;
    int gray_capacity;
    Obj** gray_stack;
} VM;

extern VM vm;

bool call_closure(ObjClosure* closure, int arg_count);
void init_vm();
void free_vm(); 
InterpretResult interpret_code(const char* source);
void push(Value value);
Value pop();
Value peek(int distance);
InterpretResult run();
void runtime_error(const char* format, ...);
InterpretResult call_function(const char* fn_name, int count, ...);
void init_entity_class(Entity* entity);
void invoke_method_closure(Value receiver, ObjClosure* closure, int count, ...);

void gc_protect(Value value);
void gc_protect_clear();

#endif
