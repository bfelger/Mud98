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
#include "editor_stack.h"
#include "loot_edit.h"

#include <craft/recedit.h>

#include <act_comm.h>
#include <act_move.h>
#include <comm.h>
#include <db.h>
#include <handler.h>
#include <interp.h>
#include <lookup.h>
#include <skills.h>
#include <tables.h>
#include <persist/command/command_persist.h>

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

#if  defined(__GNUC__) || defined(__clang__)
#  define UNUSED __attribute__((unused))
#else
#  define UNUSED /**/
#endif

void UpdateOLCScreen(Descriptor*);
extern const OlcCmd theme_edit_table[];

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

////////////////////////////////////////////////////////////////////////////////
// Editor Stack API
////////////////////////////////////////////////////////////////////////////////

void set_editor(Descriptor* d, EditorType editor, uintptr_t pEdit)
{
    // Clear stack and push the new editor as the base
    editor_stack_clear(&d->editor_stack);
    editor_stack_push(&d->editor_stack, editor, pEdit);
    if (d->page < 1)
        d->page = 1;
    InitScreen(d);
}

void push_editor(Descriptor* d, EditorType ed_type, uintptr_t pEdit)
{
    editor_stack_push(&d->editor_stack, ed_type, pEdit);
}

bool pop_editor(Descriptor* d)
{
    return editor_stack_pop(&d->editor_stack);
}

EditorType get_editor(Descriptor* d)
{
    EditorFrame* frame = editor_stack_top(&d->editor_stack);
    return frame ? frame->editor : ED_NONE;
}

uintptr_t get_pEdit(Descriptor* d)
{
    EditorFrame* frame = editor_stack_top(&d->editor_stack);
    return frame ? frame->pEdit : 0;
}

bool set_pEdit(Descriptor* d, uintptr_t pEdit)
{
    return editor_stack_set_pEdit(&d->editor_stack, pEdit);
}

bool has_parent_editor(Descriptor* d)
{
    return editor_stack_depth(&d->editor_stack) > 1;
}

bool in_editor(Descriptor* d)
{
    return !editor_stack_empty(&d->editor_stack);
}

int editor_depth(Descriptor* d)
{
    return editor_stack_depth(&d->editor_stack);
}

bool in_string_editor(Descriptor* d)
{
    return get_editor(d) == ED_STRING;
}

bool in_lox_editor(Descriptor* d)
{
    return get_editor(d) == ED_LOX_SCRIPT;
}

char** get_string_pEdit(Descriptor* d)
{
    if (get_editor(d) != ED_STRING) return NULL;
    return (char**)get_pEdit(d);
}

ObjString* get_lox_pEdit(Descriptor* d)
{
    if (get_editor(d) != ED_LOX_SCRIPT) return NULL;
    return (ObjString*)get_pEdit(d);
}

void set_lox_pEdit(Descriptor* d, ObjString* script)
{
    if (get_editor(d) != ED_LOX_SCRIPT) {
        bug("set_lox_pEdit: not in ED_LOX_SCRIPT mode");
        return;
    }
    set_pEdit(d, (uintptr_t)script);
}

/* Executed from comm.c.  Minimizes compiling when changes are made. */
bool run_olc_editor(Descriptor* d, char* incomm)
{
    int16_t editor = get_editor(d);
    
    switch (editor) {
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
    case ED_THEME:
        theme_edit(d->character, incomm);
        break;
    case ED_TUTORIAL:
        tedit(d->character, incomm);
        break;
    case ED_SCRIPT:
        scredit(d->character, incomm);
        break;
    case ED_LOOT:
        ledit(d->character, incomm);
        break;
    case ED_LOOT_GROUP:
        loot_group_edit(d->character, incomm);
        break;
    case ED_LOOT_TABLE:
        loot_table_edit(d->character, incomm);
        break;
    case ED_RECIPE:
        recedit(d->character, incomm);
        break;
    default:
        return false;
    }
    return true;
}

