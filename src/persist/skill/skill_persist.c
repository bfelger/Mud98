////////////////////////////////////////////////////////////////////////////////
// persist/skill/skill_persist.c
// Selectors for skill and skill group persistence (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#include "skill_persist.h"

#include <persist/persist_io_adapters.h>

#ifdef ENABLE_ROM_OLC_PERSISTENCE
#include <persist/skill/rom-olc/skill_persist_rom_olc.h>
#endif

#ifdef ENABLE_JSON_PERSISTENCE
#include <persist/skill/json/skill_persist_json.h>
#endif

#include <config.h>
#include <db.h>

#include <stdio.h>
#include <string.h>

static const SkillPersistFormat* skill_format_from_name(const char* filename)
{
    if (filename) {
        const char* ext = strrchr(filename, '.');
#ifdef ENABLE_JSON_PERSISTENCE
        if (ext && !str_cmp(ext, ".json"))
            return &SKILL_PERSIST_JSON;
#endif
    }
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return &SKILL_PERSIST_ROM_OLC;
#else
    return NULL;
#endif
}

static const SkillGroupPersistFormat* skill_group_format_from_name(const char* filename)
{
    if (filename) {
        const char* ext = strrchr(filename, '.');
#ifdef ENABLE_JSON_PERSISTENCE
        if (ext && !str_cmp(ext, ".json"))
            return &SKILL_GROUP_PERSIST_JSON;
#endif
    }
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return &SKILL_GROUP_PERSIST_ROM_OLC;
#else
    return NULL;
#endif
}

static PersistResult persist_load_common(const char* fname, const char* default_name,
    const SkillPersistFormat* fmt)
{
    char path[MIL];
    const char* file = fname ? fname : default_name;
    sprintf(path, "%s%s", cfg_get_data_dir(), file);
    FILE* fp = fopen(path, "r");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "skill load: could not open file", -1 };
    PersistReader reader = persist_reader_from_file(fp, file ? file : path);
    PersistResult res = fmt->load(&reader, file ? file : path);
    fclose(fp);
    return res;
}

static PersistResult persist_save_common(const char* fname, const char* default_name,
    const SkillPersistFormat* fmt)
{
    char path[MIL];
    const char* file = fname ? fname : default_name;
    sprintf(path, "%s%s", cfg_get_data_dir(), file);
    FILE* fp = fopen(path, "w");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "skill save: could not open file", -1 };
    PersistWriter writer = persist_writer_from_file(fp, file ? file : path);
    PersistResult res = fmt->save(&writer, file ? file : path);
    fclose(fp);
    return res;
}

PersistResult skill_persist_load(const char* filename)
{
    const SkillPersistFormat* fmt = skill_format_from_name(filename ? filename : cfg_get_skills_file());
    return persist_load_common(filename, cfg_get_skills_file(), fmt);
}

PersistResult skill_persist_save(const char* filename)
{
    const SkillPersistFormat* fmt = skill_format_from_name(filename ? filename : cfg_get_skills_file());
    return persist_save_common(filename, cfg_get_skills_file(), fmt);
}

PersistResult skill_group_persist_load(const char* filename)
{
    const SkillGroupPersistFormat* fmt = skill_group_format_from_name(filename ? filename : cfg_get_groups_file());
    char path[MIL];
    const char* file = filename ? filename : cfg_get_groups_file();
    sprintf(path, "%s%s", cfg_get_data_dir(), file);
    FILE* fp = fopen(path, "r");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "skill group load: could not open file", -1 };
    PersistReader reader = persist_reader_from_file(fp, file ? file : path);
    PersistResult res = fmt->load(&reader, file ? file : path);
    fclose(fp);
    return res;
}

PersistResult skill_group_persist_save(const char* filename)
{
    const SkillGroupPersistFormat* fmt = skill_group_format_from_name(filename ? filename : cfg_get_groups_file());
    char path[MIL];
    const char* file = filename ? filename : cfg_get_groups_file();
    sprintf(path, "%s%s", cfg_get_data_dir(), file);
    FILE* fp = fopen(path, "w");
    if (!fp)
        return (PersistResult){ PERSIST_ERR_IO, "skill group save: could not open file", -1 };
    PersistWriter writer = persist_writer_from_file(fp, file ? file : path);
    PersistResult res = fmt->save(&writer, file ? file : path);
    fclose(fp);
    return res;
}
