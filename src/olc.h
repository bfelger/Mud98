/***************************************************************************
 *  File: olc.h                                                            *
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

 /*
 * This is a header file for all the OLC files.  Feel free to copy it into
 * merc.h if you wish.  Many of these routines may be handy elsewhere in
 * the code.  -Jason Dinkel
 */

#pragma once
#ifndef MUD98__OLC_H
#define MUD98__OLC_H

#include "interp.h"
#include "tables.h"
#include "tablesave.h"

#include "entities/mob_prototype.h"

#include "data/race.h"

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

/*
 * The version info.  Please use this info when reporting bugs.
 * It is displayed in the game by typing 'version' while editing.
 * Do not remove these from the code - by request of Jason Dinkel
 */
#define OLC_VERSION	"ILAB Online Creation [Beta 1.0, ROM 2.3 modified]\n\r" \
                    "     Port to ROM 2.4 v2.01\n\r"
#define OLC_AUTHOR	"     By Jason(jdinkel@mines.colorado.edu)\n\r"         \
                    "     Modified for use with ROM 2.3\n\r"                \
                    "     By Hans Birkeland (hansbi@ifi.uio.no)\n\r"        \
                    "     Modificado para uso en ROM 2.4v4\n\r"	            \
                    "     Por Birdie (itoledo.geo@yahoo.com)\n\r"
#define OLC_DATE	"     (Apr. 7, 1995 - ROM mod, Apr 16, 1995)\n\r"       \
                    "     (Port to ROM 2.4 - Nov 2, 1996)\n\r"
#define OLC_CREDITS	"     Original by Surreality(cxw197@psu.edu) and  "     \
                         "Locke(locke@lm.com)"

/*
 * New typedefs.
 */
typedef	bool OLC_FUN(CharData* ch, char* argument);
#define DECLARE_OLC_FUN( fun )	OLC_FUN fun

typedef bool ED_FUN(char*, CharData*, char*, uintptr_t, const uintptr_t);
#define DECLARE_ED_FUN( fun )	ED_FUN fun

#define ED_FUN_DEC(blah)	bool blah (char* n_fun, CharData* ch,        \
                            char* argument, uintptr_t arg, const uintptr_t par)

/* Command procedures needed ROM OLC */
DECLARE_DO_FUN(do_help);
DECLARE_SPELL_FUN(spell_null);

/*
 * Connected states for editor.
 */
#define ED_NONE		0
#define	ED_AREA		1
#define	ED_ROOM		2
#define	ED_OBJECT	3
#define	ED_MOBILE	4
#define	ED_PROG		5
#define	ED_CLAN		6
#define	ED_RACE		7
#define	ED_SOCIAL	8
#define ED_SKILL	9
#define ED_CMD		10
#define ED_GROUP	11
#define ED_HELP		13

/*
 * Interpreter Prototypes
 */
void    aedit       (CharData* ch, char* argument);
void    redit       (CharData* ch, char* argument);
void    medit       (CharData* ch, char* argument);
void    oedit       (CharData* ch, char* argument);
void    pedit       (CharData* ch, char* argument);
void	raedit		(CharData* ch, char* argument);
void	sedit		(CharData* ch, char* argument);
void	skedit		(CharData* ch, char* argument);
void	cmdedit		(CharData* ch, char* argument);
void	gedit		(CharData* ch, char* argument);
void	hedit		(CharData* ch, char* argument);

/*
 * OLC Constants
 */
#define MAX_MOB	1		/* Default maximum number for resetting mobs */

#define	MIN_PEDIT_SECURITY	    1
#define	MIN_SKEDIT_SECURITY	    5
#define MIN_CMDEDIT_SECURITY	7
#define MIN_SEDIT_SECURITY	    3

/*
 * Structure for an OLC editor command.
 */
struct olc_cmd_type {
    char* const	name;
    OLC_FUN* olc_fun;
};

struct olc_comm_type {
    char* name;
    uintptr_t argument;
    ED_FUN* function;
    const uintptr_t parameter;
};

extern const struct olc_comm_type mob_olc_comm_table[];
extern const struct olc_comm_type obj_olc_comm_table[];
extern const struct olc_comm_type room_olc_comm_table[];
extern const struct olc_comm_type skill_olc_comm_table[];
extern const struct olc_comm_type race_olc_comm_table[];
extern const struct olc_comm_type cmd_olc_comm_table[];
extern const struct olc_comm_type prog_olc_comm_table[];
extern const struct olc_comm_type social_olc_comm_table[];

bool process_olc_command(CharData*, char* argument, const struct olc_comm_type*);

/*
 * Structure for an OLC editor startup command.
 */
struct editor_cmd_type {
    char* const	name;
    DoFunc* do_fun;
};

/*
 * Utils.
 */
AreaData* get_vnum_area(VNUM vnum);
AreaData* get_area_data(VNUM vnum);
FLAGS flag_value(const struct flag_type* flag_table, char* argument);
void add_reset(RoomData*, ResetData*, int);
void set_editor(Descriptor*, int, uintptr_t);

