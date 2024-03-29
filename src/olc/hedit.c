////////////////////////////////////////////////////////////////////////////////
// hedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "comm.h"
#include "db.h"
#include "lookup.h"
#include "olc.h"
#include "recycle.h"
#include "string_edit.h"
#include "tables.h"

#include "entities/area.h"
#include "entities/descriptor.h"
#include "entities/player_data.h"

#include "data/mobile_data.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#include <sys/types.h>
#include <time.h>

#define HEDIT(fun) bool fun(Mobile *ch, char*argument)

const OlcCmd hedit_table[] = {
/*	{	command		function	}, */
    {	"keyword",	hedit_keyword	},
    {	"text",		hedit_text	},
    {	"new",		hedit_new	},
    {	"level",	hedit_level	},
    {	"commands",	show_commands	},
    {	"delete",	hedit_delete	},
    {	"list",		hedit_list	},
    {	"show",		hedit_show	},
    {	"?",		show_help	},
    {	NULL,		0		}
};

HelpArea* get_help_area(HelpData* help)
{
    HelpArea* temp;
    HelpData* thelp;

    FOR_EACH(temp, help_area_list)
        for (thelp = temp->first; thelp; thelp = thelp->next_area)
            if (thelp == help)
                return temp;

    return NULL;
}

HEDIT(hedit_show)
{
    HelpData* help;
    char buf[MSL * 2];

    EDIT_HELP(ch, help);

    sprintf(buf, "Keyword : [%s]\n\r"
        "Level   : [%d]\n\r"
        "Text    :\n\r"
        "%s-END-\n\r",
        help->keyword,
        help->level,
        help->text);
    send_to_char(buf, ch);

    return false;
}

HEDIT(hedit_level)
{
    HelpData* help;
    LEVEL lev;

    EDIT_HELP(ch, help);

    if (IS_NULLSTR(argument) || !is_number(argument)) {
        send_to_char("Syntax : level [-1..MAX_LEVEL]\n\r", ch);
        return false;
    }

    lev = (LEVEL)atoi(argument);

    if (lev < -1 || lev > MAX_LEVEL) {
        printf_to_char(ch, "HEdit : level must be between -1 and %d, inclusive.\n\r", MAX_LEVEL);
        return false;
    }

    help->level = lev;
    send_to_char("Ok.\n\r", ch);
    return true;
}

HEDIT(hedit_keyword)
{
    HelpData* help;

    EDIT_HELP(ch, help);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : keyword [keyword(s)]\n\r", ch);
        return false;
    }

    free_string(help->keyword);
    help->keyword = str_dup(argument);

    send_to_char("Ok.\n\r", ch);
    return true;
}

HEDIT(hedit_new)
{
    char arg[MIL], fullarg[MIL];
    HelpArea* had;
    HelpData* help;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : new [name]\n\r", ch);
        send_to_char("         new [area] [name]\n\r", ch);
        return false;
    }

    strcpy(fullarg, argument);
    READ_ARG(arg);

    if (!(had = had_lookup(arg))) {
        had = ch->in_room->area->data->helps;
        argument = fullarg;
    }

    if (help_lookup(argument)) {
        send_to_char("HEdit : That helpfile already exists.\n\r", ch);
        return false;
    }

    // No helpfiles in this area yet
    if (!had) {
        had = new_help_area();
        had->filename = str_dup(ch->in_room->area->data->file_name);
        had->area_data = ch->in_room->area->data;
        had->first = NULL;
        had->last = NULL;
        had->changed = true;
        had->next = help_area_list;
        help_area_list = had;
        ch->in_room->area->data->helps = had;
        SET_BIT(ch->in_room->area->data->area_flags, AREA_CHANGED);
    }

    help = new_help_data();
    help->level = 0;
    help->keyword = str_dup(argument);
    help->text = str_dup("");

    if (help_last)
        help_last->next = help;

    if (help_first == NULL)
        help_first = help;

    help_last = help;
    help->next = NULL;

    if (!had->first)
        had->first = help;
    if (!had->last)
        had->last = help;

    had->last->next_area = help;
    had->last = help;
    help->next_area = NULL;

    ch->desc->pEdit = (uintptr_t)help;
    ch->desc->editor = ED_HELP;

    send_to_char("Ok.\n\r", ch);
    return false;
}

