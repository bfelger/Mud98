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

struct flag_type;

#pragma once
#ifndef MUD98__TABLES_H
#define MUD98__TABLES_H

#include "merc.h"

#include <stdbool.h>

struct flag_type {
    char* name;
    FLAGS bit;
    bool settable;
};

#include "entities/mobile.h"

#include "data/damage.h"

typedef enum cmd_type_t {
    TYP_NUL         = 0,
    TYP_UNDEF       = 1,
    TYP_CBT	        = 2,
    TYP_SPC         = 3,
    TYP_GRP         = 4,
    TYP_OBJ         = 5,
    TYP_INF         = 6,
    TYP_OTH         = 7,
    TYP_MVT         = 8,
    TYP_CNF         = 9,
    TYP_CMM         = 10,
    TYP_LNG         = 11,
    TYP_PLR         = 12,
    TYP_OLC         = 13,
} CmdType;


struct clan_type {
    char* name;
    char* who_name;
    int16_t hall;
    bool independent; /* true for loners */
};

struct bit_type {
    const struct flag_type* table;
    char* help;
};

struct recval_type {
    int16_t numhit;
    int16_t typhit;
    int16_t bonhit;
    int16_t ac;
    int16_t numdam;
    int16_t typdam;
    int16_t bondam;
};

/* game tables */
extern const struct clan_type clan_table[MAX_CLAN];

/* flag tables */
extern const struct flag_type act_flag_table[];
extern const struct flag_type plr_flag_table[];
extern const struct flag_type affect_flag_table[];
extern const struct flag_type off_flag_table[];
extern const struct flag_type imm_flag_table[];
extern const struct flag_type form_flag_table[];
extern const struct flag_type part_flag_table[];
extern const struct flag_type comm_flag_table[];
extern const struct flag_type extra_flag_table[];
extern const struct flag_type wear_flag_table[];
extern const struct flag_type weapon_flag_table[];
extern const struct flag_type container_flag_table[];
extern const struct flag_type portal_flag_table[];
extern const struct flag_type room_flag_table[];
extern const struct flag_type exit_flag_table[];

/* OLC */
extern const struct flag_type mprog_flag_table[];
extern const struct flag_type area_flag_table[];
extern const struct flag_type sector_flag_table[];
extern const struct flag_type door_resets[];
extern const struct flag_type wear_loc_strings[];
extern const struct flag_type wear_loc_flag_table[];
extern const struct flag_type res_flag_table[];
extern const struct flag_type imm_flag_table[];
extern const struct flag_type vuln_flag_table[];
extern const struct flag_type type_flag_table[];
extern const struct flag_type apply_flag_table[];
extern const struct flag_type sex_flag_table[];
extern const struct flag_type furniture_flag_table[];
extern const struct flag_type weapon_class[];
extern const struct flag_type apply_types[];
extern const struct flag_type weapon_type2[];
extern const struct flag_type apply_types[];
extern const struct flag_type size_flag_table[MOB_SIZE_COUNT+1];
extern const struct flag_type position_flag_table[];
extern const struct flag_type ac_type[];
extern const struct bit_type bitvector_type[];
extern const struct recval_type recval_table[];
extern const struct flag_type target_table[];
extern const struct flag_type dam_classes[DAM_TYPE_COUNT];
extern const struct flag_type log_flag_table[];
extern const struct flag_type show_flag_table[];
extern const struct flag_type stat_table[STAT_COUNT+1];
extern const struct flag_type inst_type_table[];

void show_flags_to_char(Mobile* ch, const struct flag_type* flags);

#endif // !MUD98__TABLES_H
