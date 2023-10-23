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
#include "config.h"
#include "handler.h"
#include "lookup.h"
#include "magic.h"
#include "music.h"
#include "note.h"
#include "pcg_basic.h"
#include "recycle.h"
#include "skills.h"
#include "special.h"
#include "stringutils.h"
#include "tables.h"
#include "weather.h"

#include "olc/olc.h"

#include "entities/area.h"
#include "entities/descriptor.h"
#include "entities/room_exit.h"
#include "entities/mob_prototype.h"
#include "entities/object.h"
#include "entities/player_data.h"
#include "entities/reset.h"
#include "entities/room.h"
#include "entities/shop_data.h"

#include "data/class.h"
#include "data/direction.h"
#include "data/mobile_data.h"
#include "data/race.h"
#include "data/skill.h"
#include "data/social.h"

#include "mth/mth.h"

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

// Globals.

MobProgCode* mprog_list;

char bug_buf[2 * MAX_INPUT_LENGTH];
char* help_greeting;
char log_buf[2 * MAX_INPUT_LENGTH];
KillData kill_table[MAX_LEVEL];

#define GSN(x) SKNUM x;
#include "gsn.h"
#undef GSN

// Locals.
char* string_hash[MAX_KEY_HASH];

char* string_space;
char* top_string;
char str_empty[1];

int extra_desc_count;
int	top_mprog_index;    // OLC

/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
//#define MAX_STRING     1413120
#define MAX_STRING      2119680
#define MAX_PERM_BLOCK  131072
#define MAX_MEM_LIST    15

void* rgFreeList[MAX_MEM_LIST];
const size_t rgSizeList[MAX_MEM_LIST] = {
    16, 32, 64, 128, 256, 1024, 2048, 4096,
    MAX_STRING_LENGTH,          // vvv
    8192,
    MAX_STRING_LENGTH * 2,      // Doesn't follow pattern, but frequently used.
    16384,
    MAX_STRING_LENGTH * 4,      // ^^^
    32768 - 64,
    65536,
};

int nAllocString;
size_t sAllocString;
int nAllocPerm;
size_t sAllocPerm;

// Semi-locals.
bool fBootDb;
FILE* strArea;
char fpArea[MAX_INPUT_LENGTH];
Area* current_area;

// Local booting procedures.
void init_mm();
void load_area(FILE* fp);
void load_helps(FILE* fp, char* fname);
void load_resets(FILE* fp);
void load_rooms(FILE* fp);
void load_shops(FILE* fp);
void load_specials(FILE* fp);
void load_notes();
void load_mobprogs(FILE* fp);

void fix_exits();
void fix_mobprogs();

void reset_area(Area * area);

// Big mama top level function.
void boot_db()
{
    // Init some data space stuff.
    {
        if ((string_space = calloc(1, MAX_STRING)) == NULL) {
            bug("Boot_db: can't alloc %d string space.", MAX_STRING);
            exit(1);
        }
        top_string = string_space;
        fBootDb = true;
    }

    // Init random number generator.
    {
        init_mm();
    }

    load_skill_table();
    load_class_table();
    load_race_table();
    load_command_table();
    load_social_table();
    load_skill_group_table();

    // I uncomment these as I transmogrify file formats.
    //save_class_table();
    //save_race_table();
    //save_skill_group_table();
    //save_skill_table();

    // Set time and weather.
    init_weather_info();

    // Assign gsn's for skills which have them.
    {
        SKNUM sn;

        for (sn = 0; sn < skill_count; sn++) {
            if (skill_table[sn].pgsn != NULL) 
                *skill_table[sn].pgsn = sn;
        }
    }

    // Read in all the area files.
    {
        FILE* fpList;
        char area_file[256];

        OPEN_OR_DIE(fpList = open_read_area_list());

        current_area = NULL;

        for (;;) {
            strcpy(fpArea, fread_word(fpList));
            if (fpArea[0] == '$')
                break;

            sprintf(area_file, "%s%s", cfg_get_area_dir(), fpArea);
            OPEN_OR_DIE(strArea = open_read_file(area_file));

            for (;;) {
                char* word;

                if (fread_letter(strArea) != '#') {
                    bug("Boot_db: # not found.", 0);
                    exit(1);
                }

                word = fread_word(strArea);

                if (word[0] == '$')
                    break;
                else if (!str_cmp(word, "AREADATA"))
                    load_area(strArea);
                else if (!str_cmp(word, "HELPS"))
                    load_helps(strArea, fpArea);
                else if (!str_cmp(word, "MOBILES"))
                    load_mobiles(strArea);
                else if (!str_cmp(word, "MOBPROGS"))
                    load_mobprogs(strArea);
                else if (!str_cmp(word, "OBJECTS"))
                    load_objects(strArea);
                else if (!str_cmp(word, "RESETS"))
                    load_resets(strArea);
                else if (!str_cmp(word, "ROOMS"))
                    load_rooms(strArea);
                else if (!str_cmp(word, "SHOPS"))
                    load_shops(strArea);
                else if (!str_cmp(word, "SPECIALS"))
                    load_specials(strArea);
                else if (!str_cmp(word, "QUEST"))
                    load_quest(strArea);
                else {
                    bug("Boot_db: bad section name.", 0);
                    exit(1);
                }
            }

            close_file(strArea);
            strArea = NULL;
        }
        close_file(fpList);
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
        area_update();
        load_notes();
        load_bans();
        load_songs();
    }

    init_mth();

    return;
}

