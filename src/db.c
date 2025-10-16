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

#include <olc/olc.h>

#include <entities/area.h>
#include <entities/descriptor.h>
#include <entities/room_exit.h>
#include <entities/mob_prototype.h>
#include <entities/object.h>
#include <entities/player_data.h>
#include <entities/reset.h>
#include <entities/room.h>
#include <entities/shop_data.h>

#include <data/class.h>
#include <data/direction.h>
#include <data/events.h>
#include <data/mobile_data.h>
#include <data/race.h>
#include <data/skill.h>
#include <data/social.h>

#include <mth/mth.h>

#include <lox/lox.h>
#include <lox/native.h>
#include <lox/object.h>
#include <lox/memory.h>

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

void load_lox_public_scripts();
MobProgCode* pedit_prog(VNUM);

void load_lox_scripts(FILE* fp);
void load_event(FILE* fp, Entity* owner);

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

int	top_mprog_index;    // OLC

/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
//#define MAX_STRING     1413120
#define MAX_STRING      2119680
//#define MAX_PERM_BLOCK  131072
#define MAX_PERM_BLOCK  131072 * 2
#define MAX_MEM_LIST    17

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
    65536 * 2,
    65536 * 4,
};

int nAllocString;
size_t sAllocString;
int nAllocPerm;
size_t sAllocPerm;

// Semi-locals.
bool fBootDb;
FILE* strArea;
char fpArea[MAX_INPUT_LENGTH];
AreaData* current_area_data;

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
void load_lox_scripts(FILE* fp);

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

    init_value_array(&global_areas);
    init_table(&global_rooms);
    init_table(&mob_protos);
    init_table(&obj_protos);
    init_list(&mob_list);
    init_list(&obj_list);

    load_config();

    // Init random number generator.
    {
        init_mm();
    }

    init_vm();

    init_const_natives();
    load_lox_public_scripts();

    load_skill_table();
    load_class_table();
    load_race_table();
    load_command_table();

    // We need the commands before we can add them to Lox.
    init_native_cmds();

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

        current_area_data = NULL;

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
                else if (!str_cmp(word, "LOX"))
                    load_lox_scripts(strArea);
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
            // Only create single-instance areas.
            // All others are created on-demand.
            if (current_area_data 
                && current_area_data->inst_type == AREA_INST_SINGLE)
                create_area_instance(current_area_data, false);
        }
        close_file(fpList);
    }

    //// Run boot-time verification of Lox event bindings on prototypes.
    //verify_lox_event_bindings();
    //
    init_world_natives();

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

// Sets vnum range for area using OLC protection features.
void assign_area_vnum(VNUM vnum)
{
    AreaData* area_data_last = LAST_AREA_DATA;
    if (area_data_last->min_vnum == 0 || area_data_last->max_vnum == 0)
        area_data_last->min_vnum = area_data_last->max_vnum = vnum;
    if (vnum != URANGE(area_data_last->min_vnum, vnum, area_data_last->max_vnum)) {
        if (vnum < area_data_last->min_vnum)
            area_data_last->min_vnum = vnum;
        else
            area_data_last->max_vnum = vnum;
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
            had->area_data = current_area_data;
            if (current_area_data)
                current_area_data->helps = had;
            help_area_list = had;
        }
        else if (str_cmp(fname, help_area_list->filename)) {
            if ((had = new_help_area()) == NULL) {
                perror("load_helps (2): could not allocate new HelpArea!");
                exit(-1);
            }
            had->filename = str_dup(fname);
            had->area_data = current_area_data;
            if (current_area_data)
                current_area_data->helps = had;
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
    }
}

static void append_reset(RoomData* room_data, Reset* reset)
{
    Reset* pr;

    if (!room_data)
        return;

    pr = room_data->reset_last;

    if (!pr) {
        room_data->reset_first = reset;
        room_data->reset_last = reset;
    }
    else {
        room_data->reset_last->next = reset;
        room_data->reset_last = reset;
        room_data->reset_last->next = NULL;
    }

    // top_reset++; We aren't allocating memory!!!! 

    return;
}

