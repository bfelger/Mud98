////////////////////////////////////////////////////////////////////////////////
// gedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "magic.h"
#include "olc.h"
#include "skills.h"

#include "entities/descriptor.h"
#include "entities/player_data.h"

#include "data/class.h"
#include "data/mobile_data.h"
#include "data/skill.h"

#define GEDIT(fun) bool fun(Mobile *ch, char *argument)

const OlcCmd gedit_table[] = {
    { "name",		gedit_name	    },
    { "rating",	    gedit_rating	},
    { "spell",	    gedit_spell	    },
    { "list",		gedit_list	    },
    { "commands",	show_commands	},
    { "show",		gedit_show	    },
    { NULL,		    0		        }
};

void gedit(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;

    strcpy(arg, argument);
    READ_ARG(command);

    if (ch->pcdata->security < MIN_SKEDIT_SECURITY) {
        send_to_char("GEdit: You do not have enough security to edit groups.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (command[0] == '\0') {
        gedit_show(ch, argument);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    if (!str_cmp(command, "save")) {
        save_skill_group_table();
        return;
    }

    for (cmd = 0; gedit_table[cmd].name != NULL; cmd++) {
        if (!str_prefix(command, gedit_table[cmd].name)) {
            if ((*gedit_table[cmd].olc_fun) (ch, argument))
                save_skill_group_table();
            return;
        }
    }

    interpret(ch, arg);
    return;
}

void do_gedit(Mobile* ch, char* argument)
{
    const SkillGroup* pGroup;
    char command[MSL];
    int group;

    if (IS_NPC(ch))
        return;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : GEdit [group]\n\r", ch);
        return;
    }

    if (ch->pcdata->security < MIN_SKEDIT_SECURITY) {
        send_to_char("GEdit : You do not have enough security to edit groups.\n\r", ch);
        return;
    }

    READ_ARG(command);

    if ((group = group_lookup(command)) == -1) {
        send_to_char("GEdit : That group does not exist\n\r", ch);
        return;
    }

    pGroup = &skill_group_table[group];

    ch->desc->pEdit = (uintptr_t)pGroup;
    ch->desc->editor = ED_GROUP;

    return;
}

GEDIT(gedit_show)
{
    SkillGroup* pGrp;
    char buf[MIL], buf2[MIL];
    int i;

    EDIT_GROUP(ch, pGrp);

    sprintf(buf, "Name      : [%s]\n\r", pGrp->name);
    send_to_char(buf, ch);

    sprintf(buf, "Class    + ");

    for (i = 0; i < class_count; ++i) {
        strcat(buf, class_table[i].name);
        strcat(buf, " ");
    }

    strcat(buf, "\n\r");
    send_to_char(buf, ch);

    sprintf(buf, "Rating   | ");

    for (i = 0; i < class_count; ++i) {
        sprintf(buf2, "%3d ", GET_ELEM(&pGrp->rating, i));
        strcat(buf, buf2);
    }

    strcat(buf, "\n\r");
    send_to_char(buf, ch);

    i = 0;

    while (i < MAX_IN_GROUP && !IS_NULLSTR(pGrp->skills[i])) {
        sprintf(buf, "%2d. " COLOR_ALT_TEXT_1 "%s" COLOR_CLEAR "\n\r", i, pGrp->skills[i]);
        send_to_char(buf, ch);
        i++;
    }

    return false;
}

GEDIT(gedit_name)
{
    SkillGroup* pGrp;

    EDIT_GROUP(ch, pGrp);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : name [group name]\n\r", ch);
        return false;
    }

    if (group_lookup(argument) != -1) {
        send_to_char("GEdit : That group already exists.\n\r", ch);
        return false;
    }

    free_string(pGrp->name);
    pGrp->name = str_dup(argument);

    send_to_char("Ok.\n\r", ch);
    return true;
}

GEDIT(gedit_rating)
{
    SkillGroup* pGrp;
    char arg[MIL];
    SkillRating rating;
    int16_t class_;

    EDIT_GROUP(ch, pGrp);

    READ_ARG(arg);

    if (IS_NULLSTR(argument) || IS_NULLSTR(arg) || !is_number(argument)) {
        send_to_char("Syntax : rating [class] [cost]\n\r", ch);
        return false;
    }

    if ((class_ = class_lookup(arg)) < 0) {
        send_to_char("GEdit : That class does not exist.\n\r", ch);
        return false;
    }

    rating = (SkillRating)atoi(argument);

    if (rating < -1) {
        send_to_char("GEdit : Rating must be at least 0.\n\r", ch);
        return false;
    }

    SET_ELEM(pGrp->rating, class_, rating);
    send_to_char("Ok.\n\r", ch);
    return true;
}

GEDIT(gedit_spell)
{
    SkillGroup* pGrp;
    char arg[MSL];
    int i = 0, j = 0;

    EDIT_GROUP(ch, pGrp);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax: spell new [name]\n\r", ch);
        send_to_char("        spell delete [name]\n\r", ch);
        send_to_char("        spell delete [number]\n\r", ch);
        return false;
    }

    READ_ARG(arg);

    if (!str_cmp(arg, "new")) {
        for (i = 0; !IS_NULLSTR(pGrp->skills[i]) && (i < MAX_IN_GROUP); ++i)
            ;

        if (i == MAX_IN_GROUP) {
            send_to_char("GEdit : the group is full.\n\r", ch);
            return false;
        }

        if (skill_lookup(argument) == -1 && group_lookup(argument) == -1) {
            send_to_char("GEdit : skill/spell/group does not exist.\n\r", ch);
            return false;
        }

        free_string(pGrp->skills[i]);
        pGrp->skills[i] = str_dup(argument);
        send_to_char("Ok.\n\r", ch);
        return true;
    }

    if (!str_cmp(arg, "delete")) {
        int num = is_number(argument) ? atoi(argument) : -1;

        if (is_number(argument) && (num < 0 || num >= MAX_IN_GROUP)) {
            send_to_char("GEdit : Invalid argument.\n\r", ch);
            return false;
        }

        while (i < MAX_IN_GROUP && !IS_NULLSTR(pGrp->skills[i])) {
            if (i == num || !str_cmp(pGrp->skills[i], argument)) {
                for (j = i; j < MAX_IN_GROUP - 1; ++j) {
                    free_string(pGrp->skills[j]);
                    pGrp->skills[j] = str_dup(pGrp->skills[j + 1]);
                }
                free_string(pGrp->skills[MAX_IN_GROUP - 1]);
                pGrp->skills[MAX_IN_GROUP - 1] = str_dup("");
                send_to_char("Ok.\n\r", ch);
                return true;
            }
            ++i;
        }

        send_to_char("GEdit : Skill/spell/group does not exist.\n\r", ch);
        return false;
    }

    gedit_spell(ch, "");
    return false;
}

GEDIT(gedit_list)
{
    const SkillGroup* pGrp;
    int i, cnt = 0;
    char buf[MIL];

    for (i = 0; i < skill_group_count; ++i) {
        if ((pGrp = &skill_group_table[i]) == NULL)
            break;

        sprintf(buf, "%2d. " COLOR_ALT_TEXT_1 "%20s" COLOR_CLEAR " ", i, skill_group_table[i].name);

        if (cnt++ % 2)
            strcat(buf, "\n\r");
        else
            strcat(buf, " - ");

        send_to_char(buf, ch);
    }

    if (cnt % 2)
        send_to_char("\n\r", ch);

    return false;
}
