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

#include "act_move.h"
#include "bit.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interp.h"
#include "lookup.h"
#include "skills.h"
#include "tables.h"

#include "entities/descriptor.h"
#include "entities/object_data.h"
#include "entities/player_data.h"
#include "entities/reset_data.h"

#include "data/mobile.h"

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Local functions.
 */
AreaData* get_area_data args((VNUM vnum));

COMMAND(do_clear)
COMMAND(do_purge)

void UpdateOLCScreen(Descriptor*);

MobPrototype		xMob;
ObjectPrototype		xObj;
RoomData		xRoom;
struct	skill_type	xSkill;
struct	race_type	xRace;
MPROG_CODE		    xProg;
struct	cmd_type	xCmd;
struct	social_type	xSoc;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const struct olc_comm_type mob_olc_comm_table[] = {
    { "name",	    U(&xMob.name),          ed_line_string,		0		        },
    { "short",	    U(&xMob.short_descr),	ed_line_string,		0		        },
    { "long",	    U(&xMob.long_descr),	ed_line_string,		U(1)	        },
    { "material",	U(&xMob.material),	    ed_line_string,		0		        },
    { "desc",	    U(&xMob.description),	ed_desc,		    0		        },
    { "level",	    U(&xMob.level),		    ed_number_level,    0		        },
    { "align",	    U(&xMob.alignment),	    ed_number_align,	0		        },
    { "group",	    U(&xMob.group),		    ed_olded,		    U(medit_group)	},
    { "imm",	    U(&xMob.imm_flags),	    ed_flag_toggle,		U(imm_flag_table)	},
    { "res",	    U(&xMob.res_flags),	    ed_flag_toggle,		U(res_flag_table)	},
    { "vuln",	    U(&xMob.vuln_flags),	ed_flag_toggle,		U(vuln_flag_table)	},
    { "act",	    U(&xMob.act_flags),		ed_flag_toggle,		U(act_flag_table)	},
    { "affect",	    U(&xMob.affect_flags),	ed_flag_toggle,		U(affect_flag_table)},
    { "off",	    U(&xMob.atk_flags),	    ed_flag_toggle,		U(off_flag_table)	},
    { "form",	    U(&xMob.form),		    ed_flag_toggle,		U(form_flag_table)	},
    { "parts",	    U(&xMob.parts),		    ed_flag_toggle,		U(part_flag_table)	},
    { "shop",	    U(&xMob),			    ed_shop,		    0		        },
    { "create",	    0,				        ed_new_mob,		    0		        },
    { "spec",	    U(&xMob.spec_fun),	    ed_gamespec,		0		        },
    { "recval",	    U(&xMob),			    ed_recval,		    0		        },
    { "sex",	    U(&xMob.sex),		    ed_int16lookup,		U(sex_lookup)	},
    { "size",	    U(&xMob.size),		    ed_int16lookup,		U(size_lookup)	},
    { "startpos",	U(&xMob.start_pos),	    ed_int16lookup,		U(position_lookup)},
    { "defaultpos", U(&xMob.default_pos),	ed_int16lookup,		U(position_lookup)},
    { "damtype",	U(&xMob.dam_type),	    ed_int16poslookup,	U(attack_lookup)},
    { "race",	    U(&xMob),			    ed_race,		    0		        },
    { "armor",	    U(&xMob),			    ed_ac,			    0		        },
    { "hitdice",	U(&xMob.hit[0]),		ed_dice,		    0		        },
    { "manadice",	U(&xMob.mana[0]),		ed_dice,		    0		        },
    { "damdice",	U(&xMob.damage[0]),	    ed_dice,		    0		        },
    { "hitroll",	U(&xMob.hitroll),		ed_number_s_pos,	0		        },
    { "wealth",	    U(&xMob.wealth),		ed_number_l_pos,	0		        },
    { "addprog",	U(&xMob.mprogs),		ed_addprog,		    0		        },
    { "delprog",	U(&xMob.mprogs),		ed_delprog,		    0		        },
    { "mshow",	    0,				        ed_olded,		    U(medit_show)	},
    { "oshow",	    0,				        ed_olded,		    U(oedit_show)	},
    { "olist",	    U(&xMob.area),		    ed_olist,		    0		        },
    { "copy",	    0,				        ed_olded,		    U(medit_copy)	},
    { "commands",	0,				        ed_olded,		    U(show_commands)},
    { "?",		    0,				        ed_olded,		    U(show_help)	},
    { "version",	0,				        ed_olded,		    U(show_version)	},
    { NULL,	        0,				        NULL,			    0		        }
};

