////////////////////////////////////////////////////////////////////////////////
// update.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__UPDATE_H
#define MUD98__UPDATE_H

#include "entities/char_data.h"

void advance_level(CharData* ch, bool hide);
void gain_exp(CharData* ch, int gain);
void gain_condition(CharData* ch, int iCond, int value);
void update_handler();

#endif // !MUD98__UPDATE_H
