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

#include "alias.h"

#include "comm.h"
#include "db.h"

#include "olc/olc.h"

#include "entities/descriptor.h"
#include "entities/player_data.h"

#include "data/mobile.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

/* does aliasing and other fun stuff */
void substitute_alias(Descriptor* d, char* argument)
{
    CharData* ch;
    char buf[MAX_STRING_LENGTH], prefix[MAX_INPUT_LENGTH],
        name[MAX_INPUT_LENGTH];
    char* point;
    int alias;

    ch = d->original ? d->original : d->character;

    /* check for prefix */
    if (ch->prefix[0] != '\0' && str_prefix("prefix", argument)) {
        if (strlen(ch->prefix) + strlen(argument) > MAX_INPUT_LENGTH)
            send_to_char("Line too long, prefix not processed.\r\n", ch);
        else {
            sprintf(prefix, "%s %s", ch->prefix, argument);
            argument = prefix;
        }
    }

    if (IS_NPC(ch) || ch->pcdata->alias[0] == NULL
        || !str_prefix("alias", argument) || !str_prefix("una", argument)
        || !str_prefix("prefix", argument)) {
        if (!run_olc_editor(d, argument))
            interpret(d->character, argument);
        return;
    }

    strcpy(buf, argument);

    for (alias = 0; alias < MAX_ALIAS; alias++) /* go through the aliases */
    {
        if (ch->pcdata->alias[alias] == NULL) break;

        if (!str_prefix(ch->pcdata->alias[alias], argument)) {
            point = one_argument(argument, name);
            if (!strcmp(ch->pcdata->alias[alias], name)) {
                buf[0] = '\0';
                strcat(buf, ch->pcdata->alias_sub[alias]);
                strcat(buf, " ");
                strcat(buf, point);

                if (strlen(buf) > MAX_INPUT_LENGTH - 1) {
                    send_to_char("Alias substitution too long. Truncated.\r\n",
                                 ch);
                    buf[MAX_INPUT_LENGTH - 1] = '\0';
                }
                break;
            }
        }
    }
    if (!run_olc_editor(d, buf))
        interpret(d->character, buf);
}

void do_alia(CharData* ch, char* argument)
{
    send_to_char("I'm sorry, alias must be entered in full.\n\r", ch);
    return;
}

void do_alias(CharData* ch, char* argument)
{
    CharData* rch;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int idx;

    smash_tilde(argument);

    if (ch->desc == NULL)
        rch = ch;
    else
        rch = ch->desc->original ? ch->desc->original : ch;

    if (IS_NPC(rch)) return;

    READ_ARG(arg);

    if (arg[0] == '\0') {
        if (rch->pcdata->alias[0] == NULL) {
            send_to_char("You have no aliases defined.\n\r", ch);
            return;
        }
        send_to_char("Your current aliases are:\n\r", ch);

        for (idx = 0; idx < MAX_ALIAS; idx++) {
            if (rch->pcdata->alias[idx] == NULL
                || rch->pcdata->alias_sub[idx] == NULL)
                break;

            sprintf(buf, "    %s:  %s\n\r", rch->pcdata->alias[idx],
                    rch->pcdata->alias_sub[idx]);
            send_to_char(buf, ch);
        }
        return;
    }

    if (!str_prefix("una", arg) || !str_cmp("alias", arg)) {
        send_to_char("Sorry, that word is reserved.\n\r", ch);
        return;
    }

    if (argument[0] == '\0') {
        for (idx = 0; idx < MAX_ALIAS; idx++) {
            if (rch->pcdata->alias[idx] == NULL
                || rch->pcdata->alias_sub[idx] == NULL)
                break;

            if (!str_cmp(arg, rch->pcdata->alias[idx])) {
                sprintf(buf, "%s aliases to '%s'.\n\r", rch->pcdata->alias[idx],
                        rch->pcdata->alias_sub[idx]);
                send_to_char(buf, ch);
                return;
            }
        }

        send_to_char("That alias is not defined.\n\r", ch);
        return;
    }

    if (!str_prefix(argument, "delete") || !str_prefix(argument, "prefix")) {
        send_to_char("That shall not be done!\n\r", ch);
        return;
    }

    for (idx = 0; idx < MAX_ALIAS; idx++) {
        if (rch->pcdata->alias[idx] == NULL) break;

        if (!str_cmp(arg, rch->pcdata->alias[idx])) /* redefine an alias */
        {
            free_string(rch->pcdata->alias_sub[idx]);
            rch->pcdata->alias_sub[idx] = str_dup(argument);
            sprintf(buf, "%s is now realiased to '%s'.\n\r", arg, argument);
            send_to_char(buf, ch);
            return;
        }
    }

    if (idx >= MAX_ALIAS) {
        send_to_char("Sorry, you have reached the alias limit.\n\r", ch);
        return;
    }

    /* make a new alias */
    rch->pcdata->alias[idx] = str_dup(arg);
    rch->pcdata->alias_sub[idx] = str_dup(argument);
    sprintf(buf, "%s is now aliased to '%s'.\n\r", arg, argument);
    send_to_char(buf, ch);
}

void do_unalias(CharData* ch, char* argument)
{
    CharData* rch;
    char arg[MAX_INPUT_LENGTH];
    int idx;
    bool found = false;

    if (ch->desc == NULL)
        rch = ch;
    else
        rch = ch->desc->original ? ch->desc->original : ch;

    if (IS_NPC(rch)) return;

    READ_ARG(arg);

    if (arg[0] == '\0') {
        send_to_char("Unalias what?\n\r", ch);
        return;
    }

    for (idx = 0; idx < MAX_ALIAS; idx++) {
        if (rch->pcdata->alias[idx] == NULL) break;

        if (found) {
            rch->pcdata->alias[idx - 1] = rch->pcdata->alias[idx];
            rch->pcdata->alias_sub[idx - 1] = rch->pcdata->alias_sub[idx];
            rch->pcdata->alias[idx] = NULL;
            rch->pcdata->alias_sub[idx] = NULL;
            continue;
        }

        if (!strcmp(arg, rch->pcdata->alias[idx])) {
            send_to_char("Alias removed.\n\r", ch);
            free_string(rch->pcdata->alias[idx]);
            free_string(rch->pcdata->alias_sub[idx]);
            rch->pcdata->alias[idx] = NULL;
            rch->pcdata->alias_sub[idx] = NULL;
            found = true;
        }
    }

    if (!found) send_to_char("No alias of that name to remove.\n\r", ch);
}
