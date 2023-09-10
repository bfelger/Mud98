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

#include "tables.h"
#include "tablesave.h"

#include "entities/mob_prototype.h"

#include <inttypes.h>

char* flag_string(const struct flag_type*, long);

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
typedef	bool OLC_FUN		args((CharData* ch, char* argument));
#define DECLARE_OLC_FUN( fun )	OLC_FUN fun

typedef bool ED_FUN		args((char*, CharData*, char*, uintptr_t, const uintptr_t));
#define DECLARE_ED_FUN( fun )	ED_FUN fun

#define ED_FUN_DEC(blah)	bool blah (char* n_fun, CharData* ch,        \
                            char* argument, uintptr_t arg, const uintptr_t par )

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
void    aedit       args((CharData* ch, char* argument));
void    redit       args((CharData* ch, char* argument));
void    medit       args((CharData* ch, char* argument));
void    oedit       args((CharData* ch, char* argument));
void    pedit       args((CharData* ch, char* argument));
void	cedit		args((CharData* ch, char* argument));
void	raedit		args((CharData* ch, char* argument));
void	sedit		args((CharData* ch, char* argument));
void	skedit		args((CharData* ch, char* argument));
void	cmdedit		args((CharData* ch, char* argument));
void	gedit		args((CharData* ch, char* argument));
void	scedit		args((CharData* ch, char* argument));
void	hedit		args((CharData* ch, char* argument));

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
    DO_FUN* do_fun;
};

/*
 * Utils.
 */
AREA_DATA* get_vnum_area args((VNUM vnum));
AREA_DATA* get_area_data args((VNUM vnum));
int	flag_value args((const struct flag_type* flag_table, char* argument));
void add_reset args((ROOM_INDEX_DATA*, RESET_DATA*, int));
void set_editor args((DESCRIPTOR_DATA*, int, uintptr_t));

/*
 * Interpreter Table Prototypes
 */
extern const struct olc_cmd_type aedit_table[];
extern const struct olc_cmd_type cedit_table[];
extern const struct olc_cmd_type raedit_table[];
extern const struct olc_cmd_type gedit_table[];
extern const struct olc_cmd_type scedit_table[];
extern const struct olc_cmd_type hedit_table[];

/*
 * Editor Commands.
 */
DECLARE_DO_FUN(do_aedit);
DECLARE_DO_FUN(do_redit);
DECLARE_DO_FUN(do_oedit);
DECLARE_DO_FUN(do_medit);
DECLARE_DO_FUN(do_pedit);
DECLARE_DO_FUN(do_cedit);
DECLARE_DO_FUN(do_raedit);
DECLARE_DO_FUN(do_sedit);
DECLARE_DO_FUN(do_skedit);
DECLARE_DO_FUN(do_cmdedit);
DECLARE_DO_FUN(do_gedit);
DECLARE_DO_FUN(do_scedit);
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
DECLARE_OLC_FUN(redit_limpiar);

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
 * Clan editor.
 */
DECLARE_OLC_FUN(cedit_show);
DECLARE_OLC_FUN(cedit_name);
DECLARE_OLC_FUN(cedit_who);
DECLARE_OLC_FUN(cedit_god);
DECLARE_OLC_FUN(cedit_hall);
DECLARE_OLC_FUN(cedit_recall);
DECLARE_OLC_FUN(cedit_death);
DECLARE_OLC_FUN(cedit_new);
DECLARE_OLC_FUN(cedit_flags);
DECLARE_OLC_FUN(cedit_pit);

/*
 * Race editor.
 */
DECLARE_OLC_FUN(raedit_show);
DECLARE_OLC_FUN(raedit_new);
DECLARE_OLC_FUN(raedit_list);
DECLARE_OLC_FUN(raedit_cmult);
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
 * Script editor.
 */
DECLARE_OLC_FUN(scedit_show);
DECLARE_OLC_FUN(scedit_new);
DECLARE_OLC_FUN(scedit_add);
DECLARE_OLC_FUN(scedit_remove);
DECLARE_OLC_FUN(scedit_delete);
DECLARE_OLC_FUN(scedit_list);

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
DECLARE_ED_FUN(ed_direccion);
DECLARE_ED_FUN(ed_docomm);
DECLARE_ED_FUN(ed_olist);
DECLARE_ED_FUN(ed_objrecval);

/*
 * Macros
 */

/* Return pointers to what is being edited. */
#define EDIT_MOB(Ch, Mob)	( Mob = (MobPrototype *)Ch->desc->pEdit )
#define EDIT_OBJ(Ch, Obj)	( Obj = (ObjectPrototype *)Ch->desc->pEdit )
#define EDIT_ROOM(Ch, Room)	( Room = (ROOM_INDEX_DATA *)Ch->desc->pEdit )
#define EDIT_AREA(Ch, Area)	( Area = (AREA_DATA *)Ch->desc->pEdit )
#define EDIT_CLAN(Ch, Clan)	( Clan = (CLAN_TYPE *)Ch->desc->pEdit )
#define EDIT_RACE(Ch, Race)	( Race = (struct race_type *)Ch->desc->pEdit )
#define EDIT_SOCIAL(Ch, Social)	( Social = (struct social_type *)Ch->desc->pEdit )
#define EDIT_SKILL(Ch, Skill)	( Skill = (struct skill_type *)Ch->desc->pEdit )
#define EDIT_CMD(Ch, Cmd)	( Cmd = (struct cmd_type *)Ch->desc->pEdit )
#define EDIT_GROUP(Ch, Grp)	( Grp = (struct group_type *)Ch->desc->pEdit )
#define EDIT_HELP(Ch, Help)	( Help = (HELP_DATA *)Ch->desc->pEdit )
#define EDIT_PROG(Ch, Code)	( Code = (MPROG_CODE*)Ch->desc->pEdit )

/*
 * Prototypes
 */
/* mem.c - memory prototypes. */
#define ED EXTRA_DESCR_DATA
RESET_DATA*     new_reset_data      args((void));
void            free_reset_data     args((RESET_DATA* pReset));
AREA_DATA*      new_area            args((void));
void            free_area           args((AREA_DATA* pArea));
EXIT_DATA*      new_exit            args((void));
void            free_exit           args((EXIT_DATA*));
ED*             new_extra_descr	    args((void));
void            free_extra_descr    args((ED* pExtra));
ROOM_INDEX_DATA* new_room_index     args((void));
void            free_room_index     args((ROOM_INDEX_DATA* pRoom));
AFFECT_DATA*    new_affect          args((void));
void            free_affect         args((AFFECT_DATA* pAf));
SHOP_DATA*      new_shop            args((void));
void            free_shop           args((SHOP_DATA* pShop));
ObjectPrototype* new_object_prototype       args((void));
void            free_object_prototype      args((ObjectPrototype* pObj));
#undef	ED

MPROG_LIST*     new_mprog           args((void));
void            free_mprog          args((MPROG_LIST* mp));

MPROG_CODE*     new_mpcode          args((void));
void            free_mpcode         args((MPROG_CODE* pMcode));

void		    show_liqlist		args((CharData* ch));
void		    show_poslist		args((CharData* ch));
void		    show_damlist		args((CharData* ch));
void		    show_sexlist		args((CharData* ch));
void		    show_sizelist		args((CharData* ch));

extern		    ROOM_INDEX_DATA 	xRoom;
extern		    MobPrototype 		xMob;
extern		    ObjectPrototype		xObj;

extern void     InitScreen		    args((DESCRIPTOR_DATA*));

#endif // !MUD98__OLC_H
