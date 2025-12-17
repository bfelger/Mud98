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

////////////////////////////////////////////////////////////////////////////////
// persist/rom-olc/db_rom_olc.h
// Internal header exposing ROM legacy database functions for ROM-OLC format.
// 
// IMPORTANT: This header is for ROM-OLC format implementation ONLY.
// DO NOT include in new code. Use the persist abstractions instead.
//
// These functions are original ROM code that must be preserved for backward
// compatibility with existing .are/.olc area files, but should not be used
// for new features.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__ROM_OLC__DB_ROM_OLC_H
#define MUD98__PERSIST__ROM_OLC__DB_ROM_OLC_H

#include <stdio.h>

// Legacy ROM area file parsers.
// These functions read specific sections of ROM-OLC format area files.
// They maintain global state and use setjmp/longjmp for error handling.

void load_helps(FILE* fp, char* fname);
void load_shops(FILE* fp);
void load_specials(FILE* fp);
void load_mobprogs(FILE* fp);

#endif // !MUD98__PERSIST__ROM_OLC__DB_ROM_OLC_H
