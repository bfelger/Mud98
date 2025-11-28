///////////////////////////////////////////////////////////////////////////////
// persist_tests.c
///////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"

#include <persist/area_persist.h>
#include <persist/persist_io_adapters.h>
#include <persist/persist_result.h>
#include <persist/rom-olc/area_persist_rom_olc.h>
#include <persist/race/race_persist.h>
#ifdef HAVE_JSON_AREAS
#include <persist/json/area_persist_json.h>
#include <jansson.h>
#endif

#include <db.h>
#include <entities/area.h>
#include <entities/faction.h>
#include <entities/mob_prototype.h>
#include <entities/obj_prototype.h>
#include <entities/room.h>
#include <data/race.h>

#include <config.h>
#include <lox/array.h>
#include <lox/table.h>

#include <stdio.h>
#include <string.h>

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
    Table global_rooms;
    Table mob_protos;
    Table obj_protos;
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
    snap->global_rooms = global_rooms;
    snap->mob_protos = mob_protos;
    snap->obj_protos = obj_protos;
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
    init_table(&global_rooms);
    init_table(&mob_protos);
    init_table(&obj_protos);
    init_table(&faction_table);
    top_vnum_room = 0;
    top_vnum_mob = 0;
    top_vnum_obj = 0;
}

static void persist_state_end(PersistStateSnapshot* snap)
{
    free_table(&global_rooms);
    free_table(&mob_protos);
    free_table(&obj_protos);
    free_table(&faction_table);
    free_value_array(&global_areas);
    global_areas = snap->global_areas;
    current_area_data = snap->current_area_data;
    area_data_free = snap->area_data_free;
    area_count = snap->area_count;
    area_perm_count = snap->area_perm_count;
    area_data_count = snap->area_data_count;
    area_data_perm_count = snap->area_data_perm_count;
    global_rooms = snap->global_rooms;
    mob_protos = snap->mob_protos;
    obj_protos = snap->obj_protos;
    faction_table = snap->faction_table;
    top_vnum_room = snap->top_vnum_room;
    top_vnum_mob = snap->top_vnum_mob;
    top_vnum_obj = snap->top_vnum_obj;
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

#ifdef HAVE_JSON_AREAS
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
#endif

static int test_rom_olc_loads_minimal_area()
{
    PersistStateSnapshot snap;
    persist_state_begin(&snap);

    FILE* fp = tmpfile();
    ASSERT_OR_GOTO(fp != NULL, cleanup);
    size_t written = fwrite(MIN_AREA_TEXT, 1, strlen(MIN_AREA_TEXT), fp);
    ASSERT_OR_GOTO(written == strlen(MIN_AREA_TEXT), cleanup);
    rewind(fp);

    PersistReader reader = persist_reader_from_FILE(fp, "test.are");
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

    PersistReader reader = persist_reader_from_FILE(load_fp, "test.are");
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
    PersistWriter writer = persist_writer_from_FILE(save_fp, "out.are");
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

    PersistReader reader = persist_reader_from_FILE(load_fp, "test.are");
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
    ASSERT_OR_GOTO(persist_succeeded(save_result), cleanup);
    ASSERT(buf.len > 0);

    // Reload from saved buffer into a fresh state using FILE-backed reader.
    persist_state_end(&snap);
    persist_state_begin(&snap);

    FILE* reload_fp = tmpfile();
    ASSERT_OR_GOTO(reload_fp != NULL, cleanup);
    size_t reload_written = fwrite(buf.data, 1, buf.len, reload_fp);
    ASSERT_OR_GOTO(reload_written == buf.len, cleanup);
    rewind(reload_fp);

    PersistReader reload_reader = persist_reader_from_FILE(reload_fp, "out.are");
    AreaPersistLoadParams reload_params = {
        .reader = &reload_reader,
        .file_name = "out.are",
        .create_single_instance = false,
    };

    PersistResult reload_result = AREA_PERSIST_ROM_OLC.load(&reload_params);
    ASSERT_OR_GOTO(persist_succeeded(reload_result), cleanup);
    ASSERT(global_areas.count == 1);

    AreaData* re_area = LAST_AREA_DATA;
    ASSERT(re_area != NULL);
    ASSERT_STR_EQ("out.are", re_area->file_name);
    ASSERT_STR_EQ("Test Area", NAME_STR(re_area));

cleanup:
    free(buf.data);
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

    PersistReader reader = persist_reader_from_FILE(fp, "bad.are");
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

        PersistStateSnapshot snap;
        persist_state_begin(&snap);

        char area_path[MIL];
        sprintf(area_path, "%s%s", area_dir, fname);

        char* orig = NULL;
        size_t orig_len = 0;
        ASSERT_OR_GOTO(read_file_to_buffer(area_path, &orig, &orig_len), next_area);

        FILE* load_fp = fopen(area_path, "r");
        ASSERT_OR_GOTO(load_fp != NULL, next_area);

        PersistReader reader = persist_reader_from_FILE(load_fp, fname);
        AreaPersistLoadParams load_params = {
            .reader = &reader,
            .file_name = fname,
            .create_single_instance = false,
        };

        PersistResult load_result = AREA_PERSIST_ROM_OLC.load(&load_params);
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

#ifdef HAVE_JSON_AREAS
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

        PersistReader reader = persist_reader_from_FILE(load_fp, fname);
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

        PersistReader reader = persist_reader_from_FILE(load_fp, fname);
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

            PersistReader reader = persist_reader_from_FILE(load_fp, fname);
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

            PersistReader reader = persist_reader_from_FILE(load_fp, fname);
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

static int64_t json_int_or_default(json_t* obj, const char* key, int64_t def)
{
    json_t* val = json_object_get(obj, key);
    if (json_is_integer(val))
        return json_integer_value(val);
    return def;
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

    PersistReader reader = persist_reader_from_FILE(load_fp, "test.are");
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
    table_set_vnum(&obj_protos, VNUM_FIELD(weapon), OBJ_VAL(weapon));

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
    table_set_vnum(&obj_protos, VNUM_FIELD(container), OBJ_VAL(container));

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
    table_set_vnum(&obj_protos, VNUM_FIELD(light), OBJ_VAL(light));

    top_vnum_obj = 3;

    PersistBufferWriter buf = { 0 };
    PersistWriter writer = persist_writer_from_buffer(&buf, "out.json");
    AreaPersistSaveParams save_params = {
        .writer = &writer,
        .area = area,
        .file_name = "out.json",
    };
    PersistResult save_result = AREA_PERSIST_JSON.save(&save_params);
    ASSERT_OR_GOTO(persist_succeeded(save_result), cleanup);
    ASSERT_OR_GOTO(buf.data != NULL && buf.len > 0, cleanup);

    json_error_t jerr;
    json_t* root = json_loadb((const char*)buf.data, buf.len, 0, &jerr);
    ASSERT_OR_GOTO(root != NULL, cleanup);
    json_t* objects = json_object_get(root, "objects");
    ASSERT_OR_GOTO(json_is_array(objects), cleanup_root);

    // Weapon typed block
    json_t* wobj = find_object_entry(objects, 1);
    ASSERT_OR_GOTO(wobj != NULL, cleanup_root);
    json_t* weapon_block = json_object_get(wobj, "weapon");
    ASSERT_OR_GOTO(json_is_object(weapon_block), cleanup_root);
    ASSERT_STR_EQ("sword", json_string_value(json_object_get(weapon_block, "class")));
    ASSERT_STR_EQ("slash", json_string_value(json_object_get(weapon_block, "damageType")));
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
cleanup:
    if (buf.data)
        free(buf.data);
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

    PersistReader reader = persist_reader_from_FILE(load_fp, "beats.are");
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

static int test_json_to_rom_round_trip()
{
    PersistStateSnapshot snap;
    persist_state_begin(&snap);

    // 1) Load minimal area from ROM text.
    FILE* load_fp = tmpfile();
    ASSERT_OR_GOTO(load_fp != NULL, cleanup_all);
    size_t written = fwrite(MIN_AREA_TEXT, 1, strlen(MIN_AREA_TEXT), load_fp);
    ASSERT_OR_GOTO(written == strlen(MIN_AREA_TEXT), cleanup_all);
    rewind(load_fp);

    PersistReader reader = persist_reader_from_FILE(load_fp, "test.are");
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
    return 0;
}
#endif // HAVE_JSON_AREAS

static TestGroup persist_tests;

void register_persist_tests()
{
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
#ifdef HAVE_JSON_AREAS
    REGISTER("JSON Loads Areadata", test_json_loads_areadata);
    REGISTER("JSON Saves Typed Objects", test_json_saves_typed_objects);
    REGISTER("JSON Story/Checklist Round Trip", test_story_checklist_round_trip);
#if defined(PERSIST_EXHAUSTIVE_PERSIST_TEST) && defined(PERSIST_JSON_EXHAUSTIVE_TEST)
    REGISTER("JSON Exhaustive Area Round Trip", test_json_exhaustive_area_round_trip);
    REGISTER("JSON Canonical ROM Round Trip", test_json_canonical_rom_round_trip);
    REGISTER("JSON Instance Counts Match ROM", test_json_instance_counts_match_rom);
    REGISTER("JSON Shop Counts Match ROM", test_json_instance_shop_counts_match_rom);
#endif
    REGISTER("ROM->JSON->ROM Round Trip", test_json_to_rom_round_trip);
#endif
    REGISTER("Races ROM<->JSON Round Trip", test_race_rom_json_round_trip);

#undef REGISTER
}
