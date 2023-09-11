////////////////////////////////////////////////////////////////////////////////
// act_comm.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ACT_COMM_H
#define MUD98__ACT_COMM_H

#include "entities/char_data.h"

void add_follower(CharData* ch, CharData* master);
void stop_follower(CharData* ch);
void nuke_pets(CharData* ch);
void die_follower(CharData* ch);
bool is_same_group(CharData* ach, CharData* bch);

#endif // !MUD98__ACT_COMM_H
