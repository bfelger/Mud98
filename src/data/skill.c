////////////////////////////////////////////////////////////////////////////////
// data/skill.c
////////////////////////////////////////////////////////////////////////////////

#include "skill.h"

#include "spell.h"

#include <comm.h>
#include <config.h>
#include <db.h>
#include <tables.h>
#include <persist/skill/skill_persist.h>

extern bool test_output_enabled;

DEFINE_ARRAY(SkillRating, DEFAULT_SKILL_RATING)
DEFINE_ARRAY(LEVEL, DEFAULT_SKILL_LEVEL)

Skill* skill_table = NULL;
int skill_count = 0;
SkillGroup* skill_group_table = NULL;
int skill_group_count = 0;

void load_skill_group_table()
{
    PersistResult res = skill_group_persist_load(cfg_get_groups_file());
    if (!persist_succeeded(res)) {
        bugf("load_skill_group_table: failed to load groups (%s)",
            res.message ? res.message : "unknown error");
        exit(1);
    }

    if (!test_output_enabled)
        printf_log("Groups table loaded (%d groups).", skill_group_count);
}

void load_skill_table()
{
    PersistResult res = skill_persist_load(cfg_get_skills_file());
    if (!persist_succeeded(res)) {
        bugf("load_skill_table: failed to load skills (%s)",
            res.message ? res.message : "unknown error");
        exit(1);
    }
    
    if (!test_output_enabled)
        printf_log("Skills table loaded (%d skills).", skill_count);
}

void save_skill_group_table()
{
    PersistResult res = skill_group_persist_save(cfg_get_groups_file());
    if (!persist_succeeded(res))
        bugf("save_skill_group_table: failed to save groups (%s)",
            res.message ? res.message : "unknown error");
}

void save_skill_table()
{
    PersistResult res = skill_persist_save(cfg_get_skills_file());
    if (!persist_succeeded(res))
        bugf("save_skill_table: failed to save skills (%s)",
            res.message ? res.message : "unknown error");
}

SKNUM skill_lookup(const char* name)
{
    SKNUM sn;

    for (sn = 0; sn < skill_count; sn++) {
        if (skill_table[sn].name == NULL) break;
        if (LOWER(name[0]) == LOWER(skill_table[sn].name[0])
            && !str_prefix(name, skill_table[sn].name))
            return sn;
    }

    return -1;
}