char* olc_ed_name(Mobile* ch)
{
    static char buf[64];
    buf[0] = '\0';
    
    int depth = editor_depth(ch->desc);
    if (depth == 0) {
        sprintf(buf, " ");
        return buf;
    }
    
    // Build name from bottom of stack to top (e.g., "AEdit/Loot/Table")
    for (int i = 0; i < depth; i++) {
        EditorFrame* frame = editor_stack_at(&ch->desc->editor_stack, i);
        if (!frame) continue;
        
        if (i > 0) strcat(buf, "/");
        
        switch (frame->editor) {
        case ED_AREA:     strcat(buf, "AEdit"); break;
        case ED_ROOM:     strcat(buf, "REdit"); break;
        case ED_OBJECT:   strcat(buf, "OEdit"); break;
        case ED_MOBILE:   strcat(buf, "MEdit"); break;
        case ED_PROG:     strcat(buf, "MPEdit"); break;
        case ED_RACE:     strcat(buf, "RAEdit"); break;
        case ED_SOCIAL:   strcat(buf, "SEdit"); break;
        case ED_SKILL:    strcat(buf, "SKEdit"); break;
        case ED_CMD:      strcat(buf, "CMDEdit"); break;
        case ED_CLASS:    strcat(buf, "CEdit"); break;
        case ED_GROUP:    strcat(buf, "GEdit"); break;
        case ED_HELP:     strcat(buf, "HEdit"); break;
        case ED_QUEST:    strcat(buf, "QEdit"); break;
        case ED_THEME:    strcat(buf, "ThemeEd"); break;
        case ED_TUTORIAL: strcat(buf, "TEdit"); break;
        case ED_SCRIPT:   strcat(buf, "ScrEdit"); break;
        case ED_LOOT:     strcat(buf, "LEdit"); break;
        case ED_LOOT_GROUP: strcat(buf, "LGrp"); break;
        case ED_LOOT_TABLE: strcat(buf, "LTbl"); break;
        case ED_RECIPE:   strcat(buf, "RecEdit"); break;
        case ED_STRING:   strcat(buf, "Str"); break;
        case ED_LOX_SCRIPT: strcat(buf, "Lox"); break;
        default:          strcat(buf, "?"); break;
        }
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
    Tutorial* pTutorial;
    LoxScriptEntry* script;
    static char buf[10];

    int16_t editor = get_editor(ch->desc);
    uintptr_t pEdit = get_pEdit(ch->desc);

    buf[0] = '\0';
    switch (editor) {
    case ED_AREA:
        area = (AreaData*)pEdit;
        sprintf(buf, "%"PRVNUM, area ? VNUM_FIELD(area) : 0);
        break;
    case ED_ROOM:
        pRoom = ch->in_room;
        sprintf(buf, "%"PRVNUM, pRoom ? VNUM_FIELD(pRoom) : 0);
        break;
    case ED_OBJECT:
        pObj = (ObjPrototype*)pEdit;
        sprintf(buf, "%"PRVNUM, pObj ? VNUM_FIELD(pObj) : 0);
        break;
    case ED_MOBILE:
        pMob = (MobPrototype*)pEdit;
        sprintf(buf, "%"PRVNUM, pMob ? VNUM_FIELD(pMob) : 0);
        break;
    case ED_PROG:
        pMcode = (MobProgCode*)pEdit;
        sprintf(buf, "%"PRVNUM, pMcode ? pMcode->vnum : 0);
        break;
    case ED_QUEST:
        pQuest = (Quest*)pEdit;
        sprintf(buf, "%d", pQuest ? VNUM_FIELD(pQuest) : 0);
        break;
    case ED_RACE:
        pRace = (Race*)pEdit;
        sprintf(buf, "%s", pRace ? pRace->name : "");
        break;
    case ED_SOCIAL:
        pSocial = (Social*)pEdit;
        sprintf(buf, "%s", pSocial ? pSocial->name : "");
        break;
    case ED_SKILL:
        pSkill = (Skill*)pEdit;
        sprintf(buf, "%s", pSkill ? pSkill->name : "");
        break;
    case ED_CMD:
        pCmd = (CmdInfo*)pEdit;
        sprintf(buf, "%s", pCmd ? pCmd->name : "");
        break;
    case ED_HELP:
        pHelp = (HelpData*)pEdit;
        sprintf(buf, "%s", pHelp ? pHelp->keyword : "");
        break;
    case ED_CLASS:
        pClass = (Class*)pEdit;
        sprintf(buf, "%s", pClass ? pClass->name : "");
        break;
    case ED_TUTORIAL:
        pTutorial = (Tutorial*)pEdit;
        sprintf(buf, "%s", pTutorial ? pTutorial->name : "");
        break;
    case ED_SCRIPT:
        script = lox_script_entry_get((size_t)pEdit);
        if (script)
            sprintf(buf, "%zu", (size_t)pEdit);
        else
            sprintf(buf, " ");
        break;
    case ED_LOOT:
        sprintf(buf, "global");
        break;
    default:
        sprintf(buf, " ");
        break;
    }

    return buf;
}

static const OlcCmdEntry* get_olc_table(int editor)
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
    case ED_TUTORIAL: return tutorial_olc_comm_table;
    }
    return NULL;
}

