////////////////////////////////////////////////////////////////////////////////
// cmdedit.c
////////////////////////////////////////////////////////////////////////////////

#include <merc.h>

#include "bit.h"
#include "olc.h"

#include <comm.h>
#include <config.h>
#include <db.h>
#include <handler.h>
#include <interp.h>
#include <tables.h>
#include <lookup.h>
#include <recycle.h>
#include <persist/command/command_persist.h>

#ifdef _MSC_VER
#include <io.h>
#define access _access
#else
#include <unistd.h>
#endif

#ifndef F_OK
#define F_OK 0
#endif

#include <entities/descriptor.h>
#include <entities/player_data.h>

#include <data/mobile_data.h>

#define CMDEDIT(fun)bool fun( Mobile *ch, char *argument )

typedef struct cmd_entry_t {
    char* name;
    DoFunc* function;
} CmdEntry;

int max_cmd;
CmdInfo* cmd_table;
void create_command_table();

#define COMMAND(cmd)	{	#cmd,	cmd	},

const CmdEntry cmd_list[] = {
#include "command.h"
    {	NULL,		NULL		}
};

CmdInfo xCmd;

typedef struct cmdedit_help_entry_t {
    const char* name;
    const char* usage;
    const char* desc;
} CmdeditHelpEntry;

static const CmdeditHelpEntry cmdedit_help_table[] = {
    { "show",      "show",                        "Display the currently selected command." },
    { "list",      "list commands [min max] [filter]\n\r            list functions", "List commands or available C functions. Optional min/max levels and substring filter narrow the command list." },
    { "select",    "select <name>",               "Switch the editor to another command without exiting CMDEdit." },
    { "name",      "name <new name>",             "Rename the current command. Names must be unique." },
    { "function",  "function <do_func>",          "Bind the command to a native C function from command.h." },
    { "lox",       "lox <loxFunc|clear>",         "Bind or clear a Lox closure invoked instead of the native function." },
    { "level",     "level <0..MAX_LEVEL>",        "Set the minimum trust level required to use the command." },
    { "position",  "position <pos>",              "Set the required position (dead, sleeping, standing, etc.)." },
    { "log",       "log <log_normal|log_always|log_never>", "Control auditing of this command." },
    { "type",      "type <category>",             "Assign the command to a 'show' category (communication, combat, ...)." },
    { "new",       "new <name>",                  "Create a new command entry with default values." },
    { "delete",    "delete <name>",               "Remove an existing command." },
    { "save",      "save [json|olc]",             "Persist the command table (optionally forcing a format)." },
    { "commands",  "commands",                    "List available CMDEdit commands with short summaries." },
    { "help",      "help [command]",              "Show contextual help for a command (alias '?')." },
    { "done",      "done",                        "Exit CMDEdit and return to the game." },
    { NULL,        NULL,                          NULL }
};

static const CmdeditHelpEntry* cmdedit_help_lookup(const char* name);
static void cmdedit_print_commands(Mobile* ch);
static void cmdedit_print_help(Mobile* ch, const char* topic);
static bool cmdedit_matches_filter(const CmdInfo* cmd, const char* filter);

static bool save_commands_persist(void)
{
    PersistResult res = command_persist_save(NULL);
    if (!persist_succeeded(res)) {
        bugf("CMDEdit: failed to save command table (%s)",
            res.message ? res.message : "unknown error");
        return false;
    }
    return true;
}

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const OlcCmdEntry cmd_olc_comm_table[] = {
    { "name",	    0,		            ed_olded,	        U(cmdedit_name)	    },
    { "function",	0,		            ed_olded,	        U(cmdedit_function)	},
    { "lox",	    0,		            ed_olded,	        U(cmdedit_lox)	    },
    { "level",	    0,		            ed_olded,	        U(cmdedit_level)	},
    { "position",	U(&xCmd.position),	ed_int16lookup,	    U(position_lookup)	},
    { "log",	    U(&xCmd.log),	    ed_flag_set_long,   U(log_flag_table)   },
    { "type",	    U(&xCmd.show),	    ed_flag_set_long,   U(show_flag_table)  },
    { "new",	    0,		            ed_olded,	        U(cmdedit_new)	    },
    { "delete",	    0,		            ed_olded,	        U(cmdedit_delete)	},
    { "select",     0,                  ed_olded,           U(cmdedit_select)   },
    { "list",	    0,  		        ed_olded,	        U(cmdedit_list)	    },
    { "show",	    0,  		        ed_olded,	        U(cmdedit_show)	    },
    { "save",       0,                  ed_olded,           U(cmdedit_save)     },
    { "commands",	0,  		        ed_olded,	        U(cmdedit_commands)	},
    { "help",       0,                  ed_olded,           U(cmdedit_help)     },
    { "?",		    0,  		        ed_olded,	        U(cmdedit_help)	    },
    { "version",	0,  		        ed_olded,	        U(show_version)	    },
    { NULL,		    0,  		        NULL,		        0		            }
};

