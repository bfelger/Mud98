////////////////////////////////////////////////////////////////////////////////
// lox.c
// Handles the "lox" command to handle in-game ad hoc scripts
////////////////////////////////////////////////////////////////////////////////

#include "comm.h"
#include "config.h"
#include "db.h"
#include "file.h"

#include "entities/mobile.h"

#include "lox/lox.h"
#include "lox/vm.h"

Table spell_scripts;

bool lox_read(void* temp, const char* arg)
{
    String* key = lox_string(arg);

    Entry** sk_script = (Entry**)temp;
    Entry* entry;
        
    if (!table_get_entry(&vm.globals, key, &entry)) {
        bug("lox_read(): Unknown script '%s'.", arg);
        *sk_script = NULL;
        return false;
    }

    if (!IS_CLOSURE(entry->value)) {
        bug("lox_read(): '%s' is not a callable object.", arg);
        printf("    - ");
        print_value(entry->value);
        *sk_script = NULL;
        return false;
    }

    *sk_script = entry;
    return true;
}

const char* lox_str(void* temp)
{
    Entry** entry = (Entry**)temp;
    return (AS_STRING((*entry)->key))->chars;
}

static void compile_lox_script(const String* source)
{
    InterpretResult result = interpret_code(source->chars);

    switch (result) {
    case INTERPRET_OK:
        break;
    case INTERPRET_COMPILE_ERROR:
        bug("compile_lox_script(): Compile error.");
        break;
    case INTERPRET_RUNTIME_ERROR:
        bug("compile_lox_script(): Runtime error.");
        break;
    }
}

static void load_lox_script(const char* source_file)
{
    FILE* fp;
    char* raw_source;
    String* source;
    String* script_name;
    char path[MAX_INPUT_LENGTH*2];
    Table* targ_table;

    sprintf(path, "%s%s%s", cfg_get_data_dir(), cfg_get_scripts_dir(), source_file);

#ifdef DEBUG_INTEGRATION
    printf("    - %s\n", path);
#endif

    OPEN_OR_RETURN(fp = open_read_file(path));

    char* word = fread_word(fp);

    while (strcmp(word, "#END") != 0) {
        if (strcmp(word, "#SPELL") == 0) {
            targ_table = &spell_scripts;
        }
        else {
            bugf("compile_lox_script : section '%s' is invalid", word);
            break;
        }

        script_name = fread_lox_string(fp);
        
        raw_source = fread_lox_script(fp);
        int len = (int)strlen(raw_source);
        source = copy_string(raw_source, len);

        table_set(targ_table, script_name, OBJ_VAL(source));

#ifdef DEBUG_INTEGRATION
        printf("      * Compiling %s()\n", script_name->chars);
#endif

        compile_lox_script(source);

        word = fread_word(fp);
    }

    fclose(fp);
}

// Used to load lox scripts from the scripts/ directory on bootup
void load_lox_scripts()
{
    FILE* fp;
    char buf[MAX_INPUT_LENGTH*2];

    printf_log("Loading Lox scripts.");

    init_table(&spell_scripts);

    sprintf(buf, "%s%s.lox", cfg_get_data_dir(), cfg_get_scripts_dir());

    OPEN_OR_RETURN(fp = open_read_file(buf));

    char line[MAX_INPUT_LENGTH*2];
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;

        if (line[0] == '\0' || line[0] == '#')
            continue;

        // Compile each line
        load_lox_script(line);
    } 

    fclose(fp);
}

static void lox_eval(Mobile* ch, char* argument)
{
    compile_context.me = ch;
    exec_context.me = ch;
    exec_context.is_repl = true;

    interpret_code(argument);

    compile_context.me = NULL;
    exec_context.me = NULL;
    exec_context.is_repl = false;
}

void do_lox(Mobile* ch, char* argument)
{
    static const char* help =
        "{jUSAGE: {*LOX EVAL <script>\n\r"
        "       LOX NEW <name>\n\r"
        "{j\n\r"
        "Type '{*LOX <option>{j' for more information.{x\n\r";

    char arg[MAX_INPUT_LENGTH];

    if (!ch->pcdata)
        return;

    READ_ARG(arg);

    if (arg[0] == '\0' || arg[0] == '?' ) {
        send_to_char(help, ch);
        return;
    }

    if (!str_cmp(arg, "eval")) {
        lox_eval(ch, argument);
        return;
    }
}