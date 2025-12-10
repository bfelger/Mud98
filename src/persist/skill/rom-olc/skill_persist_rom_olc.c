////////////////////////////////////////////////////////////////////////////////
// persist/skill/rom-olc/skill_persist_rom_olc.c
////////////////////////////////////////////////////////////////////////////////

#include "skill_persist_rom_olc.h"

#include <persist/persist_io_adapters.h>

#include <tablesave.h>

#include <comm.h>
#include <db.h>
#include <lookup.h>

#include <data/skill.h>
#include <data/spell.h>

#include <tables.h>

#include <stdlib.h>

#ifdef U
#undef U
#endif
#define U(x) (uintptr_t)(x)

extern bool test_output_enabled;

static Skill tmp_sk;
static SkillGroup tmp_grp;

static bool pgsn_read(void* temp, char* arg);
static char* pgsn_str(void* temp);

static const SaveTableEntry skill_save_table[] = {
    { "name",            FIELD_STRING,                U(&tmp_sk.name),               0,                  0 },
    { "skill_level",     FIELD_LEVEL_DYNARRAY,        U(&tmp_sk.skill_level),        0,                  0 },
    { "rating",          FIELD_RATING_DYNARRAY,       U(&tmp_sk.rating),             0,                  0 },
    { "spell_fun",       FIELD_FUNCTION_INT_TO_STR,   U(&tmp_sk.spell_fun),          U(spell_fun_str),   U(spell_fun_read) },
    { "lox_spell",       FIELD_LOX_CLOSURE,           U(&tmp_sk.lox_spell_name),     U(&tmp_sk.lox_closure), 0 },
    { "target",          FIELD_INT16_FLAGSTRING,      U(&tmp_sk.target),             U(target_table),    0 },
    { "minimum_position",FIELD_FUNCTION_INT16_TO_STR, U(&tmp_sk.minimum_position),   U(position_str),    U(position_read) },
    { "pgsn",            FIELD_FUNCTION_INT_TO_STR,   U(&tmp_sk.pgsn),               U(pgsn_str),        U(pgsn_read) },
    { "slot",            FIELD_INT,                   U(&tmp_sk.slot),               0,                  0 },
    { "min_mana",        FIELD_INT,                   U(&tmp_sk.min_mana),           0,                  0 },
    { "beats",           FIELD_INT,                   U(&tmp_sk.beats),              0,                  0 },
    { "noun_damage",     FIELD_STRING,                U(&tmp_sk.noun_damage),        0,                  0 },
    { "msg_off",         FIELD_STRING,                U(&tmp_sk.msg_off),            0,                  0 },
    { "msg_obj",         FIELD_STRING,                U(&tmp_sk.msg_obj),            0,                  0 },
    { NULL,              0,                           0,                             0,                  0 }
};

static const SaveTableEntry skill_group_save_table[] = {
    { "name",            FIELD_STRING,                U(&tmp_grp.name),              0,                  0 },
    { "rating",          FIELD_RATING_DYNARRAY,       U(&tmp_grp.rating),            0,                  0 },
    { "skills",          FIELD_STRING_ARRAY,          U(&tmp_grp.skills),            U(MAX_IN_GROUP),    0 },
    { NULL,              0,                           0,                             0,                  0 }
};

const SkillPersistFormat SKILL_PERSIST_ROM_OLC = {
    .name = "rom-olc",
    .load = skill_persist_rom_olc_load,
    .save = skill_persist_rom_olc_save,
};

const SkillGroupPersistFormat SKILL_GROUP_PERSIST_ROM_OLC = {
    .name = "rom-olc",
    .load = skill_group_persist_rom_olc_load,
    .save = skill_group_persist_rom_olc_save,
};

static bool ensure_skill_capacity(int size)
{
    extern Skill* skill_table;
    extern int skill_count;
    if (skill_table)
        free(skill_table);
    skill_count = size;

    skill_table = calloc((size_t)skill_count + 1, sizeof(Skill));
    if (!skill_table)
        return false;
    return true;
}

static bool ensure_group_capacity(int size)
{
    extern SkillGroup* skill_group_table;
    extern int skill_group_count;
    if (skill_group_table)
        free(skill_group_table);
    skill_group_count = size;

    skill_group_table = calloc((size_t)skill_group_count + 1, sizeof(SkillGroup));
    if (!skill_group_table)
        return false;
    return true;
}

static bool skill_has_name(const Skill* skill)
{
    return skill && skill->name && skill->name[0] != '\0';
}

static bool skill_group_has_name(const SkillGroup* group)
{
    return group && group->name && group->name[0] != '\0';
}