char* cmd_func_name(DoFunc* command)
{
    int cmd;

    for (cmd = 0; cmd_list[cmd].name; cmd++)
        if (cmd_list[cmd].function == command)
            return cmd_list[cmd].name;

    return "";
}

DoFunc* cmd_func_lookup(char* arg)
{
    int cmd;

    for (cmd = 0; cmd_list[cmd].name; cmd++)
        if (!str_cmp(cmd_list[cmd].name, arg))
            return cmd_list[cmd].function;

    if (fBootDb) {
        bugf("cmd_func_lookup : function %s does not exist.", arg);
        return do_nothing;
    }

    return NULL;
}

int cmd_lookup(char* arg)
{
    int cmd;

    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++)
        if (LOWER(arg[0]) == LOWER(cmd_table[cmd].name[0])
            && !str_prefix(arg, cmd_table[cmd].name))
            return cmd;

    return -1;
}

void cmdedit(Mobile* ch, char* argument)
{
    if (ch->pcdata->security < MIN_CMDEDIT_SECURITY) {
        send_to_char("CMDEdit: You do not have enough security to edit commands.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (emptystring(argument)) {
        cmdedit_show(ch, argument);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    /* Search Table and Dispatch Command. */
    if (!process_olc_command(ch, argument, cmd_olc_comm_table))
        interpret(ch, argument);

    return;
}

void do_cmdedit(Mobile* ch, char* argument)
{
    const CmdInfo* pCmd;
    char command[MSL];
    int iCmd = 0;

    if (IS_NPC(ch))
        return;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : CMDEdit [command]\n\r", ch);
        return;
    }

    if (ch->pcdata->security < MIN_CMDEDIT_SECURITY) {
        send_to_char("CMDEdit : You do not have enough security to edit commands.\n\r", ch);
        return;
    }

    READ_ARG(command);

    if (!str_cmp(command, "new")) {
        if (cmdedit_new(ch, argument))
            save_commands_persist();
        return;
    }

    if (!str_cmp(command, "delete")) {
        if (cmdedit_delete(ch, argument))
            save_commands_persist();
        return;
    }

    if ((iCmd = cmd_lookup(command)) == -1) {
        send_to_char("CMDEdit : That command does not exist.\n\r", ch);
        return;
    }

    pCmd = &cmd_table[iCmd];

    ch->desc->pEdit = (uintptr_t)pCmd;
    ch->desc->editor = ED_CMD;

    cmdedit_show(ch, "");

    return;
}

CMDEDIT(cmdedit_show)
{
    CmdInfo* pCmd;
    const char* func_name;
    const char* log_name;
    //const char* show_name;

    EDIT_CMD(ch, pCmd);

    func_name = cmd_func_name(pCmd->do_fun);
    if (IS_NULLSTR(func_name))
        func_name = "(not assigned)";

    log_name = flag_string(log_flag_table, pCmd->log);
    if (IS_NULLSTR(log_name))
        log_name = "log_normal";

    //show_name = flag_string(show_flag_table, pCmd->show);
    //if (IS_NULLSTR(show_name))
    //    show_name = "none";

    olc_print_str(ch, "Name", pCmd->name);
    olc_print_str(ch, "Function", func_name);
    olc_print_str(ch, "Lox", (pCmd->lox_fun_name && pCmd->lox_fun_name->chars) ? pCmd->lox_fun_name->chars : "(none)");
    olc_print_num(ch, "Level", pCmd->level);
    olc_print_str(ch, "Position", position_table[pCmd->position].name);
    olc_print_str(ch, "Log", log_name);
    //olc_print_str(ch, "Category", show_name);

    return false;
}

void list_functions(Buffer* pBuf)
{
    int i;
    char buf[MSL];

    sprintf(buf, COLOR_TITLE "Num %-13.13s Num %-13.13s Num %-13.13s Num %-13.13s" COLOR_EOL,
        "Name", "Name", "Name", "Name");
    add_buf(pBuf, buf);

    for (i = 0; cmd_list[i].name; i++) {
        sprintf(buf, COLOR_ALT_TEXT_1 "%3d" COLOR_CLEAR " %-13.13s", i, cmd_list[i].name);
        if (i % 4 == 3)
            strcat(buf, "\n\r");
        else
            strcat(buf, " ");
        add_buf(pBuf, buf);
    }

    if (i % 4 != 0)
        add_buf(pBuf, "\n\r");
}

static bool cmdedit_matches_filter(const CmdInfo* cmd, const char* filter)
{
    if (cmd == NULL || IS_NULLSTR(filter))
        return true;

    if (!str_infix(filter, cmd->name))
        return true;

    const char* func = cmd_func_name(cmd->do_fun);
    if (!IS_NULLSTR(func) && !str_infix(filter, func))
        return true;

    if (cmd->lox_fun_name && cmd->lox_fun_name->chars
        && !str_infix(filter, cmd->lox_fun_name->chars))
        return true;

    return false;
}

static void list_commands(Buffer* pBuf, int minlev, int maxlev, const char* filter)
{
    char buf[MSL];
    int i, cnt = 0;

    sprintf(buf, COLOR_TITLE "Lv %-12.12s Src %-13.13s    Lv %-12.12s Src %-13.13s " COLOR_EOL,
        "Name", "Function", "Name", "Function");
    add_buf(pBuf, buf);
    add_buf(pBuf, COLOR_DECOR_2 "== ============ === ============== " 
        COLOR_DECOR_1 "|" COLOR_DECOR_2 " == ============ === ==============" COLOR_EOL);

    for (i = 0; i < max_cmd; ++i) {
        if (cmd_table[i].level < minlev || cmd_table[i].level > maxlev)
            continue;
        if (!cmdedit_matches_filter(&cmd_table[i], filter))
            continue;

        const char* func;

        if (cmd_table[i].lox_fun_name != NULL && cmd_table[i].lox_fun_name->chars != NULL)
            func = cmd_table[i].lox_fun_name->chars;
        else
            func = cmd_func_name(cmd_table[i].do_fun);

        if (IS_NULLSTR(func))
            func = "(unset)";
        //const char* log_name = flag_string(log_flag_table, cmd_table[i].log);
        //if (IS_NULLSTR(log_name))
        //    log_name = "log_normal";
        //const char* show_name = flag_string(show_flag_table, cmd_table[i].show);
        //if (IS_NULLSTR(show_name))
        //    show_name = "none";

        char src = (cmd_table[i].lox_fun_name && cmd_table[i].lox_fun_name->chars
            && cmd_table[i].lox_fun_name->chars[0] != '\0')
            ? 'L' : 'C';

        sprintf(buf, "%2d " COLOR_ALT_TEXT_1 "%-12.12s" COLOR_CLEAR "  %c "
            COLOR_ALT_TEXT_2 " %-13.13s ",
            cmd_table[i].level,
            cmd_table[i].name,
            src,
            func);
        if (cnt % 2 == 1)
            strcat(buf, COLOR_EOL);
        else
            strcat(buf, COLOR_DECOR_1 " | " COLOR_CLEAR);
        add_buf(pBuf, buf);
        cnt++;
    }

    if (cnt == 0) {
        add_buf(pBuf, COLOR_INFO "No commands matched your criteria." COLOR_EOL);
    }
    else {
        if (cnt % 2 != 0)
            add_buf(pBuf, "\n\r");
        add_buf(pBuf, COLOR_INFO "Src legend: C = native C function, L = Lox script." COLOR_EOL);
    }
}

CMDEDIT(cmdedit_list)
{
    Buffer* pBuf;
    char arg[MIL], arg2[MIL], arg3[MIL];
    char filter[MIL] = "";
    int minlev = 0;
    int maxlev = MAX_LEVEL;

    READ_ARG(arg);

    if (IS_NULLSTR(arg) || !is_name(arg, "commands functions")) {
        send_to_char("Syntax : list commands [min level] [max level] [name filter]\n\r"
                     "         list functions\n\r", ch);
        return false;
    }

    if (!IS_NULLSTR(argument)) {
        READ_ARG(arg2);

        if (!IS_NULLSTR(arg2) && is_number(arg2)) {
            minlev = atoi(arg2);

            if (!IS_NULLSTR(argument)) {
                READ_ARG(arg3);
                if (!IS_NULLSTR(arg3) && is_number(arg3)) {
                    maxlev = atoi(arg3);
                }
                else if (!IS_NULLSTR(arg3)) {
                    strncpy(filter, arg3, sizeof(filter) - 1);
                    filter[sizeof(filter) - 1] = '\0';
                }
            }
        }
        else if (!IS_NULLSTR(arg2)) {
            strncpy(filter, arg2, sizeof(filter) - 1);
            filter[sizeof(filter) - 1] = '\0';
        }

        if (IS_NULLSTR(filter) && !IS_NULLSTR(argument)) {
            strncpy(filter, argument, sizeof(filter) - 1);
            filter[sizeof(filter) - 1] = '\0';
        }

        if (maxlev < 1 || maxlev > MAX_LEVEL)
            maxlev = minlev;
    }

    pBuf = new_buf();

    if (!str_prefix(arg, "commands"))
        list_commands(pBuf, minlev, maxlev, filter);
    else if (!str_prefix(arg, "functions"))
        list_functions(pBuf);
    else
        add_buf(pBuf, "Unknown list target.\n\r");

    page_to_char(BUF(pBuf), ch);
    free_buf(pBuf);
    return false;
}

static const CmdeditHelpEntry* cmdedit_help_lookup(const char* name)
{
    if (IS_NULLSTR(name))
        return NULL;

    for (const CmdeditHelpEntry* entry = cmdedit_help_table; entry->name != NULL; ++entry) {
        if (!str_prefix(name, entry->name))
            return entry;
    }
    return NULL;
}

static void cmdedit_print_commands(Mobile* ch)
{
    Buffer* buf = new_buf();
    char line[MSL];

    add_buf(buf, COLOR_TITLE "CMDEdit Commands" COLOR_EOL);
    for (const CmdeditHelpEntry* entry = cmdedit_help_table; entry->name != NULL; ++entry) {
        snprintf(line, sizeof(line), COLOR_ALT_TEXT_1 "%-10s" COLOR_CLEAR " - %s\n\r",
            entry->name, entry->desc);
        add_buf(buf, line);
    }
    page_to_char(BUF(buf), ch);
    free_buf(buf);
}

static void cmdedit_print_help(Mobile* ch, const char* topic)
{
    if (IS_NULLSTR(topic)) {
        send_to_char("Usage: cmdedit help <command>\n\r", ch);
        cmdedit_print_commands(ch);
        return;
    }

    const CmdeditHelpEntry* entry = cmdedit_help_lookup(topic);
    if (!entry) {
        printf_to_char(ch, "No help available for '%s'.\n\r", topic);
        return;
    }

    printf_to_char(ch, COLOR_TITLE "%s" COLOR_CLEAR "\n\r", entry->name);
    printf_to_char(ch, COLOR_INFO "Usage: " COLOR_CLEAR "%s\n\r", entry->usage);
    printf_to_char(ch, "%s\n\r", entry->desc);
}

void do_nothing(Mobile* ch, char* argument)
{
    return;
}

CMDEDIT(cmdedit_select)
{
    int cmd;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : select <command name>\n\r", ch);
        return false;
    }

    if ((cmd = cmd_lookup(argument)) == -1) {
        send_to_char("CMDEdit : That command does not exist.\n\r", ch);
        return false;
    }

    if (ch->desc == NULL) {
        send_to_char("CMDEdit : You do not have a descriptor.\n\r", ch);
        return false;
    }

    ch->desc->pEdit = U(&cmd_table[cmd]);
    cmdedit_show(ch, "");
    return false;
}

