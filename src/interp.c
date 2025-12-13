/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St�rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@hypercube.org)                                *
 *      Gabrielle Taylor (gtaylor@hypercube.org)                           *
 *      Brian Moore (zump@rom.org)                                         *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

#include "merc.h"

#include "act_wiz.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interp.h"
#include "stringutils.h"

#include "entities/mobile.h"
#include "entities/descriptor.h"

#include "data/mobile_data.h"
#include "data/player.h"
#include "data/social.h"

#include <lox/lox.h>
#include <lox/vm.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

bool check_social args((Mobile * ch, char* command, char* argument));

// Log-all switch.
bool fLogAll = false;

static void execute_lox_command(const CmdInfo* cmd, Mobile* ch, char* argument)
{
    if (!cmd || !cmd->lox_closure || ch == NULL)
        return;

    const char* args = argument ? argument : "";
    ObjString* arg_str = (args[0] == '\0') ? lox_empty_string
        : copy_string(args, (int)strlen(args));

    ExecContext previous = exec_context;
    exec_context.me = ch;
    exec_context.is_repl = false;

    invoke_closure(cmd->lox_closure, 2, OBJ_VAL(ch), OBJ_VAL(arg_str));

    exec_context = previous;
}

bool cmd_set_lox_closure(CmdInfo* cmd, const char* name)
{
    if (!cmd)
        return false;

    cmd->lox_fun_name = NULL;
    cmd->lox_closure = NULL;

    if (IS_NULLSTR(name))
        return true;

    cmd->lox_fun_name = lox_string(name);

    Value value;
    if (!table_get(&vm.globals, cmd->lox_fun_name, &value)) {
        bugf("cmd_set_lox_closure: unknown Lox callable '%s'", name);
        return false;
    }
    if (!IS_CLOSURE(value)) {
        bugf("cmd_set_lox_closure: '%s' is not callable", name);
        return false;
    }

    cmd->lox_closure = AS_CLOSURE(value);
    return true;
}

// Command table.

void init_command_table(void)
{
    int i, j, cnt, ilet, calc;
    int maxcnt = 0;
    int cntl[27] = { 0 };
    char letter;
    CmdInfo* new_cmd_table;
    CmdInfo** temp_table;

    for (i = 0; i < 27; ++i)
        cntl[i] = 0;

    for (i = 0; i < 26; i++) {
        cnt = 0;

        for (j = 0; j < max_cmd; ++j) {
            letter = LOWER(cmd_table[j].name[0]) - 'a';
            if (i == (int)letter)
                cnt++;
        }

        if (cnt > maxcnt)
            maxcnt = cnt;
    }

    temp_table = calloc(((size_t)maxcnt * 27), sizeof(CmdInfo*));

    for (i = 0; i < (maxcnt * 27); ++i)
        temp_table[i] = NULL;

    for (i = 0; i < max_cmd; ++i) {
        if (!ISALPHA(cmd_table[i].name[0]))
            temp_table[cntl[0]++] = &cmd_table[i];
        else {
            letter = LOWER(cmd_table[i].name[0]);
            ilet = (int)letter;
            ilet -= 'a';
            ilet++;
            cntl[ilet]++;
            calc = (int)((maxcnt * ilet) + cntl[ilet]);
            temp_table[calc] = &cmd_table[i];
        }
    }

    if ((new_cmd_table = malloc(sizeof(CmdInfo) * ((size_t)max_cmd + 1))) == NULL) {
        perror("init_command_table: Could not allocate new_cmd_table!");
        exit(-1);
    }

    i = cnt = 0;
    while (i < (maxcnt * 27)) {
        if (temp_table[i])
            new_cmd_table[cnt++] = *temp_table[i];
        i++;
    }

    new_cmd_table[max_cmd].name = str_dup("");

    free(temp_table);
    free(cmd_table);
    cmd_table = new_cmd_table;
}

int	num_letters[26];

void create_command_table()
{
    int cmd;
    char letter;

    /* Sort table and initialize array */
    init_command_table();
    for (cmd = 0; cmd < 26; cmd++)
        num_letters[cmd] = -1;

    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; ++cmd) {
        letter = LOWER(cmd_table[cmd].name[0]);
        if (!isalpha(letter))
            continue;
        letter -= 'a'; /* range 0...26-1 */
        if (num_letters[(int)letter] == -1)
            num_letters[(int)letter] = cmd;
    }
}

