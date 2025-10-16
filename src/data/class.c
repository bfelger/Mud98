////////////////////////////////////////////////////////////////////////////////
// class.c
////////////////////////////////////////////////////////////////////////////////

#include "class.h"

#include "item.h"

#include <comm.h>
#include <config.h>
#include <db.h>
#include <tablesave.h>

Class* class_table = NULL;
int class_count = 0;

Class tmp_class;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const SaveTableEntry class_save_table[] = {
    { "name",	        FIELD_STRING,           U(&tmp_class.name),	        0,		            0   },
    { "wname",	        FIELD_STRING,           U(&tmp_class.who_name),	    0,		            0   },
    { "basegrp",        FIELD_STRING,           U(&tmp_class.base_group),   0,                  0   },
    { "dfltgrp",        FIELD_STRING,           U(&tmp_class.default_group),0,                  0   },
    { "weap",           FIELD_VNUM,             U(&tmp_class.weapon),       0,                  0   },
    { "guild",          FIELD_VNUM_ARRAY,       U(&tmp_class.guild),        U(MAX_GUILD),       0   },
    { "pstat",          FIELD_INT16,            U(&tmp_class.prime_stat),   0,                  0   },
    { "scap",           FIELD_INT16,            U(&tmp_class.skill_cap),    0,                  0   },
    { "t0_0",           FIELD_INT16,            U(&tmp_class.thac0_00),     0,                  0   },
    { "t0_32",          FIELD_INT16,            U(&tmp_class.thac0_32),     0,                  0   },
    { "hpmin",          FIELD_INT16,            U(&tmp_class.hp_min),       0,                  0   },
    { "hpmax",          FIELD_INT16,            U(&tmp_class.hp_max),       0,                  0   },
    { "fmana",          FIELD_BOOL,             U(&tmp_class.fMana),        0,                  0   },
    { "titles",         FIELD_N_STRING_ARRAY,   U(&tmp_class.titles),       U((MAX_LEVEL+1)*2), 0   },
    { "start_loc",      FIELD_VNUM,             U(&tmp_class.start_loc),    0,                  0   },
    { NULL,		        0,				        0,			                0,		            0   }
};

void load_class_table()
{
    FILE* fp;
    char* word;
    int i;

    OPEN_OR_DIE(fp = open_read_classes_file());

    int maxclass = fread_number(fp);

    size_t new_size = sizeof(Class) * ((size_t)maxclass + 1);
    printf_log("Creating class of length %d, size %zu", maxclass + 1, new_size);

    if ((class_table = calloc(sizeof(Class), (size_t)maxclass + 1)) == NULL) {
        perror("load_class_table(): Could not allocate class_table!");
        exit(-1);
    }

    i = 0;

    while (true) {
        word = fread_word(fp);

        if (str_cmp(word, "#CLASS")) {
            bugf("load_class_table : word %s", word);
            close_file(fp);
            return;
        }

        load_struct(fp, U(&tmp_class), class_save_table, U(&class_table[i++]));

        if (i == maxclass) {
            printf_log("Class table loaded.");
            close_file(fp);
            class_count = maxclass;
            class_table[i].name = NULL;
            return;
        }
    }
}

void save_class_table()
{
    FILE* fp;
    const Class* temp;

    char tempclasses_file[256];
    sprintf(tempclasses_file, "%s%s", cfg_get_data_dir(), "tempclasses");

    OPEN_OR_RETURN(fp = open_write_file(tempclasses_file));

    fprintf(fp, "%d\n\n", class_count);

    int i;
    for (temp = class_table, i = 0; i < class_count; ++i, ++temp) {
        fprintf(fp, "#CLASS\n");
        save_struct(fp, U(&tmp_class), class_save_table, U(temp));
        fprintf(fp, "#END\n\n");
    }

    close_file(fp);

    char classes_file[256];
    sprintf(classes_file, "%s%s", cfg_get_data_dir(), cfg_get_classes_file());

#ifdef _MSC_VER
    if (!MoveFileExA(tempclasses_file, classes_file, MOVEFILE_REPLACE_EXISTING)) {
        bugf("save_class : Could not rename %s to %s!", tempclasses_file, classes_file);
        perror(classes_file);
    }
#else
    if (rename(tempclasses_file, classes_file) != 0) {
        bugf("save_class : Could not rename %s to %s!", tempclasses_file, classes_file);
        perror(classes_file);
    }
#endif
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
