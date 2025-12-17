////////////////////////////////////////////////////////////////////////////////
// lox.c
// Handles the "lox" command to handle in-game ad hoc scripts
////////////////////////////////////////////////////////////////////////////////

#include "comm.h"
#include "config.h"
#include "db.h"
#include "fileutils.h"

#include <entities/mobile.h>

#include <lox/lox.h>
#include <lox/vm.h>

#include <persist/lox/lox_persist.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#include <direct.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

extern bool test_output_enabled;

typedef struct lox_script_registry_t {
    LoxScriptEntry* entries;
    size_t count;
    size_t capacity;
} LoxScriptRegistry;

static LoxScriptRegistry lox_scripts = { 0 };
static bool post_scripts_executed = false;
static bool lox_catalog_dirty = false;

static void reset_lox_script_registry();
static LoxScriptEntry* allocate_lox_script_entry();
static bool remove_lox_entry(size_t index, bool mark_dirty);
static bool ensure_lox_script_source(LoxScriptEntry* entry);
static bool run_lox_script_entry(LoxScriptEntry* entry);
static void free_lox_entry_source(LoxScriptEntry* entry, bool force);
static void build_lox_source_path(char* out, size_t out_len, const LoxScriptEntry* entry);
static char* read_lox_source_file(const char* path);
static bool save_lox_script_file(LoxScriptEntry* entry);
static const char* lox_when_to_string(LoxScriptWhen when);
static char* lox_strdup(const char* src);
static void destroy_lox_script_entry(LoxScriptEntry* entry);
static bool ensure_parent_directory(const char* path);

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
    if (!test_output_enabled)
        printf_log("Loading Lox scripts.");

    reset_lox_script_registry();
    PersistResult res = lox_persist_load(cfg_get_lox_file());
    if (!persist_succeeded(res)) {
        bugf("load_lox_public_scripts: failed to load catalog (%s)",
            res.message ? res.message : "unknown error");
        return;
    }

    for (size_t i = 0; i < lox_scripts.count; ++i) {
        LoxScriptEntry* entry = &lox_scripts.entries[i];
        if (!ensure_lox_script_source(entry)) {
            remove_lox_entry(i, false);
            --i;
            continue;
        }

        if (entry->when == LOX_SCRIPT_WHEN_PRE)
            run_lox_script_entry(entry);
    }
}

