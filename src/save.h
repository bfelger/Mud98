////////////////////////////////////////////////////////////////////////////////
// save.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__SAVE_H
#define MUD98__SAVE_H

#include "entities/mobile.h"
#include "entities/descriptor.h"

void save_char_obj(Mobile* ch);
bool load_char_obj(Descriptor* d, char* name);
int	race_exp_per_level(int race, int ch_class, int points);

#endif // !MUD98__SAVE_H
