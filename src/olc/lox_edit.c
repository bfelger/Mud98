////////////////////////////////////////////////////////////////////////////////
// lox_exit.c
////////////////////////////////////////////////////////////////////////////////

#include "lox_edit.h"

#include "olc.h"
#include "string_edit.h"

#include <entities/entity.h>

#include <lox/scanner.h>
#include <lox/vm.h>

#include <comm.h>

char* prettify_lox_script(char* string);

bool olc_edit_lox(Mobile* ch, char* argument)
{
    Entity* entity;

    EDIT_ENTITY(ch, entity);

    if (!IS_NULLSTR(argument)) {
        send_to_char("Syntax : lox\n\r", ch);
        return false;
    }

    lox_script_append(ch, entity->script);

    return true;
}

void lox_script_append(Mobile* ch, ObjString* script)
{
    send_to_char(COLOR_DECOR_2 "-========- " COLOR_ALT_TEXT_1 "Entering EDIT Mode " COLOR_DECOR_2 "-=========-" COLOR_ALT_TEXT_2 "\n\r", ch);
    send_to_char("    Type .h on a new line for help\n\r", ch);
    send_to_char(" Terminate with a @ on a blank line.\n\r", ch);
    send_to_char(COLOR_DECOR_2 "-=======================================-" COLOR_EOL, ch);

    if (script == NULL) {
        script = lox_string("");
    }
    send_to_char(prettify_lox_script(script->chars), ch);

    ch->desc->pLoxScript = script;

    return;
}

