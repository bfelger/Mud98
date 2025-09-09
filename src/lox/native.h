////////////////////////////////////////////////////////////////////////////////
// native.h
// Extra native functions for Lox
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_native_h
#define clox_native_h

#include "lox/function.h"
#include "lox/object.h"
#include "lox/value.h"

typedef struct {
    const char* name;
    NativeFn func;
} NativeFuncEntry;

typedef struct {
    const char* name;
    NativeMethod method;
} NativeMethodEntry;

void add_global(const char* name, Value val);
void init_const_natives();
void init_world_natives();

extern const NativeFuncEntry native_func_entries[];
extern const NativeMethodEntry native_method_entries[];
extern Table native_methods;

#endif 
