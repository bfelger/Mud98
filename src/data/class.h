////////////////////////////////////////////////////////////////////////////////
// class.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__CLASS_H
#define MUD98__DATA__CLASS_H

#include "merc.h"

#include "stats.h"

#include <stdint.h>
#include <stdbool.h>

typedef enum archetype_t {
    ARCH_NOT_FOUND  = -1,
    ARCH_ARCANE     = 0,
    ARCH_DIVINE     = 1,
    ARCH_ROGUE      = 2,
    ARCH_MARTIAL    = 3,
} Archetype;

#define ARCH_MIN ARCH_ARCANE
#define ARCH_MAX ARCH_MARTIAL
#define ARCH_COUNT 4

// Circumvent C33011 "Unchecked upper bound for enum arch used as index."
#define CHECK_ARCH(a)   (a < 0 ? 0 : (a >= ARCH_COUNT ? ARCH_COUNT-1 : a))

typedef struct archetype_info_t {
    const char* name;
    const char* short_name;
    const Archetype arch;
} ArchetypeInfo;

extern const ArchetypeInfo arch_table[ARCH_COUNT];

#define MAX_GUILD 2

typedef struct class_t {
    char* name; 
    char* who_name; 
    char* base_group;
    char* default_group;
    VNUM weapon;                    // First weapon
    VNUM guild[MAX_GUILD];          // Vnum of guild rooms
    Stat prime_stat;
    Archetype arch;                 // Used for saves, titles, etc.
    int16_t skill_cap;              // Maximum skill level
    int16_t thac0_00;               // Thac0 for level 0
    int16_t thac0_32;               // Thac0 for level 32
    int16_t hp_min;                 // Min hp gained on leveling
    int16_t hp_max;                 // Max hp gained on leveling
    bool fMana;                     // Class gains mana on level
} Class;

void load_class_table();
void save_class_table();

extern int class_count;
extern Class* class_table;
extern char* const title_table[ARCH_COUNT][MAX_LEVEL + 1][2];

#endif // !MUD98__DATA__CLASS_H
