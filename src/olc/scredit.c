////////////////////////////////////////////////////////////////////////////////
// scredit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "comm.h"
#include "db.h"
#include "interp.h"
#include "olc.h"
#include "lox_edit.h"
#include "config.h"
#include "fileutils.h"

#include <lox/lox.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREDIT(fun) bool fun(Mobile *ch, char *argument)

static void scredit_list_entries(Mobile* ch);
static bool scredit_parse_when(const char* arg, LoxScriptWhen* when_out);
static void scredit_show_entry(Mobile* ch, size_t index, const LoxScriptEntry* entry);

SCREDIT(scredit_show);
SCREDIT(scredit_list);
SCREDIT(scredit_category);
SCREDIT(scredit_file);
SCREDIT(scredit_when);
SCREDIT(scredit_script);
SCREDIT(scredit_execute);
SCREDIT(scredit_select);
SCREDIT(scredit_create);
SCREDIT(scredit_delete);
SCREDIT(scredit_save);
SCREDIT(scredit_commands);
SCREDIT(scredit_help);

typedef struct scredit_help_entry_t {
    const char* name;
    const char* usage;
    const char* desc;
} ScreditHelpEntry;

static const ScreditHelpEntry scredit_help_table[] = {
    { "list",        "list",                               "Show all registered scripts and their status." },
    { "show",        "show",                               "Display the metadata for the current script." },
    { "select",      "select <index>",                     "Switch to another script entry by index." },
    { "create",      "create <category> <file> [pre|post]", "Add a script entry to the registry." },
    { "delete",      "delete [index]",                     "Remove a script entry (defaults to the current entry)." },
    { "category",    "category <folder>",                  "Set the folder/category for the current script." },
    { "file",        "file <name>",                        "Rename the script file inside the category." },
    { "when",        "when <pre|post>",                    "Choose whether the script executes before or after area load." },
    { "script",      "script",                             "Open the multiline text editor for the script source." },
    { "execute",     "execute",                            "Compile and run the current script immediately." },
    { "save",        "save [olc|json]",                    "Write dirty catalog entries and script files (optionally forcing format)." },
    { "commands",    "commands",                           "List available SCredit commands." },
    { "help",        "help [command]",                     "Show help for SCredit commands (alias '?')." },
    { "?",           "?",                                  "Alias for 'help'." },
    { "done",        "done",                               "Exit the SCredit editor." },
    { NULL,          NULL,                                 NULL }
};

const OlcCmd scredit_table[] = {
    { "show",      scredit_show     },
    { "list",      scredit_list     },
    { "category",  scredit_category },
    { "file",      scredit_file     },
    { "when",      scredit_when     },
    { "script",    scredit_script   },
    { "execute",   scredit_execute  },
    { "select",    scredit_select   },
    { "create",    scredit_create   },
    { "delete",    scredit_delete   },
    { "save",      scredit_save     },
    { "commands",  scredit_commands },
    { "help",      scredit_help     },
    { "?",         scredit_help     },
    { NULL,        0                }
};

