////////////////////////////////////////////////////////////////////////////////
// social.c
////////////////////////////////////////////////////////////////////////////////

#include "social.h"

#include "comm.h"
#include "config.h"
#include "db.h"
#include "tables.h"
#include "tablesave.h"

Social* social_table;
int social_count;
Social tmp_soc;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const SaveTableEntry socialsavetable[] = {
    { "name",		    FIELD_STRING,	U(&tmp_soc.name),		    0,      0},
    { "char_no_arg",	FIELD_STRING,	U(&tmp_soc.char_no_arg), 	0,      0},
    { "others_no_arg",  FIELD_STRING,	U(&tmp_soc.others_no_arg),	0,      0},
    { "char_found",	    FIELD_STRING,	U(&tmp_soc.char_found),	    0,      0},
    { "others_found",	FIELD_STRING,	U(&tmp_soc.others_found),	0,      0},
    { "vict_found",	    FIELD_STRING,	U(&tmp_soc.vict_found),	    0,      0},
    { "char_auto",	    FIELD_STRING,	U(&tmp_soc.char_auto),	    0,      0},
    { "others_auto",	FIELD_STRING,	U(&tmp_soc.others_auto),	0,      0},
    { NULL,		        0,		        0,				            0,      0}
};

void load_social_table()
{
    FILE* fp;
    int i;
    char* word;

    OPEN_OR_DIE(fp = open_read_socials_file());

    if (fscanf(fp, "%d\n", &social_count) < 1) {
        bug("load_social_table: Could not read social_count!");
        return;
    }

    printf_log("Creating social_table of length %d, size %zu", social_count + 1,
        sizeof(Social) * ((size_t)social_count + 1));
    /* IMPORTANT to use malloc so we can realloc later on */
    if ((social_table = calloc(sizeof(Social), (size_t)social_count + 1)) == NULL) {
        bug("load_social_table: Could not allocate social_table!");
        return;
    }

    for (i = 0; i < social_count; i++) {
        if (str_cmp((word = fread_word(fp)), "#SOCIAL")) {
            bugf("load_social_table : Expected '#SOCIAL', got '%s'.", word);
            exit(1);
        }
        load_struct(fp, U(&tmp_soc), socialsavetable, U(&social_table[i]));
    }

    /* For backwards compatibility */
    social_table[social_count].name = str_dup(""); /* empty! */

    close_file(fp);

    printf_log("Social table loaded.");
}

void save_social_table(void)
{
    FILE* fp;
    int i;

    OPEN_OR_RETURN(fp = open_write_socials_file());

    fprintf(fp, "%d\n\n", social_count);

    for (i = 0; i < social_count; i++) {
        fprintf(fp, "#SOCIAL\n");
        save_struct(fp, U(&tmp_soc), socialsavetable, U(&social_table[i]));
        fprintf(fp, "#END\n\n");
    }

    close_file(fp);
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
