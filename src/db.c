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

#include "comm.h"
#include "db.h"
#include "lookup.h"
#include "music.h"
#include "pcg_basic.h"
#include "recycle.h"
#include "strings.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifndef _MSC_VER 
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#endif

/* externals for counting purposes */
extern OBJ_DATA* obj_free;
extern CHAR_DATA* char_free;
extern DESCRIPTOR_DATA* descriptor_free;
extern PC_DATA* pcdata_free;
extern AFFECT_DATA* affect_free;

/*
 * Globals.
 */
HELP_DATA* help_first;
HELP_DATA* help_last;

SHOP_DATA* shop_first;
SHOP_DATA* shop_last;

extern NOTE_DATA* note_free;

char bug_buf[2 * MAX_INPUT_LENGTH];
CHAR_DATA* char_list;
char* help_greeting;
char log_buf[2 * MAX_INPUT_LENGTH];
KILL_DATA kill_table[MAX_LEVEL];
extern NOTE_DATA* note_list;
OBJ_DATA* object_list;
TIME_INFO_DATA time_info;
WEATHER_DATA weather_info;

int16_t gsn_backstab;
int16_t gsn_dodge;
int16_t gsn_envenom;
int16_t gsn_hide;
int16_t gsn_peek;
int16_t gsn_pick_lock;
int16_t gsn_sneak;
int16_t gsn_steal;

int16_t gsn_disarm;
int16_t gsn_enhanced_damage;
int16_t gsn_kick;
int16_t gsn_parry;
int16_t gsn_rescue;
int16_t gsn_second_attack;
int16_t gsn_third_attack;

int16_t gsn_blindness;
int16_t gsn_charm_person;
int16_t gsn_curse;
int16_t gsn_invis;
int16_t gsn_mass_invis;
int16_t gsn_poison;
int16_t gsn_plague;
int16_t gsn_sleep;
int16_t gsn_sanctuary;
int16_t gsn_fly;
/* new gsns */

int16_t gsn_axe;
int16_t gsn_dagger;
int16_t gsn_flail;
int16_t gsn_mace;
int16_t gsn_polearm;
int16_t gsn_shield_block;
int16_t gsn_spear;
int16_t gsn_sword;
int16_t gsn_whip;

int16_t gsn_bash;
int16_t gsn_berserk;
int16_t gsn_dirt;
int16_t gsn_hand_to_hand;
int16_t gsn_trip;

int16_t gsn_fast_healing;
int16_t gsn_haggle;
int16_t gsn_lore;
int16_t gsn_meditation;

int16_t gsn_scrolls;
int16_t gsn_staves;
int16_t gsn_wands;
int16_t gsn_recall;

/*
 * Locals.
 */
MOB_INDEX_DATA* mob_index_hash[MAX_KEY_HASH];
OBJ_INDEX_DATA* obj_index_hash[MAX_KEY_HASH];
ROOM_INDEX_DATA* room_index_hash[MAX_KEY_HASH];
char* string_hash[MAX_KEY_HASH];

AREA_DATA* area_first;
AREA_DATA* area_last;

char* string_space;
char* top_string;
char str_empty[1];

int top_affect;
int top_area;
int top_ed;
int top_exit;
int top_help;
int top_mob_index;
int top_obj_index;
int top_reset;
int top_room;
int top_shop;
int mobile_count = 0;
int newmobs = 0;
int newobjs = 0;

/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
#define MAX_STRING     1413120
#define MAX_PERM_BLOCK 131072
#define MAX_MEM_LIST   11

void* rgFreeList[MAX_MEM_LIST];
const size_t rgSizeList[MAX_MEM_LIST]
    = {16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768 - 64};

int nAllocString;
size_t sAllocString;
int nAllocPerm;
size_t sAllocPerm;

/*
 * Semi-locals.
 */
bool fBootDb;
FILE* fpArea;
char strArea[MAX_INPUT_LENGTH];

/*
 * Local booting procedures.
 */
void init_mm args((void));
void load_area args((FILE * fp));
void load_helps args((FILE * fp));
void load_old_mob args((FILE * fp));
void load_mobiles args((FILE * fp));
void load_old_obj args((FILE * fp));
void load_objects args((FILE * fp));
void load_resets args((FILE * fp));
void load_rooms args((FILE * fp));
void load_shops args((FILE * fp));
void load_socials args((FILE * fp));
void load_specials args((FILE * fp));
void load_notes args((void));
void load_bans args((void));

void fix_exits args((void));

void reset_area args((AREA_DATA * pArea));

/*
 * Big mama top level function.
 */
void boot_db(void)
{
    /*
     * Init some data space stuff.
     */
    {
        if ((string_space = calloc(1, MAX_STRING)) == NULL) {
            bug("Boot_db: can't alloc %d string space.", MAX_STRING);
            exit(1);
        }
        top_string = string_space;
        fBootDb = true;
    }

    /*
     * Init random number generator.
     */
    {
        init_mm();
    }

    /*
     * Set time and weather.
     */
    {
        long lhour = (long)((current_time - 1684281600) 
            / (PULSE_TICK / PULSE_PER_SECOND));
        long lday = lhour / 24;
        long lmonth = lday / 35;

        time_info.hour = lhour % 24;
        time_info.day = lday % 35;
        time_info.month = lmonth % 17;
        time_info.year = lmonth / 17;

        if (time_info.hour < 5)
            weather_info.sunlight = SUN_DARK;
        else if (time_info.hour < 6)
            weather_info.sunlight = SUN_RISE;
        else if (time_info.hour < 19)
            weather_info.sunlight = SUN_LIGHT;
        else if (time_info.hour < 20)
            weather_info.sunlight = SUN_SET;
        else
            weather_info.sunlight = SUN_DARK;

        weather_info.change = 0;
        weather_info.mmhg = 960;
        if (time_info.month >= 7 && time_info.month <= 12)
            weather_info.mmhg += number_range(1, 50);
        else
            weather_info.mmhg += number_range(1, 80);

        if (weather_info.mmhg <= 980)
            weather_info.sky = SKY_LIGHTNING;
        else if (weather_info.mmhg <= 1000)
            weather_info.sky = SKY_RAINING;
        else if (weather_info.mmhg <= 1020)
            weather_info.sky = SKY_CLOUDY;
        else
            weather_info.sky = SKY_CLOUDLESS;
    }

    /*
     * Assign gsn's for skills which have them.
     */
    {
        int sn;

        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].pgsn != NULL) *skill_table[sn].pgsn = sn;
        }
    }

    /*
     * Read in all the area files.
     */
    {
        FILE* fpList;
        char area_list[256];
        char area_file[256];
        sprintf(area_list, "%s%s", area_dir, AREA_LIST);
        if ((fpList = fopen(area_list, "r")) == NULL) {
            perror(area_list);
            exit(1);
        }

        for (;;) {
            strcpy(strArea, fread_word(fpList));
            if (strArea[0] == '$') break;

            if (strArea[0] == '-') {
                fpArea = stdin; 
            }
            else {
                sprintf(area_file, "%s%s", area_dir, strArea);
                if ((fpArea = fopen(area_file, "r")) == NULL) {
                    perror(area_file);
                    exit(1);
                }
            }

            for (;;) {
                char* word;

                if (fread_letter(fpArea) != '#') {
                    bug("Boot_db: # not found.", 0);
                    exit(1);
                }

                word = fread_word(fpArea);

                if (word[0] == '$')
                    break;
                else if (!str_cmp(word, "AREA"))
                    load_area(fpArea);
                else if (!str_cmp(word, "HELPS"))
                    load_helps(fpArea);
                else if (!str_cmp(word, "MOBOLD"))
                    load_old_mob(fpArea);
                else if (!str_cmp(word, "MOBILES"))
                    load_mobiles(fpArea);
                else if (!str_cmp(word, "OBJOLD"))
                    load_old_obj(fpArea);
                else if (!str_cmp(word, "OBJECTS"))
                    load_objects(fpArea);
                else if (!str_cmp(word, "RESETS"))
                    load_resets(fpArea);
                else if (!str_cmp(word, "ROOMS"))
                    load_rooms(fpArea);
                else if (!str_cmp(word, "SHOPS"))
                    load_shops(fpArea);
                else if (!str_cmp(word, "SOCIALS"))
                    load_socials(fpArea);
                else if (!str_cmp(word, "SPECIALS"))
                    load_specials(fpArea);
                else {
                    bug("Boot_db: bad section name.", 0);
                    exit(1);
                }
            }

            if (fpArea != stdin) fclose(fpArea);
            fpArea = NULL;
        }
        fclose(fpList);
    }

    /*
     * Fix up exits.
     * Declare db booting over.
     * Reset all areas once.
     * Load up the songs, notes and ban files.
     */
    {
        fix_exits();
        fBootDb = false;
        area_update();
        load_notes();
        load_bans();
        load_songs();
    }

    return;
}

/*
 * Snarf an 'area' header line.
 */
void load_area(FILE* fp)
{
    AREA_DATA* pArea;

    pArea = alloc_perm(sizeof(*pArea));
    pArea->reset_first = NULL;
    pArea->reset_last = NULL;
    pArea->file_name = fread_string(fp);
    pArea->name = fread_string(fp);
    pArea->credits = fread_string(fp);
    pArea->min_vnum = fread_number(fp);
    pArea->max_vnum = fread_number(fp);
    pArea->age = 15;
    pArea->nplayer = 0;
    pArea->empty = false;

    if (area_first == NULL) area_first = pArea;
    if (area_last != NULL) area_last->next = pArea;
    area_last = pArea;
    pArea->next = NULL;

    top_area++;
    return;
}

/*
 * Snarf a help section.
 */
void load_helps(FILE* fp)
{
    HELP_DATA* pHelp;

    for (;;) {
        if ((pHelp = alloc_perm(sizeof(*pHelp))) == NULL) {
            bug("load_helps: can't alloc helps.");
            exit(1);
        }
        pHelp->level = fread_number(fp);
        pHelp->keyword = fread_string(fp);
        if (pHelp->keyword[0] == '$') break;
        pHelp->text = fread_string(fp);

        if (!str_cmp(pHelp->keyword, "greeting")) help_greeting = pHelp->text;

        if (help_first == NULL)
            help_first = pHelp;
        if (help_last != NULL)
            help_last->next = pHelp;

        help_last = pHelp;
        if (pHelp)
            pHelp->next = NULL;
        top_help++;
    }

    return;
}