void scredit(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;

    strcpy(arg, argument);
    READ_ARG(command);

    if (command[0] == '\0') {
        scredit_show(ch, argument);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    for (cmd = 0; scredit_table[cmd].name != NULL; ++cmd) {
        if (!str_prefix(command, scredit_table[cmd].name)) {
            (*scredit_table[cmd].olc_fun)(ch, argument);
            return;
        }
    }

    interpret(ch, arg);
}

void do_scredit(Mobile* ch, char* argument)
{
    char command[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
        return;

    if (ch->pcdata->security < MIN_SCREDIT_SECURITY) {
        send_to_char(COLOR_INFO "You do not have enough security to edit scripts." COLOR_EOL, ch);
        return;
    }

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: scredit <index>\n\r"
            "        scredit list\n\r"
            "        scredit create <category> <file> [pre|post]\n\r"
            "        scredit delete <index>" COLOR_EOL, ch);
        return;
    }

    READ_ARG(command);

    if (!str_cmp(command, "list")) {
        scredit_list_entries(ch);
        return;
    }

    if (!str_cmp(command, "create")) {
        char category[MAX_INPUT_LENGTH];
        char file[MAX_INPUT_LENGTH];
        char when_buf[MAX_INPUT_LENGTH];

        READ_ARG(category);
        READ_ARG(file);
        READ_ARG(when_buf);

        if (IS_NULLSTR(file)) {
            send_to_char(COLOR_INFO "Usage: scredit create <category> <file> [pre|post]" COLOR_EOL, ch);
            return;
        }

        LoxScriptWhen when = LOX_SCRIPT_WHEN_PRE;
        if (!IS_NULLSTR(when_buf) && !scredit_parse_when(when_buf, &when)) {
            send_to_char(COLOR_INFO "When must be 'pre' or 'post'." COLOR_EOL, ch);
            return;
        }

        LoxScriptEntry* entry = lox_script_entry_create(
            IS_NULLSTR(category) ? NULL : category,
            file,
            when);
        if (!entry) {
            send_to_char(COLOR_INFO "Failed to create script entry." COLOR_EOL, ch);
            return;
        }

        set_editor(ch->desc, ED_SCRIPT, (uintptr_t)(lox_script_entry_count() - 1));
        scredit_show(ch, "");
        return;
    }

    if (!str_cmp(command, "delete")) {
        char index_arg[MAX_INPUT_LENGTH];
        READ_ARG(index_arg);

        if (IS_NULLSTR(index_arg) || !is_number(index_arg)) {
            send_to_char(COLOR_INFO "Usage: scredit delete <index>" COLOR_EOL, ch);
            return;
        }

        size_t index = (size_t)atoi(index_arg) - 1;
        if (index >= lox_script_entry_count() || (int)index < 0) {
            send_to_char(COLOR_INFO "That script index does not exist." COLOR_EOL, ch);
            return;
        }

        LoxScriptEntry* entry = lox_script_entry_get(index);
        const char* file = entry && entry->file ? entry->file : "";
        if (!lox_script_entry_delete(index)) {
            send_to_char(COLOR_INFO "Unable to delete that script entry." COLOR_EOL, ch);
            return;
        }

        printf_to_char(ch, COLOR_INFO "Deleted script %zu (%s)." COLOR_EOL, index, file);
        return;
    }

    if (!is_number(command)) {
        send_to_char(COLOR_INFO "scredit: invalid index." COLOR_EOL, ch);
        return;
    }

    int index = atoi(command) - 1;
    if (index < 0 || (size_t)index >= lox_script_entry_count()) {
        send_to_char(COLOR_INFO "That script index does not exist." COLOR_EOL, ch);
        return;
    }

    set_editor(ch->desc, ED_SCRIPT, (uintptr_t)index);
    scredit_show(ch, argument);
}

SCREDIT(scredit_show)
{
    LoxScriptEntry* entry;
    EDIT_SCRIPT(ch, entry);

    if (!entry) {
        send_to_char(COLOR_INFO "No script selected." COLOR_EOL, ch);
        return false;
    }

    scredit_show_entry(ch, (size_t)ch->desc->pEdit, entry);
    return false;
}

SCREDIT(scredit_list)
{
    scredit_list_entries(ch);
    return false;
}

SCREDIT(scredit_category)
{
    LoxScriptEntry* entry;
    EDIT_SCRIPT(ch, entry);

    if (!entry) {
        send_to_char(COLOR_INFO "No script selected." COLOR_EOL, ch);
        return false;
    }

    char arg[MAX_INPUT_LENGTH];
    READ_ARG(arg);

    if (IS_NULLSTR(arg)) {
        send_to_char(COLOR_INFO "Usage: category <folder>" COLOR_EOL, ch);
        return false;
    }

    if (!lox_script_entry_set_category(entry, arg)) {
        send_to_char(COLOR_INFO "Failed to set category." COLOR_EOL, ch);
        return false;
    }

    printf_to_char(ch, COLOR_INFO "Category set to '%s'." COLOR_EOL, arg);
    return false;
}

