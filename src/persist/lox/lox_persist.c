////////////////////////////////////////////////////////////////////////////////
// persist/lox/lox_persist.c
// Selector for Lox script catalog persistence.
////////////////////////////////////////////////////////////////////////////////

#include "lox_persist.h"

#include <persist/persist_io_adapters.h>
#include <persist/lox/rom-olc/lox_persist_rom_olc.h>
#include <persist/lox/json/lox_persist_json.h>

#include <merc.h>
#include <db.h>
#include <config.h>
#include <stringutils.h>

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <ctype.h>
#endif

static bool path_is_absolute(const char* path)
{
    if (!path || !path[0])
        return false;

    if (path[0] == '/' || path[0] == '\\')
        return true;

#ifdef _WIN32
    if (isalpha((unsigned char)path[0]) && path[1] == ':' &&
        (path[2] == '\\' || path[2] == '/'))
        return true;
#endif

    return false;
}

static void build_catalog_path(char* out, size_t out_len, const char* override_filename)
{
    const char* catalog = (override_filename && override_filename[0]) ? override_filename : cfg_get_lox_file();
    if (!catalog || !catalog[0])
        catalog = "lox.olc";

    if (path_is_absolute(catalog)) {
        snprintf(out, out_len, "%s", catalog);
    }
    else if (strpbrk(catalog, "/\\")) {
        snprintf(out, out_len, "%s%s", cfg_get_data_dir(), catalog);
    }
    else {
        snprintf(out, out_len, "%s%s%s",
            cfg_get_data_dir(), cfg_get_scripts_dir(), catalog);
    }
}

static const LoxPersistFormat* lox_format_from_name(const char* filename)
{
    const char* ext = filename ? strrchr(filename, '.') : NULL;
    if (ext && !str_cmp(ext, ".json"))
        return &LOX_PERSIST_JSON;
    return &LOX_PERSIST_ROM_OLC;
}

PersistResult lox_persist_load(const char* filename)
{
    char path[MIL];
    build_catalog_path(path, sizeof(path), filename);
    FILE* fp = fopen(path, "r");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "lox load: could not open catalog", -1 };

    PersistReader reader = persist_reader_from_file(fp, path);
    const LoxPersistFormat* fmt = lox_format_from_name(filename ? filename : path);
    PersistResult res = fmt->load(&reader, path);
    fclose(fp);
    return res;
}

PersistResult lox_persist_save(const char* filename)
{
    char path[MIL];
    build_catalog_path(path, sizeof(path), filename);
    FILE* fp = fopen(path, "w");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "lox save: could not open catalog", -1 };

    PersistWriter writer = persist_writer_from_file(fp, path);
    const LoxPersistFormat* fmt = lox_format_from_name(filename ? filename : path);
    PersistResult res = fmt->save(&writer, path);
    if (writer.ops && writer.ops->flush)
        writer.ops->flush(writer.ctx);
    fclose(fp);
    return res;
}
