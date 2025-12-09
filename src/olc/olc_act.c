
/***************************************************************************
 *  File: olc_act.c                                                        *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/

#include "merc.h"


#include "olc.h"
#include "bit.h"
#include "string_edit.h"

#include <act_comm.h>
#include <act_move.h>
#include <comm.h>
#include <db.h>
#include <format.h>
#include <handler.h>
#include <interp.h>
#include <lookup.h>
#include <magic.h>
#include <mob_cmds.h>
#include <recycle.h>
#include <skills.h>
#include <special.h>
#include <stringutils.h>
#include <tables.h>

#include <entities/descriptor.h>
#include <entities/room_exit.h>
#include <entities/object.h>
#include <entities/player_data.h>

#include <data/mobile_data.h>
#include <data/race.h>
#include <data/skill.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

typedef int LookupFunc(const char*);

struct olc_help_type {
    char* command;
    const uintptr_t structure;
    char* desc;
};

bool show_version(Mobile* ch, char* argument)
{
    send_to_char(OLC_VERSION, ch);
    send_to_char("\n\r", ch);
    send_to_char(OLC_AUTHOR, ch);
    send_to_char("\n\r", ch);
    send_to_char(OLC_DATE, ch);
    send_to_char("\n\r", ch);
    send_to_char(OLC_CREDITS, ch);
    send_to_char("\n\r", ch);

    return false;
}

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

/*
 * This table contains help commands and a brief description of each.
 * ------------------------------------------------------------------
 */
const struct olc_help_type help_table[] =
{
    { "area",       U(area_flag_table),         "Area flags."               },
    { "room",       U(room_flag_table),         "Room flags."               },
    { "sector",     U(sector_flag_table),       "Sector flags."             },
    { "exit",       U(exit_flag_table),         "Exit flags."               },
    { "type",       U(type_flag_table),         "Object types."             },
    { "extra",      U(extra_flag_table),        "Object extra flags."       },
    { "wear",       U(wear_flag_table),         "Object wear flags."        },
    { "spec",       U(spec_table),              "Special procedures."       },
    { "sex",        U(sex_table),               "Sexes."                    },
    { "act",        U(act_flag_table),          "Mobile act flags."         },
    { "affect",     U(affect_flag_table),       "Mob affect flags"          },
    { "wear-loc",   U(wear_loc_flag_table),     "Wear loc flags."           },
    { "spells",     0,                          "Spell names."              },
    { "container",  U(container_flag_table),    "Container flags."          },
    { "armor",      U(ac_type),                 "Armor types."              },
    { "apply",      U(apply_flag_table),        "Apply types."              },
    { "form",       U(form_flag_table),         "Mob form flags."           },
    { "part",       U(part_flag_table),         "Mob part flags."           },
    { "imm",        U(imm_flag_table),          "Mob immunity flags."       },
    { "res",        U(res_flag_table),          "Mob resistance flags."     },
    { "vuln",       U(vuln_flag_table),         "Mob vulnerability flags."  },
    { "off",        U(off_flag_table),          "Mob offensive flags."      },
    { "size",       U(mob_size_table),          "Sizes."                    },
    { "wclass",     U(weapon_class),            "Weapon classes."           },
    { "wtype",      U(weapon_type2),            "Weapon types."             },
    { "portal",     U(portal_flag_table),       "Portal flags."             },
    { "furniture",  U(furniture_flag_table),    "Furniture flags."          },
    { "liquid",     U(liquid_table),            "Liquid types."             },
    { "damtype",    U(attack_table),            "Damtypes."                 },
    { "weapon",     U(attack_table),            NULL                        },
    { "position",   U(position_table),          "Positions."                },
    { "mprog",      U(mprog_flag_table),        "Mprog flags."              },
    { "apptype",    U(apply_types),             "Apply types (2)."          },
    { "target",     U(target_table),            "Spell target table."       },
    { "damclass",   U(dam_classes),             "Damage classes."           },
    { "log",        U(log_flag_table),          "Log flags."                },
    { "show",       U(show_flag_table),         "Command display flags."    },
    { NULL,         0,                          NULL                        }
};

/*****************************************************************************
 Name:          show_flag_cmds
 Purpose:       Displays settable flags and stats.
 Called by:     show_help(olc_act.c).
 ****************************************************************************/
