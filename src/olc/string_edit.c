/***************************************************************************
 *  File: string.c                                                         *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/

#include "string_edit.h"

#include "comm.h"
#include "db.h"
#include "olc.h"
#include "stringutils.h"

#include "entities/char_data.h"
#include "entities/descriptor.h"

#include "data/skill.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

char* numlineas(char*);
char* get_line(char*, char*);
char* linedel(char*, int);
char* lineadd(char*, char*, int);

/*****************************************************************************
 Name:		string_edit
 Purpose:	Clears string and puts player into editing mode.
 Called by:	none
 ****************************************************************************/
void string_edit(CharData* ch, char** pString)
{
    send_to_char("{=-========- {*Entering EDIT Mode {=-=========-{_\n\r", ch);
    send_to_char("    Type .h on a new line for help\n\r", ch);
    send_to_char(" Terminate with a @ on a blank line.\n\r", ch);
    send_to_char("{=-=======================================-{x\n\r", ch);

    if (*pString == NULL) {
        *pString = str_dup("");
    }
    else {
        **pString = '\0';
    }

    ch->desc->pString = pString;

    return;
}

/*****************************************************************************
 Name:		string_append
 Purpose:	Puts player into append mode for given string.
 Called by:	(many)olc_act.c
 ****************************************************************************/
void string_append(CharData* ch, char** pString)
{
    send_to_char("{=-========- {*Entering EDIT Mode {=-=========-{_\n\r", ch);
    send_to_char("    Type .h on a new line for help\n\r", ch);
    send_to_char(" Terminate with a @ on a blank line.\n\r", ch);
    send_to_char("{=-=======================================-{x\n\r", ch);

    if (*pString == NULL) {
        *pString = str_dup("");
    }
    send_to_char(numlineas(*pString), ch);

    ch->desc->pString = pString;

    return;
}

/*****************************************************************************
 Name:		string_replace
 Purpose:	Substitutes one string for another.
 Called by:	string_add(string.c) (aedit_builder)olc_act.c.
 ****************************************************************************/
char* string_replace(char* orig, char* old, char* new)
{
    char xbuf[MAX_STRING_LENGTH] = "";
    size_t i;
    size_t old_len = strlen(old);

    if (strlen(orig) - old_len + strlen(new) > MAX_STRING_LENGTH - 10)
        return orig;

    xbuf[0] = '\0';
    strcpy(xbuf, orig);
    char* pos = strstr(orig, old);
    if (pos != NULL) {
        i = strlen(orig) - strlen(pos);
        xbuf[i] = '\0';
        strcat(xbuf, new);
        strcat(xbuf, &orig[i + old_len]);
        free_string(orig);
    }

    return str_dup(xbuf);
}

/*****************************************************************************
 Name:		string_add
 Purpose:	Interpreter for string editing.
 Called by:	game_loop_xxxx(comm.c).
 ****************************************************************************/
