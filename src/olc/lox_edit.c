////////////////////////////////////////////////////////////////////////////////
// lox_exit.c
////////////////////////////////////////////////////////////////////////////////

#include "lox_edit.h"

#include "olc.h"
#include "string_edit.h"

#include <entities/entity.h>

#include <lox/lox.h>
#include <lox/scanner.h>
#include <lox/vm.h>

#include <comm.h>
#include <stringutils.h>

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

    push_editor(ch->desc, ED_LOX_SCRIPT, (uintptr_t)script);

    return;
}

static bool lox_script_entry_compile(Mobile* ch, bool assign);

static bool lox_script_compile(Mobile* ch, bool assign)
{
    if (get_editor(ch->desc) == ED_SCRIPT)
        return lox_script_entry_compile(ch, assign);

    Entity* entity = NULL;
    char* entity_type_name = NULL;

    if (get_pEdit(ch->desc)) {
        switch (get_editor(ch->desc)) {
        case ED_ROOM:
            entity = (Entity*)get_pEdit(ch->desc);
            entity_type_name = "room";
            break;
        case ED_MOBILE:
            entity = (Entity*)get_pEdit(ch->desc);
            entity_type_name = "mob";
            break;
        case ED_OBJECT:
            entity = (Entity*)get_pEdit(ch->desc);
            entity_type_name = "obj";
            break;
        default:
            break;
        }
    }

    if (entity == NULL) {
        bug("Attempt to save Lox script to non-Entity.");
        pop_editor(ch->desc);
        return false;
    }

    ObjString* pLoxScript = get_lox_pEdit(ch->desc);
    char class_name[MAX_INPUT_LENGTH];

    sprintf(class_name, "%s_%" PRVNUM, entity_type_name, entity->vnum);

    compile_context.me = ch;
    ObjClass* klass = create_entity_class(entity, class_name, pLoxScript->chars);
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
        entity->script = pLoxScript;
        pop_editor(ch->desc);
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

static bool lox_script_entry_compile(Mobile* ch, bool assign)
{
    LoxScriptEntry* entry = NULL;
    EDIT_SCRIPT(ch, entry);

    if (!entry) {
        bug("lox_script_entry_compile: missing entry context.");
        pop_editor(ch->desc);
        return false;
    }

    ObjString* pLoxScript = get_lox_pEdit(ch->desc);

    if (!assign) {
        InterpretResult result = interpret_code(pLoxScript->chars);
        if (result == INTERPRET_OK) {
            printf_to_char(ch, COLOR_DECOR_1 "[" COLOR_GREEN "***" COLOR_DECOR_1 "]"
                COLOR_INFO "Script compiled successfully.\n\r" COLOR_CLEAR);
            return true;
        }

        printf_to_char(ch, COLOR_DECOR_1 "[" COLOR_RED "!!!" COLOR_DECOR_1 "]"
            COLOR_INFO "Script failed to compile (check log for details).\n\r" COLOR_CLEAR);
        return false;
    }

    if (!lox_script_entry_update_source(entry, pLoxScript->chars)) {
        printf_to_char(ch, COLOR_DECOR_1 "[" COLOR_RED "!!!" COLOR_DECOR_1 "]"
            COLOR_INFO "Failed to update script contents.\n\r" COLOR_CLEAR);
        return false;
    }

    pop_editor(ch->desc);
    entry->executed = false;
    printf_to_char(ch, COLOR_DECOR_1 "[" COLOR_GREEN "***" COLOR_DECOR_1 "]"
        COLOR_INFO "Script saved. Use the EXECUTE command in SCREDIT to run it.\n\r"
        COLOR_CLEAR);
    return true;
}

void lox_script_add(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    ObjString* pLoxScript = get_lox_pEdit(ch->desc);

    if (pLoxScript == NULL) {
        bug("lox_script_add: NULL pLoxScript");
        pop_editor(ch->desc);
        return;
    }

    if (*argument == '.') {
        char arg1[MAX_INPUT_LENGTH];

        READ_ARG(arg1);

        if (!str_cmp(arg1, ".clear")) {
            printf_to_char(ch, "Script cleared.\n\r");
            set_lox_pEdit(ch->desc, lox_string(""));
            return;
        }
        else if (!str_cmp(arg1, ".s")) {
            printf_to_char(ch, "Script so far:\n\r");
            send_to_char(prettify_lox_script(pLoxScript->chars), ch);
            return;
        }
        else if (!str_cmp(arg1, ".r")) {
            char arg2[MAX_INPUT_LENGTH];
            char arg3[MAX_INPUT_LENGTH];
            argument = first_arg(argument, arg2, false);
            argument = first_arg(argument, arg3, false);

            if (arg2[0] == '\0') {
                printf_to_char(ch, "usage:  .r \"old script\" \"new script\"\n\r");
                return;
            }

            char* script = string_replace(pLoxScript->chars, arg2, arg3);
            set_lox_pEdit(ch->desc, lox_string(script));
            free_string(script);
            sprintf(buf, "'%s' replaced with '%s'.\n\r", arg2, arg3);
            write_to_buffer(ch->desc, buf, 0);
            return;
        }
        else if (!str_cmp(arg1, ".f")) {
            //TODO: This is coming. But not quite yet.
            printf_to_char(ch, "Lox script formatting not yet supported.\n\r");
            return;
        }
        else if (!str_cmp(arg1, ".ld")) {
            char arg2[MAX_INPUT_LENGTH];
            argument = first_arg(argument, arg2, false);
            char* str = linedel(pLoxScript->chars, atoi(arg2));
            set_lox_pEdit(ch->desc, lox_string(str));
            free_string(str);
            printf_to_char(ch, "Line deleted.\n\r");
            return;
        }
        else if (!str_cmp(arg1, ".li")) {
            char arg2[MAX_INPUT_LENGTH];
            argument = first_arg(argument, arg2, false);
            int line_num = atoi(arg2);

            // Rewind argument to capture the padding spaces (minus 1).
            while (ISSPACE(*(argument-1)))
                argument--;

            if (pLoxScript->length + strlen(argument) >=
                (MAX_STRING_LENGTH - 4)) {
                printf_to_char(ch, "That would make the full text too long; delete a line first.\n\r");
                return;
            }

            char* str = lineadd(pLoxScript->chars, argument, line_num);
            set_lox_pEdit(ch->desc, lox_string(str));
            free_string(str);
            printf_to_char(ch, "Line inserted.\n\r");
            return;
        }
        else if (!str_cmp(arg1, ".lr")) {
            
            char arg2[MAX_INPUT_LENGTH];
            char tmparg3[MIL];

            argument = first_arg(argument, arg2, false);
            strcpy(tmparg3, argument);
            char* str = str_dup(pLoxScript->chars);
            str = linedel(str, atoi(arg2));
            str = lineadd(str, tmparg3, atoi(arg2));
            set_lox_pEdit(ch->desc, lox_string(str));
            printf_to_char(ch, "Line replaced.\n\r");
            return;
        }
        else if (!str_cmp(arg1, ".v")) {
            lox_script_compile(ch, false);
            return;
        }
        else if (!str_cmp(arg1, ".x")) {
            pop_editor(ch->desc);
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

        printf_to_char(ch, "LoxEdit:  Invalid dot command.\n\r");
        return;
    }
    else if (*argument == '@') {
        if (ch->desc->showstr_head) {
            printf_to_char(ch, COLOR_INFO "[" COLOR_DECOR_1 "!!!" COLOR_INFO "] You received the following messages while you "
                "were writing:" COLOR_EOL);
            show_string(ch->desc, "");
        }

        lox_script_compile(ch, true);

        return;
    }

    // Get fresh pointer in case it changed
    pLoxScript = get_lox_pEdit(ch->desc);
    strcpy(buf, pLoxScript->chars);

    // Truncate strings to MAX_STRING_LENGTH.
    if (strlen(buf) + strlen(argument) >= (MAX_STRING_LENGTH - 4)) {
        printf_to_char(ch, "String too long, last line skipped.\n\r");

        /* Force character out of editing mode. */
        pop_editor(ch->desc);
        return;
    }

    strcat(buf, argument);
    strcat(buf, "\n");
    set_lox_pEdit(ch->desc, lox_string(buf));

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

    int i;
    for (i = 0; i == 0 || (token->start[i] != '\0' && token->start[i] != '"'); i++) {
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
        case '\\':
            // Escape backslashes
            *ws++ = c;
            *ws++ = token->start[++i];
            break;
        case COLOR_ESC_CHAR:
            // Escape color codes
            *ws++ = c;
            *ws++ = c;
            break;
        default:
            *ws++ = c;
            break;
        }
    }

    if (token->start[i] == '"') {
        *ws++ = '"';
    }

    if (ws_buf[0] != '\0') {
        *ws = '\0';
        addf_buf(buf, "%s%s", COLOR_LOX_STRING, ws_buf);
        scanner.current += (i - token->length) + 1;
        scanner.line = line;
        scanner.interp = STR_INT_NONE;
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

void olc_display_entity_class_info(Mobile* ch, Entity* entity)
{
    if (entity->klass != NULL) {
        olc_print_str_box(ch, "Lox Class", entity->klass->name->chars,
            "Type '" COLOR_TITLE "LOX" COLOR_ALT_TEXT_2 "' to edit.");
    }
    else {
        olc_print_str_box(ch, "Lox Class", "(none)", "Type '" COLOR_TITLE "LOX"
            COLOR_ALT_TEXT_2 "' to create one.");
    }
}
