////////////////////////////////////////////////////////////////////////////////
// social.c
////////////////////////////////////////////////////////////////////////////////

#include "social.h"

#include "comm.h"
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
    extern int maxSocial;
    char* word;

    char social_file[256];
    sprintf(social_file, "%s%s", area_dir, SOCIAL_FILE);
    fp = fopen(social_file, "r");

    if (!fp) {
        perror(SOCIAL_FILE);
        exit(1);
    }

    if (fscanf(fp, "%d\n", &maxSocial) < 1) {
        bug("load_social_table: Could not read maxSocial!");
        return;
    }

    flog("Creating social_table of length %d, size %zu", maxSocial + 1,
        sizeof(Social) * ((size_t)maxSocial + 1));
    /* IMPORTANT to use malloc so we can realloc later on */
    if ((social_table = calloc(sizeof(Social), (size_t)maxSocial + 1)) == NULL) {
        bug("load_social_table: Could not allocate social_table!");
        return;
    }

    for (i = 0; i < maxSocial; i++) {
        if (str_cmp((word = fread_word(fp)), "#SOCIAL")) {
            bugf("load_social_table : Expected '#SOCIAL', got '%s'.", word);
            exit(1);
        }
        load_struct(fp, U(&tmp_soc), socialsavetable, U(&social_table[i]));
    }

    /* For backwards compatibility */
    social_table[maxSocial].name = str_dup(""); /* empty! */

    fclose(fp);

    flog("Social table loaded.");
}

void save_social_table(void)
{
    FILE* fp;
    int i;
    extern int maxSocial;

    char social_file[256];
    sprintf(social_file, "%s%s", area_dir, SOCIAL_FILE);
    fp = fopen(social_file, "w");
    if (!fp) {
        bugf("save_socials: Can't open %s", social_file);
        return;
    }

    fprintf(fp, "%d\n\n", maxSocial);

    for (i = 0; i < maxSocial; i++) {
        fprintf(fp, "#SOCIAL\n");
        save_struct(fp, U(&tmp_soc), socialsavetable, U(&social_table[i]));
        fprintf(fp, "#END\n\n");
    }

    fclose(fp);
}


#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif