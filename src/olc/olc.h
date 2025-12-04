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
#ifndef MUD98__OLC__OLC_H
#define MUD98__OLC__OLC_H

#include <tables.h>
#include <tablesave.h>

#include <entities/mob_prototype.h>

#include <data/class.h>
#include <data/race.h>
#include <data/skill.h>
#include <data/spell.h>
#include <data/social.h>
#include <data/quest.h>

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

typedef struct cmd_info_t CmdInfo;
typedef struct mob_prog_code_t  MobProgCode;
typedef struct skill_t Skill;

// New typedefs.
typedef	bool OlcFunc(Mobile* ch, char* argument);
#define DECLARE_OLC_FUN(fun)    OlcFunc fun

typedef bool EditFunc(char*, Mobile*, char*, uintptr_t, const uintptr_t);
#define DECLARE_ED_FUN(fun)     EditFunc fun

#define ED_FUN_DEC(blah)        bool blah (char* n_fun, Mobile* ch, \
    char* argument, uintptr_t arg, const uintptr_t par)

/* Command procedures needed ROM OLC */
DECLARE_DO_FUN(do_help);
DECLARE_SPELL_FUN(spell_null);

// Connected states for editor.

typedef enum editor_t {
    ED_NONE     = 0,
    ED_AREA     = 1,
    ED_ROOM     = 2,
    ED_OBJECT   = 3,
    ED_MOBILE	= 4,
    ED_PROG     = 5,
    ED_CLAN     = 6,
    ED_RACE     = 7,
    ED_SOCIAL   = 8,
    ED_SKILL    = 9,
    ED_CMD      = 10,
    ED_GROUP    = 11,
    ED_CLASS    = 12,
    ED_HELP     = 13,
    ED_QUEST    = 14,
} EditorType;

// Interpreter Prototypes
void    aedit       (Mobile* ch, char* argument);
void    cedit       (Mobile* ch, char* argument);
void	cmdedit		(Mobile* ch, char* argument);
void	gedit		(Mobile* ch, char* argument);
void	hedit		(Mobile* ch, char* argument);
void    medit       (Mobile* ch, char* argument);
void    oedit       (Mobile* ch, char* argument);
void    pedit       (Mobile* ch, char* argument);
void	raedit		(Mobile* ch, char* argument);
void    redit       (Mobile* ch, char* argument);
void	sedit		(Mobile* ch, char* argument);
void	skedit		(Mobile* ch, char* argument);
void    qedit       (Mobile* ch, char* argument);

// OLC Constants
#define MAX_MOB	1		/* Default maximum number for resetting mobs */

#define MIN_AEDIT_SECURITY      5
#define MIN_CMDEDIT_SECURITY	7
#define MIN_CEDIT_SECURITY      7
#define MIN_HEDIT_SECURITY      5
#define	MIN_PEDIT_SECURITY	    3
#define MIN_RAEDIT_SECURITY     7
#define	MIN_SKEDIT_SECURITY	    5
#define MIN_SEDIT_SECURITY	    3
#define MIN_QEDIT_SECURITY      3

// Structure for an OLC editor command.
typedef struct olc_cmd_t {
    char* const	name;
    OlcFunc* olc_fun;
} OlcCmd;

typedef struct olc_cmd_entry_t {
    char* name;
    uintptr_t argument;
    EditFunc* function;
    const uintptr_t parameter;
} OlcCmdEntry;

extern const OlcCmdEntry area_olc_comm_table[];
extern const OlcCmdEntry mob_olc_comm_table[];
extern const OlcCmdEntry obj_olc_comm_table[];
extern const OlcCmdEntry room_olc_comm_table[];
extern const OlcCmdEntry skill_olc_comm_table[];
extern const OlcCmdEntry race_olc_comm_table[];
extern const OlcCmdEntry cmd_olc_comm_table[];
extern const OlcCmdEntry prog_olc_comm_table[];
extern const OlcCmdEntry social_olc_comm_table[];
extern const OlcCmdEntry class_olc_comm_table[];
extern const OlcCmdEntry quest_olc_comm_table[];

bool process_olc_command(Mobile*, char* argument, const OlcCmdEntry*);

// Structure for an OLC editor startup command.
typedef struct editor_cmd_t {
    char* const	name;
    DoFunc* do_fun;
} EditCmd;

// Utils.
AreaData* get_vnum_area(VNUM vnum);
AreaData* get_area_data(VNUM vnum);
FLAGS flag_value(const struct flag_type* flag_table, char* argument);
void add_reset(RoomData*, Reset*, int);
void set_editor(Descriptor*, int, uintptr_t);

