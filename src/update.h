////////////////////////////////////////////////////////////////////////////////
// update.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__UPDATE_H
#define MUD98__UPDATE_H

#include "entities/mobile.h"

void advance_level(Mobile* ch, bool hide);
void gain_exp(Mobile* ch, int gain);
void gain_condition(Mobile* ch, int iCond, int value);
void update_handler();

#endif // !MUD98__UPDATE_H