void show_flag_cmds(Mobile* ch, const struct flag_type* flag_table)
{
    char buf[MAX_STRING_LENGTH] = "";
    char buf1[MAX_STRING_LENGTH] = "";
    int  flag;
    int  col;

    buf1[0] = '\0';
    col = 0;
    for (flag = 0; flag_table[flag].name != NULL; flag++) {
        if (flag_table[flag].settable) {
            sprintf(buf, "%-19.18s", flag_table[flag].name);
            strcat(buf1, buf);
            if (++col % 4 == 0)
                strcat(buf1, "\n\r");
        }
    }

    if (col % 4 != 0)
        strcat(buf1, "\n\r");

    send_to_char(buf1, ch);
    return;
}

/*****************************************************************************
 Name:          show_skill_cmds
 Purpose:       Displays all skill functions.
                Does remove those damn immortal commands from the list.
                Could be improved by:
                (1) Adding a check for a particular class.
                (2) Adding a check for a level range.
 Called by:     show_help(olc_act.c).
 ****************************************************************************/
void show_skill_cmds(Mobile* ch, SkillTarget tar)
{
    char buf[MAX_STRING_LENGTH] = "";
    char buf1[MAX_STRING_LENGTH * 2] = "";
    int  sn;
    int  col;

    buf1[0] = '\0';
    col = 0;
    for (sn = 0; sn < skill_count; sn++) {
        if (!skill_table[sn].name)
            break;

        if (!str_cmp(skill_table[sn].name, "reserved")
            || !HAS_SPELL_FUNC(sn))
            continue;

        if (tar == SKILL_TARGET_ALL || skill_table[sn].target == tar) {
            sprintf(buf, "%-19.18s", skill_table[sn].name);
            strcat(buf1, buf);
            if (++col % 4 == 0)
                strcat(buf1, "\n\r");
        }
    }

    if (col % 4 != 0)
        strcat(buf1, "\n\r");

    printf_to_char(ch, COLOR_INFO "%s" COLOR_EOL, buf1);
    return;
}

/*****************************************************************************
 Name:          show_spec_cmds
 Purpose:       Displays settable special functions.
 Called by:     show_help(olc_act.c).
 ****************************************************************************/
void show_spec_cmds(Mobile* ch)
{
    char buf[MAX_STRING_LENGTH] = "";
    char buf1[MAX_STRING_LENGTH] = "";
    int  spec;
    int  col;

    buf1[0] = '\0';
    col = 0;
    send_to_char(COLOR_INFO "Precede special functions with 'spec_'\n\r" COLOR_EOL, ch);
    for (spec = 0; spec_table[spec].function != NULL; spec++) {
        sprintf(buf, "%-19.18s", &spec_table[spec].name[5]);
        strcat(buf1, buf);
        if (++col % 4 == 0)
            strcat(buf1, "\n\r");
    }

    if (col % 4 != 0)
        strcat(buf1, "\n\r");

    send_to_char(buf1, ch);
    return;
}

/*****************************************************************************
 Name:          show_help
 Purpose:       Displays help for many tables used in OLC.
 Called by:     olc interpreters.
 ****************************************************************************/
