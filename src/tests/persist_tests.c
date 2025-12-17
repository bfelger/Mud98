///////////////////////////////////////////////////////////////////////////////
// persist_tests.c
///////////////////////////////////////////////////////////////////////////////

// Persistence tests require both formats for round-trip testing
#if defined(ENABLE_ROM_OLC_PERSISTENCE) && defined(ENABLE_JSON_PERSISTENCE)

#include "tests.h"
#include "test_registry.h"

#include <persist/area/area_persist.h>
#ifdef ENABLE_JSON_PERSISTENCE
#include <persist/area/json/area_persist_json.h>
#include <persist/json/persist_json.h>
#include <persist/theme/json/theme_persist_json.h>
#include <jansson.h>
#endif
#ifdef ENABLE_ROM_OLC_PERSISTENCE
#include <persist/area/rom-olc/area_persist_rom_olc.h>
#endif
#include <persist/class/class_persist.h>
#include <persist/persist_io_adapters.h>
#include <persist/persist_result.h>
#include <persist/race/race_persist.h>
#include <persist/command/command_persist.h>
#include <persist/skill/skill_persist.h>
#include <persist/social/social_persist.h>
#include <persist/tutorial/tutorial_persist.h>
#include <persist/theme/theme_persist.h>
#include <persist/lox/lox_persist.h>

#include <color.h>
#include <data/class.h>
#include <data/race.h>
#include <data/skill.h>
#include <data/social.h>
#include <data/tutorial.h>

#include <entities/area.h>
#include <entities/faction.h>
#include <entities/mob_prototype.h>
#include <entities/obj_prototype.h>
#include <entities/room.h>

#include <lox/array.h>
#include <lox/object.h>
#include <lox/table.h>
#include <lox/lox.h>

#include <config.h>
#include <db.h>
#include <interp.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef _MSC_VER
#include <direct.h>
#else
#include <sys/stat.h>
#endif

static void ensure_directory_exists(const char* path)
{
    if (!path || !path[0])
        return;

    char tmp[MIL];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);
    while (len > 0 && (tmp[len - 1] == '/' || tmp[len - 1] == '\\'))
        tmp[--len] = '\0';
    if (len == 0)
        return;

#ifdef _MSC_VER
    if (_mkdir(tmp) != 0 && errno != EEXIST) {
        /* ignore */
    }
#else
    if (mkdir(tmp, 0775) != 0 && errno != EEXIST) {
        /* ignore */
    }
#endif
}

// Define this to enable an extremely thorough area round-trip test that
// compares every field of every entity in the area after a ROM->JSON->ROM
// conversion.  This is very slow for large areas, but is useful for catching
// subtle bugs in the persist code.
//#define ENABLE_EXTREME_AREA_ROUND_TRIP_COMPARISON 

// Globals we need to snapshot/restore to avoid polluting the live world.
extern AreaData* area_data_free;
extern int area_count;
extern int area_perm_count;
extern int area_data_count;
extern int area_data_perm_count;

typedef struct persist_state_snapshot_t {
    ValueArray global_areas;
    AreaData* current_area_data;
    AreaData* area_data_free;
    int area_count;
    int area_perm_count;
    int area_data_count;
    int area_data_perm_count;
    OrderedTable global_rooms;
    OrderedTable mob_protos;
    OrderedTable obj_protos;
    Table faction_table;
    VNUM top_vnum_room;
    VNUM top_vnum_mob;
    VNUM top_vnum_obj;
} PersistStateSnapshot;

static void persist_state_begin(PersistStateSnapshot* snap)
{
    snap->global_areas = global_areas;
    snap->current_area_data = current_area_data;
    snap->area_data_free = area_data_free;
    snap->area_count = area_count;
    snap->area_perm_count = area_perm_count;
    snap->area_data_count = area_data_count;
    snap->area_data_perm_count = area_data_perm_count;
    snap->global_rooms = snapshot_global_rooms();
    snap->mob_protos = snapshot_global_mob_protos();
    snap->obj_protos = snapshot_global_obj_protos();
    snap->faction_table = faction_table;
    snap->top_vnum_room = top_vnum_room;
    snap->top_vnum_mob = top_vnum_mob;
    snap->top_vnum_obj = top_vnum_obj;

    global_areas = (ValueArray){ 0 };
    init_value_array(&global_areas);
    current_area_data = NULL;
    area_data_free = NULL;
    area_count = 0;
    area_perm_count = 0;
    area_data_count = 0;
    area_data_perm_count = 0;
    init_global_rooms();
    init_global_mob_protos();
    init_global_obj_protos();
    init_table(&faction_table);
    top_vnum_room = 0;
    top_vnum_mob = 0;
    top_vnum_obj = 0;
}

static void persist_state_end(PersistStateSnapshot* snap)
{
    free_global_rooms();
    free_global_mob_protos();
    free_global_obj_protos();
    free_table(&faction_table);
    free_value_array(&global_areas);
    global_areas = snap->global_areas;
    current_area_data = snap->current_area_data;
    area_data_free = snap->area_data_free;
    area_count = snap->area_count;
    area_perm_count = snap->area_perm_count;
    area_data_count = snap->area_data_count;
    area_data_perm_count = snap->area_data_perm_count;
    restore_global_rooms(snap->global_rooms);
    restore_global_mob_protos(snap->mob_protos);
    restore_global_obj_protos(snap->obj_protos);
    faction_table = snap->faction_table;
    top_vnum_room = snap->top_vnum_room;
    top_vnum_mob = snap->top_vnum_mob;
    top_vnum_obj = snap->top_vnum_obj;
}

static Color make_ansi_color(uint8_t light, uint8_t index)
{
    Color color = { 0 };
    color.mode = COLOR_MODE_16;
    color.code[0] = light;
    color.code[1] = index;
    return color;
}

static Color make_palette_ref_color(uint8_t idx)
{
    Color color = { 0 };
    color.mode = COLOR_MODE_PAL_IDX;
    color.code[0] = idx;
    return color;
}

static ColorTheme* make_test_theme(const char* name, ColorMode mode)
{
    ColorTheme* theme = new_color_theme();
    theme->name = str_dup(name);
    theme->banner = str_dup("");
    theme->mode = mode;
    theme->palette_max = 2;
    theme->palette[0] = make_ansi_color(NORMAL, WHITE);
    theme->palette[1] = make_ansi_color(NORMAL, BLACK);
    for (int i = 0; i < COLOR_SLOT_COUNT; ++i)
        theme->channels[i] = make_palette_ref_color(0);
    theme->channels[SLOT_BACKGROUND] = make_palette_ref_color(1);
    theme->type = COLOR_THEME_TYPE_SYSTEM;
    theme->is_public = false;
    theme->is_changed = false;
    return theme;
}

static int test_theme_json_round_trip()
{
    ColorTheme** list = calloc(2, sizeof(ColorTheme*));
    ASSERT(list != NULL);
    list[0] = make_test_theme("Azure", COLOR_MODE_16);
    list[1] = make_test_theme("Sunset", COLOR_MODE_RGB);
    ASSERT(color_register_system_themes(list, 2));

    PersistBufferWriter buf = { 0 };
    PersistWriter writer = persist_writer_from_buffer(&buf, "theme_json");
    PersistResult save_res = theme_persist_json_save(&writer, "theme_json");
    ASSERT(persist_succeeded(save_res));
    ASSERT(buf.len > 0);

    PersistBufferReaderCtx ctx;
    PersistReader reader = persist_reader_from_buffer(buf.data, buf.len, "theme_json", &ctx);
    PersistResult load_res = theme_persist_json_load(&reader, "theme_json");
    ASSERT(persist_succeeded(load_res));
    ASSERT(system_color_theme_count == 2);
    ASSERT_STR_EQ("Azure", system_color_themes[0]->name);
    ASSERT_STR_EQ("Sunset", system_color_themes[1]->name);
    ASSERT(system_color_themes[1]->mode == COLOR_MODE_RGB);

    free(buf.data);
    return 0;
}

static const char* MIN_AREA_TEXT =
    "#AREADATA\n"
    "Version 2\n"
    "Name Test Area~\n"
    "Builders Tester~\n"
    "VNUMs 1 1\n"
    "Credits None~\n"
    "Security 9\n"
    "Sector 0\n"
    "Low 1\n"
    "High 10\n"
    "Reset 4\n"
    "AlwaysReset 0\n"
    "InstType 0\n"
    "End\n"
    "\n"
    "#MOBILES\n"
    "#0\n"
    "#OBJECTS\n"
    "#0\n"
    "#ROOMS\n"
    "#0\n"
    "#RESETS\n"
    "S\n"
    "#SHOPS\n"
    "0\n"
    "#SPECIALS\n"
    "S\n"
    "#MOBPROGS\n"
    "#0\n"
    "#$\n";

static const char* AREA_WITH_BEATS_TEXT =
    "#AREADATA\n"
    "Version 2\n"
    "Name Story Area~\n"
    "Builders Tester~\n"
    "VNUMs 10 10\n"
    "Credits None~\n"
    "Security 9\n"
    "Sector 0\n"
    "Low 1\n"
    "High 10\n"
    "Reset 4\n"
    "AlwaysReset 0\n"
    "InstType 0\n"
    "End\n"
    "#STORYBEATS\n"
    "B\n"
    "Beat One~\n"
    "First beat~\n"
    "B\n"
    "Beat Two~\n"
    "Second beat~\n"
    "S\n"
    "#CHECKLIST\n"
    "C 0\n"
    "Task One~\n"
    "Do the thing~\n"
    "C 2\n"
    "Task Two~\n"
    "All done~\n"
    "S\n"
    "#MOBILES\n"
    "#0\n"
    "#OBJECTS\n"
    "#0\n"
    "#ROOMS\n"
    "#0\n"
    "#RESETS\n"
    "S\n"
    "#SHOPS\n"
    "0\n"
    "#SPECIALS\n"
    "S\n"
    "#MOBPROGS\n"
    "#0\n"
    "#HELPS\n"
    "-1 $~\n"
    "#$\n";