/*****************************************************************************
 Name:		show_olc_cmds
 Purpose:	Format up the commands from given table.
 Called by:	show_commands(olc_act.c).
 ****************************************************************************/
static void show_olc_cmds(Mobile* ch)
{
    char    buf[MAX_STRING_LENGTH] = "";
    char    buf1[MAX_STRING_LENGTH] = "";
    const OlcCmdEntry* table;
    int     col;

    buf1[0] = '\0';
    col = 0;

    if (get_editor(ch->desc) == ED_THEME) {
        for (int cmd = 0; theme_edit_table[cmd].name != NULL; cmd++) {
            sprintf(buf, "%-15.15s", theme_edit_table[cmd].name);
            strcat(buf1, buf);
            if (++col % 5 == 0)
                strcat(buf1, "\n\r");
        }
        if (col % 5 != 0)
            strcat(buf1, "\n\r");
        send_to_char(buf1, ch);
        return;
    }

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

    table = get_olc_table(get_editor(ch->desc));

    if (table == NULL) {
        bugf("slow_olc_cmds : NULL table, editor %d",
            get_editor(ch->desc));
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
    // Pop one level from the editor stack
    if (has_parent_editor(ch->desc)) {
        // There's a parent editor to return to
        pop_editor(ch->desc);
        send_to_char("Returning to parent editor.\n\r", ch);
        return false;
    }

    // No parent - exiting OLC entirely
    if (in_editor(ch->desc))
        send_to_char("Exiting the editor.\n\r", ch);
    editor_stack_clear(&ch->desc->editor_stack);
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
   {    "loot",     do_ledit    },
   {	"quest",    do_qedit	},
   {    "script",   do_scredit  },
   {    "tutorial", do_tedit    },
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
    Tutorial* pTutorial;
    int temp;
    uintptr_t pointer;

    READ_ARG(arg);

    for (temp = 0; table[temp].name; temp++) {
        if (LOWER(arg[0]) == LOWER(table[temp].name[0])
            && !str_prefix(arg, table[temp].name)) {
            switch (get_editor(ch->desc)) {
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
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter)) {
                    PersistResult res = command_persist_save(NULL);
                    if (!persist_succeeded(res))
                        bugf("CMDEdit: failed to save command table (%s)",
                            res.message ? res.message : "unknown error");
                }
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

            case ED_TUTORIAL:
                EDIT_TUTORIAL(ch, pTutorial);
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xTutorial) + U(pTutorial));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter))
                    save_tutorials();
                return true;
                break;

            case ED_RECIPE: {
                Recipe* pRecipe;
                EDIT_RECIPE(ch, pRecipe);
                tArea = pRecipe ? pRecipe->area : NULL;
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xRecipe) + U(pRecipe));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter)
                    && tArea != NULL)
                    SET_BIT(tArea->area_flags, AREA_CHANGED);
                return true;
            }

            default:
                break;
            }
        }
    }

    return false;
}

UNUSED
static void do_page(Mobile* ch, char* argument)
{
    int16_t num;

    if (IS_NPC(ch)
        || ch->desc == NULL
        || get_editor(ch->desc) == ED_NONE) {
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
        COLOR_DECOR_1 " ]"  COLOR_EOL, label, 
        flag_string(flag_table, flags));
}

void olc_print_flags_ex(Mobile* ch, const char* label, const struct flag_type* flag_table, const struct flag_type* defaults, FLAGS flags)
{
    const char* flag_str = flag_string(flag_table, flags);

    printf_to_char(ch, LABEL_FMT" : " COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 "%10s"
        COLOR_DECOR_1 " ]", label, flag_str);

    const char* preset = olc_match_flag_default(flags, defaults);
    if (preset) {
        if (strlen(label) + strlen(flag_str) > 68) {
            // Too long, move preset to next line
            printf_to_char(ch, "\n\r                ");
        }
        printf_to_char(ch, " " COLOR_ALT_TEXT_2 "(%s)", preset);
    }

    printf_to_char(ch, COLOR_EOL);
}

void olc_print_num(Mobile* ch, const char* label, int num)
{
    printf_to_char(ch, LABEL_FMT " : " COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 "%10d"
        COLOR_DECOR_1 " ]" COLOR_EOL, label, num);
}

void olc_print_range(Mobile* ch, const char* label, int num1, int num2)
{
    char buf[MIL];
    sprintf(buf, "%d-%d", num1, num2);
    printf_to_char(ch, LABEL_FMT " : "  COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1
        "%10s" COLOR_DECOR_1 " ]" COLOR_EOL, label, buf);
}