bool show_help(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char spell[MAX_INPUT_LENGTH];
    int cnt;

    READ_ARG(arg);
    one_argument(argument, spell);

    // Display syntax.
    if (arg[0] == '\0') {
        int blah = 0;

        send_to_char(COLOR_INFO "Syntax:  ? [command]\n\r\n\r" COLOR_EOL, ch);
        sprintf(buf, "%-9.9s   %-26.26s %-9.9s   %-26.26s\n\r",
            "[command]", "[description]",
            "[command]", "[description]");
        send_to_char(buf, ch);

        for (cnt = 0; help_table[cnt].command != NULL; cnt++)
            if (help_table[cnt].desc) {
                sprintf(buf, COLOR_ALT_TEXT_1 "%-9.9s" COLOR_CLEAR " - %-26.26s",
                    capitalize(help_table[cnt].command),
                    help_table[cnt].desc);
                if (blah % 2 == 1)
                    strcat(buf, "\n\r");
                else
                    strcat(buf, " ");
                send_to_char(buf, ch);
                blah++;
            }

        if (blah % 2 == 1)
            send_to_char("\n\r", ch);

        return false;
    }

    // Find the command, show changeable data.
    for (cnt = 0; help_table[cnt].command != NULL; cnt++) {
        if (arg[0] == help_table[cnt].command[0]
            && !str_prefix(arg, help_table[cnt].command)) {
            if (help_table[cnt].structure == U(spec_table)) {
                show_spec_cmds(ch);
                return false;
            }
            else if (help_table[cnt].structure == U(liquid_table)) {
                show_liqlist(ch);
                return false;
            }
            else if (help_table[cnt].structure == U(attack_table)) {
                show_damlist(ch);
                return false;
            }
            else if (help_table[cnt].structure == U(position_table)) {
                show_poslist(ch);
                return false;
            }
            else if (help_table[cnt].structure == U(sex_table)) {
                show_sexlist(ch);
                return false;
            }
            else if (help_table[cnt].structure == U(mob_size_table)) {
                show_sizelist(ch);
                return false;
            }
            else if (!str_prefix(arg, "spells") && help_table[cnt].structure == 0) {

                if (spell[0] == '\0') {
                    send_to_char(COLOR_INFO "Syntax:  ? spells "
                        "[ignore/attack/defend/self/object/all]" COLOR_EOL, ch);
                    return false;
                }

                if (!str_prefix(spell, "all"))
                    show_skill_cmds(ch, SKILL_TARGET_ALL);
                else if (!str_prefix(spell, "ignore"))
                    show_skill_cmds(ch, SKILL_TARGET_IGNORE);
                else if (!str_prefix(spell, "attack"))
                    show_skill_cmds(ch, SKILL_TARGET_CHAR_OFFENSIVE);
                else if (!str_prefix(spell, "defend"))
                    show_skill_cmds(ch, SKILL_TARGET_CHAR_DEFENSIVE);
                else if (!str_prefix(spell, "self"))
                    show_skill_cmds(ch, SKILL_TARGET_CHAR_SELF);
                else if (!str_prefix(spell, "object"))
                    show_skill_cmds(ch, SKILL_TARGET_OBJ_INV);
                else
                    send_to_char(COLOR_INFO "Syntax:  ? spell "
                        "[ignore/attack/defend/self/object/all]" COLOR_EOL, ch);

                return false;
            }
            else {
                show_flag_cmds(ch, (const struct flag_type*)help_table[cnt].structure);
                return false;
            }
        }
    }

    show_help(ch, "");
    return false;
}

void show_liqlist(Mobile* ch)
{
    int liq;
    Buffer* buffer;
    char buf[MAX_STRING_LENGTH];

    buffer = new_buf();

    for (liq = 0; liq < LIQ_COUNT; liq++) {
        if ((liq % 21) == 0)
            add_buf(buffer, COLOR_TITLE "Name                 Color          Proof Full Thirst Food Ssize" COLOR_ALT_TEXT_1);

        sprintf(buf, "%-20s %-14s %5d %4d %6d %4d %5d\n\r",
            liquid_table[liq].name, liquid_table[liq].color,
            liquid_table[liq].proof, liquid_table[liq].full,
            liquid_table[liq].thirst, liquid_table[liq].food,
            liquid_table[liq].sip_size);
        add_buf(buffer, buf);
    }

    add_buf(buffer, COLOR_EOL);

    page_to_char(BUF(buffer), ch);
    free_buf(buffer);

    return;
}

void show_damlist(Mobile* ch)
{
    int att;
    Buffer* buffer;
    char buf[MAX_STRING_LENGTH];

    buffer = new_buf();

    for (att = 0; att < ATTACK_COUNT; att++) {
        if ((att % 21) == 0)
            add_buf(buffer, COLOR_TITLE "Name                 Noun" COLOR_EOL);

        sprintf(buf, COLOR_ALT_TEXT_1 "%-20s %-20s" COLOR_EOL,
            attack_table[att].name, attack_table[att].noun);
        add_buf(buffer, buf);
    }

    page_to_char(BUF(buffer), ch);
    free_buf(buffer);

    return;
}

void show_poslist(Mobile* ch)
{
    int pos;
    Buffer* buffer;
    char buf[MAX_STRING_LENGTH];

    buffer = new_buf();

    for (pos = 0; pos < POS_MAX; pos++) {
        if ((pos % 21) == 0)
            add_buf(buffer, COLOR_TITLE "Name                 Short Name" COLOR_EOL);

        sprintf(buf, COLOR_ALT_TEXT_1 "%-20s %-20s" COLOR_EOL,
            position_table[pos].name, position_table[pos].short_name);
        add_buf(buffer, buf);
    }

    page_to_char(BUF(buffer), ch);
    free_buf(buffer);

    return;
}

