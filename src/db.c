/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.   *
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
#include "color.h"
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

#include <persist/rom-olc/loader_guard.h>
#include <persist/area/rom-olc/area_persist_rom_olc.h>
#include <persist/area/area_persist.h>
#include <persist/command/command_persist.h>
#include <persist/persist_io_adapters.h>

#include <olc/olc.h>

#include <entities/area.h>
#include <entities/event.h>
#include <entities/faction.h>
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
#include <data/tutorial.h>

#include <mth/mth.h>

#include <lox/lox.h>
#include <lox/native.h>
#include <lox/object.h>
#include <lox/memory.h>
#include <lox/vm.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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
typedef struct string_block {
    struct string_block* next;
    uint32_t hash;
} StringBlock;

#define STRING_BLOCK_OVERHEAD ((int)sizeof(StringBlock))

static inline char* string_block_text(StringBlock* block)
{
    return (char*)(block + 1);
}

StringBlock* string_hash[MAX_KEY_HASH];

static uint32_t boot_hash_string(const char* str, size_t len)
{
    // FNV-1a hash
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint8_t)str[i];
        hash *= 16777619u;
    }
    return hash;
}

char* string_space;
char* top_string;
char str_empty[1];

int	top_mprog_index;    // OLC

#ifdef COUNT_BOOT_STRINGS
static void report_boot_string_stats(void);
static void record_boot_string_stat(const char* str, size_t len);
#endif

/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
//#define MAX_STRING     1413120
#define MAX_STRING      2119680
//#define MAX_PERM_BLOCK  131072
#define MAX_PERM_BLOCK  262144      //131072 * 2
#define MAX_MEM_LIST    20

void* rgFreeList[MAX_MEM_LIST];
const size_t rgSizeList[MAX_MEM_LIST] = {
    16, 24+4, 32+4, 40+4, 64+4, 128+4, 256+4, 512+4, 1024+4, 2048+4, 4096+4,
    MAX_STRING_LENGTH + 4,
    8192,
    MAX_STRING_LENGTH*2 + 4,
    16384,
    MAX_STRING_LENGTH*4 + 4,
    32768 + 4,
    65536 + 4,
    65536 * 2 + 4,
    65536 * 4 + 4,
};

int nAllocString;
size_t sAllocString;
int nAllocPerm;
size_t sAllocPerm;

// Semi-locals.
bool fBootDb;
bool resetting;
FILE* strArea;
char fpArea[MAX_INPUT_LENGTH];
AreaData* current_area_data;

// Local booting procedures.
void init_mm();
void load_helps(FILE* fp, char* fname);
void load_shops(FILE* fp);
void load_specials(FILE* fp);
void load_notes();
void load_mobprogs(FILE* fp);

void fix_exits();
void fix_mobprogs();

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

    init_vm();

    init_value_array(&global_areas);
    init_global_rooms();
    init_global_mob_protos();
    init_global_obj_protos();
    init_table(&faction_table);
    init_list(&mob_list);
    init_list(&mob_free);
    init_list(&obj_list);
    init_list(&obj_free);

    load_config();

    // Init random number generator.
    {
        init_mm();
    }

    init_const_natives();
    load_lox_public_scripts();

    load_skill_table();
    load_class_table();
    load_race_table();
    load_command_table();
    load_tutorials();

    // We need the commands before we can add them to Lox.
    init_native_cmds();

    load_social_table();
    load_skill_group_table();
    load_system_color_themes();

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

            PersistReader reader = persist_reader_from_file(strArea, fpArea);
            const AreaPersistFormat* fmt = area_persist_select_format(fpArea);
            AreaPersistLoadParams params = {
                .reader = &reader,
                .file_name = fpArea,
                .create_single_instance = true,
            };

            PersistResult load_result = fmt->load(&params);
            close_file(strArea);
            strArea = NULL;

            if (!persist_succeeded(load_result)) {
                bugf("Boot_db: failed to load area %s (%s)", fpArea, load_result.message ? load_result.message : "unknown error");
                exit(1);
            }

            gc_protect_clear();
        }
        close_file(fpList);
    }

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
        gc_protect_clear();
        load_notes();
        load_bans();
        load_songs();
    }

    init_mth();

