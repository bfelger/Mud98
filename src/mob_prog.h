////////////////////////////////////////////////////////////////////////////////
// mob_prog.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__MOB_PROG_H
#define MUD98__MOB_PROG_H

#include "merc.h"

#include "entities/char_data.h"
#include "entities/object_data.h"

void program_flow(VNUM vnum, char* source, CharData* mob,
    CharData* ch, const void* arg1, const void* arg2);
void mp_act_trigger(char* argument, CharData* mob, CharData* ch,
    const void* arg1, const void* arg2, int type);
bool mp_percent_trigger(CharData* mob, CharData* ch, const void* arg1,
    const void* arg2, int type);
void mp_bribe_trigger(CharData* mob, CharData* ch, int amount);
bool mp_exit_trigger(CharData* ch, int dir);
void mp_give_trigger(CharData* mob, CharData* ch, ObjectData* obj);
void mp_greet_trigger(CharData* ch);
void mp_hprct_trigger(CharData* mob, CharData* ch);

#endif // !MUD98__MOB_PROG_H
