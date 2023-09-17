////////////////////////////////////////////////////////////////////////////////
// skill.c
////////////////////////////////////////////////////////////////////////////////

#include "skill.h"

#include "spell.h"

#include "comm.h"
#include "db.h"
#include "tables.h"
#include "tablesave.h"

Skill* skill_table = NULL;
int max_skill = 0;
Skill tmp_sk;

SkillGroup* skill_group_table = NULL;
int max_skill_group = 0;
SkillGroup tmp_grp;

bool pgsn_read(void* temp, char* arg);
char* pgsn_str(void* temp);

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const SaveTableEntry skill_save_table[] = {
    { "name",		        FIELD_STRING,			    U(&tmp_sk.name),		    0,			        0	                },
    { "skill_level",	    FIELD_INT16_ARRAY,		    U(&tmp_sk.skill_level),	    U(ARCH_COUNT),	    0	                },
    { "rating",	            FIELD_INT16_ARRAY,		    U(&tmp_sk.rating),		    U(ARCH_COUNT),	    0	                },
    { "spell_fun",	        FIELD_FUNCTION_INT_TO_STR,	U(&tmp_sk.spell_fun),	    U(spell_fun_str),   U(spell_fun_read)   },
    { "target",	            FIELD_INT16_FLAGSTRING,		U(&tmp_sk.target),		    U(target_table),    0	                },
    { "minimum_position",   FIELD_FUNCTION_INT16_TO_STR,U(&tmp_sk.minimum_position),U(position_str),    U(position_read)    },
    { "pgsn",		        FIELD_FUNCTION_INT_TO_STR,	U(&tmp_sk.pgsn),		    U(pgsn_str),	    U(pgsn_read)	    },
    { "slot",		        FIELD_INT,			        U(&tmp_sk.slot),		    0,			        0	                },
    { "min_mana",	        FIELD_INT,			        U(&tmp_sk.min_mana),	    0,			        0	                },
    { "beats",	            FIELD_INT,			        U(&tmp_sk.beats),		    0,			        0	                },
    { "noun_damage",	    FIELD_STRING,			    U(&tmp_sk.noun_damage),	    0,			        0	                },
    { "msg_off",	        FIELD_STRING,			    U(&tmp_sk.msg_off),		    0,			        0	                },
    { "msg_obj",	        FIELD_STRING,			    U(&tmp_sk.msg_obj),		    0,			        0	                },
    { NULL,		            0,				            0,				            0,			        0	                }
};

const SaveTableEntry skill_group_save_table[] = {
    { "name",		        FIELD_STRING,			    U(&tmp_grp.name),           0,                  0	                },
    { "rating",	            FIELD_INT16_ARRAY,		    U(&tmp_grp.rating),         U(ARCH_COUNT),	    0	                },
    { "skills",             FIELD_STRING_ARRAY,	        U(&tmp_grp.skills),         U(MAX_IN_GROUP),    0	                },
    { NULL,		            0,				            0,				            0,			        0	                }
};

void load_skill_group_table()
{
    FILE* fp;
    int i = 0;
    char* word;
    static SkillGroup grzero;

    char group_file[256];
    sprintf(group_file, "%s%s", area_dir, GROUP_FILE);
    fp = fopen(group_file, "r");

    if (!fp) {
        bug("Group file " GROUP_FILE " cannot be found.", 0);
        exit(1);
    }

    int tmp_max_group;
    if (fscanf(fp, "%d\n", &tmp_max_group) < 1) {
        perror("load_groups_table: Could not read max_skill_group!");
        return;
    }
    max_skill_group = tmp_max_group;

    flog("Creating group table of length %d, size %zu",
        max_skill_group + 1, sizeof(SkillGroup) * ((size_t)max_skill_group + 1));
    if ((skill_group_table = calloc(sizeof(SkillGroup), (size_t)max_skill_group + 1)) == NULL) {
        bug("load_groups_table(): Could not allocate skill_group_table!");
        exit(1);
    }

    for (; ; ) {
        word = fread_word(fp);

        if (!str_cmp(word, "#!"))
            break;

        if (str_cmp(word, "#GROUP")) {
            bugf("Load_groups : non-existent section (%s)", word);
            exit(1);
        }

        if (i >= max_skill_group) {
            bug("Load_groups : the number of groups is greater than max_skill_group", 0);
            exit(1);
        }

        skill_group_table[i] = grzero;
        load_struct(fp, U(&tmp_grp), skill_group_save_table, U(&skill_group_table[i++]));
    }

    skill_group_table[max_skill_group].name = NULL;

    fclose(fp);

    flog("Groups table loaded.");
}

