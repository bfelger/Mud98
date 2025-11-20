////////////////////////////////////////////////////////////////////////////////
// chunk.h
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_chunk_h
#define clox_chunk_h

#include "lox/array.h"
#include "lox/common.h"
#include "lox/object.h"
#include "lox/value.h"

typedef enum {
    OP_CONSTANT, 
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_GET_SUPER,
    OP_EQUAL,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_GET_AT_INDEX,
    OP_SET_AT_INDEX,
    OP_BOX_PTR,
    OP_UNBOX_VAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_ARRAY,
    OP_INVOKE,
    OP_SUPER_INVOKE,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_RETURN,
    OP_CLASS,
    OP_INHERIT,
    OP_METHOD,
    OP_EACH_PRIME,
    OP_EACH_OR_END,
    OP_EACH_ADVANCE,
    OP_INTERP,
    // Game entities
    OP_SELF,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

void init_chunk(Chunk* chunk);
void free_chunk(Chunk* chunk);
void write_chunk(Chunk* chunk, uint8_t byte, int line);
int add_constant(Chunk* chunk, Value value);

#endif