const struct olc_comm_type obj_olc_comm_table[] = {
    { "name",	    U(&xObj.name),		    ed_line_string,		0		        },
    { "short",	    U(&xObj.short_descr),	ed_line_string,		0		        },
    { "long",	    U(&xObj.description),	ed_line_string,		0		        },
    { "material",	U(&xObj.material),	    ed_line_string,		0		        },
    { "cost",	    U(&xObj.cost),		    ed_number_pos,		0		        },
    { "level",	    U(&xObj.level),		    ed_number_level,    0		        },
    { "condition",	U(&xObj.condition),	    ed_number_s_pos,	0		        },
    { "weight",	    U(&xObj.weight),		ed_number_s_pos,	0		        },
    { "extra",	    U(&xObj.extra_flags),   ed_flag_toggle,		U(extra_flag_table)  },
    { "wear",	    U(&xObj.wear_flags),	ed_flag_toggle,		U(wear_flag_table)   },
    { "ed",	        U(&xObj.extra_desc),	ed_ed,			    0		        },
    { "type",	    U(&xObj.item_type),	    ed_flag_set_sh,		U(type_flag_table)   },
    { "addaffect",	U(&xObj),			    ed_addaffect,		0		        },
    { "delaffect",	U(&xObj.affected),	    ed_delaffect,		0		        },
    { "addapply",	U(&xObj),			    ed_addapply,		0		        },
    { "v0",	        0,				        ed_value,		    0		        },
    { "v1",	        0,				        ed_value,		    1	            },
    { "v2",	        0,				        ed_value,		    2	            },
    { "v3",	        0,				        ed_value,		    3	            },
    { "v4",	        0,				        ed_value,		    4	            },
    { "create",	    0,				        ed_new_obj,		    0		        },
    { "mshow",	    0,				        ed_olded,		    U(medit_show)   },
    { "oshow",	    0,				        ed_olded,		    U(oedit_show)   },
    { "olist",	    U(&xObj.area),		    ed_olist,		    0		        },
    { "recval",	    U(&xObj),			    ed_objrecval,		0		        },
    { "commands",	0,				        ed_olded,		    U(show_commands)},
    { "?",		    0,				        ed_olded,		    U(show_help)	},
    { "version",	0,				        ed_olded,		    U(show_version)	},
    { NULL,	        0,				        NULL,			    0		        }
};

const struct olc_comm_type room_olc_comm_table[] = {
    { "name",	    U(&xRoom.name),		    ed_line_string,		0		        },
    { "desc",	    U(&xRoom.description),	ed_desc,		    0		        },
    { "ed",	        U(&xRoom.extra_desc),	ed_ed,			    0		        },
    { "heal",	    U(&xRoom.heal_rate),	ed_number_s_pos,	0		        },
    { "mana",	    U(&xRoom.mana_rate),	ed_number_s_pos,	0		        },
    { "owner",	    U(&xRoom.owner),		ed_line_string,		0		        },
    { "roomflags",	U(&xRoom.room_flags),	ed_flag_toggle,		U(room_flag_table)	},
    { "clan",	    U(&xRoom.clan),		    ed_int16lookup,		U(clan_lookup)	},
    { "sector",	    U(&xRoom.sector_type),	ed_flag_set_sh,		U(sector_flag_table)},
    { "north",	    0,				        ed_direction,		DIR_NORTH	    },
    { "south",	    0,				        ed_direction,		DIR_SOUTH	    },
    { "east",	    0,				        ed_direction,		DIR_EAST	    },
    { "west",	    0,				        ed_direction,		DIR_WEST	    },
    { "up",	        0,				        ed_direction,		DIR_UP		    },
    { "down",	    0,				        ed_direction,		DIR_DOWN	    },
    { "rlist",	    0,				        ed_olded,		    U(redit_rlist)	},
    { "mlist",	    0,				        ed_olded,		    U(redit_mlist)	},
    { "olist",	    U(&xRoom.area),	        ed_olist,		    0		        },
    { "copy",	    0,				        ed_olded,		    U(redit_copy)	},
    { "listreset",	0,				        ed_olded,		    U(redit_listreset)  },
    { "checkobj",	0,				        ed_olded,		    U(redit_checkobj)	},
    { "checkmob",	0,				        ed_olded,		    U(redit_checkmob)	},
    { "checkrooms", 0,				        ed_olded,		    U(redit_checkrooms) },
    { "mreset",	    0,				        ed_olded,		    U(redit_mreset)	},
    { "oreset",	    0,				        ed_olded,		    U(redit_oreset)	},
    { "create",	    0,				        ed_olded,		    U(redit_create)	},
    { "format",	    0,				        ed_olded,		    U(redit_format)	},
    { "mshow",	    0,				        ed_olded,		    U(medit_show)	},
    { "oshow",	    0,				        ed_olded,		    U(oedit_show)	},
    { "purge",	    0,				        ed_docomm,		    U(do_purge)	    },
    { "clear",	    0,				        ed_olded,		    U(redit_limpiar)},
    { "commands",	0,				        ed_olded,		    U(show_commands)},
    { "?",		    0,				        ed_olded,		    U(show_help)	},
    { "version",	0,				        ed_olded,		    U(show_version)	},
    { NULL,	        0,				        NULL,			    0		        }
};

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
    default:
        return false;
    }
    return true;
}

char* olc_ed_name(CharData* ch)
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
    case ED_GROUP:
        sprintf(buf, "GEdit");
        break;
    case ED_HELP:
        sprintf(buf, "HEdit");
        break;
    default:
        sprintf(buf, " ");
        break;
    }
    return buf;
}