void string_add(CharData* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];

    if (*argument == '.') {
        char arg1[MAX_INPUT_LENGTH];
        char arg2[MAX_INPUT_LENGTH];
        char arg3[MAX_INPUT_LENGTH];
        char tmparg3[MIL];

        READ_ARG(arg1);
        argument = first_arg(argument, arg2, false);
        strcpy(tmparg3, argument);
        argument = first_arg(argument, arg3, false);

        if (!str_cmp(arg1, ".c")) {
            write_to_buffer(ch->desc, "String cleared.\n\r", 0);
            free_string(*ch->desc->pString);
            *ch->desc->pString = str_dup("");
            /*            **ch->desc->pString = '\0'; */
            return;
        }

        if (!str_cmp(arg1, ".s")) {
            write_to_buffer(ch->desc, "String so far:\n\r", 0);
            write_to_buffer(ch->desc, numlineas(*ch->desc->pString), 0);
            return;
        }

        if (!str_cmp(arg1, ".r")) {
            if (arg2[0] == '\0') {
                write_to_buffer(ch->desc,
                    "usage:  .r \"old string\" \"new string\"\n\r", 0);
                return;
            }

            *ch->desc->pString = string_replace(*ch->desc->pString, arg2, arg3);
            sprintf(buf, "'%s' replaced with '%s'.\n\r", arg2, arg3);
            write_to_buffer(ch->desc, buf, 0);
            return;
        }

        if (!str_cmp(arg1, ".f")) {
            *ch->desc->pString = format_string(*ch->desc->pString);
            write_to_buffer(ch->desc, "String formatted.\n\r", 0);
            return;
        }

        if (!str_cmp(arg1, ".ld")) {
            *ch->desc->pString = linedel(*ch->desc->pString, atoi(arg2));
            write_to_buffer(ch->desc, "Line deleted.\n\r", 0);
            return;
        }

        if (!str_cmp(arg1, ".li")) {
            if (strlen(*ch->desc->pString) + strlen(tmparg3) >=
                (MAX_STRING_LENGTH - 4)) {
                write_to_buffer(
                    ch->desc,
                    "That would make the full text too long; delete a line first.\n\r",
                    0);
                return;
            }

            *ch->desc->pString = lineadd(*ch->desc->pString, tmparg3, atoi(arg2));
            write_to_buffer(ch->desc, "Line inserted.\n\r", 0);
            return;
        }

        if (!str_cmp(arg1, ".lr")) {
            *ch->desc->pString = linedel(*ch->desc->pString, atoi(arg2));
            *ch->desc->pString = lineadd(*ch->desc->pString, tmparg3, atoi(arg2));
            write_to_buffer(ch->desc, "Line replaced.\n\r", 0);
            return;
        }

        if (!str_cmp(arg1, ".h")) {
            write_to_buffer(ch->desc,
                "Sedit help (commands on blank line):   \n\r"
                ".r 'old' 'new'   - replace a substring \n\r"
                "                   (requires '', \"\") \n\r"
                ".h               - get help (this info)\n\r"
                ".s               - show string so far  \n\r"
                ".f               - (word wrap) string  \n\r"
                ".c               - clear string so far \n\r"
                ".ld <num>        - delete line <num>\n\r"
                ".li <num> <txt>  - insert <txt> on line <num>\n\r"
                ".lr <num> <txt>  - replace line <num> with <txt>\n\r"
                "@                - end string          \n\r",
                0);
            return;
        }

        write_to_buffer(ch->desc, "SEdit:  Invalid dot command.\n\r", 0);
        return;
    }

    if (*argument == '@') {
        ch->desc->pString = NULL;

        switch (ch->desc->editor) {
        case ED_SKILL:
            save_skill_table();
            break;
        }

        if (ch->desc->showstr_head) {
            write_to_buffer(ch->desc,
                "{j[{|!!!{j] You received the following messages while you "
                "were writing:{x\n\r",
                0);
            show_string(ch->desc, "");
        }

        if (ch->desc->editor == ED_PROG && ch->desc->pEdit) {
            int hash = 0;
            MobPrototype* mob;
            MobProg* mp;
            MobProgCode* mpc = (MobProgCode*)ch->desc->pEdit;

            mpc->changed = true;

            for (; hash < MAX_KEY_HASH; hash++) {
                for (mob = mob_prototype_hash[hash]; mob; mob = mob->next)
                    for (mp = mob->mprogs; mp; mp = mp->next)
                        if (mp->vnum == mpc->vnum) {
                            mp->code = mpc->code;
                            printf_to_char(ch, "Updated mob %d.\n\r", mob->vnum);
                        }
            }
        }

        return;
    }

    strcpy(buf, *ch->desc->pString);

    /*
     * Truncate strings to MAX_STRING_LENGTH.
     * --------------------------------------
     */
    if (strlen(buf) + strlen(argument) >= (MAX_STRING_LENGTH - 4)) {
        write_to_buffer(ch->desc, "String too long, last line skipped.\n\r", 0);

        /* Force character out of editing mode. */
        ch->desc->pString = NULL;
        return;
    }

    strcat(buf, argument);
    strcat(buf, "\n\r");
    free_string(*ch->desc->pString);
    *ch->desc->pString = str_dup(buf);

    return;
}

/*
 * Thanks to Kalgen for the new procedure (no more bug!)
 * Original wordwrap() written by Surreality.
 */
/*****************************************************************************
 Name:		format_string
 Purpose:	Special string formating and word-wrapping.
 Called by:	string_add(string.c) (many)olc_act.c
 ****************************************************************************/
