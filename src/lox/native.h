////////////////////////////////////////////////////////////////////////////////
// native.h
// Extra native functions for Lox
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_native_h
#define clox_native_h

#include "function.h"
#include "object.h"
#include "value.h"

#include <merc.h>

typedef struct {
    const char* name;
    NativeFn func;
} NativeFuncEntry;

typedef struct {
    const char* name;
    NativeMethod method;
} NativeMethodEntry;

void add_global(const char* name, Value val);
void init_const_natives(void);
void init_native_cmds(void);
void init_world_natives(void);

extern const NativeFuncEntry native_func_entries[];
extern const NativeMethodEntry native_method_entries[];
extern Table native_methods;
extern Table native_cmds;
extern Table native_mob_cmds;

#endif 