CMDEDIT(cmdedit_save)
{
    char format[MIL];
    READ_ARG(format);

    const char* requested_ext = NULL;
    bool force_format = false;

    if (!IS_NULLSTR(format)) {
        if (!str_cmp(format, "json")) {
            requested_ext = ".json";
            force_format = true;
        }
        else if (!str_cmp(format, "olc")) {
            requested_ext = ".olc";
            force_format = true;
        }
        else {
            send_to_char("Usage : save [json|olc]\n\r", ch);
            return false;
        }
    }

    const char* commands_file = cfg_get_commands_file();
    const char* ext = strrchr(commands_file, '.');
    bool has_ext = (ext != NULL);

    if (!force_format) {
        if (!has_ext) {
            if (access(commands_file, F_OK) != 0) {
                const char* def = cfg_get_default_format();
                if (def && !str_cmp(def, "json"))
                    requested_ext = ".json";
                else
                    requested_ext = ".olc";
            }
        }
    }

    if (requested_ext != NULL) {
        size_t base_len = has_ext ? (size_t)(ext - commands_file) : strlen(commands_file);
        char newname[MIL];
        snprintf(newname, sizeof(newname), "%.*s%s", (int)base_len, commands_file, requested_ext);
        cfg_set_commands_file(newname);
    }

    send_to_char("Saving command table...", ch);
    if (save_commands_persist())
        send_to_char("Done.\n\r", ch);
    else
        send_to_char("Failed.\n\r", ch);
    return true;
}