void lox_script_registry_clear(void)
{
    reset_lox_script_registry();
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

size_t lox_script_entry_count(void)
{
    return lox_scripts.count;
}

LoxScriptEntry* lox_script_entry_get(size_t index)
{
    if (index >= lox_scripts.count)
        return NULL;
    return &lox_scripts.entries[index];
}

int lox_script_entry_index(const LoxScriptEntry* entry)
{
    if (!entry)
        return -1;
    for (size_t i = 0; i < lox_scripts.count; ++i) {
        if (&lox_scripts.entries[i] == entry)
            return (int)i;
    }
    return -1;
}

LoxScriptEntry* lox_script_entry_create(const char* category, const char* file, LoxScriptWhen when)
{
    LoxScriptEntry* entry = allocate_lox_script_entry();
    if (!entry)
        return NULL;

    if (category)
        lox_script_entry_set_category(entry, category);
    if (file)
        lox_script_entry_set_file(entry, file);
    lox_script_entry_set_when(entry, when);

    entry->script_dirty = true;
    entry->executed = false;
    lox_catalog_dirty = true;
    return entry;
}

LoxScriptEntry* lox_script_entry_append_loaded(const char* category, const char* file, LoxScriptWhen when)
{
    LoxScriptEntry* entry = allocate_lox_script_entry();
    if (!entry)
        return NULL;

    if (entry->category)
        free(entry->category);
    entry->category = lox_strdup(category ? category : "");

    if (entry->file)
        free(entry->file);
    entry->file = lox_strdup(file ? file : "");

    entry->when = when;
    entry->script_dirty = false;
    entry->has_source = false;
    entry->executed = false;
    return entry;
}

static bool remove_lox_entry(size_t index, bool mark_dirty)
{
    if (index >= lox_scripts.count)
        return false;

    destroy_lox_script_entry(&lox_scripts.entries[index]);

    size_t remaining = lox_scripts.count - index - 1;
    if (remaining > 0) {
        memmove(&lox_scripts.entries[index],
            &lox_scripts.entries[index + 1],
            remaining * sizeof(LoxScriptEntry));
    }

    --lox_scripts.count;
    memset(&lox_scripts.entries[lox_scripts.count], 0, sizeof(LoxScriptEntry));
    if (mark_dirty)
        lox_catalog_dirty = true;
    return true;
}

bool lox_script_entry_delete(size_t index)
{
    return remove_lox_entry(index, true);
}

bool lox_script_entry_set_category(LoxScriptEntry* entry, const char* category)
{
    if (!entry)
        return false;
    char* copy = lox_strdup(category ? category : "");
    if (entry->category)
        free(entry->category);
    entry->category = copy;
    lox_catalog_dirty = true;
    return true;
}

bool lox_script_entry_set_file(LoxScriptEntry* entry, const char* file)
{
    if (!entry)
        return false;
    char* copy = lox_strdup(file ? file : "");
    if (entry->file)
        free(entry->file);
    entry->file = copy;
    lox_catalog_dirty = true;
    return true;
}

bool lox_script_entry_set_when(LoxScriptEntry* entry, LoxScriptWhen when)
{
    if (!entry)
        return false;
    entry->when = when;
    lox_catalog_dirty = true;
    return true;
}

bool lox_script_entry_update_source(LoxScriptEntry* entry, const char* text)
{
    if (!entry)
        return false;
    const char* src = text ? text : "";
    size_t len = strlen(src);
    char* copy = malloc(len + 1);
    if (!copy) {
        bug("lox_script_entry_update_source: allocation failed", 0);
        return false;
    }
    memcpy(copy, src, len + 1);
    if (entry->source)
        free(entry->source);
    entry->source = copy;
    entry->has_source = true;
    entry->script_dirty = true;
    entry->executed = false;
    return true;
}

bool lox_script_entry_ensure_source(LoxScriptEntry* entry)
{
    if (!entry)
        return false;
    return ensure_lox_script_source(entry);
}

bool lox_script_entry_execute(LoxScriptEntry* entry)
{
    if (!entry)
        return false;
    return run_lox_script_entry(entry);
}

const char* lox_script_when_name(LoxScriptWhen when)
{
    return lox_when_to_string(when);
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
        destroy_lox_script_entry(&lox_scripts.entries[i]);

    lox_scripts.count = 0;
    post_scripts_executed = false;
    lox_catalog_dirty = false;
}

static LoxScriptEntry* allocate_lox_script_entry()
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
    memset(entry, 0, sizeof(*entry));
    entry->category = lox_strdup("");
    entry->file = lox_strdup("");
    entry->when = LOX_SCRIPT_WHEN_PRE;
    entry->source = NULL;
    entry->has_source = false;
    entry->executed = false;
    entry->script_dirty = false;
    return entry;
}

bool lox_script_when_parse(const char* value, LoxScriptWhen* when_out)
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

static bool ensure_lox_script_source(LoxScriptEntry* entry)
{
    if (entry->source) {
        entry->has_source = true;
        return true;
    }

    char path[MAX_INPUT_LENGTH * 3];
    build_lox_source_path(path, sizeof(path), entry);

    entry->source = read_lox_source_file(path);
    if (!entry->source) {
        bugf("load_lox_public_scripts: unable to read %s", path);
        return false;
    }

    entry->has_source = true;
    return true;
}

static bool run_lox_script_entry(LoxScriptEntry* entry)
{
    if (!entry->source && !ensure_lox_script_source(entry))
        return false;

    compile_lox_script(entry->source);
    entry->executed = true;
    free_lox_entry_source(entry, false);
    return true;
}