#ifdef COUNT_SIZE_ALLOCS
    report_size_allocs();
    printf("\nScratch Space: %d values, %d capacity.\n\n", 
        gc_protect_vals.count, gc_protect_vals.capacity);
#endif

#ifdef COUNT_BOOT_STRINGS
    report_boot_string_stats();
#endif

    // Clear our scratch space and force a GC to clear them.
    gc_protect_clear();
    collect_garbage_nongrowing();

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

// Snarf a shop section.
void load_shops(FILE* fp)
{
    ShopData* shop;

    for (;;) {
        MobPrototype* mob_proto;
        int iTrade;

        int16_t keeper = (int16_t)fread_number(fp);
        if (keeper == 0)
            break;
        if ((shop = new_shop_data()) == NULL) {
            bug("load_shops: Failed to create shops.");
            exit(1);
        }

        shop->keeper = keeper;
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
                    resetting = true;
                    reset_area(area);
                    resetting = false;
                    sprintf(buf, "%s has just been reset.", NAME_STR(area));
                    wiznet(buf, NULL, NULL, WIZ_RESETS, 0, 0);
                    area->reset_timer = 0;
                }
            }
        }
        gc_protect_clear();
    }

    return;
}

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

static char* intern_string(char* plast)
{
    plast[-1] = '\0';

    StringBlock* new_block = (StringBlock*)top_string;
    char* text = string_block_text(new_block);
    size_t len = (size_t)(plast - text - 1);
    uint32_t hash = boot_hash_string(text, len);
    int bucket = (int)(hash % MAX_KEY_HASH);

    for (StringBlock* block = string_hash[bucket]; block != NULL; block = block->next) {
        if (block->hash == hash && strcmp(string_block_text(block), text) == 0)
            return string_block_text(block);
    }

    if (!fBootDb)
        return str_dup(text);

    new_block->hash = hash;
    new_block->next = string_hash[bucket];
    string_hash[bucket] = new_block;

    nAllocString += 1;
    sAllocString += (size_t)(plast - (char*)new_block);
    top_string = plast;
    return text;
}

char* fread_lox_script(FILE* fp)
{
    char* plast;
    char c = '\0';
    char last_char = '\0';

    plast = top_string + STRING_BLOCK_OVERHEAD;
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
                --plast;
            return intern_string(plast);
        }
    }
}

char* boot_intern_string(const char* str)
{
    if (str == NULL || str[0] == '\0')
        return &str_empty[0];

    if (!fBootDb)
        return str_dup(str);

    size_t len = strlen(str);
    StringBlock* block = (StringBlock*)top_string;
    char* dest = string_block_text(block);
    if (dest + len + 1 > &string_space[MAX_STRING]) {
        bug("Boot_intern_string: MAX_STRING %d exceeded.", MAX_STRING);
        exit(1);
    }

    memcpy(dest, str, len + 1);
    char* plast = dest + len + 1;
    return intern_string(plast);
}

/*
 * Read and allocate space for a string from a file.
 * These strings are read-only and shared.
 * Strings are hashed:
 *   each string prepended with a header linking to the previous string
 *   in the same bucket, hash code is FNV-1a on the string contents.
 *   this function takes 40% to 50% of boot-up time.
 */
char* fread_string(FILE* fp)
{
    char* plast;
    char c;

    plast = top_string + STRING_BLOCK_OVERHEAD;
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
                return intern_string(plast);
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

    plast = top_string + STRING_BLOCK_OVERHEAD;
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
        case '\r':
            return intern_string(plast);
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
    static char buffer[MAX_INPUT_LENGTH];
    char* word = buffer;
    char* pword;
    char cEnd;

    if (fBootDb) {
        word = top_string + STRING_BLOCK_OVERHEAD;
        if (word > &string_space[MAX_STRING - MAX_STRING_LENGTH]) {
            bug("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
            exit(1);
        }
    }

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
            if (fBootDb)
                return intern_string(pword + 1);
            else
                return word;
        }
    }

    bug("Fread_word: word too long.", 0);
    exit(1);
}

