/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@hypercube.org)                                *
 *      Gabrielle Taylor (gtaylor@hypercube.org)                           *
 *      Brian Moore (zump@rom.org)                                         *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

#include "merc.h"

#include "db.h"
#include "lookup.h"
#include "tables.h"

#include "entities/mob_prototype.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#ifndef _MSC_VER
#include <sys/time.h>
#endif

extern int flag_lookup(const char* name, const struct flag_type* flag_table);

/* values for db2.c */
struct social_type* social_table;
int social_count;

/* snarf a socials file */
void load_socials(FILE* fp)
{
    extern int maxSocial;

    for (;;) {
        struct social_type social = { 0 };
        char* temp;
        /* clear social */
        social.char_no_arg = NULL;
        social.others_no_arg = NULL;
        social.char_found = NULL;
        social.others_found = NULL;
        social.vict_found = NULL;
        social.char_not_found = NULL;
        social.char_auto = NULL;
        social.others_auto = NULL;

        temp = fread_word(fp);
        if (!strcmp(temp, "#0")) return; /* done */
#if defined(social_debug)
        else
            fprintf(stderr, "%s\n\r", temp);
#endif

        social.name = str_dup(temp);
        fread_to_eol(fp);

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_no_arg = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_no_arg = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_found = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_found = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.vict_found = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.vict_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_not_found = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_not_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_auto = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_auto = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_auto = NULL;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_auto = temp;

        social_table[social_count] = social;
        social_count++;
        maxSocial++;
    }
    return;
}

/*
 * Snarf an obj section. new style
 */
