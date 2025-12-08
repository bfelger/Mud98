////////////////////////////////////////////////////////////////////////////////
// persist/tutorial/tutorial_persist.c
////////////////////////////////////////////////////////////////////////////////

#include "tutorial_persist.h"

#include <persist/persist_io_adapters.h>
#include <persist/tutorial/rom-olc/tutorial_persist_rom_olc.h>
#include <persist/tutorial/json/tutorial_persist_json.h>

#include <config.h>
#include <db.h>

#include <stdio.h>
#include <string.h>

static const TutorialPersistFormat* tutorial_format_from_name(const char* filename)
{
    if (filename) {
        const char* ext = strrchr(filename, '.');
        if (ext && !str_cmp(ext, ".json"))
            return &TUTORIAL_PERSIST_JSON;
    }
    return &TUTORIAL_PERSIST_ROM_OLC;
}

PersistResult tutorial_persist_load(const char* filename)
{
    char path[MIL];
    const char* fname = filename ? filename : cfg_get_tutorials_file();
    sprintf(path, "%s%s", cfg_get_data_dir(), fname);
    FILE* fp = fopen(path, "r");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "tutorial load: could not open file", -1 };
    PersistReader reader = persist_reader_from_file(fp, fname ? fname : path);
    const TutorialPersistFormat* fmt = tutorial_format_from_name(fname ? fname : path);
    PersistResult res = fmt->load(&reader, fname ? fname : path);
    fclose(fp);
    return res;
}

PersistResult tutorial_persist_save(const char* filename)
{
    char path[MIL];
    const char* fname = filename ? filename : cfg_get_tutorials_file();
    sprintf(path, "%s%s", cfg_get_data_dir(), fname);
    FILE* fp = fopen(path, "w");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "tutorial save: could not open file", -1 };
    PersistWriter writer = persist_writer_from_file(fp, fname ? fname : path);
    const TutorialPersistFormat* fmt = tutorial_format_from_name(fname ? fname : path);
    PersistResult res = fmt->save(&writer, fname ? fname : path);
    fclose(fp);
    return res;
}
