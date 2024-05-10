////////////////////////////////////////////////////////////////////////////////
// lox.c
// Handles the "lox" command to handle in-game ad hoc scripts
////////////////////////////////////////////////////////////////////////////////

#include "comm.h"
#include "db.h"

#include "entities/mobile.h"

#include "lox/lox.h"

static void lox_eval(Mobile* ch, char* argument)
{
    char script[MSL];

    // Add a semicolon in case it was omitted.
    sprintf(script, "%s;", argument);

    compile_context.me = ch;
    exec_context.me = ch;
    exec_context.is_repl = true;

    interpret_code(script);

    compile_context.me = NULL;
    exec_context.me = NULL;
    exec_context.is_repl = false;
}

void do_lox(Mobile* ch, char* argument)
{
    static const char* help =
        "USAGE: {*LOX EVAL <script>\n\r"
        "       LOX NEW <name>\n\r"
        "\n\r"
        "Type '{*LOX <option>{x' for more information.\n\r";

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