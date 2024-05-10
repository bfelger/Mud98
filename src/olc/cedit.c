////////////////////////////////////////////////////////////////////////////////
// cedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "db.h"
#include "comm.h"
#include "handler.h"
#include "olc.h"

#include "data/class.h"

#define CEDIT(fun)      bool fun(Mobile *ch, char *argument)

Class xClass;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const OlcCmdEntry class_olc_comm_table[] = {
    { "show",       0,                          ed_olded,           U(cedit_show)       },
    { "name",       U(&xClass.name),            ed_line_string,     0                   },
    { "who",        U(&xClass.who_name),        ed_line_string,     0                   },
    { "basegroup",  U(&xClass.base_group),      ed_skillgroup,      0                   },
    { "defaultgrp", U(&xClass.default_group),   ed_skillgroup,      0                   },
    { "weapon",     U(&xClass.weapon),          ed_olded,           U(cedit_weapon)     },
    { "guild",      U(&xClass.guild),           ed_olded,           U(cedit_guild)      },
    { "primestat",  U(&xClass.prime_stat),      ed_int16lookup,     U(stat_table)       },
    { "skillcap",   U(&xClass.skill_cap),       ed_olded,           U(cedit_skillcap)   },
    { "thac0_00",   U(&xClass.thac0_00),        ed_olded,           U(cedit_thac0_00)   },
    { "thac0_32",   U(&xClass.thac0_32),        ed_olded,           U(cedit_thac0_32)   },
    { "hpmin",      U(&xClass.hp_min),          ed_olded,           U(cedit_hpmin)      },
    { "hpmax",      U(&xClass.hp_max),          ed_olded,           U(cedit_hpmax)      },
    { "titles",     U(&xClass.titles),          ed_olded,           U(cedit_title)      },
    { "mana",       U(&xClass.fMana),           ed_bool,            0                   },
    { "new",        0,                          ed_olded,           U(cedit_new)        },
    { "list",       0,                          ed_olded,           U(cedit_list)       },
    { "commands",   0,                          ed_olded,           U(show_commands)    },
    { "?",          0,                          ed_olded,           U(show_help)        },
    { "version",    0,                          ed_olded,           U(show_version)     },
    { NULL,         0,                          NULL,               0                   }
};

void cedit(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];

    strcpy(arg, argument);
    one_argument(argument, command);

    if (ch->pcdata->security < MIN_CEDIT_SECURITY) {
        send_to_char("CEdit: You do not have enough security to edit classes.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (command[0] == '\0') {
        cedit_show(ch, argument);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    if (!str_cmp(command, "save")) {
        save_class_table();
        return;
    }

    if (!process_olc_command(ch, argument, class_olc_comm_table))
        interpret(ch, argument);
    return;
}

void do_cedit(Mobile* ch, char* argument)
{
    const Class* pClass;
    char command[MSL];
    int class_;

    if (IS_NPC(ch))
        return;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : CEdit [class]\n\r", ch);
        return;
    }

    if (ch->pcdata->security < MIN_CEDIT_SECURITY) {
        send_to_char("CEdit : You do not have enough security to edit classes.\n\r", ch);
        return;
    }

    READ_ARG(command);

    if (!str_cmp(command, "new")) {
        if (cedit_new(ch, argument))
            save_class_table();
        return;
    }

    if (!str_cmp(command, "list")) {
        cedit_list(ch, argument);
        return;
    }

    if ((class_ = class_lookup(command)) == -1) {
        send_to_char("CEdit : That class does not exist\n\r", ch);
        return;
    }

    pClass = &class_table[class_];

    ch->desc->pEdit = (uintptr_t)pClass;
    ch->desc->editor = ED_CLASS;

    cedit_show(ch, "");

    return;
}

