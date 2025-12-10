////////////////////////////////////////////////////////////////////////////////
// raedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "bit.h"
#include "comm.h"
#include "config.h"
#include "db.h"
#include "handler.h"
#include "lookup.h"
#include "magic.h"
#include "olc.h"
#include "recycle.h"
#include "save.h"
#include "tables.h"

#include "entities/descriptor.h"
#include "entities/player_data.h"

#include "data/mobile_data.h"
#include "data/race.h"

#ifdef _MSC_VER
#include <io.h>
#define access _access
#else
#include <unistd.h>
#endif

#ifndef F_OK
#define F_OK 0
#endif

#define RAEDIT( fun )		bool fun( Mobile *ch, char *argument )

Race xRace;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const OlcCmdEntry race_olc_comm_table[] = {
    { "show",       0,                  ed_olded,           U(raedit_show)  },
    { "name",       U(&xRace.name),     ed_line_string,     0               },
    { "pcrace",     U(&xRace.pc_race),  ed_bool,            0               },
    { "act",        U(&xRace.act_flags),ed_flag_toggle,     U(act_flag_table)   },
    { "aff",        U(&xRace.aff),      ed_flag_toggle,     U(affect_flag_table)},
    { "off",        U(&xRace.off),      ed_flag_toggle,     U(off_flag_table)   },
    { "res",        U(&xRace.res),      ed_flag_toggle,     U(res_flag_table)   },
    { "vuln",       U(&xRace.vuln),     ed_flag_toggle,     U(vuln_flag_table)  },
    { "imm",        U(&xRace.imm),      ed_flag_toggle,     U(imm_flag_table)   },
    { "form",       U(&xRace.form),     ed_flag_toggle,     U(form_flag_table)  },
    { "part",       U(&xRace.parts),    ed_flag_toggle,     U(part_flag_table)  },
    { "who",        U(&xRace.who_name), ed_line_string,     0               },
    { "points",     U(&xRace.points),   ed_number_s_pos,    0               },
    { "start_loc",  0,                  ed_olded,           U(raedit_start_loc) },
    { "cmult",      0,                  ed_olded,           U(raedit_cmult) },
    { "stats",      0,                  ed_olded,           U(raedit_stats) },
    { "maxstats",   0,                  ed_olded,           U(raedit_maxstats)},
    { "skills",     0,                  ed_olded,           U(raedit_skills)},
    { "size",       U(&xRace.size),     ed_int16poslookup,  U(size_lookup)  },
    { "new",        0,                  ed_olded,           U(raedit_new)   },
    { "list",       0,                  ed_olded,           U(raedit_list)  },
    { "commands",   0,                  ed_olded,           U(show_commands)},
    { "?",          0,                  ed_olded,           U(show_help)    },
    { "version",    0,                  ed_olded,           U(show_version) },
    { NULL,         0,                  NULL,               0               }
};

