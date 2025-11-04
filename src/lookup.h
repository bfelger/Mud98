/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@hypercube.org)                                *
 *      Gabrielle Taylor (gtaylor@hypercube.org)                           *
 *      Brian Moore (zump@rom.org)                                         *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

#pragma once
#ifndef MUD98__LOOKUP_H
#define MUD98__LOOKUP_H

#include "merc.h"

#include "tables.h"

#include "data/mobile_data.h"

int clan_lookup(const char* name);
Position position_lookup(const char* name);
Sex sex_lookup(const char* name);
MobSize size_lookup(const char* name);
FLAGS flag_lookup(const char*, const struct flag_type*);
HelpData* help_lookup(char*);
HelpArea* had_lookup(char*);
int16_t race_lookup(const char* name);
Stat stat_lookup(const char* name);
ItemType item_lookup(const char* name);
int liquid_lookup(const char* name);

#endif // !MUD98__LOOKUP_H