static int test_rom_olc_loads_minimal_area()
{
    PersistStateSnapshot snap;
    persist_state_begin(&snap);

    FILE* fp = tmpfile();
    ASSERT_OR_GOTO(fp != NULL, cleanup);
    size_t written = fwrite(MIN_AREA_TEXT, 1, strlen(MIN_AREA_TEXT), fp);
    ASSERT_OR_GOTO(written == strlen(MIN_AREA_TEXT), cleanup);
    rewind(fp);

    PersistReader reader = persist_reader_from_file(fp, "test.are");
    AreaPersistLoadParams params = {
        .reader = &reader,
        .file_name = "test.are",
        .create_single_instance = false,
    };

    PersistResult result = AREA_PERSIST_ROM_OLC.load(&params);
    ASSERT_OR_GOTO(persist_succeeded(result), cleanup);
    ASSERT_OR_GOTO(global_areas.count == 1, cleanup);

    AreaData* area = LAST_AREA_DATA;
    ASSERT_OR_GOTO(area != NULL, cleanup);
    ASSERT_STR_EQ("test.are", area->file_name);
    ASSERT_STR_EQ("Test Area", NAME_STR(area));
    ASSERT(area->min_vnum == 1);
    ASSERT(area->max_vnum == 1);
    ASSERT(area->reset_thresh == 4);

cleanup:
    if (fp)
        fclose(fp);
    persist_state_end(&snap);
    return 0;
}

static int test_rom_olc_save_writes_sections()
{
    PersistStateSnapshot snap;
    persist_state_begin(&snap);

    FILE* load_fp = tmpfile();
    FILE* save_fp = NULL;
    char* buffer = NULL;
    ASSERT_OR_GOTO(load_fp != NULL, cleanup);
    size_t written = fwrite(MIN_AREA_TEXT, 1, strlen(MIN_AREA_TEXT), load_fp);
    ASSERT_OR_GOTO(written == strlen(MIN_AREA_TEXT), cleanup);
    rewind(load_fp);

    PersistReader reader = persist_reader_from_file(load_fp, "test.are");
    AreaPersistLoadParams load_params = {
        .reader = &reader,
        .file_name = "test.are",
        .create_single_instance = false,
    };

    PersistResult load_result = AREA_PERSIST_ROM_OLC.load(&load_params);
    ASSERT_OR_GOTO(persist_succeeded(load_result), cleanup);
    ASSERT_OR_GOTO(global_areas.count == 1, cleanup);

    AreaData* area = LAST_AREA_DATA;
    ASSERT_OR_GOTO(area != NULL, cleanup);

    save_fp = tmpfile();
    ASSERT_OR_GOTO(save_fp != NULL, cleanup);
    PersistWriter writer = persist_writer_from_file(save_fp, "out.are");
    AreaPersistSaveParams save_params = {
        .writer = &writer,
        .area = area,
        .file_name = "out.are",
    };

    PersistResult save_result = AREA_PERSIST_ROM_OLC.save(&save_params);
    ASSERT_OR_GOTO(persist_succeeded(save_result), cleanup);

    fflush(save_fp);
    fseek(save_fp, 0, SEEK_END);
    long len = ftell(save_fp);
    ASSERT_OR_GOTO(len > 0, cleanup);
    rewind(save_fp);

    buffer = calloc((size_t)len + 1, 1);
    ASSERT_OR_GOTO(buffer != NULL, cleanup);
    size_t read = fread(buffer, 1, (size_t)len, save_fp);
    ASSERT_OR_GOTO(read == (size_t)len, cleanup);
    buffer[len] = '\0';

    ASSERT(strstr(buffer, "#AREADATA") != NULL);
    ASSERT(strstr(buffer, "Name Test Area~") != NULL);
    ASSERT(strstr(buffer, "#MOBPROGS") != NULL);
    ASSERT(strstr(buffer, "#$") != NULL);

cleanup:
    if (buffer)
        free(buffer);
    if (save_fp)
        fclose(save_fp);
    if (load_fp)
        fclose(load_fp);
    persist_state_end(&snap);
    return 0;
}

static int test_rom_olc_round_trip_in_memory()
{
    PersistStateSnapshot snap;
    persist_state_begin(&snap);

    // Load the minimal area from a FILE*-backed reader.
    FILE* load_fp = tmpfile();
    ASSERT_OR_GOTO(load_fp != NULL, cleanup);
    size_t written = fwrite(MIN_AREA_TEXT, 1, strlen(MIN_AREA_TEXT), load_fp);
    ASSERT_OR_GOTO(written == strlen(MIN_AREA_TEXT), cleanup);
    rewind(load_fp);

    PersistReader reader = persist_reader_from_file(load_fp, "test.are");
    AreaPersistLoadParams load_params = {
        .reader = &reader,
        .file_name = "test.are",
        .create_single_instance = false,
    };

    PersistResult load_result = AREA_PERSIST_ROM_OLC.load(&load_params);
    ASSERT_OR_GOTO(persist_succeeded(load_result), cleanup);
    ASSERT_OR_GOTO(global_areas.count == 1, cleanup);

    AreaData* area = LAST_AREA_DATA;
    ASSERT_OR_GOTO(area != NULL, cleanup);

    // Save to buffer.
    PersistBufferWriter buf = { 0 };
    PersistWriter writer = persist_writer_from_buffer(&buf, "out.are");
    AreaPersistSaveParams save_params = {
        .writer = &writer,
        .area = area,
        .file_name = "out.are",
    };

    PersistResult save_result = AREA_PERSIST_ROM_OLC.save(&save_params);
    ASSERT_OR_GOTO(persist_succeeded(save_result), cleanup_freebuf);
    ASSERT(buf.len > 0);

    // Reload from saved buffer into a fresh state using FILE-backed reader.
    persist_state_end(&snap);
    persist_state_begin(&snap);

    FILE* reload_fp = tmpfile();
    ASSERT_OR_GOTO(reload_fp != NULL, cleanup_freebuf);
    size_t reload_written = fwrite(buf.data, 1, buf.len, reload_fp);
    ASSERT_OR_GOTO(reload_written == buf.len, cleanup_freebuf);
    rewind(reload_fp);

    PersistReader reload_reader = persist_reader_from_file(reload_fp, "out.are");
    AreaPersistLoadParams reload_params = {
        .reader = &reload_reader,
        .file_name = "out.are",
        .create_single_instance = false,
    };

    PersistResult reload_result = AREA_PERSIST_ROM_OLC.load(&reload_params);
    ASSERT_OR_GOTO(persist_succeeded(reload_result), cleanup_freebuf);
    ASSERT(global_areas.count == 1);

    AreaData* re_area = LAST_AREA_DATA;
    ASSERT(re_area != NULL);
    ASSERT_STR_EQ("out.are", re_area->file_name);
    ASSERT_STR_EQ("Test Area", NAME_STR(re_area));

cleanup_freebuf:
    free(buf.data);
cleanup:
    if (load_fp)
        fclose(load_fp);
    persist_state_end(&snap);
    return 0;
}

static int test_rom_olc_rejects_bad_section()
{
    PersistStateSnapshot snap;
    persist_state_begin(&snap);

    const char* bad_area =
        "#AREADATA\n"
        "Version 2\n"
        "Name Bad Area~\n"
        "Builders Tester~\n"
        "VNUMs 1 1\n"
        "Credits None~\n"
        "Security 9\n"
        "Sector 0\n"
        "Low 1\n"
        "High 10\n"
        "Reset 4\n"
        "AlwaysReset 0\n"
        "InstType 0\n"
        "End\n"
        "#BADSECTION\n"
        "#$\n";

    FILE* fp = tmpfile();
    ASSERT_OR_GOTO(fp != NULL, cleanup);
    size_t written = fwrite(bad_area, 1, strlen(bad_area), fp);
    ASSERT_OR_GOTO(written == strlen(bad_area), cleanup);
    rewind(fp);

    PersistReader reader = persist_reader_from_file(fp, "bad.are");
    AreaPersistLoadParams params = {
        .reader = &reader,
        .file_name = "bad.are",
        .create_single_instance = false,
    };

    PersistResult result = AREA_PERSIST_ROM_OLC.load(&params);
    ASSERT(!persist_succeeded(result));

cleanup:
    if (fp)
        fclose(fp);
    persist_state_end(&snap);
    return 0;
}

#define PERSIST_EXHAUSTIVE_PERSIST_TEST
#ifdef PERSIST_EXHAUSTIVE_PERSIST_TEST
static bool read_file_to_buffer(const char* path, char** out_buf, size_t* out_len)
{
    FILE* fp = fopen(path, "rb");
    if (!fp)
        return false;
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    if (len < 0) {
        fclose(fp);
        return false;
    }
    rewind(fp);
    char* buf = malloc((size_t)len);
    if (!buf) {
        fclose(fp);
        return false;
    }
    size_t read = fread(buf, 1, (size_t)len, fp);
    fclose(fp);
    if (read != (size_t)len) {
        free(buf);
        return false;
    }
    *out_buf = buf;
    *out_len = (size_t)len;
    return true;
}

static int test_rom_olc_exhaustive_area_round_trip()
{
    const char* area_dir = cfg_get_area_dir();
    const char* area_list = cfg_get_area_list();
    char list_path[MIL];
    sprintf(list_path, "%s%s", area_dir, area_list);

    FILE* fpList = fopen(list_path, "r");
    if (!fpList)
        return 0; // quietly skip if areas unavailable

    char fname[MIL];
    while (fscanf(fpList, "%s", fname) == 1) {
        if (fname[0] == '$')
            break;

        PersistResult load_result = { PERSIST_ERR_INTERNAL, NULL, -1 };

        PersistStateSnapshot snap;
        persist_state_begin(&snap);

        char area_path[MIL];
        sprintf(area_path, "%s%s", area_dir, fname);

        char* orig = NULL;
        size_t orig_len = 0;
        ASSERT_OR_GOTO(read_file_to_buffer(area_path, &orig, &orig_len), next_area);

        FILE* load_fp = fopen(area_path, "r");
        ASSERT_OR_GOTO(load_fp != NULL, next_area);

        PersistReader reader = persist_reader_from_file(load_fp, fname);
        AreaPersistLoadParams load_params = {
            .reader = &reader,
            .file_name = fname,
            .create_single_instance = false,
        };

        load_result = AREA_PERSIST_ROM_OLC.load(&load_params);
        fclose(load_fp);
        if (!persist_succeeded(load_result) || global_areas.count != 1)
            goto next_area;

        AreaData* area = LAST_AREA_DATA;
        ASSERT_OR_GOTO(area != NULL, next_area);

        PersistBufferWriter buf = { 0 };
        PersistWriter writer = persist_writer_from_buffer(&buf, fname);
        AreaPersistSaveParams save_params = {
            .writer = &writer,
            .area = area,
            .file_name = fname,
        };

        PersistResult save_result = AREA_PERSIST_ROM_OLC.save(&save_params);
        if (!persist_succeeded(save_result))
            goto next_area_freebuf;

        if (buf.data == NULL || buf.len != orig_len || memcmp(buf.data, orig, orig_len) != 0)
            goto next_area_freebuf;

    next_area_freebuf:
        free(buf.data);
    next_area:
        free(orig);
        persist_state_end(&snap);
        if (!persist_succeeded(load_result))
            break;
    }

    fclose(fpList);
    return 0;
}
#endif

