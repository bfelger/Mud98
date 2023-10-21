////////////////////////////////////////////////////////////////////////////////
// exit_data.c
// Utilities to handle room exits
////////////////////////////////////////////////////////////////////////////////

#include "exit_data.h"

#include "db.h"

int exit_count;
ExitData* exit_free;

ExitData* new_exit()
{
    ExitData* pExit;

    if (!exit_free) {
        pExit = alloc_perm(sizeof(*pExit));
        exit_count++;
    }
    else {
        pExit = exit_free;
        NEXT_LINK(exit_free);
    }

    pExit->u1.to_room = NULL;                  /* ROM OLC */
    pExit->next = NULL;
/*  pExit->vnum         =   0;                        ROM OLC */
    pExit->exit_flags = 0;
    pExit->key = 0;
    pExit->keyword = &str_empty[0];
    pExit->description = &str_empty[0];
    pExit->exit_reset_flags = 0;

    return pExit;
}

void free_exit(ExitData* pExit)
{
    if (pExit == NULL)
        return;

    free_string(pExit->keyword);
    free_string(pExit->description);

    pExit->next = exit_free;
    exit_free = pExit;

    return;
}