char* format_string(char* oldstring /*, bool fSpace */)
{
    char xbuf[MAX_STRING_LENGTH] = { 0 };
    char xbuf2[MAX_STRING_LENGTH] = { 0 };
    char* rdesc;
    int i = 0;
    bool cap = true;

    xbuf[0] = xbuf2[0] = 0;

    i = 0;

#ifdef CHK
#define OLD_CHK CHK
#endif
#define CHK(x)      (x + i > 0 && x + i < MAX_STRING_LENGTH)

    for (rdesc = oldstring; *rdesc; rdesc++) {
        if (*rdesc == '\n') {
                if (CHK(-1) && xbuf[i - 1] != ' ') {
                    xbuf[i] = ' ';
                    i++;
            }
        }
        else if (*rdesc == '\r')
            ;
        else if (*rdesc == ' ') {
            if (CHK(-1) && xbuf[i - 1] != ' ') {
                xbuf[i] = ' ';
                i++;
            }
        }
        else if (*rdesc == ')') {
            if (CHK(-2) && xbuf[i - 1] == ' ' && xbuf[i - 2] == ' ' &&
                (xbuf[i - 3] == '.' || xbuf[i - 3] == '?' || xbuf[i - 3] == '!')) {
                xbuf[i - 2] = *rdesc;
                xbuf[i - 1] = ' ';
                xbuf[i] = ' ';
                i++;
            }
            else {
                xbuf[i] = *rdesc;
                i++;
            }
        }
        else if (*rdesc == '.' || *rdesc == '?' || *rdesc == '!') {
            if (CHK(-3) && xbuf[i - 1] == ' ' && xbuf[i - 2] == ' ' &&
                (xbuf[i - 3] == '.' || xbuf[i - 3] == '?' || xbuf[i - 3] == '!')) {
                if (i >= 2)
                    xbuf[i - 2] = *rdesc;
                if (*(rdesc + 1) != '\"') {
                    xbuf[i - 1] = ' ';
                    xbuf[i] = ' ';
                    i++;
                }
                else {
                    if (i >= 1)
                        xbuf[i - 1] = '\"';
                    xbuf[i] = ' ';
                    xbuf[i + 1] = ' ';
                    i += 2;
                    rdesc++;
                }
            }
            else {
                xbuf[i] = *rdesc;
                if (*(rdesc + 1) != '\"') {
                    xbuf[i + 1] = ' ';
                    xbuf[i + 2] = ' ';
                    i += 3;
                }
                else {
                    xbuf[i + 1] = '\"';
                    xbuf[i + 2] = ' ';
                    xbuf[i + 3] = ' ';
                    i += 4;
                    rdesc++;
                }
            }
            cap = true;
        }
        else {
            xbuf[i] = *rdesc;
            if (cap) {
                cap = false;
                xbuf[i] = UPPER(xbuf[i]);
            }
            i++;
        }
    }
    xbuf[i] = 0;
    strcpy(xbuf2, xbuf);

    rdesc = xbuf2;

    xbuf[0] = 0;

    for (;;) {
        for (i = 0; i < 77; i++) {
            if (!*(rdesc + i))
                break;
        }
        if (i < 77) {
            break;
        }
        for (i = (xbuf[0] ? 76 : 73); i; i--) {
            if (*(rdesc + i) == ' ')
                break;
        }
        if (i) {
            *(rdesc + i) = 0;
            strcat(xbuf, rdesc);
            strcat(xbuf, "\n\r");
            rdesc += (i + 1);
            while (*rdesc == ' ')
                rdesc++;
        }
        else {
            bug("No spaces", 0);
            *(rdesc + 75) = 0;
            strcat(xbuf, rdesc);
            strcat(xbuf, "-\n\r");
            rdesc += 76;
        }
    }
    while (*(rdesc + i) &&
        (*(rdesc + i) == ' ' || *(rdesc + i) == '\n' || *(rdesc + i) == '\r'))
        i--;
    *(rdesc + i + 1) = 0;
    strcat(xbuf, rdesc);
    if (xbuf[strlen(xbuf) - 2] != '\n')
        strcat(xbuf, "\n\r");

    free_string(oldstring);
    return (str_dup(xbuf));
#undef CHK
#ifdef OLD_CHK
#define CHK OLD_CHK
#undef OLD_CHK
#endif
}

/*
 * Used above in string_add.  Because this function does not
 * modify case if fCase is false and because it understands
 * parenthesis, it would probably make a nice replacement
 * for one_argument.
 */
/*****************************************************************************
 Name:		first_arg
 Purpose:	Pick off one argument from a string and return the rest.
                Understands quates, parenthesis (barring ) ('s) and
                percentages.
 Called by:	string_add(string.c)
 ****************************************************************************/
