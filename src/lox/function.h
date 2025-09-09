////////////////////////////////////////////////////////////////////////////////
// function.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_function_h
#define clox_function_h

#include "lox/common.h"

#include "lox/chunk.h"
#include "lox/object.h"
#include "lox/table.h"

#include <merc.h>

typedef struct {
    Obj obj;
    int arity;
    int upvalue_count;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

typedef Value(*NativeFn)(int arg_count, Value* args);
typedef Value(*NativeMethod)(Value receiver, int arg_count, Value* args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalue_count;
} ObjClosure;

typedef struct {
    Obj obj;
    ObjString* name;
    Table methods;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass* klass;
    Table fields;
} ObjInstance;

typedef struct {
    Obj obj;
    Value receiver;
    ObjClosure* method;
} ObjBoundMethod;

typedef struct {
    Obj obj;
    NativeMethod native;
} ObjNativeMethod;

typedef struct {
    Obj obj;
    DoFunc* native;
} ObjNativeCmd;

ObjBoundMethod* new_bound_method(Value receiver, ObjClosure* method);
ObjNativeMethod* new_native_method(NativeMethod native);
ObjNativeCmd* new_native_cmd(DoFunc* native);
ObjClass* new_class(ObjString* name);
ObjClosure* new_closure(ObjFunction* function);
ObjFunction* new_function();
ObjInstance* new_instance(ObjClass* klass);
ObjNative* new_native(NativeFn function);

#endif // !clox_function_h
