////////////////////////////////////////////////////////////////////////////////
// cedit.c
////////////////////////////////////////////////////////////////////////////////

#include <merc.h>

#include "olc.h"

#include <comm.h>
#include <config.h>
#include <db.h>
#include <handler.h>

#ifdef _MSC_VER
#include <io.h>
#define access _access
#else
#include <unistd.h>
#endif

#ifndef F_OK
#define F_OK 0
#endif

#include <data/class.h>

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
    { "armorprof",  U(&xClass.armor_prof),  ed_int16lookup,     U(armor_type_lookup)},
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

    if (!str_prefix("save", command)) {
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
        const char* classes_file = cfg_get_classes_file();
        const char* ext = strrchr(classes_file, '.');
        bool has_ext = (ext != NULL);

        if (!force_format) {
            if (has_ext) {
                requested_ext = NULL; // respect existing extension
            } else {
                // Only apply default format when this looks like a new file
                if (access(classes_file, F_OK) != 0) {
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
            size_t base_len = has_ext ? (size_t)(ext - classes_file) : strlen(classes_file);
            char newname[MIL];
            snprintf(newname, sizeof(newname), "%.*s%s", (int)base_len, classes_file, requested_ext);
            cfg_set_classes_file(newname);
        }
        save_class_table();
        send_to_char("Classes saved.\n\r", ch);
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

    set_editor(ch->desc, ED_CLASS, (uintptr_t)pClass);

    cedit_show(ch, "");

    return;
}

CEDIT(cedit_new)
{
    Descriptor* d;
    Mobile* tch;
    Class* new_class_table;

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

        if (get_editor(tch->desc) == ED_CLASS)
            edit_done(ch);
    }

    /* reallocate the table */
    ++class_count;
    new_class_table = realloc(class_table, sizeof(Class) * ((size_t)class_count + 1));

    if (!new_class_table) /* realloc failed */
    {
        send_to_char("Realloc failed. Prepare for impact.\n\r", ch);
        return false;
    }

    class_table = new_class_table;
    Class* pClass = &class_table[class_count - 1];
    memset(pClass, 0, sizeof(Class));

    char who_buf[4];
    sprintf(who_buf, "%.3s", argument);

    pClass->name = str_dup(argument);
    pClass->who_name = str_dup(capitalize(who_buf));
    pClass->base_group = str_dup("");
    pClass->default_group = str_dup("");
    pClass->weapon = 0;
    pClass->armor_prof = ARMOR_OLD_STYLE;
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

    set_editor(ch->desc, ED_CLASS, U(&class_table[class_count - 1]));

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

    addf_buf(page, COLOR_TITLE "%-20s    %20s %37s      %2s\n\r", "", "Skill Groups", "THAC0", "HP");
    addf_buf(page, COLOR_TITLE "%-20s  %3s  %-15s %-15s %-10s %-5s %-4s %-4s %-3s %-3s %-5s" COLOR_EOL,
        "Name", "Who", "Base", "Default", "Weapon", "SkCap", "00", "32", "Min", "Max", "Mana?");

    for (int i = 0; i < class_count; ++i) {
        Class* class_ = &class_table[i];
        int w = get_weapon_index(class_->weapon);
        const char* w_name = (w > 0) ? weapon_table[w].name : "(unknown)";
        //                 name       who       base def  weap cap t00 t32 hp+ hp- mana 
        addf_buf(page, "%-20s " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%3s" COLOR_DECOR_1 "]" COLOR_ALT_TEXT_1 " %-15s %-15s %-10s %-5d %-4d %-4d %-3d %-3d %-5s" COLOR_EOL,
            class_->name, class_->who_name, class_->base_group, 
            class_->default_group, w_name, class_->skill_cap, class_->thac0_00, 
            class_->thac0_32, class_->hp_min, class_->hp_max, 
            (class_->fMana ? COLOR_B_GREEN "Yes" COLOR_CLEAR : COLOR_B_RED "No" COLOR_CLEAR ));
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

    addf_buf(out, COLOR_TITLE "Class Name  " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "%s" COLOR_DECOR_1 "]\n\r", pClass->name);
    addf_buf(out, COLOR_TITLE "Who Abbr.   " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "%s" COLOR_DECOR_1 "]\n\r", pClass->who_name);
    addf_buf(out, COLOR_TITLE "Base Group  " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "%s" COLOR_DECOR_1 "]\n\r", pClass->base_group);
    addf_buf(out, COLOR_TITLE "Default Grp " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "%s" COLOR_DECOR_1 "]\n\r", pClass->default_group);

    int w = get_weapon_index(pClass->weapon);

    if (w > 0)
        addf_buf(out, COLOR_TITLE "Weapon      " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "%s " COLOR_ALT_TEXT_2 "(%d)" COLOR_DECOR_1 "]\n\r", weapon_table[w].name, pClass->weapon);
    else
        addf_buf(out, COLOR_TITLE "Weapon      " COLOR_DECOR_1 ": []\n\r");

    addf_buf(out, COLOR_TITLE "Armor Prof  " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "%s" COLOR_DECOR_1 "]\n\r",
        armor_type_name(armor_type_from_value(pClass->armor_prof)));

    addf_buf(out, COLOR_TITLE "Guild VNUMs " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "");

    for (int i = 0; i < MAX_GUILD; ++i) {
        addf_buf(out, "%d", pClass->guild[i]);
        if (i < MAX_GUILD - 1)
            addf_buf(out, ", ");
    }
    addf_buf(out, COLOR_DECOR_1 "]\n\r");
    addf_buf(out, COLOR_TITLE "Prime Stat  " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "%s" COLOR_DECOR_1 "]\n\r", stat_table[pClass->prime_stat].name);
    addf_buf(out, COLOR_TITLE "Skill Cap   " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "%d" COLOR_DECOR_1 "]\n\r", pClass->skill_cap);
    addf_buf(out, COLOR_TITLE "THAC0 @00   " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "%d" COLOR_DECOR_1 "]\n\r", pClass->thac0_00);
    addf_buf(out, COLOR_TITLE "THAC0 @32   " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "%d" COLOR_DECOR_1 "]\n\r", pClass->thac0_32);
    addf_buf(out, COLOR_TITLE "HP Min/Lvl  " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "%d" COLOR_DECOR_1 "]\n\r", pClass->hp_min);
    addf_buf(out, COLOR_TITLE "HP Max/Lvl  " COLOR_DECOR_1 ": [" COLOR_ALT_TEXT_1 "%d" COLOR_DECOR_1 "]\n\r", pClass->hp_max);
    addf_buf(out, COLOR_TITLE "Mana?       " COLOR_DECOR_1 ": [%s" COLOR_DECOR_1 "]\n\r", (pClass->fMana ? COLOR_B_GREEN "Yes" : COLOR_B_RED "No"));

    addf_buf(out, COLOR_TITLE "Titles      " COLOR_DECOR_1 ":\n\r");
    for (int i = 0; i <= MAX_LEVEL; ++i) {
        const char* m_title = pClass->titles[i][0] ? pClass->titles[i][0] : "";
        const char* f_title = pClass->titles[i][1] ? pClass->titles[i][1] : "";
        if (!m_title[0] && !f_title[0])
            continue;
        addf_buf(out, "    " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%3d " COLOR_DECOR_1 "] " COLOR_ALT_TEXT_1 "%-25s %-25s\n\r", i, m_title, f_title);
    }
    add_buf(out, COLOR_EOL);

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
        send_to_char("Syntax : " COLOR_ALT_TEXT_1 "WEAPON <WEAPON NAME>" COLOR_CLEAR "\n\r\n\r", ch);
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
        send_to_char("Syntax : " COLOR_ALT_TEXT_1 "START_LOC <VNUM>" COLOR_CLEAR "\n\r\n\r", ch);
        return false;
    }

    vnum = (VNUM)atoi(vnum_str);

    if (!get_room_data(vnum)) {
        printf(COLOR_INFO "CEdit : There is no room with VNUM %d." COLOR_EOL, vnum);
        return false;
    }

    class_->start_loc = vnum;
    send_to_char(COLOR_INFO "Ok." COLOR_EOL, ch);
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
        send_to_char("Syntax : " COLOR_ALT_TEXT_1 "GUILD <SLOT> <VNUM>" COLOR_CLEAR "\n\r\n\r", ch);
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
        send_to_char("Syntax : " COLOR_ALT_TEXT_1 "TITLE <LEVEL> <MALE/FEMALE/BOTH> <TITLE>" COLOR_CLEAR "\n\r\n\r", ch);
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
        send_to_char("Syntax : " COLOR_ALT_TEXT_1 "TITLE <LEVEL> <MALE/FEMALE/BOTH> <TITLE>" COLOR_CLEAR "\n\r\n\r", ch);
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