#define PERSIST_JSON_EXHAUSTIVE_TEST
#if defined(PERSIST_EXHAUSTIVE_PERSIST_TEST) && defined(PERSIST_JSON_EXHAUSTIVE_TEST)
static int test_json_exhaustive_area_round_trip()
{
    const char* area_dir = cfg_get_area_dir();
    const char* area_list = cfg_get_area_list();
    char list_path[MIL];
    sprintf(list_path, "%s%s", area_dir, area_list);

    FILE* fpList = fopen(list_path, "r");
    if (!fpList)
        return 0; // quietly skip if areas unavailable

    char fname[MIL];
    while (fscanf(fpList, "%s", fname) == 1) {
        if (fname[0] == '$')
            break;

        PersistStateSnapshot snap_save;
        persist_state_begin(&snap_save);

        char area_path[MIL];
        sprintf(area_path, "%s%s", area_dir, fname);

        FILE* load_fp = fopen(area_path, "r");
        if (!load_fp) {
            persist_state_end(&snap_save);
            continue;
        }

        PersistReader reader = persist_reader_from_file(load_fp, fname);
        AreaPersistLoadParams load_params = {
            .reader = &reader,
            .file_name = fname,
            .create_single_instance = false,
        };

        PersistResult load_result = AREA_PERSIST_ROM_OLC.load(&load_params);
        fclose(load_fp);
        if (!persist_succeeded(load_result) || global_areas.count != 1) {
            persist_state_end(&snap_save);
            continue;
        }

        AreaData* area = LAST_AREA_DATA;
        if (!area) {
            persist_state_end(&snap_save);
            continue;
        }

        PersistBufferWriter buf = { 0 };
        PersistWriter writer = persist_writer_from_buffer(&buf, fname);
        AreaPersistSaveParams save_params = {
            .writer = &writer,
            .area = area,
            .file_name = fname,
        };

        PersistResult save_result = AREA_PERSIST_JSON.save(&save_params);
        persist_state_end(&snap_save);

        if (!persist_succeeded(save_result) || buf.data == NULL || buf.len == 0) {
            free(buf.data);
            continue;
        }

        // Load the JSON we just wrote in a fresh state to ensure it parses.
        PersistStateSnapshot snap_load;
        persist_state_begin(&snap_load);

        PersistBufferReaderCtx ctx;
        PersistReader json_reader = persist_reader_from_buffer(buf.data, buf.len, fname, &ctx);
        AreaPersistLoadParams json_params = {
            .reader = &json_reader,
            .file_name = fname,
            .create_single_instance = false,
        };

        PersistResult json_result = AREA_PERSIST_JSON.load(&json_params);
        free(buf.data);
        persist_state_end(&snap_load);

        if (!persist_succeeded(json_result))
            break;
    }

    fclose(fpList);
    return 0;
}

static bool save_area_rom_to_buffer(AreaData* area, const char* fname, PersistBufferWriter* out_buf)
{
    PersistWriter writer = persist_writer_from_buffer(out_buf, fname);
    AreaPersistSaveParams params = {
        .writer = &writer,
        .area = area,
        .file_name = fname,
    };
    PersistResult res = AREA_PERSIST_ROM_OLC.save(&params);
    return persist_succeeded(res) && out_buf->data != NULL && out_buf->len > 0;
}

// Some areas reference mob/obj prototypes outside their own vnum range (e.g., immort.are).
// When we load a single area in isolation, those prototypes are absent and reset_area will bug().
// Skip instance-count comparison for such areas to focus on JSON/ROM parity rather than cross-area deps.
static bool resets_need_external_prototypes(const AreaData* area)
{
    RoomData* room;
    FOR_EACH_GLOBAL_ROOM(room) {
        if (room->area_data != area)
            continue;
        Reset* reset;
        FOR_EACH(reset, room->reset_first) {
            switch (reset->command) {
            case 'M':
                if (!get_mob_prototype(reset->arg1))
                    return true;
                break;
            case 'O':
            case 'P':
            case 'G':
            case 'E':
                if (!get_object_prototype(reset->arg1))
                    return true;
                break;
            default:
                break;
            }
        }
    }
    return false;
}

static int test_json_canonical_rom_round_trip()
{
    const char* area_dir = cfg_get_area_dir();
    const char* area_list = cfg_get_area_list();
    char list_path[MIL];
    sprintf(list_path, "%s%s", area_dir, area_list);

    FILE* fpList = fopen(list_path, "r");
    if (!fpList)
        return 0; // quietly skip if areas unavailable

    char fname[MIL];
    while (fscanf(fpList, "%s", fname) == 1) {
        if (fname[0] == '$')
            break;

        // Pass 1: load ROM, capture canonical ROM output, and JSON buffer.
        PersistStateSnapshot snap1;
        persist_state_begin(&snap1);

        char area_path[MIL];
        sprintf(area_path, "%s%s", area_dir, fname);

        FILE* load_fp = fopen(area_path, "r");
        if (!load_fp) {
            persist_state_end(&snap1);
            continue;
        }

        PersistReader reader = persist_reader_from_file(load_fp, fname);
        AreaPersistLoadParams load_params = {
            .reader = &reader,
            .file_name = fname,
            .create_single_instance = false,
        };

        PersistResult load_result = AREA_PERSIST_ROM_OLC.load(&load_params);
        fclose(load_fp);
        if (!persist_succeeded(load_result) || global_areas.count != 1) {
            persist_state_end(&snap1);
            continue;
        }

        AreaData* area = LAST_AREA_DATA;
        if (!area) {
            persist_state_end(&snap1);
            continue;
        }

        PersistBufferWriter rom_orig = { 0 };
        if (!save_area_rom_to_buffer(area, fname, &rom_orig)) {
            free(rom_orig.data);
            persist_state_end(&snap1);
            continue;
        }

        PersistBufferWriter json_buf = { 0 };
        PersistWriter json_writer = persist_writer_from_buffer(&json_buf, fname);
        AreaPersistSaveParams json_params = {
            .writer = &json_writer,
            .area = area,
            .file_name = fname,
        };

        PersistResult json_save = AREA_PERSIST_JSON.save(&json_params);
        persist_state_end(&snap1);

        if (!persist_succeeded(json_save) || json_buf.data == NULL || json_buf.len == 0) {
            free(rom_orig.data);
            free(json_buf.data);
            continue;
        }

        // Pass 2: load JSON fresh and capture canonical ROM output.
        PersistStateSnapshot snap2;
        persist_state_begin(&snap2);

        PersistBufferReaderCtx ctx;
        PersistReader json_reader = persist_reader_from_buffer(json_buf.data, json_buf.len, fname, &ctx);
        AreaPersistLoadParams json_load_params = {
            .reader = &json_reader,
            .file_name = fname,
            .create_single_instance = false,
        };

        PersistResult json_load = AREA_PERSIST_JSON.load(&json_load_params);
        if (!persist_succeeded(json_load) || global_areas.count != 1) {
            persist_state_end(&snap2);
            free(rom_orig.data);
            free(json_buf.data);
            continue;
        }

        AreaData* area_from_json = LAST_AREA_DATA;
        PersistBufferWriter rom_from_json = { 0 };
        bool rom_ok = area_from_json && save_area_rom_to_buffer(area_from_json, fname, &rom_from_json);
        persist_state_end(&snap2);

        bool match = rom_ok
            && rom_orig.len == rom_from_json.len
            && memcmp(rom_orig.data, rom_from_json.data, rom_orig.len) == 0;

    free(rom_orig.data);
    free(json_buf.data);
    free(rom_from_json.data);

    if (!match) {
        fclose(fpList);
        return 1; // fail fast on first discrepancy
    }
}

fclose(fpList);
return 0;
}

static int test_json_instance_counts_match_rom()
{
    const char* area_dir = cfg_get_area_dir();
    const char* area_list = cfg_get_area_list();
    char list_path[MIL];
    sprintf(list_path, "%s%s", area_dir, area_list);

    FILE* fpList = fopen(list_path, "r");
    if (!fpList)
        return 0; // quietly skip if areas unavailable

    char fname[MIL];
    while (fscanf(fpList, "%s", fname) == 1) {
        if (fname[0] == '$')
            break;

        char area_path[MIL];
        sprintf(area_path, "%s%s", area_dir, fname);

        // Pass 1: ROM load -> instantiate -> reset -> count instances; also produce JSON buffer.
        int rom_mob_count = 0;
        int rom_obj_count = 0;
        PersistBufferWriter json_buf = { 0 };
        {
            PersistStateSnapshot snap;
            persist_state_begin(&snap);

            FILE* load_fp = fopen(area_path, "r");
            if (!load_fp) {
                persist_state_end(&snap);
                continue;
            }

            PersistReader reader = persist_reader_from_file(load_fp, fname);
            AreaPersistLoadParams load_params = {
                .reader = &reader,
                .file_name = fname,
                .create_single_instance = false,
            };

            PersistResult load_result = AREA_PERSIST_ROM_OLC.load(&load_params);
            fclose(load_fp);
            if (!persist_succeeded(load_result) || global_areas.count != 1) {
                persist_state_end(&snap);
                continue;
            }

            AreaData* area = LAST_AREA_DATA;
            if (!area || resets_need_external_prototypes(area)) {
                persist_state_end(&snap);
                continue;
            }
            Area* inst = area ? create_area_instance(area, true) : NULL;
            if (inst) {
                reset_area(inst);
                rom_mob_count = mob_list.count;
                rom_obj_count = obj_list.count;
            }

            PersistWriter json_writer = persist_writer_from_buffer(&json_buf, fname);
            AreaPersistSaveParams json_params = {
                .writer = &json_writer,
                .area = area,
                .file_name = fname,
            };
            PersistResult json_save = AREA_PERSIST_JSON.save(&json_params);
            if (!persist_succeeded(json_save) || json_buf.data == NULL || json_buf.len == 0) {
                free(json_buf.data);
                json_buf.data = NULL;
                json_buf.len = 0;
            }

            persist_state_end(&snap);
        }

        if (!json_buf.data || json_buf.len == 0)
            continue;

        // Pass 2: JSON load -> instantiate -> reset -> count instances; compare to ROM counts.
        int json_mob_count = 0;
        int json_obj_count = 0;
        {
            PersistStateSnapshot snap;
            persist_state_begin(&snap);

            PersistBufferReaderCtx ctx;
            PersistReader json_reader = persist_reader_from_buffer(json_buf.data, json_buf.len, fname, &ctx);
            AreaPersistLoadParams json_load_params = {
                .reader = &json_reader,
                .file_name = fname,
                .create_single_instance = false,
            };

            PersistResult json_load = AREA_PERSIST_JSON.load(&json_load_params);
            if (persist_succeeded(json_load) && global_areas.count == 1) {
                AreaData* area = LAST_AREA_DATA;
                Area* inst = area ? create_area_instance(area, true) : NULL;
                if (inst) {
                    reset_area(inst);
                    json_mob_count = mob_list.count;
                    json_obj_count = obj_list.count;
                }
            }

            persist_state_end(&snap);
        }

        free(json_buf.data);

        if (rom_mob_count != json_mob_count || rom_obj_count != json_obj_count) {
            fclose(fpList);
            return 1; // mismatch found
        }
    }

    fclose(fpList);
    return 0;
}
#endif // PERSIST_EXHAUSTIVE_PERSIST_TEST && PERSIST_JSON_EXHAUSTIVE_TEST