/*
 * Snarf a mob section.  old style
 */
void load_old_mob(FILE* fp)
{
    MOB_INDEX_DATA* pMobIndex;
    /* for race updating */
    int race;
    char name[MAX_STRING_LENGTH];

    for (;;) {
        int16_t vnum;
        char letter;
        int iHash;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_mobiles: # not found.", 0);
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0) break;

        fBootDb = false;
        if (get_mob_index(vnum) != NULL) {
            bug("Load_mobiles: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        pMobIndex = alloc_perm(sizeof(*pMobIndex));
        pMobIndex->vnum = vnum;
        pMobIndex->new_format = false;
        pMobIndex->player_name = fread_string(fp);
        pMobIndex->short_descr = fread_string(fp);
        pMobIndex->long_descr = fread_string(fp);
        pMobIndex->description = fread_string(fp);

        pMobIndex->long_descr[0] = UPPER(pMobIndex->long_descr[0]);
        pMobIndex->description[0] = UPPER(pMobIndex->description[0]);

        pMobIndex->act = fread_flag(fp) | ACT_IS_NPC;
        pMobIndex->affected_by = fread_flag(fp);
        pMobIndex->pShop = NULL;
        pMobIndex->alignment = fread_number(fp);
        letter = fread_letter(fp);
        pMobIndex->level = fread_number(fp);

        /*
         * The unused stuff is for imps who want to use the old-style
         * stats-in-files method.
         */
        fread_number(fp); /* Unused */
        fread_number(fp); /* Unused */
        fread_number(fp); /* Unused */
        /* 'd'		*/ fread_letter(fp); /* Unused */
        fread_number(fp); /* Unused */
        /* '+'		*/ fread_letter(fp); /* Unused */
        fread_number(fp); /* Unused */
        fread_number(fp); /* Unused */
        /* 'd'		*/ fread_letter(fp); /* Unused */
        fread_number(fp); /* Unused */
        /* '+'		*/ fread_letter(fp); /* Unused */
        fread_number(fp); /* Unused */
        pMobIndex->wealth = fread_number(fp) / 20;
        /* xp can't be used! */ fread_number(fp); /* Unused */
        pMobIndex->start_pos = fread_number(fp); /* Unused */
        pMobIndex->default_pos = fread_number(fp); /* Unused */

        if (pMobIndex->start_pos < POS_SLEEPING)
            pMobIndex->start_pos = POS_STANDING;
        if (pMobIndex->default_pos < POS_SLEEPING)
            pMobIndex->default_pos = POS_STANDING;

        /*
         * Back to meaningful values.
         */
        pMobIndex->sex = fread_number(fp);

        /* compute the race BS */
        one_argument(pMobIndex->player_name, name);

        if (name[0] == '\0' || (race = race_lookup(name)) == 0) {
            /* fill in with blanks */
            pMobIndex->race = race_lookup("human");
            pMobIndex->off_flags
                = OFF_DODGE | OFF_DISARM | OFF_TRIP | ASSIST_VNUM;
            pMobIndex->imm_flags = 0;
            pMobIndex->res_flags = 0;
            pMobIndex->vuln_flags = 0;
            pMobIndex->form
                = FORM_EDIBLE | FORM_SENTIENT | FORM_BIPED | FORM_MAMMAL;
            pMobIndex->parts = PART_HEAD | PART_ARMS | PART_LEGS | PART_HEART
                               | PART_BRAINS | PART_GUTS;
        }
        else {
            pMobIndex->race = race;
            pMobIndex->off_flags = OFF_DODGE | OFF_DISARM | OFF_TRIP
                                   | ASSIST_RACE | race_table[race].off;
            pMobIndex->imm_flags = race_table[race].imm;
            pMobIndex->res_flags = race_table[race].res;
            pMobIndex->vuln_flags = race_table[race].vuln;
            pMobIndex->form = race_table[race].form;
            pMobIndex->parts = race_table[race].parts;
        }

        if (letter != 'S') {
            bug("Load_mobiles: vnum %d non-S.", vnum);
            exit(1);
        }

        iHash = vnum % MAX_KEY_HASH;
        pMobIndex->next = mob_index_hash[iHash];
        mob_index_hash[iHash] = pMobIndex;
        top_mob_index++;
        kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL - 1)].number++;
    }

    return;
}

/*
 * Snarf an obj section.  old style
 */
