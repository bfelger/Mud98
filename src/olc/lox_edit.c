////////////////////////////////////////////////////////////////////////////////
// lox_exit.c
////////////////////////////////////////////////////////////////////////////////

#include "lox_edit.h"

#include "olc.h"
#include "string_edit.h"

#include <lox/scanner.h>

#include <comm.h>

char* prettify_lox_script(char* string);

void lox_script_append(Mobile* ch, char** pScript)
{
    send_to_char("{=-========- {*Entering EDIT Mode {=-=========-{_\n\r", ch);
    send_to_char("    Type .h on a new line for help\n\r", ch);
    send_to_char(" Terminate with a @ on a blank line.\n\r", ch);
    send_to_char("{=-=======================================-{x\n\r", ch);

    if (*pScript == NULL) {
        *pScript = str_dup("");
    }
    send_to_char(prettify_lox_script(*pScript), ch);

    *ch->desc->pLoxScript = str_dup(*pScript);

    return;
}

void lox_script_add(Mobile* ch, char* argument)
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
            write_to_buffer(ch->desc, "Script cleared.\n\r", 0);
            free_string(*ch->desc->pLoxScript);
            *ch->desc->pLoxScript = str_dup("");
            return;
        }

        if (!str_cmp(arg1, ".s")) {
            write_to_buffer(ch->desc, "Script so far:\n\r", 0);
            //write_to_buffer(ch->desc, prettify_lox_script(*ch->desc->pLoxScript), 0);
            send_to_char(prettify_lox_script(*ch->desc->pLoxScript), ch);
            return;
        }

        if (!str_cmp(arg1, ".r")) {
            if (arg2[0] == '\0') {
                write_to_buffer(ch->desc,
                    "usage:  .r \"old script\" \"new script\"\n\r", 0);
                return;
            }

            *ch->desc->pLoxScript = string_replace(*ch->desc->pLoxScript, arg2, arg3);
            sprintf(buf, "'%s' replaced with '%s'.\n\r", arg2, arg3);
            write_to_buffer(ch->desc, buf, 0);
            return;
        }

        if (!str_cmp(arg1, ".f")) {
            *ch->desc->pLoxScript = format_string(*ch->desc->pLoxScript);
            write_to_buffer(ch->desc, "Script formatted.\n\r", 0);
            return;
        }

        if (!str_cmp(arg1, ".ld")) {
            *ch->desc->pLoxScript = linedel(*ch->desc->pLoxScript, atoi(arg2));
            write_to_buffer(ch->desc, "Line deleted.\n\r", 0);
            return;
        }

        if (!str_cmp(arg1, ".li")) {
            if (strlen(*ch->desc->pLoxScript) + strlen(tmparg3) >=
                (MAX_STRING_LENGTH - 4)) {
                write_to_buffer(
                    ch->desc,
                    "That would make the full text too long; delete a line first.\n\r",
                    0);
                return;
            }

            *ch->desc->pLoxScript = lineadd(*ch->desc->pLoxScript, tmparg3, atoi(arg2));
            write_to_buffer(ch->desc, "Line inserted.\n\r", 0);
            return;
        }

        if (!str_cmp(arg1, ".lr")) {
            *ch->desc->pLoxScript = linedel(*ch->desc->pLoxScript, atoi(arg2));
            *ch->desc->pLoxScript = lineadd(*ch->desc->pLoxScript, tmparg3, atoi(arg2));
            write_to_buffer(ch->desc, "Line replaced.\n\r", 0);
            return;
        }

        if (!str_cmp(arg1, ".h")) {
            write_to_buffer(ch->desc,
                "Lox Script help (commands on blank line):   \n\r"
                ".r 'old' 'new'   - replace a substring \n\r"
                "                   (requires '', \"\") \n\r"
                ".h               - get help (this info)\n\r"
                ".s               - show script so far  \n\r"
                ".f               - (word wrap) string  \n\r"
                ".c               - clear script so far \n\r"
                ".ld <num>        - delete line <num>\n\r"
                ".li <num> <txt>  - insert <txt> on line <num>\n\r"
                ".lr <num> <txt>  - replace line <num> with <txt>\n\r"
                "@                - compile script and save\n\r",
                0);
            return;
        }

        write_to_buffer(ch->desc, "LoxEdit:  Invalid dot command.\n\r", 0);
        return;
    }

    if (*argument == '@') {
        if (ch->desc->showstr_head) {
            write_to_buffer(ch->desc,
                "{j[{|!!!{j] You received the following messages while you "
                "were writing:{x\n\r",
                0);
            show_string(ch->desc, "");
        }

        Entity* entity = NULL;
        char* entity_type_name = NULL;

        if (ch->desc->pEdit) {
            switch (ch->desc->editor) {
            case ED_ROOM:
                entity = (Entity*)ch->desc->pEdit;
                entity_type_name = "room";
                break;
            default:
                break;
            }
        }

        if (entity == NULL) {
            bug("Attempt to save Lox script to non-Entity.");
            ch->desc->pLoxScript = NULL;
            return;
        }

        char class_name[MAX_INPUT_LENGTH];
        String* script = lox_string(*ch->desc->pLoxScript);

        sprintf(class_name, "%s_%" PRVNUM, entity_type_name, entity->vnum);

        ObjClass* klass = create_entity_class(entity, class_name, script->chars);
        if (!klass) {
            bugf("lox_script_add: failed to create class for %s %" PRVNUM, entity_type_name, entity->vnum);
        }
        else {
            free_string(*ch->desc->pLoxScript);
            ch->desc->pLoxScript = NULL;
            entity->klass = klass;
            entity->script = script;
        }

        return;
    }

    strcpy(buf, *ch->desc->pLoxScript);

    // Truncate strings to MAX_STRING_LENGTH.
    if (strlen(buf) + strlen(argument) >= (MAX_STRING_LENGTH - 4)) {
        write_to_buffer(ch->desc, "String too long, last line skipped.\n\r", 0);

        /* Force character out of editing mode. */
        ch->desc->pLoxScript = NULL;
        return;
    }

    strcat(buf, argument);
    strcat(buf, "\n\r");
    free_string(*ch->desc->pLoxScript);
    *ch->desc->pLoxScript = str_dup(buf);

    return;
}

