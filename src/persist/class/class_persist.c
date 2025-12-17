////////////////////////////////////////////////////////////////////////////////
// persist/class/class_persist.c
// Selector for class persistence (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#include "class_persist.h"

#include <persist/persist_io_adapters.h>

#ifdef ENABLE_ROM_OLC_PERSISTENCE
#include <persist/class/rom-olc/class_persist_rom_olc.h>
#endif

#ifdef ENABLE_JSON_PERSISTENCE
#include <persist/class/json/class_persist_json.h>
#endif

#include <config.h>
#include <db.h>
#include <stdio.h>
#include <string.h>

static const ClassPersistFormat* class_format_from_name(const char* filename)
{
    if (filename) {
        const char* ext = strrchr(filename, '.');
#ifdef ENABLE_JSON_PERSISTENCE
        if (ext && !str_cmp(ext, ".json"))
            return &CLASS_PERSIST_JSON;
#endif
    }
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return &CLASS_PERSIST_ROM_OLC;
#else
    return NULL;
#endif
}

PersistResult class_persist_load(const char* filename)
{
    char path[MIL];
    sprintf(path, "%s%s", cfg_get_data_dir(), filename ? filename : cfg_get_classes_file());
    FILE* fp = fopen(path, "r");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "class load: could not open file", -1 };
    PersistReader reader = persist_reader_from_file(fp, filename ? filename : path);
    const ClassPersistFormat* fmt = class_format_from_name(filename ? filename : path);
    PersistResult res = fmt->load(&reader, filename ? filename : path);
    fclose(fp);
    return res;
}

PersistResult class_persist_save(const char* filename)
{
    char path[MIL];
    sprintf(path, "%s%s", cfg_get_data_dir(), filename ? filename : cfg_get_classes_file());
    FILE* fp = fopen(path, "w");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "class save: could not open file", -1 };
    PersistWriter writer = persist_writer_from_file(fp, filename ? filename : path);
    const ClassPersistFormat* fmt = class_format_from_name(filename ? filename : path);
    PersistResult res = fmt->save(&writer, filename ? filename : path);
    fclose(fp);
    return res;
}
