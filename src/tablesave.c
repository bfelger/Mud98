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
#include "data/race.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include "Windows.h"
#endif

// The following is for an indeterminate number of items bounded by @
#define MAX_LOAD 1000

CmdInfo tmp_cmd;
struct social_type tmp_soc;
MobProgCode tmp_pcode;

char* cmd_func_name(DoFunc*);
DoFunc* cmd_func_lookup(char*);

typedef char* STR_FUNC(void*);
typedef bool STR_READ_FUNC(void*, char*);

char* do_fun_str(void* temp)
{
    DoFunc** fun = (DoFunc**)temp;

    return cmd_func_name(*fun);
}

char* race_str(void* temp)
{
    int16_t* race = (int16_t*)temp;

    return race_table[*race].name;
}

char* clan_str(void* temp)
{
    int16_t* clan = (int16_t*)temp;

    return clan_table[*clan].name;
}

char* class_str(void* temp)
{
    int16_t* class_ = (int16_t*)temp;

    return class_table[*class_].name;
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

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const SaveTableEntry progcodesavetable[] = {
    { "vnum",		    FIELD_VNUM,                 U(&tmp_pcode.vnum),	0,	0	},
    { "code",		    FIELD_STRING,	            U(&tmp_pcode.code),	0,	0	},
    { NULL,		        0,				            0,			        0,	0	}
};

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

const SaveTableEntry cmdsavetable[] = {
    { "name",		FIELD_STRING,			    U(&tmp_cmd.name),	    0,		            0               },
    { "do_fun",	    FIELD_FUNCTION_INT_TO_STR,	U(&tmp_cmd.do_fun),	    U(do_fun_str),	    U(do_fun_read)  },
    { "position",	FIELD_FUNCTION_INT16_TO_STR,U(&tmp_cmd.position),   U(position_str),    U(position_read)},
    { "level",	    FIELD_INT16,			    U(&tmp_cmd.level),	    0,		            0               },
    { "log",		FIELD_INT16_FLAGSTRING,		U(&tmp_cmd.log),	    U(log_flag_table),	0               },
    { "show",		FIELD_INT16_FLAGSTRING,		U(&tmp_cmd.show),	    U(show_flag_table),	0               },
    { NULL,		    0,				            0,			            0,		            0               }
};

void load_struct(FILE* fp, uintptr_t base_type, const SaveTableEntry* table, const uintptr_t pointer)
{
    char* word;
    const SaveTableEntry* temp;
    int16_t* pInt16;
    int* pBitVector;
    char** pString;
    char* string;
    int* pInt;
    STR_READ_FUNC* function;
    struct flag_type* flagtable;
    bool found = false;
    bool* pbool;
    VNUM* p_vnum;
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

                case FIELD_VNUM:
                    p_vnum = (VNUM*)(temp->field_ptr - base_type + pointer);
                    *p_vnum = fread_number(fp);
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

                case FIELD_VNUM_ARRAY:
                    p_vnum = (VNUM*)(temp->field_ptr - base_type + pointer);
                    i = 0;
                    while (str_cmp((string = fread_word(fp)), "@")) {
                        if (i == (int)temp->argument)
                            bugf("load_struct : FIELD_vnum_array %s has too many elements",
                                temp->field_name);
                        else
                            p_vnum[i++] = (VNUM)atoi(string);
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

void save_struct(FILE* fp, uintptr_t base_type, const SaveTableEntry* table, const uintptr_t pointer)
{
    const SaveTableEntry* temp;
    char** pString;
    int16_t* pInt16;
    int* pInt;
    VNUM* p_vnum;
    STR_FUNC* function;
    char* string;
    FLAGS* pBitVector;
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
            pInt = (int*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s %d\n", temp->field_name, *pInt);
            break;

        case FIELD_VNUM:
            p_vnum = (VNUM*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s %d\n", temp->field_name, *p_vnum);
            break;

        case FIELD_FUNCTION_INT_TO_STR:
            function = (STR_FUNC*)temp->argument;
            pInt = (int*)(temp->field_ptr - base_type + pointer);
            string = (*function)((void*)pInt);
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
            pBitVector = (FLAGS*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s %s~\n", temp->field_name, flag_string(flagtable, *pBitVector));
            break;

        case FIELD_INT16_FLAGSTRING:
            flagtable = (struct flag_type*)temp->argument;
            pInt16 = (int16_t*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s %s~\n", temp->field_name, flag_string(flagtable, *pInt16));
            break;

        case FIELD_FLAGVECTOR:
            pBitVector = (FLAGS*)(temp->field_ptr - base_type + pointer);
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

        case FIELD_VNUM_ARRAY:
            p_vnum = (VNUM*)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s ", temp->field_name);
            for (i = 0; i < (int)temp->argument; i++)
                fprintf(fp, "%d ", p_vnum[i]);
            fprintf(fp, "@\n");
            break;

        case FIELD_STRING_ARRAY:
            pString = (char**)(temp->field_ptr - base_type + pointer);
            fprintf(fp, "%s ", temp->field_name);
            for (i = 0; i < (int)temp->argument; i++) {
                if (IS_NULLSTR(pString[i]))
                    break;
                fprintf(fp, "%s~ ", pString[i]);
            }
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

void load_socials_table()
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
        if (str_cmp((word = fread_word(fp)), "#SOCIAL")) {
            bugf("load_socials_table : Expected '#SOCIAL', got '%s'.", word);
            exit(1);
        }
        load_struct(fp, U(&tmp_soc), socialsavetable, U(&social_table[i]));
    }

    /* For backwards compatibility */
    social_table[maxSocial].name = str_dup(""); /* empty! */

    fclose(fp);

    flog("Social table loaded.");
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

    fprintf(fp, "%d\n\n", maxSocial);

    for (i = 0; i < maxSocial; i++) {
        fprintf(fp, "#SOCIAL\n");
        save_struct(fp, U(&tmp_soc), socialsavetable, U(&social_table[i]));
        fprintf(fp, "#END\n\n");
    }

    fclose(fp);
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