void load_skill_table()
{
    FILE* fp;
    static Skill skzero;
    int i = 0;
    char* word;
    int tmp_max_skill;

    char skill_file[256];
    sprintf(skill_file, "%s%s", area_dir, SKILL_FILE);
    fp = fopen(skill_file, "r");

    if (!fp) {
        bug("Skill file " SKILL_FILE " cannot be found.", 0);
        exit(1);
    }

    if (fscanf(fp, "%d\n", &tmp_max_skill) < 1) {
        bug("load_skill_table(): Could not read max_skill!");
        return;
    }

    max_skill = (SKNUM)tmp_max_skill;

    flog("Creating skill table of length %d, size %zu",
        max_skill + 1, sizeof(Skill) * ((size_t)max_skill + 1));
    if ((skill_table = calloc(sizeof(Skill), (size_t)max_skill + 1)) == NULL) {
        bug("load_skill_table(): Could not allocate skill_table!");
        exit(1);
    }

    for (; ; ) {
        word = fread_word(fp);

        if (!str_cmp(word, "#!"))
            break;

        if (str_cmp(word, "#SKILL")) {
            bugf("Load_skills : non-existent section (%s)", word);
            exit(1);
        }

        if (i >= max_skill) {
            bug("Load_skills : the number of skills is greater than max_skill", 0);
            exit(1);
        }

        skill_table[i] = skzero;
        load_struct(fp, U(&tmp_sk), skill_save_table, U(&skill_table[i++]));
    }

    skill_table[max_skill].name = NULL;

    fclose(fp);

    flog("Skills table loaded.");
}

void save_skill_group_table()
{
    FILE* fpn;
    int i;

    char group_file[256];
    sprintf(group_file, "%s%s", area_dir, GROUP_FILE);
    fpn = fopen(group_file, "w");
    if (fpn == NULL) {
        bugf("save_groups: Can't open %s", group_file);
        return;
    }

    fprintf(fpn, "%d\n\n", (int)max_skill_group);

    for (i = 0; i < max_skill_group; ++i) {
        fprintf(fpn, "#GROUP\n");
        save_struct(fpn, U(&tmp_grp), skill_group_save_table, U(&skill_group_table[i]));
        fprintf(fpn, "#END\n\n");
    }

    fprintf(fpn, "#!\n");

    fclose(fpn);
}

void save_skill_table()
{
    FILE* fpn;
    int i;

    char skill_file[256];
    sprintf(skill_file, "%s%s", area_dir, SKILL_FILE);
    fpn = fopen(skill_file, "w");
    if (fpn == NULL) {
        bugf("save_skill_table: Can't open %s", skill_file);
        return;
    }

    fprintf(fpn, "%d\n\n", (int)max_skill);

    for (i = 0; i < max_skill; ++i) {
        fprintf(fpn, "#SKILL\n");
        save_struct(fpn, U(&tmp_sk), skill_save_table, U(&skill_table[i]));
        fprintf(fpn, "#END\n\n");
    }

    fprintf(fpn, "#!\n");

    fclose(fpn);
}

SKNUM skill_lookup(const char* name)
{
    SKNUM sn;

    for (sn = 0; sn < max_skill; sn++) {
        if (skill_table[sn].name == NULL) break;
        if (LOWER(name[0]) == LOWER(skill_table[sn].name[0])
            && !str_prefix(name, skill_table[sn].name))
            return sn;
    }

    return -1;
}

bool pgsn_read(void* temp, char* arg)
{
    int16_t** pgsn = (int16_t**)temp;
    int16_t* blah = gsn_lookup(arg);

    *pgsn = blah;

    return !str_cmp(arg, "") || blah != NULL;
}

char* pgsn_str(void* temp)
{
    int16_t** pgsn = (int16_t**)temp;

    return gsn_name(*pgsn);
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
