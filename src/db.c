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

#include "db.h"

#include "merc.h"

#include "act_move.h"
#include "act_wiz.h"
#include "ban.h"
#include "comm.h"
#include "handler.h"
#include "lookup.h"
#include "magic.h"
#include "music.h"
#include "note.h"
#include "olc.h"
#include "pcg_basic.h"
#include "recycle.h"
#include "skills.h"
#include "special.h"
#include "strings.h"
#include "tables.h"

#include "entities/area_data.h"
#include "entities/descriptor.h"
#include "entities/exit_data.h"
#include "entities/mob_prototype.h"
#include "entities/object_data.h"
#include "entities/player_data.h"
#include "entities/reset_data.h"
#include "entities/room_data.h"
#include "entities/shop_data.h"

#include "data/direction.h"
#include "data/mobile.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifdef _MSC_VER 
#include <Windows.h>
#else
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#endif

MobProgCode* pedit_prog(VNUM);

/*
 * Globals.
 */

MobProgCode* mprog_list;

char bug_buf[2 * MAX_INPUT_LENGTH];
char* help_greeting;
char log_buf[2 * MAX_INPUT_LENGTH];
KILL_DATA kill_table[MAX_LEVEL];
TIME_INFO_DATA time_info;
WEATHER_DATA weather_info;

#define GSN(x) SKNUM x;
#include "gsn.h"
#undef GSN

/*
 * Locals.
 */
char* string_hash[MAX_KEY_HASH];

char* string_space;
char* top_string;
char str_empty[1];

int top_affect;
int top_ed;
int top_help;
int	top_mprog_index;    // OLC

/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
#define MAX_STRING     1413120
#define MAX_PERM_BLOCK 131072
#define MAX_MEM_LIST   14

void* rgFreeList[MAX_MEM_LIST];
const size_t rgSizeList[MAX_MEM_LIST] = {
    16, 32, 64, 128, 256, 1024, 2048, 4096,
    MAX_STRING_LENGTH,      // vvv
    8192,
    MAX_STRING_LENGTH * 2,    // Doesn't follow pattern, but frequently used.
    16384,
    MAX_STRING_LENGTH * 4,     // ^^^
    32768 - 64
};

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
AreaData* current_area;

/*
 * Local booting procedures.
 */
void init_mm args((void));
void load_area args((FILE * fp)); 
void new_load_area args((FILE * fp));   // OOC
void load_helps	args((FILE * fp, char* fname));
void load_old_obj args((FILE * fp));
void load_objects args((FILE * fp));
void load_resets args((FILE * fp));
void load_rooms args((FILE * fp));
void load_shops args((FILE * fp));
void load_socials args((FILE * fp));
void load_specials args((FILE * fp));
void load_notes args((void));
void load_mobprogs args((FILE * fp));
void load_groups(void);

void fix_exits args((void));
void fix_mobprogs args((void));

void reset_area args((AreaData * pArea));

static bool check_dir(const char* dir)
{
    char buf[256];
    sprintf(buf, "%s%s", area_dir, dir);

#ifdef _MSC_VER
    if (CreateDirectoryA(buf, NULL) == 0)  {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            bugf("Could not create directory: %s\n", buf);
            return false;
        }
    }
    else {
        printf("Created directory: %s\n", buf);
    }
#else
    struct stat st = { 0 };

    if (stat(buf, &st) == -1) {
        mkdir(buf, 0700);
        if (stat(buf, &st) == -1) {
            bugf("Could not create directory: %s\n", buf);
            return false;
        }
        else {
            printf("Created directory: %s\n", buf);
        }
    }

#endif

    return true;
}

/*
 * Big mama top level function.
 */
