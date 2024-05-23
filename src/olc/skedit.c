////////////////////////////////////////////////////////////////////////////////
// skedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "bit.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "lookup.h"
#include "magic.h"
#include "spell_list.h"
#include "olc.h"
#include "recycle.h"
#include "skills.h"
#include "tables.h"

#include "entities/descriptor.h"
#include "entities/player_data.h"

#include "data/mobile_data.h"
#include "data/skill.h"
#include "data/spell.h"

#define SKEDIT(fun) bool fun(Mobile *ch, char *argument)

SkillHash* skill_hash_table[26];

void create_skills_hash_table(void);
void delete_skills_hash_table(void);

struct gsn_type {
    char* name;
    SKNUM* pgsn;
};

#define GSN(x) { #x, &x },

const struct gsn_type gsn_table[] = {
#include "gsn.h"
    { NULL, NULL }
};

Skill xSkill;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const OlcCmdEntry skill_olc_comm_table[] = {
    { "name",       0,                          ed_olded,       U(skedit_name)  },
    { "beats",      U(&xSkill.beats),           ed_number_s_pos,0               },
    { "position",   U(&xSkill.minimum_position),ed_int16lookup, U(position_lookup)},
    { "slot",       0,                          ed_olded,       U(skedit_slot)  },
    { "target",     U(&xSkill.target),          ed_flag_set_sh, U(target_table) },
    { "mana",       U(&xSkill.min_mana),        ed_number_s_pos,0               },
    { "level",      0,                          ed_olded,       U(skedit_level) },
    { "rating",     0,                          ed_olded,       U(skedit_rating)},
    { "gsn",        0,                          ed_olded,       U(skedit_gsn)   },
    { "spell",      0,                          ed_olded,       U(skedit_spell) },
    { "noun",       U(&xSkill.noun_damage),     ed_line_string, 0               },
    { "off",        U(&xSkill.msg_off),         ed_line_string, 0               },
    { "obj",        U(&xSkill.msg_obj),         ed_line_string, 0               },
    { "new",        0,                          ed_olded,       U(skedit_new)   },
    { "list",       0,                          ed_olded,       U(skedit_list)  },
    { "show",       0,                          ed_olded,       U(skedit_show)  },
    { "commands",   0,                          ed_olded,       U(show_commands)},
    { "?",          0,                          ed_olded,       U(show_help)    },
    { "version",    0,                          ed_olded,       U(show_version) },
    { NULL,         0,                          0,              0               }
};


char* gsn_name(SKNUM* pgsn)
{
    int i = 0;

    if (pgsn == NULL)
        return "";

    while (gsn_table[i].name)
        if (gsn_table[i].pgsn == pgsn)
            return gsn_table[i].name;
        else
            i++;

    if (fBootDb)
        bug("gsn_name : pgsn %d does not exist", *pgsn);

    return "";
}


void check_gsns(void)
{
    int i;

    for (i = 0; gsn_table[i].name; ++i)
        if (*gsn_table[i].pgsn == 0)
            bugf("check_gsns : gsn %d(%s) has not been assigned!", i,
                gsn_table[i].name);

    return;
}

SKNUM* gsn_lookup(char* argument)
{
    int i;
    char buf[MSL];

    if (IS_NULLSTR(argument))
        return NULL;

    for (i = 0; gsn_table[i].name; ++i)
        if (!str_cmp(gsn_table[i].name, argument))
            return gsn_table[i].pgsn;

    if (fBootDb == true) {
        sprintf(buf, "gsn_lookup : gsn %s does not exist", argument);
        bug(buf, 0);
    }

    return NULL;
}