CEDIT(cedit_new)
{
    Descriptor* d;
    Mobile* tch;
    Class* new_table;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : new [name]\n\r", ch);
        return false;
    }

    if (class_lookup(argument) != -1) {
        send_to_char("That class already exists!\n\r", ch);
        return false;
    }

    FOR_EACH(d, descriptor_list) {
        if (d->connected != CON_PLAYING || (tch = CH(d)) == NULL || tch->desc == NULL)
            continue;

        if (tch->desc->editor == ED_CLASS)
            edit_done(ch);
    }

    /* reallocate the table */
    ++class_count;
    new_table = realloc(class_table, sizeof(Class) * ((size_t)class_count + 1));

    if (!new_table) /* realloc failed */
    {
        send_to_char("Realloc failed. Prepare for impact.\n\r", ch);
        return false;
    }

    class_table = new_table;
    Class* pClass = &class_table[class_count - 1];
    memset(pClass, 0, sizeof(Class));

    char who_buf[4];
    sprintf(who_buf, "%.3s", argument);

    pClass->name = str_dup(argument);
    pClass->who_name = str_dup(capitalize(who_buf));
    pClass->base_group = str_dup("");
    pClass->default_group = str_dup("");
    pClass->weapon = 0;
    for (int i = 0; i < MAX_GUILD; ++i)
        pClass->guild[i] = 0;
    pClass->prime_stat = 0;
    pClass->skill_cap = DEFAULT_SKILL_LEVEL;
    pClass->thac0_00 = 0;
    pClass->thac0_32 = 0;
    pClass->hp_min = 0;
    pClass->hp_max = 0;
    pClass->fMana = false;

    pClass->titles[0][0] = str_dup("Man");
    pClass->titles[0][1] = str_dup("Woman");
    pClass->titles[1][0] = str_dup(capitalize(pClass->name));
    pClass->titles[1][1] = str_dup(capitalize(pClass->name));
    // Class titles are now sparse. They only change if there is a value 
    // specified at that level.
    pClass->titles[MAX_LEVEL - 1][0] = str_dup("Creator");
    pClass->titles[MAX_LEVEL - 1][1] = str_dup("Creator");
    pClass->titles[MAX_LEVEL][0] = str_dup("Implementor");
    pClass->titles[MAX_LEVEL][1] = str_dup("Implementor");

    ch->desc->editor = ED_CLASS;
    ch->desc->pEdit = U(&class_table[class_count - 1]);

    send_to_char("New class created.\n\r", ch);

    cedit_show(ch, "");

    return true;
}

static int get_weapon_index(VNUM vnum)
{
    int w = -1;
    for (int i = 1; i < WEAPON_TYPE_COUNT; ++i)
        if (vnum == weapon_table[i].vnum) {
            w = i;
            break;
        }

    return w;
}

CEDIT(cedit_list)
{
    INIT_BUF(page, MSL);

    addf_buf(page, "{T%-20s    %20s %37s      %2s\n\r", "", "Skill Groups", "THAC0", "HP");
    addf_buf(page, "{T%-20s  %3s  %-15s %-15s %-10s %-5s %-4s %-4s %-3s %-3s %-5s{x\n\r",
        "Name", "Who", "Base", "Default", "Weapon", "SkCap", "00", "32", "Min", "Max", "Mana?");

    for (int i = 0; i < class_count; ++i) {
        Class* class_ = &class_table[i];
        int w = get_weapon_index(class_->weapon);
        const char* w_name = (w > 0) ? weapon_table[w].name : "(unknown)";
        //                 name       who       base def  weap cap t00 t32 hp+ hp- mana 
        addf_buf(page, "%-20s {|[{*%3s{|]{* %-15s %-15s %-10s %-5d %-4d %-4d %-3d %-3d %-5s{x\n\r",
            class_->name, class_->who_name, class_->base_group, 
            class_->default_group, w_name, class_->skill_cap, class_->thac0_00, 
            class_->thac0_32, class_->hp_min, class_->hp_max, 
            (class_->fMana ? "{GYes{x" : "{RNo{x"));
    }

    page_to_char(page->string, ch);
    free_buf(page);
    return true;
}