#ifdef COUNT_SIZE_ALLOCS

// See if we can optimize memory allocations more efficiently by identifying the 
// sizes most commonly allocated.

typedef struct mem_size_alloc {
    size_t mem_size;
    size_t alloc_size;
    long count;
    struct mem_size_alloc* next;
} MemSizeAlloc;

MemSizeAlloc* mem_size_alloc_list = NULL;
size_t amt_perm_alloced = 0;
size_t amt_temp_alloced = 0;
size_t amt_temp_freed = 0;
size_t sizes_count = 0;

void count_size_alloc(size_t mem_size, size_t alloc_size)
{
    MemSizeAlloc* psa = NULL;
    MemSizeAlloc* last_psa = NULL;
    for (psa = mem_size_alloc_list; psa != NULL && psa->mem_size <= mem_size; psa = psa->next) {
        if (psa->mem_size == mem_size) {
            psa->count++;
            return;
        }
        last_psa = psa;
    }
    MemSizeAlloc* new_psa = (MemSizeAlloc*)alloc_perm(sizeof(MemSizeAlloc));
    sizes_count++;
    new_psa->mem_size = mem_size;
    new_psa->alloc_size = alloc_size;
    new_psa->count = 1;
    if (psa == mem_size_alloc_list) {
        new_psa->next = mem_size_alloc_list;
        mem_size_alloc_list = new_psa;
        return;
    }
    last_psa->next = new_psa;
    new_psa->next = psa;
    return;
}

void decrement_size_alloc(size_t mem_size)
{
    MemSizeAlloc* psa = NULL;
    for (psa = mem_size_alloc_list; psa != NULL; psa = psa->next) {
        if (psa->mem_size == mem_size) {
            psa->count--;
            return;
        }
    }
    return;
}

void report_size_allocs()
{
    MemSizeAlloc* psa;
    FILE* fp;
    size_t magic = sizeof(int);
    size_t total_waste = 0;
    fp = fopen("size_allocs.txt", "w");
    if (!fp) {
        perror("report_size_allocs: fopen");
        return;
    }
    fprintf(fp, "Size Allocations Report: %zu different sizes allocated.\n\n", sizes_count);
    fprintf(fp, "Magic overhead per allocation: %zu bytes\n\n", magic);
    fprintf(fp, "%-10s %-10s %-10s %-10s\n", "Size", "Alloc", "Count", "Waste");
    fprintf(fp, "--------------------------------------\n");
    for (psa = mem_size_alloc_list; psa != NULL; psa = psa->next) {
        size_t waste = (psa->alloc_size - (psa->mem_size + magic)) * psa->count;
        total_waste += waste;
        fprintf(fp, "%-10zu %-10zu %-10ld %-10zu\n", psa->mem_size, psa->alloc_size, psa->count, waste);
    }
    fprintf(fp, "\nTotal wasted memory due to allocation overhead: %zu bytes\n", total_waste);
    fclose(fp);
    return;
}

#endif

#ifdef COUNT_BOOT_STRINGS

typedef struct boot_string_stat_t {
    char* sample;
    size_t length;
    uint32_t hash;
    size_t count;
} BootStringStat;

static BootStringStat* boot_string_table = NULL;
static size_t boot_string_capacity = 0;
static size_t boot_string_unique = 0;
static size_t boot_string_total = 0;
static size_t boot_string_dup_total = 0;
static size_t boot_string_dup_bytes = 0;

static uint32_t boot_string_hash(const char* str, size_t len)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint8_t)str[i];
        hash *= 16777619u;
    }
    return hash;
}

