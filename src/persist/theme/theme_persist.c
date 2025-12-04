////////////////////////////////////////////////////////////////////////////////
// persist/theme/theme_persist.c
// Selector for theme persistence (ROM-OLC text).
////////////////////////////////////////////////////////////////////////////////

#include "theme_persist.h"

#include <persist/persist_io_adapters.h>
#include <persist/theme/rom-olc/theme_persist_rom_olc.h>

#include <config.h>
#include <db.h>

#include <stdio.h>
#include <string.h>

static const ThemePersistFormat* theme_format_from_name(const char* filename)
{
    (void)filename;
    return &THEME_PERSIST_ROM_OLC;
}

PersistResult theme_persist_load(const char* filename)
{
    const char* cfg_name = filename ? filename : cfg_get_themes_file();
    char path[MIL];
    sprintf(path, "%s%s", cfg_get_data_dir(), cfg_name);
    FILE* fp = fopen(path, "r");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "theme load: could not open file", -1 };
    PersistReader reader = persist_reader_from_file(fp, filename ? filename : path);
    const ThemePersistFormat* fmt = theme_format_from_name(filename ? filename : path);
    PersistResult res = fmt->load(&reader, filename ? filename : path);
    fclose(fp);
    return res;
}

PersistResult theme_persist_save(const char* filename)
{
    const char* cfg_name = filename ? filename : cfg_get_themes_file();
    char path[MIL];
    sprintf(path, "%s%s", cfg_get_data_dir(), cfg_name);
    FILE* fp = fopen(path, "w");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "theme save: could not open file", -1 };
    PersistWriter writer = persist_writer_from_file(fp, filename ? filename : path);
    const ThemePersistFormat* fmt = theme_format_from_name(filename ? filename : path);
    PersistResult res = fmt->save(&writer, filename ? filename : path);
    fclose(fp);
    return res;
}