/*
 * OLC
 * Use these macros to load any new area formats that you choose to
 * support on your MUD.  See the load_area format below for
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
void load_area(FILE* fp)
{
    Area* area;
    char* word;
    int version = 1;

    area = alloc_perm(sizeof(*area));
    area->reset_thresh = 6;
    area->nplayer = 0;
    area->file_name = str_dup(fpArea);
    area->vnum = area_count;
    area->name = str_dup("New Area");
    area->builders = str_dup("");
    area->security = 9;
    area->min_vnum = 0;
    area->max_vnum = 0;
    area->area_flags = 0;
    area->reset_timer = 0;
    area->always_reset = false;

    for (; ; ) {
        word = feof(fp) ? "End" : fread_word(fp);

        switch (UPPER(word[0])) {
        case 'A':
            KEY("AlwaysReset", area->always_reset, (bool)fread_number(fp));
        case 'B':
            SKEY("Builders", area->builders);
            break;
        case 'C':
            SKEY("Credits", area->credits);
            break;
        case 'E':
            if (!str_cmp(word, "End")) {
                if (area_first == NULL)
                    area_first = area;
                if (area_last != NULL)
                    area_last->next = area;
                area->reset_timer = area->reset_thresh;
                area_last = area;
                area->next = NULL;
                current_area = area;
                area_count++;
                return;
            }
            break;
        case 'H':
            KEY("High", area->high_range, (LEVEL)fread_number(fp));
            break;
        case 'L':
            KEY("Low", area->low_range, (LEVEL)fread_number(fp));
            break;
        case 'N':
            SKEY("Name", area->name);
            break;
        case 'R':
            KEY("Reset", area->reset_thresh, (int16_t)fread_number(fp));
            break;
        case 'S':
            KEY("Security", area->security, fread_number(fp));
            KEY("Sector", area->sector, fread_number(fp));
            break;
        case 'V':
            if (!str_cmp(word, "VNUMs")) {
                area->min_vnum = fread_number(fp);
                area->max_vnum = fread_number(fp);
                break;
            }
            KEY("Version", version, fread_number(fp));
            break;
        // End switch
        }
    }
}

// Sets vnum range for area using OLC protection features.
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

// Snarf a help section.
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

        help_count++;
    }

    return;
}

// Snarf a reset section.
void load_resets(FILE* fp)
{
    Reset* reset;
    RoomExit* room_exit;
    Room* room;
    VNUM vnum = VNUM_NONE;

    if (area_last == NULL) {
        bug("Load_resets: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        char letter;

        if ((letter = fread_letter(fp)) == 'S') 
            break;

        if (letter == '*') {
            fread_to_eol(fp);
            continue;
        }

        reset = new_reset();
        reset->command = letter;
        /* if_flag */ fread_number(fp);
        reset->arg1 = fread_vnum(fp);
        reset->arg2 = (int16_t)fread_number(fp);
        reset->arg3 = (letter == 'G' || letter == 'R') ? 0 : fread_vnum(fp);
        reset->arg4 = (letter == 'P' || letter == 'M') ? (int16_t)fread_number(fp) : 0;
        fread_to_eol(fp);

        switch (reset->command) {
        case 'M':
        case 'O':
            vnum = reset->arg3;
            break;

        case 'P':
        case 'G':
        case 'E':
            break;

        case 'D':
            room = get_room((vnum = reset->arg1));
            if (reset->arg2 < 0
                || reset->arg2 >= DIR_MAX
                || !room
                || !(room_exit = room->exit[reset->arg2])
                || !IS_SET(room_exit->exit_reset_flags, EX_ISDOOR)) {
                bugf("Load_resets: 'D': exit %d, room %"PRVNUM" not door.", reset->arg2, reset->arg1);
                exit(1);
            }

            switch (reset->arg3) {
            default: bug("Load_resets: 'D': bad 'locks': %d.", reset->arg3); break;
            case 0: 
                break;
            case 1: 
                SET_BIT(room_exit->exit_reset_flags, EX_CLOSED);
                SET_BIT(room_exit->exit_flags, EX_CLOSED); 
                break;
            case 2: 
                SET_BIT(room_exit->exit_reset_flags, EX_CLOSED | EX_LOCKED);
                SET_BIT(room_exit->exit_flags, EX_CLOSED | EX_LOCKED); 
                break;
            }
            break;

        case 'R':
            vnum = reset->arg1;
            break;
        }

        if (vnum == VNUM_NONE) {
            bugf("load_resets : vnum == VNUM_NONE");
            exit(1);
        }

        if (reset->command != 'D')
            add_reset(get_room(vnum), reset, -1);
        else
            free_reset(reset);
    }

    return;
}

