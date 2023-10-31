////////////////////////////////////////////////////////////////////////////////
// mob_memory.c
// Utilities to handle monsters remembering enemies
////////////////////////////////////////////////////////////////////////////////

#include "mob_memory.h"

#include "db.h"

MobMemory* mob_memory_free;

int mob_memory_count;
int mob_memory_perm_count;

MobMemory* new_mob_memory()
{
    LIST_ALLOC_PERM(mob_memory, MobMemory);

    VALIDATE(mob_memory);

    return mob_memory;
}

void free_mob_memory(MobMemory* mob_memory)
{
    if (!IS_VALID(mob_memory))
        return;

    INVALIDATE(mob_memory);

    LIST_FREE(mob_memory);
}
