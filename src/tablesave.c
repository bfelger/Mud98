////////////////////////////////////////////////////////////////////////////////
// tablesave.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "bit.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interp.h"
#include "lookup.h"
#include "olc.h"
#include "recycle.h"
#include "skills.h"
#include "tables.h"

#include "entities/descriptor.h"

#include "data/mobile.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include "Windows.h"
#endif

struct savetable_type {
    char* field_name;
    int16_t	field_type;
    uintptr_t field_ptr;
    const uintptr_t argument;
    const uintptr_t argument2;
};

CmdInfo tmp_cmd;
struct race_type tmp_race;
struct social_type tmp_soc;
struct skill_type tmp_sk;
MobProgCode tmp_pcode;

char* cmd_func_name(DoFunc*);
DoFunc* cmd_func_lookup(char*);

char* gsn_name(int16_t*);
int16_t* gsn_lookup(char*);

char* spell_name(SpellFunc*);
SpellFunc* spell_function(char*);

typedef char* STR_FUNC(void*);
typedef bool   STR_READ_FUNC(void*, char*);

char* do_fun_str(void* temp)
{
    DoFunc** fun = (DoFunc**)temp;

    return cmd_func_name(*fun);
}

const char* position_str(void* temp)
{
    Position* flags = (Position*)temp;

    return position_table[*flags].name;
}

const char* size_str(void* temp)
{
    MobSize* size = (MobSize*)temp;

    return mob_size_table[UMAX(0, *size)].name;
}

char* race_str(void* temp)
{
    int16_t* race = (int16_t*)temp;

    return race_table[*race].name;
}

char* clan_str(void* temp)
{
    int16_t* klan = (int16_t*)temp;

    return clan_table[*klan].name;
}

char* class_str(void* temp)
{
    int16_t* class = (int16_t*)temp;

    return class_table[*class].name;
}

char* pgsn_str(void* temp)
{
    int16_t** pgsn = (int16_t**)temp;

    return gsn_name(*pgsn);
}

char* spell_fun_str(void* temp)
{
    SpellFunc** spfun = (SpellFunc**)temp;

    return spell_name(*spfun);
}

bool race_read(void* temp, char* arg)
{
    int16_t* race = (int16_t*)temp;

    *race = race_lookup(arg);

    return (*race == 0) ? false : true;
}

bool clan_read(void* temp, char* arg)
{
    int16_t* klan = (int16_t*)temp;

    *klan = (int16_t)clan_lookup(arg);

    return true;
}

bool class_read(void* temp, char* arg)
{
    int16_t* class_ = (int16_t*)temp;

    *class_ = (int16_t)class_lookup(arg);

    if (*class_ == -1) {
        *class_ = 0;
        return false;
    }

    return true;
}

bool do_fun_read(void* temp, char* arg)
{
    DoFunc** fun = (DoFunc**)temp;

    *fun = cmd_func_lookup(arg);

    return true;
}

bool position_read(void* temp, char* arg)
{
    int16_t* posic = (int16_t*)temp;
    int16_t ffg = (int16_t)position_lookup(arg);

    *posic = UMAX(0, ffg);

    if (ffg == -1)
        return false;
    else
        return true;
}

bool size_read(void* temp, char* arg)
{
    int16_t* size = (int16_t*)temp;
    int ffg = size_lookup(arg);

    *size = UMAX(0, (int16_t)ffg);

    if (ffg == -1)
        return false;
    else
        return true;
}

bool pgsn_read(void* temp, char* arg)
{
    int16_t** pgsn = (int16_t**)temp;
    int16_t* blah = gsn_lookup(arg);

    *pgsn = blah;

    return !str_cmp(arg, "") || blah != NULL;
}

bool spell_fun_read(void* temp, char* arg)
{
    SpellFunc** spfun = (SpellFunc**)temp;
    SpellFunc* blah = spell_function(arg);

    *spfun = blah;

    return !str_cmp(arg, "") || blah != NULL;
}

