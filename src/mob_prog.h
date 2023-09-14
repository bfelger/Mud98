////////////////////////////////////////////////////////////////////////////////
// mob_prog.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__MOB_PROG_H
#define MUD98__MOB_PROG_H

typedef struct mob_prog_t MobProg;
typedef struct mob_prog_code_t MobProgCode;

#include "merc.h"

#include "entities/char_data.h"
#include "entities/object_data.h"

typedef enum mob_prog_trigger_t {
    TRIG_ACT    = BIT(0),
    TRIG_BRIBE  = BIT(1),
    TRIG_DEATH  = BIT(2),
    TRIG_ENTRY  = BIT(3),
    TRIG_FIGHT  = BIT(4),
    TRIG_GIVE   = BIT(5),
    TRIG_GREET  = BIT(6),
    TRIG_GRALL  = BIT(7),
    TRIG_KILL   = BIT(8),
    TRIG_HPCNT  = BIT(9),
    TRIG_RANDOM = BIT(10),
    TRIG_SPEECH = BIT(12),
    TRIG_EXIT   = BIT(13),
    TRIG_EXALL  = BIT(14),
    TRIG_DELAY  = BIT(15),
    TRIG_SURR   = BIT(16),
} MobProgTrigger;

typedef struct mob_prog_t {
    MobProg* next;
    char* trig_phrase;
    char* code;
    MobProgTrigger trig_type;
    VNUM vnum;
    bool valid;
} MobProg;

typedef struct mob_prog_code_t {
    MobProgCode* next;
    VNUM vnum;
    bool changed;
    char* code;
} MobProgCode;

MobProg* new_mprog();
void free_mprog(MobProg* mp);
MobProgCode* new_mpcode();

void program_flow(VNUM vnum, char* source, CharData* mob, CharData* ch, 
    const void* arg1, const void* arg2);
void mp_act_trigger(char* argument, CharData* mob, CharData* ch, 
    const void* arg1, const void* arg2, int type);
bool mp_percent_trigger(CharData* mob, CharData* ch, const void* arg1,
    const void* arg2, int type);
void mp_bribe_trigger(CharData* mob, CharData* ch, int amount);
bool mp_exit_trigger(CharData* ch, int dir);
void mp_give_trigger(CharData* mob, CharData* ch, ObjectData* obj);
void mp_greet_trigger(CharData* ch);
void mp_hprct_trigger(CharData* mob, CharData* ch);

extern MobProgCode* mprog_list;

#endif // !MUD98__MOB_PROG_H