void raedit(Mobile* ch, char* argument)
{
    if (ch->pcdata->security < MIN_RAEDIT_SECURITY) {
        send_to_char(COLOR_INFO "You do not have enough security to edit races." COLOR_CLEAR, ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (!str_prefix("save", argument)) {
        char arg1[MIL];
        char arg2[MIL];
        argument = one_argument(argument, arg1); // "save"
        argument = one_argument(argument, arg2); // optional format
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
        const char* races_file = cfg_get_races_file();
        const char* ext = strrchr(races_file, '.');
        bool has_ext = (ext != NULL);

        if (!force_format) {
            if (has_ext) {
                requested_ext = NULL; // respect existing extension
            } else {
                // Only apply default format when this looks like a new file
                if (access(races_file, F_OK) != 0) {
                    const char* def = cfg_get_default_format();
                    if (def && !str_cmp(def, "json"))
                        requested_ext = ".json";
                    else
                        requested_ext = ".olc";
                } else {
                    requested_ext = NULL; // existing no-extension file stays ROM-OLC
                }
            }
        }
        // When format is forced, always apply the requested extension

        if (requested_ext != NULL) {
            size_t base_len = has_ext ? (size_t)(ext - races_file) : strlen(races_file);
            char newname[MIL];
            snprintf(newname, sizeof(newname), "%.*s%s", (int)base_len, races_file, requested_ext);
            cfg_set_races_file(newname);
        }
        save_race_table();
        send_to_char(COLOR_INFO "Races saved." COLOR_CLEAR, ch);
        return;
    }

    if (emptystring(argument)) {
        raedit_show(ch, argument);
        return;
    }

    /* Search Table and Dispatch Command. */
    if (!process_olc_command(ch, argument, race_olc_comm_table))
        interpret(ch, argument);

    return;
}

void do_raedit(Mobile* ch, char* argument)
{
    const Race* pRace;
    int race;
    char arg[MSL];

    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    if (ch->pcdata->security < MIN_RAEDIT_SECURITY) {
        send_to_char(COLOR_INFO "You do not have enough security to edit races." COLOR_CLEAR, ch);
        return;
    }

    one_argument(argument, arg);

    if (!str_cmp(arg, "new")) {
        READ_ARG(arg);
        if (raedit_new(ch, argument))
            save_race_table();
        return;
    }

    if (IS_NULLSTR(argument) || (race = race_lookup(argument)) == 0) {
        send_to_char(COLOR_INFO "Syntax: raedit <race name>" COLOR_CLEAR, ch);
        return;
    }

    pRace = &race_table[race];

    ch->desc->pEdit = U(pRace);
    ch->desc->editor = ED_RACE;

    raedit_show(ch, "");

    return;
}

RAEDIT(raedit_show)
{
    Race* pRace;
    char buf[MSL];
    static const char* Stats[] = { "Str", "Int", "Wis", "Dex", "Con" };
    int i = 0;

    EDIT_RACE(ch, pRace);

    RoomData* room = NULL;
    if (pRace->start_loc > 0)
        room = get_room_data(pRace->start_loc);

    olc_print_str_box(ch, "Race Name", pRace->name, NULL);
    olc_print_yesno(ch, "PC Race?", pRace->pc_race);
    if (pRace->pc_race)
        olc_print_str(ch, "Who Name", pRace->who_name);
    olc_print_flags(ch, "Act Flags",act_flag_table, pRace->act_flags);
    olc_print_flags(ch, "Aff Flags", affect_flag_table, pRace->aff);
    olc_print_flags(ch, "Off Flags", off_flag_table, pRace->off);
    olc_print_flags(ch, "Imm Flags", imm_flag_table, pRace->imm);
    olc_print_flags(ch, "Res Flags", res_flag_table, pRace->res);
    olc_print_flags(ch, "Vuln Flags", vuln_flag_table, pRace->vuln);
    olc_print_flags_ex(ch, "Form Flags", form_flag_table, form_defaults_flag_table, pRace->form);
    olc_print_flags_ex(ch, "Part Flags", part_flag_table, part_defaults_flag_table, pRace->parts);
    olc_print_num(ch, "Points", pRace->points);
    olc_print_str_box(ch, "Size", mob_size_table[pRace->size].name, NULL);
    if (cfg_get_start_loc_by_race()) {
        olc_print_num_str(ch, "Start Loc", pRace->start_loc, room ? NAME_STR(room) : "");
        printf_to_char(ch, COLOR_TITLE "    Class      XPmult  XP/lvl(pts)   Start Loc" COLOR_EOL);
        for (i = 0; i < class_count; ++i) {
            VNUM vnum = GET_ELEM(&pRace->class_start, i);

            if (vnum > 0)
                room = get_room_data(vnum);
            else
                room = NULL;
            sprintf(buf, "    %-7.7s     " COLOR_ALT_TEXT_1 "%3d     %4d" COLOR_DECOR_1 "(" COLOR_ALT_TEXT_1 "%3d" COLOR_DECOR_1 ")" COLOR_CLEAR "    " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "] " COLOR_ALT_TEXT_2 "%s %s" COLOR_EOL,
                capitalize(class_table[i].name),
                GET_ELEM(&pRace->class_mult, i),
                race_exp_per_level(pRace->race_id, i, get_points(pRace->race_id, i)),
                get_points(pRace->race_id, i),
                vnum, 
                room ? NAME_STR(room) : "",
                cfg_get_start_loc_by_class() && cfg_get_start_loc_by_race() ? "" : " " COLOR_ALT_TEXT_2 "(not used)"
            );
            send_to_char(buf, ch);
        }
    } else {
        olc_print_num_str(ch, "Start Loc", 0, "(not used)");
    }

    for (i = 0; i < STAT_COUNT; ++i) {
        sprintf(buf, "%s: " COLOR_ALT_TEXT_1 "%2d" COLOR_DECOR_1 "(" COLOR_ALT_TEXT_1 "%2d" COLOR_DECOR_1 ")" COLOR_CLEAR " ", Stats[i], pRace->stats[i], pRace->max_stats[i]);
        send_to_char(buf, ch);
    }
    send_to_char("\n\r", ch);

    for (i = 0; i < RACE_NUM_SKILLS; ++i)
        if (!IS_NULLSTR(pRace->skills[i])) {
            printf_to_char(ch, "%2d. " COLOR_ALT_TEXT_1 "%s" COLOR_EOL, i, pRace->skills[i]);
        }

    return false;
}

RAEDIT(raedit_list)
{
    int i;
    char buf[MSL];
    Buffer* fBuf;

    fBuf = new_buf();

    for (i = 0; race_table[i].name; i++) {
        sprintf(buf, "%2d %c " COLOR_ALT_TEXT_1 "%-33.33s" COLOR_CLEAR , i,
            race_table[i].pc_race ? '+' : '-',
            race_table[i].name);
        if (i % 2 == 1)
            strcat(buf, "\n\r");
        else
            strcat(buf, " ");
        add_buf(fBuf, buf);
    }

    if (i % 2 == 1)
        add_buf(fBuf, "\n\r");

    page_to_char(BUF(fBuf), ch);
    free_buf(fBuf);

    return false;
}

RAEDIT(raedit_new)
{
    Descriptor* d;
    Mobile* tch;
    Race* new_race_table;
    size_t maxRace;

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: new <race name>" COLOR_CLEAR, ch);
        return false;
    }

    if (race_lookup(argument) != 0) {
        send_to_char(COLOR_INFO "That race already exists!" COLOR_CLEAR, ch);
        return false;
    }

    FOR_EACH(d, descriptor_list) {
        if (d->connected != CON_PLAYING || (tch = CH(d)) == NULL || tch->desc == NULL)
            continue;

        if (tch->desc->editor == ED_RACE)
            edit_done(ch);
    }

    /* reallocate the table */

    for (maxRace = 0; race_table[maxRace].name; maxRace++)
        ;

    maxRace++;
    new_race_table = realloc(race_table, sizeof(Race) * (maxRace + 1));

    if (!new_race_table) /* realloc failed */
    {
        send_to_char(COLOR_INFO "Realloc failed. Prepare for impact." COLOR_CLEAR, ch);
        return false;
    }

    race_table = new_race_table;

    race_table[maxRace - 1].name = str_dup("unique");
    race_table[maxRace - 1].pc_race = false;
    race_table[maxRace - 1].act_flags = 0;
    race_table[maxRace - 1].aff = 0;
    race_table[maxRace - 1].off = 0;
    race_table[maxRace - 1].imm = 0;
    race_table[maxRace - 1].res = 0;
    race_table[maxRace - 1].vuln = 0;
    race_table[maxRace - 1].form = 0;
    race_table[maxRace - 1].parts = 0;

    free_string(race_table[maxRace - 2].name);

    race_table[maxRace - 2].name = str_dup(argument);
    race_table[maxRace - 2].pc_race = false;
    race_table[maxRace - 2].act_flags = 0;
    race_table[maxRace - 2].aff = 0;
    race_table[maxRace - 2].off = 0;
    race_table[maxRace - 2].imm = 0;
    race_table[maxRace - 2].res = 0;
    race_table[maxRace - 2].vuln = 0;
    race_table[maxRace - 2].form = 0;
    race_table[maxRace - 2].parts = 0;

    race_table[maxRace].name = NULL;

    ch->desc->editor = ED_RACE;
    ch->desc->pEdit = U(&race_table[maxRace - 2]);

    send_to_char(COLOR_INFO "New race created." COLOR_CLEAR, ch);
    return true;
}

RAEDIT(raedit_cmult)
{
    Race* race;
    ClassMult mult;
    int class_;
    char class_name[MIL];

    EDIT_RACE(ch, race);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: cmult <class name> <multiplier>" COLOR_CLEAR, ch);
        return false;
    }

    READ_ARG(class_name);

    if ((class_ = class_lookup(class_name)) == -1) {
        send_to_char(COLOR_INFO "That class does not exist." COLOR_CLEAR, ch);
        return false;
    }

    if (!is_number(argument)) {
        send_to_char(COLOR_INFO "The multiplier must be a number." COLOR_CLEAR, ch);
        return false;
    }

    mult = (ClassMult)atoi(argument);

    if (mult < 1 || mult > 200) {
        send_to_char(COLOR_INFO "The multiplier must be between 1 and 200." COLOR_CLEAR, ch);
        return false;
    }

    SET_ELEM(race->class_mult, class_, mult);
    send_to_char(COLOR_INFO "Ok." COLOR_CLEAR, ch);
    return true;
}

