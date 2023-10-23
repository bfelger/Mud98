////////////////////////////////////////////////////////////////////////////////
// race.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__RACE_H
#define MUD98__DATA__RACE_H

#include "merc.h"

#include "class.h"
#include "mobile_data.h"
#include "stats.h"

#include "array.h"

#include <stdbool.h>
#include <stdint.h>

#define RACE_NUM_SKILLS 5

typedef int16_t ClassMult;
DECLARE_ARRAY(ClassMult)

typedef VNUM StartLoc;
DECLARE_ARRAY(StartLoc)

typedef struct race_t {
    char* name; 
    char* who_name;
    char* skills[RACE_NUM_SKILLS];
    ARRAY(StartLoc) class_start;    // Special start locations per class
    VNUM start_loc;
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
    ARRAY(ClassMult) class_mult;    // exp multiplier for class * 100
    int16_t stats[STAT_COUNT];	    // starting stats
    int16_t max_stats[STAT_COUNT];	// maximum stats
    MobSize size;			        
    bool pc_race;                   // can be chosen by pcs */
} Race;

void load_race_table();
void save_race_table();

extern int race_count;
extern Race* race_table;

#endif // !MUD98__DATA__RACE_H
