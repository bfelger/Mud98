////////////////////////////////////////////////////////////////////////////////
// raedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "comm.h"
#include "lookup.h"
#include "olc.h"
#include "recycle.h"
#include "tables.h"

#include "entities/player_data.h"

struct	race_type* race_table;
int	maxrace;

#define RACE_FILE	DATA_DIR "races"
#define RAEDIT( fun )		bool fun( CHAR_DATA *ch, char *argument )

extern struct race_type xRace;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const struct olc_comm_type race_olc_comm_table[] = {
    { "show",       0,                  ed_olded,           U(raedit_show)  },
    { "name",       U(&xRace.name),     ed_line_string,     0               },
    { "pcrace",     U(&xRace.pc_race),  ed_bool,            0               },
    { "act",        U(&xRace.act),      ed_flag_toggle,     U(act_flag_table)   },
    { "aff",        U(&xRace.aff),      ed_flag_toggle,     U(affect_flag_table)},
    { "off",        U(&xRace.off),      ed_flag_toggle,     U(off_flag_table)   },
    { "res",        U(&xRace.res),      ed_flag_toggle,     U(res_flag_table)   },
    { "vuln",       U(&xRace.vuln),     ed_flag_toggle,     U(vuln_flag_table)  },
    { "imm",        U(&xRace.imm),      ed_flag_toggle,     U(imm_flag_table)   },
    { "form",       U(&xRace.form),     ed_flag_toggle,     U(form_flag_table)  },
    { "part",       U(&xRace.parts),    ed_flag_toggle,     U(part_flag_table)  },
    { "who",        U(&xRace.who_name), ed_line_string,     0               },
    { "points",     U(&xRace.points),   ed_number_s_pos,    0               },
    { "cmult",      0,                  ed_olded,           U(raedit_cmult) },
    { "stats",      0,                  ed_olded,           U(raedit_stats) },
    { "maxstats",   0,                  ed_olded,           U(raedit_maxstats)},
    { "skills",     0,                  ed_olded,           U(raedit_skills)},
    { "size",       U(&xRace.size),     ed_int16poslookup,  U(size_lookup)  },
//  { "new",        0,                  ed_olded,           U(raedit_new)   },
    { "list",       0,                  ed_olded,           U(raedit_list)  },
    { "commands",   0,                  ed_olded,           U(show_commands)},
    { "?",          0,                  ed_olded,           U(show_help)    },
    { "version",    0,                  ed_olded,           U(show_version) },
    { NULL,         0,                  NULL,               0               }
};