void skedit(Mobile* ch, char* argument)
{
    if (ch->pcdata->security < MIN_SKEDIT_SECURITY) {
        send_to_char("SKEdit : You do not have enough security to edit skills.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "save")) {
        save_skill_table();
        return;
    }

    if (emptystring(argument)) {
        skedit_show(ch, argument);
        return;
    }

    /* Search Table and Dispatch Command. */
    if (!process_olc_command(ch, argument, skill_olc_comm_table))
        interpret(ch, argument);

    return;
}

void do_sklist(Mobile* ch, char* argument)
{
    static char* help = "{jSyntax : {*SKLIST <OPTION>{x\n\r\n\r"
        "{*<OPTION>{x can be one of the following:\n\r"
        "    {*RATING{x - List rating by class.\n\r"
        "    {*LEVEL{x - List minimum level by class.\n\r\n\r";

    INIT_BUF(page, MSL);

    if (ch->pcdata->security < MIN_SKEDIT_SECURITY) {
        send_to_char("{jSKList : You do not have enough security to list skills.{x\n\r", ch);
        return;
    }

    if (IS_NULLSTR(argument)) {
        send_to_char(help, ch);
        return;
    }

    char command[MIL];
    
    bool show_rating = false;
    bool show_level = false;

    READ_ARG(command);

    if (!str_prefix(command, "RATING")) {
        show_rating = true;
    }
    else if (!str_prefix(command, "LEVEL")) {
        show_level = true;
    }
    else {
        send_to_char(help, ch);
        return;
    }

    addf_buf(page, "\n\r{T%-20s", "Skill");
    for (int j = 0; j < class_count; ++j) {
        addf_buf(page, "  %3s", class_table[j].who_name);
    }
    add_buf(page, "\n\r");
    for (int i = 0; i < skill_count; ++i) {
        if (!skill_table[i].name[0])
            break;
        addf_buf(page, "{*%-20s{x", skill_table[i].name);
        for (int j = 0; j < class_count; ++j) {
            if (show_rating)
                addf_buf(page, "%4d ", GET_ELEM(&skill_table[i].rating, j));
            else if (show_level)
                addf_buf(page, "%4d ", GET_ELEM(&skill_table[i].skill_level, j));
        }
        add_buf(page, "\n\r");
    }
    add_buf(page, "\n\r");

    page_to_char(page->string, ch);

    free_buf(page);
}

void do_skedit(Mobile* ch, char* argument)
{
    const Skill* pSkill;
    char command[MSL];
    SKNUM skill;

    if (IS_NPC(ch))
        return;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : SKEdit [skill]\n\r", ch);
        return;
    }

    if (ch->pcdata->security < MIN_SKEDIT_SECURITY) {
        send_to_char("SKEdit : You do not have enough security to edit skills.\n\r", ch);
        return;
    }

    one_argument(argument, command);

    if (!str_cmp(command, "new")) {
        READ_ARG(command);
        if (skedit_new(ch, argument))
            save_skill_table();
        return;
    }

    if ((skill = skill_lookup(argument)) == -1) {
        send_to_char("SKEdit : That skill does not exist.\n\r", ch);
        return;
    }

    pSkill = &skill_table[skill];

    ch->desc->pEdit = U(pSkill);
    ch->desc->editor = ED_SKILL;

    return;
}

void skill_list(Buffer* pBuf)
{
    char buf[MSL];
    int i;

    sprintf(buf, "Lev   %-20.20s Lev   %-20.20s Lev   %-20.20s\n\r",
        "Name", "Name", "Name");
    add_buf(pBuf, buf);

    for (i = 0; i < skill_count; ++i) {
        sprintf(buf, "{*%3d{x %c %-20.20s", i,
            skill_table[i].spell_fun == spell_null ? '-' : '+',
            skill_table[i].name);
        if (i % 3 == 2)
            strcat(buf, "\n\r");
        else
            strcat(buf, " ");
        add_buf(pBuf, buf);
    }

    if (i % 3 != 0)
        add_buf(pBuf, "\n\r");
}

void gsn_list(Buffer* pBuf)
{
    char buf[MSL];
    int i;

    sprintf(buf, "Num %-22.22s Num %-22.2s Num %-22.22s\n\r",
        "Name", "Name", "Name");
    add_buf(pBuf, buf);

    for (i = 0; gsn_table[i].name; ++i) {
        sprintf(buf, "{*%3d{x %-22.22s", i,
            gsn_table[i].name);
        if (i % 3 == 2)
            strcat(buf, "\n\r");
        else
            strcat(buf, " ");
        add_buf(pBuf, buf);
    }

    if (i % 3 != 0)
        add_buf(pBuf, "\n\r");
}

void slot_list(Buffer* pBuf)
{
    char buf[MSL];
    int i, cnt;

    sprintf(buf, "Num %-22.22s Num %-22.2s Num %-22.22s\n\r",
        "Name", "Name", "Name");
    add_buf(pBuf, buf);

    cnt = 0;
    for (i = 0; i < skill_count; ++i) {
        if (skill_table[i].slot) {
            sprintf(buf, "{*%3d{x %-22.22s",
                skill_table[i].slot,
                skill_table[i].name);
            if (cnt % 3 == 2)
                strcat(buf, "\n\r");
            else
                strcat(buf, " ");
            add_buf(pBuf, buf);
            cnt++;
        }
    }

    if (cnt % 3 != 0)
        add_buf(pBuf, "\n\r");
}

