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

/* Online Social Editting Module,
 * (c) 1996 Erwin S. Andreasen <erwin@pip.dknet.dk>
 * See the file "License" for important licensing information
 */

#include "merc.h"

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "olc.h"

#include "entities/descriptor.h"
#include "entities/player_data.h"

#include "data/mobile_data.h"
#include "data/social.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define SEDIT(fun) bool fun(Mobile *ch, char *argument)

Social xSoc;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const OlcCmdEntry social_olc_comm_table[] = {
    { "cnoarg",     U(&xSoc.char_no_arg),   ed_line_string, 0                   },
    { "onoarg",     U(&xSoc.others_no_arg), ed_line_string, 0                   },
    { "cfound",     U(&xSoc.char_found),    ed_line_string, 0                   },
    { "vfound",     U(&xSoc.vict_found),    ed_line_string, 0                   },
    { "ofound",     U(&xSoc.others_found),  ed_line_string, 0                   },
    { "cself",      U(&xSoc.char_auto),     ed_line_string, 0                   },
    { "oself",      U(&xSoc.others_auto),   ed_line_string, 0                   },
    { "new",        0,                      ed_olded,       U(sedit_new)        },
    { "delete",     0,                      ed_olded,       U(sedit_delete)     },
    { "show",       0,                      ed_olded,       U(sedit_show)       },
    { "commands",   0,                      ed_olded,       U(show_commands)    },
    { "version",    0,                      ed_olded,       U(show_version)     },
    { "?",          0,                      ed_olded,       U(show_help)        },
    { NULL,         0,                      NULL,           0                   }
};

/* Find a social based on name */
int social_lookup(const char* name)
{
    int i;

    for (i = 0; i < social_count; i++)
        if (!str_cmp(name, social_table[i].name))
            return i;

    return -1;
}

void sedit(Mobile* ch, char* argument)
{
    if (ch->pcdata->security < MIN_SEDIT_SECURITY) {
        send_to_char("SEdit: You do not have enough security to edit socials.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (emptystring(argument)) {
        sedit_show(ch, argument);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (!process_olc_command(ch, argument, social_olc_comm_table))
        interpret(ch, argument);

    return;
}

void do_sedit(Mobile* ch, char* argument)
{
    Social* pSocial;
    char command[MIL];
    int social;

    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : SEdit [social]\n\r", ch);
        send_to_char("         SEdit new [social]\n\r", ch);
        send_to_char("         SEdit delete [social]\n\r", ch);
        return;
    }

    if (ch->pcdata->security < MIN_SEDIT_SECURITY) {
        send_to_char("SEdit : You do not have enough security to edit socials.\n\r", ch);
        return;
    }

    READ_ARG(command);

    if (!str_cmp(command, "new")) {
        if (sedit_new(ch, argument))
            save_social_table();
        return;
    }

    if (!str_cmp(command, "delete")) {
        if (sedit_delete(ch, argument))
            save_social_table();
        return;
    }

    if ((social = social_lookup(command)) == -1) {
        send_to_char("SEdit : That social does not exist.\n\r", ch);
        return;
    }

    pSocial = &social_table[social];

    ch->desc->pEdit = U(pSocial);
    ch->desc->editor = ED_SOCIAL;

    return;
}

SEDIT(sedit_show)
{
    Social* pSocial;
    char buf[MSL];

    EDIT_SOCIAL(ch, pSocial);

    sprintf(buf, "Social: %s\n\r"
        "(cnoarg) No argument, character sees :\n\r"
        "%s\n\r\n\r"
        "(onoarg) No argument, others see :\n\r"
        "%s\n\r\n\r"
        "(cfound) Victim found, character sees :\n\r"
        "%s\n\r\n\r"
        "(ofound) Victim found, others see :\n\r"
        "%s\n\r\n\r"
        "(vfound) Victim found, victim sees :\n\r"
        "%s\n\r\n\r"
        "(cself) Performed on self, character sees :\n\r"
        "%s\n\r\n\r"
        "(oself) Performed on self, others see :\n\r"
        "%s\n\r",

        pSocial->name,
        pSocial->char_no_arg,
        pSocial->others_no_arg,
        pSocial->char_found,
        pSocial->others_found,
        pSocial->vict_found,
        pSocial->char_auto,
        pSocial->others_auto);

    send_to_char(buf, ch);
    return false;
}

SEDIT(sedit_new)
{
    Descriptor* d;
    Mobile* tch;
    Social* new_table;
    int iSocial;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : new [name]\n\r", ch);
        return false;
    }

    iSocial = social_lookup(argument);

    if (iSocial != -1) {
        send_to_char("There is already a social of that name!\n\r", ch);
        return false;
    }

    FOR_EACH(d, descriptor_list) {
        if (d->connected != CON_PLAYING || (tch = CH(d)) == NULL || tch->desc == NULL)
            continue;

        if (tch->desc->editor == ED_SOCIAL)
            edit_done(ch);
    }

    /* reallocate the table */
    /* Note that the table contains social_count socials PLUS one empty spot! */

    social_count++;
    new_table = realloc(social_table, sizeof(Social) * ((size_t)social_count + 1));

    if (!new_table) /* realloc failed */
    {
        send_to_char("Realloc failed. Prepare for impact.\n\r", ch);
        return false;
    }

    social_table = new_table;

    social_table[social_count - 1].name = str_dup(argument);
    social_table[social_count - 1].char_no_arg = str_dup("");
    social_table[social_count - 1].others_no_arg = str_dup("");
    social_table[social_count - 1].char_found = str_dup("");
    social_table[social_count - 1].others_found = str_dup("");
    social_table[social_count - 1].vict_found = str_dup("");
    social_table[social_count - 1].char_auto = str_dup("");
    social_table[social_count - 1].others_auto = str_dup("");
    social_table[social_count].name = str_dup(""); /* 'terminating' empty string */

    ch->desc->editor = ED_SOCIAL;
    ch->desc->pEdit = U(&social_table[social_count - 1]);

    send_to_char("New social created.\n\r", ch);
    return true;
}

SEDIT(sedit_delete)
{
    Descriptor* d;
    Mobile* tch;
    int i, j, iSocial;
    Social* new_table;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : delete [name]\n\r", ch);
        return false;
    }

    iSocial = social_lookup(argument);

    if (iSocial == -1) {
        send_to_char("SEdit : That social does not exist. Perhaps someone else got to it first. :)\n\r", ch);
        return false;
    }

    FOR_EACH(d, descriptor_list) {
        if (d->connected != CON_PLAYING || (tch = CH(d)) == NULL || tch->desc == NULL)
            continue;

        if (tch->desc->editor == ED_SOCIAL)
            edit_done(ch);
    }

    new_table = malloc(sizeof(Social) * social_count);

    if (!new_table) {
        send_to_char("Memory allocation failed. Brace for impact...\n\r", ch);
        return false;
    }

    /* Copy all elements of old table into new table, except the deleted social */
    for (i = 0, j = 0; i < social_count + 1; i++)
        if (i != iSocial) /* copy, increase only if copied */
        {
            new_table[j] = social_table[i];
            j++;
        }

    free(social_table);
    social_table = new_table;

    social_count--; /* Important :() */

    send_to_char("That social is history!\n\r", ch);
    return true;
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