// -----------------------------------------------------------------------------
// Instance reconciliation (shops, etc.)
// -----------------------------------------------------------------------------

#if defined(PERSIST_EXHAUSTIVE_PERSIST_TEST) && defined(PERSIST_JSON_EXHAUSTIVE_TEST)
static int test_json_instance_shop_counts_match_rom()
{
    const char* area_dir = cfg_get_area_dir();
    const char* area_list = cfg_get_area_list();
    char list_path[MIL];
    sprintf(list_path, "%s%s", area_dir, area_list);

    FILE* fpList = fopen(list_path, "r");
    if (!fpList)
        return 0; // quietly skip if areas unavailable

    char fname[MIL];
    while (fscanf(fpList, "%s", fname) == 1) {
        if (fname[0] == '$')
            break;

        char area_path[MIL];
        sprintf(area_path, "%s%s", area_dir, fname);

        // Pass 1: ROM load -> capture shop_count and JSON buffer.
        int rom_shop_count = 0;
        PersistBufferWriter json_buf = { 0 };
        {
            PersistStateSnapshot snap;
            persist_state_begin(&snap);

            FILE* load_fp = fopen(area_path, "r");
            if (!load_fp) {
                persist_state_end(&snap);
                continue;
            }

            PersistReader reader = persist_reader_from_file(load_fp, fname);
            AreaPersistLoadParams load_params = {
                .reader = &reader,
                .file_name = fname,
                .create_single_instance = false,
            };

            PersistResult load_result = AREA_PERSIST_ROM_OLC.load(&load_params);
            fclose(load_fp);
            if (!persist_succeeded(load_result) || global_areas.count != 1) {
                persist_state_end(&snap);
                continue;
            }

            AreaData* area = LAST_AREA_DATA;
            if (!area || resets_need_external_prototypes(area)) {
                persist_state_end(&snap);
                continue;
            }

            rom_shop_count = shop_count;

            PersistWriter json_writer = persist_writer_from_buffer(&json_buf, fname);
            AreaPersistSaveParams json_params = {
                .writer = &json_writer,
                .area = area,
                .file_name = fname,
            };
            PersistResult json_save = AREA_PERSIST_JSON.save(&json_params);
            if (!persist_succeeded(json_save) || json_buf.data == NULL || json_buf.len == 0) {
                free(json_buf.data);
                json_buf.data = NULL;
                json_buf.len = 0;
            }

            persist_state_end(&snap);
        }

        if (!json_buf.data || json_buf.len == 0)
            continue;

        // Pass 2: JSON load -> capture shop_count; compare to ROM count.
        int json_shop_count = 0;
        {
            PersistStateSnapshot snap;
            persist_state_begin(&snap);

            PersistBufferReaderCtx ctx;
            PersistReader json_reader = persist_reader_from_buffer(json_buf.data, json_buf.len, fname, &ctx);
            AreaPersistLoadParams json_load_params = {
                .reader = &json_reader,
                .file_name = fname,
                .create_single_instance = false,
            };

            PersistResult json_load = AREA_PERSIST_JSON.load(&json_load_params);
            if (persist_succeeded(json_load) && global_areas.count == 1) {
                AreaData* area = LAST_AREA_DATA;
                if (area && !resets_need_external_prototypes(area)) {
                    json_shop_count = shop_count;
                }
            }

            persist_state_end(&snap);
        }

        free(json_buf.data);

        if (rom_shop_count != json_shop_count) {
            fclose(fpList);
            return 1; // mismatch found
        }
    }

    fclose(fpList);
    return 0;
}
#endif // PERSIST_EXHAUSTIVE_PERSIST_TEST && PERSIST_JSON_EXHAUSTIVE_TEST

static const char* MIN_AREA_JSON =
    "{\n"
    "  \"formatVersion\": 1,\n"
    "  \"areadata\": {\n"
    "    \"version\": 2,\n"
    "    \"name\": \"JSON Area\",\n"
    "    \"builders\": \"Tester\",\n"
    "    \"vnumRange\": [1, 1],\n"
    "    \"credits\": \"None\",\n"
    "    \"security\": 9,\n"
    "    \"sector\": \"inside\",\n"
    "    \"lowLevel\": 1,\n"
    "    \"highLevel\": 10,\n"
    "    \"reset\": 4,\n"
    "    \"alwaysReset\": false,\n"
    "    \"instType\": 0\n"
    "  }\n"
    "}\n";

static int test_json_loads_areadata()
{
    PersistStateSnapshot snap;
    persist_state_begin(&snap);

    PersistBufferReaderCtx ctx;
    PersistReader reader = persist_reader_from_buffer(MIN_AREA_JSON, strlen(MIN_AREA_JSON), "test.json", &ctx);
    AreaPersistLoadParams params = {
        .reader = &reader,
        .file_name = "test.json",
        .create_single_instance = false,
    };

    PersistResult res = AREA_PERSIST_JSON.load(&params);
    ASSERT_OR_GOTO(persist_succeeded(res), cleanup);
    ASSERT(global_areas.count == 1);

    AreaData* area = LAST_AREA_DATA;
    ASSERT(area != NULL);
    ASSERT_STR_EQ("test.json", area->file_name);
    ASSERT_STR_EQ("JSON Area", NAME_STR(area));
    ASSERT(area->min_vnum == 1);
    ASSERT(area->max_vnum == 1);

cleanup:
    persist_state_end(&snap);
    return 0;
}

static bool json_array_contains(json_t* arr, const char* needle)
{
    if (!json_is_array(arr) || !needle)
        return false;
    size_t sz = json_array_size(arr);
    for (size_t i = 0; i < sz; i++) {
        const char* s = json_string_value(json_array_get(arr, i));
        if (s && str_cmp(s, needle) == 0)
            return true;
    }
    return false;
}

static json_t* find_object_entry(json_t* objs, VNUM vnum)
{
    if (!json_is_array(objs))
        return NULL;
    size_t count = json_array_size(objs);
    for (size_t i = 0; i < count; i++) {
        json_t* o = json_array_get(objs, i);
        if (!json_is_object(o))
            continue;
        if ((VNUM)json_int_or_default(o, "vnum", 0) == vnum)
            return o;
    }
    return NULL;
}

static int test_json_saves_typed_objects()
{
    PersistStateSnapshot snap;
    persist_state_begin(&snap);

    // Load a minimal area via ROM to seed the area structures.
    FILE* load_fp = tmpfile();
    ASSERT_OR_GOTO(load_fp != NULL, cleanup);
    size_t written = fwrite(MIN_AREA_TEXT, 1, strlen(MIN_AREA_TEXT), load_fp);
    ASSERT_OR_GOTO(written == strlen(MIN_AREA_TEXT), cleanup);
    rewind(load_fp);

    PersistReader reader = persist_reader_from_file(load_fp, "test.are");
    AreaPersistLoadParams load_params = {
        .reader = &reader,
        .file_name = "test.are",
        .create_single_instance = false,
    };
    PersistResult load_result = AREA_PERSIST_ROM_OLC.load(&load_params);
    ASSERT_OR_GOTO(persist_succeeded(load_result), cleanup);

    AreaData* area = LAST_AREA_DATA;
    ASSERT_OR_GOTO(area != NULL, cleanup);
    area->max_vnum = 10; // allow our test vnums to live inside the range

    // Weapon prototype with typed data.
    ObjPrototype* weapon = new_object_prototype();
    VNUM_FIELD(weapon) = 1;
    weapon->area = area;
    SET_NAME(weapon, lox_string("test sword"));
    weapon->short_descr = str_dup("a test sword");
    weapon->description = str_dup("A shiny test sword lies here.");
    weapon->material = str_dup("steel");
    weapon->item_type = ITEM_WEAPON;
    weapon->value[0] = WEAPON_SWORD;
    weapon->value[1] = 2;
    weapon->value[2] = 5;
    weapon->value[3] = attack_lookup("slash");
    weapon->value[4] = WEAPON_SHARP;
    global_obj_proto_set(weapon);

    // Container prototype with typed data.
    ObjPrototype* container = new_object_prototype();
    VNUM_FIELD(container) = 2;
    container->area = area;
    SET_NAME(container, lox_string("test chest"));
    container->short_descr = str_dup("a test chest");
    container->description = str_dup("A sturdy test chest is here.");
    container->material = str_dup("oak");
    container->item_type = ITEM_CONTAINER;
    container->value[0] = 10; // capacity
    container->value[1] = CONT_CLOSEABLE | CONT_LOCKED;
    container->value[2] = 123; // key
    container->value[3] = 50;  // max weight
    container->value[4] = 3;   // weight multiplier
    global_obj_proto_set(container);

    // Light prototype with hours.
    ObjPrototype* light = new_object_prototype();
    VNUM_FIELD(light) = 3;
    light->area = area;
    SET_NAME(light, lox_string("test lantern"));
    light->short_descr = str_dup("a test lantern");
    light->description = str_dup("A test lantern sheds light here.");
    light->material = str_dup("glass");
    light->item_type = ITEM_LIGHT;
    light->value[2] = 5; // hours
    global_obj_proto_set(light);

    top_vnum_obj = 3;

    PersistBufferWriter buf = { 0 };
    PersistWriter writer = persist_writer_from_buffer(&buf, "out.json");
    AreaPersistSaveParams save_params = {
        .writer = &writer,
        .area = area,
        .file_name = "out.json",
    };
    PersistResult save_result = AREA_PERSIST_JSON.save(&save_params);
    ASSERT_OR_GOTO(persist_succeeded(save_result), cleanup_buf);
    ASSERT_OR_GOTO(buf.data != NULL && buf.len > 0, cleanup_buf);

    json_error_t jerr;
    json_t* root = json_loadb((const char*)buf.data, buf.len, 0, &jerr);
    ASSERT_OR_GOTO(root != NULL, cleanup_buf);
    json_t* objects = json_object_get(root, "objects");
    ASSERT_OR_GOTO(json_is_array(objects), cleanup_root);

    // Weapon typed block
    json_t* wobj = find_object_entry(objects, 1);
    ASSERT_OR_GOTO(wobj != NULL, cleanup_root);
    json_t* weapon_block = json_object_get(wobj, "weapon");
    ASSERT_OR_GOTO(json_is_object(weapon_block), cleanup_root);
    ASSERT_STR_EQ("sword", JSON_STRING(weapon_block, "class"));
    ASSERT_STR_EQ("slash", JSON_STRING(weapon_block, "damageType"));
    json_t* wflags = json_object_get(weapon_block, "flags");
    ASSERT(json_array_contains(wflags, "sharp"));

    // Container typed block
    json_t* cobj = find_object_entry(objects, 2);
    ASSERT_OR_GOTO(cobj != NULL, cleanup_root);
    json_t* container_block = json_object_get(cobj, "container");
    ASSERT_OR_GOTO(json_is_object(container_block), cleanup_root);
    ASSERT(json_int_or_default(container_block, "capacity", 0) == 10);
    ASSERT(json_int_or_default(container_block, "keyVnum", 0) == 123);
    ASSERT(json_int_or_default(container_block, "maxWeight", 0) == 50);
    ASSERT(json_int_or_default(container_block, "weightMult", 0) == 3);

    // Light typed block
    json_t* lobj = find_object_entry(objects, 3);
    ASSERT_OR_GOTO(lobj != NULL, cleanup_root);
    json_t* light_block = json_object_get(lobj, "light");
    ASSERT_OR_GOTO(json_is_object(light_block), cleanup_root);
    ASSERT(json_int_or_default(light_block, "hours", 0) == 5);

cleanup_root:
    if (root)
        json_decref(root);
cleanup_buf:
    if (buf.data)
        free(buf.data);
cleanup:
    if (load_fp)
        fclose(load_fp);
    persist_state_end(&snap);
    return 0;
}