CEDIT(cedit_show)
{
    INIT_BUF(out, MSL);

    Class* pClass;

    EDIT_CLASS(ch, pClass);

    addf_buf(out, "{TClass Name  {|: [{*%s{|]\n\r", pClass->name);
    addf_buf(out, "{TWho Abbr.   {|: [{*%s{|]\n\r", pClass->who_name);
    addf_buf(out, "{TBase Group  {|: [{*%s{|]\n\r", pClass->base_group);
    addf_buf(out, "{TDefault Grp {|: [{*%s{|]\n\r", pClass->default_group);

    int w = get_weapon_index(pClass->weapon);

    if (w > 0)
        addf_buf(out, "{TWeapon      {|: [{*%s {_(%d){|]\n\r", weapon_table[w].name, pClass->weapon);
    else
        addf_buf(out, "{TWeapon      {|: []\n\r");

    addf_buf(out, "{TGuild VNUMs {|: [{*");

    for (int i = 0; i < MAX_GUILD; ++i) {
        addf_buf(out, "%d", pClass->guild[i]);
        if (i < MAX_GUILD - 1)
            addf_buf(out, ", ");
    }
    addf_buf(out, "{|]\n\r");
    addf_buf(out, "{TPrime Stat  {|: [{*%s{|]\n\r", stat_table[pClass->prime_stat].name);
    addf_buf(out, "{TSkill Cap   {|: [{*%d{|]\n\r", pClass->skill_cap);
    addf_buf(out, "{TTHAC0 @00   {|: [{*%d{|]\n\r", pClass->thac0_00);
    addf_buf(out, "{TTHAC0 @32   {|: [{*%d{|]\n\r", pClass->thac0_32);
    addf_buf(out, "{THP Min/Lvl  {|: [{*%d{|]\n\r", pClass->hp_min);
    addf_buf(out, "{THP Max/Lvl  {|: [{*%d{|]\n\r", pClass->hp_max);
    addf_buf(out, "{TMana?       {|: [%s{|]\n\r", (pClass->fMana ? "{GYes" : "{RNo"));

    addf_buf(out, "{TTitles      {|:\n\r");
    for (int i = 0; i <= MAX_LEVEL; ++i) {
        const char* m_title = pClass->titles[i][0] ? pClass->titles[i][0] : "";
        const char* f_title = pClass->titles[i][1] ? pClass->titles[i][1] : "";
        if (!m_title[0] && !f_title[0])
            continue;
        addf_buf(out, "    {|[{*%3d {|] {*%-25s %-25s\n\r", i, m_title, f_title);
    }
    add_buf(out, "{x\n\r");

    page_to_char(out->string, ch);
    free_buf(out);

    return false;
}

static void show_weapon_list(Mobile* ch)
{
    send_to_char("Choices include: ", ch);
    for (int i = 1; i < WEAPON_TYPE_COUNT; ++i) {
        if (i < WEAPON_TYPE_COUNT - 1)
            printf_to_char(ch, "%s, ", weapon_table[i].name);
        else
            printf_to_char(ch, "%s\n\r");
    }
}