static void boot_string_resize(size_t new_capacity)
{
    BootStringStat* new_table = calloc(new_capacity, sizeof(BootStringStat));
    if (new_table == NULL) {
        perror("boot_string_resize");
        return;
    }

    if (boot_string_table != NULL) {
        for (size_t i = 0; i < boot_string_capacity; i++) {
            BootStringStat* entry = &boot_string_table[i];
            if (entry->sample == NULL)
                continue;
            size_t index = entry->hash & (new_capacity - 1);
            while (new_table[index].sample != NULL)
                index = (index + 1) & (new_capacity - 1);
            new_table[index] = *entry;
        }
        free(boot_string_table);
    }

    boot_string_table = new_table;
    boot_string_capacity = new_capacity;
}

static void boot_string_ensure_capacity(void)
{
    if (boot_string_capacity == 0) {
        boot_string_resize(1024);
        return;
    }

    size_t threshold = (boot_string_capacity * 3) / 4;
    if (boot_string_unique + 1 > threshold)
        boot_string_resize(boot_string_capacity * 2);
}

static BootStringStat* boot_string_find(const char* str, size_t len, uint32_t hash)
{
    size_t mask = boot_string_capacity - 1;
    size_t index = hash & mask;

    for (;;) {
        BootStringStat* entry = &boot_string_table[index];
        if (entry->sample == NULL)
            return entry;

        if (entry->hash == hash && entry->length == len
            && memcmp(entry->sample, str, len) == 0)
            return entry;

        index = (index + 1) & mask;
    }
}

static void record_boot_string_stat(const char* str, size_t len)
{
    if (len == 0)
        return;

    boot_string_ensure_capacity();
    if (boot_string_capacity == 0)
        return;

    boot_string_total++;
    uint32_t hash = boot_string_hash(str, len);

    BootStringStat* entry = boot_string_find(str, len, hash);
    if (entry->sample == NULL) {
        entry->sample = malloc(len + 1);
        if (entry->sample == NULL) {
            perror("record_boot_string_stat");
            return;
        }

        memcpy(entry->sample, str, len + 1);
        entry->length = len;
        entry->hash = hash;
        entry->count = 1;
        boot_string_unique++;
    }
    else {
        entry->count++;
        boot_string_dup_total++;
        boot_string_dup_bytes += len + 1;
    }
}

static int compare_boot_string_stats(const void* lhs, const void* rhs)
{
    const BootStringStat* const* a = (const BootStringStat* const*)lhs;
    const BootStringStat* const* b = (const BootStringStat* const*)rhs;

    if ((*a)->count < (*b)->count)
        return 1;
    if ((*a)->count > (*b)->count)
        return -1;
    if ((*a)->length < (*b)->length)
        return 1;
    if ((*a)->length > (*b)->length)
        return -1;
    return 0;
}

static void format_boot_string_preview(const char* src, size_t len, char* dest, size_t dest_size)
{
    size_t written = 0;
    for (size_t i = 0; i < len && written + 1 < dest_size; i++) {
        unsigned char ch = (unsigned char)src[i];
        if (ch == '\n' || ch == '\r') {
            if (written + 2 >= dest_size)
                break;
            dest[written++] = '\\';
            dest[written++] = 'n';
        }
        else if (isprint(ch)) {
            dest[written++] = (char)ch;
        }
        else {
            dest[written++] = '.';
        }
    }
    dest[written] = '\0';
}