SCREDIT(scredit_file)
{
    LoxScriptEntry* entry;
    EDIT_SCRIPT(ch, entry);

    if (!entry) {
        send_to_char(COLOR_INFO "No script selected." COLOR_EOL, ch);
        return false;
    }

    char arg[MAX_INPUT_LENGTH];
    READ_ARG(arg);

    if (IS_NULLSTR(arg)) {
        send_to_char(COLOR_INFO "Usage: file <name>" COLOR_EOL, ch);
        return false;
    }

    if (!lox_script_entry_set_file(entry, arg)) {
        send_to_char(COLOR_INFO "Failed to set file name." COLOR_EOL, ch);
        return false;
    }

    printf_to_char(ch, COLOR_INFO "File set to '%s'." COLOR_EOL, arg);
    return false;
}

SCREDIT(scredit_when)
{
    LoxScriptEntry* entry;
    EDIT_SCRIPT(ch, entry);

    if (!entry) {
        send_to_char(COLOR_INFO "No script selected." COLOR_EOL, ch);
        return false;
    }

    char arg[MAX_INPUT_LENGTH];
    READ_ARG(arg);

    if (IS_NULLSTR(arg)) {
        send_to_char(COLOR_INFO "Usage: when <pre|post>" COLOR_EOL, ch);
        return false;
    }

    LoxScriptWhen when;
    if (!scredit_parse_when(arg, &when)) {
        send_to_char(COLOR_INFO "When must be 'pre' or 'post'." COLOR_EOL, ch);
        return false;
    }

    lox_script_entry_set_when(entry, when);
    printf_to_char(ch, COLOR_INFO "Set boot phase to '%s'." COLOR_EOL, lox_script_when_name(when));
    return false;
}

SCREDIT(scredit_script)
{
    LoxScriptEntry* entry;
    EDIT_SCRIPT(ch, entry);

    if (!entry) {
        send_to_char(COLOR_INFO "No script selected." COLOR_EOL, ch);
        return false;
    }

    if (!entry->source) {
        if (!entry->has_source) {
            if (!lox_script_entry_update_source(entry, "")) {
                send_to_char(COLOR_INFO "Unable to initialize script contents." COLOR_EOL, ch);
                return false;
            }
        }
        else if (!lox_script_entry_ensure_source(entry)) {
            send_to_char(COLOR_INFO "Unable to load script contents." COLOR_EOL, ch);
            return false;
        }
    }

    ObjString* text = lox_string(entry->source ? entry->source : "");
    lox_script_append(ch, text);
    return false;
}

SCREDIT(scredit_execute)
{
    LoxScriptEntry* entry;
    EDIT_SCRIPT(ch, entry);

    if (!entry) {
        send_to_char(COLOR_INFO "No script selected." COLOR_EOL, ch);
        return false;
    }

    if (!lox_script_entry_execute(entry)) {
        send_to_char(COLOR_INFO "Failed to execute script (see log for details)." COLOR_EOL, ch);
        return false;
    }

    send_to_char(COLOR_INFO "Script executed." COLOR_EOL, ch);
    return false;
}

SCREDIT(scredit_select)
{
    char arg[MAX_INPUT_LENGTH];
    READ_ARG(arg);

    if (IS_NULLSTR(arg) || !is_number(arg)) {
        send_to_char(COLOR_INFO "Usage: select <index>" COLOR_EOL, ch);
        return false;
    }

    size_t index = (size_t)atoi(arg) - 1;
    if (index >= lox_script_entry_count() || (int)index < 0) {
        send_to_char(COLOR_INFO "That script index does not exist." COLOR_EOL, ch);
        return false;
    }

    ch->desc->pEdit = (uintptr_t)index;
    scredit_show(ch, "");
    return false;
}

SCREDIT(scredit_create)
{
    char category[MAX_INPUT_LENGTH];
    char file[MAX_INPUT_LENGTH];
    char when_buf[MAX_INPUT_LENGTH];

    READ_ARG(category);
    READ_ARG(file);
    READ_ARG(when_buf);

    if (IS_NULLSTR(file)) {
        send_to_char(COLOR_INFO "Usage: create <category> <file> [pre|post]" COLOR_EOL, ch);
        return false;
    }

    LoxScriptWhen when = LOX_SCRIPT_WHEN_PRE;
    if (!IS_NULLSTR(when_buf) && !scredit_parse_when(when_buf, &when)) {
        send_to_char(COLOR_INFO "When must be 'pre' or 'post'." COLOR_EOL, ch);
        return false;
    }

    LoxScriptEntry* entry = lox_script_entry_create(
        IS_NULLSTR(category) ? NULL : category,
        file,
        when);
    if (!entry) {
        send_to_char(COLOR_INFO "Failed to create script entry." COLOR_EOL, ch);
        return false;
    }

    ch->desc->pEdit = (uintptr_t)(lox_script_entry_count() - 1);
    scredit_show(ch, "");
    return false;
}