bool run_olc_editor(Descriptor* d, char* incomm);
char* olc_ed_name(Mobile* ch);
char* olc_ed_vnum(Mobile* ch);

// Interpreter Table Prototypes
//extern const OlcCmd aedit_table[];
extern const OlcCmd raedit_table[];
extern const OlcCmd gedit_table[];
extern const OlcCmd hedit_table[];

// Dummy objects needed for calc'ing field offsets
extern AreaData xArea;
extern Class xClass;
extern CmdInfo xCmd;
extern MobProgCode xProg;
extern MobPrototype xMob;
extern ObjPrototype xObj;
extern Race xRace;
extern RoomData xRoom;
extern Skill xSkill;
extern Social xSoc;
extern Quest xQuest;

// Editor Commands.
DECLARE_DO_FUN(do_aedit);
DECLARE_DO_FUN(do_cedit);
DECLARE_DO_FUN(do_cmdedit);
DECLARE_DO_FUN(do_gedit);
DECLARE_DO_FUN(do_hedit);
DECLARE_DO_FUN(do_medit);
DECLARE_DO_FUN(do_oedit);
DECLARE_DO_FUN(do_pedit);
DECLARE_DO_FUN(do_qedit);
DECLARE_DO_FUN(do_raedit);
DECLARE_DO_FUN(do_redit);
DECLARE_DO_FUN(do_sedit);
DECLARE_DO_FUN(do_skedit);

// General Functions
bool show_commands(Mobile* ch, char* argument);
bool show_help(Mobile* ch, char* argument);
bool edit_done(Mobile* ch);
bool show_version(Mobile* ch, char* argument);

// Area Editor Prototypes
DECLARE_OLC_FUN(aedit_show);
DECLARE_OLC_FUN(aedit_create);
DECLARE_OLC_FUN(aedit_file);
DECLARE_OLC_FUN(aedit_reset);
DECLARE_OLC_FUN(aedit_security);
DECLARE_OLC_FUN(aedit_builder);
DECLARE_OLC_FUN(aedit_levels);
DECLARE_OLC_FUN(aedit_vnums);
DECLARE_OLC_FUN(aedit_lvnum);
DECLARE_OLC_FUN(aedit_uvnum);
DECLARE_OLC_FUN(aedit_lowrange);
DECLARE_OLC_FUN(aedit_highrange);
DECLARE_OLC_FUN(aedit_faction);

// Room Editor Prototypes
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

// Object Editor Prototypes
DECLARE_OLC_FUN(oedit_show);
DECLARE_OLC_FUN(oedit_create);
DECLARE_OLC_FUN(oedit_addaffect);
DECLARE_OLC_FUN(oedit_delaffect);
DECLARE_OLC_FUN(oedit_addapply);
DECLARE_OLC_FUN(oedit_addoprog);
DECLARE_OLC_FUN(oedit_copy);

/* Mob editor. */
DECLARE_OLC_FUN(medit_show);
DECLARE_OLC_FUN(medit_group);
DECLARE_OLC_FUN(medit_copy);
DECLARE_OLC_FUN(medit_wealth);
DECLARE_OLC_FUN(medit_faction);

// Race editor.
DECLARE_OLC_FUN(raedit_show);
DECLARE_OLC_FUN(raedit_new);
DECLARE_OLC_FUN(raedit_list);
DECLARE_OLC_FUN(raedit_cmult);
DECLARE_OLC_FUN(raedit_stats);
DECLARE_OLC_FUN(raedit_maxstats);
DECLARE_OLC_FUN(raedit_skills);
DECLARE_OLC_FUN(raedit_start_loc);

// Class editor.
DECLARE_OLC_FUN(cedit_show);
DECLARE_OLC_FUN(cedit_weapon);
DECLARE_OLC_FUN(cedit_guild);
DECLARE_OLC_FUN(cedit_skillcap);
DECLARE_OLC_FUN(cedit_thac0_00);
DECLARE_OLC_FUN(cedit_thac0_32);
DECLARE_OLC_FUN(cedit_hpmin);
DECLARE_OLC_FUN(cedit_hpmax);
DECLARE_OLC_FUN(cedit_title);
DECLARE_OLC_FUN(cedit_new);
DECLARE_OLC_FUN(cedit_list);

// Social editor.
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

// Skill editor.
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