void show_sexlist(Mobile* ch)
{
    int sex;
    Buffer* buffer;
    char buf[MAX_STRING_LENGTH];

    buffer = new_buf();

    for (sex = 0; sex < SEX_COUNT; sex++) {
        sprintf(buf, COLOR_ALT_TEXT_1 "%-20s " COLOR_EOL,
            sex_table[sex].name);
        add_buf(buffer, buf);
    }

    add_buf(buffer, "\n\r");
    page_to_char(BUF(buffer), ch);
    free_buf(buffer);

    return;
}

void show_sizelist(Mobile* ch)
{
    int size;
    Buffer* buffer;
    char buf[MAX_STRING_LENGTH];

    buffer = new_buf();

    for (size = 0; size < MOB_SIZE_COUNT; size++) {
        if ((size % 3) == 0)
            add_buf(buffer, "\n\r");

        sprintf(buf, COLOR_ALT_TEXT_1 "%-20s " COLOR_EOL, mob_size_table[size].name);
        add_buf(buffer, buf);
    }

    add_buf(buffer, "\n\r");
    page_to_char(BUF(buffer), ch);
    free_buf(buffer);

    return;
}

ED_FUN_DEC(ed_line_string)
{
    char** string = (char**)arg;
    char buf[MIL];

    if (IS_NULLSTR(argument)) {
        sprintf(buf, "Syntax : %s <string>\n\r", n_fun);
        send_to_char(buf, ch);
        return false;
    }

    if (!str_cmp(argument, "null")) {
        free_string(*string);
        *string = str_dup("");
    }
    else {
        sprintf(buf, "%s%s", argument, par == 0 ? "" : "\n\r");

        free_string(*string);
        *string = str_dup(buf);
    }

    send_to_char("Ok.\n\r", ch);

    return true;
}

ED_FUN_DEC(ed_line_lox_string)
{
    String** string = (String**)arg;
    char buf[MIL];

    if (IS_NULLSTR(argument)) {
        sprintf(buf, COLOR_INFO "Syntax : %s <string>" COLOR_EOL, n_fun);
        send_to_char(buf, ch);
        return false;
    }

    if (!str_cmp(argument, "null")) {
        *string = copy_string(str_empty, 1);
    }
    else {
        sprintf(buf, "%s%s", argument, par == 0 ? "" : "\n\r");
        *string = copy_string(buf, (int)strlen(buf));
    }

    send_to_char(COLOR_INFO "Ok." COLOR_EOL, ch);

    return true;
}

#define NUM_INT16 0
#define NUM_INT32 1
#define NUM_LONG 2

bool numedit(char* n_fun, Mobile* ch, char* argument, uintptr_t arg, int16_t type, long min, long max)
{
    int temp;
    int* value = (int*)arg;
    int16_t* shvalue = (int16_t*)arg;
    long* lvalue = (long*)arg;

    if (IS_NULLSTR(argument)) {
        printf_to_char(ch, COLOR_INFO "Syntax : %s <number>" COLOR_EOL, n_fun);
        return false;
    }

    if (!is_number(argument)) {
        send_to_char(COLOR_INFO "Argument must be a number." COLOR_EOL, ch);
        return false;
    }

    temp = atoi(argument);

    if (min != -1 && temp < min) {
        printf_to_char(ch, COLOR_INFO "Number must be greater than %d." COLOR_EOL, min);
        return false;
    }

    if (max != -1 && temp > max) {
        printf_to_char(ch, COLOR_INFO "Number must be less than %d." COLOR_EOL, max);
        return false;
    }

    if (type == NUM_INT16)
        *shvalue = (int16_t)temp;
    else if (type == NUM_INT32)
        *value = (int32_t)temp;
    else
        *lvalue = temp;

    send_to_char(COLOR_INFO "Ok." COLOR_EOL, ch);
    return true;
}

ED_FUN_DEC(ed_number_align)
{
    return numedit(n_fun, ch, argument, arg, NUM_INT16, -1000, 1000);
}

ED_FUN_DEC(ed_number_pos)
{
    return numedit(n_fun, ch, argument, arg, NUM_INT32, 0, -1);
}

ED_FUN_DEC(ed_number_s_pos)
{
    return numedit(n_fun, ch, argument, arg, NUM_INT16, 0, -1);
}

ED_FUN_DEC(ed_number_l_pos)
{
    return numedit(n_fun, ch, argument, arg, NUM_LONG, 0, -1);
}

ED_FUN_DEC(ed_number_level)
{
    return numedit(n_fun, ch, argument, arg, NUM_INT16, 0, MAX_LEVEL);
}

