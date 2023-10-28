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

int reset_counter_count;
int reset_counter_perm_count;
ResetCounter* reset_counter_free;

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

ResetCounter* new_reset_counter()
{
    LIST_ALLOC_PERM(reset_counter, ResetCounter);

    return reset_counter;
}

void free_reset_counter(ResetCounter* reset_counter)
{
    LIST_FREE(reset_counter);
}

ResetCounter* find_reset_counter(ResetCounter* list, VNUM vnum)
{
    ResetCounter* counter = NULL;

    ORDERED_GET(ResetCounter, counter, list, vnum, vnum);

    return counter;
}

void inc_reset_counter(ResetCounter** list, VNUM vnum)
{
    ResetCounter* counter = find_reset_counter(*list, vnum);
    
    if (counter == NULL) {
        counter = new_reset_counter();
        counter->vnum = vnum;
        ORDERED_INSERT(ResetCounter, counter, *list, vnum);
    }

    counter->count++;
}

void dec_reset_counter(ResetCounter* list, VNUM vnum)
{
    ResetCounter* counter = find_reset_counter(list, vnum);

    if (counter == NULL) {
        bugf("dec_reset_counter: Could not find reset counter with VNUM %d.", vnum);
    }
}

int get_reset_count(ResetCounter* list, VNUM vnum)
{
    ResetCounter* counter = find_reset_counter(list, vnum);

    if (counter == NULL)
        return 0;

    return counter->count;
}