SCREDIT(scredit_delete)
{
    size_t current = (size_t)ch->desc->pEdit;
    size_t target = current;

    char arg[MAX_INPUT_LENGTH];
    READ_ARG(arg);

    if (!IS_NULLSTR(arg)) {
        if (!is_number(arg)) {
            send_to_char(COLOR_INFO "Usage: delete [index]" COLOR_EOL, ch);
            return false;
        }
        target = (size_t)atoi(arg) - 1;
    }

    size_t count = lox_script_entry_count();
    if (target >= count) {
        send_to_char(COLOR_INFO "That script index does not exist." COLOR_EOL, ch);
        return false;
    }

    if (!lox_script_entry_delete(target)) {
        send_to_char(COLOR_INFO "Unable to delete that script entry." COLOR_EOL, ch);
        return false;
    }

    send_to_char(COLOR_INFO "Script deleted." COLOR_EOL, ch);

    count = lox_script_entry_count();
    if (count == 0) {
        edit_done(ch);
        return false;
    }

    if (current == target) {
        if (target >= count)
            target = count - 1;
        ch->desc->pEdit = (uintptr_t)target;
        scredit_show(ch, "");
    }
    else if (current > target) {
        ch->desc->pEdit = (uintptr_t)(current - 1);
    }

    return false;
}

SCREDIT(scredit_save)
{
    char arg[MAX_INPUT_LENGTH];
    READ_ARG(arg);

    const char* requested_ext = NULL;
    bool force_format = false;

    if (!IS_NULLSTR(arg)) {
        if (!str_cmp(arg, "json")) {
            requested_ext = ".json";
            force_format = true;
        }
        else if (!str_cmp(arg, "olc")) {
            requested_ext = ".olc";
            force_format = true;
        }
        else {
            send_to_char(COLOR_INFO "Usage: save [olc|json]" COLOR_EOL, ch);
            return false;
        }
    }

    const char* lox_file = cfg_get_lox_file();
    const char* ext = strrchr(lox_file, '.');
    bool has_ext = (ext != NULL);

    if (!force_format) {
        if (!has_ext) {
            if (!lox_file_exists()) {
                const char* def = cfg_get_default_format();
                if (def && !str_cmp(def, "json"))
                    requested_ext = ".json";
                else
                    requested_ext = ".olc";
            }
        }
    }

    if (requested_ext != NULL) {
        size_t base_len = has_ext ? (size_t)(ext - lox_file) : strlen(lox_file);
        char newname[MIL];
        snprintf(newname, sizeof(newname), "%.*s%s", (int)base_len, lox_file, requested_ext);
        cfg_set_lox_file(newname);
    }

    save_lox_public_scripts(force_format);
    printf_to_char(ch, COLOR_INFO "Scripts saved to %s." COLOR_EOL, cfg_get_lox_file());
    return false;
}

SCREDIT(scredit_commands)
{
    send_to_char(COLOR_TITLE "SCredit Commands" COLOR_EOL, ch);

    int col = 0;
    for (size_t i = 0; scredit_help_table[i].name != NULL; ++i) {
        printf_to_char(ch, COLOR_ALT_TEXT_1 "%-12.12s" COLOR_CLEAR, scredit_help_table[i].name);
        if (++col % 4 == 0)
            send_to_char("\n\r", ch);
        else
            send_to_char(" ", ch);
    }

    if (col % 4 != 0)
        send_to_char("\n\r", ch);

    send_to_char(COLOR_INFO "Use 'help <command>' for syntax details." COLOR_EOL, ch);
    return false;
}

