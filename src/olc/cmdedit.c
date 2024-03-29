////////////////////////////////////////////////////////////////////////////////
// skedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "bit.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interp.h"
#include "tables.h"
#include "lookup.h"
#include "olc.h"
#include "recycle.h"

#include "entities/descriptor.h"
#include "entities/player_data.h"

#include "data/mobile_data.h"

#define CMDEDIT(fun)bool fun( Mobile *ch, char *argument )

void save_command_table(void);

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

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const OlcCmdEntry cmd_olc_comm_table[] = {
    { "name",	    0,		            ed_olded,	        U(cmdedit_name)	    },
    { "function",	0,		            ed_olded,	        U(cmdedit_function)	},
    { "level",	    0,		            ed_olded,	        U(cmdedit_level)	},
    { "position",	U(&xCmd.position),	ed_int16lookup,	    U(position_lookup)	},
    { "log",	    U(&xCmd.log),	    ed_flag_set_long,   U(log_flag_table)   },
    { "type",	    U(&xCmd.show),	    ed_flag_set_long,   U(show_flag_table)  },
    { "new",	    0,		            ed_olded,	        U(cmdedit_new)	    },
    { "delete",	    0,		            ed_olded,	        U(cmdedit_delete)	},
    { "list",	    0,  		        ed_olded,	        U(cmdedit_list)	    },
    { "show",	    0,  		        ed_olded,	        U(cmdedit_show)	    },
    { "commands",	0,  		        ed_olded,	        U(show_commands)	},
    { "?",		    0,  		        ed_olded,	        U(show_help)	    },
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

    if (!str_cmp(argument, "save")) {
        send_to_char("Saving command table...", ch);
        save_command_table();
        send_to_char("Done.\n\r", ch);
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
            save_command_table();
        return;
    }

    if (!str_cmp(command, "delete")) {
        if (cmdedit_delete(ch, argument))
            save_command_table();
        return;
    }

    if ((iCmd = cmd_lookup(command)) == -1) {
        send_to_char("CMDEdit : That command does not exist.\n\r", ch);
        return;
    }

    pCmd = &cmd_table[iCmd];

    ch->desc->pEdit = (uintptr_t)pCmd;
    ch->desc->editor = ED_CMD;

    return;
}

CMDEDIT(cmdedit_show)
{
    CmdInfo* pCmd;
    char buf[MIL];

    EDIT_CMD(ch, pCmd);

    sprintf(buf, "Name     : [%s]\n\r", pCmd->name);
    send_to_char(buf, ch);

    sprintf(buf, "Function : [%s]\n\r", cmd_func_name(pCmd->do_fun));
    send_to_char(buf, ch);

    sprintf(buf, "Level    : [%d]\n\r", pCmd->level);
    send_to_char(buf, ch);

    sprintf(buf, "Position : [%s]\n\r", position_table[pCmd->position].name);
    send_to_char(buf, ch);

    sprintf(buf, "Log?     : [%s]\n\r", flag_string(log_flag_table, pCmd->log));
    send_to_char(buf, ch);

    sprintf(buf, "Show?    : [%s]\n\r", flag_string(show_flag_table, pCmd->show));
    send_to_char(buf, ch);

    return false;
}

void list_functions(Buffer* pBuf)
{
    int i;
    char buf[MSL];

    sprintf(buf, "{TNum %-13.13s Num %-13.13s Num %-13.13s Num %-13.13s{x\n\r",
        "Name", "Name", "Name", "Name");
    add_buf(pBuf, buf);

    for (i = 0; cmd_list[i].name; i++) {
        sprintf(buf, "{*%3d{x %-13.13s", i, cmd_list[i].name);
        if (i % 4 == 3)
            strcat(buf, "\n\r");
        else
            strcat(buf, " ");
        add_buf(pBuf, buf);
    }

    if (i % 4 != 0)
        add_buf(pBuf, "\n\r");
}

void list_commands(Buffer* pBuf, int minlev, int maxlev)
{
    char buf[MSL];
    int i, cnt = 0;

    sprintf(buf, "{TNv %-12.12s %-13.13s Pos Log Nv %-12.12s %-13.13s Pos Log{x\n\r",
        "Name", "Function", "Name", "Function");
    add_buf(pBuf, buf);

    for (i = 0; i < max_cmd; ++i) {
        if (cmd_table[i].level < minlev || cmd_table[i].level > maxlev)
            continue;

        sprintf(buf, "%2d {*%-12.12s{x %-13.13s %-3.3s %-3.3s",
            cmd_table[i].level,
            cmd_table[i].name,
            cmd_func_name(cmd_table[i].do_fun),
            position_table[cmd_table[i].position].name,
            &(flag_string(log_flag_table, cmd_table[i].log))[4]);
        if (cnt % 2 == 1)
            strcat(buf, "\n\r");
        else
            strcat(buf, " ");
        add_buf(pBuf, buf);
        cnt++;
    }

    if (cnt % 2 != 0)
        add_buf(pBuf, "\n\r");
}

CMDEDIT(cmdedit_list)
{
    Buffer* pBuf;
    char arg[MIL], arg2[MIL];
    int minlev, maxlev;

    READ_ARG(arg);

    if (IS_NULLSTR(arg) || !is_name(arg, "commands functions")) {
        send_to_char("Syntax : list [commands/functions] [min level] [max level]\n\r", ch);
        return false;
    }

    minlev = 0;
    maxlev = MAX_LEVEL;

    if (!IS_NULLSTR(argument)) {
        READ_ARG(arg2);

        if (!is_number(arg2)) {
            send_to_char("CMDEdit : The level must be a number, obviously.\n\r", ch);
            return false;
        }

        minlev = atoi(arg2);

        if (!IS_NULLSTR(argument))
            maxlev = atoi(argument);
        else
            maxlev = 0;

        if (maxlev < 1 || maxlev > MAX_LEVEL)
            maxlev = minlev;
    }

    pBuf = new_buf();

    if (!str_prefix(arg, "commands"))
        list_commands(pBuf, minlev, maxlev);
    else if (!str_prefix(arg, "functions"))
        list_functions(pBuf);
    else
        add_buf(pBuf, "Idiot!\n\r");

    page_to_char(BUF(pBuf), ch);
    free_buf(pBuf);
    return false;
}

void do_nothing(Mobile* ch, char* argument)
{
    return;
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
    CmdInfo* new_table;
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
    new_table = realloc(cmd_table, sizeof(CmdInfo) * (size_t)(max_cmd + 1));

    if (!new_table) /* realloc failed */
    {
        send_to_char("Realloc failed. Prepare for impact.\n\r", ch);
        return false;
    }

    cmd_table = new_table;

    cmd_table[max_cmd - 1].name = str_dup(argument);
    cmd_table[max_cmd - 1].do_fun = do_nothing;
    cmd_table[max_cmd - 1].position = position_lookup("standing");
    cmd_table[max_cmd - 1].level = MAX_LEVEL;
    cmd_table[max_cmd - 1].log = LOG_ALWAYS;
    cmd_table[max_cmd - 1].show = 0;

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
    CmdInfo* new_table;

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

    if ((new_table = calloc(sizeof(CmdInfo), (size_t)max_cmd + 1)) == NULL) {
        perror("cmdedit_delete: Could not allocate new_table!");
        exit(-1);
    }

    for (i = 0, j = 0; i < max_cmd + 1; i++) {
        if (i != iCmd && j < max_cmd) {
            // copy, increase only if copied
            new_table[j] = cmd_table[i];
            j++;
        }
    }

    free(cmd_table);
    cmd_table = new_table;

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
