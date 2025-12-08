////////////////////////////////////////////////////////////////////////////////
// persist/social/social_persist.c
////////////////////////////////////////////////////////////////////////////////

#include "social_persist.h"

#include <persist/persist_io_adapters.h>
#include <persist/social/rom-olc/social_persist_rom_olc.h>
#include <persist/social/json/social_persist_json.h>

#include <config.h>
#include <db.h>

#include <stdio.h>
#include <string.h>

static const SocialPersistFormat* social_format_from_name(const char* filename)
{
    if (filename) {
        const char* ext = strrchr(filename, '.');
        if (ext && !str_cmp(ext, ".json"))
            return &SOCIAL_PERSIST_JSON;
    }
    return &SOCIAL_PERSIST_ROM_OLC;
}

PersistResult social_persist_load(const char* filename)
{
    char path[MIL];
    const char* fname = filename ? filename : cfg_get_socials_file();
    sprintf(path, "%s%s", cfg_get_data_dir(), fname);
    FILE* fp = fopen(path, "r");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "social load: could not open file", -1 };
    PersistReader reader = persist_reader_from_file(fp, fname ? fname : path);
    const SocialPersistFormat* fmt = social_format_from_name(fname ? fname : path);
    PersistResult res = fmt->load(&reader, fname ? fname : path);
    fclose(fp);
    return res;
}

PersistResult social_persist_save(const char* filename)
{
    char path[MIL];
    const char* fname = filename ? filename : cfg_get_socials_file();
    sprintf(path, "%s%s", cfg_get_data_dir(), fname);
    FILE* fp = fopen(path, "w");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "social save: could not open file", -1 };
    PersistWriter writer = persist_writer_from_file(fp, fname ? fname : path);
    const SocialPersistFormat* fmt = social_format_from_name(fname ? fname : path);
    PersistResult res = fmt->save(&writer, fname ? fname : path);
    fclose(fp);
    return res;
}