static void report_boot_string_stats(void)
{
    FILE* fp = fopen("boot_string_stats.txt", "w");
    if (fp == NULL) {
        perror("report_boot_string_stats: fopen");
        return;
    }

    fprintf(fp, "Boot-time str_dup() statistics\n");
    fprintf(fp, "Total strings: %zu\n", boot_string_total);
    fprintf(fp, "Unique strings: %zu\n", boot_string_unique);
    fprintf(fp, "Duplicate strings: %zu\n", boot_string_dup_total);
    fprintf(fp, "Duplicate heap bytes: %zu\n", boot_string_dup_bytes);

    size_t string_space_used = (size_t)(top_string - string_space);
    fprintf(fp, "String space usage: %zu / %d bytes (%.2f%%)\n",
        string_space_used, MAX_STRING,
        MAX_STRING > 0 ? (100.0 * (double)string_space_used / (double)MAX_STRING) : 0.0);

    if (boot_string_unique > 0) {
        size_t entry_count = 0;
        for (size_t i = 0; i < boot_string_capacity; i++) {
            if (boot_string_table[i].sample != NULL)
                entry_count++;
        }

        BootStringStat** entries = malloc(entry_count * sizeof(BootStringStat*));
        if (entries != NULL) {
            size_t idx = 0;
            for (size_t i = 0; i < boot_string_capacity; i++) {
                if (boot_string_table[i].sample != NULL)
                    entries[idx++] = &boot_string_table[i];
            }

            qsort(entries, entry_count, sizeof(BootStringStat*), compare_boot_string_stats);

            fprintf(fp, "\nTop duplicate strings:\n");
            fprintf(fp, "%8s %8s %12s  %s\n", "Count", "Length", "DupBytes", "Sample");
            size_t limit = entry_count < 100 ? entry_count : 100;
            for (size_t i = 0; i < limit; i++) {
                BootStringStat* entry = entries[i];
                size_t dup_bytes = (entry->count - 1) * (entry->length + 1);
                char preview[81];
                format_boot_string_preview(entry->sample, entry->length, preview, sizeof(preview));
                fprintf(fp, "%8zu %8zu %12zu  %s\n",
                    entry->count, entry->length, dup_bytes, preview);
            }

            free(entries);
        }
    }

    fclose(fp);

    if (boot_string_table != NULL) {
        for (size_t i = 0; i < boot_string_capacity; i++) {
            free(boot_string_table[i].sample);
            boot_string_table[i].sample = NULL;
        }
        free(boot_string_table);
        boot_string_table = NULL;
        boot_string_capacity = 0;
    }
}

#endif

/*
 * Allocate some ordinary memory,
 *   with the expectation of freeing it someday.
 */
