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

#define MAX_GUILD 2

typedef struct class_t {
    char* name; 
    char* who_name; 
    char* base_group;
    char* default_group;
    VNUM weapon;                    // First weapon
    VNUM guild[MAX_GUILD];          // Vnum of guild rooms
    Stat prime_stat;
    int16_t skill_cap;              // Maximum skill level
    int16_t thac0_00;               // Thac0 for level 0
    int16_t thac0_32;               // Thac0 for level 32
    int16_t hp_min;                 // Min hp gained on leveling
    int16_t hp_max;                 // Max hp gained on leveling
    bool fMana;                     // Class gains mana on level
    const char* titles[MAX_LEVEL + 1][2];
} Class;

void load_class_table();
void save_class_table();

extern int class_count;
extern Class* class_table;

#endif // !MUD98__DATA__CLASS_H