SKEDIT(skedit_list)
{
    Buffer* pBuf;

    if (IS_NULLSTR(argument) || !is_name(argument, "gsns skills spells slots")) {
        send_to_char("Syntax : list [gsns/skills/spells/slots]\n\r", ch);
        return false;
    }

    pBuf = new_buf();

    if (!str_prefix(argument, "skills"))
        skill_list(pBuf);
    else if (!str_prefix(argument, "spells"))
        spell_list(pBuf);
    else if (!str_prefix(argument, "slots"))
        slot_list(pBuf);
    else if (!str_prefix(argument, "gsns"))
        gsn_list(pBuf);
    else
        add_buf(pBuf, "You may only list gsns/skills/spells/slots.\n\r");

    page_to_char(BUF(pBuf), ch);

    free_buf(pBuf);

    return false;
}

SKEDIT(skedit_show)
{
    Skill* pSkill;
    char buf[MAX_STRING_LENGTH];
    char buf2[MSL];
    int i;
    SKNUM sn = 0;

    EDIT_SKILL(ch, pSkill);

    sprintf(buf, "Name     : [%s]\n\r",
        pSkill->name);
    send_to_char(buf, ch);

    while ((sn < skill_count) && (pSkill != &skill_table[sn]))
        sn++;

    if (sn != skill_count) {
        sprintf(buf, "Sn       : [%3d/%3d]\n\r", sn, skill_count);
        send_to_char(buf, ch);
    }

    sprintf(buf, "Class    + ");

    for (i = 0; i < class_count; ++i) {
        strcat(buf, class_table[i].who_name);
        strcat(buf, " ");
    }

    strcat(buf, "\n\r");
    send_to_char(buf, ch);

    sprintf(buf, "Level    | ");

    for (i = 0; i < class_count; ++i) {
        sprintf(buf2, "%3d ", GET_ELEM(&pSkill->skill_level, i));
        strcat(buf, buf2);
    }

    strcat(buf, "\n\r");
    send_to_char(buf, ch);

    sprintf(buf, "Rating   | ");

    for (i = 0; i < class_count; ++i) {
        sprintf(buf2, "%3d ", GET_ELEM(&pSkill->rating, i));
        strcat(buf, buf2);
    }

    strcat(buf, "\n\r");
    send_to_char(buf, ch);

    sprintf(buf, "Spell    : [%s]\n\r", spell_name(pSkill->spell_fun));
    send_to_char(buf, ch);

    sprintf(buf, "Target   : [%s]\n\r", flag_string(target_table, pSkill->target));
    send_to_char(buf, ch);

    sprintf(buf, "Min Pos  : [%s]\n\r", position_table[pSkill->minimum_position].name);
    send_to_char(buf, ch);

    sprintf(buf, "pGsn     : [%s]\n\r", gsn_name(pSkill->pgsn));
    send_to_char(buf, ch);

    sprintf(buf, "Slot     : [%d]\n\r", pSkill->slot);
    send_to_char(buf, ch);

    sprintf(buf, "Min Mana : [%d]\n\r", pSkill->min_mana);
    send_to_char(buf, ch);

    sprintf(buf, "Beats    : [%d], %.2f seconds(s).\n\r",
        pSkill->beats,
        pSkill->beats / (float)PULSE_PER_SECOND);
    send_to_char(buf, ch);

    sprintf(buf, "Noun Dmg : [%s]\n\r", pSkill->noun_damage);
    send_to_char(buf, ch);

    sprintf(buf, "Msg Off  : [%s]\n\r", pSkill->msg_off);
    send_to_char(buf, ch);

    sprintf(buf, "Msg Obj  : [%s]\n\r", pSkill->msg_obj);
    send_to_char(buf, ch);

    return false;
}

