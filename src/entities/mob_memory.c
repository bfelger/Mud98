////////////////////////////////////////////////////////////////////////////////
// mob_memory.c
// Utilities to handle monsters remembering enemies
////////////////////////////////////////////////////////////////////////////////

#include "mob_memory.h"

#include "db.h"

MobMemory* mob_memory_free;

MobMemory* new_mob_memory(void)
{
    MobMemory* memory;

    if (mob_memory_free == NULL)
        memory = alloc_mem(sizeof(*memory));
    else {
        memory = mob_memory_free;
        mob_memory_free = mob_memory_free->next;
    }

    memory->next = NULL;
    memory->id = 0;
    memory->reaction = 0;
    memory->when = 0;
    VALIDATE(memory);

    return memory;
}

void free_mob_memory(MobMemory* memory)
{
    if (!IS_VALID(memory)) return;

    memory->next = mob_memory_free;
    mob_memory_free = memory;
    INVALIDATE(memory);
}