ED_FUN_DEC(ed_desc)
{
    if (emptystring(argument)) {
        if (IS_SET(ch->comm_flags, COMM_OLCX))
            do_clear(ch, "reset");

        string_append(ch, (char**)arg);
        return true;
    }

    send_to_char(COLOR_INFO "Syntax : desc" COLOR_EOL, ch);

    return false;
}

ED_FUN_DEC(ed_bool)
{
    if (emptystring(argument)) {
        printf_to_char(ch, COLOR_INFO "Syntax : %s <true/false>" COLOR_EOL, n_fun);
        return false;
    }

    if (!str_cmp(argument, "true") || !str_cmp(argument, "yes"))
        *(bool*)arg = true;
    else
        if (!str_cmp(argument, "false") || !str_cmp(argument, "no"))
            *(bool*)arg = false;
        else {
            send_to_char(COLOR_INFO "Argument must be true or false." COLOR_EOL, ch);
            return false;
        }

    send_to_char(COLOR_INFO "Ok." COLOR_EOL, ch);
    return true;
}

ED_FUN_DEC(ed_skillgroup)
{
    SKNUM gn;

    if (emptystring(argument)) {
        printf_to_char(ch, COLOR_INFO "Syntax : %s <skill group>" COLOR_EOL, n_fun);
        return false;
    }

    if ((gn = group_lookup(argument)) < 0) {
        printf_to_char(ch, COLOR_INFO "Could not find group '" COLOR_ALT_TEXT_1 "%s" COLOR_INFO "'" COLOR_EOL, argument);
        return false;
    }

    char** group_name = (char**)arg;
    free_string(*group_name);
    *group_name = str_dup(skill_group_table[gn].name);
    return true;
}

static const struct flag_type* get_flag_defaults_for_command(const char* command)
{
    if (!command || command[0] == '\0')
        return NULL;
    if (!str_cmp(command, "form") || !str_cmp(command, "forms"))
        return form_defaults_flag_table;
    if (!str_cmp(command, "parts") || !str_cmp(command, "part"))
        return part_defaults_flag_table;
    return NULL;
}

ED_FUN_DEC(ed_flag_toggle)
{
    FLAGS value;
    const struct flag_type* defaults = get_flag_defaults_for_command(n_fun);

    if (!emptystring(argument)) {
        char flag_str[MIL];
        READ_ARG(flag_str);
        bool found = false;
        while (flag_str[0]) {
            if ((value = flag_value((struct flag_type*)par, flag_str)) != NO_FLAG) {
                *(FLAGS*)arg ^= value;
                printf_to_char(ch, COLOR_INFO "%c%s flag toggled." COLOR_EOL, toupper(n_fun[0]), &n_fun[1]);
                found = true;
            }
            else if (defaults && (value = flag_value((struct flag_type*)defaults, flag_str)) != NO_FLAG) {
                *(FLAGS*)arg ^= value;
                printf_to_char(ch, COLOR_INFO "%s preset toggled." COLOR_EOL, capitalize(flag_str));
                found = true;
            }
            else
                printf_to_char(ch, COLOR_INFO "Unknown flag '%s'." COLOR_EOL, flag_str);
            READ_ARG(flag_str);
        }
        return found;
    }

    INIT_BUF(set_out, MSL);
    INIT_BUF(unset_out, MSL);
    Buffer* preset_out = NULL;
    if (defaults)
        preset_out = new_buf();

    printf_to_char(ch, COLOR_INFO "Syntax : %s <flags>" COLOR_EOL, n_fun);
    
    FLAGS current = *(FLAGS*)arg;

    for (struct flag_type* flag = (struct flag_type*)par; flag->name != NULL; ++flag) {
        if (IS_SET(current, flag->bit))
            addf_buf(set_out, "    " COLOR_ALT_TEXT_1 "%-20s" COLOR_EOL, flag->name);
        else if (flag->settable)
            addf_buf(unset_out, "    " COLOR_ALT_TEXT_1 "%-20s" COLOR_EOL, flag->name);
    }

    if (set_out->size > 0) {
        printf_to_char(ch, COLOR_INFO "\n\rCurrent Flags:\n\r " COLOR_ALT_TEXT_1 "%s", set_out->string);
    }

    if (unset_out->size > 0) {
        printf_to_char(ch, COLOR_INFO "\n\rAvailable Flags:\n\r " COLOR_ALT_TEXT_1 "%s", unset_out->string);
    }

    if (preset_out) {
        for (const struct flag_type* def = defaults; def && def->name != NULL; ++def) {
            addf_buf(preset_out, "    " COLOR_ALT_TEXT_1 "%-20s" COLOR_ALT_TEXT_2 " (%s)",
                def->name, flag_string((const struct flag_type*)par, def->bit));
        }
        if (preset_out->size > 0)
            printf_to_char(ch, COLOR_INFO "\n\rPresets:\n\r" COLOR_ALT_TEXT_1 "%s", preset_out->string);
    }

    send_to_char(COLOR_EOL, ch);

    free_buf(set_out);
    free_buf(unset_out);
    if (preset_out)
        free_buf(preset_out);
    return false;
}

