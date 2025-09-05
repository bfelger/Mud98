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
    char* name;
    NativeFn func;
} NativeFuncEntry;

void add_global(const char* name, Value val);
void init_const_natives();
void init_world_natives();

extern const NativeFuncEntry native_funcs[];

#endif 
