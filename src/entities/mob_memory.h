////////////////////////////////////////////////////////////////////////////////
// mob_memory.h
// Utilities to handle monsters remembering enemies
////////////////////////////////////////////////////////////////////////////////

typedef struct mob_memory_t MobMemory;

#pragma once
#ifndef MUD98__ENTITIES__MOB_MEMORY_H
#define MUD98__ENTITIES__MOB_MEMORY_H

#include <stdbool.h>
#include <time.h>

#define MOB_MEM_CUSTOMER BIT(0)
#define MOB_MEM_SELLER   BIT(1)
#define MOB_MEM_HOSTILE  BIT(2)
#define MOB_MEM_AFRAID   BIT(3)

/* memory for mobs */
typedef struct mob_memory_t {
    MobMemory* next;
    bool valid;
    int id;
    int reaction;
    time_t when;
} MobMemory;

#endif // !MUD98__ENTITIES__MOB_MEMORY_H