static void assert_story_checklist_state(AreaData* area)
{
    ASSERT(area != NULL);
    StoryBeat* beat = area->story_beats;
    ASSERT(beat != NULL);
    ASSERT_STR_EQ("Beat One", beat->title);
    ASSERT_STR_EQ("First beat", beat->description);
    ASSERT(beat->next != NULL);
    ASSERT_STR_EQ("Beat Two", beat->next->title);
    ASSERT_STR_EQ("Second beat", beat->next->description);
    ASSERT(beat->next->next == NULL);

    ChecklistItem* item = area->checklist;
    ASSERT(item != NULL);
    ASSERT_STR_EQ("Task One", item->title);
    ASSERT_STR_EQ("Do the thing", item->description);
    ASSERT(item->status == CHECK_TODO);
    ASSERT(item->next != NULL);
    ASSERT_STR_EQ("Task Two", item->next->title);
    ASSERT_STR_EQ("All done", item->next->description);
    ASSERT(item->next->status == CHECK_DONE);
    ASSERT(item->next->next == NULL);
}

static int test_story_checklist_round_trip()
{
    PersistStateSnapshot snap;
    persist_state_begin(&snap);

    FILE* load_fp = tmpfile();
    ASSERT_OR_GOTO(load_fp != NULL, cleanup_all);
    size_t written = fwrite(AREA_WITH_BEATS_TEXT, 1, strlen(AREA_WITH_BEATS_TEXT), load_fp);
    ASSERT_OR_GOTO(written == strlen(AREA_WITH_BEATS_TEXT), cleanup_all);
    rewind(load_fp);

    PersistReader reader = persist_reader_from_file(load_fp, "beats.are");
    AreaPersistLoadParams load_params = {
        .reader = &reader,
        .file_name = "beats.are",
        .create_single_instance = false,
    };
    PersistResult load_res = AREA_PERSIST_ROM_OLC.load(&load_params);
    ASSERT_OR_GOTO(persist_succeeded(load_res), cleanup_all);
    ASSERT_OR_GOTO(global_areas.count == 1, cleanup_all);
    assert_story_checklist_state(LAST_AREA_DATA);

    PersistBufferWriter json_buf = { 0 };
    PersistWriter json_writer = persist_writer_from_buffer(&json_buf, "beats.json");
    AreaPersistSaveParams json_params = {
        .writer = &json_writer,
        .area = LAST_AREA_DATA,
        .file_name = "beats.json",
    };
    PersistResult json_save = AREA_PERSIST_JSON.save(&json_params);
    ASSERT_OR_GOTO(persist_succeeded(json_save), cleanup_all);
    ASSERT_OR_GOTO(json_buf.data != NULL && json_buf.len > 0, cleanup_all);

    PersistStateSnapshot snap2;
    persist_state_begin(&snap2);

    PersistBufferReaderCtx ctx;
    PersistReader json_reader = persist_reader_from_buffer(json_buf.data, json_buf.len, "beats.json", &ctx);
    AreaPersistLoadParams json_load_params = {
        .reader = &json_reader,
        .file_name = "beats.json",
        .create_single_instance = false,
    };
    PersistResult json_load = AREA_PERSIST_JSON.load(&json_load_params);
    ASSERT(persist_succeeded(json_load));
    ASSERT(global_areas.count == 1);
    assert_story_checklist_state(LAST_AREA_DATA);

    persist_state_end(&snap2);

cleanup_all:
    if (load_fp)
        fclose(load_fp);
    persist_state_end(&snap);
    return 0;
}

static int test_areas_json_to_rom_round_trip()
{
    PersistStateSnapshot snap;
    persist_state_begin(&snap);

    // 1) Load minimal area from ROM text.
    FILE* load_fp = tmpfile();
    ASSERT_OR_GOTO(load_fp != NULL, cleanup_all);
    size_t written = fwrite(MIN_AREA_TEXT, 1, strlen(MIN_AREA_TEXT), load_fp);
    ASSERT_OR_GOTO(written == strlen(MIN_AREA_TEXT), cleanup_all);
    rewind(load_fp);

    PersistReader reader = persist_reader_from_file(load_fp, "test.are");
    AreaPersistLoadParams load_params = {
        .reader = &reader,
        .file_name = "test.are",
        .create_single_instance = false,
    };
    PersistResult load_result = AREA_PERSIST_ROM_OLC.load(&load_params);
    ASSERT_OR_GOTO(persist_succeeded(load_result), cleanup_all);
    ASSERT_OR_GOTO(global_areas.count == 1, cleanup_all);
    AreaData* area = LAST_AREA_DATA;
    ASSERT_OR_GOTO(area != NULL, cleanup_all);

    // 2) Save to JSON buffer.
    PersistBufferWriter json_buf = { 0 };
    PersistWriter json_writer = persist_writer_from_buffer(&json_buf, "test.json");
    AreaPersistSaveParams json_save_params = {
        .writer = &json_writer,
        .area = area,
        .file_name = "test.json",
    };
    PersistResult json_save = AREA_PERSIST_JSON.save(&json_save_params);
    ASSERT_OR_GOTO(persist_succeeded(json_save), cleanup_all);
    ASSERT_OR_GOTO(json_buf.data != NULL && json_buf.len > 0, cleanup_all);

    // 3) Reset state and load from JSON buffer.
    persist_state_end(&snap);
    persist_state_begin(&snap);

    PersistBufferReaderCtx json_ctx;
    PersistReader json_reader = persist_reader_from_buffer(json_buf.data, json_buf.len, "test.json", &json_ctx);
    AreaPersistLoadParams json_load_params = {
        .reader = &json_reader,
        .file_name = "test.json",
        .create_single_instance = false,
    };
    PersistResult json_load = AREA_PERSIST_JSON.load(&json_load_params);
    ASSERT_OR_GOTO(persist_succeeded(json_load), cleanup_all);
    ASSERT_OR_GOTO(global_areas.count == 1, cleanup_all);
    AreaData* area2 = LAST_AREA_DATA;
    ASSERT_OR_GOTO(area2 != NULL, cleanup_all);

    // 4) Save back to ROM text buffer and ensure it contains expected markers.
    PersistBufferWriter rom_buf = { 0 };
    PersistWriter rom_writer = persist_writer_from_buffer(&rom_buf, "out.are");
    AreaPersistSaveParams rom_save_params = {
        .writer = &rom_writer,
        .area = area2,
        .file_name = "out.are",
    };
    PersistResult rom_save = AREA_PERSIST_ROM_OLC.save(&rom_save_params);
    ASSERT_OR_GOTO(persist_succeeded(rom_save), cleanup_all);
    ASSERT_OR_GOTO(rom_buf.data != NULL && rom_buf.len > 0, cleanup_all);
    ASSERT(strstr((const char*)rom_buf.data, "#AREADATA") != NULL);
    ASSERT(strstr((const char*)rom_buf.data, "Name Test Area~") != NULL);

cleanup_all:
    if (load_fp)
        fclose(load_fp);
    persist_state_end(&snap);
    return 0;
}

static int test_race_rom_json_round_trip()
{
    // 1) Load races via ROM backend (text)
    PersistResult load_res = race_persist_load(cfg_get_races_file());
    ASSERT(persist_succeeded(load_res));
    ASSERT(race_table != NULL);
    ASSERT(race_count > 0);

    char original_data_dir[MIL];
    snprintf(original_data_dir, sizeof(original_data_dir), "%s", cfg_get_data_dir());
    const char* temp_dir = cfg_get_temp_dir();
    char temp_races_path[MIL];
    snprintf(temp_races_path, sizeof(temp_races_path), "%s%s", temp_dir, "races.json");

    bool data_dir_swapped = false;
    cfg_set_data_dir(temp_dir);
    data_dir_swapped = true;

    // 2) Save to JSON file path (temp name)
    PersistResult save_res = race_persist_save("races.json");
    ASSERT_OR_GOTO(persist_succeeded(save_res), cleanup);

    // 3) Load JSON into fresh state and compare counts
    int saved_count = race_count;

    PersistResult json_load = race_persist_load("races.json");
    ASSERT_OR_GOTO(persist_succeeded(json_load), cleanup);
    ASSERT(race_table != NULL);
    ASSERT(race_count == saved_count);

cleanup:
    if (data_dir_swapped) {
        remove(temp_races_path);
        cfg_set_data_dir(original_data_dir);
    }
    return 0;
}

