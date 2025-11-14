////////////////////////////////////////////////////////////////////////////////
// memory.h
// From Bob Nystrom's "Crafting Interpreters" (http://craftinginterpreters.com)
// Shared under the MIT License
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef clox_memory_h
#define clox_memory_h

#include "lox/common.h"
#include "lox/object.h"

//#define COUNT_GCS

#ifdef COUNT_GCS
extern uint64_t gc_count;
#endif

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define FREE_RAW_PTR(val)   FREE(ObjRawPtr, AS_RAW_PTR(val))

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, old_count, new_count) \
    (type*)reallocate(pointer, sizeof(type) * (old_count), \
        sizeof(type) * (new_count))

#define FREE_ARRAY(type, pointer, old_count) \
    reallocate(pointer, sizeof(type) * (old_count), 0)

void* reallocate(void* pointer, size_t old_size, size_t new_size);
void* reallocate_nogc(void* pointer, size_t old_size, size_t new_size);
void mark_object(Obj* object);
void mark_value(Value value);
void collect_garbage();
void collect_garbage_nongrowing();
void free_objects();

void gc_protect(Value value);
void gc_protect_clear();

#endif
