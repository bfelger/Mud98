////////////////////////////////////////////////////////////////////////////////
// persist/recipe/recipe_persist.c
// Selector for recipe persistence (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#include "recipe_persist.h"

#include <persist/persist_io_adapters.h>

#ifdef ENABLE_ROM_OLC_PERSISTENCE
#include <persist/recipe/rom-olc/recipe_persist_rom_olc.h>
#endif

#ifdef ENABLE_JSON_PERSISTENCE
#include <persist/recipe/json/recipe_persist_json.h>
#endif

#include <config.h>
#include <db.h>
#include <comm.h>

#include <stdio.h>
#include <string.h>

const RecipePersistFormat* recipe_persist_select_format(const char* filename)
{
    if (filename) {
        const char* ext = strrchr(filename, '.');
#ifdef ENABLE_JSON_PERSISTENCE
        if (ext && !str_cmp(ext, ".json"))
            return &RECIPE_PERSIST_JSON;
#endif
    }
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return &RECIPE_PERSIST_ROM_OLC;
#else
    return NULL;
#endif
}

PersistResult recipe_persist_load(const char* filename)
{
    // TODO: Implement once config entries for recipe file are added
    (void)filename;
    return (PersistResult){ PERSIST_OK, NULL, 0 };
}

PersistResult recipe_persist_save(const char* filename)
{
    // TODO: Implement once config entries for recipe file are added
    (void)filename;
    return (PersistResult){ PERSIST_OK, NULL, 0 };
}

PersistResult recipe_persist_load_section(const PersistReader* reader, Entity* owner)
{
    // Delegate to ROM-OLC format for now (area sections are always OLC format)
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return RECIPE_PERSIST_ROM_OLC.load(reader, NULL, owner);
#else
    (void)reader;
    (void)owner;
    return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "recipe section load: ROM-OLC not enabled", -1 };
#endif
}

PersistResult recipe_persist_save_section(const PersistWriter* writer, Entity* owner)
{
    // Delegate to ROM-OLC format for now (area sections are always OLC format)
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return RECIPE_PERSIST_ROM_OLC.save(writer, NULL, owner);
#else
    (void)writer;
    (void)owner;
    return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "recipe section save: ROM-OLC not enabled", -1 };
#endif
}