void create_skills_hash_table(void)
{
    int value;
    SkillHash* data;
    SkillHash* temp;
    SKNUM sn;

    for (sn = 0; sn < skill_count; sn++) {
        if (IS_NULLSTR(skill_table[sn].name))
            continue;

        value = (int)(LOWER(skill_table[sn].name[0]) - 'a');

        if (value < 0 || value > 25) {
            bug("create_skills_hash_table : value '%d' is invalid.", value);
            exit(1);
        }

        if ((data = new_skill_hash()) == NULL) {
            perror("create_skills_hash_table: Could not allocate new skill_hash!");
            exit(-1);
        }
        data->sn = sn;

        // Now link to the table
        if (skill_hash_table[value] && (skill_table[sn].spell_fun == spell_null)) // skill
        {
            // Skills go last!
            FOR_EACH(temp, skill_hash_table[value])
                if (temp->next == NULL)
                    break;

            if (!temp || temp->next) {
                bug("Skill_hash_table : ???", 0);
                exit(1);
            }

            temp->next = data;
            data->next = NULL;
        }
        else {
            data->next = skill_hash_table[value];
            skill_hash_table[value] = data;
        }
    }
}

void delete_skills_hash_table(void)
{
    SkillHash* temp = NULL;
    SkillHash* temp_next = NULL;
    int i;

    for (i = 0; i < 26; ++i) {
        for (temp = skill_hash_table[i]; temp; temp = temp_next) {
            temp_next = temp->next;
            free_skill_hash(temp);
        }
        skill_hash_table[i] = NULL;
    }
}

SKEDIT(skedit_name)
{
    Skill* pSkill;

    EDIT_SKILL(ch, pSkill);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : name [name]\n\r", ch);
        return false;
    }

    if (skill_lookup(argument) != -1) {
        send_to_char("No skill or spell of that name exists.\n\r", ch);
        return false;
    }

    free_string(pSkill->name);
    pSkill->name = str_dup(argument);

    delete_skills_hash_table();
    create_skills_hash_table();

    send_to_char("Ok.\n\r", ch);
    return true;
}

SKEDIT(skedit_slot)
{
    Skill* pSkill;

    EDIT_SKILL(ch, pSkill);

    if (IS_NULLSTR(argument) || !is_number(argument) || atoi(argument) < 0) {
        send_to_char("Syntax : slot [number]\n\r", ch);
        return false;
    }

    int slot = atoi(argument);

    if (skill_slot_lookup(slot) != -1) {
        send_to_char("That slot is already in use.\n\r", ch);
        return false;
    }

    pSkill->slot = slot;

    send_to_char("Ok.\n\r", ch);
    return true;
}

SKEDIT(skedit_level)
{
    Skill* pSkill;
    char arg[MIL];
    LEVEL level;
    int16_t class_;

    EDIT_SKILL(ch, pSkill);

    READ_ARG(arg);

    if (IS_NULLSTR(argument) || IS_NULLSTR(arg) || !is_number(argument)) {
        send_to_char("SKEdit : level [class] [level]\n\r", ch);
        return false;
    }

    if ((class_ = class_lookup(arg)) == -1) {
        send_to_char("SKEdit : That class does not exist.\n\r", ch);
        return false;
    }

    level = (LEVEL)atoi(argument);

    if (level < 0 || level > MAX_LEVEL) {
        send_to_char("SKEdit : Invalid level.\n\r", ch);
        return false;
    }

    SET_ELEM(pSkill->skill_level, class_, level);
    send_to_char("Ok.\n\r", ch);
    return true;
}

SKEDIT(skedit_rating)
{
    Skill* pSkill;
    char arg[MIL];
    int16_t rating;
    int16_t class_;

    EDIT_SKILL(ch, pSkill);

    READ_ARG(arg);

    if (IS_NULLSTR(argument) || IS_NULLSTR(arg) || !is_number(argument)) {
        send_to_char("Syntax : rating [class] [level]\n\r", ch);
        return false;
    }

    if ((class_ = class_lookup(arg)) == -1) {
        send_to_char("SKEdit : That class does not exist.\n\r", ch);
        return false;
    }

    rating = (int16_t)atoi(argument);

    if (rating < 0) {
        send_to_char("SKEdit : Invalid rating.\n\r", ch);
        return false;
    }

    SET_ELEM(pSkill->rating, class_, rating);
    send_to_char("Ok.\n\r", ch);
    return true;
}

SKEDIT(skedit_spell)
{
    Skill* pSkill;
    SpellFunc* spell;

    EDIT_SKILL(ch, pSkill);

    if (IS_NULLSTR(argument)) {
        send_to_char("Sintaxis : spell [function name]\n\r", ch);
        send_to_char("           spell spell_null\n\r", ch);
        return false;
    }

    if ((spell = spell_function(argument)) == spell_null
        && str_cmp(argument, "spell_null")) {
        send_to_char("SKEdit : That spell function does not exist.\n\r", ch);
        return false;
    }

    pSkill->spell_fun = spell;
    send_to_char("Ok.\n\r", ch);
    return true;
}

