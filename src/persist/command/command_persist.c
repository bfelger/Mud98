////////////////////////////////////////////////////////////////////////////////
// persist/command/command_persist.c
// Selector for command persistence (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#include "command_persist.h"

#include <persist/persist_io_adapters.h>

#ifdef ENABLE_ROM_OLC_PERSISTENCE
#include <persist/command/rom-olc/command_persist_rom_olc.h>
#endif

#ifdef ENABLE_JSON_PERSISTENCE
#include <persist/command/json/command_persist_json.h>
#endif

#include <comm.h>
#include <config.h>
#include <db.h>

#include <stdio.h>
#include <string.h>

extern bool test_output_enabled;

void load_command_table()
{
    PersistResult cmd_res = command_persist_load(cfg_get_commands_file());
    if (!persist_succeeded(cmd_res)) {
        bugf("load_command_table: failed to load commands (%s)",
            cmd_res.message ? cmd_res.message : "unknown error");
        exit(1);
    }
 
    if (!test_output_enabled)
        printf_log("Command table loaded (%d commands).", max_cmd);
}

static const CommandPersistFormat* command_format_from_name(const char* filename)
{
    if (filename) {
        const char* ext = strrchr(filename, '.');
#ifdef ENABLE_JSON_PERSISTENCE
        if (ext && !str_cmp(ext, ".json"))
            return &COMMAND_PERSIST_JSON;
#endif
    }
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return &COMMAND_PERSIST_ROM_OLC;
#else
    return NULL;
#endif
}

PersistResult command_persist_load(const char* filename)
{
    char path[MIL];
    const char* fname = filename ? filename : cfg_get_commands_file();
    sprintf(path, "%s%s", cfg_get_data_dir(), fname);
    FILE* fp = fopen(path, "r");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "command load: could not open file", -1 };
    PersistReader reader = persist_reader_from_file(fp, fname ? fname : path);
    const CommandPersistFormat* fmt = command_format_from_name(fname ? fname : path);
    PersistResult res = fmt->load(&reader, fname ? fname : path);
    fclose(fp);

    return res;
}

PersistResult command_persist_save(const char* filename)
{
    char path[MIL];
    const char* fname = filename ? filename : cfg_get_commands_file();
    sprintf(path, "%s%s", cfg_get_data_dir(), fname);
    FILE* fp = fopen(path, "w");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "command save: could not open file", -1 };
    PersistWriter writer = persist_writer_from_file(fp, fname ? fname : path);
    const CommandPersistFormat* fmt = command_format_from_name(fname ? fname : path);
    PersistResult res = fmt->save(&writer, fname ? fname : path);
    fclose(fp);
    return res;
}
