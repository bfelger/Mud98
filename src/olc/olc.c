/***************************************************************************
 *  File: olc.c                                                            *
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

#include "olc.h"

#include "bit.h"

#include <act_comm.h>
#include <act_move.h>
#include <comm.h>
#include <db.h>
#include <handler.h>
#include <interp.h>
#include <lookup.h>
#include <skills.h>
#include <tables.h>

#include <entities/descriptor.h>
#include <entities/object.h>
#include <entities/player_data.h>
#include <entities/reset.h>

#include <data/mobile_data.h>
#include <data/race.h>
#include <data/skill.h>
#include <data/social.h>

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void UpdateOLCScreen(Descriptor*);

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

void set_editor(Descriptor* d, int editor, uintptr_t param)
{
    d->editor = (int16_t)editor;
    d->pEdit = param;
    if (d->page < 1)
        d->page = 1;
    InitScreen(d);
}

/* Executed from comm.c.  Minimizes compiling when changes are made. */
bool run_olc_editor(Descriptor* d, char* incomm)
{
    switch (d->editor) {
    case ED_AREA:
        aedit(d->character, incomm);
        break;
    case ED_ROOM:
        redit(d->character, incomm);
        break;
    case ED_OBJECT:
        oedit(d->character, incomm);
        break;
    case ED_MOBILE:
        medit(d->character, incomm);
        break;
    case ED_PROG:
        pedit(d->character, incomm);
        break;
    case ED_RACE:
        raedit(d->character, incomm);
        break;
    case ED_SOCIAL:
        sedit(d->character, incomm);
        break;
    case ED_SKILL:
        skedit(d->character, incomm);
        break;
    case ED_CMD:
        cmdedit(d->character, incomm);
        break;
    case ED_GROUP:
        gedit(d->character, incomm);
        break;
    case ED_HELP:
        hedit(d->character, incomm);
        break;
    case ED_CLASS:
        cedit(d->character, incomm);
        break;
    case ED_QUEST:
        qedit(d->character, incomm);
        break;
    default:
        return false;
    }
    return true;
}

char* olc_ed_name(Mobile* ch)
{
    static char buf[10];

    buf[0] = '\0';
    switch (ch->desc->editor) {
    case ED_AREA:
        sprintf(buf, "AEdit");
        break;
    case ED_ROOM:
        sprintf(buf, "REdit");
        break;
    case ED_OBJECT:
        sprintf(buf, "OEdit");
        break;
    case ED_MOBILE:
        sprintf(buf, "MEdit");
        break;
    case ED_PROG:
        sprintf(buf, "MPEdit");
        break;
    case ED_RACE:
        sprintf(buf, "RAEdit");
        break;
    case ED_SOCIAL:
        sprintf(buf, "SEdit");
        break;
    case ED_SKILL:
        sprintf(buf, "SKEdit");
        break;
    case ED_CMD:
        sprintf(buf, "CMDEdit");
        break;
    case ED_CLASS:
        sprintf(buf, "CEdit");
        break;
    case ED_GROUP:
        sprintf(buf, "GEdit");
        break;
    case ED_HELP:
        sprintf(buf, "HEdit");
        break;
    case ED_QUEST:
        sprintf(buf, "QEdit");
        break;
    default:
        sprintf(buf, " ");
        break;
    }
    return buf;
}