static bool lox_script_compile(Mobile* ch, bool assign)
{
    Entity* entity = NULL;
    char* entity_type_name = NULL;

    if (ch->desc->pEdit) {
        switch (ch->desc->editor) {
        case ED_ROOM:
            entity = (Entity*)ch->desc->pEdit;
            entity_type_name = "room";
            break;
        case ED_MOBILE:
            entity = (Entity*)ch->desc->pEdit;
            entity_type_name = "mob";
            break;
        case ED_OBJECT:
            entity = (Entity*)ch->desc->pEdit;
            entity_type_name = "obj";
            break;
        default:
            break;
        }
    }

    if (entity == NULL) {
        bug("Attempt to save Lox script to non-Entity.");
        ch->desc->pLoxScript = NULL;
        return false;
    }

    char class_name[MAX_INPUT_LENGTH];

    sprintf(class_name, "%s_%" PRVNUM, entity_type_name, entity->vnum);

    compile_context.me = ch;
    ObjClass* klass = create_entity_class(entity, class_name, ch->desc->pLoxScript->chars);
    compile_context.me = NULL;
    if (!klass) {
        bugf("lox_script_add: failed to create class for %s %" PRVNUM, 
            entity_type_name, entity->vnum);
        printf_to_char(ch, COLOR_DECOR_1 "[" COLOR_GREEN "***" COLOR_DECOR_1 "]"
            COLOR_INFO "Class \"%s\" [%d] failed to compile.\n\r"
            COLOR_CLEAR, class_name, entity->vnum);
        return false;
    }
    else if (assign) {
        entity->klass = klass;
        entity->script = ch->desc->pLoxScript;
        ch->desc->pLoxScript = NULL;
        printf_to_char(ch, COLOR_DECOR_1 "[" COLOR_GREEN "***" COLOR_DECOR_1 "]"
            COLOR_INFO "Class \"%s\" for %s %d compiled successfully and "
            "assigned.\n\r"
            COLOR_CLEAR, class_name, entity_type_name, entity->vnum);
        if (entity->obj.type == OBJ_ROOM
            && entity->vnum == ch->in_room->header.vnum) {
            ch->in_room->header.klass = entity->klass;
            init_entity_class((Entity*)ch->in_room);
        }
    }
    else {
        printf_to_char(ch, COLOR_DECOR_1 "[" COLOR_GREEN "***" COLOR_DECOR_1 "]"
            COLOR_INFO "Class \"%s\" for %s %d compiled successfully.\n\r"
            COLOR_CLEAR, class_name, entity_type_name, entity->vnum);
    }

    return true;
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

        if (!str_cmp(arg1, ".clear")) {
            write_to_buffer(ch->desc, "Script cleared.\n\r", 0);
            ch->desc->pLoxScript = lox_string("");
            return;
        }
        else if (!str_cmp(arg1, ".s")) {
            write_to_buffer(ch->desc, "Script so far:\n\r", 0);
            send_to_char(prettify_lox_script(ch->desc->pLoxScript->chars), ch);
            return;
        }
        else if (!str_cmp(arg1, ".r")) {
            if (arg2[0] == '\0') {
                write_to_buffer(ch->desc,
                    "usage:  .r \"old script\" \"new script\"\n\r", 0);
                return;
            }

            char* script = string_replace(ch->desc->pLoxScript->chars, arg2, arg3);
            ch->desc->pLoxScript = lox_string(script);
            free_string(script);
            sprintf(buf, "'%s' replaced with '%s'.\n\r", arg2, arg3);
            write_to_buffer(ch->desc, buf, 0);
            return;
        }
        else if (!str_cmp(arg1, ".f")) {
            //TODO: This is coming. But not quite yet.
            //*ch->desc->pLoxScript = format_string(*ch->desc->pLoxScript);
            //write_to_buffer(ch->desc, "Script formatted.\n\r", 0);
            write_to_buffer(ch->desc, "Lox script formatting not yet supported."
                "\n\r", 0);
            return;
        }
        else if (!str_cmp(arg1, ".ld")) {
            char* str = linedel(ch->desc->pLoxScript->chars, atoi(arg2));
            ch->desc->pLoxScript = lox_string(str);
            free_string(str);
            write_to_buffer(ch->desc, "Line deleted.\n\r", 0);
            return;
        }
        else if (!str_cmp(arg1, ".li")) {
            if (ch->desc->pLoxScript->length + strlen(tmparg3) >=
                (MAX_STRING_LENGTH - 4)) {
                write_to_buffer(
                    ch->desc,
                    "That would make the full text too long; delete a line first.\n\r",
                    0);
                return;
            }

            char* str = lineadd(ch->desc->pLoxScript->chars, tmparg3, atoi(arg2));
            ch->desc->pLoxScript = lox_string(str);
            free_string(str);
            write_to_buffer(ch->desc, "Line inserted.\n\r", 0);
            return;
        }
        else if (!str_cmp(arg1, ".lr")) {
            char* str = str_dup(ch->desc->pLoxScript->chars);
            str = linedel(str, atoi(arg2));
            str = lineadd(str, tmparg3, atoi(arg2));
            ch->desc->pLoxScript = lox_string(str);
            write_to_buffer(ch->desc, "Line replaced.\n\r", 0);
            return;
        }
        else if (!str_cmp(arg1, ".v")) {
            lox_script_compile(ch, false);
            return;
        }
        else if (!str_cmp(arg1, ".x")) {
            ch->desc->pLoxScript = NULL;
            send_to_char(COLOR_INFO "Script changes discarded." COLOR_EOL, ch);
            return;
        }
        else if (!str_cmp(arg1, ".h")) {
            send_to_char(COLOR_INFO
                "Lox Script help (commands on blank line):   \n\r"
                COLOR_ALT_TEXT_1 ".r 'old' 'new'   " COLOR_INFO "- replace a substring \n\r"
                COLOR_ALT_TEXT_1 "                 " COLOR_INFO "  (requires '', \"\") \n\r"
                COLOR_ALT_TEXT_1 ".h               " COLOR_INFO "- get help (this info)\n\r"
                COLOR_ALT_TEXT_1 ".s               " COLOR_INFO "- show script so far  \n\r"
                COLOR_ALT_TEXT_1 ".clear           " COLOR_INFO "- clear script so far \n\r"
                COLOR_ALT_TEXT_1 ".v               " COLOR_INFO "- validate byte-code compilation\n\r"
                COLOR_ALT_TEXT_1 ".ld <num>        " COLOR_INFO "- delete line <num>\n\r"
                COLOR_ALT_TEXT_1 ".li <num> <txt>  " COLOR_INFO "- insert <txt> on line <num>\n\r"
                COLOR_ALT_TEXT_1 ".lr <num> <txt>  " COLOR_INFO "- replace line <num> with <txt>\n\r"
                COLOR_ALT_TEXT_1 ".x               " COLOR_INFO "- cancel changes\n\r"
                COLOR_ALT_TEXT_1 "@                " COLOR_INFO "- compile and save script" COLOR_EOL,
                ch);
            return;
        }

        write_to_buffer(ch->desc, "LoxEdit:  Invalid dot command.\n\r", 0);
        return;
    }
    else if (*argument == '@') {
        if (ch->desc->showstr_head) {
            write_to_buffer(ch->desc,
                COLOR_INFO "[" COLOR_DECOR_1 "!!!" COLOR_INFO "] You received the following messages while you "
                "were writing:" COLOR_EOL,
                0);
            show_string(ch->desc, "");
        }

        lox_script_compile(ch, true);

        return;
    }

    strcpy(buf, ch->desc->pLoxScript->chars);

    // Truncate strings to MAX_STRING_LENGTH.
    if (strlen(buf) + strlen(argument) >= (MAX_STRING_LENGTH - 4)) {
        write_to_buffer(ch->desc, "String too long, last line skipped.\n\r", 0);

        /* Force character out of editing mode. */
        ch->desc->pLoxScript = NULL;
        return;
    }

    strcat(buf, argument);
    strcat(buf, "\n");
    ch->desc->pLoxScript = lox_string(buf);

    return;
}
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
        case LOX_FMT_COMMENT:   return COLOR_LOX_COMMENT;
        case LOX_FMT_STRING:    return COLOR_LOX_STRING;
        case LOX_FMT_OPERATOR:  return COLOR_LOX_OPERATOR;
        case LOX_FMT_LITERAL:   return COLOR_LOX_LITERAL;
        case LOX_FMT_KEYWORD:   return COLOR_LOX_KEYWORD;
        default:                return COLOR_CLEAR;
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
            addf_buf(buf, "\n\r" COLOR_ALT_TEXT_1 "%2d" COLOR_CLEAR ". ", line);
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
                    // Check for color code in the comments and escape it.
                    if (*c == COLOR_ESC_CHAR)
                        *ws++ = COLOR_ESC_CHAR;
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
                addf_buf(buf, "%s%s", COLOR_LOX_STRING, ws_buf);
                ws = &ws_buf[0];
                *ws = '\0';
            }
            line++;
            addf_buf(buf, "\n\r" COLOR_ALT_TEXT_1 "%2d" COLOR_CLEAR ". ", line);
            curr_lox_fmt = LOX_FMT_STRING;
            break;
        case COLOR_ESC_CHAR:
            // Escape color codes in strings.
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
        addf_buf(buf, "%s%s", COLOR_LOX_STRING, ws_buf);
        ws = &ws_buf[0];
        *ws = '\0';
    }
}