bool run_olc_editor(Descriptor* d, char* incomm);
char* olc_ed_name(CharData* ch);
char* olc_ed_vnum(CharData* ch);

/*
 * Interpreter Table Prototypes
 */
extern const struct olc_cmd_type aedit_table[];
extern const struct olc_cmd_type raedit_table[];
extern const struct olc_cmd_type gedit_table[];
extern const struct olc_cmd_type hedit_table[];

/*
 * Editor Commands.
 */
DECLARE_DO_FUN(do_aedit);
DECLARE_DO_FUN(do_redit);
DECLARE_DO_FUN(do_oedit);
DECLARE_DO_FUN(do_medit);
DECLARE_DO_FUN(do_pedit);
DECLARE_DO_FUN(do_raedit);
DECLARE_DO_FUN(do_sedit);
DECLARE_DO_FUN(do_skedit);
DECLARE_DO_FUN(do_cmdedit);
DECLARE_DO_FUN(do_gedit);
DECLARE_DO_FUN(do_hedit);

/*
 * General Functions
 */
bool show_commands		args((CharData* ch, char* argument));
bool show_help			args((CharData* ch, char* argument));
bool edit_done			args((CharData* ch));
bool show_version		args((CharData* ch, char* argument));

/*
 * Area Editor Prototypes
 */
DECLARE_OLC_FUN(aedit_show);
DECLARE_OLC_FUN(aedit_create);
DECLARE_OLC_FUN(aedit_name);
DECLARE_OLC_FUN(aedit_file);
DECLARE_OLC_FUN(aedit_age);
/* DECLARE_OLC_FUN( aedit_recall	);       ROM OLC */
DECLARE_OLC_FUN(aedit_reset);
DECLARE_OLC_FUN(aedit_security);
DECLARE_OLC_FUN(aedit_builder);
DECLARE_OLC_FUN(aedit_vnum);
DECLARE_OLC_FUN(aedit_lvnum);
DECLARE_OLC_FUN(aedit_uvnum);
DECLARE_OLC_FUN(aedit_credits);
DECLARE_OLC_FUN(aedit_lowrange);
DECLARE_OLC_FUN(aedit_highrange);

/*
 * Room Editor Prototypes
 */
DECLARE_OLC_FUN(redit_show);
DECLARE_OLC_FUN(redit_create);
DECLARE_OLC_FUN(redit_format);
DECLARE_OLC_FUN(redit_mreset);
DECLARE_OLC_FUN(redit_oreset);
DECLARE_OLC_FUN(redit_mlist);
DECLARE_OLC_FUN(redit_rlist);
DECLARE_OLC_FUN(redit_addrprog);
DECLARE_OLC_FUN(redit_listreset);
DECLARE_OLC_FUN(redit_checkmob);
DECLARE_OLC_FUN(redit_checkobj);
DECLARE_OLC_FUN(redit_copy);
DECLARE_OLC_FUN(redit_checkrooms);
DECLARE_OLC_FUN(redit_clear);

/*
 * Object Editor Prototypes
 */
DECLARE_OLC_FUN(oedit_show);
DECLARE_OLC_FUN(oedit_create);
DECLARE_OLC_FUN(oedit_addaffect);
DECLARE_OLC_FUN(oedit_delaffect);
DECLARE_OLC_FUN(oedit_addapply);
DECLARE_OLC_FUN(oedit_addoprog);

/* Mob editor. */
DECLARE_OLC_FUN(medit_show);
DECLARE_OLC_FUN(medit_group);
DECLARE_OLC_FUN(medit_copy);

/*
 * Race editor.
 */
DECLARE_OLC_FUN(raedit_show);
DECLARE_OLC_FUN(raedit_new);
DECLARE_OLC_FUN(raedit_list);
DECLARE_OLC_FUN(raedit_amult);
DECLARE_OLC_FUN(raedit_stats);
DECLARE_OLC_FUN(raedit_maxstats);
DECLARE_OLC_FUN(raedit_skills);

/*
 * Social editor.
 */
DECLARE_OLC_FUN(sedit_show);
DECLARE_OLC_FUN(sedit_cnoarg);
DECLARE_OLC_FUN(sedit_onoarg);
DECLARE_OLC_FUN(sedit_cfound);
DECLARE_OLC_FUN(sedit_ofound);
DECLARE_OLC_FUN(sedit_vfound);
DECLARE_OLC_FUN(sedit_cself);
DECLARE_OLC_FUN(sedit_oself);
DECLARE_OLC_FUN(sedit_new);
DECLARE_OLC_FUN(sedit_delete);

/*
 * Skill editor.
 */
DECLARE_OLC_FUN(skedit_show);
DECLARE_OLC_FUN(skedit_name);
DECLARE_OLC_FUN(skedit_nombre);
DECLARE_OLC_FUN(skedit_slot);
DECLARE_OLC_FUN(skedit_level);
DECLARE_OLC_FUN(skedit_rating);
DECLARE_OLC_FUN(skedit_list);
DECLARE_OLC_FUN(skedit_gsn);
DECLARE_OLC_FUN(skedit_spell);
DECLARE_OLC_FUN(skedit_new);
DECLARE_OLC_FUN(skedit_script);
DECLARE_OLC_FUN(skedit_type);
DECLARE_OLC_FUN(skedit_free);
DECLARE_OLC_FUN(skedit_cmsg);
DECLARE_OLC_FUN(skedit_callback);