SCREDIT(scredit_help)
{
    char arg[MAX_INPUT_LENGTH];
    READ_ARG(arg);

    if (IS_NULLSTR(arg)) {
        send_to_char(COLOR_INFO "SCredit manages public Lox scripts stored on disk." COLOR_EOL, ch);
        send_to_char(COLOR_INFO "General usage: LIST, SELECT <index>, then edit metadata and SCRIPT." COLOR_EOL, ch);
        scredit_commands(ch, "");
        return false;
    }

    for (size_t i = 0; scredit_help_table[i].name != NULL; ++i) {
        if (!str_prefix(arg, scredit_help_table[i].name)) {
            const char* usage = scredit_help_table[i].usage;
            printf_to_char(ch,
                COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%s" COLOR_DECOR_1 "] "
                COLOR_INFO "%s" COLOR_EOL,
                capitalize(scredit_help_table[i].name),
                scredit_help_table[i].desc);
            if (usage && usage[0] != '\0')
                printf_to_char(ch, COLOR_ALT_TEXT_1 "Usage:" COLOR_CLEAR " %s" COLOR_EOL, usage);
            return false;
        }
    }

    printf_to_char(ch, COLOR_INFO "No help found for '%s'." COLOR_EOL, arg);
    send_to_char(COLOR_INFO "Use 'commands' to list valid SCredit commands." COLOR_EOL, ch);
    return false;
}

static void scredit_list_entries(Mobile* ch)
{
    size_t count = lox_script_entry_count();
    send_to_char(COLOR_TITLE "  #    When   Category/File                   Status" COLOR_EOL, ch);
    send_to_char(COLOR_DECOR_2 "===== ====== ================================ =====================" COLOR_EOL, ch);

    for (size_t i = 0; i < count; ++i) {
        const LoxScriptEntry* entry = lox_script_entry_get(i);
        const char* category = (entry && entry->category && entry->category[0] != '\0') ? entry->category : "(root)";
        const char* file = (entry && entry->file && entry->file[0] != '\0') ? entry->file : "(unnamed)";
        const char* when = entry ? lox_script_when_name(entry->when) : "";
        const char* executed = (entry && entry->executed) ? "executed" : "pending";
        const char* dirty = (entry && entry->script_dirty) ? "dirty" : "clean";

        printf_to_char(ch,
            COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%3zu" COLOR_DECOR_1 "] ["
            COLOR_INFO "%-4s" COLOR_DECOR_1 "] " COLOR_ALT_TEXT_1 "%9.9s" COLOR_CLEAR "/"
            COLOR_ALT_TEXT_1 "%-22.22s " COLOR_INFO "%-9s %s" COLOR_EOL,
            i+1, when, category, file, executed, dirty);
    }

    if (count == 0)
        send_to_char(COLOR_INFO "No scripts are registered." COLOR_EOL, ch);
}

static bool scredit_parse_when(const char* arg, LoxScriptWhen* when_out)
{
    if (!arg || !when_out)
        return false;

    if (!str_cmp(arg, "pre")) {
        *when_out = LOX_SCRIPT_WHEN_PRE;
        return true;
    }

    if (!str_cmp(arg, "post")) {
        *when_out = LOX_SCRIPT_WHEN_POST;
        return true;
    }

    return false;
}

static void scredit_show_entry(Mobile* ch, size_t index, const LoxScriptEntry* entry)
{
    if (!entry) {
        send_to_char(COLOR_INFO "No script selected." COLOR_EOL, ch);
        return;
    }

    char path[MAX_INPUT_LENGTH * 3];
    lox_script_entry_source_path(path, sizeof(path), entry);

    const char* category = entry->category && entry->category[0] != '\0' ? entry->category : "(root)";
    const char* file = entry->file && entry->file[0] != '\0' ? entry->file : "(unnamed)";

    olc_print_num(ch, "Index", (int)index+1);
    olc_print_str(ch, "Category", category);
    olc_print_str(ch, "File", file);
    olc_print_str(ch, "Boot phase", lox_script_when_name(entry->when));
    olc_print_yesno(ch, "Executed", entry->executed);
    olc_print_yesno(ch, "Script dirty", entry->script_dirty);
    olc_print_yesno(ch, "Loaded", entry->has_source);
    olc_print_str(ch, "Path", path);
}