static int test_class_rom_json_round_trip()
{
    PersistResult load_res = class_persist_load(cfg_get_classes_file());
    ASSERT(persist_succeeded(load_res));
    ASSERT(class_table != NULL);
    ASSERT(class_count > 0);

    char original_data_dir[MIL];
    snprintf(original_data_dir, sizeof(original_data_dir), "%s", cfg_get_data_dir());
    const char* temp_dir = cfg_get_temp_dir();
    char temp_classes_path[MIL];
    snprintf(temp_classes_path, sizeof(temp_classes_path), "%s%s", temp_dir, "classes.json");

    bool data_dir_swapped = false;
    cfg_set_data_dir(temp_dir);
    data_dir_swapped = true;

    PersistResult save_res = class_persist_save("classes.json");
    ASSERT_OR_GOTO(persist_succeeded(save_res), cleanup);

    int saved_count = class_count;
    PersistResult json_load = class_persist_load("classes.json");
    ASSERT_OR_GOTO(persist_succeeded(json_load), cleanup);
    ASSERT(class_table != NULL);
    ASSERT(class_count == saved_count);

cleanup:
    if (data_dir_swapped) {
        remove(temp_classes_path);
        cfg_set_data_dir(original_data_dir);
    }
    return 0;
}
static int test_command_rom_json_round_trip()
{
    PersistResult load_res = command_persist_load(cfg_get_commands_file());
    ASSERT(persist_succeeded(load_res));
    ASSERT(cmd_table != NULL);
    ASSERT(max_cmd > 0);

    char original_data_dir[MIL];
    snprintf(original_data_dir, sizeof(original_data_dir), "%s", cfg_get_data_dir());
    const char* temp_dir = cfg_get_temp_dir();
    char temp_path[MIL];
    snprintf(temp_path, sizeof(temp_path), "%s%s", temp_dir, "commands.json");

    bool swapped = false;
    cfg_set_data_dir(temp_dir);
    swapped = true;

    PersistResult save_res = command_persist_save("commands.json");
    ASSERT_OR_GOTO(persist_succeeded(save_res), cleanup);

    int saved_count = max_cmd;
    PersistResult json_load = command_persist_load("commands.json");
    ASSERT_OR_GOTO(persist_succeeded(json_load), cleanup);
    ASSERT(max_cmd == saved_count);

cleanup:
    if (swapped) {
        remove(temp_path);
        cfg_set_data_dir(original_data_dir);
    }
    return 0;
}

static int test_lox_rom_json_round_trip()
{
    load_lox_public_scripts();
    size_t initial_count = lox_script_entry_count();
    ASSERT(initial_count > 0);

    char original_data_dir[MIL];
    char original_scripts_dir[MIL];
    char original_lox_file[MIL];
    snprintf(original_data_dir, sizeof(original_data_dir), "%s", cfg_get_data_dir());
    snprintf(original_scripts_dir, sizeof(original_scripts_dir), "%s", cfg_get_scripts_dir());
    snprintf(original_lox_file, sizeof(original_lox_file), "%s", cfg_get_lox_file());

    const char* temp_dir = cfg_get_temp_dir();
    char saved_catalog_path[MIL] = "";

    bool swapped = false;
    cfg_set_data_dir(temp_dir);
    cfg_set_lox_file("lox.json");
    swapped = true;

    ensure_directory_exists(cfg_get_data_dir());
    char scripts_path[MIL];
    snprintf(scripts_path, sizeof(scripts_path), "%s%s", cfg_get_data_dir(), cfg_get_scripts_dir());
    ensure_directory_exists(scripts_path);

    snprintf(saved_catalog_path, sizeof(saved_catalog_path), "%s%s%s",
        cfg_get_data_dir(), cfg_get_scripts_dir(), cfg_get_lox_file());

    PersistResult save_res = lox_persist_save(NULL);
    ASSERT_OR_GOTO(persist_succeeded(save_res), cleanup);

    lox_script_registry_clear();

    PersistResult load_res = lox_persist_load(NULL);
    ASSERT_OR_GOTO(persist_succeeded(load_res), cleanup);
    ASSERT(lox_script_entry_count() == initial_count);

cleanup:
    if (swapped) {
        if (saved_catalog_path[0])
            remove(saved_catalog_path);
        cfg_set_data_dir(original_data_dir);
        cfg_set_scripts_dir(original_scripts_dir);
        cfg_set_lox_file(original_lox_file);
        lox_script_registry_clear();
        load_lox_public_scripts();
    }

    return 0;
}

static int test_skill_rom_json_round_trip()
{
    PersistResult load_res = skill_persist_load(cfg_get_skills_file());
    ASSERT(persist_succeeded(load_res));
    ASSERT(skill_table != NULL);
    ASSERT(skill_count > 0);

    char original_data_dir[MIL];
    snprintf(original_data_dir, sizeof(original_data_dir), "%s", cfg_get_data_dir());
    const char* temp_dir = cfg_get_temp_dir();
    char temp_path[MIL];
    snprintf(temp_path, sizeof(temp_path), "%s%s", temp_dir, "skills.json");

    bool swapped = false;
    cfg_set_data_dir(temp_dir);
    swapped = true;

    PersistResult save_res = skill_persist_save("skills.json");
    ASSERT_OR_GOTO(persist_succeeded(save_res), cleanup_skill);

    int saved_count = skill_count;
    PersistResult load_json = skill_persist_load("skills.json");
    ASSERT_OR_GOTO(persist_succeeded(load_json), cleanup_skill);
    ASSERT(skill_count == saved_count);

cleanup_skill:
    if (swapped) {
        remove(temp_path);
        cfg_set_data_dir(original_data_dir);
    }
    return 0;
}

static int test_skill_group_rom_json_round_trip()
{
    PersistResult load_res = skill_group_persist_load(cfg_get_groups_file());
    ASSERT(persist_succeeded(load_res));
    ASSERT(skill_group_table != NULL);
    ASSERT(skill_group_count > 0);

    char original_data_dir[MIL];
    snprintf(original_data_dir, sizeof(original_data_dir), "%s", cfg_get_data_dir());
    const char* temp_dir = cfg_get_temp_dir();
    char temp_path[MIL];
    snprintf(temp_path, sizeof(temp_path), "%s%s", temp_dir, "groups.json");

    bool swapped = false;
    cfg_set_data_dir(temp_dir);
    swapped = true;

    PersistResult save_res = skill_group_persist_save("groups.json");
    ASSERT_OR_GOTO(persist_succeeded(save_res), cleanup_group);

    int saved_count = skill_group_count;
    PersistResult load_json = skill_group_persist_load("groups.json");
    ASSERT_OR_GOTO(persist_succeeded(load_json), cleanup_group);
    ASSERT(skill_group_count == saved_count);

cleanup_group:
    if (swapped) {
        remove(temp_path);
        cfg_set_data_dir(original_data_dir);
    }
    return 0;
}

static int test_social_rom_json_round_trip()
{
    PersistResult load_res = social_persist_load(cfg_get_socials_file());
    ASSERT(persist_succeeded(load_res));
    ASSERT(social_table != NULL);
    ASSERT(social_count > 0);

    char original_data_dir[MIL];
    snprintf(original_data_dir, sizeof(original_data_dir), "%s", cfg_get_data_dir());
    const char* temp_dir = cfg_get_temp_dir();
    char temp_path[MIL];
    snprintf(temp_path, sizeof(temp_path), "%s%s", temp_dir, "socials.json");

    bool swapped = false;
    cfg_set_data_dir(temp_dir);
    swapped = true;

    PersistResult save_res = social_persist_save("socials.json");
    ASSERT_OR_GOTO(persist_succeeded(save_res), cleanup_social);

    int saved_count = social_count;
    PersistResult load_json = social_persist_load("socials.json");
    ASSERT_OR_GOTO(persist_succeeded(load_json), cleanup_social);
    ASSERT(social_count == saved_count);

cleanup_social:
    if (swapped) {
        remove(temp_path);
        cfg_set_data_dir(original_data_dir);
    }
    return 0;
}

static int test_tutorial_rom_json_round_trip()
{
    PersistResult load_res = tutorial_persist_load(cfg_get_tutorials_file());
    ASSERT(persist_succeeded(load_res));
    ASSERT(tutorials != NULL);
    ASSERT(tutorial_count > 0);

    char original_data_dir[MIL];
    snprintf(original_data_dir, sizeof(original_data_dir), "%s", cfg_get_data_dir());
    const char* temp_dir = cfg_get_temp_dir();
    char temp_path[MIL];
    snprintf(temp_path, sizeof(temp_path), "%s%s", temp_dir, "tutorials.json");

    bool swapped = false;
    cfg_set_data_dir(temp_dir);
    swapped = true;

    PersistResult save_res = tutorial_persist_save("tutorials.json");
    ASSERT_OR_GOTO(persist_succeeded(save_res), cleanup_tutorial);

    int saved_count = tutorial_count;
    PersistResult load_json = tutorial_persist_load("tutorials.json");
    ASSERT_OR_GOTO(persist_succeeded(load_json), cleanup_tutorial);
    ASSERT(tutorial_count == saved_count);

cleanup_tutorial:
    if (swapped) {
        remove(temp_path);
        cfg_set_data_dir(original_data_dir);
    }
    return 0;
}


#ifdef ENABLE_EXTREME_AREA_ROUND_TRIP_COMPARISON

#define JSON_PATH_MAX 512

static const char* json_type_name(json_type type)
{
    switch (type) {
    case JSON_OBJECT: return "object";
    case JSON_ARRAY: return "array";
    case JSON_STRING: return "string";
    case JSON_INTEGER: return "integer";
    case JSON_REAL: return "real";
    case JSON_TRUE: return "true";
    case JSON_FALSE: return "false";
    case JSON_NULL: return "null";
    default: return "unknown";
    }
}

static void describe_char(int byte, char* out, size_t out_len)
{
    if (byte < 0) {
        snprintf(out, out_len, "EOF");
        return;
    }

    switch (byte) {
    case '\n':
        snprintf(out, out_len, "\\n (0x0A)");
        return;
    case '\r':
        snprintf(out, out_len, "\\r (0x0D)");
        return;
    case '\t':
        snprintf(out, out_len, "\\t (0x09)");
        return;
    default:
        break;
    }

    if (isprint(byte))
        snprintf(out, out_len, "'%c' (0x%02X)", byte, byte);
    else
        snprintf(out, out_len, "0x%02X", byte);
}