char* olc_ed_vnum(CharData* ch)
{
    AreaData* pArea;
    RoomData* pRoom;
    ObjectPrototype* pObj;
    MobPrototype* pMob;
    MPROG_CODE* pMcode;
    HelpData* pHelp;
    struct race_type* pRace;
    struct social_type* pSocial;
    struct skill_type* pSkill;
    struct cmd_type* pCmd;
    static char buf[10];

    buf[0] = '\0';
    switch (ch->desc->editor) {
    case ED_AREA:
        pArea = (AreaData*)ch->desc->pEdit;
        sprintf(buf, "%"PRVNUM, pArea ? pArea->vnum : 0);
        break;
    case ED_ROOM:
        pRoom = ch->in_room;
        sprintf(buf, "%"PRVNUM, pRoom ? pRoom->vnum : 0);
        break;
    case ED_OBJECT:
        pObj = (ObjectPrototype*)ch->desc->pEdit;
        sprintf(buf, "%"PRVNUM, pObj ? pObj->vnum : 0);
        break;
    case ED_MOBILE:
        pMob = (MobPrototype*)ch->desc->pEdit;
        sprintf(buf, "%"PRVNUM, pMob ? pMob->vnum : 0);
        break;
    case ED_PROG:
        pMcode = (MPROG_CODE*)ch->desc->pEdit;
        sprintf(buf, "%"PRVNUM, pMcode ? pMcode->vnum : 0);
        break;
    case ED_RACE:
        pRace = (struct race_type*)ch->desc->pEdit;
        sprintf(buf, "%s", pRace ? pRace->name : "");
        break;
    case ED_SOCIAL:
        pSocial = (struct social_type*)ch->desc->pEdit;
        sprintf(buf, "%s", pSocial ? pSocial->name : "");
        break;
    case ED_SKILL:
        pSkill = (struct skill_type*)ch->desc->pEdit;
        sprintf(buf, "%s", pSkill ? pSkill->name : "");
        break;
    case ED_CMD:
        pCmd = (struct cmd_type*)ch->desc->pEdit;
        sprintf(buf, "%s", pCmd ? pCmd->name : "");
        break;
    case ED_HELP:
        pHelp = (HelpData*)ch->desc->pEdit;
        sprintf(buf, "%s", pHelp ? pHelp->keyword : "");
        break;
    default:
        sprintf(buf, " ");
        break;
    }

    return buf;
}

const struct olc_comm_type* get_olc_table(int editor)
{
    switch (editor) {
    case ED_MOBILE:	return mob_olc_comm_table;
    case ED_OBJECT:	return obj_olc_comm_table;
    case ED_ROOM:	return room_olc_comm_table;
    case ED_SKILL:	return skill_olc_comm_table;
    case ED_RACE:	return race_olc_comm_table;
    case ED_CMD:	return cmd_olc_comm_table;
    case ED_PROG:	return prog_olc_comm_table;
    case ED_SOCIAL:	return social_olc_comm_table;
    }
    return NULL;
}

/*****************************************************************************
 Name:		show_olc_cmds
 Purpose:	Format up the commands from given table.
 Called by:	show_commands(olc_act.c).
 ****************************************************************************/
