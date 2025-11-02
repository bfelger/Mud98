////////////////////////////////////////////////////////////////////////////////
// compiler.h
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_compiler_h
#define clox_compiler_h

#include <stdarg.h>

#include "lox/object.h"
#include "lox/vm.h"

ObjFunction* compile(const char* source);
void compile_errorf(const char* fmt, ...);
void mark_compiler_roots();
void init_compiler_tables();

#endif