char* olc_ed_vnum(Mobile* ch)
{
    AreaData* area;
    Room* pRoom;
    ObjPrototype* pObj;
    MobPrototype* pMob;
    MobProgCode* pMcode;
    HelpData* pHelp;
    Race* pRace;
    Social* pSocial;
    Skill* pSkill;
    Class* pClass;
    CmdInfo* pCmd;
    Quest* pQuest;
    static char buf[10];

    buf[0] = '\0';
    switch (ch->desc->editor) {
    case ED_AREA:
        area = (AreaData*)ch->desc->pEdit;
        sprintf(buf, "%"PRVNUM, area ? VNUM_FIELD(area) : 0);
        break;
    case ED_ROOM:
        pRoom = ch->in_room;
        sprintf(buf, "%"PRVNUM, pRoom ? VNUM_FIELD(pRoom) : 0);
        break;
    case ED_OBJECT:
        pObj = (ObjPrototype*)ch->desc->pEdit;
        sprintf(buf, "%"PRVNUM, pObj ? VNUM_FIELD(pObj) : 0);
        break;
    case ED_MOBILE:
        pMob = (MobPrototype*)ch->desc->pEdit;
        sprintf(buf, "%"PRVNUM, pMob ? VNUM_FIELD(pMob) : 0);
        break;
    case ED_PROG:
        pMcode = (MobProgCode*)ch->desc->pEdit;
        sprintf(buf, "%"PRVNUM, pMcode ? pMcode->vnum : 0);
        break;
    case ED_QUEST:
        pQuest = (Quest*)ch->desc->pEdit;
        sprintf(buf, "%d", pQuest ? pQuest->vnum : 0);
        break;
    case ED_RACE:
        pRace = (Race*)ch->desc->pEdit;
        sprintf(buf, "%s", pRace ? pRace->name : "");
        break;
    case ED_SOCIAL:
        pSocial = (Social*)ch->desc->pEdit;
        sprintf(buf, "%s", pSocial ? pSocial->name : "");
        break;
    case ED_SKILL:
        pSkill = (Skill*)ch->desc->pEdit;
        sprintf(buf, "%s", pSkill ? pSkill->name : "");
        break;
    case ED_CMD:
        pCmd = (CmdInfo*)ch->desc->pEdit;
        sprintf(buf, "%s", pCmd ? pCmd->name : "");
        break;
    case ED_HELP:
        pHelp = (HelpData*)ch->desc->pEdit;
        sprintf(buf, "%s", pHelp ? pHelp->keyword : "");
        break;
    case ED_CLASS:
        pClass = (Class*)ch->desc->pEdit;
        sprintf(buf, "%s", pClass ? pClass->name : "");
        break;
    default:
        sprintf(buf, " ");
        break;
    }

    return buf;
}

const OlcCmdEntry* get_olc_table(int editor)
{
    switch (editor) {
    case ED_AREA:   return area_olc_comm_table;
    case ED_MOBILE:	return mob_olc_comm_table;
    case ED_OBJECT:	return obj_olc_comm_table;
    case ED_ROOM:	return room_olc_comm_table;
    case ED_SKILL:	return skill_olc_comm_table;
    case ED_RACE:	return race_olc_comm_table;
    case ED_CMD:	return cmd_olc_comm_table;
    case ED_PROG:	return prog_olc_comm_table;
    case ED_SOCIAL:	return social_olc_comm_table;
    case ED_CLASS:  return class_olc_comm_table;
    case ED_QUEST:  return quest_olc_comm_table;
    }
    return NULL;
}

/*****************************************************************************
 Name:		show_olc_cmds
 Purpose:	Format up the commands from given table.
 Called by:	show_commands(olc_act.c).
 ****************************************************************************/
void show_olc_cmds(Mobile* ch)
{
    char    buf[MAX_STRING_LENGTH] = "";
    char    buf1[MAX_STRING_LENGTH] = "";
    const OlcCmdEntry* table;
    int     col;

    buf1[0] = '\0';
    col = 0;

    //if (ch->desc->editor == ED_AREA) {
    //    // Areas have a cmd_type, not a comm_type
    //    for (int cmd = 0; aedit_table[cmd].name != NULL; cmd++) {
    //        sprintf(buf, "%-15.15s", aedit_table[cmd].name);
    //        strcat(buf1, buf);
    //        if (++col % 5 == 0)
    //            strcat(buf1, "\n\r");
    //    }
    //
    //    if (col % 5 != 0)
    //        strcat(buf1, "\n\r");
    //
    //    send_to_char(buf1, ch);
    //    return;
    //}

    table = get_olc_table(ch->desc->editor);

    if (table == NULL) {
        bugf("slow_olc_cmds : NULL table, editor %d",
            ch->desc->editor);
        return;
    }

    for (int cmd = 0; table[cmd].name != NULL; cmd++) {
        sprintf(buf, "%-15.15s", table[cmd].name);
        strcat(buf1, buf);
        if (++col % 5 == 0)
            strcat(buf1, "\n\r");
    }

    if (col % 5 != 0)
        strcat(buf1, "\n\r");

    send_to_char(buf1, ch);
    return;
}

/*****************************************************************************
 Name:		show_commands
 Purpose:	Display all olc commands.
 Called by:	olc interpreters.
 ****************************************************************************/
bool show_commands(Mobile* ch, char* argument)
{
    show_olc_cmds(ch);

    return false;
}

/*****************************************************************************
 Name:		edit_done
 Purpose:	Resets builder information on completion.
 Called by:	aedit, redit, oedit, medit(olc.c)
 ****************************************************************************/
