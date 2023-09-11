////////////////////////////////////////////////////////////////////////////////
// reset_data.c
// Utilities to handle area resets of mobs and items
////////////////////////////////////////////////////////////////////////////////

#include "reset_data.h"

#include "db.h"

int top_reset;
ResetData* reset_free;

ResetData* new_reset_data()
{
    ResetData* pReset;

    if (!reset_free) {
        pReset = alloc_perm(sizeof(ResetData));
        top_reset++;
    }
    else {
        pReset = reset_free;
        reset_free = reset_free->next;
    }

    pReset->next = NULL;
    pReset->command = 'X';
    pReset->arg1 = 0;
    pReset->arg2 = 0;
    pReset->arg3 = 0;
    pReset->arg4 = 0;

    return pReset;
}

void free_reset_data(ResetData* pReset)
{
    pReset->next = reset_free;
    reset_free = pReset;
    return;
}