// Snarf a reset section.
void load_resets(FILE* fp)
{
    Reset* reset;
    RoomExitData* room_exit_data;
    RoomData* room_data;
    VNUM vnum = VNUM_NONE;

    if (global_areas.count == 0) {
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
            room_data = get_room_data((vnum = reset->arg1));
            if (reset->arg2 < 0
                || reset->arg2 >= DIR_MAX
                || !room_data
                || !(room_exit_data = room_data->exit_data[reset->arg2])
                || !IS_SET(room_exit_data->exit_reset_flags, EX_ISDOOR)) {
                bugf("Load_resets: 'D': exit %d, room %"PRVNUM" not door.", reset->arg2, reset->arg1);
                exit(1);
            }

            switch (reset->arg3) {
            default: bug("Load_resets: 'D': bad 'locks': %d.", reset->arg3); break;
            case 0: 
                break;
            case 1: 
                SET_BIT(room_exit_data->exit_reset_flags, EX_CLOSED);
                break;
            case 2: 
                SET_BIT(room_exit_data->exit_reset_flags, EX_CLOSED | EX_LOCKED);
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
            append_reset(get_room_data(vnum), reset);
        else
            free_reset(reset);
    }

    return;
}

// Snarf a room section.
void load_rooms(FILE* fp)
{
    RoomData* room_data;

    if (global_areas.count == 0) {
        bug("Load_resets: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        VNUM vnum;
        char letter;
        int door;

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

        room_data = new_room_data();
        push(OBJ_VAL(room_data));
        room_data->owner = str_dup("");
        room_data->area_data = LAST_AREA_DATA;
        VNUM_FIELD(room_data) = vnum;
        SET_NAME(room_data, fread_lox_string(fp));
        room_data->description = fread_string(fp);
        /* Area number */ fread_number(fp);
        room_data->room_flags = fread_flag(fp);
        /* horrible hack */
        if (3000 <= vnum && vnum < 3400)
            SET_BIT(room_data->room_flags, ROOM_LAW);
        room_data->sector_type = (int16_t)fread_number(fp);

        /* defaults */
        room_data->heal_rate = 100;
        room_data->mana_rate = 100;

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'S') 
                break;

            if (letter == 'H') /* healing room */
                room_data->heal_rate = (int16_t)fread_number(fp);

            else if (letter == 'M') /* mana room */
                room_data->mana_rate = (int16_t)fread_number(fp);

            else if (letter == 'C') /* clan */
            {
                if (room_data->clan) {
                    bug("Load_rooms: duplicate clan fields.", 0);
                    exit(1);
                }
                room_data->clan = (int16_t)clan_lookup(fread_string(fp));
            }

            else if (letter == 'D') {
                int locks;

                door = fread_number(fp);
                if (door < 0 || door >= DIR_MAX) {
                    bug("Fread_rooms: vnum %"PRVNUM" has bad door number.", vnum);
                    exit(1);
                }

                RoomExitData* room_exit_data = new_room_exit_data();
                room_exit_data->description = fread_string(fp);
                room_exit_data->keyword = fread_string(fp);
                locks = fread_number(fp);
                room_exit_data->key = (int16_t)fread_number(fp);
                room_exit_data->to_vnum = fread_number(fp);
                room_exit_data->orig_dir = door;

                switch (locks) {
                case 1:
                    room_exit_data->exit_reset_flags = EX_ISDOOR;
                    break;
                case 2:
                    room_exit_data->exit_reset_flags = EX_ISDOOR | EX_PICKPROOF;
                    break;
                case 3:
                    room_exit_data->exit_reset_flags = EX_ISDOOR | EX_NOPASS;
                    break;
                case 4:
                    room_exit_data->exit_reset_flags = EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
                    break;
                }

                room_data->exit_data[door] = room_exit_data;
            }
            else if (letter == 'E') {
                ExtraDesc* ed;
                ed = new_extra_desc();
                if (ed == NULL)
                    exit(1);
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ADD_EXTRA_DESC(room_data, ed)
            }

            else if (letter == 'O') {
                if (room_data->owner[0] != '\0') {
                    bug("Load_rooms: duplicate owner.", 0);
                    exit(1);
                }

                room_data->owner = fread_string(fp);
            }

            else if (letter == 'V') {
                load_event(fp, &room_data->header);
            }

            else if (letter == 'L') {
                load_lox_class(fp, "room", &room_data->header);
            }
            else {
                bug("Load_rooms: vnum %"PRVNUM" has flag not 'DES'.", vnum);
                exit(1);
            }
        }

        table_set_vnum(&global_rooms, vnum, OBJ_VAL(room_data));
        top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room;
        assign_area_vnum(vnum);
        pop();
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
                bug("Load_specials: 'M': vnum %"PRVNUM".", VNUM_FIELD(mob_proto));
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
    RoomData* room_data;
    RoomData* to_room;
    RoomExitData* room_exit;
    RoomExitData* room_exit_rev;
    Reset* reset;
    RoomData* last_room_index;
    RoomData* last_obj_index;
    int door;

    FOR_EACH_GLOBAL_ROOM(room_data) {
        last_room_index = last_obj_index = NULL;

        /* OLC : New reset check */
        FOR_EACH(reset, room_data->reset_first) {
            switch (reset->command) {
            default:
                bugf("fix_exits : room %d with reset cmd %c", 
                    VNUM_FIELD(room_data), reset->command);
                exit(1);
                break;

            case 'M':
                get_mob_prototype(reset->arg1);
                last_room_index = get_room_data(reset->arg3);
                break;

            case 'O':
                get_object_prototype(reset->arg1);
                last_obj_index = get_room_data(reset->arg3);
                break;

            case 'P':
                get_object_prototype(reset->arg1);
                if (last_obj_index == NULL) {
                    bugf("fix_exits : reset in room %d with last_obj_index NULL", 
                        VNUM_FIELD(room_data));
                    exit(1);
                }
                break;

            case 'G':
            case 'E':
                get_object_prototype(reset->arg1);
                if (last_room_index == NULL) {
                    bugf("fix_exits : reset in room %d with last_room_index NULL", 
                        VNUM_FIELD(room_data));
                    exit(1);
                }
                last_obj_index = last_room_index;
                break;

            case 'D':
                bugf("???");
                break;

            case 'R':
                get_room_data(reset->arg1);
                if (reset->arg2 < 0 || reset->arg2 > DIR_MAX) {
                    bugf("fix_exits : reset in room %d with arg2 %d >= DIR_MAX",
                        VNUM_FIELD(room_data), reset->arg2);
                    exit(1);
                }
                break;
            }
        }

        for (door = 0; door < DIR_MAX; door++) {
            if ((room_exit = room_data->exit_data[door]) != NULL) {
                if (room_exit->to_vnum <= 0
                    || get_room_data(room_exit->to_vnum) == NULL)
                    room_exit->to_room = NULL;
                else {
                    room_exit->to_room = get_room_data(room_exit->to_vnum);
                }
            }
        }
    }

    FOR_EACH_GLOBAL_ROOM(room_data) {
        for (door = 0; door <= 5; door++) {
            if ((room_exit = room_data->exit_data[door]) != NULL
                && (to_room = room_exit->to_room) != NULL
                && (room_exit_rev = to_room->exit_data[dir_list[door].rev_dir]) != NULL
                && room_exit_rev->to_room != room_data
                && (VNUM_FIELD(room_data) < 1200 || VNUM_FIELD(room_data) > 1299)) {
                sprintf(buf, "Fix_exits: %d:%d(%s) -> %d:%d(%s) -> %d.",
                    VNUM_FIELD(room_data), door, dir_list[door].name_abbr, VNUM_FIELD(to_room),
                        dir_list[door].rev_dir, dir_list[dir_list[door].rev_dir].name_abbr,
                        (room_exit_rev->to_room == NULL)
                            ? 0 : VNUM_FIELD(room_exit_rev->to_room));
                bug(buf, 0);
            }
        }
    }

    AreaData* area_data;
    Area* area;
    FOR_EACH_AREA(area_data) {
        FOR_EACH_AREA_INST(area, area_data) {
            create_instance_exits(area);
        }
    }
}

// Repopulate areas periodically.
void area_update()
{
    AreaData* area_data;
    Area* area;
    char buf[MAX_STRING_LENGTH];

    FOR_EACH_AREA(area_data) {
        int thresh = area_data->reset_thresh;
        FOR_EACH_AREA_INST(area, area_data) {
            if (area->nplayer == 0)
                thresh /= 2;

            ++area->reset_timer;

            if (area->reset_timer >= thresh) {
                if (area->data->inst_type == AREA_INST_MULTI && area->nplayer == 0) {
                    // If the area is "instanced": delete, don't reset.
                    // TODO: See if we also need to manually clean up rooms
                    list_remove_node(&area_data->instances, area_loop.node);
                }
                else {
                    reset_area(area);
                    sprintf(buf, "%s has just been reset.", NAME_STR(area));
                    wiznet(buf, NULL, NULL, WIZ_RESETS, 0, 0);
                    area->reset_timer = 0;
                }
            }
        }
    }

    return;
}

// Load Lox section
void load_lox_scripts(FILE* fp)
{
//    char word[MAX_INPUT_LENGTH];
//
//    while (fscanf(fp, "%s", word) == 1) {
//        if (strcmp(word, "#END") == 0)
//            break;
//
//        // Consume a script name and body so the file pointer advances
//        String* name = fread_lox_string(fp);
//        char* body = fread_lox_script(fp);
//        if (body) free(body);
//        (void)name; // silence unused variable warning
//    }
//
//    return;
}

//
//// Verify that events referencing Lox functions on room prototypes actually
//// have a corresponding compiled function attached to the RoomData. This
//// helps catch typos in area files at boot time and warns the builder.
//void verify_lox_event_bindings()
//{
//    RoomData* rd;
//
//    FOR_EACH_GLOBAL_ROOM(rd) {
//        // Iterate events attached to the room prototype
//        for (Node* n = rd->header.events.front; n != NULL; n = n->next) {
//            Event* ev = (Event*)n->value;
//            if (!ev || !ev->func_name || ev->func_name == lox_empty_string)
//                continue;
//
//            bool found = false;
//
//            // Look through compiled ObjFunction array for a name match 
//            for (int i = 0; i < rd->lox_function_count; ++i) {
//                ObjFunction* fn = rd->lox_functions[i];
//                if (fn && fn->name && fn->name->chars && ev->func_name->chars
//                    && strcmp(fn->name->chars, ev->func_name->chars) == 0) {
//                    found = true;
//                    break;
//                }
//            }
//
//            if (!found) {
//                bugf("verify_lox_event_bindings: room %" PRVNUM
//                     " references missing Lox function '%s' for event '%s'.",
//                     VNUM_FIELD(rd), ev->func_name->chars ? ev->func_name->chars : "<null>",
//                     ev->name && ev->name->chars ? ev->name->chars : "<unnamed>");
//            }
//        }
//    }
//
//    return;
//}

// Load mobprogs section
void load_mobprogs(FILE* fp)
{
    MobProgCode* pMprog;

    if (global_areas.count == 0) {
        bug("Load_mobprogs: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
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

        pMprog = new_mob_prog_code();
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
void fix_mobprogs()
{
    MobPrototype* p_mob_proto;
    MobProg* list;
    MobProgCode* prog;

    FOR_EACH_MOB_PROTO(p_mob_proto) {
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

/* OLC
 * Reset one room.  Called by reset_area and olc.
 */
void reset_room(Room* room)
{
    Reset* reset;
    Mobile* mob = NULL;
    Object* obj;
    Mobile* last_mob = NULL;
    Object* last_obj = NULL;
    int iExit;
    int level = 0;
    bool last;

    if (!room)
        return;

    last = false;

    for (iExit = 0; iExit < DIR_MAX; iExit++) {
        RoomExit* room_exit;
        if ((room_exit = room->exit[iExit])) {
            room_exit->exit_flags = room_exit->data->exit_reset_flags;
            if ((room_exit->to_room != NULL)
                && ((room_exit = room_exit->to_room->exit[dir_list[iExit].rev_dir]))) {
                  /* nail the other side */
                room_exit->exit_flags = room_exit->data->exit_reset_flags;
            }
        }
    }

    FOR_EACH(reset, room->data->reset_first) {
        MobPrototype* mob_proto;
        ObjPrototype* obj_proto;
        ObjPrototype* obj_to_proto;
        Room* target_room;
        char buf[MAX_STRING_LENGTH];
        int count, limit = 0;

        switch (reset->command) {
        default:
            bug("Reset_room: bad command %c.", reset->command);
            break;

        case 'M':
        {
            if ((mob_proto = get_mob_prototype(reset->arg1)) == NULL) {
                bug("Reset_room: 'M': bad vnum %"PRVNUM".", reset->arg1);
                continue;
            }

            if ((target_room = get_room(room->area, reset->arg3)) == NULL) {
                bug("Reset_area: 'R': bad vnum %"PRVNUM".", reset->arg3);
                continue;
            }

            if (reset->arg2 == -1)
                limit = 999;    // No limit
            else
                limit = reset->arg2;

            if (mob_proto->count >= limit) {
                last = false;
                break;
            }

            /* */

            count = 0;
            Mobile* temp_mob;
            FOR_EACH_ROOM_MOB(temp_mob, target_room)
                if (temp_mob->prototype == mob_proto) {
                    count++;
                    if (count >= reset->arg4) {
                        last = false;
                        break;
                    }
                }

            if (count >= reset->arg4)
                break;

            /* */

            mob = create_mobile(mob_proto);

            // Some more hard coding.
            if (room_is_dark(room))
                SET_BIT(mob->affect_flags, AFF_INFRARED);

            // Pet shop mobiles get ACT_PET set.
            {
                RoomData* petshop_inv;
                if ((petshop_inv = get_room_data(VNUM_FIELD(room->data) - 1)) != NULL
                    && IS_SET(petshop_inv->room_flags, ROOM_PET_SHOP))
                    SET_BIT(mob->act_flags, ACT_PET);
            }

            mob_to_room(mob, room);
            last_mob = mob;
            level = URANGE(0, mob->level - 2, LEVEL_HERO - 1);
            last = true;
            break;
        }

        case 'O':
            if (!(obj_proto = get_object_prototype(reset->arg1))) {
                bug("Reset_room: 'O' 1 : bad vnum %"PRVNUM"", reset->arg1);
                sprintf(buf, "%"PRVNUM" %d %"PRVNUM" %d", reset->arg1, 
                    reset->arg2, reset->arg3, reset->arg4);
                bug(buf, 1);
                continue;
            }

            if (!(target_room = get_room(room->area, reset->arg3))) {
                bug("Reset_room: 'O' 2 : bad vnum %"PRVNUM".", reset->arg3);
                sprintf(buf, "%"PRVNUM" %d %"PRVNUM" %d", reset->arg1, 
                    reset->arg2, reset->arg3, reset->arg4);
                bug(buf, 1);
                continue;
            }

            if ((room->area->nplayer > 0 && !room->area->data->always_reset)
                || count_obj_list(obj_proto, &room->objects) > 0
                ) {
                last = false;
                break;
            }

            obj = create_object(obj_proto, (LEVEL)UMIN(number_fuzzy(level),
                LEVEL_HERO - 1)); 
            obj->cost = 0;
            obj_to_room(obj, room);
            last = true;
            break;

        case 'P':
            if ((obj_proto = get_object_prototype(reset->arg1)) == NULL) {
                bug("Reset_room: 'P': bad vnum %"PRVNUM".", reset->arg1);
                continue;
            }
            if ((obj_to_proto = get_object_prototype(reset->arg3)) == NULL) {
                bug("Reset_room: 'P': bad vnum %"PRVNUM".", reset->arg3);
                continue;
            }

            if (reset->arg2 > 50)
                limit = 6;
            else if (reset->arg2 <= 0)
                limit = 999;
            else
                limit = reset->arg2;

            count = 0;
            if ((room->area->nplayer > 0 && !room->area->data->always_reset)
                || (last_obj = get_obj_type(obj_to_proto)) == NULL
                || (last_obj->in_room == NULL && !last)
                || (obj_proto->count >= limit /* && number_range(0,4) != 0 */)
                || (count = count_obj_list(obj_proto, &last_obj->objects))
                > reset->arg4) {
                last = false;
                break;
            }

            /* lastObj->level  -  ROM */
            while (count < reset->arg4) {
                obj = create_object(obj_proto, (LEVEL)number_fuzzy(last_obj->level));
                obj_to_obj(obj, last_obj);
                count++;
                if (obj_proto->count >= limit)
                    break;
            }

            /* fix object lock state! */
            last_obj->value[1] = last_obj->prototype->value[1];
            last = true;
            break;

        case 'G':
        case 'E':
            if ((obj_proto = get_object_prototype(reset->arg1)) == NULL) {
                bug("Reset_room: 'E' or 'G': bad vnum %"PRVNUM".", reset->arg1);
                continue;
            }

            if (!last)
                break;

            if (!last_mob) {
                bug("Reset_room: 'E' or 'G': null mob for vnum %"PRVNUM".",
                    reset->arg1);
                last = false;
                break;
            }

            if (last_mob->prototype->pShop != NULL) { // Shopkeeper?
                int olevel = 0;
                obj = create_object(obj_proto, (LEVEL)olevel);
                SET_BIT(obj->extra_flags, ITEM_INVENTORY);
            }
            else {
                if (reset->arg2 > 50)
                    limit = 6;      // Old format
                else if (reset->arg2 == -1 || reset->arg2 == 0)
                    limit = 999;    // No limit
                else
                    limit = reset->arg2;

                if (obj_proto->count < limit || number_range(0, 4) == 0) {
                    obj = create_object(obj_proto,
                        (LEVEL)UMIN(number_fuzzy(level), LEVEL_HERO - 1));
                    // Error message if it is too high
                    if (obj->level > last_mob->level + 3
                        || (obj->item_type == ITEM_WEAPON
                            && reset->command == 'E'
                            && obj->level < last_mob->level - 5 && obj->level < 45))
                        fprintf(stderr,
                            "Err: obj %s [%"PRVNUM"] Lvl %d -- mob %s [%"PRVNUM"] Lvl %d\n",
                            obj->short_descr, VNUM_FIELD(obj->prototype), obj->level, 
                            last_mob->short_descr, VNUM_FIELD(last_mob->prototype), 
                            last_mob->level);
                }
                else
                    break;
            }

            obj_to_char(obj, last_mob);
            if (reset->command == 'E')
                equip_char(last_mob, obj, reset->arg3);
            last = true;
            break;

        case 'D':
            break;

        case 'R':
            if (!(target_room = get_room(room->area, reset->arg1))) {
                bug("Reset_room: 'R': bad vnum %"PRVNUM".", reset->arg1);
                continue;
            }

            {
                RoomExit* room_exit;
                int d0;
                int d1;

                for (d0 = 0; d0 < reset->arg2 - 1; d0++) {
                    d1 = number_range(d0, reset->arg2 - 1);
                    room_exit = target_room->exit[d0];
                    target_room->exit[d0] = target_room->exit[d1];
                    target_room->exit[d1] = room_exit;
                }
            }
            break;
        }
    }

    return;
}

void reset_area(Area* area)
{
    Room* room;

    FOR_EACH_AREA_ROOM(room, area)
        reset_room(room);
}

MobProgCode* get_mprog_index(VNUM vnum)
{
    for (MobProgCode* prg = mprog_list; prg; NEXT_LINK(prg)) {
        if (prg->vnum == vnum)
            return(prg);
    }
    return NULL;
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

String* fread_lox_string(FILE* fp)
{
    char* str = fread_string(fp);
    int len = (int)strlen(str);
    return copy_string(str, len);
}

char* fread_lox_script(FILE* fp)
{
    char* plast;
    char c = '\0';
    char last_char = '\0';

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
        last_char = c;
        switch (*plast = c = (char)getc(fp)) {
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
            if (last_char == '#')  
            {
                // Special case for scripts
                --plast;
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
        } // end switch
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

    //if (IS_OBJ((Value)magic))
    //        return;

    if (*magic != MAGIC_NUM) {
        bug("Attempt to recycle invalid memory of size %zu.", sMem);
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

    if (str[0] == '\0') 
        return &str_empty[0];

    if (IS_PERM_STRING(str)) 
        return (char*)str;

    str_new = alloc_mem(strlen(str) + 1);
    strcpy(str_new, str);
    return str_new;
}

char* str_append(char* str1, const char* str2)
{
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    char* str_new = alloc_mem(len1 + len2 + 1);
    memcpy(str_new, str1, len1);
    memcpy(str_new + len1, str2, len2);
    str_new[len1 + len2] = '\0';
    free_string(str1);
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

    if (argument[0] != '\0') {
        send_to_char("No argument is used with this command.\n\r", ch);
        return;
    }

    int count = global_areas.count;

    for (int i = 0; i < count; i += 2) {
        AreaData* area1 = AS_AREA_DATA(global_areas.values[i]);
        AreaData* area2 = NULL;
        if (i + 1 < count)
            area2 = AS_AREA_DATA(global_areas.values[i + 1]);
        sprintf(buf, "%-39s%-39s\n\r", 
            (area1 != NULL) ? area1->credits : "",
            (area2 != NULL) ? area2->credits : "");
        send_to_char(buf, ch);
    }

    return;
}

static void memory_to_buffer(Buffer* buf)
{
    addf_buf(buf, "              Perm      In Use\n\r");
    addf_buf(buf, "Affects      %5d     %5d\n\r", affect_perm_count, affect_count);
    addf_buf(buf, "Areas        %5d     %5d\n\r", area_data_perm_count, global_areas.count);
    addf_buf(buf, "- Insts      %5d     %5d\n\r", area_perm_count, area_count);
    addf_buf(buf, "Extra Descs  %5d     %5d\n\r", extra_desc_perm_count, extra_desc_count);
    addf_buf(buf, "Help Areas   %5d     %5d\n\r", help_area_perm_count, help_area_count);
    addf_buf(buf, "Helps        %5d     %5d\n\r", help_perm_count, help_count);
    addf_buf(buf, "Mobs         %5d     %5d\n\r", mob_proto_perm_count, mob_proto_count);
    addf_buf(buf, "- Insts      %5d     %5d\n\r", mob_list.count + mob_free.count, mob_list.count);
    addf_buf(buf, "MobProgs     %5d     %5d\n\r", mob_prog_perm_count, mob_prog_count);
    addf_buf(buf, "MobProgCodes %5d     %5d\n\r", mob_prog_code_perm_count, mob_prog_code_count);
    addf_buf(buf, "Objs         %5d     %5d\n\r", obj_proto_perm_count, obj_proto_count);
    addf_buf(buf, "- Insts      %5d     %5d\n\r", obj_list.count + obj_free.count, obj_list.count);
    addf_buf(buf, "Resets       %5d     %5d\n\r", reset_perm_count, reset_count);
    addf_buf(buf, "Rooms        %5d     %5d\n\r", room_data_perm_count, room_data_count);
    addf_buf(buf, "- Insts      %5d     %5d\n\r", room_perm_count, room_count);
    addf_buf(buf, "Room Exits   %5d     %5d\n\r", room_exit_data_perm_count, room_exit_data_count);
    addf_buf(buf, "- Insts      %5d     %5d\n\r", room_exit_perm_count, room_exit_count);
    addf_buf(buf, "Shops        %5d     %5d\n\r", shop_perm_count, shop_count);
    addf_buf(buf, "Socials      %5d\n\r", social_count);
    addf_buf(buf, "Quests       %5d     %5d\n\r", quest_perm_count, quest_count);

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
    count = mob_list.count;
    count2 = mob_free.count;
    FOR_EACH_GLOBAL_MOB(fch) {
        if (fch->pcdata != NULL)
            num_pcs++;
        FOR_EACH(af, fch->affected) 
            aff_count++;
    }

    fprintf(fp, "Mobs   %4d (%8zu bytes), %2d free (%zu bytes)\n", count,
            count * (sizeof(*fch)), count2, count2 * (sizeof(*fch)));

    /* pcdata */
    count = 0;
    FOR_EACH(pc, player_data_free) 
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
    count = obj_list.count;
    FOR_EACH_GLOBAL_OBJ(obj) {
        FOR_EACH(af, obj->affected)
            aff_count++;
    }
    count2 = obj_free.count;

    fprintf(fp, "Objs	%4d (%8zu bytes), %2d free (%zu bytes)\n", count,
            count * (sizeof(*obj)), count2, count2 * (sizeof(*obj)));

    /* affects */
    count = 0;
    FOR_EACH(af, affect_free) 
        count++;

    fprintf(fp, "Affects	%4d (%8zu bytes), %2d free (%zu bytes)\n", aff_count,
            aff_count * (sizeof(*af)), count, count * (sizeof(*af)));

    /* rooms */
    fprintf(fp, "Rooms	%4d (%8zu bytes)\n", room_data_count,
            room_data_count * (sizeof(*room)));

    /* exits */
    fprintf(fp, "Exits	%4d (%8zu bytes)\n", room_exit_data_count,
            room_exit_data_count * (sizeof(*exit)));

    
    close_file(fp);

    /* start printing out mobile data */

    OPEN_OR_RETURN(fp = open_write_mob_dump_file());

    fprintf(fp, "\nMobile Analysis\n");
    fprintf(fp, "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < mob_proto_count; vnum++)
        if ((p_mob_proto = get_mob_prototype(vnum)) != NULL) {
            nMatch++;
            fprintf(fp, "#%-4d %3d active %3d killed     %s\n", VNUM_FIELD(p_mob_proto),
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
            fprintf(fp, "#%-4d %3d active %3d reset      %s\n", VNUM_FIELD(obj_proto),
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
        if (LOWER(*astr) != LOWER(*bstr))
            return true;
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

    fprintf(fp, "[%5d] %s: %s\n", ch->in_room ? VNUM_FIELD(ch->in_room->data) : 0, NAME_STR(ch), str);

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

String* lox_string(const char* str)
{
    int len = (int)strlen(str);
    return copy_string(str, len);
}