void show_olc_cmds(CharData* ch)
{
    char    buf[MAX_STRING_LENGTH] = "";
    char    buf1[MAX_STRING_LENGTH] = "";
    const struct olc_comm_type* table;
    int     col;

    buf1[0] = '\0';
    col = 0;

    if (ch->desc->editor == ED_AREA) {
        // Areas have a cmd_type, not a comm_type
        for (int cmd = 0; aedit_table[cmd].name != NULL; cmd++) {
            sprintf(buf, "%-15.15s", aedit_table[cmd].name);
            strcat(buf1, buf);
            if (++col % 5 == 0)
                strcat(buf1, "\n\r");
        }

        if (col % 5 != 0)
            strcat(buf1, "\n\r");

        send_to_char(buf1, ch);
        return;
    }

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
bool show_commands(CharData* ch, char* argument)
{
    show_olc_cmds(ch);

    return false;
}

/*****************************************************************************
 *                           Interpreter Tables.                             *
 *****************************************************************************/
const struct olc_cmd_type aedit_table[] =
{
// { command		function	    },
   { "age", 		aedit_age	    },
   { "builder", 	aedit_builder	},	// s removed -- Hugin
   { "commands", 	show_commands	},
   { "create", 	    aedit_create	},
   { "filename", 	aedit_file	    },
   { "name", 	    aedit_name	    },
   { "reset", 	    aedit_reset	    },
   { "security", 	aedit_security	},
   { "show", 	    aedit_show	    },
   { "vnum", 	    aedit_vnum	    },
   { "lvnum", 	    aedit_lvnum	    },
   { "uvnum", 	    aedit_uvnum	    },
   { "credits", 	aedit_credits	},
   { "lowrange", 	aedit_lowrange	},
   { "highrange", 	aedit_highrange	},
   { "?",		    show_help	    },
   { "version", 	show_version	},
   { NULL, 		    0		        }
};

/*****************************************************************************
 *                          End Interpreter Tables.                          *
 *****************************************************************************/

/*****************************************************************************
 Name:		get_area_data
 Purpose:	Returns pointer to area with given vnum.
 Called by:	do_aedit(olc.c).
 ****************************************************************************/
AreaData* get_area_data(VNUM vnum)
{
    AreaData* pArea;

    for (pArea = area_first; pArea; pArea = pArea->next) {
        if (pArea->vnum == vnum)
            return pArea;
    }

    return 0;
}



/*****************************************************************************
 Name:		edit_done
 Purpose:	Resets builder information on completion.
 Called by:	aedit, redit, oedit, medit(olc.c)
 ****************************************************************************/
bool edit_done(CharData* ch)
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


/* Area Interpreter, called by do_aedit. */
void aedit(CharData* ch, char* argument)
{
    AreaData* pArea;
    char    command[MAX_INPUT_LENGTH];
    char    arg[MAX_INPUT_LENGTH];
    int     cmd;
    int     value;

    EDIT_AREA(ch, pArea);

    strcpy(arg, argument);
    argument = one_argument(argument, command);

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("AEdit:  Insufficient security to modify area.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    if (command[0] == '\0') {
        aedit_show(ch, argument);
        return;
    }

    if ((value = flag_value(area_flag_table, command)) != NO_FLAG) {
        TOGGLE_BIT(pArea->area_flags, value);

        send_to_char("Flag toggled.\n\r", ch);
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; aedit_table[cmd].name != NULL; cmd++) {
        if (!str_prefix(command, aedit_table[cmd].name)) {
            if ((*aedit_table[cmd].olc_fun) (ch, argument)) {
                SET_BIT(pArea->area_flags, AREA_CHANGED);
                return;
            }
            else
                return;
        }
    }

    /* Default to Standard Interpreter. */
    interpret(ch, arg);
    return;
}

/* Room Interpreter, called by do_redit. */
void redit(CharData* ch, char* argument)
{
    RoomData* pRoom;
    AreaData* pArea;

    EDIT_ROOM(ch, pRoom);
    pArea = pRoom->area;

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("REdit:  Insufficient security to modify room.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (emptystring(argument)) {
        redit_show(ch, argument);
        return;
    }

    /* Search Table and Dispatch Command. */
    if (!process_olc_command(ch, argument, room_olc_comm_table))
        interpret(ch, argument);

    return;
}

/* Object Interpreter, called by do_oedit. */
void oedit(CharData* ch, char* argument)
{
    AreaData* pArea;
    ObjectPrototype* pObj;

    EDIT_OBJ(ch, pObj);
    pArea = pObj->area;

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("OEdit: Insufficient security to modify area.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (emptystring(argument)) {
        oedit_show(ch, argument);
        return;
    }

    /* Search Table and Dispatch Command. */
    if (!process_olc_command(ch, argument, obj_olc_comm_table))
        interpret(ch, argument);

    return;
}

/* Mobile Interpreter, called by do_medit. */
void    medit(CharData* ch, char* argument)
{
    AreaData* pArea;
    MobPrototype* pMob;

    EDIT_MOB(ch, pMob);
    pArea = pMob->area;

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("MEdit: Insufficient security to modify area.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (emptystring(argument)) {
        medit_show(ch, argument);
        return;
    }

    /* Search Table and Dispatch Command. */
    if (!process_olc_command(ch, argument, mob_olc_comm_table))
        interpret(ch, argument);

    return;
}




const struct editor_cmd_type editor_table[] =
{
/* {	command		function	}, */

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
   {	"help",		do_hedit	},

   {	NULL,		0		}
};


/* Entry point for all editors. */
void    do_olc(CharData* ch, char* argument)
{
    char    command[MAX_INPUT_LENGTH];
    int     cmd;

    argument = one_argument(argument, command);

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



/* Entry point for editing area_data. */
void do_aedit(CharData* ch, char* argument)
{
    AreaData* pArea;
    VNUM vnum;
    char arg[MAX_STRING_LENGTH];

    pArea = ch->in_room->area;

    argument = one_argument(argument, arg);
    if (is_number(arg)) {
        vnum = STRTOVNUM(arg);
        if (!(pArea = get_area_data(vnum))) {
            send_to_char("That area vnum does not exist.\n\r", ch);
            return;
        }
    }
    else if (!str_cmp(arg, "create")) {
        if (!aedit_create(ch, argument))
            return;
        else
            pArea = area_last;
    }

    if (!IS_BUILDER(ch, pArea) || ch->pcdata->security < 9) {
        send_to_char("You do not have enough security to edit areas.\n\r", ch);
        return;
    }

    set_editor(ch->desc, ED_AREA, U(pArea));
/*	ch->desc->pEdit = (void *) pArea;
    ch->desc->editor = ED_AREA; */
    return;
}

/* Entry point for editing room_index_data. */
void do_redit(CharData* ch, char* argument)
{
    RoomData* pRoom;
    char arg1[MIL];

    argument = one_argument(argument, arg1);

    pRoom = ch->in_room;

    if (!str_cmp(arg1, "reset")) {
        if (!IS_BUILDER(ch, pRoom->area)) {
            send_to_char("You do not have enough security to edit rooms.\n\r", ch);
            return;
        }

        reset_room(pRoom);
        send_to_char("Room reset.\n\r", ch);
        return;
    }
    else if (!str_cmp(arg1, "create")) {
        if (argument[0] == '\0' || atoi(argument) == 0) {
            send_to_char("Syntax : edit room create [vnum]\n\r", ch);
            return;
        }

        if (redit_create(ch, argument)) {
            char_from_room(ch);
            char_to_room(ch, (RoomData*)ch->desc->pEdit);
            SET_BIT(pRoom->area->area_flags, AREA_CHANGED);
            pRoom = ch->in_room;
        }
    }
    else if (!IS_NULLSTR(arg1)) {
        pRoom = get_room_data(atoi(arg1));

        if (pRoom == NULL) {
            send_to_char("That room does not exist.\n\r", ch);
            return;
        }
    }

    if (!IS_BUILDER(ch, pRoom->area)) {
        send_to_char("You do not have enough security to edit rooms.\n\r", ch);
        return;
    }

    if (pRoom == NULL)
        bugf("do_redit : NULL pRoom, ch %s!", ch->name);

    if (ch->in_room != pRoom) {
        char_from_room(ch);
        char_to_room(ch, pRoom);
    }

    set_editor(ch->desc, ED_ROOM, U(pRoom));

/*	ch->desc->editor	= ED_ROOM;
    ch->desc->pEdit		= pRoom;

    if (ch->desc->page == 0)
        ch->desc->page = 1; */

    return;
}

/* Entry point for editing object_prototype_data. */
void do_oedit(CharData* ch, char* argument)
{
    ObjectPrototype* pObj;
    AreaData* pArea;
    char arg1[MAX_STRING_LENGTH];
    int  value;

    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    argument = one_argument(argument, arg1);

    if (is_number(arg1)) {
        value = atoi(arg1);
        if (!(pObj = get_object_prototype(value))) {
            send_to_char("OEdit:  That vnum does not exist.\n\r", ch);
            return;
        }

        if (!IS_BUILDER(ch, pObj->area)) {
            send_to_char("You do not have enough security to edit objects.\n\r", ch);
            return;
        }

        set_editor(ch->desc, ED_OBJECT, U(pObj));
/*		ch->desc->pEdit = (void *) pObj;
        ch->desc->editor = ED_OBJECT; */
        return;
    }
    else {
        if (!str_cmp(arg1, "create")) {
            value = atoi(argument);
            if (argument[0] == '\0' || value == 0) {
                send_to_char("Syntax:  edit object create [vnum]\n\r", ch);
                return;
            }

            pArea = get_vnum_area(value);

            if (!pArea) {
                send_to_char("OEdit:  That vnum is not assigned an area.\n\r", ch);
                return;
            }

            if (!IS_BUILDER(ch, pArea)) {
                send_to_char("You do not have enough security to edit objects.\n\r", ch);
                return;
            }

            if (oedit_create(ch, argument)) {
                SET_BIT(pArea->area_flags, AREA_CHANGED);
                ch->desc->editor = ED_OBJECT;
            }
            return;
        }
    }

    send_to_char("OEdit:  There is no default object to edit.\n\r", ch);
    return;
}



/* Entry point for editing mob_prototype_data. */
void do_medit(CharData* ch, char* argument)
{
    MobPrototype* pMob;
    AreaData* pArea;
    int     value;
    char    arg1[MAX_STRING_LENGTH];

    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    argument = one_argument(argument, arg1);

    if (is_number(arg1)) {
        value = atoi(arg1);
        if (!(pMob = get_mob_prototype(value))) {
            send_to_char("MEdit:  That vnum does not exist.\n\r", ch);
            return;
        }

        if (!IS_BUILDER(ch, pMob->area)) {
            send_to_char("You do not have enough security to edit mobs.\n\r", ch);
            return;
        }

        set_editor(ch->desc, ED_MOBILE, U(pMob));
/*		ch->desc->pEdit = (void *) pMob;
        ch->desc->editor = ED_MOBILE; */
        return;
    }
    else {
        if (!str_cmp(arg1, "create")) {
            value = atoi(argument);
            if (arg1[0] == '\0' || value == 0) {
                send_to_char("Syntax:  edit mobile create [vnum]\n\r", ch);
                return;
            }

            pArea = get_vnum_area(value);

            if (!pArea) {
                send_to_char("MEdit:  That vnum is not assigned an area.\n\r", ch);
                return;
            }

            if (!IS_BUILDER(ch, pArea)) {
                send_to_char("You do not have enough security to edit mobs.\n\r", ch);
                return;
            }

            if (ed_new_mob("create", ch, argument, 0, 0)) {
                SET_BIT(pArea->area_flags, AREA_CHANGED);
                ch->desc->editor = ED_MOBILE;
            }
            return;
        }
    }

    send_to_char("MEdit:  There is no default mobile to edit.\n\r", ch);
    return;
}

void    display_resets(CharData* ch, RoomData* pRoom)
{
    ResetData* pReset;
    MobPrototype* pMob = NULL;
    char    buf[MAX_STRING_LENGTH] = "";
    char    final[MAX_STRING_LENGTH] = "";
    int     iReset = 0;

    final[0] = '\0';

    send_to_char(
        " No.  Loads    Description       Location         Vnum   Ar Rm Description"
        "\n\r"
        "==== ======== ============= =================== ======== ===== ==========="
        "\n\r", ch);

    for (pReset = pRoom->reset_first; pReset; pReset = pReset->next) {
        ObjectPrototype* pObj;
        MobPrototype* p_mob_proto;
        ObjectPrototype* obj_proto;
        ObjectPrototype* pObjToIndex;
        RoomData* pRoomIndex;

        final[0] = '\0';
        sprintf(final, "[%2d] ", ++iReset);

        switch (pReset->command) {
        default:
            sprintf(buf, "Bad reset command: %c.", pReset->command);
            strcat(final, buf);
            break;

        case 'M':
            if (!(p_mob_proto = get_mob_prototype(pReset->arg1))) {
                sprintf(buf, "Load Mobile - Bad Mob %d\n\r", pReset->arg1);
                strcat(final, buf);
                continue;
            }

            if (!(pRoomIndex = get_room_data(pReset->arg3))) {
                sprintf(buf, "Load Mobile - Bad Room %d\n\r", pReset->arg3);
                strcat(final, buf);
                continue;
            }

            pMob = p_mob_proto;
            sprintf(buf, "M[%5d] %-13.13s en el suelo         R[%5d] %2d-%2d %-15.15s\n\r",
                pReset->arg1, pMob->short_descr, pReset->arg3,
                pReset->arg2, pReset->arg4, pRoomIndex->name);
            strcat(final, buf);

            /*
             * Check for pet shop.
             * -------------------
             */
            {
                RoomData* pRoomIndexPrev;

                pRoomIndexPrev = get_room_data(pRoomIndex->vnum - 1);
                if (pRoomIndexPrev
                    && IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
                    final[5] = 'P';
            }

            break;

        case 'O':
            if (!(obj_proto = get_object_prototype(pReset->arg1))) {
                sprintf(buf, "Load Object - Bad Object %d\n\r",
                    pReset->arg1);
                strcat(final, buf);
                continue;
            }

            pObj = obj_proto;

            if (!(pRoomIndex = get_room_data(pReset->arg3))) {
                sprintf(buf, "Load Object - Bad Room %d\n\r", pReset->arg3);
                strcat(final, buf);
                continue;
            }

            sprintf(buf, "O[%5d] %-13.13s en el suelo         "
                "R[%5d]       %-15.15s\n\r",
                pReset->arg1, pObj->short_descr,
                pReset->arg3, pRoomIndex->name);
            strcat(final, buf);

            break;

        case 'P':
            if (!(obj_proto = get_object_prototype(pReset->arg1))) {
                sprintf(buf, "Put Object - Bad Object %d\n\r",
                    pReset->arg1);
                strcat(final, buf);
                continue;
            }

            pObj = obj_proto;

            if (!(pObjToIndex = get_object_prototype(pReset->arg3))) {
                sprintf(buf, "Put Object - Bad To Object %d\n\r",
                    pReset->arg3);
                strcat(final, buf);
                continue;
            }

            sprintf(buf,
                "O[%5d] %-13.13s inside              O[%5d] %2d-%2d %-15.15s\n\r",
                pReset->arg1,
                pObj->short_descr,
                pReset->arg3,
                pReset->arg2,
                pReset->arg4,
                pObjToIndex->short_descr);
            strcat(final, buf);

            break;

        case 'G':
        case 'E':
            if (!(obj_proto = get_object_prototype(pReset->arg1))) {
                sprintf(buf, "Give/Equip Object - Bad Object %d\n\r",
                    pReset->arg1);
                strcat(final, buf);
                continue;
            }

            pObj = obj_proto;

            if (!pMob) {
                sprintf(buf, "Give/Equip Object - No Previous Mobile\n\r");
                strcat(final, buf);
                break;
            }

            if (pMob->pShop) {
                sprintf(buf,
                    "O[%5d] %-13.13s in the inventory of S[%5d]       %-15.15s\n\r",
                    pReset->arg1,
                    pObj->short_descr,
                    pMob->vnum,
                    pMob->short_descr);
            }
            else
                sprintf(buf,
                    "O[%5d] %-13.13s %-19.19s M[%5d]       %-15.15s\n\r",
                    pReset->arg1,
                    pObj->short_descr,
                    (pReset->command == 'G') ?
                    flag_string(wear_loc_strings, WEAR_NONE)
                    : flag_string(wear_loc_strings, pReset->arg3),
                    pMob->vnum,
                    pMob->short_descr);
            strcat(final, buf);
            break;

            /*
             * Doors are set in exit_reset_flags don't need to be displayed.
             * If you want to display them then uncomment the new_reset
             * line in the case 'D' in load_resets in db.c and here.
             */
        case 'D':
            pRoomIndex = get_room_data(pReset->arg1);
            sprintf(buf, "R[%5d] %s door of %-19.19s reset to %s\n\r",
                pReset->arg1,
                capitalize(dir_list[pReset->arg2].name),
                pRoomIndex->name,
                flag_string(door_resets, pReset->arg3));
            strcat(final, buf);

            break;
            /*
             * End Doors Comment.
             */
        case 'R':
            if (!(pRoomIndex = get_room_data(pReset->arg1))) {
                sprintf(buf, "Randomize Exits - Bad Room %d\n\r",
                    pReset->arg1);
                strcat(final, buf);
                continue;
            }

            sprintf(buf, "R[%5d] Exits are randomized in %s\n\r",
                pReset->arg1, pRoomIndex->name);
            strcat(final, buf);

            break;
        }
        send_to_char(final, ch);
    }

    return;
}



/*****************************************************************************
 Name:		add_reset
 Purpose:	Inserts a new reset in the given index slot.
 Called by:	do_resets(olc.c).
 ****************************************************************************/
void    add_reset(RoomData* room, ResetData* pReset, int indice)
{
    ResetData* reset;
    int     iReset = 0;

    if (!room->reset_first) {
        room->reset_first = pReset;
        room->reset_last = pReset;
        pReset->next = NULL;
        return;
    }

    indice--;

    if (indice == 0)				/* First slot (1) selected. */
    {
        pReset->next = room->reset_first;
        room->reset_first = pReset;
        return;
    }

    /*
     * If negative slot( <= 0 selected) then this will find the last.
     */
    for (reset = room->reset_first; reset->next; reset = reset->next) {
        if (++iReset == indice)
            break;
    }

    pReset->next = reset->next;
    reset->next = pReset;
    if (!pReset->next)
        room->reset_last = pReset;
    return;
}

void    do_resets(CharData* ch, char* argument)
{
    char    arg1[MAX_INPUT_LENGTH];
    char    arg2[MAX_INPUT_LENGTH];
    char    arg3[MAX_INPUT_LENGTH];
    char    arg4[MAX_INPUT_LENGTH];
    char    arg5[MAX_INPUT_LENGTH];
    char    arg6[MAX_INPUT_LENGTH];
    char    arg7[MAX_INPUT_LENGTH];
    ResetData* pReset = NULL;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);
    argument = one_argument(argument, arg4);
    argument = one_argument(argument, arg5);
    argument = one_argument(argument, arg6);
    argument = one_argument(argument, arg7);

    if (!IS_BUILDER(ch, ch->in_room->area)) {
        send_to_char("Resets: Invalid security for editing this area.\n\r",
            ch);
        return;
    }

    /*
     * Display resets in current room.
     * -------------------------------
     */
    if (arg1[0] == '\0') {
        if (ch->in_room->reset_first) {
            send_to_char(
                "Resets: M = mobile, R = room, O = object, "
                "P = pet, S = shopkeeper\n\r", ch);
            display_resets(ch, ch->in_room);
        }
        else
            send_to_char("No resets in this room.\n\r", ch);
    }

    /*
     * Take index number and search for commands.
     * ------------------------------------------
     */
    if (is_number(arg1)) {
        RoomData* pRoom = ch->in_room;

        /*
         * Delete a reset.
         * ---------------
         */
        if (!str_cmp(arg2, "delete")) {
            int     insert_loc = atoi(arg1);

            if (!ch->in_room->reset_first) {
                send_to_char("No resets in this area.\n\r", ch);
                return;
            }

            if (insert_loc - 1 <= 0) {
                pReset = pRoom->reset_first;
                pRoom->reset_first = pRoom->reset_first->next;
                if (!pRoom->reset_first)
                    pRoom->reset_last = NULL;
            }
            else {
                int     iReset = 0;
                ResetData* prev = NULL;

                for (pReset = pRoom->reset_first;
                    pReset;
                    pReset = pReset->next) {
                    if (++iReset == insert_loc)
                        break;
                    prev = pReset;
                }

                if (!pReset) {
                    send_to_char("Reset not found.\n\r", ch);
                    return;
                }

                if (prev)
                    prev->next = prev->next->next;
                else
                    pRoom->reset_first = pRoom->reset_first->next;

                for (pRoom->reset_last = pRoom->reset_first;
                    pRoom->reset_last->next;
                    pRoom->reset_last = pRoom->reset_last->next);
            }

            free_reset_data(pReset);
            send_to_char("Reset deleted.\n\r", ch);
            SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
        }
        else
            /*
             * Add a reset.
             * ------------
             */
            if ((!str_cmp(arg2, "mob") && is_number(arg3))
                || (!str_cmp(arg2, "obj") && is_number(arg3))) {
                 /*
                  * Check for Mobile reset.
                  * -----------------------
                  */
                if (!str_cmp(arg2, "mob")) {
                    if (get_mob_prototype(is_number(arg3) ? atoi(arg3) : 1) == NULL) {
                        send_to_char("That mob does not exist.\n\r", ch);
                        return;
                    }
                    pReset = new_reset_data();
                    pReset->command = 'M';
                    pReset->arg1 = (VNUM)atoi(arg3);
                    pReset->arg2 = is_number(arg4) ? (int16_t)atoi(arg4) : 1;	/* Max # */
                    pReset->arg3 = ch->in_room->vnum;
                    pReset->arg4 = is_number(arg5) ? (int16_t)atoi(arg5) : 1;	/* Min # */
                }
                else
                    /*
                     * Check for Object reset.
                     * -----------------------
                     */
                    if (!str_cmp(arg2, "obj")) {
                        /*
                         * Inside another object.
                         * ----------------------
                         */
                        if (!str_prefix(arg4, "inside")) {
                            ObjectPrototype* temp;

                            temp = get_object_prototype(is_number(arg5) ? (VNUM)atoi(arg5) : 1);
                            if ((temp->item_type != ITEM_CONTAINER) &&
                                (temp->item_type != ITEM_CORPSE_NPC)) {
                                send_to_char("Object 2 is not a container.\n\r", ch);
                                return;
                            }
                            pReset = new_reset_data();
                            pReset->arg1 = (VNUM)atoi(arg3);
                            pReset->command = 'P';
                            pReset->arg2 = is_number(arg6) ? (int16_t)atoi(arg6) : 1;
                            pReset->arg3 = is_number(arg5) ? (VNUM)atoi(arg5) : 1;
                            pReset->arg4 = is_number(arg7) ? (int16_t)atoi(arg7) : 1;
                        }
                        else
                            /*
                             * Inside the room.
                             * ----------------
                             */
                            if (!str_cmp(arg4, "room")) {
                                if (get_object_prototype(atoi(arg3)) == NULL) {
                                    send_to_char("That object does not exist.\n\r", ch);
                                    return;
                                }
                                pReset = new_reset_data();
                                pReset->arg1 = (VNUM)atoi(arg3);
                                pReset->command = 'O';
                                pReset->arg2 = 0;
                                pReset->arg3 = ch->in_room->vnum;
                                pReset->arg4 = 0;
                            }
                            else
                                /*
                                 * Into a Mobile's inventory.
                                 * --------------------------
                                 */
                            {
                                int blah = flag_value(wear_loc_flag_table, arg4);

                                if (blah == NO_FLAG) {
                                    send_to_char("Resets: '? wear-loc'\n\r", ch);
                                    return;
                                }
                                if (get_object_prototype(atoi(arg3)) == NULL) {
                                    send_to_char("That vnum does not exist.\n\r", ch);
                                    return;
                                }
                                pReset = new_reset_data();
                                pReset->arg1 = atoi(arg3);
                                pReset->arg3 = blah;
                                if (pReset->arg3 == WEAR_NONE)
                                    pReset->command = 'G';
                                else
                                    pReset->command = 'E';
                            }
                    }
                add_reset(ch->in_room, pReset, atoi(arg1));
                SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
                send_to_char("Reset added.\n\r", ch);
            }
            else if (!str_cmp(arg2, "random") && is_number(arg3)) {
                if (atoi(arg3) < 1 || atoi(arg3) > 6) {
                    send_to_char("Invalid argument.\n\r", ch);
                    return;
                }
                pReset = new_reset_data();
                pReset->command = 'R';
                pReset->arg1 = ch->in_room->vnum;
                pReset->arg2 = (int16_t)atoi(arg3);
                add_reset(ch->in_room, pReset, atoi(arg1));
                SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
                send_to_char("Random exits reset added.\n\r", ch);
            }
            else {
                send_to_char("Syntax: RESET <number> OBJ <vnum> <wear_loc>\n\r", ch);
                send_to_char("        RESET <number> OBJ <vnum> inside <vnum> [limit] [count]\n\r", ch);
                send_to_char("        RESET <number> OBJ <vnum> room\n\r", ch);
                send_to_char("        RESET <number> MOB <vnum> [max #x area] [max #x room]\n\r", ch);
                send_to_char("        RESET <number> DELETE\n\r", ch);
                send_to_char("        RESET <number> RANDOM [#x exits]\n\r", ch);
            }
    }
    else // arg1 no es un number
    {
        if (!str_cmp(arg1, "add")) {
            int tvar = 0, found = 0;
            char* arg;

            if (is_number(arg2)) {
                send_to_char("Invalid syntax.\n\r"
                    "Your options are:\n\r"
                    "reset add mob [vnum/name]\n\r"
                    "reset add obj [vnum/name]\n\r"
                    "reset add [name]\n\r", ch);
                return;
            }

            if (!str_cmp(arg2, "mob")) {
                tvar = 1;
                arg = arg3;
            }
            else
                if (!str_cmp(arg2, "obj")) {
                    tvar = 2;
                    arg = arg3;
                }
                else
                    arg = arg2;

            if (tvar == 0 || tvar == 1) {
                if (is_number(arg))
                    found = get_mob_prototype(atoi(arg)) ? atoi(arg) : 0;
                else
                    found = get_vnum_mob_name_area(arg, ch->in_room->area);
                if (found)
                    tvar = 1;
            }

            if (found == 0 && (tvar == 0 || tvar == 2)) {
                if (is_number(arg))
                    found = get_object_prototype(atoi(arg)) ? atoi(arg) : 0;
                else
                    found = get_vnum_obj_name_area(arg, ch->in_room->area);
                if (found)
                    tvar = 2;
            }

            if (found == 0) {
                printf_to_char(ch, "%s was not found in the area.\n\r",
                    (tvar == 0) ? "Mob/object" : ((tvar == 1) ? "Mob" : "Object"));
                return;
            }
            pReset = new_reset_data();
            pReset->command = tvar == 1 ? 'M' : 'O';
            pReset->arg1 = found;
            pReset->arg2 = (tvar == 2) ? 0 : MAX_MOB;	/* Max # */
            pReset->arg3 = ch->in_room->vnum;
            pReset->arg4 = (tvar == 2) ? 0 : MAX_MOB;	/* Min # */

            printf_to_char(ch, "Added reset of %s %d...", tvar == 1 ? "mob" : "object", found);
            add_reset(ch->in_room, pReset, -1); // al final
            SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
            send_to_char("Done.\n\r", ch);
        } // anadir
    }

    return;
}

/*****************************************************************************
 Name:		do_alist
 Purpose:	Normal command to list areas and display area information.
 Called by:	interpreter(interp.c)
 ****************************************************************************/
void do_alist(CharData* ch, char* argument)
{
    char    buf[MAX_STRING_LENGTH];
    char    result[MAX_STRING_LENGTH * 2];	/* May need tweaking. */
    AreaData* pArea;

    sprintf(result, "[%3s] [%-27s] (%-5s-%5s) [%-10s] %3s [%-10s]\n\r",
        "Num", "Area Name", "lvnum", "uvnum", "Filename", "Sec", "Builders");

    for (pArea = area_first; pArea; pArea = pArea->next) {
        sprintf(buf, "[%3d] %-29.29s (%-5d-%5d) %-12.12s [%d] [%-10.10s]\n\r",
            pArea->vnum,
            pArea->name,
            pArea->min_vnum,
            pArea->max_vnum,
            pArea->file_name,
            pArea->security,
            pArea->builders);
        strcat(result, buf);
    }

    send_to_char(result, ch);
    return;
}

bool process_olc_command(CharData* ch, char* argument, const struct olc_comm_type* table)
{
    char arg[MIL];
    MobPrototype* pMob;
    ObjectPrototype* pObj;
    RoomData* pRoom;
    struct race_type* pRace;
    struct skill_type* pSkill;
    struct cmd_type* pCmd;
    AreaData* tArea;
    MPROG_CODE* pProg;
    struct social_type* pSoc;
    int temp;
    uintptr_t pointer;

    argument = one_argument(argument, arg);

    for (temp = 0; table[temp].name; temp++) {
        if (LOWER(arg[0]) == LOWER(table[temp].name[0])
            && !str_prefix(arg, table[temp].name)) {
            switch (ch->desc->editor) {
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
                tArea = pRoom->area;
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xRoom) + U(pRoom));
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
                    save_skills();
                return true;
                break;

            case ED_RACE:
                EDIT_RACE(ch, pRace);
                if (table[temp].argument)
                    pointer = (table[temp].argument - U(&xRace) + U(pRace));
                else
                    pointer = 0;
                if ((*table[temp].function) (table[temp].name, ch, argument, pointer, table[temp].parameter))
                    save_races();
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
                    save_socials();
                return true;
                break;
            }
        }
    }

    return false;
}

DO_FUN_DEC(do_page)
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

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