#define FIELD_STRING			    0
#define FIELD_FUNCTION_INT_TO_STR	1
#define FIELD_INT16			        2
#define FIELD_FLAGSTRING		    3
#define FIELD_INT			        4
#define FIELD_FLAGVECTOR		    5
#define FIELD_BOOL			        6
#define FIELD_INT16_ARRAY		    7
#define FIELD_STRING_ARRAY		    8
#define FIELD_FUNCTION_INT16_TO_STR	9
#define FIELD_INT16_FLAGSTRING		10
#define FIELD_BOOL_ARRAY		    11
#define FIELD_INUTIL			    12

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const struct savetable_type progcodesavetable[] = {
    { "vnum",		    FIELD_INT16,	            U(&tmp_pcode.vnum),	0,	0	},
    { "code",		    FIELD_STRING,	            U(&tmp_pcode.code),	0,	0	},
    { NULL,		        0,				            0,			        0,	0	}
};

const struct savetable_type skillsavetable[] = {
    {"name",		    FIELD_STRING,			    U(&tmp_sk.name),		    0,			    0	},
    {"skill_level",	    FIELD_INT16_ARRAY,		    U(&tmp_sk.skill_level),	    U(MAX_CLASS),	0	},
    {"rating",	        FIELD_INT16_ARRAY,		    U(&tmp_sk.rating),		    U(MAX_CLASS),	0	},
    {"spell_fun",	    FIELD_FUNCTION_INT_TO_STR,	U(&tmp_sk.spell_fun),	    U(spell_fun_str),U(spell_fun_read)	},
    {"target",	        FIELD_INT16_FLAGSTRING,		U(&tmp_sk.target),		    U(target_table),0	},
    {"minimum_position",FIELD_FUNCTION_INT16_TO_STR,U(&tmp_sk.minimum_position),U(position_str),U(position_read)	},
    {"pgsn",		    FIELD_FUNCTION_INT_TO_STR,	U(&tmp_sk.pgsn),		    U(pgsn_str),	U(pgsn_read)	},
    {"slot",		    FIELD_INT16,			    U(&tmp_sk.slot),		    0,			    0	},
    {"min_mana",	    FIELD_INT16,			    U(&tmp_sk.min_mana),	    0,			    0	},
    {"beats",	        FIELD_INT16,			    U(&tmp_sk.beats),		    0,			    0	},
    {"noun_damage",	    FIELD_STRING,			    U(&tmp_sk.noun_damage),	    0,			    0	},
    {"msg_off",	        FIELD_STRING,			    U(&tmp_sk.msg_off),		    0,			    0	},
    {"msg_obj",	        FIELD_STRING,			    U(&tmp_sk.msg_obj),		    0,			    0	},
    {NULL,		        0,				            0,				            0,			    0	}
};