void load_old_obj(FILE* fp)
{
    OBJ_INDEX_DATA* pObjIndex;

    for (;;) {
        int16_t vnum;
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
            bug("Load_objects: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        pObjIndex = alloc_perm(sizeof(*pObjIndex));
        pObjIndex->vnum = vnum;
        pObjIndex->new_format = false;
        pObjIndex->reset_num = 0;
        pObjIndex->name = fread_string(fp);
        pObjIndex->short_descr = fread_string(fp);
        pObjIndex->description = fread_string(fp);
        /* Action description */ fread_string(fp);

        pObjIndex->short_descr[0] = LOWER(pObjIndex->short_descr[0]);
        pObjIndex->description[0] = UPPER(pObjIndex->description[0]);
        pObjIndex->material = str_dup("");

        pObjIndex->item_type = fread_number(fp);
        pObjIndex->extra_flags = fread_flag(fp);
        pObjIndex->wear_flags = fread_flag(fp);
        pObjIndex->value[0] = fread_number(fp);
        pObjIndex->value[1] = fread_number(fp);
        pObjIndex->value[2] = fread_number(fp);
        pObjIndex->value[3] = fread_number(fp);
        pObjIndex->value[4] = 0;
        pObjIndex->level = 0;
        pObjIndex->condition = 100;
        pObjIndex->weight = fread_number(fp);
        pObjIndex->cost = fread_number(fp); /* Unused */
        /* Cost per day */ fread_number(fp);

        if (pObjIndex->item_type == ITEM_WEAPON) {
            if (is_name("two", pObjIndex->name)
                || is_name("two-handed", pObjIndex->name)
                || is_name("claymore", pObjIndex->name))
                SET_BIT(pObjIndex->value[4], WEAPON_TWO_HANDS);
        }

        for (;;) {
            char letter;

            letter = fread_letter(fp);

            if (letter == 'A') {
                AFFECT_DATA* paf;

                paf = alloc_perm(sizeof(*paf));
                paf->where = TO_OBJECT;
                paf->type = -1;
                paf->level = 20; /* RT temp fix */
                paf->duration = -1;
                paf->location = fread_number(fp);
                paf->modifier = fread_number(fp);
                paf->bitvector = 0;
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

        /* fix armors */
        if (pObjIndex->item_type == ITEM_ARMOR) {
            pObjIndex->value[1] = pObjIndex->value[0];
            pObjIndex->value[2] = pObjIndex->value[1];
        }

        /*
         * Translate spell "slot numbers" to internal "skill numbers."
         */
        switch (pObjIndex->item_type) {
        case ITEM_PILL:
        case ITEM_POTION:
        case ITEM_SCROLL:
            pObjIndex->value[1] = slot_lookup(pObjIndex->value[1]);
            pObjIndex->value[2] = slot_lookup(pObjIndex->value[2]);
            pObjIndex->value[3] = slot_lookup(pObjIndex->value[3]);
            pObjIndex->value[4] = slot_lookup(pObjIndex->value[4]);
            break;

        case ITEM_STAFF:
        case ITEM_WAND:
            pObjIndex->value[3] = slot_lookup(pObjIndex->value[3]);
            break;
        }

        iHash = vnum % MAX_KEY_HASH;
        pObjIndex->next = obj_index_hash[iHash];
        obj_index_hash[iHash] = pObjIndex;
        top_obj_index++;
    }

    return;
}

/*
 * Snarf a reset section.
 */
void load_resets(FILE* fp)
{
    RESET_DATA* pReset;

    if (area_last == NULL) {
        bug("Load_resets: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        ROOM_INDEX_DATA* pRoomIndex;
        EXIT_DATA* pexit;
        char letter;
        OBJ_INDEX_DATA* temp_index;

        if ((letter = fread_letter(fp)) == 'S') break;

        if (letter == '*') {
            fread_to_eol(fp);
            continue;
        }

        if ((pReset = alloc_perm(sizeof(*pReset))) == NULL) {
            bug("load_resets: can't alloc resets.");
            exit(1);
        }
        pReset->command = letter;
        /* if_flag */ fread_number(fp);
        pReset->arg1 = fread_number(fp);
        pReset->arg2 = fread_number(fp);
        pReset->arg3 = (letter == 'G' || letter == 'R') ? 0 : fread_number(fp);
        pReset->arg4 = (letter == 'P' || letter == 'M') ? fread_number(fp) : 0;
        fread_to_eol(fp);

        /*
         * Validate parameters.
         * We're calling the index functions for the side effect.
         */
        switch (letter) {
        default:
            bug("Load_resets: bad command '%c'.", letter);
            exit(1);
            break;

        case 'M':
            get_mob_index(pReset->arg1);
            get_room_index(pReset->arg3);
            break;

        case 'O':
            temp_index = get_obj_index(pReset->arg1);
            temp_index->reset_num++;
            get_room_index(pReset->arg3);
            break;

        case 'P':
            temp_index = get_obj_index(pReset->arg1);
            temp_index->reset_num++;
            get_obj_index(pReset->arg3);
            break;

        case 'G':
        case 'E':
            temp_index = get_obj_index(pReset->arg1);
            temp_index->reset_num++;
            break;

        case 'D':
            pRoomIndex = get_room_index(pReset->arg1);

            if (pReset->arg2 < 0 || pReset->arg2 > 5
                || (pexit = pRoomIndex->exit[pReset->arg2]) == NULL
                || !IS_SET(pexit->exit_info, EX_ISDOOR)) {
                bug("Load_resets: 'D': exit %d not door.", pReset->arg2);
                exit(1);
            }

            if (pReset->arg3 < 0 || pReset->arg3 > 2) {
                bug("Load_resets: 'D': bad 'locks': %d.", pReset->arg3);
                exit(1);
            }

            break;

        case 'R':
            pRoomIndex = get_room_index(pReset->arg1);

            if (pReset->arg2 < 0 || pReset->arg2 > 6) {
                bug("Load_resets: 'R': bad exit %d.", pReset->arg2);
                exit(1);
            }

            break;
        }

        if (area_last->reset_first == NULL) area_last->reset_first = pReset;
        if (area_last->reset_last != NULL) area_last->reset_last->next = pReset;

        area_last->reset_last = pReset;
        if (pReset)
            pReset->next = NULL;
        top_reset++;
    }

    return;
}

/*
 * Snarf a room section.
 */
void load_rooms(FILE* fp)
{
    ROOM_INDEX_DATA* pRoomIndex;

    if (area_last == NULL) {
        bug("Load_resets: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        int16_t vnum;
        char letter;
        int door;
        int iHash;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_rooms: # not found.", 0);
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0) break;

        fBootDb = false;
        if (get_room_index(vnum) != NULL) {
            bug("Load_rooms: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        pRoomIndex = alloc_perm(sizeof(*pRoomIndex));
        pRoomIndex->owner = str_dup("");
        pRoomIndex->people = NULL;
        pRoomIndex->contents = NULL;
        pRoomIndex->extra_descr = NULL;
        pRoomIndex->area = area_last;
        pRoomIndex->vnum = vnum;
        pRoomIndex->name = fread_string(fp);
        pRoomIndex->description = fread_string(fp);
        /* Area number */ fread_number(fp);
        pRoomIndex->room_flags = fread_flag(fp);
        /* horrible hack */
        if (3000 <= vnum && vnum < 3400)
            SET_BIT(pRoomIndex->room_flags, ROOM_LAW);
        pRoomIndex->sector_type = fread_number(fp);
        pRoomIndex->light = 0;
        for (door = 0; door <= 5; door++) pRoomIndex->exit[door] = NULL;

        /* defaults */
        pRoomIndex->heal_rate = 100;
        pRoomIndex->mana_rate = 100;

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'S') break;

            if (letter == 'H') /* healing room */
                pRoomIndex->heal_rate = fread_number(fp);

            else if (letter == 'M') /* mana room */
                pRoomIndex->mana_rate = fread_number(fp);

            else if (letter == 'C') /* clan */
            {
                if (pRoomIndex->clan) {
                    bug("Load_rooms: duplicate clan fields.", 0);
                    exit(1);
                }
                pRoomIndex->clan = clan_lookup(fread_string(fp));
            }

            else if (letter == 'D') {
                EXIT_DATA* pexit;
                int locks;

                door = fread_number(fp);
                if (door < 0 || door > 5) {
                    bug("Fread_rooms: vnum %d has bad door number.", vnum);
                    exit(1);
                }

                pexit = alloc_perm(sizeof(*pexit));
                pexit->description = fread_string(fp);
                pexit->keyword = fread_string(fp);
                pexit->exit_info = 0;
                locks = fread_number(fp);
                pexit->key = fread_number(fp);
                pexit->u1.vnum = fread_number(fp);

                switch (locks) {
                case 1:
                    pexit->exit_info = EX_ISDOOR;
                    break;
                case 2:
                    pexit->exit_info = EX_ISDOOR | EX_PICKPROOF;
                    break;
                case 3:
                    pexit->exit_info = EX_ISDOOR | EX_NOPASS;
                    break;
                case 4:
                    pexit->exit_info = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
                    break;
                }

                pRoomIndex->exit[door] = pexit;
                pRoomIndex->old_exit[door] = pexit;
                top_exit++;
            }
            else if (letter == 'E') {
                EXTRA_DESCR_DATA* ed;

                ed = alloc_perm(sizeof(*ed));
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ed->next = pRoomIndex->extra_descr;
                pRoomIndex->extra_descr = ed;
                top_ed++;
            }

            else if (letter == 'O') {
                if (pRoomIndex->owner[0] != '\0') {
                    bug("Load_rooms: duplicate owner.", 0);
                    exit(1);
                }

                pRoomIndex->owner = fread_string(fp);
            }

            else {
                bug("Load_rooms: vnum %d has flag not 'DES'.", vnum);
                exit(1);
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        pRoomIndex->next = room_index_hash[iHash];
        room_index_hash[iHash] = pRoomIndex;
        top_room++;
    }

    return;
}

/*
 * Snarf a shop section.
 */
void load_shops(FILE* fp)
{
    SHOP_DATA* pShop;

    for (;;) {
        MOB_INDEX_DATA* pMobIndex;
        int iTrade;

        if ((pShop = (SHOP_DATA*)alloc_perm(sizeof(*pShop))) == NULL) {
            bug("load_shops: Failed to create shops.");
            exit(1);
        }

        pShop->keeper = fread_number(fp);
        if (pShop->keeper == 0) break;
        for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
            pShop->buy_type[iTrade] = fread_number(fp);
        pShop->profit_buy = fread_number(fp);
        pShop->profit_sell = fread_number(fp);
        pShop->open_hour = fread_number(fp);
        pShop->close_hour = fread_number(fp);
        fread_to_eol(fp);
        pMobIndex = get_mob_index(pShop->keeper);
        pMobIndex->pShop = pShop;

        if (shop_first == NULL)
            shop_first = pShop;
        if (shop_last != NULL) 
            shop_last->next = pShop;
        shop_last = pShop;
        if (pShop)
            pShop->next = NULL;
        top_shop++;
    }

    return;
}

/*
 * Snarf spec proc declarations.
 */
void load_specials(FILE* fp)
{
    for (;;) {
        MOB_INDEX_DATA* pMobIndex;
        char letter = 0;

        switch (letter = fread_letter(fp)) {
        default:
            bug("Load_specials: letter '%c' not *MS.", letter);
            exit(1);

        case 'S':
            return;

        case '*':
            break;

        case 'M':
            pMobIndex = get_mob_index(fread_number(fp));
            pMobIndex->spec_fun = spec_lookup(fread_word(fp));
            if (pMobIndex->spec_fun == 0) {
                bug("Load_specials: 'M': vnum %d.", pMobIndex->vnum);
                exit(1);
            }
            break;
        }

        fread_to_eol(fp);
    }
}

/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits(void)
{
    extern const int16_t rev_dir[];
    char buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA* pRoomIndex;
    ROOM_INDEX_DATA* to_room;
    EXIT_DATA* pexit;
    EXIT_DATA* pexit_rev;
    int iHash;
    int door;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++) {
        for (pRoomIndex = room_index_hash[iHash]; pRoomIndex != NULL;
             pRoomIndex = pRoomIndex->next) {
            bool fexit;

            fexit = false;
            for (door = 0; door <= 5; door++) {
                if ((pexit = pRoomIndex->exit[door]) != NULL) {
                    if (pexit->u1.vnum <= 0
                        || get_room_index(pexit->u1.vnum) == NULL)
                        pexit->u1.to_room = NULL;
                    else {
                        fexit = true;
                        pexit->u1.to_room = get_room_index(pexit->u1.vnum);
                    }
                }
            }
            if (!fexit) SET_BIT(pRoomIndex->room_flags, ROOM_NO_MOB);
        }
    }

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++) {
        for (pRoomIndex = room_index_hash[iHash]; pRoomIndex != NULL;
             pRoomIndex = pRoomIndex->next) {
            for (door = 0; door <= 5; door++) {
                if ((pexit = pRoomIndex->exit[door]) != NULL
                    && (to_room = pexit->u1.to_room) != NULL
                    && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
                    && pexit_rev->u1.to_room != pRoomIndex
                    && (pRoomIndex->vnum < 1200 || pRoomIndex->vnum > 1299)) {
                    sprintf(buf, "Fix_exits: %d:%d -> %d:%d -> %d.",
                            pRoomIndex->vnum, door, to_room->vnum,
                            rev_dir[door],
                            (pexit_rev->u1.to_room == NULL)
                                ? 0
                                : pexit_rev->u1.to_room->vnum);
                    bug(buf, 0);
                }
            }
        }
    }

    return;
}

/*
 * Repopulate areas periodically.
 */
void area_update(void)
{
    AREA_DATA* pArea;
    char buf[MAX_STRING_LENGTH];

    for (pArea = area_first; pArea != NULL; pArea = pArea->next) {
        if (++pArea->age < 3) continue;

        /*
         * Check age and reset.
         * Note: Mud School resets every 3 minutes (not 15).
         */
        if ((!pArea->empty && (pArea->nplayer == 0 || pArea->age >= 15))
            || pArea->age >= 31) {
            ROOM_INDEX_DATA* pRoomIndex;

            reset_area(pArea);
            sprintf(buf, "%s has just been reset.", pArea->name);
            wiznet(buf, NULL, NULL, WIZ_RESETS, 0, 0);

            pArea->age = number_range(0, 3);
            pRoomIndex = get_room_index(ROOM_VNUM_SCHOOL);
            if (pRoomIndex != NULL && pArea == pRoomIndex->area)
                pArea->age = 15 - 2;
            else if (pArea->nplayer == 0)
                pArea->empty = true;
        }
    }

    return;
}

/*
 * Reset one area.
 */
void reset_area(AREA_DATA* pArea)
{
    RESET_DATA* pReset;
    CHAR_DATA* mob;
    bool last;
    int level;

    mob = NULL;
    last = true;
    level = 0;
    for (pReset = pArea->reset_first; pReset != NULL; pReset = pReset->next) {
        ROOM_INDEX_DATA* pRoomIndex;
        MOB_INDEX_DATA* pMobIndex;
        OBJ_INDEX_DATA* pObjIndex;
        OBJ_INDEX_DATA* pObjToIndex;
        EXIT_DATA* pexit;
        OBJ_DATA* obj;
        OBJ_DATA* obj_to;
        int count, limit;

        switch (pReset->command) {
        default:
            bug("Reset_area: bad command %c.", pReset->command);
            break;

        case 'M':
            if ((pMobIndex = get_mob_index(pReset->arg1)) == NULL) {
                bug("Reset_area: 'M': bad vnum %d.", pReset->arg1);
                continue;
            }

            if ((pRoomIndex = get_room_index(pReset->arg3)) == NULL) {
                bug("Reset_area: 'R': bad vnum %d.", pReset->arg3);
                continue;
            }

            if (pMobIndex->count >= pReset->arg2) {
                last = false;
                break;
            }

            count = 0;
            for (mob = pRoomIndex->people; mob != NULL; mob = mob->next_in_room)
                if (mob->pIndexData == pMobIndex) {
                    count++;
                    if (count >= pReset->arg4) {
                        last = false;
                        break;
                    }
                }

            if (count >= pReset->arg4) break;

            mob = create_mobile(pMobIndex);

            /*
             * Check for pet shop.
             */
            {
                ROOM_INDEX_DATA* pRoomIndexPrev;
                pRoomIndexPrev = get_room_index(pRoomIndex->vnum - 1);
                if (pRoomIndexPrev != NULL
                    && IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
                    SET_BIT(mob->act, ACT_PET);
            }

            /* set area */
            mob->zone = pRoomIndex->area;

            char_to_room(mob, pRoomIndex);
            level = URANGE(0, mob->level - 2, LEVEL_HERO - 1);
            last = true;
            break;

        case 'O':
            if ((pObjIndex = get_obj_index(pReset->arg1)) == NULL) {
                bug("Reset_area: 'O': bad vnum %d.", pReset->arg1);
                continue;
            }

            if ((pRoomIndex = get_room_index(pReset->arg3)) == NULL) {
                bug("Reset_area: 'R': bad vnum %d.", pReset->arg3);
                continue;
            }

            if (pArea->nplayer > 0
                || count_obj_list(pObjIndex, pRoomIndex->contents) > 0) {
                last = false;
                break;
            }

            obj = create_object(pObjIndex,
                                UMIN(number_fuzzy(level), LEVEL_HERO - 1));
            obj->cost = 0;
            obj_to_room(obj, pRoomIndex);
            last = true;
            break;

        case 'P':
            if ((pObjIndex = get_obj_index(pReset->arg1)) == NULL) {
                bug("Reset_area: 'P': bad vnum %d.", pReset->arg1);
                continue;
            }

            if ((pObjToIndex = get_obj_index(pReset->arg3)) == NULL) {
                bug("Reset_area: 'P': bad vnum %d.", pReset->arg3);
                continue;
            }

            if (pReset->arg2 > 50) /* old format */
                limit = 6;
            else if (pReset->arg2 == -1) /* no limit */
                limit = 999;
            else
                limit = pReset->arg2;

            if (pArea->nplayer > 0
                || (obj_to = get_obj_type(pObjToIndex)) == NULL
                || (obj_to->in_room == NULL && !last)
                || (pObjIndex->count >= limit && number_range(0, 4) != 0)
                || (count = count_obj_list(pObjIndex, obj_to->contains))
                       > pReset->arg4) {
                last = false;
                break;
            }

            while (count < pReset->arg4) {
                obj = create_object(pObjIndex, number_fuzzy(obj_to->level));
                obj_to_obj(obj, obj_to);
                count++;
                if (pObjIndex->count >= limit) break;
            }
            /* fix object lock state! */
            obj_to->value[1] = obj_to->pIndexData->value[1];
            last = true;
            break;

        case 'G':
        case 'E':
            if ((pObjIndex = get_obj_index(pReset->arg1)) == NULL) {
                bug("Reset_area: 'E' or 'G': bad vnum %d.", pReset->arg1);
                continue;
            }

            if (!last) break;

            if (mob == NULL) {
                bug("Reset_area: 'E' or 'G': null mob for vnum %d.",
                    pReset->arg1);
                last = false;
                break;
            }

            if (mob->pIndexData->pShop != NULL) {
                int olevel = 0, i, j;

                if (!pObjIndex->new_format) switch (pObjIndex->item_type) {
                    case ITEM_PILL:
                    case ITEM_POTION:
                    case ITEM_SCROLL:
                        olevel = 53;
                        for (i = 1; i < 5; i++) {
                            if (pObjIndex->value[i] > 0) {
                                for (j = 0; j < MAX_CLASS; j++) {
                                    olevel = UMIN(
                                        olevel, skill_table[pObjIndex->value[i]]
                                                    .skill_level[j]);
                                }
                            }
                        }

                        olevel = UMAX(0, (olevel * 3 / 4) - 2);
                        break;
                    case ITEM_WAND:
                        olevel = number_range(10, 20);
                        break;
                    case ITEM_STAFF:
                        olevel = number_range(15, 25);
                        break;
                    case ITEM_ARMOR:
                        olevel = number_range(5, 15);
                        break;
                    case ITEM_WEAPON:
                        olevel = number_range(5, 15);
                        break;
                    case ITEM_TREASURE:
                        olevel = number_range(10, 20);
                        break;
                    }

                obj = create_object(pObjIndex, olevel);
                SET_BIT(obj->extra_flags, ITEM_INVENTORY);
            }

            else {
                if (pReset->arg2 > 50) /* old format */
                    limit = 6;
                else if (pReset->arg2 == -1) /* no limit */
                    limit = 999;
                else
                    limit = pReset->arg2;

                if (pObjIndex->count < limit || number_range(0, 4) == 0) {
                    obj = create_object(
                        pObjIndex, UMIN(number_fuzzy(level), LEVEL_HERO - 1));
                    /* error message if it is too high */
                    if (obj->level > mob->level + 3
                        || (obj->item_type == ITEM_WEAPON
                            && pReset->command == 'E'
                            && obj->level < mob->level - 5 && obj->level < 45))
                        fprintf(stderr,
                                "Err: obj %s (%d) -- %d, mob %s (%d) -- %d\n",
                                obj->short_descr, obj->pIndexData->vnum,
                                obj->level, mob->short_descr,
                                mob->pIndexData->vnum, mob->level);
                }
                else
                    break;
            }
            obj_to_char(obj, mob);
            if (pReset->command == 'E') equip_char(mob, obj, pReset->arg3);
            last = true;
            break;

        case 'D':
            if ((pRoomIndex = get_room_index(pReset->arg1)) == NULL) {
                bug("Reset_area: 'D': bad vnum %d.", pReset->arg1);
                continue;
            }

            if ((pexit = pRoomIndex->exit[pReset->arg2]) == NULL) break;

            switch (pReset->arg3) {
            case 0:
                REMOVE_BIT(pexit->exit_info, EX_CLOSED);
                REMOVE_BIT(pexit->exit_info, EX_LOCKED);
                break;

            case 1:
                SET_BIT(pexit->exit_info, EX_CLOSED);
                REMOVE_BIT(pexit->exit_info, EX_LOCKED);
                break;

            case 2:
                SET_BIT(pexit->exit_info, EX_CLOSED);
                SET_BIT(pexit->exit_info, EX_LOCKED);
                break;
            }

            last = true;
            break;

        case 'R':
            if ((pRoomIndex = get_room_index(pReset->arg1)) == NULL) {
                bug("Reset_area: 'R': bad vnum %d.", pReset->arg1);
                continue;
            }

            {
                int d0;
                int d1;

                for (d0 = 0; d0 < pReset->arg2 - 1; d0++) {
                    d1 = number_range(d0, pReset->arg2 - 1);
                    pexit = pRoomIndex->exit[d0];
                    pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
                    pRoomIndex->exit[d1] = pexit;
                }
            }
            break;
        }
    }

    return;
}

/*
 * Create an instance of a mobile.
 */
CHAR_DATA* create_mobile(MOB_INDEX_DATA* pMobIndex)
{
    CHAR_DATA* mob;
    int i;
    AFFECT_DATA af = { 0 };

    mobile_count++;

    if (pMobIndex == NULL) {
        bug("Create_mobile: NULL pMobIndex.", 0);
        exit(1);
    }

    mob = new_char();

    mob->pIndexData = pMobIndex;

    mob->name = pMobIndex->player_name;
    mob->id = get_mob_id();
    mob->short_descr = pMobIndex->short_descr;
    mob->long_descr = pMobIndex->long_descr;
    mob->description = pMobIndex->description;
    mob->spec_fun = pMobIndex->spec_fun;
    mob->prompt = NULL;

    if (pMobIndex->wealth == 0) {
        mob->silver = 0;
        mob->gold = 0;
    }
    else {
        long wealth;

        wealth = number_range(pMobIndex->wealth / 2, 3 * pMobIndex->wealth / 2);
        mob->gold = number_range(wealth / 200, wealth / 100);
        mob->silver = wealth - (mob->gold * 100);
    }

    if (pMobIndex->new_format)
    /* load in new style */
    {
        /* read from prototype */
        mob->group = pMobIndex->group;
        mob->act = pMobIndex->act;
        mob->comm = COMM_NOCHANNELS | COMM_NOSHOUT | COMM_NOTELL;
        mob->affected_by = pMobIndex->affected_by;
        mob->alignment = pMobIndex->alignment;
        mob->level = pMobIndex->level;
        mob->hitroll = pMobIndex->hitroll;
        mob->damroll = pMobIndex->damage[DICE_BONUS];
        mob->max_hit
            = dice(pMobIndex->hit[DICE_NUMBER], pMobIndex->hit[DICE_TYPE])
              + pMobIndex->hit[DICE_BONUS];
        mob->hit = mob->max_hit;
        mob->max_mana
            = dice(pMobIndex->mana[DICE_NUMBER], pMobIndex->mana[DICE_TYPE])
              + pMobIndex->mana[DICE_BONUS];
        mob->mana = mob->max_mana;
        mob->damage[DICE_NUMBER] = pMobIndex->damage[DICE_NUMBER];
        mob->damage[DICE_TYPE] = pMobIndex->damage[DICE_TYPE];
        mob->dam_type = pMobIndex->dam_type;
        if (mob->dam_type == 0) switch (number_range(1, 3)) {
            case (1):
                mob->dam_type = 3;
                break; /* slash */
            case (2):
                mob->dam_type = 7;
                break; /* pound */
            case (3):
                mob->dam_type = 11;
                break; /* pierce */
            }
        for (i = 0; i < 4; i++) mob->armor[i] = pMobIndex->ac[i];
        mob->off_flags = pMobIndex->off_flags;
        mob->imm_flags = pMobIndex->imm_flags;
        mob->res_flags = pMobIndex->res_flags;
        mob->vuln_flags = pMobIndex->vuln_flags;
        mob->start_pos = pMobIndex->start_pos;
        mob->default_pos = pMobIndex->default_pos;
        mob->sex = pMobIndex->sex;
        if (mob->sex == 3) /* random sex */
            mob->sex = number_range(1, 2);
        mob->race = pMobIndex->race;
        mob->form = pMobIndex->form;
        mob->parts = pMobIndex->parts;
        mob->size = pMobIndex->size;
        mob->material = str_dup(pMobIndex->material);

        /* computed on the spot */

        for (i = 0; i < MAX_STATS; i++)
            mob->perm_stat[i] = UMIN(25, 11 + mob->level / 4);

        if (IS_SET(mob->act, ACT_WARRIOR)) {
            mob->perm_stat[STAT_STR] += 3;
            mob->perm_stat[STAT_INT] -= 1;
            mob->perm_stat[STAT_CON] += 2;
        }

        if (IS_SET(mob->act, ACT_THIEF)) {
            mob->perm_stat[STAT_DEX] += 3;
            mob->perm_stat[STAT_INT] += 1;
            mob->perm_stat[STAT_WIS] -= 1;
        }

        if (IS_SET(mob->act, ACT_CLERIC)) {
            mob->perm_stat[STAT_WIS] += 3;
            mob->perm_stat[STAT_DEX] -= 1;
            mob->perm_stat[STAT_STR] += 1;
        }

        if (IS_SET(mob->act, ACT_MAGE)) {
            mob->perm_stat[STAT_INT] += 3;
            mob->perm_stat[STAT_STR] -= 1;
            mob->perm_stat[STAT_DEX] += 1;
        }

        if (IS_SET(mob->off_flags, OFF_FAST)) mob->perm_stat[STAT_DEX] += 2;

        mob->perm_stat[STAT_STR] += mob->size - SIZE_MEDIUM;
        mob->perm_stat[STAT_CON] += (mob->size - SIZE_MEDIUM) / 2;

        /* let's get some spell action */
        if (IS_AFFECTED(mob, AFF_SANCTUARY)) {
            af.where = TO_AFFECTS;
            af.type = skill_lookup("sanctuary");
            af.level = mob->level;
            af.duration = -1;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = AFF_SANCTUARY;
            affect_to_char(mob, &af);
        }

        if (IS_AFFECTED(mob, AFF_HASTE)) {
            af.where = TO_AFFECTS;
            af.type = skill_lookup("haste");
            af.level = mob->level;
            af.duration = -1;
            af.location = APPLY_DEX;
            af.modifier = 1 + (mob->level >= 18) + (mob->level >= 25)
                          + (mob->level >= 32);
            af.bitvector = AFF_HASTE;
            affect_to_char(mob, &af);
        }

        if (IS_AFFECTED(mob, AFF_PROTECT_EVIL)) {
            af.where = TO_AFFECTS;
            af.type = skill_lookup("protection evil");
            af.level = mob->level;
            af.duration = -1;
            af.location = APPLY_SAVES;
            af.modifier = -1;
            af.bitvector = AFF_PROTECT_EVIL;
            affect_to_char(mob, &af);
        }

        if (IS_AFFECTED(mob, AFF_PROTECT_GOOD)) {
            af.where = TO_AFFECTS;
            af.type = skill_lookup("protection good");
            af.level = mob->level;
            af.duration = -1;
            af.location = APPLY_SAVES;
            af.modifier = -1;
            af.bitvector = AFF_PROTECT_GOOD;
            affect_to_char(mob, &af);
        }
    }
    else /* read in old format and convert */
    {
        mob->act = pMobIndex->act;
        mob->affected_by = pMobIndex->affected_by;
        mob->alignment = pMobIndex->alignment;
        mob->level = pMobIndex->level;
        mob->hitroll = pMobIndex->hitroll;
        mob->damroll = 0;
        mob->max_hit = mob->level * 8
                       + number_range(mob->level * mob->level / 4,
                                      mob->level * mob->level);
        mob->max_hit = (int16_t)((double)mob->max_hit * 0.9);
        mob->hit = mob->max_hit;
        mob->max_mana = 100 + dice(mob->level, 10);
        mob->mana = mob->max_mana;
        switch (number_range(1, 3)) {
        case (1):
            mob->dam_type = 3;
            break; /* slash */
        case (2):
            mob->dam_type = 7;
            break; /* pound */
        case (3):
            mob->dam_type = 11;
            break; /* pierce */
        }
        for (i = 0; i < 3; i++)
            mob->armor[i] = interpolate(mob->level, 100, -100);
        mob->armor[3] = interpolate(mob->level, 100, 0);
        mob->race = pMobIndex->race;
        mob->off_flags = pMobIndex->off_flags;
        mob->imm_flags = pMobIndex->imm_flags;
        mob->res_flags = pMobIndex->res_flags;
        mob->vuln_flags = pMobIndex->vuln_flags;
        mob->start_pos = pMobIndex->start_pos;
        mob->default_pos = pMobIndex->default_pos;
        mob->sex = pMobIndex->sex;
        mob->form = pMobIndex->form;
        mob->parts = pMobIndex->parts;
        mob->size = SIZE_MEDIUM;
        mob->material = "";

        for (i = 0; i < MAX_STATS; i++) mob->perm_stat[i] = 11 + mob->level / 4;
    }

    mob->position = mob->start_pos;

    /* link the mob to the world list */
    mob->next = char_list;
    char_list = mob;
    pMobIndex->count++;
    return mob;
}

/* duplicate a mobile exactly -- except inventory */
void clone_mobile(CHAR_DATA* parent, CHAR_DATA* clone)
{
    int i;
    AFFECT_DATA* paf;

    if (parent == NULL || clone == NULL || !IS_NPC(parent)) return;

    /* start fixing values */
    clone->name = str_dup(parent->name);
    clone->version = parent->version;
    clone->short_descr = str_dup(parent->short_descr);
    clone->long_descr = str_dup(parent->long_descr);
    clone->description = str_dup(parent->description);
    clone->group = parent->group;
    clone->sex = parent->sex;
    clone->ch_class = parent->ch_class;
    clone->race = parent->race;
    clone->level = parent->level;
    clone->trust = 0;
    clone->timer = parent->timer;
    clone->wait = parent->wait;
    clone->hit = parent->hit;
    clone->max_hit = parent->max_hit;
    clone->mana = parent->mana;
    clone->max_mana = parent->max_mana;
    clone->move = parent->move;
    clone->max_move = parent->max_move;
    clone->gold = parent->gold;
    clone->silver = parent->silver;
    clone->exp = parent->exp;
    clone->act = parent->act;
    clone->comm = parent->comm;
    clone->imm_flags = parent->imm_flags;
    clone->res_flags = parent->res_flags;
    clone->vuln_flags = parent->vuln_flags;
    clone->invis_level = parent->invis_level;
    clone->affected_by = parent->affected_by;
    clone->position = parent->position;
    clone->practice = parent->practice;
    clone->train = parent->train;
    clone->saving_throw = parent->saving_throw;
    clone->alignment = parent->alignment;
    clone->hitroll = parent->hitroll;
    clone->damroll = parent->damroll;
    clone->wimpy = parent->wimpy;
    clone->form = parent->form;
    clone->parts = parent->parts;
    clone->size = parent->size;
    clone->material = str_dup(parent->material);
    clone->off_flags = parent->off_flags;
    clone->dam_type = parent->dam_type;
    clone->start_pos = parent->start_pos;
    clone->default_pos = parent->default_pos;
    clone->spec_fun = parent->spec_fun;

    for (i = 0; i < 4; i++) clone->armor[i] = parent->armor[i];

    for (i = 0; i < MAX_STATS; i++) {
        clone->perm_stat[i] = parent->perm_stat[i];
        clone->mod_stat[i] = parent->mod_stat[i];
    }

    for (i = 0; i < 3; i++) clone->damage[i] = parent->damage[i];

    /* now add the affects */
    for (paf = parent->affected; paf != NULL; paf = paf->next)
        affect_to_char(clone, paf);
}

/*
 * Create an instance of an object.
 */
OBJ_DATA* create_object(OBJ_INDEX_DATA* pObjIndex, int level)
{
    AFFECT_DATA* paf;
    OBJ_DATA* obj;
    int i;

    if (pObjIndex == NULL) {
        bug("Create_object: NULL pObjIndex.", 0);
        exit(1);
    }

    obj = new_obj();

    obj->pIndexData = pObjIndex;
    obj->in_room = NULL;
    obj->enchanted = false;

    if (pObjIndex->new_format)
        obj->level = pObjIndex->level;
    else
        obj->level = UMAX(0, level);
    obj->wear_loc = -1;

    obj->name = pObjIndex->name;
    obj->short_descr = pObjIndex->short_descr;
    obj->description = pObjIndex->description;
    obj->material = str_dup(pObjIndex->material);
    obj->item_type = pObjIndex->item_type;
    obj->extra_flags = pObjIndex->extra_flags;
    obj->wear_flags = pObjIndex->wear_flags;
    obj->value[0] = pObjIndex->value[0];
    obj->value[1] = pObjIndex->value[1];
    obj->value[2] = pObjIndex->value[2];
    obj->value[3] = pObjIndex->value[3];
    obj->value[4] = pObjIndex->value[4];
    obj->weight = pObjIndex->weight;

    if (level == -1 || pObjIndex->new_format)
        obj->cost = pObjIndex->cost;
    else
        obj->cost
            = number_fuzzy(10) * number_fuzzy(level) * number_fuzzy(level);

    /*
     * Mess with object properties.
     */
    switch (obj->item_type) {
    default:
        bug("Read_object: vnum %d bad type.", pObjIndex->vnum);
        break;

    case ITEM_LIGHT:
        if (obj->value[2] == 999) obj->value[2] = -1;
        break;

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
    case ITEM_PORTAL:
        if (!pObjIndex->new_format) obj->cost /= 5;
        break;

    case ITEM_TREASURE:
    case ITEM_WARP_STONE:
    case ITEM_ROOM_KEY:
    case ITEM_GEM:
    case ITEM_JEWELRY:
        break;

    case ITEM_JUKEBOX:
        for (i = 0; i < 5; i++) obj->value[i] = -1;
        break;

    case ITEM_SCROLL:
        if (level != -1 && !pObjIndex->new_format)
            obj->value[0] = number_fuzzy(obj->value[0]);
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        if (level != -1 && !pObjIndex->new_format) {
            obj->value[0] = number_fuzzy(obj->value[0]);
            obj->value[1] = number_fuzzy(obj->value[1]);
            obj->value[2] = obj->value[1];
        }
        if (!pObjIndex->new_format) obj->cost *= 2;
        break;

    case ITEM_WEAPON:
        if (level != -1 && !pObjIndex->new_format) {
            obj->value[1] = number_fuzzy(number_fuzzy(1 * level / 4 + 2));
            obj->value[2] = number_fuzzy(number_fuzzy(3 * level / 4 + 6));
        }
        break;

    case ITEM_ARMOR:
        if (level != -1 && !pObjIndex->new_format) {
            obj->value[0] = number_fuzzy(level / 5 + 3);
            obj->value[1] = number_fuzzy(level / 5 + 3);
            obj->value[2] = number_fuzzy(level / 5 + 3);
        }
        break;

    case ITEM_POTION:
    case ITEM_PILL:
        if (level != -1 && !pObjIndex->new_format)
            obj->value[0] = number_fuzzy(number_fuzzy(obj->value[0]));
        break;

    case ITEM_MONEY:
        if (!pObjIndex->new_format) obj->value[0] = obj->cost;
        break;
    }

    for (paf = pObjIndex->affected; paf != NULL; paf = paf->next)
        if (paf->location == APPLY_SPELL_AFFECT) affect_to_obj(obj, paf);

    obj->next = object_list;
    object_list = obj;
    pObjIndex->count++;

    return obj;
}

/* duplicate an object exactly -- except contents */
void clone_object(OBJ_DATA* parent, OBJ_DATA* clone)
{
    int i;
    AFFECT_DATA* paf;
    EXTRA_DESCR_DATA *ed, *ed_new;

    if (parent == NULL || clone == NULL) return;

    /* start fixing the object */
    clone->name = str_dup(parent->name);
    clone->short_descr = str_dup(parent->short_descr);
    clone->description = str_dup(parent->description);
    clone->item_type = parent->item_type;
    clone->extra_flags = parent->extra_flags;
    clone->wear_flags = parent->wear_flags;
    clone->weight = parent->weight;
    clone->cost = parent->cost;
    clone->level = parent->level;
    clone->condition = parent->condition;
    clone->material = str_dup(parent->material);
    clone->timer = parent->timer;

    for (i = 0; i < 5; i++) clone->value[i] = parent->value[i];

    /* affects */
    clone->enchanted = parent->enchanted;

    for (paf = parent->affected; paf != NULL; paf = paf->next)
        affect_to_obj(clone, paf);

    /* extended desc */
    for (ed = parent->extra_descr; ed != NULL; ed = ed->next) {
        ed_new = new_extra_descr();
        ed_new->keyword = str_dup(ed->keyword);
        ed_new->description = str_dup(ed->description);
        ed_new->next = clone->extra_descr;
        clone->extra_descr = ed_new;
    }
}

/*
 * Clear a new character.
 */
void clear_char(CHAR_DATA* ch)
{
    static CHAR_DATA ch_zero;
    int i;

    *ch = ch_zero;
    ch->name = &str_empty[0];
    ch->short_descr = &str_empty[0];
    ch->long_descr = &str_empty[0];
    ch->description = &str_empty[0];
    ch->prompt = &str_empty[0];
    ch->logon = current_time;
    ch->lines = PAGELEN;
    for (i = 0; i < 4; i++) ch->armor[i] = 100;
    ch->position = POS_STANDING;
    ch->hit = 20;
    ch->max_hit = 20;
    ch->mana = 100;
    ch->max_mana = 100;
    ch->move = 100;
    ch->max_move = 100;
    ch->on = NULL;
    for (i = 0; i < MAX_STATS; i++) {
        ch->perm_stat[i] = 13;
        ch->mod_stat[i] = 0;
    }
    return;
}

/*
 * Get an extra description from a list.
 */
char* get_extra_descr(const char* name, EXTRA_DESCR_DATA* ed)
{
    for (; ed != NULL; ed = ed->next) {
        if (is_name((char*)name, ed->keyword)) return ed->description;
    }
    return NULL;
}

/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA* get_mob_index(int vnum)
{
    MOB_INDEX_DATA* pMobIndex;

    for (pMobIndex = mob_index_hash[vnum % MAX_KEY_HASH]; pMobIndex != NULL;
         pMobIndex = pMobIndex->next) {
        if (pMobIndex->vnum == vnum) return pMobIndex;
    }

    if (fBootDb) {
        bug("Get_mob_index: bad vnum %d.", vnum);
        exit(1);
    }

    return NULL;
}

/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA* get_obj_index(int vnum)
{
    OBJ_INDEX_DATA* pObjIndex;

    for (pObjIndex = obj_index_hash[vnum % MAX_KEY_HASH]; pObjIndex != NULL;
         pObjIndex = pObjIndex->next) {
        if (pObjIndex->vnum == vnum) return pObjIndex;
    }

    if (fBootDb) {
        bug("Get_obj_index: bad vnum %d.", vnum);
        exit(1);
    }

    return NULL;
}

/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA* get_room_index(int vnum)
{
    ROOM_INDEX_DATA* pRoomIndex;

    for (pRoomIndex = room_index_hash[vnum % MAX_KEY_HASH]; pRoomIndex != NULL;
         pRoomIndex = pRoomIndex->next) {
        if (pRoomIndex->vnum == vnum) return pRoomIndex;
    }

    if (fBootDb) {
        bug("Get_room_index: bad vnum %d.", vnum);
        exit(1);
    }

    return NULL;
}

/*
 * Read a letter from a file.
 */
char fread_letter(FILE* fp)
{
    char c;

    do {
        c = getc(fp);
    }
    while (ISSPACE(c));

    return c;
}

/*
 * Read a number from a file.
 */
int fread_number(FILE* fp)
{
    int number;
    bool sign;
    char c;

    do {
        c = getc(fp);
    }
    while (ISSPACE(c));

    number = 0;

    sign = false;
    if (c == '+') { c = getc(fp); }
    else if (c == '-') {
        sign = true;
        c = getc(fp);
    }

    if (!ISDIGIT(c)) {
        bug("Fread_number: bad format.", 0);
        exit(1);
    }

    while (ISDIGIT(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (sign) number = 0 - number;

    if (c == '|')
        number += fread_number(fp);
    else if (c != ' ')
        ungetc(c, fp);

    return number;
}

long fread_flag(FILE* fp)
{
    int number;
    char c;
    bool negative = false;

    do {
        c = getc(fp);
    }
    while (ISSPACE(c));

    if (c == '-') {
        negative = true;
        c = getc(fp);
    }

    number = 0;

    if (!ISDIGIT(c)) {
        while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
            number += flag_convert(c);
            c = getc(fp);
        }
    }

    while (ISDIGIT(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (c == '|')
        number += fread_flag(fp);

    else if (c != ' ')
        ungetc(c, fp);

    if (negative) return -1 * number;

    return number;
}

long flag_convert(char letter)
{
    long bitsum = 0;
    char i;

    if ('A' <= letter && letter <= 'Z') {
        bitsum = 1;
        for (i = letter; i > 'A'; i--) bitsum *= 2;
    }
    else if ('a' <= letter && letter <= 'z') {
        bitsum = 67108864; /* 2^26 */
        for (i = letter; i > 'a'; i--) bitsum *= 2;
    }

    return bitsum;
}

/*
 * Read and allocate space for a string from a file.
 * These strings are read-only and shared.
 * Strings are hashed:
 *   each string prepended with hash pointer to prev string,
 *   hash code is simply the string length.
 *   this function takes 40% to 50% of boot-up time.
 */
char* fread_string(FILE* fp)
{
    char* plast;
    char c;

    plast = top_string + sizeof(char*);
    if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH]) {
        bug("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
        exit(1);
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do {
        c = getc(fp);
    }
    while (ISSPACE(c));

    if ((*plast++ = c) == '~') return &str_empty[0];

    for (;;) {
        /*
         * Back off the char type lookup,
         *   it was too dirty for portability.
         *   -- Furey
         */

        switch (*plast = getc(fp)) {
        default:
            plast++;
            break;

        case EOF:
            /* temp fix */
            bug("Fread_string: EOF", 0);
            return NULL;
            /* exit( 1 ); */
            break;

        case '\n':
            plast++;
            *plast++ = '\r';
            break;

        case '\r':
            break;

        case '~':
            plast++;
            {
                union {
                    char* pc;
                    char rgc[sizeof(char*)];
                } u1 = { 0 };
                int ic;
                int iHash;
                char* pHash;
                char* pHashPrev = NULL;
                char* pString;

                plast[-1] = '\0';
                iHash = (int)UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
                for (pHash = string_hash[iHash]; pHash; pHash = pHashPrev) {
                    for (ic = 0; ic < sizeof(char*); ic++)
                        u1.rgc[ic] = pHash[ic];
                    pHashPrev = u1.pc;
                    pHash += sizeof(char*);

                    if (top_string[sizeof(char*)] == pHash[0]
                        && !strcmp(top_string + sizeof(char*) + 1, pHash + 1))
                        return pHash;
                }

                if (fBootDb) {
                    pString = top_string;
                    top_string = plast;
                    u1.pc = string_hash[iHash];
                    for (ic = 0; ic < sizeof(char*); ic++)
                        pString[ic] = u1.rgc[ic];
                    string_hash[iHash] = pString;

                    nAllocString += 1;
                    sAllocString += top_string - pString;
                    return pString + sizeof(char*);
                }
                else {
                    return str_dup(top_string + sizeof(char*));
                }
            }
        }
    }
}

char* fread_string_eol(FILE* fp)
{
    static bool char_special[256 - EOF];
    char* plast;
    char c;

    if (char_special[EOF - EOF] != true) {
        char_special[EOF - EOF] = true;
        char_special['\n' - EOF] = true;
        char_special['\r' - EOF] = true;
    }

    plast = top_string + sizeof(char*);
    if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH]) {
        bug("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
        exit(1);
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do {
        c = getc(fp);
    }
    while (ISSPACE(c));

    if ((*plast++ = c) == '\n') return &str_empty[0];

    for (;;) {
        if (!char_special[(*plast++ = getc(fp)) - EOF]) continue;

        switch (plast[-1]) {
        default:
            break;

        case EOF:
            bug("Fread_string_eol  EOF", 0);
            exit(1);
            break;

        case '\n':
        case '\r': {
            union {
                char* pc;
                char rgc[sizeof(char*)];
            } u1 = { 0 };
            int ic;
            int iHash;
            char* pHash;
            char* pHashPrev = NULL;
            char* pString;

            plast[-1] = '\0';
            iHash = (int)UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
            for (pHash = string_hash[iHash]; pHash; pHash = pHashPrev) {
                for (ic = 0; ic < sizeof(char*); ic++) u1.rgc[ic] = pHash[ic];
                pHashPrev = u1.pc;
                pHash += sizeof(char*);

                if (top_string[sizeof(char*)] == pHash[0]
                    && !strcmp(top_string + sizeof(char*) + 1, pHash + 1))
                    return pHash;
            }

            if (fBootDb) {
                pString = top_string;
                top_string = plast;
                u1.pc = string_hash[iHash];
                for (ic = 0; ic < sizeof(char*); ic++) pString[ic] = u1.rgc[ic];
                string_hash[iHash] = pString;

                nAllocString += 1;
                sAllocString += top_string - pString;
                return pString + sizeof(char*);
            }
            else {
                return str_dup(top_string + sizeof(char*));
            }
        }
        }
    }
}

/*
 * Read to end of line (for comments).
 */
void fread_to_eol(FILE* fp)
{
    char c;

    do {
        c = getc(fp);
    }
    while (c != '\n' && c != '\r');

    do {
        c = getc(fp);
    }
    while (c == '\n' || c == '\r');

    ungetc(c, fp);
    return;
}

/*
 * Read one word (into static buffer).
 */
char* fread_word(FILE* fp)
{
    static char word[MAX_INPUT_LENGTH];
    char* pword;
    char cEnd;

    do {
        cEnd = getc(fp);
    }
    while (ISSPACE(cEnd));

    if (cEnd == '\'' || cEnd == '"') { pword = word; }
    else {
        word[0] = cEnd;
        pword = word + 1;
        cEnd = ' ';
    }

    for (; pword < word + MAX_INPUT_LENGTH; pword++) {
        *pword = getc(fp);
        if (cEnd == ' ' ? ISSPACE(*pword) : *pword == cEnd) {
            if (cEnd == ' ') ungetc(*pword, fp);
            *pword = '\0';
            return word;
        }
    }

    bug("Fread_word: word too long.", 0);
    exit(1);
    return NULL;
}

/*
 * Allocate some ordinary memory,
 *   with the expectation of freeing it someday.
 */
void* alloc_mem(size_t sMem)
{
    uintptr_t mem_addr;
    int* magic;
    int iList;

    sMem += sizeof(*magic); 

    for (iList = 0; iList < MAX_MEM_LIST; iList++) {
        if (sMem <= rgSizeList[iList]) break;
    }

    if (iList == MAX_MEM_LIST) {
        bug("Alloc_mem: size %zu too large.", sMem);
        exit(1);
    }

    if (rgFreeList[iList] == NULL) { 
        mem_addr = (uintptr_t)alloc_perm(rgSizeList[iList]); 
    }
    else {
        mem_addr = (uintptr_t)rgFreeList[iList];
        rgFreeList[iList] = *((void**)rgFreeList[iList]);
    }

    magic = (int*)mem_addr;
    *magic = MAGIC_NUM;
    mem_addr += sizeof(*magic);

    return (void*)mem_addr;
}

/*
 * Free some memory.
 * Recycle it back onto the free list for blocks of that size.
 */
void free_mem(void* pMem, size_t sMem)
{
    int iList;
    int* magic;
    uintptr_t mem_addr = (uintptr_t)pMem;

    mem_addr -= sizeof(*magic); 
    magic = (int*)mem_addr;

    if (*magic != MAGIC_NUM) {
        bug("Attempt to recyle invalid memory of size %zu.", sMem);
        bug((char*)mem_addr + sizeof(*magic), 0);
        return;
    }

    *magic = 0;
    sMem += sizeof(*magic);

    for (iList = 0; iList < MAX_MEM_LIST; iList++) {
        if (sMem <= rgSizeList[iList]) break;
    }

    if (iList == MAX_MEM_LIST) {
        bug("Free_mem: size %zu too large.", sMem);
        exit(1);
    }

    pMem = (void*)mem_addr;
    *((void**)pMem) = rgFreeList[iList];
    rgFreeList[iList] = pMem;

    return;
}
/*
 * Allocate some permanent memory.
 * Permanent memory is never freed,
 *   pointers into it may be copied safely.
 */
void* alloc_perm(size_t sMem)
{
    static char* pMemPerm;
    static size_t iMemPerm;
    void* pMem;

    while (sMem % sizeof(long) != 0) sMem++;
    if (sMem > MAX_PERM_BLOCK) {
        bug("Alloc_perm: %zu too large.", sMem);
        exit(1);
    }

    if (pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK) {
        iMemPerm = 0;
        if ((pMemPerm = calloc(1, MAX_PERM_BLOCK)) == NULL) {
            perror("Alloc_perm");
            exit(1);
        }
    }

    pMem = pMemPerm + iMemPerm;
    iMemPerm += sMem;
    nAllocPerm += 1;
    sAllocPerm += sMem;
    return pMem;
}

/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char* str_dup(const char* str)
{
    char* str_new;

    if (str[0] == '\0') return &str_empty[0];

    if (str >= string_space && str < top_string) return (char*)str;

    str_new = alloc_mem(strlen(str) + 1);
    strcpy(str_new, str);
    return str_new;
}

/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void free_string(char* pstr)
{
    if (pstr == NULL || pstr == &str_empty[0]
        || (pstr >= string_space && pstr < top_string))
        return;

    free_mem(pstr, strlen(pstr) + 1);
    return;
}

void do_areas(CHAR_DATA* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    AREA_DATA* pArea1;
    AREA_DATA* pArea2;
    int iArea;
    int iAreaHalf;

    if (argument[0] != '\0') {
        send_to_char("No argument is used with this command.\n\r", ch);
        return;
    }

    iAreaHalf = (top_area + 1) / 2;
    pArea1 = area_first;
    pArea2 = area_first;
    for (iArea = 0; iArea < iAreaHalf; iArea++) pArea2 = pArea2->next;

    for (iArea = 0; iArea < iAreaHalf; iArea++) {
        sprintf(buf, "%-39s%-39s\n\r", pArea1->credits,
                (pArea2 != NULL) ? pArea2->credits : "");
        send_to_char(buf, ch);
        pArea1 = pArea1->next;
        if (pArea2 != NULL) pArea2 = pArea2->next;
    }

    return;
}

void do_memory(CHAR_DATA* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Affects %5d\n\r", top_affect);
    send_to_char(buf, ch);
    sprintf(buf, "Areas   %5d\n\r", top_area);
    send_to_char(buf, ch);
    sprintf(buf, "ExDes   %5d\n\r", top_ed);
    send_to_char(buf, ch);
    sprintf(buf, "Exits   %5d\n\r", top_exit);
    send_to_char(buf, ch);
    sprintf(buf, "Helps   %5d\n\r", top_help);
    send_to_char(buf, ch);
    sprintf(buf, "Socials %5d\n\r", social_count);
    send_to_char(buf, ch);
    sprintf(buf, "Mobs    %5d(%d new format)\n\r", top_mob_index, newmobs);
    send_to_char(buf, ch);
    sprintf(buf, "(in use)%5d\n\r", mobile_count);
    send_to_char(buf, ch);
    sprintf(buf, "Objs    %5d(%d new format)\n\r", top_obj_index, newobjs);
    send_to_char(buf, ch);
    sprintf(buf, "Resets  %5d\n\r", top_reset);
    send_to_char(buf, ch);
    sprintf(buf, "Rooms   %5d\n\r", top_room);
    send_to_char(buf, ch);
    sprintf(buf, "Shops   %5d\n\r", top_shop);
    send_to_char(buf, ch);

    sprintf(buf, "Strings %5d strings of %zu bytes (max %d).\n\r", nAllocString,
            sAllocString, MAX_STRING);
    send_to_char(buf, ch);

    sprintf(buf, "Perms   %5d blocks  of %zu bytes.\n\r", nAllocPerm,
            sAllocPerm);
    send_to_char(buf, ch);

    return;
}

void do_dump(CHAR_DATA* ch, char* argument)
{
    int count, count2, num_pcs, aff_count;
    CHAR_DATA* fch;
    MOB_INDEX_DATA* pMobIndex;
    PC_DATA* pc;
    OBJ_DATA* obj;
    OBJ_INDEX_DATA* pObjIndex;
    ROOM_INDEX_DATA* room = NULL;
    EXIT_DATA* exit = NULL;
    DESCRIPTOR_DATA* d;
    AFFECT_DATA* af;
    FILE* fp;
    int vnum, nMatch = 0;

    /* open file */
    fclose(fpReserve);
    fp = fopen("mem.dmp", "w");

    /* report use of data structures */

    num_pcs = 0;
    aff_count = 0;

    /* mobile prototypes */
    fprintf(fp, "MobProt    %4d (%8zu bytes)\n", top_mob_index,
            top_mob_index * (sizeof(*pMobIndex)));

    /* mobs */
    count = 0;
    count2 = 0;
    for (fch = char_list; fch != NULL; fch = fch->next) {
        count++;
        if (fch->pcdata != NULL) num_pcs++;
        for (af = fch->affected; af != NULL; af = af->next) aff_count++;
    }
    for (fch = char_free; fch != NULL; fch = fch->next) count2++;

    fprintf(fp, "Mobs   %4d (%8zu bytes), %2d free (%zu bytes)\n", count,
            count * (sizeof(*fch)), count2, count2 * (sizeof(*fch)));

    /* pcdata */
    count = 0;
    for (pc = pcdata_free; pc != NULL; pc = pc->next) count++;

    fprintf(fp, "Pcdata	%4d (%8zu bytes), %2d free (%zu bytes)\n", num_pcs,
            num_pcs * (sizeof(*pc)), count, count * (sizeof(*pc)));

    /* descriptors */
    count = 0;
    count2 = 0;
    for (d = descriptor_list; d != NULL; d = d->next) count++;
    for (d = descriptor_free; d != NULL; d = d->next) count2++;

    fprintf(fp, "Descs	%4d (%8zu bytes), %2d free (%zu bytes)\n", count,
            count * (sizeof(*d)), count2, count2 * (sizeof(*d)));

    /* object prototypes */
    for (vnum = 0; nMatch < top_obj_index; vnum++)
        if ((pObjIndex = get_obj_index(vnum)) != NULL) {
            for (af = pObjIndex->affected; af != NULL; af = af->next)
                aff_count++;
            nMatch++;
        }

    fprintf(fp, "ObjProt	%4d (%8zu bytes)\n", top_obj_index,
            top_obj_index * (sizeof(*pObjIndex)));

    /* objects */
    count = 0;
    count2 = 0;
    for (obj = object_list; obj != NULL; obj = obj->next) {
        count++;
        for (af = obj->affected; af != NULL; af = af->next) aff_count++;
    }
    for (obj = obj_free; obj != NULL; obj = obj->next) count2++;

    fprintf(fp, "Objs	%4d (%8zu bytes), %2d free (%zu bytes)\n", count,
            count * (sizeof(*obj)), count2, count2 * (sizeof(*obj)));

    /* affects */
    count = 0;
    for (af = affect_free; af != NULL; af = af->next) count++;

    fprintf(fp, "Affects	%4d (%8zu bytes), %2d free (%zu bytes)\n", aff_count,
            aff_count * (sizeof(*af)), count, count * (sizeof(*af)));

    /* rooms */
    fprintf(fp, "Rooms	%4d (%8zu bytes)\n", top_room,
            top_room * (sizeof(*room)));

    /* exits */
    fprintf(fp, "Exits	%4d (%8zu bytes)\n", top_exit,
            top_exit * (sizeof(*exit)));

    fclose(fp);

    /* start printing out mobile data */
    fp = fopen("mob.dmp", "w");

    fprintf(fp, "\nMobile Analysis\n");
    fprintf(fp, "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_mob_index; vnum++)
        if ((pMobIndex = get_mob_index(vnum)) != NULL) {
            nMatch++;
            fprintf(fp, "#%-4d %3d active %3d killed     %s\n", pMobIndex->vnum,
                    pMobIndex->count, pMobIndex->killed,
                    pMobIndex->short_descr);
        }
    fclose(fp);

    /* start printing out object data */
    fp = fopen("obj.dmp", "w");

    fprintf(fp, "\nObject Analysis\n");
    fprintf(fp, "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_obj_index; vnum++)
        if ((pObjIndex = get_obj_index(vnum)) != NULL) {
            nMatch++;
            fprintf(fp, "#%-4d %3d active %3d reset      %s\n", pObjIndex->vnum,
                    pObjIndex->count, pObjIndex->reset_num,
                    pObjIndex->short_descr);
        }

    /* close file */
    fclose(fp);
    fpReserve = fopen(NULL_FILE, "r");
}

/*
 * Stick a little fuzz on a number.
 */
int number_fuzzy(int number)
{
    switch (number_bits(2)) {
    case 0:
        number -= 1;
        break;
    case 3:
        number += 1;
        break;
    }

    return UMAX(1, number);
}

/*
 * Generate a random number.
 */
int number_range(int from, int to)
{
    int power;
    int number;

    if (from == 0 && to == 0) return 0;

    if ((to = to - from + 1) <= 1) return from;

    for (power = 2; power < to; power <<= 1)
        ;

    while ((number = number_mm() & (power - 1)) >= to)
        ;

    return from + number;
}

/*
 * Generate a percentile roll.
 */
int number_percent(void)
{
    int percent;

    while ((percent = number_mm() & (128 - 1)) > 99)
        ;

    return 1 + percent;
}

/*
 * Generate a random door.
 */
int number_door(void)
{
    int door;

    while ((door = number_mm() & (8 - 1)) > 5)
        ;

    return door;
}

int number_bits(int width)
{
    return number_mm() & ((1 << width) - 1);
}

void init_mm()
{
    int rounds = 5;

    // Seed with external entropy -- the time and some program addresses
    pcg32_srandom(time(NULL) ^ (intptr_t)&printf, (intptr_t)&rounds);
}

long number_mm(void)
{
    return pcg32_random();
}

/*
 * Roll some dice.
 */
int dice(int number, int size)
{
    int idice;
    int sum;

    switch (size) {
    case 0:
        return 0;
    case 1:
        return number;
    }

    for (idice = 0, sum = 0; idice < number; idice++)
        sum += number_range(1, size);

    return sum;
}

/*
 * Simple linear interpolation.
 */
int interpolate(int level, int value_00, int value_32)
{
    return value_00 + level * (value_32 - value_00) / 32;
}

/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde(char* str)
{
    for (; *str != '\0'; str++) {
        if (*str == '~') *str = '-';
    }

    return;
}

/*
 * Compare strings, case insensitive.
 * Return true if different
 *   (compatibility with historical functions).
 */
bool str_cmp(const char* astr, const char* bstr)
{
    if (astr == NULL) {
        bug("Str_cmp: null astr.", 0);
        return true;
    }

    if (bstr == NULL) {
        bug("Str_cmp: null bstr.", 0);
        return true;
    }

    for (; *astr || *bstr; astr++, bstr++) {
        if (LOWER(*astr) != LOWER(*bstr)) return true;
    }

    return false;
}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return true if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix(const char* astr, const char* bstr)
{
    if (astr == NULL) {
        bug("Strn_cmp: null astr.", 0);
        return true;
    }

    if (bstr == NULL) {
        bug("Strn_cmp: null bstr.", 0);
        return true;
    }

    for (; *astr; astr++, bstr++) {
        if (LOWER(*astr) != LOWER(*bstr)) return true;
    }

    return false;
}

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns true is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix(const char* astr, const char* bstr)
{
    int ichar;
    char c0;

    if ((c0 = LOWER(astr[0])) == '\0') return false;

    size_t sstr1 = strlen(astr);
    size_t sstr2 = strlen(bstr);

    for (ichar = 0; ichar <= sstr2 - sstr1; ichar++) {
        if (c0 == LOWER(bstr[ichar]) && !str_prefix(astr, bstr + ichar))
            return false;
    }

    return true;
}

/*
 * Compare strings, case insensitive, for suffix matching.
 * Return true if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix(const char* astr, const char* bstr)
{
    size_t sstr1 = strlen(astr);
    size_t sstr2 = strlen(bstr);
    if (sstr1 <= sstr2 && !str_cmp(astr, bstr + sstr2 - sstr1))
        return false;
    else
        return true;
}

/*
 * Returns an initial-capped string.
 */
char* capitalize(const char* str)
{
    static char strcap[MAX_STRING_LENGTH];
    int i;

    for (i = 0; str[i] != '\0'; i++)
        strcap[i] = LOWER(str[i]);
    strcap[i] = '\0';
    strcap[0] = UPPER(strcap[0]);
    return strcap;
}

/*
 * Append a string to a file.
 */
void append_file(CHAR_DATA* ch, char* file, char* str)
{
    FILE* fp;

    if (IS_NPC(ch) || str[0] == '\0') return;

    fclose(fpReserve);
    if ((fp = fopen(file, "a")) == NULL) {
        perror(file);
        send_to_char("Could not open the file!\n\r", ch);
    }
    else {
        fprintf(fp, "[%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0,
                ch->name, str);
        fclose(fp);
    }

    fpReserve = fopen(NULL_FILE, "r");
    return;
}

/*
 * Reports a bug.
 */
void bug(const char* fmt, ...)
{
    char buf[MAX_STRING_LENGTH];

    va_list args;
    va_start(args, fmt);

    if (fpArea != NULL) {
        int iLine;
        int iChar;

        if (fpArea == stdin) { iLine = 0; }
        else {
            iChar = ftell(fpArea);
            fseek(fpArea, 0, 0);
            for (iLine = 0; ftell(fpArea) < iChar; iLine++) {
                while (getc(fpArea) != '\n')
                    ;
            }
            fseek(fpArea, iChar, 0);
        }

        sprintf(buf, "[*****] FILE: %s LINE: %d", strArea, iLine);
        log_string(buf);
    }

    strcpy(buf, "[*****] BUG: ");
    vsprintf(buf + strlen(buf), fmt, args);
    log_string(buf);

    return;
}

/*
 * Writes a string to the log.
 */
void log_string(const char* str)
{
    char* strtime;

    strtime = ctime(&current_time);
    strtime[strlen(strtime) - 1] = '\0';
    fprintf(stderr, "%s :: %s\n", strtime, str);
    return;
}

/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain(void)
{
    return;
}
