////////////////////////////////////////////////////////////////////////////////
// mob_prog.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__MOB_PROG_H
#define MUD98__MOB_PROG_H

typedef struct mob_prog_t MobProg;
typedef struct mob_prog_code_t MobProgCode;

#include "merc.h"

#include "entities/mobile.h"
#include "entities/object.h"

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

MobProg* new_mob_prog();
void free_mob_prog(MobProg* mob_prog);

MobProgCode* new_mob_prog_code();
void free_mob_prog_code(MobProgCode* mob_prog_code);

void program_flow(VNUM vnum, char* source, Mobile* mob, Mobile* ch, 
    const void* arg1, const void* arg2);
void mp_act_trigger(char* argument, Mobile* mob, Mobile* ch, 
    const void* arg1, const void* arg2, MobProgTrigger trig_type);
bool mp_percent_trigger(Mobile* mob, Mobile* ch, const void* arg1,
    const void* arg2, MobProgTrigger trig_type);
void mp_bribe_trigger(Mobile* mob, Mobile* ch, int amount);
bool mp_exit_trigger(Mobile* ch, int dir);
void mp_give_trigger(Mobile* mob, Mobile* ch, Object* obj);
void mp_greet_trigger(Mobile* ch);
void mp_hprct_trigger(Mobile* mob, Mobile* ch);

extern int mob_prog_count;
extern int mob_prog_perm_count;
extern int mob_prog_code_count;
extern int mob_prog_code_perm_count;

extern MobProgCode* mprog_list;

#endif // !MUD98__MOB_PROG_H