SKEDIT(skedit_gsn)
{
    Skill* pSkill;
    SKNUM* gsn;
    SKNUM sn;

    EDIT_SKILL(ch, pSkill);

    if (IS_NULLSTR(argument)) {
        send_to_char("SKedit : gsn [name]\n\r", ch);
        send_to_char("           gsn null\n\r", ch);
        return false;
    }

    gsn = NULL;

    if (str_cmp(argument, "null") && (gsn = gsn_lookup(argument)) == NULL) {
        send_to_char("SKEdit : That gsn does not exist.\n\r", ch);
        return false;
    }

    pSkill->pgsn = gsn;
    for (sn = 0; sn < skill_count; sn++) {
        if (skill_table[sn].pgsn != NULL)
            *skill_table[sn].pgsn = sn;
    }

    send_to_char("Ok.\n\r", ch);
    return true;
}

SKEDIT(skedit_new)
{
    Descriptor* d;
    Mobile* tch;
    Skill* new_table;
    bool* tempgendata;
    SKNUM* templearned;
    int i;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : new [name]\n\r", ch);
        return false;
    }

    if (skill_lookup(argument) != -1) {
        send_to_char("That skill already exists!\n\r", ch);
        return false;
    }

    FOR_EACH(d, descriptor_list) {
        if (d->connected != CON_PLAYING || (tch = CH(d)) == NULL || tch->desc == NULL)
            continue;

        if (tch->desc->editor == ED_SKILL)
            edit_done(tch);
    }

    /* reallocate the table */
    skill_count++;
    new_table = realloc(skill_table, sizeof(Skill) * ((size_t)skill_count + 1));

    if (!new_table) /* realloc failed */
    {
        perror("skedit_new (1): Falled to reallocate skill_table!");
        send_to_char("Realloc failed. Prepare for impact. (1)\n\r", ch);
        return false;
    }

    skill_table = new_table;

    skill_table[skill_count - 1].name = str_dup(argument);
    for (i = 0; i < class_count; ++i) {
        CREATE_ELEM(skill_table[skill_count - 1].skill_level);
        CREATE_ELEM(skill_table[skill_count - 1].rating);
    }
    skill_table[skill_count - 1].spell_fun = spell_null;
    skill_table[skill_count - 1].target = SKILL_TARGET_IGNORE;
    skill_table[skill_count - 1].minimum_position = POS_STANDING;
    skill_table[skill_count - 1].pgsn = NULL;
    skill_table[skill_count - 1].slot = 0;
    skill_table[skill_count - 1].min_mana = 0;
    skill_table[skill_count - 1].beats = 0;
    skill_table[skill_count - 1].noun_damage = str_dup("");
    skill_table[skill_count - 1].msg_off = str_dup("");
    skill_table[skill_count - 1].msg_obj = str_dup("");

    skill_table[skill_count].name = NULL;

    FOR_EACH(d, descriptor_list) {
        if ((d->connected == CON_PLAYING)
            || ((tch = CH(d)) == NULL)
            || (tch->gen_data == NULL))
            continue;

        tempgendata = realloc(tch->gen_data->skill_chosen, sizeof(bool) * skill_count);
        if (!tempgendata) {
            perror("skedit_new (2): Falled to reallocate tempgendata!");
            send_to_char("Realloc failed. Prepare for impact. (2)\n\r", ch);
            return false;
        }
        tch->gen_data->skill_chosen = tempgendata;
        tch->gen_data->skill_chosen[skill_count - 1] = 0;
    }

    FOR_EACH_GLOBAL_MOB(tch)
        if (!IS_NPC(tch)) {
            templearned = new_learned();

            /* copiamos los valuees */
            for (i = 0; i < skill_count - 1; ++i)
                templearned[i] = tch->pcdata->learned[i];

            free_learned(tch->pcdata->learned);
            tch->pcdata->learned = templearned;
        }

    delete_skills_hash_table();
    create_skills_hash_table();
    ch->desc->editor = ED_SKILL;
    ch->desc->pEdit = U(&skill_table[skill_count - 1]);

    send_to_char("Skill created.\n\r", ch);
    return true;
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