// In-game Lox syntax highlighting
char* prettify_lox_script(char* script)
{
    INIT_BUF(buf, MAX_STRING_LENGTH);

    init_scanner(script);

    addf_buf(buf, COLOR_ALT_TEXT_1 " 1" COLOR_CLEAR ". ");

    curr_lox_fmt = LOX_FMT_NONE;

    copy_whitespace(buf);

    Token token;

    while ((token = scan_token()).type != TOKEN_EOF) {
        if (token.start[0] == COLOR_ESC_CHAR) {
            addf_buf(buf, "%s%c%c", emit_fmt(LOX_FMT_OPERATOR), COLOR_ESC_CHAR, COLOR_ESC_CHAR);
        }
        else if (token.type < TOKEN_IDENTIFIER) {
            addf_buf(buf, "%s%.*s", emit_fmt(LOX_FMT_OPERATOR), token.length, token.start);
        }
        else if (token.type == TOKEN_STRING || token.type == TOKEN_STRING_INTERP) {
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

void olc_display_lox_info(Mobile* ch, Entity* entity)
{
    if (entity->klass != NULL) {
        olc_print_str_box(ch, "Lox Class", entity->klass->name->chars,
            "Type '" COLOR_INFO "LOX" COLOR_ALT_TEXT_2 "' to edit.");
        //Table* methods = &entity->klass->methods;
        //bool first = true;
        //for (int i = 0; i < methods->capacity; i++) {
        //    Entry* entry = &methods->entries[i];
        //    if (entry->key != NIL_VAL) {
        //        if (first) {
        //            printf_to_char(ch, "%14s : " COLOR_ALT_TEXT_2 "%s" COLOR_EOL,
        //                "Members", string_value(entry->key));
        //            first = false;
        //        }
        //        else {
        //            printf_to_char(ch, "%14s   " COLOR_ALT_TEXT_2 "%s" COLOR_EOL,
        //                "", string_value(entry->key));
        //        }
        //    }
        //}
    }
    else {
        olc_print_str_box(ch, "Lox Class", "(none)", "Type '" COLOR_INFO "LOX"
            COLOR_ALT_TEXT_2 "' to create one.");
    }
}
