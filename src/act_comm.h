////////////////////////////////////////////////////////////////////////////////
// act_comm.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ACT_COMM_H
#define MUD98__ACT_COMM_H

#include "entities/mobile.h"

void add_follower(Mobile* ch, Mobile* master);
void stop_follower(Mobile* ch);
void nuke_pets(Mobile* ch);
void die_follower(Mobile* ch);
bool is_same_group(Mobile* ach, Mobile* bch);
void do_clear(Mobile* ch, char* argument);

#endif // !MUD98__ACT_COMM_H
