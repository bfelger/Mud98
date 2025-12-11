////////////////////////////////////////////////////////////////////////////////
// lox.c
// Handles the "lox" command to handle in-game ad hoc scripts
////////////////////////////////////////////////////////////////////////////////

#include "comm.h"
#include "config.h"
#include "db.h"
#include "file.h"

#include <entities/mobile.h>

#include <lox/lox.h>
#include <lox/vm.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    LOX_SCRIPT_WHEN_PRE,
    LOX_SCRIPT_WHEN_POST,
} LoxScriptWhen;

typedef struct lox_script_entry_t {
    const char* category;
    const char* file;
    LoxScriptWhen when;
    char* source;
    bool executed;
} LoxScriptEntry;

typedef struct lox_script_registry_t {
    LoxScriptEntry* entries;
    size_t count;
    size_t capacity;
} LoxScriptRegistry;

static LoxScriptRegistry lox_scripts = { 0 };
static bool post_scripts_executed = false;

static void reset_lox_script_registry();
static LoxScriptEntry* append_lox_script_entry();
static bool parse_lox_when(const char* value, LoxScriptWhen* when_out);
static bool read_lox_catalog_entry(FILE* fp, LoxScriptEntry* entry);
static bool ensure_lox_script_source(LoxScriptEntry* entry);
static bool run_lox_script_entry(LoxScriptEntry* entry);
static void free_lox_entry_source(LoxScriptEntry* entry);
static void build_lox_catalog_path(char* out, size_t out_len);
static void build_lox_source_path(char* out, size_t out_len, const LoxScriptEntry* entry);
static bool is_absolute_path(const char* path);
static char* read_lox_source_file(const char* path);

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

bool load_lox_class(FILE* fp, const char* entity_type_name, Entity* entity)
{
    char class_name[MAX_INPUT_LENGTH];
    String* script = fread_lox_string(fp);
    sprintf(class_name, "%s_%" PRVNUM, entity_type_name, entity->vnum);

    ObjClass* klass = create_entity_class(entity, class_name, script->chars);
    if (!klass) {
        bugf("load_lox_class: failed to create class for %s %" PRVNUM, entity_type_name, entity->vnum);
        return false;
    }
    else {
        entity->klass = klass;
        entity->script = script;
        return true;
    }
}