/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void interpret(Mobile* ch, char* argument)
{
    char command[MAX_INPUT_LENGTH] = "";
    char logline[MAX_INPUT_LENGTH] = "";
    int cmd;
    int trust;
    bool found;

    // Strip leading spaces.
    while (ISSPACE(*argument)) 
        argument++;
    if (argument[0] == '\0') 
        return;

    // No hiding.
    REMOVE_BIT(ch->affect_flags, AFF_HIDE);

    // Implement freeze command.
    if (!IS_NPC(ch) && IS_SET(ch->act_flags, PLR_FREEZE)) {
        send_to_char("You're totally frozen!\n\r", ch);
        return;
    }

    /*
     * Grab the command word.
     * Special parsing so ' can be a command,
     *   also no spaces needed after punctuation.
     */
    strcpy(logline, argument);
    if (!ISALPHA(argument[0]) && !ISDIGIT(argument[0])) {
        command[0] = argument[0];
        command[1] = '\0';
        argument++;
        while (ISSPACE(*argument)) 
            argument++;
    }
    else {
        READ_ARG(command);
    }

    // Look for command in command table.
    found = false;
    trust = get_trust(ch);
    for (cmd = 0; !IS_NULLSTR(cmd_table[cmd].name); cmd++) {
        if (command[0] == cmd_table[cmd].name[0]
            && !str_prefix(command, cmd_table[cmd].name)
            && cmd_table[cmd].level <= trust) {
            found = true;
            break;
        }
    }

    // Log and snoop.
    if (cmd_table[cmd].log == LOG_NEVER) 
        strcpy(logline, "");

    if ((!IS_NPC(ch) && IS_SET(ch->act_flags, PLR_LOG)) || fLogAll
        || cmd_table[cmd].log == LOG_ALWAYS) {
        sprintf(log_buf, "Log %s: %s", NAME_STR(ch), logline);
        wiznet(log_buf, ch, NULL, WIZ_SECURE, 0, get_trust(ch));
        log_string(log_buf);
    }

    if (ch->desc != NULL && ch->desc->snoop_by != NULL) {
        write_to_buffer(ch->desc->snoop_by, "% ", 2);
        write_to_buffer(ch->desc->snoop_by, logline, 0);
        write_to_buffer(ch->desc->snoop_by, "\n\r", 2);
    }

    if (!found) {
        // Look for command in socials table.
        if (!check_social(ch, command, argument)) 
            send_to_char("Huh?\n\r", ch);
        return;
    }

    // Character not in position for command?
    if (ch->position < cmd_table[cmd].position) {
        switch (ch->position) {
        case POS_DEAD:
            send_to_char("Lie still; you are DEAD.\n\r", ch);
            break;

        case POS_MORTAL:
        case POS_INCAP:
            send_to_char("You are hurt far too bad for that.\n\r", ch);
            break;

        case POS_STUNNED:
            send_to_char("You are too stunned to do that.\n\r", ch);
            break;

        case POS_SLEEPING:
            send_to_char("In your dreams, or what?\n\r", ch);
            break;

        case POS_RESTING:
            send_to_char("Nah... You feel too relaxed...\n\r", ch);
            break;

        case POS_SITTING:
            send_to_char("Better stand up first.\n\r", ch);
            break;

        case POS_FIGHTING:
            send_to_char("No way!  You are still fighting!\n\r", ch);
            break;

        default:
            break;
        }
        return;
    }

    // Dispatch the command.
    if (cmd_table[cmd].lox_closure)
        execute_lox_command(&cmd_table[cmd], ch, argument);
    else
        (*cmd_table[cmd].do_fun)(ch, argument);

    return;
}

/* function to keep argument safe in all commands -- no static strings */
void do_function(Mobile* ch, DoFunc* do_fun, char* argument)
{
    char* command_string;

    /* copy the string */
    command_string = str_dup(argument);

    /* dispatch the command */
    (*do_fun)(ch, command_string);

    /* free the string */
    free_string(command_string);
}

