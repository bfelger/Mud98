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
#ifndef MUD98__RECYCLE_H
#define MUD98__RECYCLE_H

#include "merc.h"

/* stuff for providing a crash-proof buffer */

#define MAX_BUF         MAX_STRING_LENGTH*4
#define MAX_BUF_LIST    13
#define BASE_BUF        1024

/* valid states */
#define BUFFER_SAFE     0
#define BUFFER_OVERFLOW 1
#define BUFFER_FREED    2

/* note recycling */
#define ND              NOTE_DATA
ND* new_note args((void));
void free_note args((NOTE_DATA * note));
#undef ND

/* ban data recycling */
#define BD BAN_DATA
BD* new_ban args((void));
void free_ban args((BAN_DATA * ban));
#undef BD

/* descriptor recycling */
#define DD DESCRIPTOR_DATA
DD* new_descriptor args((void));
void free_descriptor args((DESCRIPTOR_DATA * d));
#undef DD

/* char gen data recycling */
#define GD GEN_DATA
GD* new_gen_data args((void));
void free_gen_data args((GEN_DATA * gen));
#undef GD

/* extra descr recycling */
#define ED EXTRA_DESCR_DATA
ED* new_extra_descr args((void));
void free_extra_descr args((EXTRA_DESCR_DATA * ed));
#undef ED

/* affect recycling */
#define AD AFFECT_DATA
AD* new_affect args((void));
void free_affect args((AFFECT_DATA * af));
#undef AD

/* object recycling */
#define OD OBJ_DATA
OD* new_obj args((void));
void free_obj args((OBJ_DATA * obj));
#undef OD

/* character recyling */
#define CD CharData
CD* new_char args((void));
void free_char args((CharData * ch));
#undef CD

/* mob id and memory procedures */
#define MD MEM_DATA
long get_pc_id args((void));
long get_mob_id args((void));
MD* new_mem_data args((void));
void free_mem_data args((MEM_DATA * memory));
MD* find_memory args((MEM_DATA * memory, long id));
#undef MD

/* buffer procedures */

BUFFER* new_buf(void);
BUFFER* new_buf_size(int size);
void free_buf(BUFFER * buffer);
bool add_buf(BUFFER * buffer, char* string);
void clear_buf(BUFFER * buffer);

#define INIT_BUF(b, sz) BUFFER* b = new_buf_size(sz)
#define SET_BUF(b, s) clear_buf(b); add_buf(b, s)
#define BUF(b) (b->string)

// OLC procedures
HELP_AREA* new_had args((void));
HELP_DATA* new_help args((void));
void free_help args((HELP_DATA*));

SKNUM* new_learned();
void free_learned(SKNUM*);

bool* new_boolarray(size_t);
void free_boolarray(bool*);

struct skhash* new_skhash(); //allocfunc(struct skhash, skhash)
void free_skhash(struct skhash*);

/* externs */
extern char str_empty[1];
extern int mobile_count;

#endif // !MUD98__RECYCLE_H