CMDEDIT(cmdedit_commands)
{
    cmdedit_print_commands(ch);
    return false;
}

CMDEDIT(cmdedit_help)
{
    char topic[MIL];
    READ_ARG(topic);
    cmdedit_print_help(ch, topic);
    return false;
}

CMDEDIT(cmdedit_name)
{
    CmdInfo* pCmd;
    int cmd;

    EDIT_CMD(ch, pCmd);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : name [name]\n\r", ch);
        return false;
    }

    if (cmd_lookup(argument) != -1) {
        send_to_char("CMDEdit : That name is already in use.\n\r", ch);
        return false;
    }

    free_string(pCmd->name);
    pCmd->name = str_dup(argument);

    create_command_table();

    cmd = cmd_lookup(argument);
    if (cmd != -1)
        ch->desc->pEdit = U(&cmd_table[cmd]);
    else
        bugf("Cmdedit_name : cmd_lookup returns -1 for %s!", argument);

    send_to_char("Ok.\n\r", ch);
    return true;
}

CMDEDIT(cmdedit_new)
{
    Descriptor* d;
    Mobile* tch;
    CmdInfo* new_cmd_table;
    int cmd;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : new [name]\n\r", ch);
        return false;
    }

    cmd = cmd_lookup(argument);

    if (cmd != -1) {
        send_to_char("CMDEdit : That command already exists.\n\r", ch);
        return false;
    }

    FOR_EACH(d, descriptor_list) {
        if (d->connected != CON_PLAYING || (tch = CH(d)) == NULL || tch->desc == NULL)
            continue;

        if (tch->desc->editor == ED_CMD)
            edit_done(tch);
    }

    /* reallocate the table */

    max_cmd++;
    new_cmd_table = realloc(cmd_table, sizeof(CmdInfo) * (size_t)(max_cmd + 1));

    if (!new_cmd_table) /* realloc failed */
    {
        send_to_char("Realloc failed. Prepare for impact.\n\r", ch);
        return false;
    }

    cmd_table = new_cmd_table;

    cmd_table[max_cmd - 1].name = str_dup(argument);
    cmd_table[max_cmd - 1].do_fun = do_nothing;
    cmd_table[max_cmd - 1].position = position_lookup("standing");
    cmd_table[max_cmd - 1].level = MAX_LEVEL;
    cmd_table[max_cmd - 1].log = LOG_ALWAYS;
    cmd_table[max_cmd - 1].show = 0;
    cmd_table[max_cmd - 1].lox_fun_name = NULL;
    cmd_table[max_cmd - 1].lox_closure = NULL;

    cmd_table[max_cmd].name = str_dup("");

    create_command_table();

    cmd = cmd_lookup(argument);
    if (cmd != -1)
        ch->desc->pEdit = U(&cmd_table[cmd]);
    else
        bugf("Cmdedit_new : cmd_lookup returns -1 for %s!", argument);

    ch->desc->editor = ED_CMD;

    send_to_char("New command created.\n\r", ch);
    return true;
}

