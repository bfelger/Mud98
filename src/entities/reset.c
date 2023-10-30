////////////////////////////////////////////////////////////////////////////////
// reset.c
// Utilities to handle area resets of mobs and items
////////////////////////////////////////////////////////////////////////////////

#include "reset.h"

#include "comm.h"
#include "db.h"

int reset_count;
int reset_perm_count;
Reset* reset_free;

Reset* new_reset()
{
    LIST_ALLOC_PERM(reset, Reset);

    reset->command = 'X';

    return reset;
}

void free_reset(Reset* reset)
{
    LIST_FREE(reset);
}