PersistResult skill_group_persist_rom_olc_load(const PersistReader* reader, const char* filename)
{
    if (!reader || reader->ops != &PERSIST_FILE_STREAM_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "skill group ROM load requires FILE reader", -1 };
    FILE* fp = (FILE*)reader->ctx;
    extern SkillGroup* skill_group_table;
    extern int skill_group_count;
    static SkillGroup grzero;
    char* word;

    int size = fread_number(fp);
    if (!ensure_group_capacity(size))
        return (PersistResult){ PERSIST_ERR_INTERNAL, "skill group ROM load: alloc failed", -1 };

    if (!test_output_enabled)
        printf_log("Creating group table of length %d, size %zu",
            skill_group_count + 1, sizeof(SkillGroup) * ((size_t)skill_group_count + 1));

    int actual = 0;
    for (int i = 0; i < skill_group_count; ++i) {
        word = fread_word(fp);

        if (!str_cmp(word, "#!"))
            break;

        if (str_cmp(word, "#GROUP"))
            return (PersistResult){ PERSIST_ERR_FORMAT, "Load_groups : non-existent section", -1 };

        SkillGroup* target = &skill_group_table[actual];
        *target = grzero;
        load_struct(fp, U(&tmp_grp), skill_group_save_table, U(target));
        if (!skill_group_has_name(target))
            continue;
        actual++;
    }

    skill_group_count = actual;
    skill_group_table[skill_group_count].name = NULL;
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult skill_group_persist_rom_olc_save(const PersistWriter* writer, const char* filename)
{
    if (!writer || writer->ops != &PERSIST_FILE_WRITER_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "skill group ROM save requires FILE writer", -1 };
    FILE* fp = (FILE*)writer->ctx;
    extern SkillGroup* skill_group_table;
    extern int skill_group_count;

    fprintf(fp, "%d\n\n", skill_group_count);

    for (int i = 0; i < skill_group_count; ++i) {
        fprintf(fp, "#GROUP\n");
        save_struct(fp, U(&tmp_grp), skill_group_save_table, U(&skill_group_table[i]));
        fprintf(fp, "#END\n\n");
    }

    fprintf(fp, "#!\n");
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult skill_persist_rom_olc_load(const PersistReader* reader, const char* filename)
{
    if (!reader || reader->ops != &PERSIST_FILE_STREAM_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "skill ROM load requires FILE reader", -1 };
    FILE* fp = (FILE*)reader->ctx;
    extern Skill* skill_table;
    static Skill skzero;
    char* word;

    int size = fread_number(fp);
    if (!ensure_skill_capacity(size))
        return (PersistResult){ PERSIST_ERR_INTERNAL, "skill ROM load: alloc failed", -1 };

    if (!test_output_enabled)
        printf_log("Creating skill table of length %d, size %zu",
            size + 1, sizeof(Skill) * ((size_t)size + 1));

    int actual = 0;
    for (int i = 0; i < size; ++i) {
        word = fread_word(fp);

        if (!str_cmp(word, "#!"))
            break;

        if (str_cmp(word, "#SKILL"))
            return (PersistResult){ PERSIST_ERR_FORMAT, "Load_skills : non-existent section", -1 };

        Skill* target = &skill_table[actual];
        *target = skzero;
        load_struct(fp, U(&tmp_sk), skill_save_table, U(target));
        if (!skill_has_name(target))
            continue;
        actual++;
    }

    skill_count = actual;
    skill_table[skill_count].name = NULL;
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult skill_persist_rom_olc_save(const PersistWriter* writer, const char* filename)
{
    if (!writer || writer->ops != &PERSIST_FILE_WRITER_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "skill ROM save requires FILE writer", -1 };
    FILE* fp = (FILE*)writer->ctx;
    extern Skill* skill_table;
    extern int skill_count;

    fprintf(fp, "%d\n\n", skill_count);

    for (int i = 0; i < skill_count; ++i) {
        fprintf(fp, "#SKILL\n");
        save_struct(fp, U(&tmp_sk), skill_save_table, U(&skill_table[i]));
        fprintf(fp, "#END\n\n");
    }

    fprintf(fp, "#!\n");
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static bool pgsn_read(void* temp, char* arg)
{
    int16_t** pgsn = (int16_t**)temp;
    int16_t* ptr = gsn_lookup(arg);

    *pgsn = ptr;

    return !str_cmp(arg, "") || ptr != NULL;
}

static char* pgsn_str(void* temp)
{
    int16_t** pgsn = (int16_t**)temp;
    return gsn_name(*pgsn);
}

#ifdef U
#undef U
#endif