ED_FUN_DEC(ed_flag_set_long)
{
    int value;

    if (!emptystring(argument)) {
        if ((value = flag_value((struct flag_type*)par, argument)) != NO_FLAG) {
            *(long*)arg = value;

            printf_to_char(ch, COLOR_INFO "%c%s flag set." COLOR_EOL,
                toupper(n_fun[0]),
                &n_fun[1]);
            return true;
        }
    }

    printf_to_char(ch, COLOR_INFO "Syntax : %s <flags>" COLOR_EOL, n_fun);
    show_flags_to_char(ch, (struct flag_type*)par);

    return false;
}

ED_FUN_DEC(ed_flag_set_sh)
{
    int value;

    if (!emptystring(argument)) {
        if ((value = flag_value((struct flag_type*)par, argument)) != NO_FLAG) {
            *(int16_t*)arg = (int16_t)value;

            printf_to_char(ch, COLOR_INFO "%c%s flag set." COLOR_EOL,
                toupper(n_fun[0]),
                &n_fun[1]);
            return true;
        }
    }

    printf_to_char(ch, COLOR_INFO "Syntax : %s <flags>" COLOR_EOL, n_fun);
    show_flags_to_char(ch, (struct flag_type*)par);

    return false;
}

ED_FUN_DEC(ed_commands)
{
    show_commands(ch, "");
    return false;
}

ED_FUN_DEC(ed_gamespec)
{
    SpecFunc** spec = (SpecFunc**)arg;

    if (argument[0] == '\0') {
        printf_to_char(ch, COLOR_INFO "Syntax : %s [%s]" COLOR_EOL,
            n_fun,
            n_fun);
        return false;
    }

    if (!str_cmp(argument, "none")) {
        *spec = NULL;
        printf_to_char(ch, COLOR_INFO "%s removed." COLOR_EOL, n_fun);
        return true;
    }

    if (spec_lookup(argument)) {
        *spec = spec_lookup(argument);
        send_to_char(COLOR_INFO "Spec set." COLOR_EOL, ch);
        return true;
    }

    send_to_char(COLOR_INFO "Spec does not exist." COLOR_EOL, ch);
    return false;
}

bool templookup(char* n_fun, Mobile* ch, char* argument, uintptr_t arg, const uintptr_t par, int temp)
{
    int value;
    LookupFunc* blah = (LookupFunc*)par;

    if (!emptystring(argument)) {
        if ((value = ((*blah) (argument))) > temp) {
            *(int16_t*)arg = (int16_t)value;
            printf_to_char(ch, COLOR_INFO "%s set." COLOR_EOL,
                n_fun);
            return true;
        }
        else {
            printf_to_char(ch, COLOR_INFO "%s does not exist." COLOR_EOL,
                n_fun);
            return false;
        }
    }

    printf_to_char(ch, COLOR_INFO "Syntax : %s [%s]\n\r"
        "Type '? %s' to list available options." COLOR_EOL,
        n_fun,
        n_fun,
        n_fun);

    return false;
}

ED_FUN_DEC(ed_int16poslookup)
{
    return templookup(n_fun, ch, argument, arg, par, 0);
}

ED_FUN_DEC(ed_int16lookup)
{
    return templookup(n_fun, ch, argument, arg, par, -1);
}