bool edit_done(Mobile* ch)
{
    if (ch->desc->editor != ED_NONE)
        send_to_char("Exiting the editor.\n\r", ch);
    ch->desc->pEdit = 0;
    ch->desc->editor = ED_NONE;
    ch->desc->page = 0;
    if (IS_SET(ch->comm_flags, COMM_OLCX)) {
        do_clear(ch, "reset");
        InitScreen(ch->desc);
    }
    return false;
}

/*****************************************************************************
 *                              Interpreters.                                *
 *****************************************************************************/

const EditCmd editor_table[] =
{
//  	Command		Function
   {	"area",		do_aedit	},
   {	"room",		do_redit	},
   {	"object",	do_oedit	},
   {	"mobile",	do_medit	},
   {	"program",	do_pedit	},
   {	"race",		do_raedit	},
   {	"social",	do_sedit	},
   {	"skill",	do_skedit	},
   {	"command",	do_cmdedit	},
   {	"group",	do_gedit	},
   {    "class",    do_cedit    },
   {	"help",		do_hedit	},
   {	"quest",    do_qedit	},
   {	NULL,		0		    }
};

/* Entry point for all editors. */
void do_olc(Mobile* ch, char* argument)
{
    char command[MAX_INPUT_LENGTH];
    int cmd;

    READ_ARG(command);

    if (command[0] == '\0') {
        do_help(ch, "olc");
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; editor_table[cmd].name != NULL; cmd++) {
        if (!str_prefix(command, editor_table[cmd].name)) {
            (*editor_table[cmd].do_fun) (ch, argument);
            return;
        }
    }

    /* Invalid command, send help. */
    do_help(ch, "olc");
    return;
}

bool process_olc_command(Mobile* ch, char* argument, const OlcCmdEntry* table)
{
    char arg[MIL];
    AreaData* area;
    MobPrototype* pMob;
    ObjPrototype* pObj;
    RoomData* pRoom;
    Race* pRace;
    Skill* pSkill;
    CmdInfo* pCmd;
    Class* pClass;
    AreaData* tArea;
    MobProgCode* pProg;
    Social* pSoc;
    Quest* pQuest;
    int temp;
    uintptr_t pointer;

    READ_ARG(arg);

    for (temp = 0; table[temp].name; temp++) {
        if (LOWER(arg[0]) == LOWER(table[temp].name[0])
            && !str_prefix(arg, table[temp].name)) {
            switch (ch->desc->editor) {
            case ED_AREA:
                EDIT_AREA(ch, area);
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xArea) + U(area));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter)
                    && area)
                    SET_BIT(area->area_flags, AREA_CHANGED);
                return true;
                break;

            case ED_MOBILE:
                EDIT_MOB(ch, pMob);
                tArea = pMob->area;
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xMob) + U(pMob));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter)
                    && tArea)
                    SET_BIT(tArea->area_flags, AREA_CHANGED);
                return true;
                break;

            case ED_OBJECT:
                EDIT_OBJ(ch, pObj);
                tArea = pObj->area;
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xObj) + U(pObj));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter)
                    && tArea != NULL)
                    SET_BIT(tArea->area_flags, AREA_CHANGED);
                return true;
                break;

            case ED_ROOM:
                EDIT_ROOM(ch, pRoom);
                tArea = pRoom->area_data;
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xRoom) + U(pRoom));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter)
                    && tArea != NULL)
                    SET_BIT(tArea->area_flags, AREA_CHANGED);
                return true;
                break;

            case ED_QUEST:
                EDIT_QUEST(ch, pQuest);
                tArea = pQuest->area_data;
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xQuest) + U(pQuest));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter)
                    && tArea != NULL)
                    SET_BIT(tArea->area_flags, AREA_CHANGED);
                return true;
                break;

            case ED_SKILL:
                EDIT_SKILL(ch, pSkill);
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xSkill) + U(pSkill));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter))
                    save_skill_table();
                return true;
                break;

            case ED_RACE:
                EDIT_RACE(ch, pRace);
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xRace) + U(pRace));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter))
                    save_race_table();
                return true;
                break;

            case ED_CLASS:
                EDIT_CLASS(ch, pClass);
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xClass) + U(pClass));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter))
                    save_class_table();
                return true;
                break;

            case ED_PROG:
                EDIT_PROG(ch, pProg);
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xProg) + U(pProg));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter))
                    pProg->changed = true;
                return true;
                break;

            case ED_CMD:
                EDIT_CMD(ch, pCmd);
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xCmd) + U(pCmd));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter))
                    save_command_table();
                return true;
                break;

            case ED_SOCIAL:
                EDIT_SOCIAL(ch, pSoc);
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xSoc) + U(pSoc));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter))
                    save_social_table();
                return true;
                break;
            }
        }
    }

    return false;
}

