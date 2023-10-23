////////////////////////////////////////////////////////////////////////////////
// reset.c
// Utilities to handle area resets of mobs and items
////////////////////////////////////////////////////////////////////////////////

#include "reset.h"

#include "db.h"

int reset_count;
Reset* reset_free;

Reset* new_reset()
{
    static Reset zero = { 0 };
    Reset* reset;

    if (!reset_free) {
        reset = alloc_perm(sizeof(*reset));
        reset_count++;
    }
    else {
        reset = reset_free;
        NEXT_LINK(reset_free);
    }

    *reset = zero;
    reset->command = 'X';

    return reset;
}

void free_reset(Reset* reset)
{
    reset->next = reset_free;
    reset_free = reset;
    return;
}