char* first_arg(char* argument, char* arg_first, bool fCase)
{
    char cEnd;

    while (*argument == ' ')
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"' || *argument == '%' ||
        *argument == '(') {
        if (*argument == '(') {
            cEnd = ')';
            argument++;
        }
        else
            cEnd = *argument++;
    }

    while (*argument != '\0') {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        if (fCase)
            *arg_first = LOWER(*argument);
        else
            *arg_first = *argument;
        arg_first++;
        argument++;
    }
    *arg_first = '\0';

    while (*argument == ' ')
        argument++;

    return argument;
}

/*
 * Used in olc_act.c for aedit_builders.
 */
char* string_unpad(char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char* s;

    s = argument;

    while (*s == ' ')
        s++;

    strcpy(buf, s);
    s = buf;

    if (*s != '\0') {
        while (*s != '\0')
            s++;
        s--;

        while (*s == ' ')
            s--;
        s++;
        *s = '\0';
    }

    free_string(argument);
    return str_dup(buf);
}

/*
 * Same as capitalize but changes the pointer's data.
 * Used in olc_act.c in aedit_builder.
 */
char* string_proper(char* argument)
{
    char* s;

    s = argument;

    while (*s != '\0') {
        if (*s != ' ') {
            *s = UPPER(*s);
            while (*s != ' ' && *s != '\0')
                s++;
        }
        else {
            s++;
        }
    }

    return argument;
}

char* rip_arg(char* argument, char* arg_first)
{
    int type;

    while (*argument && ISSPACE(*argument))
        argument++;

    if (ISALPHA(*argument))
        type = 0; /* letters */
    else if (ISDIGIT(*argument))
        type = 1; /* numbers */
    else
        type = 2; /* others */

    while (*argument) {
        if ((ISALPHA(*argument) && type != 0) ||
            (ISDIGIT(*argument) && type != 1) ||
            (!ISALPHA(*argument) && !ISDIGIT(*argument) && type != 2))
            break;

        *arg_first = LOWER(*argument);
        arg_first++;
        argument++;
    }

    *arg_first = '\0';

    while (ISSPACE(*argument))
        argument++;

    return argument;
}

int linecount(char* str)
{
    int cnt = 0;

    while (*str)
        if (*(str++) == '\n')
            cnt++;

    return cnt;
}

char* linedel(char* str, int line)
{
    char buf[MSL * 2] = "";
    char tmpb[MIL] = "";
    int temp = linecount(str), cnt = 1;
    char* strtmp = str;

    buf[0] = '\0';

    while (cnt < temp + 1) {
        strtmp = get_line(strtmp, tmpb);
        if (cnt++ != line) {
            strcat(buf, tmpb);
            strcat(buf, "\n\r");
        }
    }

    free_string(str);
    return str_dup(buf);
}

char* get_line(char* str, char* buf)
{
    int tmp = 0;
    bool found = false;

    while (*str) {
        if (*str == '\n') {
            found = true;
            break;
        }

        buf[tmp++] = *(str++);
    }

    if (found) {
        if (*(str + 1) == '\r')
            str += 2;
        else
            str += 1;
    }

    buf[tmp] = '\0';

    return str;
}

char* numlineas(char* string)
{
    int cnt = 1;
    static char buf[MSL * 2];
    char buf2[MSL + 50], tmpb[MSL];

    buf[0] = '\0';

    while (*string) {
        string = get_line(string, tmpb);
        sprintf(buf2, "{*%2d{x. %s\n\r", cnt++, tmpb);
        strcat(buf, buf2);
    }

    return buf;
}

char* lineadd(char* string, char* newstr, int line)
{
    char* strtmp = string;
    int cnt = 1;
    size_t tmp = 0;
    bool done = false;
    char buf[MSL] = "";

    buf[0] = '\0';

    for (; *strtmp != '\0' || (!done && cnt == line); strtmp++) {
        if (cnt == line && !done) {
            strcat(buf, newstr);
            strcat(buf, "\n\r");
            tmp += strlen(newstr) + 2;
            cnt++;
            done = true;
        }

        buf[tmp++] = *strtmp;

        if (done && *strtmp == '\0')
            break;

        if (*strtmp == '\n') {
            if (*(strtmp + 1) == '\r')
                buf[tmp++] = *(++strtmp);
            cnt++;
        }

        buf[tmp] = '\0';
    }

    free_string(string);
    return str_dup(buf);
}
