////////////////////////////////////////////////////////////////////////////////
// segvec.c
// Segmented Vector for Lox
////////////////////////////////////////////////////////////////////////////////

#include "segvec.h"

#include "memory.h"

#include <db.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void sv_init(SegmentedVector* sv, int max_block_bytes)
{
    memset(sv, 0, sizeof(*sv));
    int max_elems = max_block_bytes / (int)sizeof(Value);
    if (max_elems < 1) 
        max_elems = 1;
    sv->blk_elems = sv_pow2_floor(max_elems);
    if (sv->blk_elems < 1) 
        sv->blk_elems = 1;
    sv->blk_shift = sv_ctz32((unsigned)sv->blk_elems);
}

void sv_free(SegmentedVector* sv)
{
    for (int i = 0; i < sv->num_blocks; ++i)
        sv_free_block(sv->blocks[i]);

    free_mem(sv->blocks, sv->num_blocks * sizeof(void*));
    memset(sv, 0, sizeof(*sv));
}

UNUSED
static bool sv_reserve(SegmentedVector* sv, int n)
{
    if (n <= sv->capacity)
        return true;
    int need_blocks = (n + sv->blk_elems - 1) >> sv->blk_shift;
    while (sv->num_blocks < need_blocks) {
        if (!sv_add_block(sv))
            return false;
    }
    return true;
}

#pragma GCC diagnostic pop

bool sv_remove_unordered(SegmentedVector* sv, int i, Value* out)
{
    if (i < 0 || i >= (int)sv->count)
        return false;

    int last = (int)sv->count - 1;
    Value tmp = *sv_at(sv, last);

    if (out)
        *out = *sv_at(sv, i);

    *sv_at(sv, i) = tmp;
    sv->count--;
    return true;
}

// Remove (stable): shifts tail; O(n) like a classic vector.
bool sv_remove_stable(SegmentedVector* sv, int i, Value* out)
{
    if (i < 0 || i >= (int)sv->count) 
        return false;

    if (out)
        *out = *sv_at(sv, i);

    // Move [i+1, count) left by 1, possibly crossing block boundaries.
    for (int k = i + 1; k < (int)sv->count; ++k) {
        *sv_at(sv, k - 1) = *sv_at(sv, k);
    }
    sv->count--;
    return true;
}

//int main(void)
//{
//    SegmentedVector sv;
//    sv_init(&sv, /*max_block_bytes=*/ 4096); // e.g., 4 KiB per block
//
//    for (int i = 0; i < 1 << 20; ++i) {
//        Value v = INT_VAL(i);
//        if (!sv_push(&sv, &v)) 
//        {
//            fprintf(stderr, "Failed to push value %d\n", i);
//            return 1;
//        }
//    }
//
//    // Random access
//    Value* p = sv_at(&sv, 123456);
//    assert(p->x == 123456);
//
//    // Removal
//    Value removed;
//    sv_remove_unordered(&sv, 42, &removed);
//
//    sv_free(&sv);
//}