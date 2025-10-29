////////////////////////////////////////////////////////////////////////////////
// segvec.h
// Segmented Vector for Lox
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__LOX__SEGVEC_H
#define MUD98__LOX__SEGVEC_H

#include "common.h"
#include "memory.h"
#include "object.h"
#include "value.h"

#include <db.h>

#include <stdbool.h>

typedef struct {
    int capacity;           // Total capacity across all blocks
    int count;              // Number of elements currently stored
    int blk_elems;          // Elements per block (power of two)
    int blk_shift;          // log2(blk_elems) for fast indexing
    int num_blocks;         // Number of blocks currently allocated
    int blocks_capacity;    // Capacity of the array of block pointers
    Value** blocks;         // Each blocks[i] points to a block of:
                            //     blk_elems * sizeof(ElementType) bytes
} SegmentedVector;

void sv_init(SegmentedVector* sv, int max_block_bytes);
void sv_free(SegmentedVector* sv);
bool sv_remove_unordered(SegmentedVector* sv, int i, Value* out);
bool sv_remove_stable(SegmentedVector* sv, int i, Value* out);

/* Choose blk_elems so one block stays under the allocator cap. For example:
   max_block_bytes / sizeof(Value), rounded down to a power of two (>= 1). */
static inline int sv_pow2_floor(int x)
{
    // Returns largest power of two <= x (x >= 1)
    unsigned v = (unsigned)x;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return (int)(v - (v >> 1));
}

static inline int sv_ctz32(unsigned x)
{
    // Count trailing zeros; x is power of two
#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward(&idx, x);
    return (int)idx;
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(x);
#else
    // portable fallback
    int n = 0;
    while ((x & 1u) == 0u)
    {
        x >>= 1;
        ++n;
    }
    return n;
#endif
}

static bool sv_grow_blocks_array(SegmentedVector* sv, int need)
{
    if (need <= sv->blocks_capacity)
        return true;
    int old_cap = sv->blocks_capacity;
    int new_cap = sv->blocks_capacity ? sv->blocks_capacity * 2 : 4;
    if (new_cap < need) new_cap = need;
    Value** nb = (Value**)reallocate_nogc((void*)sv->blocks, (size_t)old_cap * sizeof(Value*), (size_t)new_cap * sizeof(Value*));
    if (!nb)
        return false;
    sv->blocks = nb;
    sv->blocks_capacity = new_cap;
    return true;
}

static void* sv_alloc_block(size_t bytes)
{
    return alloc_mem(bytes);
}

static void sv_free_block(void* p)
{

    int* magic = 0;
    uintptr_t ptr = (uintptr_t)p;
    int* size_ptr = (int*)(ptr - sizeof(*magic));
    int i_size = (*size_ptr - MAGIC_NUM);
    *size_ptr = MAGIC_NUM;
    size_t s_size = (size_t)i_size;

    free_mem(p, s_size);
}

static bool sv_add_block(SegmentedVector* sv)
{
    if (!sv_grow_blocks_array(sv, sv->num_blocks + 1))
        return false;

    void* blk = sv_alloc_block((size_t)sv->blk_elems * sizeof(Value));

    if (!blk)
        return false;

    sv->blocks[sv->num_blocks++] = blk;
    sv->capacity += sv->blk_elems;
    return true;
}

static inline int sv_block_index(const SegmentedVector* sv, int i)
{
    return i >> sv->blk_shift;
}

static inline int sv_block_offset(const SegmentedVector* sv, int i)
{
    return i & (sv->blk_elems - 1);
}

// Push back (amortized O(1)). 
// Returns 1 on success.
static inline bool sv_push(SegmentedVector* sv, const Value v)
{
    if (sv->count == sv->capacity) {
        if (!sv_add_block(sv))
            return false;
    }
    int idx = (int)sv->count++;
    int bi = sv_block_index(sv, idx);
    int off = sv_block_offset(sv, idx);
    ((Value*)sv->blocks[bi])[off] = v;
    return true;
}

static inline bool sv_pop(SegmentedVector* sv, Value* out)
{
    if (sv->count == 0)
        return false;

    int idx = (int)--sv->count;
    int bi = sv_block_index(sv, idx);
    int off = sv_block_offset(sv, idx);

    if (out)
        *out = ((Value*)sv->blocks[bi])[off];

    return true;
}

// Random access (O(1))
static inline Value* sv_at(SegmentedVector* sv, int i)
{
    if (i < 0 || i >= (int)sv->count)
        return NULL;

    int bi = sv_block_index(sv, i);
    int off = sv_block_offset(sv, i);
    return &((Value*)sv->blocks[bi])[off];
}

static inline const Value* sv_cat(const SegmentedVector* sv, int i)
{
    if (i < 0 || i >= (int)sv->count)
        return NULL;

    int bi = sv_block_index(sv, i);
    int off = sv_block_offset(sv, i);
    return &((const Value*)sv->blocks[bi])[off];
}

// Set by index
static inline void sv_set(SegmentedVector* sv, int i, const Value* v)
{
    *sv_at(sv, i) = *v;
}

#endif // !MUD98__LOX__SEGVEC_H
