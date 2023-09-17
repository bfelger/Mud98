////////////////////////////////////////////////////////////////////////////////
// race.c
////////////////////////////////////////////////////////////////////////////////

#include "race.h"

#include "comm.h"
#include "db.h"
#include "tablesave.h"

Race* race_table = NULL;
int race_count = 0;

Race tmp_race;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const SaveTableEntry race_save_table[] = {
    { "name",	    FIELD_STRING,		        U(&tmp_race.name),	        0,		            0           },
    { "pc",		    FIELD_BOOL,			        U(&tmp_race.pc_race),	    0,		            0           },
    { "act",	    FIELD_FLAGVECTOR,	        U(&tmp_race.act_flags),     0,		            0           },
    { "aff",	    FIELD_FLAGVECTOR,	        U(&tmp_race.aff),	        0,		            0           },
    { "off",	    FIELD_FLAGVECTOR,	        U(&tmp_race.off),	        0,		            0           },
    { "imm",	    FIELD_FLAGVECTOR,	        U(&tmp_race.imm),	        0,		            0           },
    { "res",	    FIELD_FLAGVECTOR,	        U(&tmp_race.res),	        0,		            0           },
    { "vuln",	    FIELD_FLAGVECTOR,	        U(&tmp_race.vuln),	        0,		            0           },
    { "form",	    FIELD_FLAGVECTOR,	        U(&tmp_race.form),	        0,		            0           },
    { "parts",	    FIELD_FLAGVECTOR,	        U(&tmp_race.parts),	        0,		            0           },
    { "points",	    FIELD_INT16,		        U(&tmp_race.points),	    0,			        0	        },
    { "arch_mult",	FIELD_INT16_ARRAY,	        U(&tmp_race.arch_mult),     U(ARCH_COUNT),	    0	        },
    { "who_name",	FIELD_STRING,		        U(&tmp_race.who_name),	    0,			        0	        },
    { "skills",	    FIELD_STRING_ARRAY,	        U(&tmp_race.skills),	    U(RACE_NUM_SKILLS), 0	        },
    { "stats",	    FIELD_INT16_ARRAY,	        U(&tmp_race.stats),		    U(STAT_MAX),	    0	        },
    { "max_stats",	FIELD_INT16_ARRAY,	        U(&tmp_race.max_stats),	    U(STAT_MAX),	    0	        },
    { "size",		FIELD_FUNCTION_INT16_TO_STR,U(&tmp_race.size),	        U(size_str),        U(size_read)},
    { NULL,		    0,				            0,			                0,		            0           }
};

void load_race_table()
{
    FILE* fp;
    char* word;
    int i;

    char races_file[256];
    sprintf(races_file, "%s%s", area_dir, RACE_FILE);
    fp = fopen(races_file, "r");

    if (!fp) {
        perror("load_races_table");
        return;
    }

    int maxrace = fread_number(fp);

    size_t new_size = sizeof(Race) * ((size_t)maxrace + 1);
    flog("Creating race_table of length %d, size %zu", maxrace + 1, new_size);

    if ((race_table = calloc(sizeof(Race), (size_t)maxrace + 1)) == NULL) {
        perror("load_races_table(): Could not allocate race_table!");
        exit(-1);
    }

    i = 0;

    while (true) {
        word = fread_word(fp);

        if (str_cmp(word, "#race")) {
            bugf("load_races_table : word %s", word);
            fclose(fp);
            return;
        }

        load_struct(fp, U(&tmp_race), race_save_table, U(&race_table[i++]));

        if (i == maxrace) {
            flog("Race table loaded.");
            fclose(fp);
            race_count = maxrace;
            race_table[i].name = NULL;
            return;
        }
    }
}


void save_race_table()
{
    FILE* fp;
    const Race* temp;
    int cnt = 0;

    char tempraces_file[256];
    sprintf(tempraces_file, "%s%s", area_dir, DATA_DIR "tempraces");
    fp = fopen(tempraces_file, "w");
    if (!fp) {
        bugf("save_races : Can't open %s", tempraces_file);
        return;
    }

    for (temp = race_table; !IS_NULLSTR(temp->name); temp++)
        cnt++;

    fprintf(fp, "%d\n\n", cnt);

    for (temp = race_table, cnt = 0; !IS_NULLSTR(temp->name); temp++) {
        fprintf(fp, "#RACE\n");
        save_struct(fp, U(&tmp_race), race_save_table, U(temp));
        fprintf(fp, "#END\n\n");
    }

    fclose(fp);

    char races_file[256];
    sprintf(races_file, "%s%s", area_dir, RACE_FILE);

#ifdef _MSC_VER
    if (!MoveFileExA(tempraces_file, races_file, MOVEFILE_REPLACE_EXISTING)) {
        bugf("save_race : Could not rename %s to %s!", tempraces_file, races_file);
        perror(races_file);
    }
#else
    if (rename(tempraces_file, races_file) != 0) {
        bugf("save_race : Could not rename %s to %s!", tempraces_file, races_file);
        perror(races_file);
    }
#endif
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