#define LOX_CLEAR_FMT       "{x"
#define LOX_COMMENT_FMT     "{6"
#define LOX_STRING_FMT      "{="
#define LOX_OPERATOR_FMT    "{_"
#define LOX_LITERAL_FMT     "{9"
#define LOX_KEYWORD_FMT     "{|"

typedef enum {
    LOX_FMT_NONE,
    LOX_FMT_COMMENT,
    LOX_FMT_STRING,
    LOX_FMT_OPERATOR,
    LOX_FMT_LITERAL,
    LOX_FMT_KEYWORD,
} LoxFmt;

LoxFmt curr_lox_fmt;

static const char* emit_fmt(LoxFmt fmt)
{
    if (curr_lox_fmt != fmt) {
        curr_lox_fmt = fmt;
        switch (fmt) {
        case LOX_FMT_COMMENT:   return LOX_COMMENT_FMT;
        case LOX_FMT_STRING:    return LOX_STRING_FMT;
        case LOX_FMT_OPERATOR:  return LOX_OPERATOR_FMT;
        case LOX_FMT_LITERAL:   return LOX_LITERAL_FMT;
        case LOX_FMT_KEYWORD:   return LOX_KEYWORD_FMT;
        default:                return LOX_CLEAR_FMT;
        }
    }

    return "";
}

extern Scanner scanner;

// Called from prettify_lox_script
static void copy_whitespace(Buffer* buf)
{
    char ws_buf[MSL];
    const char* c = scanner.current;
    char* ws = &ws_buf[0];
    int line = scanner.line;

    *ws = '\0';

    while (*c != '\0') {
        switch (*c) {
        case '\r':
            c++;
            break;
        case '\n':
            if (ws_buf[0] != '\0') {
                *ws = '\0';
                addf_buf(buf, ws_buf);
                ws = &ws_buf[0];
                *ws = '\0';
            }
            c++;
            line++;
            addf_buf(buf, "\n\r{*%2d{x. ", line);
            curr_lox_fmt = LOX_FMT_NONE;
            break;
        case ' ':
        case '\t':
            *ws++ = *c++;
            break;
        case '/':
            if (*(c + 1) == '/') {
                if (ws_buf[0] != '\0') {
                    *ws = '\0';
                    addf_buf(buf, ws_buf);
                    ws = &ws_buf[0];
                    *ws = '\0';
                }
                c += 2;
                while (*c != '\n' && *c != '\r' && *c != '\0') {
                    // Check for a '{' in the comments; emit '{{'.
                    if (*c == '{')
                        *ws++ = '{';
                    *ws++ = *c++;
                }
                *ws = '\0';
                addf_buf(buf, "%s//%s", emit_fmt(LOX_FMT_COMMENT), ws_buf);
                ws = &ws_buf[0];
                *ws = '\0';
                break;
            }
            // Not a comment; '/' is an operator, so fall through and get out.
        default:
            goto copy_whitespace_end;
        }
    }

copy_whitespace_end:
    if (ws_buf[0] != '\0') {
        *ws = '\0';
        addf_buf(buf, ws_buf);
        ws = &ws_buf[0];
        *ws = '\0';
    }
}