void raedit(CHAR_DATA* ch, char* argument)
{
    if (ch->pcdata->security < 7) {
        send_to_char("RAEdit : You do not have enough security to edit races.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "save")) {
        save_races();
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

void do_raedit(CHAR_DATA* ch, char* argument)
{
    const struct race_type* pRace;
    int race;

    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    if (ch->pcdata->security < 7) {
        send_to_char("RAEdit : You do not have enough security to edit races.\n\r", ch);
        return;
    }

    if (IS_NULLSTR(argument) || (race = race_lookup(argument)) == 0) {
        send_to_char("Syntax : RAEdit [race name]\n\r", ch);
        return;
    }

    pRace = &race_table[race];

    ch->desc->pEdit = U(pRace);
    ch->desc->editor = ED_RACE;

    return;
}

RAEDIT(raedit_show)
{
    struct race_type* pRace;
    char buf[MSL];
    static const char* Stats[] = { "Str", "Int", "Wis", "Dex", "Con" };
    int i = 0;

    EDIT_RACE(ch, pRace);

    sprintf(buf, "Name        : [%s]\n\r", pRace->name);
    send_to_char(buf, ch);
    sprintf(buf, "PC race?    : [%s]\n\r", pRace->pc_race ? "YES" : " NO");
    send_to_char(buf, ch);
    sprintf(buf, "Act         : [%s]\n\r", flag_string(act_flag_table, pRace->act));
    send_to_char(buf, ch);
    sprintf(buf, "Aff         : [%s]\n\r", flag_string(affect_flag_table, pRace->aff));
    send_to_char(buf, ch);
    sprintf(buf, "Off         : [%s]\n\r", flag_string(off_flag_table, pRace->off));
    send_to_char(buf, ch);
    sprintf(buf, "Imm         : [%s]\n\r", flag_string(imm_flag_table, pRace->imm));
    send_to_char(buf, ch);
    sprintf(buf, "Res         : [%s]\n\r", flag_string(res_flag_table, pRace->res));
    send_to_char(buf, ch);
    sprintf(buf, "Vuln        : [%s]\n\r", flag_string(vuln_flag_table, pRace->vuln));
    send_to_char(buf, ch);
    sprintf(buf, "Form        : [%s]\n\r", flag_string(form_flag_table, pRace->form));
    send_to_char(buf, ch);
    sprintf(buf, "Parts       : [%s]\n\r", flag_string(part_flag_table, pRace->parts));
    send_to_char(buf, ch);

#ifndef FIRST_BOOT
    sprintf(buf, "Points      : [%10d] Size   : [%-10.10s]\n\r",
        pRace->points,
        size_table[pRace->size].name);
    send_to_char(buf, ch);

    send_to_char("Name      CMu Exp       Name      CMu Exp       Name      CMu Exp\n\r", ch);
    for (i = 0; i < MAX_CLASS; ++i) {
        sprintf(buf, "%-7.7s   %3d %4d(%3d)%s",
            capitalize(class_table[i].name),
            pRace->class_mult[i],
            race_exp_per_level(pRace->race_id, i, get_points(pRace->race_id, i)),
            get_points(pRace->race_id, i),
            i % 3 == 2 ? "\n\r" : " ");
        send_to_char(buf, ch);
    }
    if (i % 3)
        send_to_char("\n\r", ch);

    for (i = 0; i < MAX_STATS; ++i) {
        sprintf(buf, "%s:%2d(%2d) ", Stats[i], pRace->stats[i], pRace->max_stats[i]);
        send_to_char(buf, ch);
    }
    send_to_char("\n\r", ch);

    for (i = 0; i < 5; ++i)
        if (!IS_NULLSTR(pRace->skills[i])) {
            sprintf(buf, "%2d. %s\n\r",
                i,
                pRace->skills[i]);
            send_to_char(buf, ch);
        }
#endif

    return false;
}

RAEDIT(raedit_list)
{
    int i;
    char buf[MSL];
    BUFFER* fBuf;

    fBuf = new_buf();

    for (i = 0; race_table[i].name; i++) {
        sprintf(buf, "%2d %c #B%-33.33s#b", i,
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
    DESCRIPTOR_DATA* d;
    CHAR_DATA* tch;
    struct race_type* new_table;
    size_t maxRace;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : new [name]\n\r", ch);
        return false;
    }

    if (race_lookup(argument) != 0) {
        send_to_char("That race already exists!\n\r", ch);
        return false;
    }

    for (d = descriptor_list; d; d = d->next) {
        if (d->connected != CON_PLAYING || (tch = CH(d)) == NULL || tch->desc == NULL)
            continue;

        if (tch->desc->editor == ED_RACE)
            edit_done(ch);
    }

    /* reallocate the table */

    for (maxRace = 0; race_table[maxRace].name; maxRace++)
        ;

    maxRace++;
    new_table = realloc(race_table, sizeof(struct race_type) * (maxRace + 1));

    if (!new_table) /* realloc failed */
    {
        send_to_char("Realloc failed. Prepare for impact.\n\r", ch);
        return false;
    }

    race_table = new_table;

    race_table[maxRace - 1].name = str_dup("unique");
    race_table[maxRace - 1].pc_race = false;
    race_table[maxRace - 1].act = 0;
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
    race_table[maxRace - 2].act = 0;
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

    send_to_char("New race created.\n\r", ch);
    return true;
}

RAEDIT(raedit_cmult)
{
    struct race_type* race;
    int mult;
    int vclase;
    char clase[MIL];

    EDIT_RACE(ch, race);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : cmult [class] [multiplier]\n\r", ch);
        return false;
    }

    argument = one_argument(argument, clase);

    if ((vclase = class_lookup(clase)) == -1) {
        send_to_char("RAEdit : That class does not exist.\n\r", ch);
        return false;
    }

    if (!is_number(argument)) {
        send_to_char("RAEdit : The multiplier must be a number.\n\r", ch);
        return false;
    }

    mult = atoi(argument);

    if (mult < 1 || mult > 200) {
        send_to_char("RAEdit : The multiplier must be between 1 and 200.\n\r", ch);
        return false;
    }

    race->class_mult[vclase] = (int16_t)mult;
    send_to_char("Ok.\n\r", ch);
    return true;
}

RAEDIT(raedit_stats)
{
    struct race_type* race;
    int vstat, value;
    char stat[MIL];

    EDIT_RACE(ch, race);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : stats [stat] [value]\n\r", ch);
        return false;
    }

    argument = one_argument(argument, stat);

    vstat = flag_value(stat_table, stat);

    if (vstat == NO_FLAG) {
        send_to_char("RAEdit : Invalid stat.\n\r", ch);
        return false;
    }

    if (!is_number(argument)) {
        send_to_char("RAEdit : The value must be a number.\n\r", ch);
        return false;
    }

    value = atoi(argument);

    if (value < 3 || value > 25) {
        send_to_char("RAEdit : The value must be between 3 and 25.\n\r", ch);
        return false;
    }

    race->stats[vstat] = (int16_t)value;
    send_to_char("Ok.\n\r", ch);
    return true;
}

RAEDIT(raedit_maxstats)
{
    struct race_type* race;
    int vstat, value;
    char stat[MIL];

    EDIT_RACE(ch, race);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : maxstats [stat] [value]\n\r", ch);
        return false;
    }

    argument = one_argument(argument, stat);

    vstat = flag_value(stat_table, stat);

    if (vstat == -1) {
        send_to_char("RAEdit : Invalid stat.\n\r", ch);
        return false;
    }

    if (!is_number(argument)) {
        send_to_char("RAEdit : The value must be a number.\n\r", ch);
        return false;
    }

    value = atoi(argument);

    if (value < 3 || value > 25) {
        send_to_char("RAEdit : The value must be between 3 and 25.\n\r", ch);
        return false;
    }

    race->max_stats[vstat] = (int16_t)value;
    send_to_char("Ok.\n\r", ch);
    return true;
}

RAEDIT(raedit_skills)
{
    struct race_type* race;
    int sk, num;
    char snum[MIL];

    EDIT_RACE(ch, race);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : skills [num] [skill]\n\r", ch);
        return false;
    }

    argument = one_argument(argument, snum);

    if (!is_number(snum)) {
        send_to_char("RAEdit : Invalid number.\n\r", ch);
        return false;
    }

    num = atoi(snum);

    if (num < 0 || num > 4) {
        send_to_char("RAEdit : The number must be between 0 and 4.\n\r", ch);
        return false;
    }

    sk = skill_lookup(argument);

    if (sk == -1) {
        send_to_char("RAEdit : That skill does not exist.\n\r", ch);
        return false;
    }

    free_string(race->skills[num]);
    race->skills[num] = str_dup(argument);
    send_to_char("Ok.\n\r", ch);
    return true;
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