RAEDIT(raedit_stats)
{
    Race* race;
    int vstat, value;
    char stat[MIL];

    EDIT_RACE(ch, race);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: stats <stat> <value>" COLOR_CLEAR, ch);
        return false;
    }

    READ_ARG(stat);

    vstat = flag_value(stat_table, stat);

    if (vstat == NO_FLAG) {
        send_to_char(COLOR_INFO "Invalid stat." COLOR_CLEAR, ch);
        return false;
    }

    if (!is_number(argument)) {
        send_to_char(COLOR_INFO "The value must be a number." COLOR_CLEAR, ch);
        return false;
    }

    value = atoi(argument);

    if (value < 3 || value > 25) {
        send_to_char(COLOR_INFO "The value must be between 3 and 25." COLOR_CLEAR, ch);
        return false;
    }

    race->stats[vstat] = (int16_t)value;
    send_to_char(COLOR_INFO "Ok." COLOR_CLEAR, ch);
    return true;
}

RAEDIT(raedit_maxstats)
{
    Race* race;
    int vstat, value;
    char stat[MIL];

    EDIT_RACE(ch, race);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: maxstats <stat> <value>" COLOR_CLEAR, ch);
        return false;
    }

    READ_ARG(stat);

    vstat = flag_value(stat_table, stat);

    if (vstat == -1) {
        send_to_char(COLOR_INFO "Invalid stat." COLOR_CLEAR, ch);
        return false;
    }

    if (!is_number(argument)) {
        send_to_char(COLOR_INFO "The value must be a number." COLOR_CLEAR, ch);
        return false;
    }

    value = atoi(argument);

    if (value < 3 || value > 25) {
        send_to_char(COLOR_INFO "The value must be between 3 and 25." COLOR_CLEAR, ch);
        return false;
    }

    race->max_stats[vstat] = (int16_t)value;
    send_to_char(COLOR_INFO "Ok." COLOR_CLEAR, ch);
    return true;
}

