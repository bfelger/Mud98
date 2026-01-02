////////////////////////////////////////////////////////////////////////////////
// gedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "comm.h"
#include "config.h"
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

#ifdef _MSC_VER
#include <io.h>
#define access _access
#else
#include <unistd.h>
#endif

#ifndef F_OK
#define F_OK 0
#endif

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
        send_to_char(COLOR_INFO "You do not have enough security to edit groups." COLOR_EOL, ch);
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

    if (!str_prefix(command, "save")) {
        char arg2[MIL];
        one_argument(argument, arg2); // optional format
        const char* requested_ext = NULL;
        bool force_format = false;
        if (!str_cmp(arg2, "json")) {
            requested_ext = ".json";
            force_format = true;
        }
        else if (!str_cmp(arg2, "olc")) {
            requested_ext = ".olc";
            force_format = true;
        }
        const char* groups_file = cfg_get_groups_file();
        const char* ext = strrchr(groups_file, '.');
        bool has_ext = (ext != NULL);

        if (!force_format) {
            if (has_ext) {
                requested_ext = NULL;
            }
            else {
                if (access(groups_file, F_OK) != 0) {
                    const char* def = cfg_get_default_format();
                    if (def && !str_cmp(def, "json"))
                        requested_ext = ".json";
                    else
                        requested_ext = ".olc";
                }
                else {
                    requested_ext = NULL;
                }
            }
        }

        if (requested_ext != NULL) {
            size_t base_len = has_ext ? (size_t)(ext - groups_file) : strlen(groups_file);
            char newname[MIL];
            snprintf(newname, sizeof(newname), "%.*s%s", (int)base_len, groups_file, requested_ext);
            cfg_set_groups_file(newname);
        }

        save_skill_group_table();
        send_to_char(COLOR_INFO "Skill groups saved." COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "Syntax : gedit <group name>" COLOR_EOL, ch);
        return;
    }

    if (ch->pcdata->security < MIN_SKEDIT_SECURITY) {
        send_to_char(COLOR_INFO "You do not have enough security to edit groups." COLOR_EOL, ch);
        return;
    }

    READ_ARG(command);

    if ((group = group_lookup(command)) == -1) {
        send_to_char(COLOR_INFO "That group does not exist." COLOR_EOL, ch);
        return;
    }

    pGroup = &skill_group_table[group];

    set_editor(ch->desc, ED_GROUP, (uintptr_t)pGroup);

    gedit_show(ch, "");

    return;
}

static void display_ratings(Mobile* ch, SkillGroup* group)
{
    send_to_char(
        "\n\r"
        COLOR_TITLE   "     Class       Rating\n\r"
        COLOR_DECOR_2 "=============== ========\n\r", ch);

    for (int i = 0; i < class_count; ++i) {
        printf_to_char(ch,
            COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 " %-11.11s " 
            COLOR_DECOR_1 "] [ " COLOR_ALT_TEXT_1 " %3d " 
            COLOR_DECOR_1 "]" COLOR_EOL,
            capitalize(class_table[i].name),
            GET_ELEM(&group->rating, i));
    }

    send_to_char("\n\r", ch);
}

GEDIT(gedit_show)
{
    SkillGroup* pGrp;
    char buf[MIL];
    int i;

    EDIT_GROUP(ch, pGrp);

    olc_print_str(ch, "Name", pGrp->name);

    i = 0;
    while (i < MAX_IN_GROUP && !IS_NULLSTR(pGrp->skills[i])) {

        sprintf(buf, "%4d. " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 " %-10s " 
            COLOR_DECOR_1 "]" COLOR_EOL, i, pGrp->skills[i]);
        send_to_char(buf, ch);
        i++;
    }

    display_ratings(ch, pGrp);

    return false;
}

GEDIT(gedit_name)
{
    SkillGroup* pGrp;

    EDIT_GROUP(ch, pGrp);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: name <group name>" COLOR_EOL, ch);
        return false;
    }

    if (group_lookup(argument) != -1) {
        send_to_char(COLOR_INFO "That group already exists." COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "Syntax: rating <class> <cost>" COLOR_EOL, ch);
        return false;
    }

    if ((class_ = class_lookup(arg)) < 0) {
        send_to_char(COLOR_INFO "That class does not exist." COLOR_EOL, ch);
        return false;
    }

    rating = (SkillRating)atoi(argument);

    if (rating < -1) {
        send_to_char(COLOR_INFO "Rating must be at least 0." COLOR_EOL, ch);
        return false;
    }

    SET_ELEM(pGrp->rating, class_, rating);
    send_to_char(COLOR_INFO "Ok." COLOR_EOL, ch);
    return true;
}

GEDIT(gedit_spell)
{
    SkillGroup* pGrp;
    char arg[MSL];
    int i = 0, j = 0;

    EDIT_GROUP(ch, pGrp);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: spell new <group name>" COLOR_EOL, ch);
        send_to_char(COLOR_INFO "        spell delete <group name>" COLOR_EOL, ch);
        send_to_char(COLOR_INFO "        spell delete <number>" COLOR_EOL, ch);
        return false;
    }

    READ_ARG(arg);

    if (!str_cmp(arg, "new")) {
        for (i = 0; !IS_NULLSTR(pGrp->skills[i]) && (i < MAX_IN_GROUP); ++i)
            ;

        if (i == MAX_IN_GROUP) {
            send_to_char(COLOR_INFO "The group is full." COLOR_EOL, ch);
            return false;
        }

        if (skill_lookup(argument) == -1 && group_lookup(argument) == -1) {
            send_to_char(COLOR_INFO "Skill/spell/group does not exist." COLOR_EOL, ch);
            return false;
        }

        free_string(pGrp->skills[i]);
        pGrp->skills[i] = str_dup(argument);
        send_to_char(COLOR_INFO "Ok." COLOR_EOL, ch);
        return true;
    }

    if (!str_cmp(arg, "delete")) {
        int num = is_number(argument) ? atoi(argument) : -1;

        if (is_number(argument) && (num < 0 || num >= MAX_IN_GROUP)) {
            send_to_char(COLOR_INFO "Invalid argument." COLOR_EOL, ch);
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
                send_to_char(COLOR_INFO "Ok." COLOR_EOL, ch);
                return true;
            }
            ++i;
        }

        send_to_char(COLOR_INFO "Skill/spell/group does not exist." COLOR_EOL, ch);
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