static void free_lox_entry_source(LoxScriptEntry* entry, bool force)
{
    if (!entry)
        return;
    if (!force && entry->script_dirty)
        return;

    if (entry->source) {
        free(entry->source);
        entry->source = NULL;
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

void lox_script_entry_source_path(char* out, size_t out_len, const LoxScriptEntry* entry)
{
    if (!out || out_len == 0)
        return;
    build_lox_source_path(out, out_len, entry);
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

static char* lox_strdup(const char* src)
{
    const char* text = src ? src : "";
    size_t len = strlen(text);
    char* copy = (char*)malloc(len + 1);
    if (!copy) {
        bug("lox_strdup: allocation failed", 0);
        exit(1);
    }
    memcpy(copy, text, len + 1);
    return copy;
}

static void destroy_lox_script_entry(LoxScriptEntry* entry)
{
    if (!entry)
        return;

    free_lox_entry_source(entry, true);
    if (entry->category) {
        free(entry->category);
        entry->category = NULL;
    }
    if (entry->file) {
        free(entry->file);
        entry->file = NULL;
    }
    entry->when = LOX_SCRIPT_WHEN_PRE;
    entry->has_source = false;
    entry->executed = false;
    entry->script_dirty = false;
}

static bool ensure_parent_directory(const char* path)
{
    if (!path || path[0] == '\0')
        return true;

    char dir[MAX_INPUT_LENGTH * 3];
    snprintf(dir, sizeof(dir), "%s", path);
    for (char* p = dir; *p; ++p)
        if (*p == '\\')
            *p = '/';
    char* slash = strrchr(dir, '/');
    if (!slash)
        return true;
    *slash = '\0';
    if (dir[0] == '\0')
        return true;

    for (char* p = dir + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
#ifdef _MSC_VER
            if (_mkdir(dir) != 0 && errno != EEXIST)
                return false;
#else
            if (mkdir(dir, 0775) != 0 && errno != EEXIST)
                return false;
#endif
            *p = '/';
        }
    }
#ifdef _MSC_VER
    if (_mkdir(dir) != 0 && errno != EEXIST)
        return false;
#else
    if (mkdir(dir, 0775) != 0 && errno != EEXIST)
        return false;
#endif
    return true;
}

void save_lox_public_scripts(bool force_catalog)
{
    bool success = true;

    if (force_catalog || lox_catalog_dirty) {
        PersistResult res = lox_persist_save(cfg_get_lox_file());
        if (!persist_succeeded(res)) {
            bugf("save_lox_public_scripts_if_dirty: failed to save catalog (%s)",
                res.message ? res.message : "unknown error");
            success = false;
        }
        else {
            lox_catalog_dirty = false;
        }
    }

    for (size_t i = 0; i < lox_scripts.count; ++i) {
        LoxScriptEntry* entry = &lox_scripts.entries[i];
        if (entry->script_dirty)
            success = save_lox_script_file(entry) && success;
    }

    if (!success)
        bug("save_lox_public_scripts_if_dirty: failed to save one or more Lox files.", 0);
}

void save_lox_public_scripts_if_dirty()
{
    save_lox_public_scripts(false);
}

static bool save_lox_script_file(LoxScriptEntry* entry)
{
    if (!entry->script_dirty)
        return true;

    if (!entry->source) {
        bug("save_lox_script_file: no script source available.", 0);
        return false;
    }

    char path[MAX_INPUT_LENGTH * 3];
    build_lox_source_path(path, sizeof(path), entry);

    if (!ensure_parent_directory(path)) {
        bugf("save_lox_script_file: unable to create directory for %s", path);
        return false;
    }

    FILE* fp;
    OPEN_OR_RETURN_FALSE(fp = open_write_file(path));

    size_t len = strlen(entry->source);
    if (len > 0)
        fwrite(entry->source, 1, len, fp);
    if (len == 0 || entry->source[len - 1] != '\n')
        fputc('\n', fp);

    close_file(fp);
    entry->script_dirty = false;
    return true;
}

static const char* lox_when_to_string(LoxScriptWhen when)
{
    switch (when) {
    case LOX_SCRIPT_WHEN_POST:
        return "post";
    case LOX_SCRIPT_WHEN_PRE:
    default:
        return "pre";
    }
}
