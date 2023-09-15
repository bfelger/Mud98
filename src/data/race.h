////////////////////////////////////////////////////////////////////////////////
// race.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__RACE_H
#define MUD98__DATA__RACE_H

#include "merc.h"

#include "stats.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct race_t {
    char* name; 
    char* who_name;
    char* skills[5];
    FLAGS act_flags;
    FLAGS aff;
    FLAGS off;
    FLAGS imm;
    FLAGS res;
    FLAGS vuln; 
    FLAGS form;
    FLAGS parts;
    int16_t race_id;
    int16_t points;			        // cost in points of the race
    int16_t class_mult[MAX_CLASS];	// exp multiplier for class, * 100
    int16_t stats[STAT_MAX];	    // starting stats
    int16_t max_stats[STAT_MAX];	// maximum stats
    int16_t size;			        
    bool pc_race;                   // can be chosen by pcs */
} Race;


extern Race* race_table;

#endif // !MUD98__DATA__RACE_H
