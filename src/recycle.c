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

#include "merc.h"

#include "color.h"
#include "comm.h"
#include "db.h"
#include "digest.h"
#include "recycle.h"
#include "skills.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef _MSC_VER 
#include <sys/time.h>
#endif

struct skhash* skhash_free;
int skhash_created;
int skhash_allocated;
int skhash_freed;

/* stuff for recycling ban structures */
BAN_DATA* ban_free;

BAN_DATA* new_ban(void)
{
    static BAN_DATA ban_zero;
    BAN_DATA* ban;

    if (ban_free == NULL)
        ban = alloc_perm(sizeof(*ban));
    else {
        ban = ban_free;
        ban_free = ban_free->next;
    }

    *ban = ban_zero;
    VALIDATE(ban);
    ban->name = &str_empty[0];
    return ban;
}

void free_ban(BAN_DATA* ban)
{
    if (!IS_VALID(ban)) return;

    free_string(ban->name);
    INVALIDATE(ban);

    ban->next = ban_free;
    ban_free = ban;
}

/* stuff for recycling descriptors */
DESCRIPTOR_DATA* descriptor_free = NULL;

DESCRIPTOR_DATA* new_descriptor()
{
    static DESCRIPTOR_DATA d_zero;
    DESCRIPTOR_DATA* d;

    if (descriptor_free == NULL)
        d = alloc_perm(sizeof(*d));
    else {
        d = descriptor_free;
        descriptor_free = descriptor_free->next;
    }

    *d = d_zero;
    VALIDATE(d);
    return d;
}

void free_descriptor(DESCRIPTOR_DATA* d)
{
    if (!IS_VALID(d)) return;

    free_string(d->host);
    free_mem(d->outbuf, d->outsize);
    INVALIDATE(d);
    d->next = descriptor_free;
    descriptor_free = d;
}

/* stuff for recycling gen_data */
GEN_DATA* gen_data_free;

GEN_DATA* new_gen_data(void)
{
    static GEN_DATA gen_zero;
    GEN_DATA* gen;

    if (gen_data_free == NULL)
        gen = alloc_perm(sizeof(*gen));
    else {
        gen = gen_data_free;
        gen_data_free = gen_data_free->next;
    }
    *gen = gen_zero;

    gen->skill_chosen = new_boolarray(max_skill);
    gen->group_chosen = new_boolarray(max_group);

    VALIDATE(gen);
    return gen;
}

void free_gen_data(GEN_DATA* gen)
{
    if (!IS_VALID(gen)) return;

    INVALIDATE(gen);

    free_boolarray(gen->skill_chosen);
    free_boolarray(gen->group_chosen);

    gen->next = gen_data_free;
    gen_data_free = gen;
}

/* stuff for recycling affects */
AFFECT_DATA* affect_free;

AFFECT_DATA* new_affect(void)
{
    static AFFECT_DATA af_zero;
    AFFECT_DATA* af;

    if (affect_free == NULL)
        af = alloc_perm(sizeof(*af));
    else {
        af = affect_free;
        affect_free = affect_free->next;
    }

    *af = af_zero;

    VALIDATE(af);
    return af;
}

void free_affect(AFFECT_DATA* af)
{
    if (!IS_VALID(af)) return;

    INVALIDATE(af);
    af->next = affect_free;
    affect_free = af;
}

/* stuff for setting ids */
long last_pc_id;

long get_pc_id(void)
{
    long val = (long)((current_time <= last_pc_id)
        ? (time_t)last_pc_id + 1
        : current_time);
    last_pc_id = val;
    return val;
}

////////////////////////////////////////////////////////////////////////////////
// Buffer recycling
////////////////////////////////////////////////////////////////////////////////

BUFFER* buf_free;

/* buffer sizes */
const size_t buf_size[MAX_BUF_LIST] = {
    16, 32, 64, 128, 256, 1024, 2048, 4096,
    MAX_STRING_LENGTH,        // vvv
    8192,                   
    MAX_STRING_LENGTH * 2,    // Doesn't follow pattern, but frequently used.
    16384,
    MAX_STRING_LENGTH * 4     // ^^^
};

/* local procedure for finding the next acceptable size */
/* SIZE_MAX indicates out-of-boundary error */
static size_t get_size(size_t val)
{
    for (int i = 0; i < MAX_BUF_LIST; i++) {
        if (buf_size[i] >= val)
            return buf_size[i];
    }

    return SIZE_MAX;
}