HEDIT(hedit_text)
{
    HelpData* help;

    EDIT_HELP(ch, help);

    if (!IS_NULLSTR(argument)) {
        send_to_char("Syntax : text\n\r", ch);
        return false;
    }

    string_append(ch, &help->text);

    return true;
}

void hedit(Mobile* ch, char* argument)
{
    HelpData* pHelp;
    HelpArea* had;
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;

    smash_tilde(argument);
    strcpy(arg, argument);
    READ_ARG(command);

    EDIT_HELP(ch, pHelp);

    had = get_help_area(pHelp);

    if (had == NULL) {
        bugf("hedit : had for helpfile %s is NULL", pHelp->keyword);
        edit_done(ch);
        return;
    }

    if (ch->pcdata->security < MIN_HEDIT_SECURITY) {
        send_to_char("HEdit: You do not have enough security to edit helpfiles.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (command[0] == '\0') {
        hedit_show(ch, argument);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    for (cmd = 0; hedit_table[cmd].name != NULL; cmd++) {
        if (!str_prefix(command, hedit_table[cmd].name)) {
            if ((*hedit_table[cmd].olc_fun) (ch, argument))
                had->changed = true;
            return;
        }
    }

    interpret(ch, arg);
    return;
}

void do_hedit(Mobile* ch, char* argument)
{
    HelpData* pHelp;

    if (IS_NPC(ch))
        return;

    if ((pHelp = help_lookup(argument)) == NULL) {
        send_to_char("HEdit : That helpfile does not exist.\n\r", ch);
        return;
    }

    ch->desc->pEdit = (uintptr_t)pHelp;
    ch->desc->editor = ED_HELP;

    return;
}

HEDIT(hedit_delete)
{
    HelpData* pHelp, * temp;
    HelpArea* had;
    Descriptor* d;
    bool found = false;

    EDIT_HELP(ch, pHelp);

    FOR_EACH(d, descriptor_list)
        if (d->editor == ED_HELP && pHelp == (HelpData*)d->pEdit)
            edit_done(d->character);

    if (help_first == pHelp)
        NEXT_LINK(help_first);
    else {
        FOR_EACH(temp, help_first)
            if (temp->next == pHelp)
                break;

        if (!temp) {
            bugf("hedit_delete : help %s was not found in help_first", pHelp->keyword);
            return false;
        }

        temp->next = pHelp->next;
    }

    FOR_EACH(had, help_area_list)
        if (pHelp == had->first) {
            found = true;
            had->first = had->first->next_area;
        }
        else {
            for (temp = had->first; temp; temp = temp->next_area)
                if (temp->next_area == pHelp)
                    break;

            if (temp) {
                temp->next_area = pHelp->next_area;
                found = true;
                break;
            }
        }

    if (!found) {
        bugf("hedit_delete : help %s was not found in help_area_list", pHelp->keyword);
        return false;
    }

    free_help_data(pHelp);

    send_to_char("Ok.\n\r", ch);
    return true;
}

HEDIT(hedit_list)
{
    char buf[MIL];
    int cnt = 0;
    HelpData* pHelp;
    Buffer* buffer;

    EDIT_HELP(ch, pHelp);

    if (!str_cmp(argument, "all")) {
        buffer = new_buf();

        FOR_EACH(pHelp, help_first) {
            sprintf(buf, "%3d. %-14.14s%s", cnt, pHelp->keyword,
                cnt % 4 == 3 ? "\n\r" : " ");
            add_buf(buffer, buf);
            cnt++;
        }

        if (cnt % 4)
            add_buf(buffer, "\n\r");

        page_to_char(BUF(buffer), ch);
        return false;
    }

    if (!str_cmp(argument, "area")) {
        if (ch->in_room->area->data->helps == NULL) {
            send_to_char("There are no helpfiles in this area.\n\r", ch);
            return false;
        }

        buffer = new_buf();

        for (pHelp = ch->in_room->area->data->helps->first; pHelp; pHelp = pHelp->next_area) {
            sprintf(buf, "%3d. %-14.14s%s", cnt, pHelp->keyword,
                cnt % 4 == 3 ? "\n\r" : " ");
            add_buf(buffer, buf);
            cnt++;
        }

        if (cnt % 4)
            add_buf(buffer, "\n\r");

        page_to_char(BUF(buffer), ch);
        return false;
    }

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : list all\n\r", ch);
        send_to_char("         list area\n\r", ch);
        return false;
    }

    return false;
}
