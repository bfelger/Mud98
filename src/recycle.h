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

typedef struct buffer_t Buffer;

#pragma once
#ifndef MUD98__RECYCLE_H
#define MUD98__RECYCLE_H

#include "merc.h"

#include "skills.h"

/* stuff for providing a crash-proof buffer */

#define MAX_BUF         MAX_STRING_LENGTH*4
#define MAX_BUF_LIST    13
#define BASE_BUF        1024

/* valid states */
#define BUFFER_SAFE     0
#define BUFFER_OVERFLOW 1
#define BUFFER_FREED    2

typedef struct buffer_t {
    Buffer* next;
    char* string; /* buffer's string */
    size_t size;  /* size in k */
    int state; /* error state of the buffer */
    bool valid;
} Buffer;

/* descriptor recycling */
#define DD DESCRIPTOR_DATA
DD* new_descriptor args((void));
void free_descriptor args((DESCRIPTOR_DATA * d));
#undef DD

/* char gen data recycling */
#define GD CharGenData
GD* new_gen_data args((void));
void free_gen_data args((CharGenData * gen));
#undef GD

long get_pc_id(void);

/* buffer procedures */

Buffer* new_buf();
Buffer* new_buf_size(int size);
void free_buf(Buffer * buffer);
bool add_buf(Buffer * buffer, char* string);
void clear_buf(Buffer * buffer);

#define INIT_BUF(b, sz) Buffer* b = new_buf_size(sz)
#define SET_BUF(b, s) clear_buf(b); add_buf(b, s)
#define BUF(b) (b->string)

MPROG_LIST* new_mprog();
void free_mprog(MPROG_LIST* mp);

SKNUM* new_learned();
void free_learned(SKNUM*);

bool* new_boolarray(size_t);
void free_boolarray(bool*);

SkillHash* new_skill_hash();
void free_skill_hash(SkillHash*);

/* externs */
extern char str_empty[1];

#endif // !MUD98__RECYCLE_H
