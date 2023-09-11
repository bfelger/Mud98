////////////////////////////////////////////////////////////////////////////////
// save.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__SAVE_H
#define MUD98__SAVE_H

#include "comm.h"

#include "entities/char_data.h"

void save_char_obj(CharData* ch);
bool load_char_obj(DESCRIPTOR_DATA* d, char* name);
int	race_exp_per_level(int race, int ch_class, int points);

#endif // !MUD98__SAVE_H