const struct savetable_type socialsavetable[] = {
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

const struct savetable_type racesavetable[] = {
    { "name",	    FIELD_STRING,		U(&tmp_race.name),	    0,		    0},
    { "pc",		    FIELD_BOOL,			U(&tmp_race.pc_race),	0,		    0},
    { "act",	    FIELD_FLAGVECTOR,	U(&tmp_race.act_flags), 0,		    0},
    { "aff",	    FIELD_FLAGVECTOR,	U(&tmp_race.aff),	    0,		    0},
    { "off",	    FIELD_FLAGVECTOR,	U(&tmp_race.off),	    0,		    0},
    { "imm",	    FIELD_FLAGVECTOR,	U(&tmp_race.imm),	    0,		    0},
    { "res",	    FIELD_FLAGVECTOR,	U(&tmp_race.res),	    0,		    0},
    { "vuln",	    FIELD_FLAGVECTOR,	U(&tmp_race.vuln),	    0,		    0},
    { "form",	    FIELD_FLAGVECTOR,	U(&tmp_race.form),	    0,		    0},
    { "parts",	    FIELD_FLAGVECTOR,	U(&tmp_race.parts),	    0,		    0},
    { "points",	    FIELD_INT16,		U(&tmp_race.points),	0,			    0	},
    { "class_mult",	FIELD_INT16_ARRAY,	U(&tmp_race.class_mult),U(MAX_CLASS),	0	},
    { "who_name",	FIELD_STRING,		U(&tmp_race.who_name),	0,			    0	},
    { "skills",	    FIELD_STRING_ARRAY,	U(&tmp_race.skills),	U(5),		    0	},
    { "stats",	    FIELD_INT16_ARRAY,	U(&tmp_race.stats),		U(MAX_STATS),	0	},
    { "max_stats",	FIELD_INT16_ARRAY,	U(&tmp_race.max_stats),	U(MAX_STATS),	0	},
    { "size",		FIELD_FUNCTION_INT16_TO_STR,U(&tmp_race.size),	U(size_str),U(size_read)},
    { NULL,		    0,				    0,			            0,		    0}
};

const struct savetable_type cmdsavetable[] = {
    { "name",		FIELD_STRING,			    U(&tmp_cmd.name),	    0,		        0 },
    { "do_fun",	    FIELD_FUNCTION_INT_TO_STR,	U(&tmp_cmd.do_fun),	    U(do_fun_str),	U(do_fun_read) },
    { "position",	FIELD_FUNCTION_INT16_TO_STR,U(&tmp_cmd.position),   U(position_str),U(position_read) },
    { "level",	    FIELD_INT16,			    U(&tmp_cmd.level),	    0,		        0 },
    { "log",		FIELD_INT16_FLAGSTRING,		U(&tmp_cmd.log),	    U(log_flag_table),	0 },
    { "show",		FIELD_INT16_FLAGSTRING,		U(&tmp_cmd.show),	    U(show_flag_table),	0 },
    { NULL,		    0,				            0,			            0,		        0 }
};

void load_struct(FILE* fp, uintptr_t base_type, const struct savetable_type* table, const uintptr_t pointer)
{
    char* word;
    const struct savetable_type* temp;
    int16_t* pInt16;
    int* pBitVector;
    char** pString;
    char* string;
    int* pInt;
    STR_READ_FUNC* function;
    struct flag_type* flagtable;
    bool found = false;
    bool* pbool;
    int cnt = 0, i;

    while (str_cmp((word = fread_word(fp)), "#END")) {
        for (temp = table; !IS_NULLSTR(temp->field_name); temp++) {
            if (!str_cmp(word, temp->field_name)) {
                // Found it!
                switch (temp->field_type) {
                case FIELD_STRING:
                    pString = (char**)(temp->field_ptr - base_type + pointer);
                    *pString = fread_string(fp);
                    found = true, cnt++;
                    break;

                case FIELD_INT16:
                    pInt16 = (int16_t*)(temp->field_ptr - base_type + pointer);
                    *pInt16 = (int16_t)fread_number(fp);
                    found = true, cnt++;
                    break;

                case FIELD_INT:
                    pInt = (int*)(temp->field_ptr - base_type + pointer);
                    *pInt = fread_number(fp);
                    found = true, cnt++;
                    break;

                case FIELD_FUNCTION_INT_TO_STR:
                    function = (STR_READ_FUNC*)temp->argument2;
                    string = fread_string(fp);
                    pInt = (int*)(temp->field_ptr - base_type + pointer);
                    if ((*function) (pInt, string) == false)
                        bugf("load_struct : value %s is invalid, string %s",
                            temp->field_name, string);
                    free_string(string);
                    found = true, cnt++;
                    break;

                case FIELD_FUNCTION_INT16_TO_STR:
                    function = (STR_READ_FUNC*)temp->argument2;
                    string = fread_string(fp);
                    pInt16 = (int16_t*)(temp->field_ptr - base_type + pointer);
                    if ((*function) (pInt16, string) == false)
                        bugf("load_struct : value %s is invalid, string %s",
                            temp->field_name, string);
                    free_string(string);
                    found = true, cnt++;
                    break;

                case FIELD_FLAGSTRING:
                    flagtable = (struct flag_type*)temp->argument;
                    pBitVector = (int*)(temp->field_ptr - base_type + pointer);
                    string = fread_string(fp);
                    if ((*pBitVector = flag_value(flagtable, string)) == NO_FLAG)
                        *pBitVector = 0;
                    free_string(string);
                    found = true, cnt++;
                    break;

                case FIELD_INT16_FLAGSTRING:
                    flagtable = (struct flag_type*)temp->argument;
                    pInt16 = (int16_t*)(temp->field_ptr - base_type + pointer);
                    string = fread_string(fp);
                    if ((*pInt16 = (int16_t)flag_value(flagtable, string)) == NO_FLAG)
                        *pInt16 = 0;
                    free_string(string);
                    found = true, cnt++;
                    break;

                case FIELD_FLAGVECTOR:
                    pBitVector = (int*)(temp->field_ptr - base_type + pointer);
                    *pBitVector = fread_flag(fp);
                    found = true, cnt++;
                    break;

                case FIELD_BOOL:
                    pbool = (bool*)(temp->field_ptr - base_type + pointer);
                    string = fread_word(fp);
                    *pbool = str_cmp(string, "false") ? true : false;
                    found = true, cnt++;
                    break;

                case FIELD_INT16_ARRAY:
                    pInt16 = (int16_t*)(temp->field_ptr - base_type + pointer);
                    i = 0;
                    while (str_cmp((string = fread_word(fp)), "@")) {
                        if (i == (int)temp->argument)
                            bugf("load_struct : FIELD_int16_array %s has too many elements",
                                temp->field_name);
                        else
                            pInt16[i++] = (int16_t)atoi(string);
                    }
                    found = true, cnt++;
                    break;

                case FIELD_STRING_ARRAY:
                    pString = (char**)(temp->field_ptr - base_type + pointer);
                    i = 0;
                    while (str_cmp((string = fread_string(fp)), "@")) {
                        if (i == (int)temp->argument)
                            bugf("load_struct : FIELD_string_array %s has too many elements",
                                temp->field_name);
                        else
                            pString[i++] = string;
                    }
                    found = true, cnt++;
                    break;

                case FIELD_INUTIL:
                    fread_to_eol(fp);
                    found = true, cnt++;
                    break;

                case FIELD_BOOL_ARRAY:
                    pbool = (bool*)(temp->field_ptr - base_type + pointer);
                    i = 0;
                    while (str_cmp((string = fread_word(fp)), "@")) {
                        if ((temp->argument != 0
                            && i == (int)temp->argument)
                            || (temp->argument == 0
                                && temp->argument2 != 0
                                && i == *((int*)temp->argument2)))
                            bugf("load_struct : FIELD_bool_array %s has too many elements",
                                temp->field_name);
                        else
                            pbool[i++] = (bool)atoi(string);
                    }
                    found = true, cnt++;
                    break;
                } // switch
                if (found == true)
                    break;
            } // if
        } // for

        if (found == false) {
            bugf("load_struct : section '%s' not found", word);
            fread_to_eol(fp);
        }
        else
            found = false;
    } // while
}

void save_struct(FILE* fp, uintptr_t base_type, const struct savetable_type* table, const uintptr_t pointer)
{
    const struct savetable_type* temp;
    char** pString;
    int16_t* pInt16;
    STR_FUNC* function;
    char* string;
    int* pBitVector;
    bool* pbool;
    const struct flag_type* flagtable;
    int cnt = 0, i;

    for (temp = table; !IS_NULLSTR(temp->field_name); temp++) {
        switch (temp->field_type) {
        default:
            bugf("save_struct : field_type %d is invalid, value %s",
                temp->field_type, temp->field_name);
            break;

        case FIELD_STRING:
            pString = (char**)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s %s~\n", temp->field_name, !IS_NULLSTR(*pString) ? *pString : "");
            break;

        case FIELD_INT16:
            pInt16 = (int16_t*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s %d\n", temp->field_name, *pInt16);
            break;

        case FIELD_INT:
            pBitVector = (int*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s %d\n", temp->field_name, *pBitVector);
            break;

        case FIELD_FUNCTION_INT_TO_STR:
            function = (STR_FUNC*)temp->argument;
            pBitVector = (int*)(temp->field_ptr - base_type + pointer);
            string = (*function)((void*)pBitVector);
            fprintf(fp, "%s %s~\n", temp->field_name, string);
            break;

        case FIELD_FUNCTION_INT16_TO_STR:
            function = (STR_FUNC*)temp->argument;
            pInt16 = (int16_t*)(temp->field_ptr - base_type + pointer);
            string = (*function)((void*)pInt16);
            fprintf(fp, "%s %s~\n", temp->field_name, string);
            break;

        case FIELD_FLAGSTRING:
            flagtable = (struct flag_type*)temp->argument;
            pBitVector = (int*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s %s~\n", temp->field_name, flag_string(flagtable, *pBitVector));
            break;

        case FIELD_INT16_FLAGSTRING:
            flagtable = (struct flag_type*)temp->argument;
            pInt16 = (int16_t*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s %s~\n", temp->field_name, flag_string(flagtable, *pInt16));
            break;

        case FIELD_FLAGVECTOR:
            pBitVector = (int*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s %s\n", temp->field_name, print_flags(*pBitVector));
            break;

        case FIELD_BOOL:
            pbool = (bool*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s %s\n", temp->field_name,
                (*pbool == true) ? "true" : "false");
            break;

        case FIELD_INT16_ARRAY:
            pInt16 = (int16_t*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s ", temp->field_name);
            for (i = 0; i < (int)temp->argument; i++)
                fprintf(fp, "%d ", pInt16[i]);
            fprintf(fp, "@\n");
            break;

        case FIELD_STRING_ARRAY:
            pString = (char**)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s ", temp->field_name);
            for (i = 0; i < (int)temp->argument; i++)
                fprintf(fp, "%s~ ", !IS_NULLSTR(pString[i]) ? pString[i] : "");
            fprintf(fp, "@~\n");
            break;

        case FIELD_BOOL_ARRAY:
            pbool = (bool*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s ", temp->field_name);
            for (i = 0; i < (temp->argument ? (int)temp->argument : *(int*)temp->argument2); i++)
                fprintf(fp, "%d ", pbool[i] == true ? 1 : 0);
            fprintf(fp, "@\n");
            break;

        case FIELD_INUTIL:
            break;
        }

        cnt++;
    }
}

void save_command_table()
{
    FILE* fp;
    const CmdInfo* temp;
    extern CmdInfo* cmd_table;
    int cnt = 0;

    char cmd_file[256];
    sprintf(cmd_file, "%s%s", area_dir, COMMAND_FILE);
    fp = fopen(cmd_file, "w");
    if (!fp) {
        bugf("save_command_table: Could not open %s", cmd_file);
        return;
    }

    for (temp = cmd_table; !IS_NULLSTR(temp->name); temp++)
        cnt++;

    fprintf(fp, "%d\n\n", cnt);

    for (temp = cmd_table; !IS_NULLSTR(temp->name); temp++) {
        fprintf(fp, "#COMMAND\n");
        save_struct(fp, U(&tmp_cmd), cmdsavetable, U(temp));
        fprintf(fp, "#END\n\n");
    }

    fclose(fp);
}

void load_command_table(void)
{
    FILE* fp;
    extern CmdInfo* cmd_table;
    extern int max_cmd;
    int i = 0;
    int size;
    char* word;

    char cmd_file[256];
    sprintf(cmd_file, "%s%s", area_dir, COMMAND_FILE);
    fp = fopen(cmd_file, "r");

    if (fp == NULL) {
        perror("load_command_table ");
        return;
    }

    size = fread_number(fp);

    max_cmd = size;

    flog("Creating cmd_table of length %d, size %zu", size + 1,
        sizeof(CmdInfo) * ((size_t)size + 1));

    if ((cmd_table = calloc(sizeof(CmdInfo), (size_t)size + 1)) == NULL) {
        perror("load_command_table: Could not allocate cmd_table!");
        exit(-1);
    }
    //memset(cmd_table, 0, (size + 1) * sizeof(CmdInfo));

    i = 0;

    while (true) {
        word = fread_word(fp);

        if (str_cmp(word, "#COMMAND")) {
            bugf("load_command_table : word %s", word);
            fclose(fp);
            return;
        }

        load_struct(fp, U(&tmp_cmd), cmdsavetable, U(&cmd_table[i++]));

        if (i == size) {
            flog("Command table loaded.");
            fclose(fp);
            cmd_table[i].name = str_dup("");
            return;
        }
    }
}

void load_races_table()
{
    FILE* fp;
    extern int maxrace;
    char* word;
    int i;

    char races_file[256];
    sprintf(races_file, "%s%s", area_dir, RACES_FILE);
    fp = fopen(races_file, "r");

    if (!fp) {
        perror("load_races_table");
        return;
    }

    maxrace = fread_number(fp);

    size_t new_size = sizeof(struct race_type) * ((size_t)maxrace + 1);
    flog("Creating race_table of length %d, size %zu", maxrace + 1, new_size);

    if ((race_table = calloc(sizeof(struct race_type), (size_t)maxrace + 1)) == NULL) {
        perror("load_races_table(): Could not allocate race_table!");
        exit(-1);
    }

    //// clear races
    //for (i = 0; i <= maxrace; i++) {
    //    race_table[i] = cRace;
    //    race_table[i].race_id = i;
    //}

    i = 0;

    while (true) {
        word = fread_word(fp);

        if (str_cmp(word, "#race")) {
            bugf("load_races_table : word %s", word);
            fclose(fp);
            return;
        }

        load_struct(fp, U(&tmp_race), racesavetable, U(&race_table[i++]));

        if (i == maxrace) {
            flog("Race table loaded.");
            fclose(fp);
            race_table[i].name = NULL;
            return;
        }
    }
}

void load_socials_table()
{
    FILE* fp;
    int i;
    extern int maxSocial;
    char* clave;

    char social_file[256];
    sprintf(social_file, "%s%s", area_dir, SOCIAL_FILE);
    fp = fopen(social_file, "r");

    if (!fp) {
        perror(SOCIAL_FILE);
        exit(1);
    }

    if (fscanf(fp, "%d\n", &maxSocial) < 1) {
        bug("load_socials_table: Could not read maxSocial!");
        return;
    }

    flog("Creating social_table of length %d, size %zu", maxSocial + 1,
        sizeof(struct social_type) * ((size_t)maxSocial + 1));
    /* IMPORTANT to use malloc so we can realloc later on */
    if ((social_table = calloc(sizeof(struct social_type), (size_t)maxSocial + 1)) == NULL) {
        bug("load_social_table: Could not allocate social_table!");
        return;
    }

    for (i = 0; i < maxSocial; i++) {
        if (str_cmp((clave = fread_word(fp)), "#SOCIAL")) {
            bugf("load_socials_table : section '%s' does not exist",
                clave);
            exit(1);
        }
        load_struct(fp, U(&tmp_soc), socialsavetable, U(&social_table[i]));
    }

    /* For backwards compatibility */
    social_table[maxSocial].name = str_dup(""); /* empty! */

    fclose(fp);

    flog("Social table loaded.");
}

void load_skills_table()
{
    FILE* fp;
    static struct skill_type skzero;
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
        bug("load_skills_table(): Could not read max_skill!");
        return;
    }

    max_skill = (SKNUM)tmp_max_skill;

    flog("Creating skill table of length %d, size %zu",
        max_skill + 1, sizeof(struct skill_type) * ((size_t)max_skill + 1));
    if ((skill_table = calloc(sizeof(struct skill_type), (size_t)max_skill + 1)) == NULL) {
        bug("load_skills_table(): Could not allocate skill_table!");
        return;
    }

    if (!skill_table) {
        bug("Error! Skill_table == NULL, max_skill : %d", max_skill);
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
        load_struct(fp, U(&tmp_sk), skillsavetable, U(&skill_table[i++]));
    }

    skill_table[max_skill].name = NULL;

    fclose(fp);
}

void save_races(void)
{
    FILE* fp;
    const struct race_type* temp;
    extern struct race_type* race_table;
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
        fprintf(fp, "#race\n");
        save_struct(fp, U(&tmp_race), racesavetable, U(temp));
        fprintf(fp, "#END\n\n");
    }

    fclose(fp);

    char races_file[256];
    sprintf(races_file, "%s%s", area_dir, RACES_FILE);

#ifdef _MSC_VER
    if (!MoveFileExA(tempraces_file, races_file, MOVEFILE_REPLACE_EXISTING)) {
        bugf("save_race : Could not rename %s to %s!", tempraces_file, races_file);
        perror(races_file);
    }
#else
    if (rename(tempraces_file, races_file) != 0) {
        bugf("save_area : Could not rename %s to %s!", tempraces_file, races_file);
        perror(races_file);
    }
#endif
}

void save_socials(void)
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

    fprintf(fp, "%d\n", maxSocial);

    for (i = 0; i < maxSocial; i++) {
        fprintf(fp, "#SOCIAL\n");
        save_struct(fp, U(&tmp_soc), socialsavetable, U(&social_table[i]));
        fprintf(fp, "#END\n\n");
    }

    fclose(fp);
}

void save_skills()
{
    FILE* fpn;
    int i;

    char skill_file[256];
    sprintf(skill_file, "%s%s", area_dir, SKILL_FILE);
    fpn = fopen(skill_file, "w");
    if (fpn == NULL) {
        bugf("save_skills: Can't open %s", skill_file);
        return;
    }

    fprintf(fpn, "%d\n", (int)max_skill);

    for (i = 0; i < max_skill; ++i) {
        fprintf(fpn, "#SKILL\n");
        save_struct(fpn, U(&tmp_sk), skillsavetable, U(&skill_table[i]));
        fprintf(fpn, "#END\n\n");
    }

    fprintf(fpn, "#!\n");

    fclose(fpn);
}

void save_progs(VNUM minvnum, VNUM maxvnum)
{
    FILE* fp;
    MobProgCode* pMprog;
    char buf[64];

    for (pMprog = mprog_list; pMprog; pMprog = pMprog->next)
        if (pMprog->changed == true
            && BETWEEN_I(minvnum, pMprog->vnum, maxvnum)) {
            sprintf(buf, "%s%s%d.prg", area_dir, PROG_DIR, pMprog->vnum);
            fp = fopen(buf, "w");
            if (!fp) {
                bugf("save_progs: Can't open %s", buf);
                return;
            }

            fprintf(fp, "#PROG\n");
            save_struct(fp, U(&tmp_pcode), progcodesavetable, U(pMprog));
            fprintf(fp, "#END\n\n");
            fclose(fp);

            pMprog->changed = false;
        }
}

void load_prog(FILE* fp, MobProgCode** prog)
{
    extern MobProgCode* mprog_list;
    static MobProgCode mprog_zero = { 0 };
    char* word = fread_word(fp);

    if (str_cmp(word, "#PROG")) {
        bugf("load_prog : section '%s' is invalid", word);
        *prog = NULL;
        return;
    }

    *prog = alloc_perm(sizeof(MobProgCode));

    // Clear it
    **prog = mprog_zero;

    load_struct(fp, U(&tmp_pcode), progcodesavetable, U(*prog));

    // Populate the linked list
    if (mprog_list == NULL)
        mprog_list = *prog;
    else {
        // At the beginning or the end?
        if ((*prog)->vnum < mprog_list->vnum) {
            (*prog)->next = mprog_list;
            mprog_list = *prog;
        }
        else {
            MobProgCode* temp;
            MobProgCode* prev = mprog_list;

            for (temp = mprog_list->next; temp; temp = temp->next) {
                if (temp->vnum > (*prog)->vnum)
                    break;
                prev = temp;
            }
            prev->next = *prog;
            (*prog)->next = temp;
        }
    }
}

MobProgCode* pedit_prog(VNUM vnum)
{
    FILE* fp;
    MobProgCode* prog;
    char buf[128];
    extern bool fBootDb;

    prog = get_mprog_index(vnum);

    if (prog != NULL)
        return prog;

    sprintf(buf, "%s%s%d.prg", area_dir, PROG_DIR, vnum);

    fp = fopen(buf, "r");

    if (!fp) {
        if (fBootDb == true)
            perror("pedit_prog");
        return NULL;
    }

    load_prog(fp, &prog);

    fclose(fp);

    return prog;
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
