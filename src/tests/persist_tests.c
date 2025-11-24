///////////////////////////////////////////////////////////////////////////////
// persist_tests.c
///////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"

#include <persist/area_persist.h>
#include <persist/persist_io_adapters.h>
#include <persist/persist_result.h>
#include <persist/rom-olc/area_persist_rom_olc.h>

#include <db.h>
#include <entities/area.h>
#include <entities/faction.h>
#include <entities/mob_prototype.h>
#include <entities/obj_prototype.h>
#include <entities/room.h>

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

#undef REGISTER
}