void load_objects(FILE* fp)
{
    OBJ_INDEX_DATA* pObjIndex;

    if (!area_last)   /* OLC */
    {
        bug("Load_objects: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        VNUM vnum;
        char letter;
        int iHash;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_objects: # not found.", 0);
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0) break;

        fBootDb = false;
        if (get_obj_index(vnum) != NULL) {
            bug("Load_objects: vnum %"PRVNUM" duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        pObjIndex = alloc_perm(sizeof(*pObjIndex));
        pObjIndex->vnum = vnum;
        pObjIndex->area = area_last;    // OLC
        pObjIndex->new_format = true;
        pObjIndex->reset_num = 0;
        newobjs++;
        pObjIndex->name = fread_string(fp);
        pObjIndex->short_descr = fread_string(fp);
        pObjIndex->description = fread_string(fp);
        pObjIndex->material = fread_string(fp);

        CHECK_POS(pObjIndex->item_type, (int16_t)item_lookup(fread_word(fp)), "item_type");
        pObjIndex->extra_flags = fread_flag(fp);
        pObjIndex->wear_flags = fread_flag(fp);
        switch (pObjIndex->item_type) {
        case ITEM_WEAPON:
            pObjIndex->value[0] = weapon_type(fread_word(fp));
            pObjIndex->value[1] = fread_number(fp);
            pObjIndex->value[2] = fread_number(fp);
            pObjIndex->value[3] = attack_lookup(fread_word(fp));
            pObjIndex->value[4] = fread_flag(fp);
            break;
        case ITEM_CONTAINER:
            pObjIndex->value[0] = fread_number(fp);
            pObjIndex->value[1] = fread_flag(fp);
            pObjIndex->value[2] = fread_number(fp);
            pObjIndex->value[3] = fread_number(fp);
            pObjIndex->value[4] = fread_number(fp);
            break;
        case ITEM_DRINK_CON:
        case ITEM_FOUNTAIN:
            pObjIndex->value[0] = fread_number(fp);
            pObjIndex->value[1] = fread_number(fp);
            CHECK_POS(pObjIndex->value[2], liq_lookup(fread_word(fp)), "liq_lookup");
            pObjIndex->value[3] = fread_number(fp);
            pObjIndex->value[4] = fread_number(fp);
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            pObjIndex->value[0] = fread_number(fp);
            pObjIndex->value[1] = fread_number(fp);
            pObjIndex->value[2] = fread_number(fp);
            pObjIndex->value[3] = skill_lookup(fread_word(fp));
            pObjIndex->value[4] = fread_number(fp);
            break;
        case ITEM_POTION:
        case ITEM_PILL:
        case ITEM_SCROLL:
            pObjIndex->value[0] = fread_number(fp);
            pObjIndex->value[1] = skill_lookup(fread_word(fp));
            pObjIndex->value[2] = skill_lookup(fread_word(fp));
            pObjIndex->value[3] = skill_lookup(fread_word(fp));
            pObjIndex->value[4] = skill_lookup(fread_word(fp));
            break;
        default:
            pObjIndex->value[0] = fread_flag(fp);
            pObjIndex->value[1] = fread_flag(fp);
            pObjIndex->value[2] = fread_flag(fp);
            pObjIndex->value[3] = fread_flag(fp);
            pObjIndex->value[4] = fread_flag(fp);
            break;
        }
        pObjIndex->level = (int16_t)fread_number(fp);
        pObjIndex->weight = (int16_t)fread_number(fp);
        pObjIndex->cost = fread_number(fp);

        /* condition */
        letter = fread_letter(fp);
        switch (letter) {
        case ('P'):
            pObjIndex->condition = 100;
            break;
        case ('G'):
            pObjIndex->condition = 90;
            break;
        case ('A'):
            pObjIndex->condition = 75;
            break;
        case ('W'):
            pObjIndex->condition = 50;
            break;
        case ('D'):
            pObjIndex->condition = 25;
            break;
        case ('B'):
            pObjIndex->condition = 10;
            break;
        case ('R'):
            pObjIndex->condition = 0;
            break;
        default:
            pObjIndex->condition = 100;
            break;
        }

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'A') {
                AFFECT_DATA* paf;

                paf = alloc_perm(sizeof(*paf));
                paf->where = TO_OBJECT;
                paf->type = -1;
                paf->level = pObjIndex->level;
                paf->duration = -1;
                paf->location = (int16_t)fread_number(fp);
                paf->modifier = (int16_t)fread_number(fp);
                paf->bitvector = 0;
                paf->next = pObjIndex->affected;
                pObjIndex->affected = paf;
                top_affect++;
            }

            else if (letter == 'F') {
                AFFECT_DATA* paf;

                paf = alloc_perm(sizeof(*paf));
                letter = fread_letter(fp);
                switch (letter) {
                case 'A':
                    paf->where = TO_AFFECTS;
                    break;
                case 'I':
                    paf->where = TO_IMMUNE;
                    break;
                case 'R':
                    paf->where = TO_RESIST;
                    break;
                case 'V':
                    paf->where = TO_VULN;
                    break;
                default:
                    bug("Load_objects: Bad where on flag set.", 0);
                    exit(1);
                }
                paf->type = -1;
                paf->level = pObjIndex->level;
                paf->duration = -1;
                paf->location = (int16_t)fread_number(fp);
                paf->modifier = (int16_t)fread_number(fp);
                paf->bitvector = fread_flag(fp);
                paf->next = pObjIndex->affected;
                pObjIndex->affected = paf;
                top_affect++;
            }

            else if (letter == 'E') {
                EXTRA_DESCR_DATA* ed;

                ed = alloc_perm(sizeof(*ed));
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ed->next = pObjIndex->extra_descr;
                pObjIndex->extra_descr = ed;
                top_ed++;
            }

            else {
                ungetc(letter, fp);
                break;
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        pObjIndex->next = obj_index_hash[iHash];
        obj_index_hash[iHash] = pObjIndex;
        top_obj_index++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;   // OLC
        assign_area_vnum(vnum);                                     // OLC
    }

    return;
}

/*****************************************************************************
 Name:	        convert_objects
 Purpose:	Converts all old format objects to new format
 Called by:	boot_db (db.c).
 Note:          Loops over all resets to find the level of the mob
                loaded before the object to determine the level of
                the object.
		It might be better to update the levels in load_resets().
		This function is not pretty.. Sorry about that :)
 Author:        Hugin
 ****************************************************************************/
void convert_objects(void)
{
    VNUM vnum;
    AREA_DATA* pArea;
    RESET_DATA* pReset;
    MobPrototype* pMob = NULL;
    OBJ_INDEX_DATA* pObj;
    ROOM_INDEX_DATA* pRoom;

    if (newobjs == top_obj_index) return; /* all objects in new format */

    for (pArea = area_first; pArea; pArea = pArea->next) {
        for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++) {
            if (!(pRoom = get_room_index(vnum))) continue;

            for (pReset = pRoom->reset_first; pReset; pReset = pReset->next) {
                switch (pReset->command) {
                case 'M':
                    if (!(pMob = get_mob_prototype(pReset->arg1)))
                        bug("Convert_objects: 'M': bad vnum %"PRVNUM".", pReset->arg1);
                    break;

                case 'O':
                    if (!(pObj = get_obj_index(pReset->arg1))) {
                        bug("Convert_objects: 'O': bad vnum %"PRVNUM".", pReset->arg1);
                        break;
                    }

                    if (pObj->new_format)
                        continue;

                    if (!pMob) {
                        bug("Convert_objects: 'O': No mob reset yet.", 0);
                        break;
                    }

                    pObj->level = pObj->level < 1 ? pMob->level - 2
                        : UMIN(pObj->level, pMob->level - 2);
                    break;

                case 'P':
                {
                    OBJ_INDEX_DATA* pObjTo;

                    if (!(pObj = get_obj_index(pReset->arg1))) {
                        bug("Convert_objects: 'P': bad vnum %"PRVNUM".", pReset->arg1);
                        break;
                    }

                    if (pObj->new_format)
                        continue;

                    if (!(pObjTo = get_obj_index(pReset->arg3))) {
                        bug("Convert_objects: 'P': bad vnum %"PRVNUM".", pReset->arg3);
                        break;
                    }

                    pObj->level = pObj->level < 1 ? pObjTo->level
                        : UMIN(pObj->level, pObjTo->level);
                }
                break;

                case 'G':
                case 'E':
                    if (!(pObj = get_obj_index(pReset->arg1))) {
                        bug("Convert_objects: 'E' or 'G': bad vnum %"PRVNUM".", pReset->arg1);
                        break;
                    }

                    if (!pMob) {
                        bug("Convert_objects: 'E' or 'G': null mob for vnum %"PRVNUM".",
                            pReset->arg1);
                        break;
                    }

                    if (pObj->new_format)
                        continue;

                    if (pMob->pShop) {
                        switch (pObj->item_type) {
                        default:
                            pObj->level = UMAX(0, pObj->level);
                            break;
                        case ITEM_PILL:
                        case ITEM_POTION:
                            pObj->level = UMAX(5, pObj->level);
                            break;
                        case ITEM_SCROLL:
                        case ITEM_ARMOR:
                        case ITEM_WEAPON:
                            pObj->level = UMAX(10, pObj->level);
                            break;
                        case ITEM_WAND:
                        case ITEM_TREASURE:
                            pObj->level = UMAX(15, pObj->level);
                            break;
                        case ITEM_STAFF:
                            pObj->level = UMAX(20, pObj->level);
                            break;
                        }
                    }
                    else
                        pObj->level = pObj->level < 1 ? pMob->level
                        : UMIN(pObj->level, pMob->level);
                    break;
                } /* switch ( pReset->command ) */
            }
        }
    }

    /* do the conversion: */

    for (pArea = area_first; pArea; pArea = pArea->next)
        for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
            if ((pObj = get_obj_index(vnum)))
                if (!pObj->new_format)
                    convert_object(pObj);

    return;
}

/*****************************************************************************
 Name:		convert_object
 Purpose:	Converts an old_format obj to new_format
 Called by:	convert_objects (db2.c).
 Note:          Dug out of create_obj (db.c)
 Author:        Hugin
 ****************************************************************************/
void convert_object(OBJ_INDEX_DATA* pObjIndex)
{
    LEVEL level;
    int number, type;  /* for dice-conversion */

    if (!pObjIndex || pObjIndex->new_format) return;

    level = pObjIndex->level;

    pObjIndex->level = UMAX(0, pObjIndex->level); /* just to be sure */
    pObjIndex->cost = 10 * level;

    switch (pObjIndex->item_type) {
    default:
        bug("Obj_convert: vnum %"PRVNUM" bad type.", pObjIndex->item_type);
        break;

    case ITEM_LIGHT:
    case ITEM_TREASURE:
    case ITEM_FURNITURE:
    case ITEM_TRASH:
    case ITEM_CONTAINER:
    case ITEM_DRINK_CON:
    case ITEM_KEY:
    case ITEM_FOOD:
    case ITEM_BOAT:
    case ITEM_CORPSE_NPC:
    case ITEM_CORPSE_PC:
    case ITEM_FOUNTAIN:
    case ITEM_MAP:
    case ITEM_CLOTHING:
    case ITEM_SCROLL:
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        pObjIndex->value[2] = pObjIndex->value[1];
        break;

    case ITEM_WEAPON:

	    /*
	     * The conversion below is based on the values generated
	     * in one_hit() (fight.c).  Since I don't want a lvl 50 
	     * weapon to do 15d3 damage, the min value will be below
	     * the one in one_hit, and to make up for it, I've made 
	     * the max value higher.
	     * (I don't want 15d2 because this will hardly ever roll
	     * 15 or 30, it will only roll damage close to 23.
	     * I can't do 4d8+11, because one_hit there is no dice-
	     * bounus value to set...)
	     *
	     * The conversion below gives:

	     level:   dice      min      max      mean
	       1:     1d8      1( 2)    8( 7)     5( 5)
	       2:     2d5      2( 3)   10( 8)     6( 6)
	       3:     2d5      2( 3)   10( 8)     6( 6)
	       5:     2d6      2( 3)   12(10)     7( 7)
	      10:     4d5      4( 5)   20(14)    12(10)
	      20:     5d5      5( 7)   25(21)    15(14)
	      30:     5d7      5(10)   35(29)    20(20)
	      50:     5d11     5(15)   55(44)    30(30)

	     */

        number = UMIN(level / 4 + 1, 5);
        type = (level + 7) / number;

        pObjIndex->value[1] = number;
        pObjIndex->value[2] = type;
        break;

    case ITEM_ARMOR:
        pObjIndex->value[0] = level / 5 + 3;
        pObjIndex->value[1] = pObjIndex->value[0];
        pObjIndex->value[2] = pObjIndex->value[0];
        break;

    case ITEM_POTION:
    case ITEM_PILL:
        break;

    case ITEM_MONEY:
        pObjIndex->value[0] = pObjIndex->cost;
        break;
    }

    pObjIndex->new_format = true;
    ++newobjs;

    return;
}