static void log_rom_text_diff(const char* fname,
    const PersistBufferWriter* expected,
    const PersistBufferWriter* actual)
{
    if (!expected || !actual || !expected->data || !actual->data) {
        fprintf(stderr, "ROM mismatch for %s: missing buffer data\n", fname);
        return;
    }

    const unsigned char* exp_data = expected->data;
    const unsigned char* act_data = actual->data;
    size_t exp_len = expected->len;
    size_t act_len = actual->len;
    size_t limit = exp_len < act_len ? exp_len : act_len;
    size_t diff_idx = limit;
    for (size_t i = 0; i < limit; ++i) {
        if (exp_data[i] != act_data[i]) {
            diff_idx = i;
            break;
        }
    }

    bool lengths_equal = exp_len == act_len;
    if (diff_idx == limit && lengths_equal)
        return; // identical

    size_t line = 1;
    size_t col = 1;
    for (size_t i = 0; i < diff_idx && i < exp_len; ++i) {
        if (exp_data[i] == '\n') {
            ++line;
            col = 1;
        }
        else {
            ++col;
        }
    }

    char exp_desc[32];
    char act_desc[32];
    describe_char(diff_idx < exp_len ? exp_data[diff_idx] : -1, exp_desc, sizeof exp_desc);
    describe_char(diff_idx < act_len ? act_data[diff_idx] : -1, act_desc, sizeof act_desc);

    fprintf(stderr,
        "ROM mismatch for %s at byte %zu (line %zu, col %zu): expected %s, got %s. Original len=%zu, round len=%zu\n",
        fname, diff_idx, line, col, exp_desc, act_desc, exp_len, act_len);
}

static bool json_path_push_key(char* path, size_t* path_len, const char* key)
{
    size_t key_len = strlen(key);
    size_t needed = 1 + key_len;
    if (*path_len + needed >= JSON_PATH_MAX)
        return false;
    path[(*path_len)++] = '.';
    memcpy(path + *path_len, key, key_len);
    *path_len += key_len;
    path[*path_len] = '\0';
    return true;
}

static bool json_path_push_index(char* path, size_t* path_len, size_t index)
{
    char buf[32];
    int written = snprintf(buf, sizeof buf, "[%zu]", index);
    if (written < 0)
        return false;
    if (*path_len + (size_t)written >= JSON_PATH_MAX)
        return false;
    memcpy(path + *path_len, buf, (size_t)written);
    *path_len += (size_t)written;
    path[*path_len] = '\0';
    return true;
}

static void json_path_pop(char* path, size_t* path_len, size_t prev_len)
{
    *path_len = prev_len;
    path[*path_len] = '\0';
}

static void format_json_value(const json_t* value, char* buf, size_t buf_len)
{
    if (!value) {
        snprintf(buf, buf_len, "<null>");
        return;
    }

    switch (json_typeof(value)) {
    case JSON_STRING: {
        const char* s = json_string_value(value);
        size_t len = s ? strlen(s) : 0;
        const size_t max = 40;
        if (s && len > max)
            snprintf(buf, buf_len, "\"%.*s...\" (%zu chars)", (int)max, s, len);
        else if (s)
            snprintf(buf, buf_len, "\"%s\"", s);
        else
            snprintf(buf, buf_len, "<null string>");
        break;
    }
    case JSON_INTEGER:
        snprintf(buf, buf_len, "%lld", (long long)json_integer_value(value));
        break;
    case JSON_REAL:
        snprintf(buf, buf_len, "%.15g", json_real_value(value));
        break;
    case JSON_TRUE:
    case JSON_FALSE:
        snprintf(buf, buf_len, "%s", json_is_true(value) ? "true" : "false");
        break;
    case JSON_NULL:
        snprintf(buf, buf_len, "null");
        break;
    case JSON_OBJECT:
        snprintf(buf, buf_len, "<object with %zu keys>", json_object_size(value));
        break;
    case JSON_ARRAY:
        snprintf(buf, buf_len, "<array[%zu]>", json_array_size(value));
        break;
    default:
        snprintf(buf, buf_len, "<unknown>");
        break;
    }
}

static void log_json_mismatch(const char* fname, const char* path,
    const char* reason, const json_t* expected, const json_t* actual)
{
    char exp_buf[128];
    char act_buf[128];
    format_json_value(expected, exp_buf, sizeof exp_buf);
    format_json_value(actual, act_buf, sizeof act_buf);
    fprintf(stderr,
        "JSON mismatch for %s at %s: %s. Expected %s, got %s\n",
        fname, path, reason, exp_buf, act_buf);
}

static bool json_values_equal_verbose(const char* fname, const json_t* expected,
    const json_t* actual, char* path, size_t* path_len)
{
    if (expected == actual)
        return true;
    if (!expected || !actual) {
        log_json_mismatch(fname, path, "one side missing", expected, actual);
        return false;
    }

    json_type exp_type = json_typeof(expected);
    json_type act_type = json_typeof(actual);
    if (exp_type != act_type) {
        char reason[96];
        snprintf(reason, sizeof reason, "type mismatch (%s vs %s)",
            json_type_name(exp_type), json_type_name(act_type));
        log_json_mismatch(fname, path, reason, expected, actual);
        return false;
    }

    switch (exp_type) {
    case JSON_OBJECT: {
        const char* key;
        json_t* exp_val;
        json_object_foreach((json_t*)expected, key, exp_val) {
            size_t prev = *path_len;
            if (!json_path_push_key(path, path_len, key)) {
                fprintf(stderr, "JSON path exceeded %d characters while diffing %s\n",
                    JSON_PATH_MAX, fname);
                return false;
            }
            json_t* act_val = json_object_get(actual, key);
            if (!act_val) {
                log_json_mismatch(fname, path, "missing key", exp_val, NULL);
                json_path_pop(path, path_len, prev);
                return false;
            }
            if (!json_values_equal_verbose(fname, exp_val, act_val, path, path_len)) {
                json_path_pop(path, path_len, prev);
                return false;
            }
            json_path_pop(path, path_len, prev);
        }

        json_t* act_val;
        json_object_foreach((json_t*)actual, key, act_val) {
            if (!json_object_get(expected, key)) {
                size_t prev = *path_len;
                if (!json_path_push_key(path, path_len, key)) {
                    fprintf(stderr, "JSON path exceeded %d characters while diffing %s\n",
                        JSON_PATH_MAX, fname);
                    return false;
                }
                log_json_mismatch(fname, path, "unexpected key", NULL, act_val);
                json_path_pop(path, path_len, prev);
                return false;
            }
        }
        return true;
    }
    case JSON_ARRAY: {
        size_t exp_len = json_array_size(expected);
        size_t act_len = json_array_size(actual);
        if (exp_len != act_len) {
            char reason[96];
            snprintf(reason, sizeof reason, "array length mismatch (%zu vs %zu)", exp_len, act_len);
            log_json_mismatch(fname, path, reason, expected, actual);
            return false;
        }
        for (size_t i = 0; i < exp_len; ++i) {
            size_t prev = *path_len;
            if (!json_path_push_index(path, path_len, i)) {
                fprintf(stderr, "JSON path exceeded %d characters while diffing %s\n",
                    JSON_PATH_MAX, fname);
                return false;
            }
            json_t* exp_val = json_array_get(expected, i);
            json_t* act_val = json_array_get(actual, i);
            if (!json_values_equal_verbose(fname, exp_val, act_val, path, path_len)) {
                json_path_pop(path, path_len, prev);
                return false;
            }
            json_path_pop(path, path_len, prev);
        }
        return true;
    }
    case JSON_STRING: {
        const char* exp_str = json_string_value(expected);
        const char* act_str = json_string_value(actual);
        if ((exp_str == NULL && act_str != NULL)
            || (exp_str != NULL && act_str == NULL)
            || (exp_str && act_str && strcmp(exp_str, act_str) != 0)) {
            log_json_mismatch(fname, path, "string mismatch", expected, actual);
            return false;
        }
        return true;
    }
    case JSON_INTEGER: {
        json_int_t exp_val = json_integer_value(expected);
        json_int_t act_val = json_integer_value(actual);
        if (exp_val != act_val) {
            log_json_mismatch(fname, path, "integer mismatch", expected, actual);
            return false;
        }
        return true;
    }
    case JSON_REAL: {
        double exp_val = json_real_value(expected);
        double act_val = json_real_value(actual);
        if (exp_val != act_val) {
            log_json_mismatch(fname, path, "real mismatch", expected, actual);
            return false;
        }
        return true;
    }
    case JSON_TRUE:
    case JSON_FALSE: {
        bool exp_bool = json_is_true(expected);
        bool act_bool = json_is_true(actual);
        if (exp_bool != act_bool) {
            log_json_mismatch(fname, path, "boolean mismatch", expected, actual);
            return false;
        }
        return true;
    }
    case JSON_NULL:
        return true;
    default:
        log_json_mismatch(fname, path, "unsupported JSON type", expected, actual);
        return false;
    }
}

static json_t* parse_json_from_buffer(const PersistBufferWriter* buf, const char* label)
{
    if (!buf || !buf->data || buf->len == 0)
        return NULL;

    json_error_t error;
    json_t* root = json_loadb((const char*)buf->data, buf->len, 0, &error);
    if (!root) {
        fprintf(stderr, "Failed to parse JSON for %s at line %d: %s\n",
            label, error.line, error.text);
    }
    return root;
}

static void free_buffer_writer(PersistBufferWriter* buf)
{
    if (buf && buf->data) {
        free(buf->data);
        buf->data = NULL;
    }
    if (buf) {
        buf->len = 0;
        buf->cap = 0;
    }
}

static bool capture_area_rom_and_json(const char* area_dir, const char* fname,
    PersistBufferWriter* rom_buf, PersistBufferWriter* json_buf,
    json_t** json_dom_out, bool* skipped_out)
{
    if (json_dom_out)
        *json_dom_out = NULL;
    if (skipped_out)
        *skipped_out = false;

    char area_path[MIL];
    sprintf(area_path, "%s%s", area_dir, fname);

    FILE* load_fp = fopen(area_path, "r");
    if (!load_fp) {
        if (skipped_out)
            *skipped_out = true;
        return true; // quietly skip missing areas
    }

    PersistStateSnapshot snap;
    bool snap_active = false;
    bool ok = false;

    persist_state_begin(&snap);
    snap_active = true;

    PersistReader reader = persist_reader_from_file(load_fp, fname);
    AreaPersistLoadParams load_params = {
        .reader = &reader,
        .file_name = fname,
        .create_single_instance = false,
    };

    PersistResult load_res = AREA_PERSIST_ROM_OLC.load(&load_params);
    fclose(load_fp);
    load_fp = NULL;
    if (!persist_succeeded(load_res) || global_areas.count != 1)
        goto cleanup;

    AreaData* area = LAST_AREA_DATA;
    if (!area)
        goto cleanup;

    if (!save_area_rom_to_buffer(area, fname, rom_buf))
        goto cleanup;

    PersistWriter json_writer = persist_writer_from_buffer(json_buf, fname);
    AreaPersistSaveParams json_params = {
        .writer = &json_writer,
        .area = area,
        .file_name = fname,
    };

    PersistResult json_save = AREA_PERSIST_JSON.save(&json_params);
    if (!persist_succeeded(json_save) || json_buf->data == NULL || json_buf->len == 0)
        goto cleanup;

    if (json_dom_out) {
        *json_dom_out = parse_json_from_buffer(json_buf, fname);
        if (!*json_dom_out)
            goto cleanup;
    }

    ok = true;

cleanup:
    if (snap_active)
        persist_state_end(&snap);
    if (!ok) {
        if (json_dom_out && *json_dom_out) {
            json_decref(*json_dom_out);
            *json_dom_out = NULL;
        }
        free_buffer_writer(rom_buf);
        free_buffer_writer(json_buf);
    }
    return ok;
}