/*
 * Comman editor.
 */
DECLARE_OLC_FUN(cmdedit_show);
DECLARE_OLC_FUN(cmdedit_name);
DECLARE_OLC_FUN(cmdedit_function);
DECLARE_OLC_FUN(cmdedit_level);
DECLARE_OLC_FUN(cmdedit_list);
DECLARE_OLC_FUN(cmdedit_new);
DECLARE_OLC_FUN(cmdedit_delete);

/*
 * Group editor.
 */
DECLARE_OLC_FUN(gedit_show);
DECLARE_OLC_FUN(gedit_name);
DECLARE_OLC_FUN(gedit_rating);
DECLARE_OLC_FUN(gedit_spell);
DECLARE_OLC_FUN(gedit_list);

/*
 * Help Editor.
 */
DECLARE_OLC_FUN(hedit_show);
DECLARE_OLC_FUN(hedit_keyword);
DECLARE_OLC_FUN(hedit_text);
DECLARE_OLC_FUN(hedit_new);
DECLARE_OLC_FUN(hedit_level);
DECLARE_OLC_FUN(hedit_delete);
DECLARE_OLC_FUN(hedit_list);

/*
 * Editors.
 */
DECLARE_ED_FUN(ed_line_string);
DECLARE_ED_FUN(ed_desc);
DECLARE_ED_FUN(ed_bool);
DECLARE_ED_FUN(ed_flag_toggle);
DECLARE_ED_FUN(ed_flag_set_long);
DECLARE_ED_FUN(ed_flag_set_sh);
DECLARE_ED_FUN(ed_number_level);
DECLARE_ED_FUN(ed_number_align);
DECLARE_ED_FUN(ed_number_s_pos);
DECLARE_ED_FUN(ed_number_pos);
DECLARE_ED_FUN(ed_number_l_pos);
DECLARE_ED_FUN(ed_shop);
DECLARE_ED_FUN(ed_new_mob);
DECLARE_ED_FUN(ed_commands);
DECLARE_ED_FUN(ed_gamespec);
DECLARE_ED_FUN(ed_recval);
DECLARE_ED_FUN(ed_int16poslookup);
DECLARE_ED_FUN(ed_int16lookup);
DECLARE_ED_FUN(ed_ac);
DECLARE_ED_FUN(ed_dice);
DECLARE_ED_FUN(ed_addprog);
DECLARE_ED_FUN(ed_delprog);
DECLARE_ED_FUN(ed_script);
DECLARE_ED_FUN(ed_ed);
DECLARE_ED_FUN(ed_addaffect);
DECLARE_ED_FUN(ed_delaffect);
DECLARE_ED_FUN(ed_addapply);
DECLARE_ED_FUN(ed_value);
DECLARE_ED_FUN(ed_new_obj);
DECLARE_ED_FUN(ed_trap);
DECLARE_ED_FUN(ed_race);
DECLARE_ED_FUN(ed_olded);
DECLARE_ED_FUN(ed_direction);
DECLARE_ED_FUN(ed_docomm);
DECLARE_ED_FUN(ed_olist);
DECLARE_ED_FUN(ed_objrecval);

/*
 * Macros
 */

/* Return pointers to what is being edited. */
#define EDIT_MOB(ch, mob)	    ( mob = (MobPrototype*)ch->desc->pEdit )
#define EDIT_OBJ(ch, obj)	    ( obj = (ObjectPrototype*)ch->desc->pEdit )
#define EDIT_ROOM(ch, room)	    ( room = (RoomData*)ch->desc->pEdit )
#define EDIT_AREA(ch, area)	    ( area = (AreaData*)ch->desc->pEdit )
#define EDIT_RACE(ch, race)	    ( race = (Race*)ch->desc->pEdit )
#define EDIT_SOCIAL(ch, social)	( social = (struct social_type*)ch->desc->pEdit )
#define EDIT_SKILL(ch, skill)	( skill = (struct skill_type*)ch->desc->pEdit )
#define EDIT_CMD(ch, cmd)	    ( cmd = (CmdInfo*)ch->desc->pEdit )
#define EDIT_GROUP(ch, grp)	    ( grp = (struct group_type*)ch->desc->pEdit )
#define EDIT_HELP(ch, help)	    ( help = (HelpData*)ch->desc->pEdit )
#define EDIT_PROG(ch, code)	    ( code = (MobProgCode*)ch->desc->pEdit )


void show_liqlist(CharData* ch);
void show_poslist(CharData* ch);
void show_damlist(CharData* ch);
void show_sexlist(CharData* ch);
void show_sizelist(CharData* ch);

extern RoomData xRoom;
extern MobPrototype xMob;
extern ObjectPrototype xObj;

extern void InitScreen(Descriptor*);
void list_archetypes(CharData* ch);

#endif // !MUD98__OLC_H