ED_FUN_DEC(ed_dice)
{
    static char syntax[] = COLOR_INFO "Arguments:  " COLOR_ALT_TEXT_1 "<number> d <type> + <bonus>\n\r" 
                           "            (or just 0 for 0d0+0)" COLOR_EOL;
    char* numb_str = NULL; 
    char* type_str = NULL; 
    char* bonus_str = NULL;
    int16_t numb;
    int16_t type;
    int16_t bonus;
    char* cp;
    int16_t* array = (int16_t*)arg;

    if (emptystring(argument)) {
        send_to_char(syntax, ch);
        return false;
    }

    numb_str = cp = argument;

    while (ISDIGIT(*cp))
        ++cp;

    // Handle plain 0 case
    if (*cp == '\0' && cp != numb_str && atoi(numb_str) == 0) {
        array[DICE_NUMBER] = 0;
        array[DICE_TYPE] = 0;
        array[DICE_BONUS] = 0;
        printf_to_char(ch, COLOR_INFO "%s set." COLOR_EOL, n_fun);
        return true;
    }

    while (*cp != '\0' && !ISDIGIT(*cp))
        *(cp++) = '\0';

    type_str = cp;

    while (ISDIGIT(*cp))
        ++cp;
    while (*cp != '\0' && !ISDIGIT(*cp))
        *(cp++) = '\0';

    bonus_str = cp;

    while (ISDIGIT(*cp)) 
        ++cp;
    *cp = '\0';

    if ((!is_number(numb_str) || (numb = (int16_t)atoi(numb_str)) < 1)
        || (!is_number(type_str) || (type = (int16_t)atoi(type_str)) < 1)
        || (!is_number(bonus_str) || (bonus = (int16_t)atoi(bonus_str)) < 0)) {
        send_to_char(syntax, ch);
        return false;
    }

    array[DICE_NUMBER] = numb;
    array[DICE_TYPE] = type;
    array[DICE_BONUS] = bonus;

    printf_to_char(ch, COLOR_INFO "%s set." COLOR_EOL, n_fun);

    return true;
}

ED_FUN_DEC(ed_ed)
{
    ExtraDesc* ed;
    ExtraDesc** pEd = (ExtraDesc**)arg;
    char command[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];

    READ_ARG(command);
    READ_ARG(keyword);

    if (command[0] == '\0') {
        send_to_char(COLOR_INFO "Syntax:  " COLOR_ALT_TEXT_1 "ed add <keyword>\n\r", ch);
        send_to_char("         ed delete <keyword>\n\r", ch);
        send_to_char("         ed edit <keyword>\n\r", ch);
        send_to_char("         ed format <keyword>\n\r", ch);
        send_to_char("         ed rename <keyword>" COLOR_EOL, ch);
        return false;
    }

    if (!str_cmp(command, "add")) {
        if (keyword[0] == '\0') {
            send_to_char(COLOR_INFO "Syntax:  " COLOR_ALT_TEXT_1 "ed add <keyword>" COLOR_EOL, ch);
            return false;
        }

        ed = new_extra_desc();
        ed->keyword = str_dup(keyword);
        ed->next = *pEd;
        *pEd = ed;

        string_append(ch, &ed->description);

        return true;
    }

    if (!str_cmp(command, "edit")) {
        if (keyword[0] == '\0') {
            send_to_char(COLOR_INFO "Syntax:  " COLOR_ALT_TEXT_1 "ed edit <keyword>" COLOR_EOL, ch);
            return false;
        }

        FOR_EACH(ed, *pEd) {
            if (is_name(keyword, ed->keyword))
                break;
        }

        if (!ed) {
            send_to_char(COLOR_INFO "ERROR : There is no extra desc with that name." COLOR_EOL, ch);
            return false;
        }

        string_append(ch, &ed->description);

        return true;
    }

    if (!str_cmp(command, "delete")) {
        ExtraDesc* ped = NULL;

        if (keyword[0] == '\0') {
            send_to_char(COLOR_INFO "Syntax:  " COLOR_ALT_TEXT_1 "ed delete <keyword>" COLOR_EOL, ch);
            return false;
        }

        FOR_EACH(ed, *pEd) {
            if (is_name(keyword, ed->keyword))
                break;
            ped = ed;
        }

        if (!ed) {
            send_to_char(COLOR_INFO "There is no extra description with that name." COLOR_EOL, ch);
            return false;
        }

        if (!ped)
            *pEd = ed->next;
        else
            ped->next = ed->next;

        free_extra_desc(ed);

        send_to_char(COLOR_INFO "Extra description deleted." COLOR_EOL, ch);
        return true;
    }

    if (!str_cmp(command, "format")) {

        if (keyword[0] == '\0') {
            send_to_char(COLOR_INFO "Syntax:  " COLOR_ALT_TEXT_1 "ed format <keyword>" COLOR_EOL, ch);
            return false;
        }

        FOR_EACH(ed, *pEd) {
            if (is_name(keyword, ed->keyword))
                break;
        }

        if (!ed) {
            send_to_char(COLOR_INFO "There is no extra description with that name." COLOR_EOL, ch);
            return false;
        }

        char* desc = format_string(ed->description);
        free_string(ed->description);
        ed->description = desc;

        send_to_char(COLOR_INFO "Extra description formatted." COLOR_EOL, ch);
        return true;
    }

    if (!str_cmp(command, "rename")) {

        if (keyword[0] == '\0') {
            send_to_char(COLOR_INFO "Syntax:  " COLOR_ALT_TEXT_1 "ed rename <old name> <new>" COLOR_EOL, ch);
            return false;
        }

        FOR_EACH(ed, *pEd) {
            if (is_name(keyword, ed->keyword))
                break;
        }

        if (!ed) {
            send_to_char(COLOR_INFO "There is no extra description with that name." COLOR_EOL, ch);
            return false;
        }

        free_string(ed->keyword);
        ed->keyword = str_dup(argument);

        send_to_char(COLOR_INFO "Extra desc renamed." COLOR_EOL, ch);
        return true;
    }

    return ed_ed(n_fun, ch, "", arg, par);
}