RAEDIT(raedit_skills)
{
    Race* race;
    SKNUM sk;
    int num;
    char snum[MIL];

    EDIT_RACE(ch, race);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: skills <num> <skill>" COLOR_CLEAR, ch);
        return false;
    }

    READ_ARG(snum);

    if (!is_number(snum)) {
        send_to_char(COLOR_INFO "Invalid number." COLOR_CLEAR, ch);
        return false;
    }

    num = atoi(snum);

    if (num < 0 || num > 4) {
        send_to_char(COLOR_INFO "The number must be between 0 and 4." COLOR_CLEAR, ch);
        return false;
    }

    sk = skill_lookup(argument);

    if (sk == -1) {
        send_to_char(COLOR_INFO "That skill does not exist." COLOR_CLEAR, ch);
        return false;
    }

    free_string(race->skills[num]);
    race->skills[num] = str_dup(argument);
    send_to_char(COLOR_INFO "Ok." COLOR_CLEAR, ch);
    return true;
}

RAEDIT(raedit_start_loc)
{
    static const char* help = COLOR_INFO "Syntax: START_LOC [CLASS NAME] <ROOM VNUM>" COLOR_CLEAR "\n\r\n\r";
    Race* race;
    char vnum_str[MIL];
    VNUM room_vnum = -1;
    int class_ = -1;

    EDIT_RACE(ch, race);

    if (IS_NULLSTR(argument)) {
        send_to_char(help, ch);
        return false;
    }

    READ_ARG(vnum_str);

    if (!is_number(vnum_str)) {
        if ((class_ = class_lookup(vnum_str)) == -1) {
            send_to_char(help, ch);
            return false;
        }

        READ_ARG(vnum_str);
        if (!is_number(vnum_str)) {
            send_to_char(help, ch);
            return false;
        }
    }

    room_vnum = (VNUM)atoi(vnum_str);

    if (room_vnum > 0 && !get_room_data(room_vnum)) {
        send_to_char(COLOR_INFO "That is not a valid room VNUM." COLOR_CLEAR, ch);
    }

    if (class_ != -1) {
        SET_ELEM(race->class_start, class_, room_vnum); // ensure array is big enough
    }
    else {
        race->start_loc = room_vnum;
    }
    send_to_char(COLOR_INFO "Ok." COLOR_CLEAR, ch);
    return true;
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