// Command editor.
DECLARE_OLC_FUN(cmdedit_show);
DECLARE_OLC_FUN(cmdedit_name);
DECLARE_OLC_FUN(cmdedit_function);
DECLARE_OLC_FUN(cmdedit_level);
DECLARE_OLC_FUN(cmdedit_list);
DECLARE_OLC_FUN(cmdedit_new);
DECLARE_OLC_FUN(cmdedit_delete);

// Group editor.
DECLARE_OLC_FUN(gedit_show);
DECLARE_OLC_FUN(gedit_name);
DECLARE_OLC_FUN(gedit_rating);
DECLARE_OLC_FUN(gedit_spell);
DECLARE_OLC_FUN(gedit_list);

// Help Editor.
DECLARE_OLC_FUN(hedit_show);
DECLARE_OLC_FUN(hedit_keyword);
DECLARE_OLC_FUN(hedit_text);
DECLARE_OLC_FUN(hedit_new);
DECLARE_OLC_FUN(hedit_level);
DECLARE_OLC_FUN(hedit_delete);
DECLARE_OLC_FUN(hedit_list);

// Quest Editor.
DECLARE_OLC_FUN(qedit_create);
DECLARE_OLC_FUN(qedit_show);
DECLARE_OLC_FUN(qedit_target);
DECLARE_OLC_FUN(qedit_upper);
DECLARE_OLC_FUN(qedit_end);
DECLARE_OLC_FUN(qedit_faction);
DECLARE_OLC_FUN(qedit_reward_reputation);
DECLARE_OLC_FUN(qedit_reward_item);

// Editors.
DECLARE_ED_FUN(ed_line_string);
DECLARE_ED_FUN(ed_line_lox_string);
DECLARE_ED_FUN(ed_desc);
DECLARE_ED_FUN(ed_bool);
DECLARE_ED_FUN(ed_skillgroup);
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

// Macros

/* Return pointers to what is being edited. */
#define EDIT_AREA(ch, area)	    ( area = (AreaData*)ch->desc->pEdit )
#define EDIT_CLASS(ch, class_)  ( class_ = (Class*)ch->desc->pEdit )
#define EDIT_CMD(ch, cmd)       ( cmd = (CmdInfo*)ch->desc->pEdit )
#define EDIT_GROUP(ch, grp)     ( grp = (SkillGroup*)ch->desc->pEdit )
#define EDIT_HELP(ch, help)     ( help = (HelpData*)ch->desc->pEdit )
#define EDIT_MOB(ch, mob)       ( mob = (MobPrototype*)ch->desc->pEdit )
#define EDIT_OBJ(ch, obj)       ( obj = (ObjPrototype*)ch->desc->pEdit )
#define EDIT_PROG(ch, code)     ( code = (MobProgCode*)ch->desc->pEdit )
#define EDIT_QUEST(ch, quest)   ( quest = (Quest*)ch->desc->pEdit )
#define EDIT_RACE(ch, race)	    ( race = (Race*)ch->desc->pEdit )
#define EDIT_ROOM(ch, room)     ( room = (RoomData*)ch->desc->pEdit )
#define EDIT_SKILL(ch, skill)   ( skill = (Skill*)ch->desc->pEdit )
#define EDIT_SOCIAL(ch, social)	( social = (Social*)ch->desc->pEdit )
#define EDIT_ENTITY(ch, room)   ( entity = (Entity*)ch->desc->pEdit )

void show_liqlist(Mobile* ch);
void show_poslist(Mobile* ch);
void show_damlist(Mobile* ch);
void show_sexlist(Mobile* ch);
void show_sizelist(Mobile* ch);

void InitScreen(Descriptor*);
char* fix_string(const char* str);
char* fix_lox_script(const char* str);

void olc_print_flags(Mobile* ch, const char* label, const struct flag_type* flag_table, FLAGS flags);
void olc_print_num(Mobile* ch, const char* label, int num);
void olc_print_range(Mobile* ch, const char* label, int num1, int num2);
void olc_print_num_str(Mobile* ch, const char* label, int num, const char* opt_str);
void olc_print_str(Mobile* ch, const char* label, const char* str); 
void olc_print_str_box(Mobile* ch, const char* label, const char* str, const char* opt_str);
void olc_print_yesno(Mobile* ch, const char* label, bool yesno);
void olc_print_text(Mobile* ch, const char* label, const char* text);

const char* olc_match_flag_default(FLAGS flags, const struct flag_type* defaults);
const char* olc_show_flags_ex(const char* label, const struct flag_type* flag_table, const struct flag_type* defaults, FLAGS flags);
const char* olc_show_flags(const char* label, const struct flag_type* flag_table, FLAGS flags);

#endif // !MUD98__OLC__OLC_H
