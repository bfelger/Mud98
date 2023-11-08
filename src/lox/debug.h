////////////////////////////////////////////////////////////////////////////////
// debug.h
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_debug_h
#define clox_debug_h

#include "lox/chunk.h"

void disassemble_chunk(Chunk* chunk, const char* name);
int disassemble_instruction(Chunk* chunk, int offset);

#endif