// Snarf a room section.
void load_rooms(FILE* fp)
{
    Room* room;

    if (area_last == NULL) {
        bug("Load_resets: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        VNUM vnum;
        char letter;
        int door;
        int hash;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_rooms: # not found.", 0);
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0) break;

        fBootDb = false;
        if (get_room(vnum) != NULL) {
            bug("Load_rooms: vnum %"PRVNUM" duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        room = new_room();
        room->owner = str_dup("");
        room->area = area_last;
        room->vnum = vnum;
        room->name = fread_string(fp);
        room->description = fread_string(fp);
        /* Area number */ fread_number(fp);
        room->room_flags = fread_flag(fp);
        /* horrible hack */
        if (3000 <= vnum && vnum < 3400)
            SET_BIT(room->room_flags, ROOM_LAW);
        room->sector_type = (int16_t)fread_number(fp);

        /* defaults */
        room->heal_rate = 100;
        room->mana_rate = 100;

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'S') break;

            if (letter == 'H') /* healing room */
                room->heal_rate = (int16_t)fread_number(fp);

            else if (letter == 'M') /* mana room */
                room->mana_rate = (int16_t)fread_number(fp);

            else if (letter == 'C') /* clan */
            {
                if (room->clan) {
                    bug("Load_rooms: duplicate clan fields.", 0);
                    exit(1);
                }
                room->clan = (int16_t)clan_lookup(fread_string(fp));
            }

            else if (letter == 'D') {
                int locks;

                door = fread_number(fp);
                if (door < 0 || door >= DIR_MAX) {
                    bug("Fread_rooms: vnum %"PRVNUM" has bad door number.", vnum);
                    exit(1);
                }

                RoomExit* room_exit = new_room_exit();
                room_exit->description = fread_string(fp);
                room_exit->keyword = fread_string(fp);
                locks = fread_number(fp);
                room_exit->key = (int16_t)fread_number(fp);
                room_exit->to_vnum = fread_number(fp);
                room_exit->orig_dir = door;

                switch (locks) {
                case 1: 
                    room_exit->exit_flags = EX_ISDOOR;
                    room_exit->exit_reset_flags = EX_ISDOOR;
                    break;
                case 2: 
                    room_exit->exit_flags = EX_ISDOOR | EX_PICKPROOF;
                    room_exit->exit_reset_flags = EX_ISDOOR | EX_PICKPROOF;
                    break;
                case 3: 
                    room_exit->exit_flags = EX_ISDOOR | EX_NOPASS;
                    room_exit->exit_reset_flags = EX_ISDOOR | EX_NOPASS;
                    break;
                case 4: 
                    room_exit->exit_flags = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
                    room_exit->exit_reset_flags = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
                    break;
                }

                room->exit[door] = room_exit;
                room_exit_count++;
            }
            else if (letter == 'E') {
                ExtraDesc* ed;
                ed = new_extra_desc();
                if (ed == NULL)
                    exit(1);
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ADD_EXTRA_DESC(room, ed)
                extra_desc_count++;
            }

            else if (letter == 'O') {
                if (room->owner[0] != '\0') {
                    bug("Load_rooms: duplicate owner.", 0);
                    exit(1);
                }

                room->owner = fread_string(fp);
            }

            else {
                bug("Load_rooms: vnum %"PRVNUM" has flag not 'DES'.", vnum);
                exit(1);
            }
        }

        hash = vnum % MAX_KEY_HASH;
        room->next = room_vnum_hash[hash];
        room_vnum_hash[hash] = room;
        room_count++;
        top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room;    // OLC
        assign_area_vnum(vnum);                                         // OLC
    }

    return;
}

// Snarf a shop section.
void load_shops(FILE* fp)
{
    ShopData* shop;

    for (;;) {
        MobPrototype* mob_proto;
        int iTrade;

        if ((shop = new_shop_data()) == NULL) {
            bug("load_shops: Failed to create shops.");
            exit(1);
        }

        shop->keeper = (int16_t)fread_number(fp);
        if (shop->keeper == 0)
            break;
        for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
            shop->buy_type[iTrade] = (int16_t)fread_number(fp);
        shop->profit_buy = (int16_t)fread_number(fp);
        shop->profit_sell = (int16_t)fread_number(fp);
        shop->open_hour = (int16_t)fread_number(fp);
        shop->close_hour = (int16_t)fread_number(fp);
        fread_to_eol(fp);
        mob_proto = get_mob_prototype(shop->keeper);
        mob_proto->pShop = shop;

        if (shop_first == NULL)
            shop_first = shop;
        if (shop_last != NULL) 
            shop_last->next = shop;
        shop_last = shop;
        if (shop)
            shop->next = NULL;
        shop_count++;
    }
}