CMDEDIT(cmdedit_delete)
{
    Descriptor* d;
    Mobile* tch;
    int i, j, iCmd;
    CmdInfo* new_cmd_table;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : delete [name]\n\r", ch);
        return false;
    }

    iCmd = cmd_lookup(argument);

    if (iCmd == -1) {
        send_to_char("CMDEdit : That command does not exist.\n\r", ch);
        return false;
    }

    FOR_EACH(d, descriptor_list) {
        if (d->connected != CON_PLAYING || (tch = CH(d)) == NULL || tch->desc == NULL)
            continue;

        if (tch->desc->editor == ED_CMD)
            edit_done(tch);
    }

    if ((new_cmd_table = calloc((size_t)max_cmd + 1, sizeof(CmdInfo))) == NULL) {
        perror("cmdedit_delete: Could not allocate new_cmd_table!");
        exit(-1);
    }

    for (i = 0, j = 0; i < max_cmd + 1; i++) {
        if (i != iCmd && j < max_cmd) {
            // copy, increase only if copied
            new_cmd_table[j] = cmd_table[i];
            j++;
        }
    }

    free(cmd_table);
    cmd_table = new_cmd_table;

    max_cmd--; /* Important :() */

    create_command_table();
    send_to_char("That command is history.\n\r", ch);
    return true;
}