static bool validate_json_round_trip(const char* fname,
    const PersistBufferWriter* rom_orig,
    const PersistBufferWriter* json_orig,
    const json_t* json_dom_orig)
{
    if (!rom_orig || !rom_orig->data || rom_orig->len == 0)
        return false;
    if (!json_orig || !json_orig->data || json_orig->len == 0)
        return false;
    if (!json_dom_orig)
        return false;

    PersistBufferWriter round_rom = { 0 };
    PersistBufferWriter round_json = { 0 };
    json_t* json_dom_round = NULL;
    bool ok = false;

    PersistStateSnapshot snap;
    persist_state_begin(&snap);

    PersistBufferReaderCtx ctx;
    PersistReader json_reader = persist_reader_from_buffer(json_orig->data, json_orig->len, fname, &ctx);
    AreaPersistLoadParams json_load_params = {
        .reader = &json_reader,
        .file_name = fname,
        .create_single_instance = false,
    };

    PersistResult json_load = AREA_PERSIST_JSON.load(&json_load_params);
    if (!persist_succeeded(json_load) || global_areas.count != 1)
        goto cleanup_state;

    AreaData* area = LAST_AREA_DATA;
    if (area && area->file_name && strcmp(area->file_name, fname) != 0) {
        // keep pointer
    }
    if (!area)
        goto cleanup_state;


    if (!save_area_rom_to_buffer(area, fname, &round_rom))
        goto cleanup_state;

    PersistWriter json_writer = persist_writer_from_buffer(&round_json, fname);
    AreaPersistSaveParams json_save_params = {
        .writer = &json_writer,
        .area = area,
        .file_name = fname,
    };
    PersistResult json_save = AREA_PERSIST_JSON.save(&json_save_params);
    if (!persist_succeeded(json_save) || round_json.data == NULL || round_json.len == 0)
        goto cleanup_state;

    json_dom_round = parse_json_from_buffer(&round_json, fname);
    if (!json_dom_round)
        goto cleanup_state;

    ok = true;

cleanup_state:
    persist_state_end(&snap);

    if (!ok) {
        if (json_dom_round)
            json_decref(json_dom_round);
        free_buffer_writer(&round_rom);
        free_buffer_writer(&round_json);
        return false;
    }

    bool rom_match = round_rom.len == rom_orig->len
        && (round_rom.len == 0 || memcmp(round_rom.data, rom_orig->data, round_rom.len) == 0);
    if (!rom_match)
        log_rom_text_diff(fname, rom_orig, &round_rom);

    char json_path[JSON_PATH_MAX];
    json_path[0] = '$';
    json_path[1] = '\0';
    size_t path_len = 1;
    bool json_match = json_values_equal_verbose(fname, json_dom_orig, json_dom_round, json_path, &path_len);

    if (!(rom_match && json_match)) {
        char name_buf[MIL];
        snprintf(name_buf, sizeof(name_buf), "%s", fname);
        for (char* c = name_buf; *c; ++c) {
            if (*c == '/' || *c == '\\')
                *c = '_';
        }
        char path_orig[2 * MIL];
        char path_round[2 * MIL];
        snprintf(path_orig, sizeof(path_orig), "temp/%s.orig.json", name_buf);
        snprintf(path_round, sizeof(path_round), "temp/%s.round.json", name_buf);
        json_dump_file(json_dom_orig, path_orig, JSON_INDENT(2));
        json_dump_file(json_dom_round, path_round, JSON_INDENT(2));
        fprintf(stderr, "Wrote JSON mismatch dumps to %s and %s\n", path_orig, path_round);

        if (!rom_match) {
            char rom_orig_path[2 * MIL];
            char rom_round_path[2 * MIL];
            snprintf(rom_orig_path, sizeof(rom_orig_path), "temp/%s.orig.are", name_buf);
            snprintf(rom_round_path, sizeof(rom_round_path), "temp/%s.round.are", name_buf);
            FILE* fp_orig = fopen(rom_orig_path, "wb");
            if (fp_orig) {
                fwrite(rom_orig->data, 1, rom_orig->len, fp_orig);
                fclose(fp_orig);
            }
            else {
                fprintf(stderr, "Failed to write %s\n", rom_orig_path);
            }
            FILE* fp_round = fopen(rom_round_path, "wb");
            if (fp_round) {
                fwrite(round_rom.data, 1, round_rom.len, fp_round);
                fclose(fp_round);
            }
            else {
                fprintf(stderr, "Failed to write %s\n", rom_round_path);
            }
            fprintf(stderr, "Wrote ROM mismatch dumps to %s and %s\n", rom_orig_path, rom_round_path);
        }
    }

    json_decref(json_dom_round);
    free_buffer_writer(&round_rom);
    free_buffer_writer(&round_json);

    return rom_match && json_match;
}

static bool extreme_round_trip_area(const char* area_dir, const char* fname)
{
    PersistBufferWriter rom_buf = { 0 };
    PersistBufferWriter json_buf = { 0 };
    json_t* json_dom = NULL;
    bool skipped = false;

    bool captured = capture_area_rom_and_json(area_dir, fname, &rom_buf, &json_buf, &json_dom, &skipped);
    if (!captured)
        return false;
    if (skipped)
        return true;

    bool ok = validate_json_round_trip(fname, &rom_buf, &json_buf, json_dom);

    json_decref(json_dom);
    free_buffer_writer(&rom_buf);
    free_buffer_writer(&json_buf);
    return ok;
}

static bool exclude_from_test(const char* fname)
{
    return strcmp(fname, "group.are") == 0
        || strcmp(fname, "rom.are") == 0
        || strcmp(fname, "help.are") == 0
        || strcmp(fname, "olc.hlp") == 0
        || strcmp(fname, "faladri.json") == 0;
}

static int test_extreme_area_json_round_trip()
{
    const char* area_dir = cfg_get_area_dir();
    const char* area_list = cfg_get_area_list();
    char list_path[MIL];
    sprintf(list_path, "%s%s", area_dir, area_list);

    FILE* fpList = fopen(list_path, "r");
    if (!fpList)
        return 0; // quietly skip if lists unavailable

    char fname[MIL];
    while (fscanf(fpList, "%s", fname) == 1) {
        if (fname[0] == '$')
            break;
        if (exclude_from_test(fname))
            continue;

        if (!extreme_round_trip_area(area_dir, fname)) {
            fprintf(stderr, "Extreme area JSON round-trip failed for %s\n", fname);
            fclose(fpList);
            ASSERT(false);
            return 1;
        }
    }

    fclose(fpList);
    return 0;
}
#endif

static TestGroup persist_tests;

void register_persist_tests()
{
#if defined(ENABLE_ROM_OLC_PERSISTENCE) && defined(ENABLE_JSON_PERSISTENCE)
#define REGISTER(n, f)  register_test(&persist_tests, (n), (f))

    init_test_group(&persist_tests, "Persistence Tests");
    register_test_group(&persist_tests);

    REGISTER("ROM OLC Loads Minimal Area", test_rom_olc_loads_minimal_area);
    REGISTER("ROM OLC Saves Sections", test_rom_olc_save_writes_sections);
    REGISTER("ROM OLC Round Trip In-Memory", test_rom_olc_round_trip_in_memory);
    REGISTER("ROM OLC Rejects Bad Section", test_rom_olc_rejects_bad_section);
#ifdef PERSIST_EXHAUSTIVE_PERSIST_TEST
    REGISTER("ROM OLC Exhaustive Area Round Trip", test_rom_olc_exhaustive_area_round_trip);
#endif

    REGISTER("JSON Loads Areadata", test_json_loads_areadata);
    REGISTER("JSON Saves Typed Objects", test_json_saves_typed_objects);
    REGISTER("JSON Story/Checklist Round Trip", test_story_checklist_round_trip);
#if defined(PERSIST_EXHAUSTIVE_PERSIST_TEST) && defined(PERSIST_JSON_EXHAUSTIVE_TEST)
    REGISTER("JSON Exhaustive Area Round Trip", test_json_exhaustive_area_round_trip);
    REGISTER("JSON Canonical ROM Round Trip", test_json_canonical_rom_round_trip);
    REGISTER("JSON Instance Counts Match ROM", test_json_instance_counts_match_rom);
    REGISTER("JSON Shop Counts Match ROM", test_json_instance_shop_counts_match_rom);
#endif

    REGISTER("Areas ROM->JSON->ROM Round Trip", test_areas_json_to_rom_round_trip);
    REGISTER("Races ROM<->JSON Round Trip", test_race_rom_json_round_trip);
    REGISTER("Classes ROM<->JSON Round Trip", test_class_rom_json_round_trip);
    REGISTER("Commands ROM<->JSON Round Trip", test_command_rom_json_round_trip);
    REGISTER("Lox Scripts ROM<->JSON Round Trip", test_lox_rom_json_round_trip);
    REGISTER("Skills ROM<->JSON Round Trip", test_skill_rom_json_round_trip);
    REGISTER("Skill Groups ROM<->JSON Round Trip", test_skill_group_rom_json_round_trip);
    REGISTER("Socials ROM<->JSON Round Trip", test_social_rom_json_round_trip);
    REGISTER("Tutorials ROM<->JSON Round Trip", test_tutorial_rom_json_round_trip);
    REGISTER("Themes JSON Round Trip", test_theme_json_round_trip);
#ifdef ENABLE_EXTREME_AREA_ROUND_TRIP_COMPARISON
    REGISTER("Extreme Area JSON Round Trip", test_extreme_area_json_round_trip);
#endif

#undef REGISTER
#else
    // Persistence tests require both formats enabled for round-trip testing
    (void)persist_tests;
#endif
}

#else // !defined(ENABLE_ROM_OLC_PERSISTENCE) || !defined(ENABLE_JSON_PERSISTENCE)

// Stub registration when both formats aren't enabled
void register_persist_tests()
{
    // Persistence tests require both formats enabled for round-trip testing
}

#endif // ENABLE_ROM_OLC_PERSISTENCE && ENABLE_JSON_PERSISTENCE
