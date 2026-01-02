////////////////////////////////////////////////////////////////////////////////
// persist/loot/loot_persist.c
// Selector for loot persistence (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#include "loot_persist.h"

#include <persist/persist_io_adapters.h>

#ifdef ENABLE_ROM_OLC_PERSISTENCE
#include <persist/loot/rom-olc/loot_persist_rom_olc.h>
#endif

#ifdef ENABLE_JSON_PERSISTENCE
#include <persist/loot/json/loot_persist_json.h>
#endif

#include <config.h>
#include <db.h>
#include <comm.h>

#include <stdio.h>
#include <string.h>

const LootPersistFormat* loot_persist_select_format(const char* filename)
{
    if (filename) {
        const char* ext = strrchr(filename, '.');
#ifdef ENABLE_JSON_PERSISTENCE
        if (ext && !str_cmp(ext, ".json"))
            return &LOOT_PERSIST_JSON;
#endif
    }
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return &LOOT_PERSIST_ROM_OLC;
#else
    return NULL;
#endif
}

PersistResult loot_persist_load(const char* filename)
{
    char path[MIL];
    sprintf(path, "%s%s", cfg_get_data_dir(), filename ? filename : cfg_get_loot_file());

    FILE* fp = fopen(path, "r");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "loot load: could not open file", -1 };

    PersistReader reader = persist_reader_from_file(fp, filename ? filename : path);
    const LootPersistFormat* fmt = loot_persist_select_format(filename ? filename : path);
    if (!fmt) {
        fclose(fp);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "loot load: no format available", -1 };
    }

    PersistResult res = fmt->load(&reader, filename ? filename : path, NULL);
    fclose(fp);
    return res;
}

PersistResult loot_persist_save(const char* filename)
{
    char path[MIL];
    sprintf(path, "%s%s", cfg_get_data_dir(), filename ? filename : cfg_get_loot_file());

    FILE* fp = fopen(path, "w");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "loot save: could not open file", -1 };

    PersistWriter writer = persist_writer_from_file(fp, filename ? filename : path);
    const LootPersistFormat* fmt = loot_persist_select_format(filename ? filename : path);
    if (!fmt) {
        fclose(fp);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "loot save: no format available", -1 };
    }

    PersistResult res = fmt->save(&writer, filename ? filename : path, NULL);
    fclose(fp);
    return res;
}

PersistResult loot_persist_load_section(const PersistReader* reader, Entity* owner)
{
    // Delegate to ROM-OLC format for now (area sections are always OLC format)
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return LOOT_PERSIST_ROM_OLC.load(reader, NULL, owner);
#else
    return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "loot section load: ROM-OLC not enabled", -1 };
#endif
}

PersistResult loot_persist_save_section(const PersistWriter* writer, Entity* owner)
{
    // Delegate to ROM-OLC format for now (area sections are always OLC format)
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return LOOT_PERSIST_ROM_OLC.save(writer, NULL, owner);
#else
    return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "loot section save: ROM-OLC not enabled", -1 };
#endif
}