static void copy_lox_literal_string(Buffer* buf, Token* token)
{
    char ws_buf[MSL];
    char* ws = &ws_buf[0];
    int line = scanner.line;
    char c;

    curr_lox_fmt = LOX_FMT_STRING;

    *ws = '\0';

    for (int i = 0; i < token->length; i++) {
        c = token->start[i];
        switch (c) {
        case '\r':
            break;
        case '\n':
            if (ws_buf[0] != '\0') {
                *ws = '\0';
                addf_buf(buf, "%s%s", LOX_STRING_FMT, ws_buf);
                ws = &ws_buf[0];
                *ws = '\0';
            }
            line++;
            addf_buf(buf, "\n\r{*%2d{x. ", line);
            curr_lox_fmt = LOX_FMT_STRING;
            break;
        case '{':
            // Escape braces in strings.
            *ws++ = c;
            *ws++ = c;
            break;
        default:
            *ws++ = c;
            break;
        }
    }

    if (ws_buf[0] != '\0') {
        *ws = '\0';
        addf_buf(buf, "%s%s", LOX_STRING_FMT, ws_buf);
        ws = &ws_buf[0];
        *ws = '\0';
    }
}

// In-game Lox syntax highlighting
char* prettify_lox_script(char* script)
{
    INIT_BUF(buf, MAX_STRING_LENGTH);

    init_scanner(script);

    addf_buf(buf, "{* 1{x. ");

    curr_lox_fmt = LOX_FMT_NONE;

    copy_whitespace(buf);

    Token token;

    while ((token = scan_token()).type != TOKEN_EOF) {
        if (token.type == TOKEN_LEFT_BRACE) {
            addf_buf(buf, "%s{{", emit_fmt(LOX_FMT_OPERATOR));
        }
        else if (token.type < TOKEN_IDENTIFIER) {
            addf_buf(buf, "%s%.*s", emit_fmt(LOX_FMT_OPERATOR), token.length, token.start);
        }
        else if (token.type == TOKEN_STRING || token.type == TOKEN_STRING_INTERP) {
            //addf_buf(buf, "%s%.*s", emit_fmt(LOX_FMT_STRING), token.length, token.start);
            copy_lox_literal_string(buf, &token);
        }
        else if (token.type == TOKEN_TRUE || token.type == TOKEN_FALSE || token.type == TOKEN_NIL || token.type == TOKEN_INT) {
            addf_buf(buf, "%s%.*s", emit_fmt(LOX_FMT_LITERAL), token.length, token.start);
        }
        else if (token.type == TOKEN_IDENTIFIER) {
            addf_buf(buf, "%s%.*s", emit_fmt(LOX_FMT_NONE), token.length, token.start);
        }
        else {
            addf_buf(buf, "%s%.*s", emit_fmt(LOX_FMT_KEYWORD), token.length, token.start);
        }

        copy_whitespace(buf);
    }

    char* ret = str_dup(buf->string);

    free_buf(buf);
    return ret;
}

void olc_display_lox_info(Mobile* ch, Entity* entity, Buffer* out)
{
    if (entity->klass != NULL) {
        addf_buf(out, "Lox Class:   {|[{*%s{|]{x\n\r", entity->klass->name->chars);
        Table* methods = &entity->klass->methods;
        bool first = true;
        for (int i = 0; i < methods->capacity; i++) {
            Entry* entry = &methods->entries[i];
            if (entry->key != NIL_VAL) {
                if (first) {
                    addf_buf(out, " - Members:   {_%s{x\n\r", string_value(entry->key));
                    first = false;
                }
                else {
                    addf_buf(out, "              {_%s{x\n\r", string_value(entry->key));
                }
            }
        }
    }
}