void olc_print_num_str(Mobile* ch, const char* label, int num, const char* opt_str)
{
    if (opt_str == NULL)
        printf_to_char(ch, LABEL_FMT " : " COLOR_DECOR_1 "[ "
            COLOR_ALT_TEXT_1 "%10d" COLOR_DECOR_1 " ]" COLOR_EOL,
            label, num);
    else
        printf_to_char(ch, LABEL_FMT " : " COLOR_DECOR_1 "[ "
            COLOR_ALT_TEXT_1 "%10d" COLOR_DECOR_1 " ] " COLOR_ALT_TEXT_2 "%s"
            COLOR_EOL, label, num, opt_str);
}

void olc_print_str(Mobile* ch, const char* label, const char* str)
{
    printf_to_char(ch, LABEL_FMT " : " COLOR_ALT_TEXT_1 "%s" COLOR_EOL,
        label, str);
}

void olc_print_str_box(Mobile* ch, const char* label, const char* str, 
    const char* opt_str)
{
    printf_to_char(ch, LABEL_FMT " : " COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 "%10s"
        COLOR_DECOR_1 " ] " COLOR_ALT_TEXT_2 "%s" COLOR_EOL, label,
        str, opt_str ? opt_str : "");
}

void olc_print_yesno_ex(Mobile* ch, const char* label, bool yesno, const char* msg)
{
    printf_to_char(ch, LABEL_FMT " : "  COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 
        "%12s" COLOR_DECOR_1 " ] " COLOR_ALT_TEXT_2 "%s" COLOR_EOL, label, 
        yesno ? COLOR_B_GREEN "YES" : COLOR_B_RED "NO", msg);
}

void olc_print_yesno(Mobile* ch, const char* label, bool yesno)
{
    // Add space for color codes!
    printf_to_char(ch, LABEL_FMT " : "  COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 
        "%12s" COLOR_DECOR_1 " ]" COLOR_EOL, label, 
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
            "%10s" COLOR_DECOR_1 " ]" COLOR_EOL, label, "(none)");
}

static void shift_string(char* str, int start, int end)
{
    for (int i = end; i > start; --i) {
        str[i] = str[i - 1];
    }
}

const char* olc_inline_text(const char* str, int width)
{
    // Keep it on one line by a specified width
    static char buf[MIL];

    int len = (int)strlen(str);
    if (width > len)
        width = len;

    memcpy(buf, str, width);
    buf[width] = '\0';

    for (int i = 0; i < width; ++i) {
        if (buf[i] == '\n') {
            if (i == width - 1) {
                // If the last character is a newline, cut it off
                buf[i] = '\0';
                break;
            }
            shift_string(buf, i + 1, width++);
            buf[i++] = '^';
            buf[i++] = '^';
            if (buf[i] == '\r') {
                buf[i] = '/';
            }
            else {
                // Bare newline; shift the rest of the string right one char
                shift_string(buf, i + 1, width++);
                buf[i] = '/';
            }
        }
    }

    buf[width] = '\0';

    return buf;
}

void olc_print_text_ex(Mobile* ch, const char* label, const char* str, int width)
{
    printf_to_char(ch, LABEL_FMT " : " COLOR_ALT_TEXT_2 "%s" COLOR_EOL,
        label, olc_inline_text(str, width));
}

const char* olc_match_flag_default(FLAGS flags, const struct flag_type* defaults)
{
    if (!defaults)
        return NULL;
    for (int i = 0; defaults[i].name != NULL; ++i) {
        FLAGS mask = defaults[i].bit;
        if (mask != 0 && (flags & mask) == mask)
            return defaults[i].name;
    }
    return NULL;
}

const char* olc_show_flags_ex(const char* label, const struct flag_type* flag_table, const struct flag_type* defaults, FLAGS flags)
{
    char label_buf[MIL];
    static char buf[MSL];

    sprintf(label_buf, "%s:", label);
    sprintf(buf, "%12s " COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 "%10s"
        COLOR_DECOR_1 " ]" COLOR_CLEAR, label_buf,
        flag_string(flag_table, flags));

    const char* preset = olc_match_flag_default(flags, defaults);
    if (preset) {
        if (strlen(buf) > 70) {
            // Too long, move preset to next line
            strcat(buf, "\n\r            ");
        }
        strcat(buf, " " COLOR_ALT_TEXT_2 "(" COLOR_ALT_TEXT_1);
        strcat(buf, preset);
        strcat(buf, COLOR_ALT_TEXT_2 ")");
        strcat(buf, COLOR_CLEAR);
    }

    return buf;
}

const char* olc_show_flags(const char* label, const struct flag_type* flag_table, FLAGS flags)
{
    return olc_show_flags_ex(label, flag_table, NULL, flags);
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