// Snarf spec proc declarations.
void load_specials(FILE* fp)
{
    for (;;) {
        MobPrototype* mob_proto;
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
            mob_proto = get_mob_prototype(fread_number(fp));
            mob_proto->spec_fun = spec_lookup(fread_word(fp));
            if (mob_proto->spec_fun == 0) {
                bug("Load_specials: 'M': vnum %"PRVNUM".", mob_proto->vnum);
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
void fix_exits()
{
    char buf[MAX_STRING_LENGTH];
    Room* room;
    Room* to_room;
    RoomExit* room_exit;
    RoomExit* room_exit_rev;
    Reset* reset;
    Room* last_room_index;
    Room* last_obj_index;
    int hash;
    int door;

    for (hash = 0; hash < MAX_KEY_HASH; hash++) {
        for (room = room_vnum_hash[hash]; room != NULL; NEXT_LINK(room)) {
            bool fexit;

            last_room_index = last_obj_index = NULL;

            /* OLC : New reset check */
            FOR_EACH(reset, room->reset_first) {
                switch (reset->command) {
                default:
                    bugf("fix_exits : room %d with reset cmd %c", 
                        room->vnum, reset->command);
                    exit(1);
                    break;

                case 'M':
                    get_mob_prototype(reset->arg1);
                    last_room_index = get_room(reset->arg3);
                    break;

                case 'O':
                    get_object_prototype(reset->arg1);
                    last_obj_index = get_room(reset->arg3);
                    break;

                case 'P':
                    get_object_prototype(reset->arg1);
                    if (last_obj_index == NULL) {
                        bugf("fix_exits : reset in room %d with last_obj_index NULL", 
                            room->vnum);
                        exit(1);
                    }
                    break;

                case 'G':
                case 'E':
                    get_object_prototype(reset->arg1);
                    if (last_room_index == NULL) {
                        bugf("fix_exits : reset in room %d with last_room_index NULL", 
                            room->vnum);
                        exit(1);
                    }
                    last_obj_index = last_room_index;
                    break;

                case 'D':
                    bugf("???");
                    break;

                case 'R':
                    get_room(reset->arg1);
                    if (reset->arg2 < 0 || reset->arg2 > DIR_MAX) {
                        bugf("fix_exits : reset in room %d with arg2 %d >= DIR_MAX",
                            room->vnum, reset->arg2);
                        exit(1);
                    }
                    break;
                }
            }

            fexit = false;
            for (door = 0; door < DIR_MAX; door++) {
                if ((room_exit = room->exit[door]) != NULL) {
                    if (room_exit->to_vnum <= 0
                        || get_room(room_exit->to_vnum) == NULL)
                        room_exit->to_room = NULL;
                    else {
                        fexit = true;
                        room_exit->to_room = get_room(room_exit->to_vnum);
                    }
                }
            }
// Removed by Halivar
//            if (!fexit)
//                SET_BIT(pRoomIndex->room_flags, ROOM_NO_MOB);
        }
    }

    for (hash = 0; hash < MAX_KEY_HASH; hash++) {
        for (room = room_vnum_hash[hash]; room != NULL; NEXT_LINK(room)) {
            for (door = 0; door <= 5; door++) {
                if ((room_exit = room->exit[door]) != NULL
                    && (to_room = room_exit->to_room) != NULL
                    && (room_exit_rev = to_room->exit[dir_list[door].rev_dir]) != NULL
                    && room_exit_rev->to_room != room
                    && (room->vnum < 1200 || room->vnum > 1299)) {
                    sprintf(buf, "Fix_exits: %d:%d(%s) -> %d:%d(%s) -> %d.",
                        room->vnum, door, dir_list[door].name_abbr, to_room->vnum,
                            dir_list[door].rev_dir, dir_list[dir_list[door].rev_dir].name_abbr,
                            (room_exit_rev->to_room == NULL)
                                ? 0
                                : room_exit_rev->to_room->vnum);
                    bug(buf, 0);
                }
            }
        }
    }

    return;
}

// Repopulate areas periodically.
void area_update()
{
    Area* area;
    char buf[MAX_STRING_LENGTH];

    FOR_EACH(area, area_first) {
        int thresh = area->reset_thresh;

        if (area->nplayer == 0)
            thresh /= 2;

        ++area->reset_timer;

        if (area->reset_timer >= thresh) {
            reset_area(area);
            sprintf(buf, "%s has just been reset.", area->name);
            wiznet(buf, NULL, NULL, WIZ_RESETS, 0, 0);
            area->reset_timer = 0;
        }
    }

    return;
}

// Load mobprogs section
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

//  Translate mobprog vnums pointers to real code
void fix_mobprogs(void)
{
    MobPrototype* p_mob_proto;
    MobProg* list;
    MobProgCode* prog;
    int hash;

    for (hash = 0; hash < MAX_KEY_HASH; hash++) {
        FOR_EACH(p_mob_proto, mob_prototype_hash[hash]) {
            FOR_EACH(list, p_mob_proto->mprogs) {
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
void reset_room(Room* pRoom)
{
    Reset* reset;
    Mobile* pMob = NULL;
    Object* pObj;
    Mobile* LastMob = NULL;
    Object* LastObj = NULL;
    int iExit;
    int level = 0;
    bool last;

    if (!pRoom)
        return;

    last = false;

    for (iExit = 0; iExit < DIR_MAX; iExit++) {
        RoomExit* room_exit;
        if ((room_exit = pRoom->exit[iExit])) {
            room_exit->exit_flags = room_exit->exit_reset_flags;
            if ((room_exit->to_room != NULL)
                && ((room_exit = room_exit->to_room->exit[dir_list[iExit].rev_dir]))) {
                  /* nail the other side */
                room_exit->exit_flags = room_exit->exit_reset_flags;
            }
        }
    }

    FOR_EACH(reset, pRoom->reset_first) {
        MobPrototype* pMobIndex;
        ObjPrototype* pObjIndex;
        ObjPrototype* pObjToIndex;
        Room* pRoomIndex;
        char buf[MAX_STRING_LENGTH];
        int count, limit = 0;

        switch (reset->command) {
        default:
            bug("Reset_room: bad command %c.", reset->command);
            break;

        case 'M':
        {
            Mobile* mob;

            if ((pMobIndex = get_mob_prototype(reset->arg1)) == NULL) {
                bug("Reset_room: 'M': bad vnum %"PRVNUM".", reset->arg1);
                continue;
            }

            if ((pRoomIndex = get_room(reset->arg3)) == NULL) {
                bug("Reset_area: 'R': bad vnum %"PRVNUM".", reset->arg3);
                continue;
            }

            if (pMobIndex->count >= reset->arg2) {
                last = false;
                break;
            }

            /* */

            count = 0;
            for (mob = pRoomIndex->people; mob != NULL; mob = mob->next_in_room)
                if (mob->prototype == pMobIndex) {
                    count++;
                    if (count >= reset->arg4) {
                        last = false;
                        break;
                    }
                }

            if (count >= reset->arg4)
                break;

            /* */

            pMob = create_mobile(pMobIndex);

            // Some more hard coding.
            if (room_is_dark(pRoom))
                SET_BIT(pMob->affect_flags, AFF_INFRARED);

            // Pet shop mobiles get ACT_PET set.
            {
                Room* pRoomIndexPrev;

                pRoomIndexPrev = get_room(pRoom->vnum - 1);
                if (pRoomIndexPrev
                    && IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
                    SET_BIT(pMob->act_flags, ACT_PET);
            }

            char_to_room(pMob, pRoom);
            LastMob = pMob;
            level = URANGE(0, pMob->level - 2, LEVEL_HERO - 1);
            last = true;
            break;
        }

        case 'O':
            if (!(pObjIndex = get_object_prototype(reset->arg1))) {
                bug("Reset_room: 'O' 1 : bad vnum %"PRVNUM"", reset->arg1);
                sprintf(buf, "%"PRVNUM" %d %"PRVNUM" %d", reset->arg1, reset->arg2, reset->arg3,
                    reset->arg4);
                bug(buf, 1);
                continue;
            }

            if (!(pRoomIndex = get_room(reset->arg3))) {
                bug("Reset_room: 'O' 2 : bad vnum %"PRVNUM".", reset->arg3);
                sprintf(buf, "%"PRVNUM" %d %"PRVNUM" %d", reset->arg1, reset->arg2, reset->arg3,
                    reset->arg4);
                bug(buf, 1);
                continue;
            }

            if ((pRoom->area->nplayer > 0 && !pRoom->area->always_reset)
                || count_obj_list(pObjIndex, pRoom->contents) > 0) {
                last = false;
                break;
            }

            pObj = create_object(pObjIndex, (LEVEL)UMIN(number_fuzzy(level),
                LEVEL_HERO - 1)); /* UMIN - ROM OLC */
            pObj->cost = 0;
            obj_to_room(pObj, pRoom);
            last = true;
            break;

        case 'P':
            if ((pObjIndex = get_object_prototype(reset->arg1)) == NULL) {
                bug("Reset_room: 'P': bad vnum %"PRVNUM".", reset->arg1);
                continue;
            }
            if ((pObjToIndex = get_object_prototype(reset->arg3)) == NULL) {
                bug("Reset_room: 'P': bad vnum %"PRVNUM".", reset->arg3);
                continue;
            }

            if (reset->arg2 > 50) /* old format */
                limit = 6;
            else if (reset->arg2 <= 0) /* no limit */
                limit = 999;
            else
                limit = reset->arg2;

            count = 0;
            if ((pRoom->area->nplayer > 0 && !pRoom->area->always_reset)
                || (LastObj = get_obj_type(pObjToIndex)) == NULL
                || (LastObj->in_room == NULL && !last)
                || (pObjIndex->count >= limit /* && number_range(0,4) != 0 */)
                || (count = count_obj_list(pObjIndex, LastObj->contains))
                > reset->arg4) {
                last = false;
                break;
            }

            /* lastObj->level  -  ROM */
            while (count < reset->arg4) {
                pObj = create_object(pObjIndex, (LEVEL)number_fuzzy(LastObj->level));
                obj_to_obj(pObj, LastObj);
                count++;
                if (pObjIndex->count >= limit) 
                    break;
            }

            /* fix object lock state! */
            LastObj->value[1] = LastObj->prototype->value[1];
            last = true;
            break;

        case 'G':
        case 'E':
            if ((pObjIndex = get_object_prototype(reset->arg1)) == NULL) {
                bug("Reset_room: 'E' or 'G': bad vnum %"PRVNUM".", reset->arg1);
                continue;
            }

            if (!last)
                break;

            if (!LastMob) {
                bug("Reset_room: 'E' or 'G': null mob for vnum %"PRVNUM".",
                    reset->arg1);
                last = false;
                break;
            }

            if (LastMob->prototype->pShop != NULL) { // Shopkeeper?
                int olevel = 0;
                pObj = create_object(pObjIndex, (LEVEL)olevel);
                SET_BIT(pObj->extra_flags, ITEM_INVENTORY);
            }
            else {
                // ROM OLC
                if (reset->arg2 > 50) /* old format */
                    limit = 6;
                else if (reset->arg2 == -1 || reset->arg2 == 0) /* no limit */
                    limit = 999;
                else
                    limit = reset->arg2;

                if (pObjIndex->count < limit || number_range(0, 4) == 0) {
                    pObj = create_object(
                        pObjIndex, (LEVEL)UMIN(number_fuzzy(level), LEVEL_HERO - 1));
                    /* error message if it is too high */
                    if (pObj->level > LastMob->level + 3
                        || (pObj->item_type == ITEM_WEAPON
                            && reset->command == 'E'
                            && pObj->level < LastMob->level - 5 && pObj->level < 45))
                        fprintf(stderr,
                            "Err: obj %s [%"PRVNUM"] Lvl %d -- mob %s [%"PRVNUM"] Lvl %d\n",
                            pObj->short_descr, pObj->prototype->vnum,
                            pObj->level, LastMob->short_descr,
                            LastMob->prototype->vnum, LastMob->level);
                }
                else
                    break;
            }

            obj_to_char(pObj, LastMob);
            if (reset->command == 'E')
                equip_char(LastMob, pObj, reset->arg3);
            last = true;
            break;

        case 'D':
            break;

        case 'R':
            if (!(pRoomIndex = get_room(reset->arg1))) {
                bug("Reset_room: 'R': bad vnum %"PRVNUM".", reset->arg1);
                continue;
            }

            {
                RoomExit* room_exit;
                int d0;
                int d1;

                for (d0 = 0; d0 < reset->arg2 - 1; d0++) {
                    d1 = number_range(d0, reset->arg2 - 1);
                    room_exit = pRoomIndex->exit[d0];
                    pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
                    pRoomIndex->exit[d1] = room_exit;
                }
            }
            break;
        }
    }

    return;
}

void reset_area(Area* area)
{
    Room* pRoom;
    VNUM  vnum;

    for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
        if ((pRoom = get_room(vnum)))
            reset_room(pRoom);
    }

    return;
}

// Duplicate a mobile exactly -- except inventory
void clone_mobile(Mobile* parent, Mobile* clone)
{
    int i;
    Affect* affect;

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

    for (i = 0; i < AC_COUNT; i++) 
        clone->armor[i] = parent->armor[i];

    for (i = 0; i < STAT_COUNT; i++) {
        clone->perm_stat[i] = parent->perm_stat[i];
        clone->mod_stat[i] = parent->mod_stat[i];
    }

    for (i = 0; i < 3; i++) 
        clone->damage[i] = parent->damage[i];

    /* now add the affects */
    FOR_EACH(affect, parent->affected)
        affect_to_char(clone, affect);
}

MobProgCode* get_mprog_index(VNUM vnum)
{
    for (MobProgCode* prg = mprog_list; prg; NEXT_LINK(prg)) {
        if (prg->vnum == vnum)
            return(prg);
    }
    return NULL;
}

// Clear a new character.
void clear_char(Mobile* ch)
{
    static Mobile ch_zero;
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
    for (i = 0; i < STAT_COUNT; i++) {
        ch->perm_stat[i] = 13;
        ch->mod_stat[i] = 0;
    }
    return;
}

// Read a letter from a file.
char fread_letter(FILE* fp)
{
    char c;

    do {
        c = (char)getc(fp);
    }
    while (ISSPACE(c));

    return c;
}

// Read a VNUM from a file.
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

// Read a number from a file.
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

    if ((*plast++ = c) == '~') 
        return &str_empty[0];

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
                int hash;
                char* pHash;
                char* pHashPrev = NULL;
                char* pString;

                plast[-1] = '\0';
                hash = (int)UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
                for (pHash = string_hash[hash]; pHash; pHash = pHashPrev) {
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
                    u1.pc = string_hash[hash];
                    for (ic = 0; ic < (int)sizeof(char*); ic++)
                        pString[ic] = u1.rgc[ic];
                    string_hash[hash] = pString;

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
            int hash;
            char* pHash;
            char* pHashPrev = NULL;
            char* pString;

            plast[-1] = '\0';
            hash = (int)UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
            for (pHash = string_hash[hash]; pHash; pHash = pHashPrev) {
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
                u1.pc = string_hash[hash];
                for (ic = 0; ic < (int)sizeof(char*); ic++) 
                    pString[ic] = u1.rgc[ic];
                string_hash[hash] = pString;

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

// Read to end of line (for comments).
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

// Read one word (into static buffer).
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

    while (sMem % sizeof(long) != 0) 
        sMem++;

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

void do_areas(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    Area* pArea1;
    Area* pArea2;
    int iArea;
    int iAreaHalf;

    if (argument[0] != '\0') {
        send_to_char("No argument is used with this command.\n\r", ch);
        return;
    }

    iAreaHalf = (area_count + 1) / 2;
    pArea1 = area_first;
    pArea2 = area_first;
    for (iArea = 0; iArea < iAreaHalf; iArea++) NEXT_LINK(pArea2);

    for (iArea = 0; iArea < iAreaHalf; iArea++) {
        sprintf(buf, "%-39s%-39s\n\r", pArea1->credits,
                (pArea2 != NULL) ? pArea2->credits : "");
        send_to_char(buf, ch);
        NEXT_LINK(pArea1);
        if (pArea2 != NULL) 
            NEXT_LINK(pArea2);
    }

    return;
}

static void memory_to_buffer(Buffer* buf)
{
    addf_buf(buf, "Affects %5d\n\r", affect_count);
    addf_buf(buf, "Areas   %5d\n\r", area_count);
    addf_buf(buf, "ExDes   %5d\n\r", extra_desc_count);
    addf_buf(buf, "Exits   %5d\n\r", room_exit_count);
    addf_buf(buf, "Helps   %5d\n\r", help_count);
    addf_buf(buf, "Socials %5d\n\r", social_count);
    addf_buf(buf, "Mobs    %5d     Protos  %5d\n\r", mob_count, mob_proto_count);
    addf_buf(buf, "Objs    %5d     Protos  %5d\n\r", obj_count, obj_proto_count);
    addf_buf(buf, "Resets  %5d\n\r", reset_count);
    addf_buf(buf, "Rooms   %5d\n\r", room_count);
    addf_buf(buf, "Shops   %5d\n\r", shop_count);

    addf_buf(buf, "Strings %5d strings of %zu bytes (max %d).\n\r", nAllocString,
        sAllocString, MAX_STRING);

    addf_buf(buf, "Perms   %5d blocks  of %zu bytes.\n\r", nAllocPerm,
        sAllocPerm);
}

void print_memory()
{
    INIT_BUF(buf, MSL);

    memory_to_buffer(buf);
    printf("%s", buf->string);

    free_buf(buf);
}

void do_memory(Mobile* ch, char* argument)
{
    INIT_BUF(buf, MSL);

    memory_to_buffer(buf);
    send_to_char(buf->string, ch);

    free_buf(buf);
}

void do_dump(Mobile* ch, char* argument)
{
    int count, count2, num_pcs, aff_count;
    Mobile* fch;
    MobPrototype* p_mob_proto;
    PlayerData* pc;
    Object* obj;
    ObjPrototype* obj_proto;
    Room* room = NULL;
    RoomExit* exit = NULL;
    Descriptor* d;
    Affect* af;
    FILE* fp;
    VNUM vnum;
    int nMatch = 0;

    OPEN_OR_RETURN(fp = open_write_mem_dump_file());

    /* report use of data structures */

    num_pcs = 0;
    aff_count = 0;

    /* mobile prototypes */
    fprintf(fp, "MobProt    %4d (%8zu bytes)\n", mob_proto_count,
            mob_proto_count * (sizeof(*p_mob_proto)));

    /* mobs */
    count = 0;
    count2 = 0;
    FOR_EACH(fch, mob_list) {
        count++;
        if (fch->pcdata != NULL) num_pcs++;
        FOR_EACH(af, fch->affected) 
            aff_count++;
    }
    FOR_EACH(fch, mob_free) 
        count2++;

    fprintf(fp, "Mobs   %4d (%8zu bytes), %2d free (%zu bytes)\n", count,
            count * (sizeof(*fch)), count2, count2 * (sizeof(*fch)));

    /* pcdata */
    count = 0;
    FOR_EACH(pc, player_free) 
        count++;
    fprintf(fp, "Pcdata	%4d (%8zu bytes), %2d free (%zu bytes)\n", num_pcs,
            num_pcs * (sizeof(*pc)), count, count * (sizeof(*pc)));

    /* descriptors */
    count = 0;
    count2 = 0;
    FOR_EACH(d, descriptor_list) 
        count++;
    FOR_EACH(d, descriptor_free) 
        count2++;

    fprintf(fp, "Descs	%4d (%8zu bytes), %2d free (%zu bytes)\n", count,
            count * (sizeof(*d)), count2, count2 * (sizeof(*d)));

    /* object prototypes */
    for (vnum = 0; nMatch < obj_proto_count; vnum++)
        if ((obj_proto = get_object_prototype(vnum)) != NULL) {
            FOR_EACH(af, obj_proto->affected)
                aff_count++;
            nMatch++;
        }

    fprintf(fp, "ObjProto   %4d (%8zu bytes)\n", obj_proto_count,
            obj_proto_count * (sizeof(*obj_proto)));

    /* objects */
    count = 0;
    count2 = 0;
    FOR_EACH(obj, obj_list) {
        count++;
        FOR_EACH(af, obj->affected) aff_count++;
    }
    FOR_EACH(obj, obj_free) count2++;

    fprintf(fp, "Objs	%4d (%8zu bytes), %2d free (%zu bytes)\n", count,
            count * (sizeof(*obj)), count2, count2 * (sizeof(*obj)));

    /* affects */
    count = 0;
    FOR_EACH(af, affect_free) count++;

    fprintf(fp, "Affects	%4d (%8zu bytes), %2d free (%zu bytes)\n", aff_count,
            aff_count * (sizeof(*af)), count, count * (sizeof(*af)));

    /* rooms */
    fprintf(fp, "Rooms	%4d (%8zu bytes)\n", room_count,
            room_count * (sizeof(*room)));

    /* exits */
    fprintf(fp, "Exits	%4d (%8zu bytes)\n", room_exit_count,
            room_exit_count * (sizeof(*exit)));

    
    close_file(fp);

    /* start printing out mobile data */

    OPEN_OR_RETURN(fp = open_write_mob_dump_file());

    fprintf(fp, "\nMobile Analysis\n");
    fprintf(fp, "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < mob_proto_count; vnum++)
        if ((p_mob_proto = get_mob_prototype(vnum)) != NULL) {
            nMatch++;
            fprintf(fp, "#%-4d %3d active %3d killed     %s\n", p_mob_proto->vnum,
                    p_mob_proto->count, p_mob_proto->killed,
                    p_mob_proto->short_descr);
        }

    close_file(fp);

    /* start printing out object data */
    OPEN_OR_RETURN(fp = open_write_obj_dump_file());

    fprintf(fp, "\nObject Analysis\n");
    fprintf(fp, "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < obj_proto_count; vnum++)
        if ((obj_proto = get_object_prototype(vnum)) != NULL) {
            nMatch++;
            fprintf(fp, "#%-4d %3d active %3d reset      %s\n", obj_proto->vnum,
                    obj_proto->count, obj_proto->reset_num,
                    obj_proto->short_descr);
        }

    /* close file */
    close_file(fp);
}

// Stick a little fuzz on a number.
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

// Generate a random number.
int number_range(int from, int to)
{
    int power;
    int number;

    if (from == 0 && to == 0) 
        return 0;

    if ((to = to - from + 1) <= 1) 
        return from;

    for (power = 2; power < to; power <<= 1)
        ;

    while ((number = number_mm() & (power - 1)) >= to)
        ;

    return from + number;
}

// Generate a percentile roll.
int number_percent(void)
{
    int percent;

    while ((percent = number_mm() & (128 - 1)) > 99)
        ;

    return 1 + percent;
}

// Generate a random door.
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

// Roll some dice.
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

// Simple linear interpolation.
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
        if (*str == '~') 
            *str = '-';
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

// Returns an initial-capped string.
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

// Append a string to a file.
void append_file(Mobile* ch, char* file, char* str)
{
    FILE* fp;

    if (IS_NPC(ch) || str[0] == '\0')
        return;

    OPEN_OR_RETURN(fp = open_append_file(file));

    fprintf(fp, "[%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name, str);

    close_file(fp);
}

// Reports a bug.
void bug(const char* fmt, ...)
{
    char buf[MAX_STRING_LENGTH];

    va_list args;
    va_start(args, fmt);

    if (strArea != NULL) {
        int iLine;
        int iChar;

        if (strArea == stdin) { 
            iLine = 0; 
        }
        else {
            iChar = ftell(strArea);
            fseek(strArea, 0, 0);
            for (iLine = 0; ftell(strArea) < iChar; iLine++) {
                while (getc(strArea) != '\n')
                    ;
            }
            fseek(strArea, iChar, 0);
        }

        sprintf(buf, "[*****] FILE: %s LINE: %d", fpArea, iLine);
        log_string(buf);
    }

    strcpy(buf, "[*****] BUG: ");
    vsprintf(buf + strlen(buf), fmt, args);
    log_string(buf);

    return;
}

// Writes a string to the log.
void log_string(const char* str)
{
    char* strtime;

    strtime = ctime(&current_time);
    strtime[strlen(strtime) - 1] = '\0';
    fprintf(stderr, "%s :: %s\n", strtime, str);
    return;
}