ED_FUN_DEC(ed_addaffect)
{
    int value;
    Affect* pAf;
    ObjPrototype* pObj = (ObjPrototype*)arg;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];

    READ_ARG(loc);
    one_argument(argument, mod);

    if (loc[0] == '\0' || mod[0] == '\0' || !is_number(mod)) {
        send_to_char(COLOR_INFO "Syntax:  " COLOR_ALT_TEXT_1 "addaffect <location> <#xmod>" COLOR_EOL, ch);
        return false;
    }

    if ((value = flag_value(apply_flag_table, loc)) == NO_FLAG) {
        send_to_char(COLOR_INFO "Valid affects are:\n\r", ch);
        show_help(ch, "apply");
        return false;
    }

    pAf = new_affect();
    pAf->location = (AffectLocation)value;
    pAf->modifier = (int16_t)atoi(mod);
    pAf->where = TO_OBJECT;
    pAf->type = -1;
    pAf->duration = -1;
    pAf->bitvector = 0;
    pAf->level = pObj->level;
    pAf->next = pObj->affected;
    pObj->affected = pAf;

    send_to_char(COLOR_INFO "Affect added." COLOR_EOL, ch);
    return true;
}

ED_FUN_DEC(ed_delaffect)
{
    Affect* pAf;
    Affect* pAf_next;
    Affect** pNaf = (Affect**)arg;
    INIT_BUF(aff_name, MAX_STRING_LENGTH);
    int  value;
    int  cnt = 0;

    one_argument(argument, BUF(aff_name));

    if (!is_number(BUF(aff_name)) || BUF(aff_name)[0] == '\0') {
        send_to_char(COLOR_INFO "Syntax:  " COLOR_ALT_TEXT_1 "delaffect <#xaffect>" COLOR_EOL, ch);
        return false;
    }

    value = atoi(BUF(aff_name));

    if (value < 0) {
        send_to_char(COLOR_INFO "Only non-negative affect-numbers allowed." COLOR_EOL, ch);
        return false;
    }

    if (!(pAf = *pNaf)) {
        send_to_char(COLOR_INFO "Non-existant affect." COLOR_EOL, ch);
        return false;
    }

    if (value == 0) {
        /* First case: Remove first affect */
        pAf = *pNaf;
        *pNaf = pAf->next;
        free_affect(pAf);
    }
    else {
        /* Affect to remove is not the first */
        while ((pAf_next = pAf->next) && (++cnt < value))
            pAf = pAf_next;

        if (pAf_next)        /* See if it's the next affect */
        {
            pAf->next = pAf_next->next;
            free_affect(pAf_next);
        }
        else                                 /* Doesn't exist */
        {
            send_to_char(COLOR_INFO "No such affect." COLOR_EOL, ch);
            return false;
        }
    }

    send_to_char(COLOR_INFO "Affect removed." COLOR_EOL, ch);

    free_buf(aff_name);

    return true;
}

ED_FUN_DEC(ed_olded)
{
    return (*(OlcFunc*)par) (ch, argument);
}

ED_FUN_DEC(ed_docomm)
{
    (*(DoFunc*)par) (ch, argument);
    return false;
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
