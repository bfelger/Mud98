////////////////////////////////////////////////////////////////////////////////
// char_data.h
// Character data
////////////////////////////////////////////////////////////////////////////////

typedef struct char_data_t CharData;

#pragma once
#ifndef MUD98__ENTITIES__CHAR_DATA_H
#define MUD98__ENTITIES__CHAR_DATA_H

#include "merc.h"

#include "interp.h"

#include "area_data.h"
#include "mob_prototype.h"
#include "object_data.h"
#include "player_data.h"
#include "room_data.h"

#include <stdbool.h>

typedef struct char_data_t {
    CharData* next;
    CharData* next_in_room;
    CharData* master;
    CharData* leader;
    CharData* fighting;
    CharData* reply;
    CharData* pet;
    CharData* mprog_target;
    MEM_DATA* memory;
    SpecFunc* spec_fun;
    MobPrototype* pIndexData;
    DESCRIPTOR_DATA* desc;
    AFFECT_DATA* affected;
    NOTE_DATA* pnote;
    ObjectData* carrying;
    ObjectData* on;
    RoomData* in_room;
    RoomData* was_in_room;
    AreaData* zone;
    PlayerData* pcdata;
    GEN_DATA* gen_data;
    char* name;
    char* material;
    char* short_descr;
    char* long_descr;
    char* description;
    char* prompt;
    char* prefix;
    time_t logon;
    time_t played;
    int id;
    int version;
    int lines; /* for the pager */
    int exp;
    int act;
    int comm;
    int wiznet;
    int imm_flags;
    int res_flags;
    int vuln_flags;
    int affected_by;
    int form;
    int parts;
    int off_flags;
    int wait;
    int16_t damage[3];
    int16_t dam_type;
    int16_t start_pos;
    int16_t default_pos;
    int16_t mprog_delay;
    int16_t group;
    int16_t clan;
    int16_t sex;
    int16_t ch_class;
    int16_t race;
    int16_t level;
    int16_t trust;
    int16_t size;
    int16_t position;
    int16_t practice;
    int16_t train;
    int16_t carry_weight;
    int16_t carry_number;
    int16_t saving_throw;
    int16_t alignment;
    int16_t hitroll;
    int16_t damroll;
    int16_t armor[4];
    int16_t wimpy;
    int16_t perm_stat[MAX_STATS];
    int16_t mod_stat[MAX_STATS];
    int16_t invis_level;
    int16_t incog_level;
    int16_t timer;
    int16_t daze;
    int16_t hit;
    int16_t max_hit;
    int16_t mana;
    int16_t max_mana;
    int16_t move;
    int16_t max_move;
    int16_t gold;
    int16_t silver;
    bool valid;
} CharData;

void free_char_data(CharData* ch);
CharData* new_char_data();

extern CharData* char_list;
extern CharData* char_free;
extern int mobile_count;

#endif // !MUD98__ENTITIES__CHAR_DATA_H