bool check_social(Mobile* ch, char* command, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;
    int cmd;
    bool found;

    found = false;
    for (cmd = 0; social_table[cmd].name[0] != '\0'; cmd++) {
        if (command[0] == social_table[cmd].name[0]
            && !str_prefix(command, social_table[cmd].name)) {
            found = true;
            break;
        }
    }

    if (!found) return false;

    if (!IS_NPC(ch) && IS_SET(ch->comm_flags, COMM_NOEMOTE)) {
        send_to_char("You are anti-social!\n\r", ch);
        return true;
    }

    switch (ch->position) {
    case POS_DEAD:
        send_to_char("Lie still; you are DEAD.\n\r", ch);
        return true;

    case POS_INCAP:
    case POS_MORTAL:
        send_to_char("You are hurt far too bad for that.\n\r", ch);
        return true;

    case POS_STUNNED:
        send_to_char("You are too stunned to do that.\n\r", ch);
        return true;

    case POS_SLEEPING:
        /*
         * I just know this is the path to a 12" 'if' statement.  :(
         * But two players asked for it already!  -- Furey
         */
        if (!str_cmp(social_table[cmd].name, "snore")) break;
        send_to_char("In your dreams, or what?\n\r", ch);
        return true;

    default:
        break;
    }

    one_argument(argument, arg);
    victim = NULL;
    if (arg[0] == '\0') {
        act(social_table[cmd].others_no_arg, ch, NULL, victim, TO_ROOM);
        act(social_table[cmd].char_no_arg, ch, NULL, victim, TO_CHAR);
    }
    else if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
    }
    else if (victim == ch) {
        act(social_table[cmd].others_auto, ch, NULL, victim, TO_ROOM);
        act(social_table[cmd].char_auto, ch, NULL, victim, TO_CHAR);
    }
    else {
        act(social_table[cmd].others_found, ch, NULL, victim, TO_NOTVICT);
        act(social_table[cmd].char_found, ch, NULL, victim, TO_CHAR);
        act(social_table[cmd].vict_found, ch, NULL, victim, TO_VICT);

        if (!IS_NPC(ch) && IS_NPC(victim) && !IS_AFFECTED(victim, AFF_CHARM)
            && IS_AWAKE(victim) && victim->desc == NULL) {
            switch (number_bits(4)) {
            case 0:

            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                act(social_table[cmd].others_found, victim, NULL, ch,
                    TO_NOTVICT);
                act(social_table[cmd].char_found, victim, NULL, ch, TO_CHAR);
                act(social_table[cmd].vict_found, victim, NULL, ch, TO_VICT);
                break;

            case 9:
            case 10:
            case 11:
            case 12:
                act("$n slaps $N.", victim, NULL, ch, TO_NOTVICT);
                act("You slap $N.", victim, NULL, ch, TO_CHAR);
                act("$n slaps you.", victim, NULL, ch, TO_VICT);
                break;
            }
        }
    }

    return true;
}

// Return true if an argument is completely numeric.
bool is_number(const char* arg)
{
    if (*arg == '\0') return false;

    if (*arg == '+' || *arg == '-') arg++;

    for (; *arg != '\0'; arg++) {
        if (!ISDIGIT(*arg)) return false;
    }

    return true;
}

// Given a string like 14.foo, return 14 and 'foo'
int number_argument(char* argument, char* arg)
{
    char* pdot;
    int number;

    for (pdot = argument; *pdot != '\0'; pdot++) {
        if (*pdot == '.' || *pdot == '*' || *pdot == ' ') {
            number = atoi(argument);
            strcpy(arg, pdot + 1);
            return number;
        }
        else if (!ISDIGIT(*pdot)) {
            break;
        }
    }

    strcpy(arg, argument);
    return 1;
}

// Given a string like 14*foo, return 14 and 'foo'
int mult_argument(char* argument, char* arg)
{
    char* pdot;
    int number;

    for (pdot = argument; *pdot != '\0'; pdot++) {
        if (*pdot == '*' || *pdot == '.' || *pdot == ' ') {
            number = atoi(argument);
            strcpy(arg, pdot + 1);
            return number;
        } 
        else if (!ISDIGIT(*pdot)) {
            break;
        }
    }

    strcpy(arg, argument);
    return 1;
}

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes.
 */
char* one_argument(char* argument, char* arg_first)
{
    char cEnd;

    while (ISSPACE(*argument))
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while (*argument != '\0') {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        *arg_first = LOWER(*argument);
        arg_first++;
        argument++;
    }
    *arg_first = '\0';

    while (ISSPACE(*argument))
        argument++;

    return argument;
}

// Contributed by Alander.
void do_commands(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    int cmd;
    int col;

    col = 0;
    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++) {
        if (cmd_table[cmd].level < LEVEL_HERO
            && cmd_table[cmd].level <= get_trust(ch) && cmd_table[cmd].show) {
            sprintf(buf, "%-12s", cmd_table[cmd].name);
            send_to_char(buf, ch);
            if (++col % 6 == 0) send_to_char("\n\r", ch);
        }
    }

    if (col % 6 != 0) send_to_char("\n\r", ch);
    return;
}

void do_wizhelp(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    int cmd;
    int col;

    col = 0;
    for (cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++) {
        if (cmd_table[cmd].level >= LEVEL_HERO
            && cmd_table[cmd].level <= get_trust(ch) && cmd_table[cmd].show) {
            sprintf(buf, "%-12s", cmd_table[cmd].name);
            send_to_char(buf, ch);
            if (++col % 6 == 0) send_to_char("\n\r", ch);
        }
    }

    if (col % 6 != 0) send_to_char("\n\r", ch);
    return;
}