CEDIT(cedit_weapon)
{
    Class* class_;
    char weapon_name[MIL];
    VNUM weapon = -1;

    EDIT_CLASS(ch, class_);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : {*WEAPON <WEAPON NAME>{x\n\r\n\r", ch);
        show_weapon_list(ch);
        return false;
    }

    READ_ARG(weapon_name);

    for (int i = 1; i < WEAPON_TYPE_COUNT; i++)
        if (!str_prefix(weapon_name, weapon_table[i].name))
            weapon = weapon_table[i].vnum;

    if (weapon < 0) {
        send_to_char("CEdit : That weapon does not exist.\n\r", ch);
        show_weapon_list(ch);
        return false;
    }

    class_->weapon = weapon;
    send_to_char("Ok.\n\r", ch);
    return true;
}

CEDIT(cedit_start_loc)
{
    Class* class_;
    EDIT_CLASS(ch, class_);
    char vnum_str[MIL];
    VNUM vnum = -1;

    EDIT_CLASS(ch, class_);

    READ_ARG(vnum_str);

    if (!vnum_str[0] || !is_number(vnum_str)) {
        send_to_char("Syntax : {*START_LOC <VNUM>{x\n\r\n\r", ch);
        return false;
    }

    vnum = (VNUM)atoi(vnum_str);

    if (!get_room_data(vnum)) {
        printf("{jCEdit : There is no room with VNUM %d.{x\n\r", vnum);
        return false;
    }

    class_->start_loc = vnum;
    send_to_char("{jOk.{x\n\r", ch);
    return true;
}

CEDIT(cedit_guild)
{
    Class* class_;
    char slot_str[MIL];
    int slot = 0;
    char vnum_str[MIL];
    VNUM vnum = -1;

    EDIT_CLASS(ch, class_);

    READ_ARG(slot_str);
    READ_ARG(vnum_str);

    if (!slot_str[0] || !is_number(slot_str) || !vnum_str[0] || !is_number(vnum_str)) {
        send_to_char("Syntax : {*GUILD <SLOT> <VNUM>{x\n\r\n\r", ch);
        return false;
    }

    slot = atoi(slot_str);
    vnum = (VNUM)atoi(vnum_str);

    if (slot < 0 || slot >= MAX_GUILD) {
        printf("CEdit : Slot number must be from 0 to %d.\n\r", MAX_GUILD - 1);
        return false;
    }

    if (!get_room_data(vnum)) {
        printf("CEdit : There is no room with VNUM %d.\n\r", vnum);
        return false;
    }

    class_->guild[slot] = vnum;
    send_to_char("Ok.\n\r", ch);
    return true;
}

CEDIT(cedit_title)
{
    Class* class_;
    LEVEL level;
    Sex choice;

    char lvl_arg[MIL];
    char sex_arg[MIL];
    
    EDIT_CLASS(ch, class_);

    READ_ARG(lvl_arg);
    READ_ARG(sex_arg);

    if (!lvl_arg[0] || !is_number(lvl_arg) || !sex_arg[0] || !argument[0]) {
        send_to_char("Syntax : {*TITLE <LEVEL> <MALE/FEMALE/BOTH> <TITLE>{x\n\r\n\r", ch);
        return false;
    }

    level = (LEVEL)atoi(lvl_arg);

    if (!str_prefix(sex_arg, "male"))
        choice = SEX_MALE;
    else if (!str_prefix(sex_arg, "female"))
        choice = SEX_FEMALE;
    else if (!str_prefix(sex_arg, "both"))
        choice = SEX_EITHER;
    else {
        send_to_char("Syntax : {*TITLE <LEVEL> <MALE/FEMALE/BOTH> <TITLE>{x\n\r\n\r", ch);
        return false;
    }

    if (choice == SEX_MALE || choice == SEX_EITHER) {
        free_string(class_->titles[level][0]);
        class_->titles[level][0] = str_dup(argument);
    }

    if (choice == SEX_FEMALE || choice == SEX_EITHER) {
        free_string(class_->titles[level][1]);
        class_->titles[level][1] = str_dup(argument);
    }

    send_to_char("Ok.\n\r", ch);
    return true;
}

#define INT16_FUNC(func_name, field_name)                                      \
CEDIT(func_name)                                                               \
{                                                                              \
    Class* pClass;                                                             \
    int16_t i;                                                                 \
                                                                               \
    EDIT_CLASS(ch, pClass);                                                    \
                                                                               \
    if (IS_NULLSTR(argument) || !is_number(argument)) {                        \
        send_to_char("CEdit : argument must be a number.\n\r", ch);            \
        return false;                                                          \
    }                                                                          \
                                                                               \
    i = (int16_t)atoi(argument);                                               \
                                                                               \
    pClass->field_name = i;                                                    \
    send_to_char("Ok.\n\r", ch);                                               \
    return true;                                                               \
}

INT16_FUNC(cedit_skillcap, skill_cap)
INT16_FUNC(cedit_thac0_00, thac0_00)
INT16_FUNC(cedit_thac0_32, thac0_32)
INT16_FUNC(cedit_hpmin, hp_min)
INT16_FUNC(cedit_hpmax, hp_max)

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