void do_page(Mobile* ch, char* argument)
{
    int16_t num;

    if (IS_NPC(ch)
        || ch->desc == NULL
        || ch->desc->editor == ED_NONE) {
        send_to_char("You are not in an editor.\n\r", ch);
        return;
    }

    if (!is_number(argument)) {
        send_to_char("How large will you make your scroll?\n\r", ch);
        return;
    }

    num = (int16_t)atoi(argument);

    if (num <= 0) {
        send_to_char("That's a strange-looking number.\n\r", ch);
        return;
    }

    ch->desc->page = num;

    InitScreen(ch->desc);
    UpdateOLCScreen(ch->desc);

    send_to_char("Changed editor scroll. If you don't see anything, change to another number.\n\r", ch);
    return;
}

#define LABEL_FMT "%-14s"

void olc_print_flags(Mobile* ch, const char* label, const struct flag_type* flag_table, FLAGS flags)
{
    printf_to_char(ch, LABEL_FMT " : " COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 "%10s"
        COLOR_DECOR_1 " ]"  COLOR_CLEAR "\n\r", label, 
        flag_string(flag_table, flags));
}

void olc_print_num(Mobile* ch, const char* label, int num)
{
    printf_to_char(ch, LABEL_FMT " : " COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 "%10d"
        COLOR_DECOR_1 " ]" COLOR_CLEAR "\n\r", label, num);
}

void olc_print_range(Mobile* ch, const char* label, int num1, int num2)
{
    char buf[MIL];
    sprintf(buf, "%d-%d", num1, num2);
    printf_to_char(ch, LABEL_FMT " : "  COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1
        "%10s" COLOR_DECOR_1 " ]" COLOR_CLEAR "\n\r", label, buf);
}

void olc_print_num_str(Mobile* ch, const char* label, int num, const char* opt_str)
{
    if (opt_str == NULL)
        printf_to_char(ch, LABEL_FMT " : " COLOR_DECOR_1 "[ "
            COLOR_ALT_TEXT_1 "%10d" COLOR_DECOR_1 " ]" COLOR_CLEAR "\n\r",
            label, num);
    else
        printf_to_char(ch, LABEL_FMT " : " COLOR_DECOR_1 "[ "
            COLOR_ALT_TEXT_1 "%10d" COLOR_DECOR_1 " ] " COLOR_ALT_TEXT_2 "%s"
            COLOR_CLEAR "\n\r", label, num, opt_str);
}

void olc_print_str(Mobile* ch, const char* label, const char* str)
{
    printf_to_char(ch, LABEL_FMT " : " COLOR_ALT_TEXT_1 "%s" COLOR_CLEAR "\n\r",
        label, str);
}

void olc_print_str_box(Mobile* ch, const char* label, const char* str, 
    const char* opt_str)
{
    printf_to_char(ch, LABEL_FMT " : " COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 "%10s"
        COLOR_DECOR_1 " ] " COLOR_ALT_TEXT_2 "%s" COLOR_CLEAR "\n\r", label,
        str, opt_str);
}

void olc_print_yesno(Mobile* ch, const char* label, bool yesno)
{
    // Add space for color codes!
    printf_to_char(ch, LABEL_FMT " : "  COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 
        "%12s" COLOR_DECOR_1 " ]" COLOR_CLEAR "\n\r", label, 
        yesno ? COLOR_B_GREEN "YES" : COLOR_B_RED "NO");
}

void olc_print_text(Mobile* ch, const char* label, const char* text)
{
    if (text && text[0]) {
        printf_to_char(ch, "%s: \n\r" COLOR_ALT_TEXT_2 "%s" COLOR_CLEAR, label, text);
        char c = text[strlen(text) - 1];
        if (c != '\n' && c != '\r')
            printf_to_char(ch, "\n\r");
    }
    else
        printf_to_char(ch, LABEL_FMT " : " COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 
            "%10s" COLOR_DECOR_1 " ]" COLOR_CLEAR "\n\r", label, "(none)");
}

const char* olc_show_flags(const char* label, const struct flag_type* flag_table, FLAGS flags)
{
    char label_buf[MIL];
    static char buf[MSL];

    sprintf(label_buf, "%s:", label);
    sprintf(buf, "%12s " COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 "%10s" 
        COLOR_DECOR_1 " ]" COLOR_CLEAR, label_buf, 
        flag_string(flag_table, flags));

    return buf;
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