BUFFER* new_buf()
{
    BUFFER* buffer;

    if (buf_free == NULL)
        buffer = alloc_perm(sizeof(*buffer));
    else {
        buffer = buf_free;
        buf_free = buf_free->next;
    }

    buffer->next = NULL;
    buffer->state = BUFFER_SAFE;
    buffer->size = get_size(BASE_BUF);

    buffer->string = alloc_mem(buffer->size);
    buffer->string[0] = '\0';
    VALIDATE(buffer);

    return buffer;
}

BUFFER* new_buf_size(int size)
{
    BUFFER* buffer;

    if (buf_free == NULL)
        buffer = alloc_perm(sizeof(*buffer));
    else {
        buffer = buf_free;
        buf_free = buf_free->next;
    }

    buffer->next = NULL;
    buffer->state = BUFFER_SAFE;
    buffer->size = get_size(size);
    if (buffer->size == SIZE_MAX) {
        bug("new_buf: buffer size %d too large.", size);
        exit(1);
    }
    buffer->string = alloc_mem(buffer->size);
    buffer->string[0] = '\0';
    VALIDATE(buffer);

    return buffer;
}

void free_buf(BUFFER* buffer)
{
    if (!IS_VALID(buffer)) return;

    free_mem(buffer->string, buffer->size);
    buffer->string = NULL;
    buffer->size = 0;
    buffer->state = BUFFER_FREED;
    INVALIDATE(buffer);

    buffer->next = buf_free;
    buf_free = buffer;
}

bool add_buf(BUFFER* buffer, char* string)
{
    char* oldstr;
    size_t oldsize;

    oldstr = buffer->string;
    oldsize = buffer->size;

    if (buffer->state == BUFFER_OVERFLOW) /* don't waste time on bad strings! */
        return false;

    size_t len = strlen(buffer->string) + strlen(string) + 1;

    if (len >= buffer->size) /* increase the buffer size */
    {
        buffer->size = get_size(len);
        if (buffer->size == SIZE_MAX) /* overflow */
        {
            buffer->size = oldsize;
            buffer->state = BUFFER_OVERFLOW;
            bug("buffer overflow past size %d", buffer->size);
            return false;
        }
    }

    if (buffer->size != oldsize) {
        buffer->string = alloc_mem(buffer->size);

        strcpy(buffer->string, oldstr);
        free_mem(oldstr, oldsize);
    }

    strcat(buffer->string, string);
    return true;
}

void clear_buf(BUFFER* buffer)
{
    buffer->string[0] = '\0';
    buffer->state = BUFFER_SAFE;
}

////////////////////////////////////////////////////////////////////////////////
// MobProg recycling
////////////////////////////////////////////////////////////////////////////////

MPROG_LIST* mprog_free = NULL;

MPROG_LIST* new_mprog()
{
    MPROG_LIST* mp;

    if (mprog_free == NULL)
        mp = alloc_perm(sizeof(MPROG_LIST));
    else {
        mp = mprog_free;
        mprog_free = mprog_free->next;
    }

    memset(mp, 0, sizeof(MPROG_LIST));
    mp->vnum = 0;
    mp->trig_type = 0;
    mp->code = str_dup("");
    VALIDATE(mp);
    return mp;
}

void free_mprog(MPROG_LIST* mp)
{
    if (!IS_VALID(mp))
        return;

    INVALIDATE(mp);
    mp->next = mprog_free;
    mprog_free = mp;
}


int16_t* new_learned(void) // Return int16_t[max_skill]
{
    int16_t* temp;
    int i;

    if ((temp = (int16_t*)malloc(sizeof(int16_t) * max_skill)) == NULL) {
        perror("malloc() failed in new_learned()");
        exit(-1);
    }

    for (i = 0; i < max_skill; ++i)
        temp[i] = 0;

    return temp;
}

void free_learned(int16_t* temp)
{
    free(temp);
    temp = NULL;
    return;
}

bool* new_boolarray(size_t size) // Return bool[size]
{
    bool* temp;

    if (size <= 0)
        size = 1;

    if ((temp = (bool*)malloc(sizeof(bool) * size)) == NULL) {
        perror("malloc failed in new_boolarray()");
        exit(-1);
    }

    for (size_t i = 0; i < size; ++i)
        temp[i] = false;

    return temp;
}

void free_boolarray(bool* temp)
{
    free(temp);
    temp = NULL;
    return;
}

struct skhash* new_skhash(void)
{
    struct skhash* temp;
    static struct skhash tZero;

    if (skhash_free) {
        temp = skhash_free;
        skhash_free = skhash_free->next;
    }
    else {
        temp = alloc_mem(sizeof(*temp));
        skhash_allocated++;
    }
    *temp = tZero;
    skhash_created++;
    return temp;
}

void free_skhash(struct skhash* temp)
{
    temp->next = skhash_free;
    skhash_free = temp;
    skhash_freed++;
}