CMDEDIT(cmdedit_function)
{
    CmdInfo* pCmd;
    DoFunc* function;

    EDIT_CMD(ch, pCmd);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : function [function name]\n\r", ch);
        return false;
    }

    if ((function = cmd_func_lookup(argument)) == NULL) {
        send_to_char("CMDEdit : That function does not exist.\n\r", ch);
        return false;
    }

    pCmd->do_fun = function;
    send_to_char("Ok.\n\r", ch);
    return true;
}

CMDEDIT(cmdedit_lox)
{
    CmdInfo* pCmd;

    EDIT_CMD(ch, pCmd);

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : lox [function name|clear]\n\r", ch);
        return false;
    }

    if (!str_cmp(argument, "clear")) {
        pCmd->lox_fun_name = NULL;
        pCmd->lox_closure = NULL;
        send_to_char("Lox function cleared.\n\r", ch);
        return true;
    }

    if (!cmd_set_lox_closure(pCmd, argument)) {
        send_to_char("CMDEdit : Unable to bind that Lox function.\n\r", ch);
        return false;
    }

    send_to_char("Ok.\n\r", ch);
    return true;
}

CMDEDIT(cmdedit_level)
{
    CmdInfo* pCmd;
    LEVEL level;

    EDIT_CMD(ch, pCmd);

    if (IS_NULLSTR(argument) || !is_number(argument)) {
        send_to_char("Syntax : level [0 <= level <= MAX_LEVEL]\n\r", ch);
        return false;
    }

    level = (LEVEL)atoi(argument);
    if (level < 0 || level > MAX_LEVEL) {
        send_to_char("CMDEdit : The level must be between 0 and MAX_LEVEL.\n\r", ch);
        return false;
    }

    if (pCmd->level > ch->level) {
        send_to_char("CMDEdit : You cannot set the level higher than your own...\n\r", ch);
        return false;
    }

    pCmd->level = level;
    send_to_char("Ok.\n\r", ch);
    return true;
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
