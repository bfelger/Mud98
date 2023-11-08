////////////////////////////////////////////////////////////////////////////////
// native.h
// Extra native functions for Lox
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_native_h
#define clox_native_h

#include "lox/object.h"
#include "lox/value.h"

typedef struct {
    char* name;
    NativeFn func;
} NativeFuncEntry;

void init_natives();

extern const NativeFuncEntry native_funcs[];

#endif 
