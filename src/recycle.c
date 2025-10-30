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
#include "db.h"
#include "digest.h"
#include "recycle.h"
#include "skills.h"

#include "entities/descriptor.h"

#include "data/skill.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef _MSC_VER 
#include <sys/time.h>
#endif

SkillHash* skill_hash_free;
int skill_hash_created;
int skill_hash_allocated;
int skill_hash_freed;

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

Buffer* buf_free;

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

Buffer* new_buf()
{
    Buffer* buffer;

    if (buf_free == NULL)
        buffer = alloc_perm(sizeof(*buffer));
    else {
        buffer = buf_free;
        NEXT_LINK(buf_free);
    }

    buffer->next = NULL;
    buffer->state = BUFFER_SAFE;
    buffer->size = get_size(BASE_BUF);

    buffer->string = alloc_mem(buffer->size);
    buffer->string[0] = '\0';
    VALIDATE(buffer);

    return buffer;
}

Buffer* new_buf_size(int size)
{
    Buffer* buffer;

    if (buf_free == NULL)
        buffer = alloc_perm(sizeof(*buffer));
    else {
        buffer = buf_free;
        NEXT_LINK(buf_free);
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

void free_buf(Buffer* buffer)
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

bool addf_buf(Buffer* buffer, char* format, ...)
{
    char buf[MAX_STRING_LENGTH];

    va_list args;

    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);

    return add_buf(buffer, buf);
}

bool add_buf(Buffer* buffer, char* string)
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

void clear_buf(Buffer* buffer)
{
    buffer->string[0] = '\0';
    buffer->state = BUFFER_SAFE;
}

int16_t* new_learned(void) // Return int16_t[skill_count]
{
    int16_t* temp;
    int i;

    if ((temp = (int16_t*)malloc(sizeof(int16_t) * skill_count)) == NULL) {
        perror("malloc() failed in new_learned()");
        exit(-1);
    }

    for (i = 0; i < skill_count; ++i)
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

SkillHash* new_skill_hash(void)
{
    SkillHash* temp;
    static SkillHash tZero;

    if (skill_hash_free) {
        temp = skill_hash_free;
        NEXT_LINK(skill_hash_free);
    }
    else {
        temp = alloc_mem(sizeof(*temp));
        skill_hash_allocated++;
    }
    *temp = tZero;
    skill_hash_created++;
    return temp;
}

void free_skill_hash(SkillHash* temp)
{
    temp->next = skill_hash_free;
    skill_hash_free = temp;
    skill_hash_freed++;
}
