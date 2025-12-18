////////////////////////////////////////////////////////////////////////////////
// persist/race/race_persist.c
// Selector for race persistence (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#include "race_persist.h"

#include <persist/persist_io_adapters.h>

#ifdef ENABLE_ROM_OLC_PERSISTENCE
#include <persist/race/rom-olc/race_persist_rom_olc.h>
#endif

#ifdef ENABLE_JSON_PERSISTENCE
#include <persist/race/json/race_persist_json.h>
#endif

#include <config.h>
#include <db.h>
#include <stdio.h>
#include <string.h>

static const RacePersistFormat* race_format_from_name(const char* filename)
{
    if (filename) {
        const char* ext = strrchr(filename, '.');
#ifdef ENABLE_JSON_PERSISTENCE
        if (ext && !str_cmp(ext, ".json"))
            return &RACE_PERSIST_JSON;
#endif
    }
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return &RACE_PERSIST_ROM_OLC;
#else
    return NULL;
#endif
}

PersistResult race_persist_load(const char* filename)
{
    char path[MIL];
    sprintf(path, "%s%s", cfg_get_data_dir(), filename ? filename : cfg_get_races_file());
    FILE* fp = fopen(path, "r");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "race load: could not open file", -1 };
    PersistReader reader = persist_reader_from_file(fp, filename ? filename : path);
    const RacePersistFormat* fmt = race_format_from_name(filename ? filename : path);
    PersistResult res = fmt->load(&reader, filename ? filename : path);
    fclose(fp);
    return res;
}

PersistResult race_persist_save(const char* filename)
{
    char path[MIL];
    sprintf(path, "%s%s", cfg_get_data_dir(), filename ? filename : cfg_get_races_file());
    FILE* fp = fopen(path, "w");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "race save: could not open file", -1 };
    PersistWriter writer = persist_writer_from_file(fp, filename ? filename : path);
    const RacePersistFormat* fmt = race_format_from_name(filename ? filename : path);
    PersistResult res = fmt->save(&writer, filename ? filename : path);
    fclose(fp);
    return res;
}