void* alloc_mem(size_t sMem)
{
    uintptr_t mem_addr;
    int* magic;
    int iList;

#ifdef COUNT_SIZE_ALLOCS
    size_t orig_sMem = sMem;
#endif

    sMem += sizeof(*magic); 

    for (iList = 0; iList < MAX_MEM_LIST; iList++) {
        if (sMem <= rgSizeList[iList])
            break;
    }

    if (iList == MAX_MEM_LIST) {
        bug("Alloc_mem: size %zu too large.", sMem);
        exit(1);
    }

#ifdef COUNT_SIZE_ALLOCS
    count_size_alloc(orig_sMem, rgSizeList[iList]);
    amt_temp_alloced += rgSizeList[iList];
#endif

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

#ifdef COUNT_SIZE_ALLOCS
    decrement_size_alloc(sMem);
    amt_temp_freed += sMem;
#endif

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

    while (sMem % sizeof(void*) != 0) 
        sMem++;

    if (sMem > MAX_PERM_BLOCK) {
        bug("Alloc_perm: %zu too large.", sMem);
        exit(1);
    }

    if (pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK) {
        iMemPerm = 0;
        if ((pMemPerm = calloc(1, MAX_PERM_BLOCK)) == NULL) {
#ifdef COUNT_SIZE_ALLOCS
            amt_perm_alloced += MAX_PERM_BLOCK;
#endif
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
    if (str[0] == '\0') 
        return &str_empty[0];

    if (IS_PERM_STRING(str)) 
        return (char*)str;

    size_t len = strlen(str);
    char* str_new = alloc_mem(len + 1);
    strcpy(str_new, str);

#ifdef COUNT_BOOT_STRINGS
    if (fBootDb)
        record_boot_string_stat(str_new, len);
#endif

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
    addf_buf(buf, "                Perm        In Use   Perm Bytes\n\r");
    addf_buf(buf, "Affects      %7d     %7d     %7d\n\r", affect_perm_count, affect_count, affect_perm_count * sizeof(Affect));
    addf_buf(buf, "Areas        %7d     %7d     %7d\n\r", area_data_perm_count, global_areas.count, area_data_perm_count * sizeof(AreaData));
    addf_buf(buf, "- Insts      %7d     %7d     %7d\n\r", area_perm_count, area_count, area_perm_count * sizeof(Area));
    addf_buf(buf, "Extra Descs  %7d     %7d     %7d\n\r", extra_desc_perm_count, extra_desc_count, extra_desc_perm_count * sizeof(ExtraDesc));
    addf_buf(buf, "Help Areas   %7d     %7d     %7d\n\r", help_area_perm_count, help_area_count, help_area_perm_count * sizeof(HelpArea));
    addf_buf(buf, "Helps        %7d     %7d     %7d\n\r", help_perm_count, help_count, help_perm_count * sizeof(HelpData));
    addf_buf(buf, "Mobs         %7d     %7d     %7d\n\r", mob_proto_perm_count, mob_proto_count, mob_proto_perm_count * sizeof(MobPrototype));
    addf_buf(buf, "- Insts      %7d     %7d     %7d\n\r", mob_list.count + mob_free.count, mob_list.count, (mob_list.count + mob_free.count) * sizeof(Mobile));
    addf_buf(buf, "MobProgs     %7d     %7d     %7d\n\r", mob_prog_perm_count, mob_prog_count, mob_prog_perm_count * sizeof(MobProg));
    addf_buf(buf, "MobProgCodes %7d     %7d     %7d\n\r", mob_prog_code_perm_count, mob_prog_code_count, mob_prog_code_perm_count * sizeof(MobProgCode));
    addf_buf(buf, "Objs         %7d     %7d     %7d\n\r", obj_proto_perm_count, obj_proto_count, obj_proto_perm_count * sizeof(ObjPrototype));
    addf_buf(buf, "- Insts      %7d     %7d     %7d\n\r", obj_list.count + obj_free.count, obj_list.count, (obj_list.count + obj_free.count) * sizeof(Object));
    addf_buf(buf, "Resets       %7d     %7d     %7d\n\r", reset_perm_count, reset_count, reset_perm_count * sizeof(Reset));
    addf_buf(buf, "Rooms        %7d     %7d     %7d\n\r", room_data_perm_count, room_data_count, room_data_perm_count * sizeof(RoomData));
    addf_buf(buf, "- Insts      %7d     %7d     %7d\n\r", room_perm_count, room_count, room_perm_count * sizeof(Room));
    addf_buf(buf, "Room Exits   %7d     %7d     %7d\n\r", room_exit_data_perm_count, room_exit_data_count, room_exit_data_perm_count * sizeof(RoomExitData));
    addf_buf(buf, "- Insts      %7d     %7d     %7d\n\r", room_exit_perm_count, room_exit_count, room_exit_perm_count * sizeof(RoomExit));
    addf_buf(buf, "Shops        %7d     %7d     %7d\n\r", shop_perm_count, shop_count, shop_perm_count * sizeof(ShopData));
    addf_buf(buf, "Socials      %7d                 %7d\n\r", social_count, social_count * sizeof(Social));
    addf_buf(buf, "Quests       %7d     %7d     %7d\n\r", quest_perm_count, quest_count, quest_perm_count * sizeof(Quest));
    addf_buf(buf, "Events       %7d     %7d     %7d\n\r", event_perm_count, event_count, event_perm_count * sizeof(Event));
    addf_buf(buf, "Lox VM                             %9d\n\r", vm.bytes_allocated);

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

// Returns an PascalCased string.
char* pascal_case(const char* str)
{
    static char strcap[MAX_STRING_LENGTH];
    int i;
    int p = 0;
    bool cap = true;

    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] == ' ') {
            cap = true;
        }
        else if (cap) {
            strcap[p++] = UPPER(str[i]);
            cap = false;
        }
        else {
            strcap[p++] = LOWER(str[i]);
        }
    }
    strcap[p] = '\0';

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

    if (current_loader_guard) {
        loader_longjmp(buf, 0);
    }

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
    ObjString* obj_str = copy_string(str, len);
    gc_protect(OBJ_VAL(obj_str));
    return obj_str;
}
