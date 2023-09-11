/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
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
#ifndef MUD98__INTERP_H
#define MUD98__INTERP_H

#include "merc.h"

#include <stdbool.h>

/* for command types */
#define ML         MAX_LEVEL /* implementor */
#define L1         MAX_LEVEL - 1 /* creator */
#define L2         MAX_LEVEL - 2 /* supreme being */
#define L3         MAX_LEVEL - 3 /* deity */
#define L4         MAX_LEVEL - 4 /* god */
#define L5         MAX_LEVEL - 5 /* immortal */
#define L6         MAX_LEVEL - 6 /* demigod */
#define L7         MAX_LEVEL - 7 /* angel */
#define L8         MAX_LEVEL - 8 /* avatar */
#define IM         LEVEL_IMMORTAL /* avatar */
#define HE         LEVEL_HERO /* hero */

#define COM_INGORE 1

struct spec_type {
    char* name; /* special function name */
    SpecFunc* function; /* the function */
};

/*
 * Structure for a command in the command lookup table.
 */
struct cmd_type {
    char* name;
    DoFunc* do_fun;
    int16_t position;
    int16_t level;
    int16_t log;
    int16_t show;
};

/* the command table itself */
#include "command.h"

void interpret(CharData* ch, char* argument);
bool is_number(char* arg);
int number_argument(char* argument, char* arg);
int mult_argument(char* argument, char* arg);
char* one_argument(char* argument, char* arg_first);

/* wrapper function for safe command execution */
void do_function(CharData* ch, DoFunc* do_fun, char* argument);

extern struct cmd_type* cmd_table;

extern int max_cmd;
extern bool fLogAll;

#endif // !MUD98__INTERP_H