void boot_db(void)
{
    check_dir(PLAYER_DIR);
    check_dir(GOD_DIR);
    check_dir(TEMP_DIR);
    check_dir(DATA_DIR);
    check_dir(PROG_DIR);

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

    load_skills_table();
    load_races_table();
    load_command_table();
    load_socials_table();
    load_groups();

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
        SKNUM sn;

        for (sn = 0; sn < max_skill; sn++) {
            if (skill_table[sn].pgsn != NULL) 
                *skill_table[sn].pgsn = sn;
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

        current_area = NULL;

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
                else if (!str_cmp(word, "AREADATA")) 
                    new_load_area(fpArea);
                else if (!str_cmp(word, "HELPS")) 
                    load_helps(fpArea, strArea);
                else if (!str_cmp(word, "MOBOLD"))
                    load_old_mob(fpArea);
                else if (!str_cmp(word, "MOBILES"))
                    load_mobiles(fpArea);
                else if (!str_cmp(word, "MOBPROGS"))
                    load_mobprogs(fpArea);
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

            if (fpArea != stdin) 
                fclose(fpArea);
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
        fix_mobprogs();
        fBootDb = false;
        convert_objects();  // OLC
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
    AreaData* pArea;

    pArea = alloc_perm(sizeof(*pArea));
    pArea->file_name = fread_string(fp);

    pArea->area_flags = AREA_LOADING;   // OLC
    pArea->security = 9;                // OLC 9 -- Hugin */
    pArea->builders = str_dup("None");  // OLC
    pArea->vnum = top_area;             // OLC

    pArea->name = fread_string(fp);
    pArea->credits = fread_string(fp);
    pArea->min_vnum = fread_number(fp);
    pArea->max_vnum = fread_number(fp);
    pArea->age = 15;
    pArea->nplayer = 0;
    pArea->empty = false;

    if (area_first == NULL) 
        area_first = pArea;

    if (area_last != NULL) {
        area_last->next = pArea;
        REMOVE_BIT(area_last->area_flags, AREA_LOADING);    // OLC
    }

    area_last = pArea;
    pArea->next = NULL;
    current_area = pArea;

    top_area++;
    return;
}

/*
 * OLC
 * Use these macros to load any new area formats that you choose to
 * support on your MUD.  See the new_load_area format below for
 * a short example.
 */
#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )    \
    if (!str_cmp(word, literal)) {      \
        field  = value;                 \
        break;                          \
    }

#define SKEY( string, field )           \
    if (!str_cmp(word, string)) {       \
        free_string(field);             \
        field = fread_string(fp);       \
        break;                          \
    }

/* OLC
 * Snarf an 'area' header line.   Check this format.  MUCH better.  Add fields
 * too.
 *
 * #AREAFILE
 * Name   { All } Locke    Newbie School~
 * Repop  A teacher pops in the room and says, 'Repop coming!'~
 * Recall 3001
 * End
 */
void new_load_area(FILE* fp)
{
    AreaData* pArea;
    char* word;

    pArea = alloc_perm(sizeof(*pArea));
    pArea->age = 15;
    pArea->nplayer = 0;
    pArea->file_name = str_dup(strArea);
    pArea->vnum = top_area;
    pArea->name = str_dup("New Area");
    pArea->builders = str_dup("");
    pArea->security = 9;                    /* 9 -- Hugin */
    pArea->min_vnum = 0;
    pArea->max_vnum = 0;
    pArea->area_flags = 0;
/*  pArea->recall       = ROOM_VNUM_TEMPLE;        ROM OLC */

    for (; ; ) {
        word = feof(fp) ? "End" : fread_word(fp);

        switch (UPPER(word[0])) {
        case 'N':
            SKEY("Name", pArea->name);
            break;
        case 'S':
            KEY("Security", pArea->security, fread_number(fp));
            break;
        case 'V':
            if (!str_cmp(word, "VNUMs")) {
                pArea->min_vnum = fread_number(fp);
                pArea->max_vnum = fread_number(fp);
            }
            break;
        case 'E':
            if (!str_cmp(word, "End")) {
                if (area_first == NULL)
                    area_first = pArea;
                if (area_last != NULL)
                    area_last->next = pArea;
                area_last = pArea;
                pArea->next = NULL;
                current_area = pArea;
                top_area++;

                return;
            }
            break;
        case 'B':
            SKEY("Builders", pArea->builders);
            break;
        case 'C':
            SKEY("Credits", pArea->credits);
            break;
        }
    }
}

/*
 * Sets vnum range for area using OLC protection features.
 */
void assign_area_vnum(VNUM vnum)
{
    if (area_last->min_vnum == 0 || area_last->max_vnum == 0)
        area_last->min_vnum = area_last->max_vnum = vnum;
    if (vnum != URANGE(area_last->min_vnum, vnum, area_last->max_vnum)) {
        if (vnum < area_last->min_vnum)
            area_last->min_vnum = vnum;
        else
            area_last->max_vnum = vnum;
    }
    return;
}

/*
 * Snarf a help section.
 */
void load_helps(FILE* fp, char* fname)
{
    HelpData* pHelp;
    LEVEL level;
    char* keyword;

    for (;;) {
        HelpArea* had = NULL;

        level = (LEVEL)fread_number(fp);
        keyword = fread_string(fp);

        if (keyword[0] == '$')
            break;

        if (help_area_list == NULL) {
            if ((had = new_help_area()) == NULL) {
                perror("load_helps (1): could not allocate new HelpArea!");
                exit(-1);
            }
            had->filename = str_dup(fname);
            had->area = current_area;
            if (current_area)
                current_area->helps = had;
            help_area_list = had;
        }
        else if (str_cmp(fname, help_area_list->filename)) {
            if ((had = new_help_area()) == NULL) {
                perror("load_helps (2): could not allocate new HelpArea!");
                exit(-1);
            }
            had->filename = str_dup(fname);
            had->area = current_area;
            if (current_area)
                current_area->helps = had;
            had->next = help_area_list;
            help_area_list = had;
        }
        else
            had = help_area_list;
        
        if ((pHelp = new_help_data()) == NULL) {
            perror("load_helps: could not allocate new HelpData!");
            exit(-1);
        }
        pHelp->level = level;
        pHelp->keyword = keyword;
        pHelp->text = fread_string(fp);

        if (!str_cmp(pHelp->keyword, "greeting")) 
            help_greeting = pHelp->text;

        if (help_first == NULL)
            help_first = pHelp;
        if (help_last != NULL)
            help_last->next = pHelp;

        help_last = pHelp;
        pHelp->next = NULL;

        if (!had->first)
            had->first = pHelp;
        if (!had->last)
            had->last = pHelp;

        had->last->next_area = pHelp;
        had->last = pHelp;
        pHelp->next_area = NULL;

        top_help++;
    }

    return;
}

/*
 * Snarf an obj section.  old style
 */
void load_old_obj(FILE* fp)
{
    ObjectPrototype* obj_proto;

    if (!area_last) {
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
        if (get_object_prototype(vnum) != NULL) {
            bug("Load_objects: vnum %"PRVNUM" duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        obj_proto = alloc_perm(sizeof(*obj_proto));
        obj_proto->vnum = vnum;
        obj_proto->area = area_last;    // OLC
        obj_proto->new_format = false;
        obj_proto->reset_num = 0;
        obj_proto->name = fread_string(fp);
        obj_proto->short_descr = fread_string(fp);
        obj_proto->description = fread_string(fp);
        /* Action description */ fread_string(fp);

        obj_proto->short_descr[0] = LOWER(obj_proto->short_descr[0]);
        obj_proto->description[0] = UPPER(obj_proto->description[0]);
        obj_proto->material = str_dup("");

        obj_proto->item_type = (int16_t)fread_number(fp);
        obj_proto->extra_flags = fread_flag(fp);
        obj_proto->wear_flags = fread_flag(fp);
        obj_proto->value[0] = fread_number(fp);
        obj_proto->value[1] = fread_number(fp);
        obj_proto->value[2] = fread_number(fp);
        obj_proto->value[3] = fread_number(fp);
        obj_proto->value[4] = 0;
        obj_proto->level = 0;
        obj_proto->condition = 100;
        obj_proto->weight = (int16_t)fread_number(fp);
        obj_proto->cost = fread_number(fp); /* Unused */
        /* Cost per day */ fread_number(fp);

        if (obj_proto->item_type == ITEM_WEAPON) {
            if (is_name("two", obj_proto->name)
                || is_name("two-handed", obj_proto->name)
                || is_name("claymore", obj_proto->name))
                SET_BIT(obj_proto->value[4], WEAPON_TWO_HANDS);
        }

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'A') {
                AffectData* paf;

                paf = alloc_perm(sizeof(*paf));
                paf->where = TO_OBJECT;
                paf->type = -1;
                paf->level = 20; /* RT temp fix */
                paf->duration = -1;
                paf->location = (int16_t)fread_number(fp);
                paf->modifier = (int16_t)fread_number(fp);
                paf->bitvector = 0;
                paf->next = obj_proto->affected;
                obj_proto->affected = paf;
                top_affect++;
            }

            else if (letter == 'E') {
                ExtraDesc* ed;

                ed = alloc_perm(sizeof(*ed));
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ed->next = obj_proto->extra_desc;
                obj_proto->extra_desc = ed;
                top_ed++;
            }

            else {
                ungetc(letter, fp);
                break;
            }
        }

        /* fix armors */
        if (obj_proto->item_type == ITEM_ARMOR) {
            obj_proto->value[1] = obj_proto->value[0];
            obj_proto->value[2] = obj_proto->value[1];
        }

        /*
         * Translate spell "slot numbers" to internal "skill numbers."
         */
        switch (obj_proto->item_type) {
        case ITEM_PILL:
        case ITEM_POTION:
        case ITEM_SCROLL:
            obj_proto->value[1] = skill_slot_lookup(obj_proto->value[1]);
            obj_proto->value[2] = skill_slot_lookup(obj_proto->value[2]);
            obj_proto->value[3] = skill_slot_lookup(obj_proto->value[3]);
            obj_proto->value[4] = skill_slot_lookup(obj_proto->value[4]);
            break;

        case ITEM_STAFF:
        case ITEM_WAND:
            obj_proto->value[3] = skill_slot_lookup(obj_proto->value[3]);
            break;

        default:
            break;
        }

        iHash = vnum % MAX_KEY_HASH;
        obj_proto->next = object_prototype_hash[iHash];
        object_prototype_hash[iHash] = obj_proto;
        top_object_prototype++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;   // OLC
        assign_area_vnum( vnum );                                   // OLC
    }

    return;
}

/*
 * Adds a reset to a room.
 * Similar to add_reset in olc.c
 */
void new_reset(RoomData* pR, ResetData* pReset)
{
    ResetData* pr;

    if (!pR)
        return;

    pr = pR->reset_last;

    if (!pr) {
        pR->reset_first = pReset;
        pR->reset_last = pReset;
    }
    else {
        pR->reset_last->next = pReset;
        pR->reset_last = pReset;
        pR->reset_last->next = NULL;
    }

    // top_reset++; We aren't allocating memory!!!! 

    return;
}

/*
 * Snarf a reset section.
 */
void load_resets(FILE* fp)
{
    ResetData* pReset;
    ExitData* pexit;
    RoomData* pRoomIndex;
    VNUM rVnum = VNUM_NONE;

    if (area_last == NULL) {
        bug("Load_resets: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        char letter;

        if ((letter = fread_letter(fp)) == 'S') break;

        if (letter == '*') {
            fread_to_eol(fp);
            continue;
        }

        pReset = new_reset_data();
        pReset->command = letter;
        /* if_flag */ fread_number(fp);
        pReset->arg1 = fread_vnum(fp);
        pReset->arg2 = (int16_t)fread_number(fp);
        pReset->arg3 = (letter == 'G' || letter == 'R') ? 0 : fread_vnum(fp);
        pReset->arg4 = (letter == 'P' || letter == 'M') ? (int16_t)fread_number(fp) : 0;
        fread_to_eol(fp);

        switch (pReset->command) {
        case 'M':
        case 'O':
            rVnum = pReset->arg3;
            break;

        case 'P':
        case 'G':
        case 'E':
            break;

        case 'D':
            pRoomIndex = get_room_data((rVnum = pReset->arg1));
            if (pReset->arg2 < 0
                || pReset->arg2 >= DIR_MAX
                || !pRoomIndex
                || !(pexit = pRoomIndex->exit[pReset->arg2])
                || !IS_SET(pexit->exit_reset_flags, EX_ISDOOR)) {
                bugf("Load_resets: 'D': exit %d, room %"PRVNUM" not door.", pReset->arg2, pReset->arg1);
                exit(1);
            }

            switch (pReset->arg3) {
            default: bug("Load_resets: 'D': bad 'locks': %d.", pReset->arg3); break;
            case 0: 
                break;
            case 1: 
                SET_BIT(pexit->exit_reset_flags, EX_CLOSED);
                SET_BIT(pexit->exit_flags, EX_CLOSED); 
                break;
            case 2: 
                SET_BIT(pexit->exit_reset_flags, EX_CLOSED | EX_LOCKED);
                SET_BIT(pexit->exit_flags, EX_CLOSED | EX_LOCKED); 
                break;
            }
            break;

        case 'R':
            rVnum = pReset->arg1;
            break;
        }

        if (rVnum == VNUM_NONE) {
            bugf("load_resets : rVnum == VNUM_NONE");
            exit(1);
        }

        if (pReset->command != 'D')
            new_reset(get_room_data(rVnum), pReset);
        else
            free_reset_data(pReset);
    }

    return;
}

/*
 * Snarf a room section.
 */
void load_rooms(FILE* fp)
{
    RoomData* pRoomIndex;

    if (area_last == NULL) {
        bug("Load_resets: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        VNUM vnum;
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
        if (get_room_data(vnum) != NULL) {
            bug("Load_rooms: vnum %"PRVNUM" duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        pRoomIndex = alloc_perm(sizeof(*pRoomIndex));
        pRoomIndex->owner = str_dup("");
        pRoomIndex->people = NULL;
        pRoomIndex->contents = NULL;
        pRoomIndex->extra_desc = NULL;
        pRoomIndex->area = area_last;
        pRoomIndex->vnum = vnum;
        pRoomIndex->name = fread_string(fp);
        pRoomIndex->description = fread_string(fp);
        /* Area number */ fread_number(fp);
        pRoomIndex->room_flags = fread_flag(fp);
        /* horrible hack */
        if (3000 <= vnum && vnum < 3400)
            SET_BIT(pRoomIndex->room_flags, ROOM_LAW);
        pRoomIndex->sector_type = (int16_t)fread_number(fp);
        pRoomIndex->light = 0;
        for (door = 0; door <= 5; door++) pRoomIndex->exit[door] = NULL;

        /* defaults */
        pRoomIndex->heal_rate = 100;
        pRoomIndex->mana_rate = 100;

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'S') break;

            if (letter == 'H') /* healing room */
                pRoomIndex->heal_rate = (int16_t)fread_number(fp);

            else if (letter == 'M') /* mana room */
                pRoomIndex->mana_rate = (int16_t)fread_number(fp);

            else if (letter == 'C') /* clan */
            {
                if (pRoomIndex->clan) {
                    bug("Load_rooms: duplicate clan fields.", 0);
                    exit(1);
                }
                pRoomIndex->clan = (int16_t)clan_lookup(fread_string(fp));
            }

            else if (letter == 'D') {
                int locks;

                door = fread_number(fp);
                if (door < 0 || door > 5) {
                    bug("Fread_rooms: vnum %"PRVNUM" has bad door number.", vnum);
                    exit(1);
                }

                ExitData* pexit = alloc_perm(sizeof(ExitData));
                pexit->description = fread_string(fp);
                pexit->keyword = fread_string(fp);
                pexit->exit_flags = 0;
                pexit->exit_reset_flags = 0;        // OLC
                locks = fread_number(fp);
                pexit->key = (int16_t)fread_number(fp);
                pexit->u1.vnum = fread_number(fp);
                pexit->orig_dir = door;    // OLC

                switch (locks) {
                case 1: 
                    pexit->exit_flags = EX_ISDOOR;
                    pexit->exit_reset_flags = EX_ISDOOR;
                    break;
                case 2: 
                    pexit->exit_flags = EX_ISDOOR | EX_PICKPROOF;
                    pexit->exit_reset_flags = EX_ISDOOR | EX_PICKPROOF;
                    break;
                case 3: 
                    pexit->exit_flags = EX_ISDOOR | EX_NOPASS;
                    pexit->exit_reset_flags = EX_ISDOOR | EX_NOPASS;
                    break;
                case 4: 
                    pexit->exit_flags = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
                    pexit->exit_reset_flags = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
                    break;
                }

                pRoomIndex->exit[door] = pexit;
                top_exit++;
            }
            else if (letter == 'E') {
                ExtraDesc* ed;

                ed = alloc_perm(sizeof(*ed));
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ed->next = pRoomIndex->extra_desc;
                pRoomIndex->extra_desc = ed;
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
                bug("Load_rooms: vnum %"PRVNUM" has flag not 'DES'.", vnum);
                exit(1);
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        pRoomIndex->next = room_index_hash[iHash];
        room_index_hash[iHash] = pRoomIndex;
        top_room++;
        top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room;    // OLC
        assign_area_vnum(vnum);                                         // OLC
    }

    return;
}

/*
 * Snarf a shop section.
 */
void load_shops(FILE* fp)
{
    ShopData* pShop;

    for (;;) {
        MobPrototype* p_mob_proto;
        int iTrade;

        if ((pShop = (ShopData*)alloc_perm(sizeof(ShopData))) == NULL) {
            bug("load_shops: Failed to create shops.");
            exit(1);
        }

        pShop->keeper = (int16_t)fread_number(fp);
        if (pShop->keeper == 0) break;
        for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
            pShop->buy_type[iTrade] = (int16_t)fread_number(fp);
        pShop->profit_buy = (int16_t)fread_number(fp);
        pShop->profit_sell = (int16_t)fread_number(fp);
        pShop->open_hour = (int16_t)fread_number(fp);
        pShop->close_hour = (int16_t)fread_number(fp);
        fread_to_eol(fp);
        p_mob_proto = get_mob_prototype(pShop->keeper);
        p_mob_proto->pShop = pShop;

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
        MobPrototype* p_mob_proto;
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
            p_mob_proto = get_mob_prototype(fread_number(fp));
            p_mob_proto->spec_fun = spec_lookup(fread_word(fp));
            if (p_mob_proto->spec_fun == 0) {
                bug("Load_specials: 'M': vnum %"PRVNUM".", p_mob_proto->vnum);
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
    char buf[MAX_STRING_LENGTH];
    RoomData* pRoomIndex;
    RoomData* to_room;
    ExitData* pexit;
    ExitData* pexit_rev;
    ResetData* pReset;
    RoomData* iLastRoom;
    RoomData* iLastObj;
    int iHash;
    int door;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++) {
        for (pRoomIndex = room_index_hash[iHash]; pRoomIndex != NULL;
             pRoomIndex = pRoomIndex->next) {
            bool fexit;

            iLastRoom = iLastObj = NULL;

            /* OLC : New reset check */
            for (pReset = pRoomIndex->reset_first; pReset; pReset = pReset->next) {
                switch (pReset->command) {
                default:
                    bugf("fix_exits : room %d with reset cmd %c", pRoomIndex->vnum, pReset->command);
                    exit(1);
                    break;

                case 'M':
                    get_mob_prototype(pReset->arg1);
                    iLastRoom = get_room_data(pReset->arg3);
                    break;

                case 'O':
                    get_object_prototype(pReset->arg1);
                    iLastObj = get_room_data(pReset->arg3);
                    break;

                case 'P':
                    get_object_prototype(pReset->arg1);
                    if (iLastObj == NULL) {
                        bugf("fix_exits : reset in room %d with iLastObj NULL", pRoomIndex->vnum);
                        exit(1);
                    }
                    break;

                case 'G':
                case 'E':
                    get_object_prototype(pReset->arg1);
                    if (iLastRoom == NULL) {
                        bugf("fix_exits : reset in room %d with iLastRoom NULL", pRoomIndex->vnum);
                        exit(1);
                    }
                    iLastObj = iLastRoom;
                    break;

                case 'D':
                    bugf("???");
                    break;

                case 'R':
                    get_room_data(pReset->arg1);
                    if (pReset->arg2 < 0 || pReset->arg2 > DIR_MAX) {
                        bugf("fix_exits : reset in room %d with arg2 %d >= DIR_MAX",
                            pRoomIndex->vnum, pReset->arg2);
                        exit(1);
                    }
                    break;
                }
            }

            fexit = false;
            for (door = 0; door <= 5; door++) {
                if ((pexit = pRoomIndex->exit[door]) != NULL) {
                    if (pexit->u1.vnum <= 0
                        || get_room_data(pexit->u1.vnum) == NULL)
                        pexit->u1.to_room = NULL;
                    else {
                        fexit = true;
                        pexit->u1.to_room = get_room_data(pexit->u1.vnum);
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
                    && (pexit_rev = to_room->exit[dir_list[door].rev_dir]) != NULL
                    && pexit_rev->u1.to_room != pRoomIndex
                    && (pRoomIndex->vnum < 1200 || pRoomIndex->vnum > 1299)) {
                    sprintf(buf, "Fix_exits: %d:%d -> %d:%d -> %d.",
                            pRoomIndex->vnum, door, to_room->vnum,
                            dir_list[door].rev_dir,
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
    AreaData* pArea;
    char buf[MAX_STRING_LENGTH];

    for (pArea = area_first; pArea != NULL; pArea = pArea->next) {
        if (++pArea->age < 3) continue;

        /*
         * Check age and reset.
         * Note: Mud School resets every 3 minutes (not 15).
         */
        if ((!pArea->empty && (pArea->nplayer == 0 || pArea->age >= 15))
            || pArea->age >= 31) {
            RoomData* pRoomIndex;

            reset_area(pArea);
            sprintf(buf, "%s has just been reset.", pArea->name);
            wiznet(buf, NULL, NULL, WIZ_RESETS, 0, 0);

            pArea->age = (int16_t)number_range(0, 3);
            pRoomIndex = get_room_data(ROOM_VNUM_SCHOOL);
            if (pRoomIndex != NULL && pArea == pRoomIndex->area)
                pArea->age = 15 - 2;
            else if (pArea->nplayer == 0)
                pArea->empty = true;
        }
    }

    return;
}

/*
 * Load mobprogs section
 */
void load_mobprogs(FILE* fp)
{
    MobProgCode* pMprog;

    if (area_last == NULL) {
        bug("Load_mobprogs: no #AREA seen yet.", 0);
        exit(1);
    }

    for (; ; ) {
        VNUM vnum;
        char letter;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_mobprogs: # not found.", 0);
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0)
            break;

        fBootDb = false;
        if (get_mprog_index(vnum) != NULL) {
            bug("Load_mobprogs: vnum %"PRVNUM" duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        pMprog = alloc_perm(sizeof(*pMprog));
        pMprog->vnum = vnum;
        pMprog->code = fread_string(fp);
        if (mprog_list == NULL)
            mprog_list = pMprog;
        else {
            pMprog->next = mprog_list;
            mprog_list = pMprog;
        }
        top_mprog_index++;
    }
    return;
}

/*
 *  Translate mobprog vnums pointers to real code
 */
void fix_mobprogs(void)
{
    MobPrototype* p_mob_proto;
    MobProg* list;
    MobProgCode* prog;
    int iHash;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++) {
        for (p_mob_proto = mob_prototype_hash[iHash];
            p_mob_proto != NULL;
            p_mob_proto = p_mob_proto->next) {
            for (list = p_mob_proto->mprogs; list != NULL; list = list->next) {
                if ((prog = pedit_prog(list->vnum)) != NULL)
                    list->code = prog->code;
                else {
                    bug("Fix_mobprogs: code vnum %"PRVNUM" not found.", list->vnum);
                    exit(1);
                }
            }
        }
    }
}

/* OLC
 * Reset one room.  Called by reset_area and olc.
 */
void reset_room(RoomData* pRoom)
{
    ResetData* pReset;
    CharData* pMob = NULL;
    ObjectData* pObj;
    CharData* LastMob = NULL;
    ObjectData* LastObj = NULL;
    int iExit;
    int level = 0;
    bool last;

    if (!pRoom) return;

    last = false;

    for (iExit = 0; iExit < DIR_MAX; iExit++) {
        ExitData* pExit;
        if ((pExit = pRoom->exit[iExit])
            /*  && !IS_SET( pExit->exit_flags, EX_BASHED )   ROM OLC */) {
            pExit->exit_flags = pExit->exit_reset_flags;
            if ((pExit->u1.to_room != NULL)
                && ((pExit = pExit->u1.to_room->exit[dir_list[iExit].rev_dir]))) {
                  /* nail the other side */
                pExit->exit_flags = pExit->exit_reset_flags;
            }
        }
    }

    for (pReset = pRoom->reset_first; pReset != NULL; pReset = pReset->next) {
        MobPrototype* p_mob_proto;
        ObjectPrototype* obj_proto;
        ObjectPrototype* pObjToIndex;
        RoomData* pRoomIndex;
        char buf[MAX_STRING_LENGTH];
        int count, limit = 0;

        switch (pReset->command) {
        default:
            bug("Reset_room: bad command %c.", pReset->command);
            break;

        case 'M':
        {
            CharData* mob;

            if ((p_mob_proto = get_mob_prototype(pReset->arg1)) == NULL) {
                bug("Reset_room: 'M': bad vnum %"PRVNUM".", pReset->arg1);
                continue;
            }

            if ((pRoomIndex = get_room_data(pReset->arg3)) == NULL) {
                bug("Reset_area: 'R': bad vnum %"PRVNUM".", pReset->arg3);
                continue;
            }

            if (p_mob_proto->count >= pReset->arg2) {
                last = false;
                break;
            }

            /* */

            count = 0;
            for (mob = pRoomIndex->people; mob != NULL; mob = mob->next_in_room)
                if (mob->prototype == p_mob_proto) {
                    count++;
                    if (count >= pReset->arg4) {
                        last = false;
                        break;
                    }
                }

            if (count >= pReset->arg4) break;

            /* */

            pMob = create_mobile(p_mob_proto);

            /*
             * Some more hard coding.
             */
            if (room_is_dark(pRoom))
                SET_BIT(pMob->affect_flags, AFF_INFRARED);

            /*
             * Pet shop mobiles get ACT_PET set.
             */
            {
                RoomData* pRoomIndexPrev;

                pRoomIndexPrev = get_room_data(pRoom->vnum - 1);
                if (pRoomIndexPrev
                    && IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
                    SET_BIT(pMob->act_flags, ACT_PET);
            }

            char_to_room(pMob, pRoom);
            LastMob = pMob;
            level = URANGE(0, pMob->level - 2, LEVEL_HERO - 1); /* -1 ROM */
            last = true;
            break;
        }

        case 'O':
            if (!(obj_proto = get_object_prototype(pReset->arg1))) {
                bug("Reset_room: 'O' 1 : bad vnum %"PRVNUM"", pReset->arg1);
                sprintf(buf, "%"PRVNUM" %d %"PRVNUM" %d", pReset->arg1, pReset->arg2, pReset->arg3,
                    pReset->arg4);
                bug(buf, 1);
                continue;
            }

            if (!(pRoomIndex = get_room_data(pReset->arg3))) {
                bug("Reset_room: 'O' 2 : bad vnum %"PRVNUM".", pReset->arg3);
                sprintf(buf, "%"PRVNUM" %d %"PRVNUM" %d", pReset->arg1, pReset->arg2, pReset->arg3,
                    pReset->arg4);
                bug(buf, 1);
                continue;
            }

            if (pRoom->area->nplayer > 0
                || count_obj_list(obj_proto, pRoom->contents) > 0) {
                last = false;
                break;
            }

            pObj = create_object(obj_proto, (int16_t)UMIN(number_fuzzy(level),
                LEVEL_HERO - 1)); /* UMIN - ROM OLC */
            pObj->cost = 0;
            obj_to_room(pObj, pRoom);
            last = true;
            break;

        case 'P':
            if ((obj_proto = get_object_prototype(pReset->arg1)) == NULL) {
                bug("Reset_room: 'P': bad vnum %"PRVNUM".", pReset->arg1);
                continue;
            }

            if ((pObjToIndex = get_object_prototype(pReset->arg3)) == NULL) {
                bug("Reset_room: 'P': bad vnum %"PRVNUM".", pReset->arg3);
                continue;
            }

            if (pReset->arg2 > 50) /* old format */
                limit = 6;
            else if (pReset->arg2 == -1) /* no limit */
                limit = 999;
            else
                limit = pReset->arg2;

            if (pRoom->area->nplayer > 0
                || (LastObj = get_obj_type(pObjToIndex)) == NULL
                || (LastObj->in_room == NULL && !last)
                || (obj_proto->count >= limit /* && number_range(0,4) != 0 */)
                || (count = count_obj_list(obj_proto, LastObj->contains))
                > pReset->arg4) {
                last = false;
                break;
            }
            /* lastObj->level  -  ROM */

            while (count < pReset->arg4) {
                pObj = create_object(obj_proto, (int16_t)number_fuzzy(LastObj->level));
                obj_to_obj(pObj, LastObj);
                count++;
                if (obj_proto->count >= limit) break;
            }

            /* fix object lock state! */
            LastObj->value[1] = LastObj->prototype->value[1];
            last = true;
            break;

        case 'G':
        case 'E':
            if ((obj_proto = get_object_prototype(pReset->arg1)) == NULL) {
                bug("Reset_room: 'E' or 'G': bad vnum %"PRVNUM".", pReset->arg1);
                continue;
            }

            if (!last) break;

            if (!LastMob) {
                bug("Reset_room: 'E' or 'G': null mob for vnum %"PRVNUM".",
                    pReset->arg1);
                last = false;
                break;
            }

            if (LastMob->prototype->pShop != NULL) { // Shopkeeper?
                LEVEL olevel = 0;
                int i, j;

                if (!obj_proto->new_format) {
                    switch (obj_proto->item_type) {

                    case ITEM_PILL:
                    case ITEM_POTION:
                    case ITEM_SCROLL:
                        olevel = 53;
                        for (i = 1; i < 5; i++) {
                            if (obj_proto->value[i] > 0) {
                                for (j = 0; j < MAX_CLASS; j++) {
                                    olevel = UMIN(
                                        olevel, skill_table[obj_proto->value[i]]
                                        .skill_level[j]);
                                }
                            }
                        }

                        olevel = UMAX(0, (olevel * 3 / 4) - 2);
                        break;
                    case ITEM_WAND:
                        olevel = (LEVEL)number_range(10, 20);
                        break;
                    case ITEM_STAFF:
                        olevel = (LEVEL)number_range(15, 25);
                        break;
                    case ITEM_ARMOR:
                        olevel = (LEVEL)number_range(5, 15);
                        break;
                    case ITEM_WEAPON:
                        olevel = (LEVEL)number_range(5, 15);
                        break;
                    case ITEM_TREASURE:
                        olevel = (LEVEL)number_range(10, 20);
                        break;
                    default:
                        olevel = 0;
                        break;
                    }
                }

                pObj = create_object(obj_proto, olevel);
                SET_BIT(pObj->extra_flags, ITEM_INVENTORY);
            }
            else {
                // ROM OLC
                if (pReset->arg2 > 50) /* old format */
                    limit = 6;
                else if (pReset->arg2 == -1 || pReset->arg2 == 0) /* no limit */
                    limit = 999;
                else
                    limit = pReset->arg2;

                if (obj_proto->count < limit || number_range(0, 4) == 0) {
                    pObj = create_object(
                        obj_proto, UMIN((LEVEL)number_fuzzy(level), LEVEL_HERO - 1));
                    /* error message if it is too high */
                    if (pObj->level > LastMob->level + 3
                        || (pObj->item_type == ITEM_WEAPON
                            && pReset->command == 'E'
                            && pObj->level < LastMob->level - 5 && pObj->level < 45))
                        fprintf(stderr,
                            "Err: obj %s (%"PRVNUM") -- %d, mob %s (%"PRVNUM") -- %d\n",
                            pObj->short_descr, pObj->prototype->vnum,
                            pObj->level, LastMob->short_descr,
                            LastMob->prototype->vnum, LastMob->level);
                }
                else
                    break;
            }

            obj_to_char(pObj, LastMob);
            if (pReset->command == 'E')
                equip_char(LastMob, pObj, (int16_t)pReset->arg3);
            last = true;
            break;

        case 'D':
            break;

        case 'R':
            if (!(pRoomIndex = get_room_data(pReset->arg1))) {
                bug("Reset_room: 'R': bad vnum %"PRVNUM".", pReset->arg1);
                continue;
            }

            {
                ExitData* pExit;
                int d0;
                int d1;

                for (d0 = 0; d0 < pReset->arg2 - 1; d0++) {
                    d1 = number_range(d0, pReset->arg2 - 1);
                    pExit = pRoomIndex->exit[d0];
                    pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
                    pRoomIndex->exit[d1] = pExit;
                }
            }
            break;
        }
    }

    return;
}

/* OLC
 * Reset one area.
 */
void reset_area(AreaData* pArea)
{
    RoomData* pRoom;
    VNUM  vnum;

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++) {
        if ((pRoom = get_room_data(vnum)))
            reset_room(pRoom);
    }

    return;
}

/* duplicate a mobile exactly -- except inventory */
void clone_mobile(CharData* parent, CharData* clone)
{
    int i;
    AffectData* paf;

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
    clone->act_flags = parent->act_flags;
    clone->comm_flags = parent->comm_flags;
    clone->imm_flags = parent->imm_flags;
    clone->res_flags = parent->res_flags;
    clone->vuln_flags = parent->vuln_flags;
    clone->invis_level = parent->invis_level;
    clone->affect_flags = parent->affect_flags;
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
    clone->atk_flags = parent->atk_flags;
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

MobProgCode* get_mprog_index(VNUM vnum)
{
    for (MobProgCode* prg = mprog_list; prg; prg = prg->next) {
        if (prg->vnum == vnum)
            return(prg);
    }
    return NULL;
}

/*
 * Clear a new character.
 */
void clear_char(CharData* ch)
{
    static CharData ch_zero;
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
 * Read a letter from a file.
 */
char fread_letter(FILE* fp)
{
    char c;

    do {
        c = (char)getc(fp);
    }
    while (ISSPACE(c));

    return c;
}

/*
 * Read a VNUM from a file.
 */
VNUM fread_vnum(FILE* fp)
{
    VNUM number;
    char c;

    do {
        c = (char)getc(fp);
    } while (ISSPACE(c));

    number = 0;

    if (!ISDIGIT(c)) {
        bug("Fread_number: bad format.", 0);
        exit(1);
    }

    while (ISDIGIT(c)) {
        number = number * 10 + c - '0';
        c = (char)getc(fp);
    }

    if (c != ' ')
        ungetc(c, fp);

    return number;
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
        c = (char)getc(fp);
    }
    while (ISSPACE(c));

    number = 0;

    sign = false;
    if (c == '+') { 
        c = (char)getc(fp); 
    }
    else if (c == '-') {
        sign = true;
        c = (char)getc(fp);
    }

    if (!ISDIGIT(c)) {
        bug("Fread_number: bad format.", 0);
        exit(1);
    }

    while (ISDIGIT(c)) {
        number = number * 10 + c - '0';
        c = (char)getc(fp);
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
        c = (char)getc(fp);
    }
    while (ISSPACE(c));

    if (c == '-') {
        negative = true;
        c = (char)getc(fp);
    }

    number = 0;

    if (!ISDIGIT(c)) {
        while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
            number += flag_convert(c);
            c = (char)getc(fp);
        }
    }

    while (ISDIGIT(c)) {
        number = number * 10 + c - '0';
        c = (char)getc(fp);
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
        c = (char)getc(fp);
    }
    while (ISSPACE(c));

    if ((*plast++ = c) == '~') return &str_empty[0];

    for (;;) {
        /*
         * Back off the char type lookup,
         *   it was too dirty for portability.
         *   -- Furey
         */

        switch (*plast = (char)getc(fp)) {
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
                    for (ic = 0; ic < (int)sizeof(char*); ic++)
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
                    for (ic = 0; ic < (int)sizeof(char*); ic++)
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
        c = (char)getc(fp);
    }
    while (ISSPACE(c));

    if ((*plast++ = c) == '\n')
        return &str_empty[0];

    for (;;) {
        if (!char_special[(*plast++ = (char)getc(fp)) - EOF]) 
            continue;

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
                for (ic = 0; ic < (int)sizeof(char*); ic++) u1.rgc[ic] = pHash[ic];
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
                for (ic = 0; ic < (int)sizeof(char*); ic++) 
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

/*
 * Read to end of line (for comments).
 */
void fread_to_eol(FILE* fp)
{
    char c;

    do {
        c = (char)getc(fp);
    }
    while (c != '\n' && c != '\r');

    do {
        c = (char)getc(fp);
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
        cEnd = (char)getc(fp);
    }
    while (ISSPACE(cEnd));

    if (cEnd == '\'' || cEnd == '"') { 
        pword = word; 
    }
    else {
        word[0] = cEnd;
        pword = word + 1;
        cEnd = ' ';
    }

    for (; pword < word + MAX_INPUT_LENGTH; pword++) {
        *pword = (char)getc(fp);
        if (cEnd == ' ' ? ISSPACE(*pword) : *pword == cEnd) {
            if (cEnd == ' ') 
                ungetc(*pword, fp);
            *pword = '\0';
            return word;
        }
    }

    bug("Fread_word: word too long.", 0);
    exit(1);
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

void do_areas(CharData* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    AreaData* pArea1;
    AreaData* pArea2;
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

void do_memory(CharData* ch, char* argument)
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
    sprintf(buf, "Mobs    %5d(%d new format)\n\r", top_mob_prototype, newmobs);
    send_to_char(buf, ch);
    sprintf(buf, "(in use)%5d\n\r", mobile_count);
    send_to_char(buf, ch);
    sprintf(buf, "Objs    %5d(%d new format)\n\r", top_object_prototype, newobjs);
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

void do_dump(CharData* ch, char* argument)
{
    int count, count2, num_pcs, aff_count;
    CharData* fch;
    MobPrototype* p_mob_proto;
    PlayerData* pc;
    ObjectData* obj;
    ObjectPrototype* obj_proto;
    RoomData* room = NULL;
    ExitData* exit = NULL;
    Descriptor* d;
    AffectData* af;
    FILE* fp;
    VNUM vnum;
    int nMatch = 0;

    /* open file */
    fclose(fpReserve);

    char dump_file[256];

    sprintf(dump_file, "%s%s", area_dir, TEMP_DIR "mem.dmp");
    fp = fopen(dump_file, "w");

    /* report use of data structures */

    num_pcs = 0;
    aff_count = 0;

    /* mobile prototypes */
    fprintf(fp, "MobProt    %4d (%8zu bytes)\n", top_mob_prototype,
            top_mob_prototype * (sizeof(*p_mob_proto)));

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
    for (pc = player_free; pc != NULL; pc = pc->next) count++;
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
    for (vnum = 0; nMatch < top_object_prototype; vnum++)
        if ((obj_proto = get_object_prototype(vnum)) != NULL) {
            for (af = obj_proto->affected; af != NULL; af = af->next)
                aff_count++;
            nMatch++;
        }

    fprintf(fp, "ObjProt	%4d (%8zu bytes)\n", top_object_prototype,
            top_object_prototype * (sizeof(*obj_proto)));

    /* objects */
    count = 0;
    count2 = 0;
    for (obj = object_list; obj != NULL; obj = obj->next) {
        count++;
        for (af = obj->affected; af != NULL; af = af->next) aff_count++;
    }
    for (obj = object_free; obj != NULL; obj = obj->next) count2++;

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
    sprintf(dump_file, "%s%s", area_dir, TEMP_DIR "mob.dmp");
    fp = fopen(dump_file, "w");

    fprintf(fp, "\nMobile Analysis\n");
    fprintf(fp, "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_mob_prototype; vnum++)
        if ((p_mob_proto = get_mob_prototype(vnum)) != NULL) {
            nMatch++;
            fprintf(fp, "#%-4d %3d active %3d killed     %s\n", p_mob_proto->vnum,
                    p_mob_proto->count, p_mob_proto->killed,
                    p_mob_proto->short_descr);
        }
    fclose(fp);

    /* start printing out object data */
    sprintf(dump_file, "%s%s", area_dir, TEMP_DIR "obj.dmp");

    fprintf(fp, "\nObject Analysis\n");
    fprintf(fp, "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_object_prototype; vnum++)
        if ((obj_proto = get_object_prototype(vnum)) != NULL) {
            nMatch++;
            fprintf(fp, "#%-4d %3d active %3d reset      %s\n", obj_proto->vnum,
                    obj_proto->count, obj_proto->reset_num,
                    obj_proto->short_descr);
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
Direction number_door()
{
    Direction door;

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
    size_t ichar;
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
void append_file(CharData* ch, char* file, char* str)
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

