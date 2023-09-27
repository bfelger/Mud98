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
#ifndef MUD98__DB_H
#define MUD98__DB_H

#include "merc.h"

#include "mob_prog.h"

#include "entities/char_data.h"
#include "entities/room_data.h"

#include "data/direction.h"

#include <stdio.h>

/* macro for flag swapping */
#define GET_UNSET(flag1, flag2) (~(flag1) & ((flag1) | (flag2)))

/* Magic number for memory allocation */
#define MAGIC_NUM               52571214

void assign_area_vnum(VNUM vnum);       // OLC
void reset_area(AreaData* pArea);      // OLC
void reset_room(RoomData* pRoom);	    // OLC
char* print_flags(FLAGS flag);
void boot_db(void);
void area_update(void);
void clone_mobile(CharData* parent, CharData* clone);
void clear_char(CharData* ch);
MobProgCode* get_mprog_index(VNUM vnum);
char fread_letter(FILE* fp);
int fread_number(FILE* fp);
long fread_flag(FILE* fp);
char* fread_string(FILE* fp);
char* fread_string_eol(FILE* fp);
void fread_to_eol(FILE* fp);
VNUM fread_vnum(FILE* fp);
char* fread_word(FILE* fp);
long flag_convert(char letter);
void* alloc_mem(size_t sMem);
void* alloc_perm(size_t sMem);
void free_mem(void* pMem, size_t sMem);
char* str_dup(const char* str);
void free_string(char* pstr);
void load_social(FILE* fp);
int number_fuzzy(int number);
int number_range(int from, int to);
int number_percent(void);
Direction number_door();
int number_bits(int width);
long number_mm(void);
int dice(int number, int size);
int interpolate(int level, int value_00, int value_32);
void smash_tilde(char* str);
bool str_cmp(const char* astr, const char* bstr);
bool str_prefix(const char* astr, const char* bstr);
bool str_infix(const char* astr, const char* bstr);
bool str_suffix(const char* astr, const char* bstr);
char* capitalize(const char* str);
void append_file(CharData* ch, char* file, char* str);
void bug(const char* fmt, ...);
void log_string(const char* str);

typedef struct kill_data_t {
    int16_t number;
    int16_t killed;
} KillData;

extern KillData kill_table[MAX_LEVEL];

/* vals from db.c */
extern char bug_buf[];
extern char log_buf[];
extern bool fBootDb;
extern int top_affect;
extern int top_ed;
extern int top_shop;

extern int _filbuf(FILE*);

#endif // !MUD98__DB_H