static void compile_lox_script(const char* source)
{
    InterpretResult result = interpret_code(source);

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

// Used to load lox scripts from the scripts/ directory on bootup
void load_lox_public_scripts()
{
    FILE* fp;
    char catalog_path[MAX_INPUT_LENGTH * 3];

    printf_log("Loading Lox scripts.");

    reset_lox_script_registry();
    build_lox_catalog_path(catalog_path, sizeof(catalog_path));

    OPEN_OR_RETURN(fp = open_read_file(catalog_path));

    int script_count = fread_number(fp);
    for (int i = 0; i < script_count; ++i) {
        char* section = fread_word(fp);
        if (str_cmp(section, "#LOX_SCRIPT")) {
            bugf("load_lox_public_scripts: expected #LOX_SCRIPT, found %s", section);
            break;
        }

        LoxScriptEntry* entry = append_lox_script_entry();
        if (!read_lox_catalog_entry(fp, entry)) {
            --lox_scripts.count;
            continue;
        }

        if (!ensure_lox_script_source(entry)) {
            --lox_scripts.count;
            continue;
        }

        if (entry->when == LOX_SCRIPT_WHEN_PRE) {
            run_lox_script_entry(entry);
        }
    }

    close_file(fp);
}

void run_post_lox_public_scripts()
{
    if (post_scripts_executed)
        return;

    for (size_t i = 0; i < lox_scripts.count; ++i) {
        LoxScriptEntry* entry = &lox_scripts.entries[i];
        if (entry->when != LOX_SCRIPT_WHEN_POST || entry->executed)
            continue;

        if (!ensure_lox_script_source(entry))
            continue;

        run_lox_script_entry(entry);
    }

    post_scripts_executed = true;
}

static void lox_eval(Mobile* ch, char* argument, bool is_repl)
{
    ExecContext old_exec_context = exec_context;
    CompileContext old_compile_context = compile_context;

    compile_context.me = ch;
    exec_context.me = ch;
    exec_context.is_repl = is_repl;

    interpret_code(argument);

    exec_context = old_exec_context;
    compile_context = old_compile_context;
}

void do_lox(Mobile* ch, char* argument)
{
    static const char* help =
        COLOR_INFO "USAGE: " COLOR_ALT_TEXT_1 "LOX [EVAL] <expression>\n\r"
        COLOR_INFO "\n\r"
        "Type '" COLOR_ALT_TEXT_1 "LOX EVAL returns a value." COLOR_EOL;

    char arg[MAX_INPUT_LENGTH] = "";
    bool is_repl = false;

    if (!ch->pcdata)
        return;

    if (!str_prefix(argument, "eval")) {
        READ_ARG(arg);
    }

    if (argument[0] == '\0' || argument[0] == '?' ) {
        send_to_char(help, ch);
        return;
    }

    if (!str_cmp(arg, "eval")) {
        is_repl = true;
    }

    lox_eval(ch, argument, is_repl);
}

static void reset_lox_script_registry()
{
    for (size_t i = 0; i < lox_scripts.count; ++i)
        free_lox_entry_source(&lox_scripts.entries[i]);

    lox_scripts.count = 0;
    post_scripts_executed = false;
}

static LoxScriptEntry* append_lox_script_entry()
{
    if (lox_scripts.capacity == 0) {
        lox_scripts.capacity = 4;
        lox_scripts.entries = calloc(lox_scripts.capacity, sizeof(LoxScriptEntry));
    }
    else if (lox_scripts.count == lox_scripts.capacity) {
        lox_scripts.capacity *= 2;
        lox_scripts.entries = realloc(lox_scripts.entries,
            lox_scripts.capacity * sizeof(LoxScriptEntry));
    }

    if (!lox_scripts.entries) {
        bug("append_lox_script_entry: allocation failed", 0);
        exit(1);
    }

    LoxScriptEntry* entry = &lox_scripts.entries[lox_scripts.count++];
    entry->category = "";
    entry->file = NULL;
    entry->when = LOX_SCRIPT_WHEN_PRE;
    entry->source = NULL;
    entry->executed = false;
    return entry;
}

static bool parse_lox_when(const char* value, LoxScriptWhen* when_out)
{
    if (!value || !when_out)
        return false;

    if (!str_cmp(value, "pre")) {
        *when_out = LOX_SCRIPT_WHEN_PRE;
        return true;
    }

    if (!str_cmp(value, "post")) {
        *when_out = LOX_SCRIPT_WHEN_POST;
        return true;
    }

    bugf("load_lox_public_scripts: unknown when value '%s'", value);
    return false;
}

static bool read_lox_catalog_entry(FILE* fp, LoxScriptEntry* entry)
{
    bool done = false;

    while (!done) {
        char* word = fread_word(fp);
        if (!str_cmp(word, "#END")) {
            done = true;
            break;
        }

        if (!str_cmp(word, "category")) {
            entry->category = fread_string(fp);
        }
        else if (!str_cmp(word, "file")) {
            entry->file = fread_string(fp);
        }
        else if (!str_cmp(word, "when")) {
            char* when_val = fread_string(fp);
            if (!parse_lox_when(when_val, &entry->when))
                return false;
        }
        else {
            bugf("load_lox_public_scripts: unknown key '%s'", word);
            fread_to_eol(fp);
        }
    }

    if (!entry->file || entry->file[0] == '\0') {
        bug("load_lox_public_scripts: script missing file name", 0);
        return false;
    }

    return true;
}

static bool ensure_lox_script_source(LoxScriptEntry* entry)
{
    if (entry->source)
        return true;

    char path[MAX_INPUT_LENGTH * 3];
    build_lox_source_path(path, sizeof(path), entry);

    entry->source = read_lox_source_file(path);
    if (!entry->source) {
        bugf("load_lox_public_scripts: unable to read %s", path);
        return false;
    }

    return true;
}

static bool run_lox_script_entry(LoxScriptEntry* entry)
{
    if (!entry->source && !ensure_lox_script_source(entry))
        return false;

    compile_lox_script(entry->source);
    entry->executed = true;
    free_lox_entry_source(entry);
    return true;
}

static void free_lox_entry_source(LoxScriptEntry* entry)
{
    if (entry->source) {
        free(entry->source);
        entry->source = NULL;
    }
}

static void build_lox_catalog_path(char* out, size_t out_len)
{
    const char* catalog = cfg_get_lox_file();
    if (!catalog)
        catalog = "lox.olc";

    if (is_absolute_path(catalog)) {
        snprintf(out, out_len, "%s", catalog);
    }
    else if (strpbrk(catalog, "/\\")) {
        snprintf(out, out_len, "%s%s", cfg_get_data_dir(), catalog);
    }
    else {
        snprintf(out, out_len, "%s%s%s",
            cfg_get_data_dir(), cfg_get_scripts_dir(), catalog);
    }
}

static void build_lox_source_path(char* out, size_t out_len, const LoxScriptEntry* entry)
{
    const char* category = entry->category ? entry->category : "";
    const char* file = entry->file ? entry->file : "";
    size_t cat_len = strlen(category);
    bool needs_sep = cat_len > 0 && category[cat_len - 1] != '/' && category[cat_len - 1] != '\\';

    snprintf(out, out_len, "%s%s%s%s%s",
        cfg_get_data_dir(),
        cfg_get_scripts_dir(),
        category,
        (cat_len > 0 && needs_sep) ? "/" : "",
        file);
}

static char* read_lox_source_file(const char* path)
{
    FILE* fp;
    OPEN_OR_RETURN_NULL(fp = open_read_file(path));

    if (fseek(fp, 0, SEEK_END) != 0) {
        close_file(fp);
        return NULL;
    }

    long size = ftell(fp);
    if (size < 0) {
        close_file(fp);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        close_file(fp);
        return NULL;
    }

    char* buffer = malloc((size_t)size + 1);
    if (!buffer) {
        close_file(fp);
        return NULL;
    }

    size_t read = fread(buffer, 1, (size_t)size, fp);
    buffer[read] = '\0';
    close_file(fp);

    return buffer;
}

static bool is_absolute_path(const char* path)
{
    if (!path || path[0] == '\0')
        return false;

    if (path[0] == '/' || path[0] == '\\')
        return true;

#ifdef _WIN32
    if (isalpha((unsigned char)path[0]) && path[1] == ':' &&
        (path[2] == '\\' || path[2] == '/'))
        return true;
#endif

    return false;
}
