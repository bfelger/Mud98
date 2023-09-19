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

#include "db.h"
#include "lookup.h"
#include "tables.h"

#include "entities/mob_prototype.h"
#include "entities/object_data.h"

#include "data/social.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#ifndef _MSC_VER
#include <sys/time.h>
#endif

extern FLAGS flag_lookup(const char* name, const struct flag_type* flag_table);

/* snarf a socials file */
void load_social(FILE* fp)
{
    extern int maxSocial;

    for (;;) {
        Social social = { 0 };
        char* temp;
        /* clear social */
        social.char_no_arg = NULL;
        social.others_no_arg = NULL;
        social.char_found = NULL;
        social.others_found = NULL;
        social.vict_found = NULL;
        social.char_not_found = NULL;
        social.char_auto = NULL;
        social.others_auto = NULL;

        temp = fread_word(fp);
        if (!strcmp(temp, "#0")) return; /* done */
#if defined(social_debug)
        else
            fprintf(stderr, "%s\n\r", temp);
#endif

        social.name = str_dup(temp);
        fread_to_eol(fp);

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_no_arg = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_no_arg = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_found = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_found = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.vict_found = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.vict_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_not_found = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_not_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_auto = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_auto = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_auto = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_auto = temp;

        social_table[social_count] = social;
        social_count++;
        maxSocial++;
    }
    return;
}
