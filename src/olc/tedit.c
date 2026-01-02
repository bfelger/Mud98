////////////////////////////////////////////////////////////////////////////////
// tedit.c
//
// This file contains the implementation of the tutorial editor (tedit) for 
// in-game tutorials accessed by the TUTORIAL command.
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "comm.h"
#include "config.h"
#include "db.h"
#include "olc.h"
#include "string_edit.h"

#include "entities/descriptor.h"
#include "entities/player_data.h"

#include "data/tutorial.h"

#ifdef _MSC_VER
#include <io.h>
#define access _access
#else
#include <unistd.h>
#endif

#ifndef F_OK
#define F_OK 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TEDIT(fun)      bool fun(Mobile* ch, char* argument)

Tutorial xTutorial;

extern Tutorial** tutorials;
extern int tutorial_count;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

static Tutorial* tutorial_exact_lookup(const char* name);
static int tutorial_index_from_ptr(const Tutorial* tut);
static void show_tutorial_steps(Mobile* ch, const Tutorial* tut);
static bool ensure_step_capacity(Mobile* ch, Tutorial* tut, int insert_at);
static void free_tutorial(Tutorial* tut);
static void detach_players_from_tutorial(const Tutorial* tut);
static void detach_editors_from_tutorial(const Tutorial* tut);
static TutorialStep* get_step(Tutorial* tut, const char* step_str, int* out_index, Mobile* ch);

TEDIT(tedit_show);
TEDIT(tedit_list);
TEDIT(tedit_new);
TEDIT(tedit_delete);
TEDIT(tedit_edit_blurb);
TEDIT(tedit_edit_finish);
TEDIT(tedit_step);
TEDIT(tedit_step_add);
TEDIT(tedit_step_delete);
TEDIT(tedit_step_prompt);
TEDIT(tedit_step_match);
TEDIT(tedit_commands);
TEDIT(tedit_help);

const OlcCmdEntry tutorial_olc_comm_table[] = {
    { "show",       0,                          ed_olded,           U(tedit_show)       },
    { "name",       U(&xTutorial.name),         ed_line_string,     0                   },
    { "blurb",      0,                          ed_olded,           U(tedit_edit_blurb) },
    { "finish",     0,                          ed_olded,           U(tedit_edit_finish)},
    { "minlevel",   U(&xTutorial.min_level),    ed_number_level,    0                   },
    { "step",       0,                          ed_olded,           U(tedit_step)       },
    { "stepadd",    0,                          ed_olded,           U(tedit_step_add)   },
    { "stepdel",    0,                          ed_olded,           U(tedit_step_delete)},
    { "stepprompt", 0,                          ed_olded,           U(tedit_step_prompt)},
    { "stepmatch",  0,                          ed_olded,           U(tedit_step_match) },
    { "delete",     0,                          ed_olded,           U(tedit_delete)     },
    { "list",       0,                          ed_olded,           U(tedit_list)       },
    { "new",        0,                          ed_olded,           U(tedit_new)        },
    { "commands",   0,                          ed_olded,           U(tedit_commands)   },
    { "help",       0,                          ed_olded,           U(tedit_help)       },
    { "?",          0,                          ed_olded,           U(tedit_help)       },
    { "version",    0,                          ed_olded,           U(show_version)     },
    { NULL,         0,                          NULL,               0                   }
};

void do_tedit(Mobile* ch, char* argument)
{
    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    if (ch->pcdata->security < MIN_TEDIT_SECURITY) {
        send_to_char(COLOR_INFO "You do not have enough security to edit tutorials." COLOR_EOL, ch);
        return;
    }

    char command[MIL];
    READ_ARG(command);

    if (!str_cmp(command, "new")) {
        if (tedit_new(ch, argument))
            save_tutorials();
        return;
    }

    if (!str_cmp(command, "list")) {
        tedit_list(ch, argument);
        return;
    }

    if (IS_NULLSTR(command)) {
        send_to_char(COLOR_INFO "Syntax: tedit <tutorial name>" COLOR_EOL, ch);
        return;
    }

    Tutorial* tut = get_tutorial(command);
    if (!tut) {
        printf_to_char(ch, COLOR_INFO "Tutorial '%s' not found." COLOR_EOL, command);
        return;
    }

    set_editor(ch->desc, ED_TUTORIAL, U(tut));

    tedit_show(ch, "");
}


void tedit(Mobile* ch, char* argument)
{
    if (ch->pcdata->security < MIN_TEDIT_SECURITY) {
        send_to_char(COLOR_INFO "You do not have enough security to edit tutorials." COLOR_EOL, ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (!str_prefix("save", argument)) {
        char arg1[MIL];
        char arg2[MIL];
        argument = one_argument(argument, arg1);
        argument = one_argument(argument, arg2);
        const char* requested_ext = NULL;
        bool force_format = false;
        if (!str_cmp(arg2, "json")) {
            requested_ext = ".json";
            force_format = true;
        }
        else if (!str_cmp(arg2, "olc")) {
            requested_ext = ".olc";
            force_format = true;
        }

        const char* tutorials_file = cfg_get_tutorials_file();
        const char* ext = strrchr(tutorials_file, '.');
        bool has_ext = (ext != NULL);

        if (!force_format) {
            if (has_ext) {
                requested_ext = NULL;
            }
            else if (access(tutorials_file, F_OK) != 0) {
                const char* def = cfg_get_default_format();
                requested_ext = (def && !str_cmp(def, "json")) ? ".json" : ".olc";
            }
        }

        if (requested_ext != NULL) {
            size_t base_len = has_ext ? (size_t)(ext - tutorials_file) : strlen(tutorials_file);
            char newname[MIL];
            snprintf(newname, sizeof(newname), "%.*s%s", (int)base_len, tutorials_file, requested_ext);
            cfg_set_tutorials_file(newname);
        }

        save_tutorials();
        send_to_char(COLOR_INFO "Tutorials saved." COLOR_EOL, ch);
        return;
    }

    if (argument[0] == '\0') {
        tedit_show(ch, argument);
        return;
    }

    if (!process_olc_command(ch, argument, tutorial_olc_comm_table))
        interpret(ch, argument);
}


static Tutorial* tutorial_exact_lookup(const char* name)
{
    if (IS_NULLSTR(name))
        return NULL;

    for (int i = 0; i < tutorial_count; ++i) {
        if (!str_cmp(name, tutorials[i]->name))
            return tutorials[i];
    }
    return NULL;
}

static int tutorial_index_from_ptr(const Tutorial* tut)
{
    if (!tut)
        return -1;
    for (int i = 0; i < tutorial_count; ++i) {
        if (tutorials[i] == tut)
            return i;
    }
    return -1;
}

static void show_tutorial_steps(Mobile* ch, const Tutorial* tut)
{
    if (!tut || tut->step_count <= 0) {
        send_to_char(COLOR_INFO "No steps defined." COLOR_EOL, ch);
        return;
    }

    printf_to_char(ch, COLOR_TITLE "Steps (%d):" COLOR_EOL, tut->step_count);
    for (int i = 0; i < tut->step_count; ++i) {
        printf_to_char(ch, COLOR_DECOR_1 "  [" COLOR_ALT_TEXT_1 "%2d" COLOR_DECOR_1 "] " COLOR_TEXT, i + 1);
        printf_to_char(ch, COLOR_TEXT "Prompt: " COLOR_ALT_TEXT_2 "%s" COLOR_EOL, olc_inline_text(tut->steps[i].prompt, 64));
        printf_to_char(ch, COLOR_TEXT "       Match: " COLOR_ALT_TEXT_1 "%s" COLOR_EOL, tut->steps[i].match);
    }
}

static bool ensure_step_capacity(Mobile* ch, Tutorial* tut, int insert_at)
{
    if (!tut)
        return false;

    int old_count = tut->step_count;
    TutorialStep* new_steps = realloc(tut->steps, sizeof(TutorialStep) * (old_count + 1));
    if (!new_steps) {
        if (ch)
            send_to_char(COLOR_INFO "Unable to allocate memory for steps." COLOR_EOL, ch);
        return false;
    }
    tut->steps = new_steps;

    if (insert_at < old_count) {
        memmove(&tut->steps[insert_at + 1], &tut->steps[insert_at],
            sizeof(TutorialStep) * (old_count - insert_at));
    }

    tut->steps[insert_at].prompt = str_dup("");
    tut->steps[insert_at].match = str_dup("");
    tut->step_count = old_count + 1;
    return true;
}

static void free_tutorial(Tutorial* tut)
{
    if (!tut)
        return;

    free_string(tut->name);
    free_string(tut->blurb);
    free_string(tut->finish);

    for (int i = 0; i < tut->step_count; ++i) {
        free_string(tut->steps[i].prompt);
        free_string(tut->steps[i].match);
    }
    free(tut->steps);
    free(tut);
}

static void detach_players_from_tutorial(const Tutorial* tut)
{
    if (!tut)
        return;

    Descriptor* d;
    FOR_EACH(d, descriptor_list) {
        Mobile* vch = d->character;
        if (!vch || IS_NPC(vch) || !vch->pcdata)
            continue;
        if (vch->pcdata->tutorial == tut) {
            vch->pcdata->tutorial = NULL;
            vch->pcdata->tutorial_step = 0;
        }
    }
}

static void detach_editors_from_tutorial(const Tutorial* tut)
{
    if (!tut)
        return;

    Descriptor* d;
    FOR_EACH(d, descriptor_list) {
        if (get_editor(d) == ED_TUTORIAL && get_pEdit(d) == (uintptr_t)tut && d->character)
            edit_done(d->character);
    }
}

static TutorialStep* get_step(Tutorial* tut, const char* step_str, int* out_index, Mobile* ch)
{
    if (!tut) {
        send_to_char(COLOR_INFO "No tutorial selected." COLOR_EOL, ch);
        return NULL;
    }

    if (IS_NULLSTR(step_str) || !is_number(step_str)) {
        send_to_char(COLOR_INFO "Syntax: <command> <step #>" COLOR_EOL, ch);
        return NULL;
    }

    int index = atoi(step_str);
    if (index < 1 || index > tut->step_count) {
        printf_to_char(ch, COLOR_INFO "Step must be between 1 and %d." COLOR_EOL, tut->step_count);
        return NULL;
    }

    if (out_index)
        *out_index = index - 1;
    return &tut->steps[index - 1];
}

TEDIT(tedit_show)
{
    Tutorial* tut;
    EDIT_TUTORIAL(ch, tut);

    if (!tut) {
        send_to_char(COLOR_INFO "No tutorial selected." COLOR_EOL, ch);
        return false;
    }

    olc_print_str(ch, "Name", tut->name ? tut->name : "(unnamed)");
    olc_print_num(ch, "Min Level", tut->min_level);
    olc_print_num(ch, "Step Count", tut->step_count);
    olc_print_text(ch, "Blurb", tut->blurb);
    olc_print_text(ch, "Finish", tut->finish);

    show_tutorial_steps(ch, tut);

    return false;
}

TEDIT(tedit_list)
{
    if (tutorial_count <= 0) {
        send_to_char(COLOR_INFO "No tutorials defined." COLOR_EOL, ch);
        return false;
    }

    INIT_BUF(out, MSL);
    addf_buf(out, COLOR_TITLE "%-3s %-25s %7s %7s" COLOR_EOL, "#", "Name", "Level", "Steps");
    for (int i = 0; i < tutorial_count; ++i) {
        Tutorial* tut = tutorials[i];
        addf_buf(out, COLOR_ALT_TEXT_1 "%-3d %-25s %7d %7d" COLOR_EOL,
            i + 1, tut->name ? tut->name : "(unnamed)", tut->min_level, tut->step_count);
    }
    page_to_char(out->string, ch);
    free_buf(out);
    return false;
}

TEDIT(tedit_new)
{
    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: new <tutorial name>" COLOR_EOL, ch);
        return false;
    }

    char name[MSL];
    strncpy(name, argument, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    char* start = name;
    while (*start && isspace((unsigned char)*start))
        start++;
    char* end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1)))
        --end;
    *end = '\0';
    if (start != name)
        memmove(name, start, end - start + 1);

    if (IS_NULLSTR(name)) {
        send_to_char(COLOR_INFO "Tutorial name cannot be blank." COLOR_EOL, ch);
        return false;
    }

    if (tutorial_exact_lookup(name)) {
        send_to_char(COLOR_INFO "A tutorial with that name already exists." COLOR_EOL, ch);
        return false;
    }

    Tutorial* tut = calloc(1, sizeof(Tutorial));
    if (!tut) {
        send_to_char(COLOR_INFO "Unable to allocate tutorial." COLOR_EOL, ch);
        return false;
    }

    tut->name = str_dup(name);
    tut->blurb = str_dup("");
    tut->finish = str_dup("You have completed the tutorial.");
    tut->min_level = 0;
    tut->steps = NULL;
    tut->step_count = 0;

    Tutorial** new_list = realloc(tutorials, sizeof(Tutorial*) * (tutorial_count + 1));
    if (!new_list) {
        free_tutorial(tut);
        send_to_char(COLOR_INFO "Unable to expand tutorial list." COLOR_EOL, ch);
        return false;
    }
    tutorials = new_list;
    tutorials[tutorial_count] = tut;
    tutorial_count++;

    set_editor(ch->desc, ED_TUTORIAL, U(tut));

    send_to_char(COLOR_INFO "Tutorial created." COLOR_EOL, ch);
    tedit_show(ch, "");
    return true;
}

TEDIT(tedit_delete)
{
    Tutorial* target = NULL;

    if (IS_NULLSTR(argument)) {
        EDIT_TUTORIAL(ch, target);
        if (!target) {
            send_to_char(COLOR_INFO "Syntax: delete <tutorial name>" COLOR_EOL, ch);
            return false;
        }
    }
    else {
        target = get_tutorial(argument);
        if (!target) {
            printf_to_char(ch, COLOR_INFO "Tutorial '%s' not found." COLOR_EOL, argument);
            return false;
        }
    }

    int index = tutorial_index_from_ptr(target);
    if (index == -1) {
        send_to_char(COLOR_INFO "Unable to locate tutorial in memory." COLOR_EOL, ch);
        return false;
    }

    detach_players_from_tutorial(target);
    detach_editors_from_tutorial(target);
    free_tutorial(target);

    for (int i = index; i < tutorial_count - 1; ++i)
        tutorials[i] = tutorials[i + 1];

    tutorial_count--;
    if (tutorial_count == 0) {
        free(tutorials);
        tutorials = NULL;
    }
    else {
        Tutorial** new_list = realloc(tutorials, sizeof(Tutorial*) * tutorial_count);
        if (new_list)
            tutorials = new_list;
    }

    send_to_char(COLOR_INFO "Tutorial deleted." COLOR_EOL, ch);
    return true;
}

TEDIT(tedit_step)
{
    Tutorial* tut;
    EDIT_TUTORIAL(ch, tut);

    if (!tut) {
        send_to_char(COLOR_INFO "No tutorial selected." COLOR_EOL, ch);
        return false;
    }

    char step_str[MIL];
    READ_ARG(step_str);

    if (IS_NULLSTR(step_str) || !is_number(step_str)) {
        show_tutorial_steps(ch, tut);
        return false;
    }

    int step_index = atoi(step_str) - 1;
    TutorialStep* step = get_step(tut, step_str, &step_index, ch);
    if (!step) {
        send_to_char(COLOR_INFO "No such step exists." COLOR_EOL, ch);
        return false;
    }

    printf_to_char(ch, COLOR_TITLE "Step %d:" COLOR_EOL, step_index + 1);
    olc_print_text(ch, "Prompt", step->prompt);
    olc_print_str(ch, "Match", step->match);

    return false;
}

TEDIT(tedit_step_add)
{
    Tutorial* tut;
    EDIT_TUTORIAL(ch, tut);

    if (!tut) {
        send_to_char(COLOR_INFO "No tutorial selected." COLOR_EOL, ch);
        return false;
    }

    int insert_at = tut->step_count;
    if (!IS_NULLSTR(argument)) {
        if (!is_number(argument)) {
            send_to_char(COLOR_INFO "Syntax: stepadd [position]" COLOR_EOL, ch);
            return false;
        }
        insert_at = atoi(argument) - 1;
        if (insert_at < 0)
            insert_at = 0;
        if (insert_at > tut->step_count)
            insert_at = tut->step_count;
    }

    if (!ensure_step_capacity(ch, tut, insert_at))
        return false;

    printf_to_char(ch, COLOR_INFO "Step %d created." COLOR_EOL, insert_at + 1);
    return true;
}

TEDIT(tedit_step_delete)
{
    Tutorial* tut;
    EDIT_TUTORIAL(ch, tut);

    if (!tut || tut->step_count == 0) {
        send_to_char(COLOR_INFO "No steps to delete." COLOR_EOL, ch);
        return false;
    }

    char step_str[MIL];
    READ_ARG(step_str);

    int step_index = -1;
    TutorialStep* step = get_step(tut, step_str, &step_index, ch);
    if (!step)
        return false;

    free_string(step->prompt);
    free_string(step->match);

    if (step_index < tut->step_count - 1) {
        memmove(&tut->steps[step_index], &tut->steps[step_index + 1],
            sizeof(TutorialStep) * (tut->step_count - step_index - 1));
    }

    tut->step_count--;
    if (tut->step_count == 0) {
        free(tut->steps);
        tut->steps = NULL;
    }
    else {
        TutorialStep* resized = realloc(tut->steps, sizeof(TutorialStep) * tut->step_count);
        if (resized)
            tut->steps = resized;
    }

    send_to_char(COLOR_INFO "Step removed." COLOR_EOL, ch);
    return true;
}

TEDIT(tedit_step_prompt)
{
    Tutorial* tut;
    EDIT_TUTORIAL(ch, tut);

    if (!tut) {
        send_to_char(COLOR_INFO "No tutorial selected." COLOR_EOL, ch);
        return false;
    }

    char step_str[MIL];
    argument = one_argument(argument, step_str);

    TutorialStep* step = get_step(tut, step_str, NULL, ch);
    if (!step)
        return false;

    if (!emptystring(argument)) {
        send_to_char(COLOR_INFO "Syntax: stepprompt <step #>" COLOR_EOL, ch);
        return false;
    }

    string_append(ch, &step->prompt);
    return false;
}

TEDIT(tedit_step_match)
{
    Tutorial* tut;
    EDIT_TUTORIAL(ch, tut);

    if (!tut) {
        send_to_char(COLOR_INFO "No tutorial selected." COLOR_EOL, ch);
        return false;
    }

    char step_str[MIL];
    argument = one_argument(argument, step_str);

    TutorialStep* step = get_step(tut, step_str, NULL, ch);
    if (!step)
        return false;

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: stepmatch <step #> <pattern>" COLOR_EOL, ch);
        return false;
    }

    free_string(step->match);
    if (!str_cmp(argument, "null"))
        step->match = str_dup("");
    else
        step->match = str_dup(argument);

    send_to_char(COLOR_INFO "Match updated." COLOR_EOL, ch);
    return true;
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
TEDIT(tedit_edit_blurb)
{
    Tutorial* tut;
    EDIT_TUTORIAL(ch, tut);

    if (!tut) {
        send_to_char(COLOR_INFO "No tutorial selected." COLOR_EOL, ch);
        return false;
    }

    if (!emptystring(argument)) {
        send_to_char(COLOR_INFO "Syntax: blurb" COLOR_EOL, ch);
        return false;
    }

    string_append(ch, &tut->blurb);
    return false;
}

TEDIT(tedit_edit_finish)
{
    Tutorial* tut;
    EDIT_TUTORIAL(ch, tut);

    if (!tut) {
        send_to_char(COLOR_INFO "No tutorial selected." COLOR_EOL, ch);
        return false;
    }

    if (!emptystring(argument)) {
        send_to_char(COLOR_INFO "Syntax: finish" COLOR_EOL, ch);
        return false;
    }

    string_append(ch, &tut->finish);
    return false;
}

TEDIT(tedit_commands)
{
    static const char* cmds =
        COLOR_TITLE "TEdit Commands" COLOR_EOL
        COLOR_ALT_TEXT_1 "  show                " COLOR_DECOR_1 "-" COLOR_INFO " Display the current tutorial" COLOR_EOL
        COLOR_ALT_TEXT_1 "  list                " COLOR_DECOR_1 "-" COLOR_INFO " List all tutorials" COLOR_EOL
        COLOR_ALT_TEXT_1 "  new <name>          " COLOR_DECOR_1 "-" COLOR_INFO " Create and edit a new tutorial" COLOR_EOL
        COLOR_ALT_TEXT_1 "  delete [name]       " COLOR_DECOR_1 "-" COLOR_INFO " Delete current or named tutorial" COLOR_EOL
        COLOR_ALT_TEXT_1 "  name <string>       " COLOR_DECOR_1 "-" COLOR_INFO " Rename the tutorial" COLOR_EOL
        COLOR_ALT_TEXT_1 "  blurb               " COLOR_DECOR_1 "-" COLOR_INFO " Edit the list blurb (multiline)" COLOR_EOL
        COLOR_ALT_TEXT_1 "  finish              " COLOR_DECOR_1 "-" COLOR_INFO " Edit the completion text (multiline)" COLOR_EOL
        COLOR_ALT_TEXT_1 "  minlevel <n>        " COLOR_DECOR_1 "-" COLOR_INFO " Set minimum level" COLOR_EOL
        COLOR_ALT_TEXT_1 "  stepadd [n]         " COLOR_DECOR_1 "-" COLOR_INFO " Insert a new step (defaults to end)" COLOR_EOL
        COLOR_ALT_TEXT_1 "  stepdel <n>         " COLOR_DECOR_1 "-" COLOR_INFO " Delete step number" COLOR_EOL
        COLOR_ALT_TEXT_1 "  stepprompt <n>      " COLOR_DECOR_1 "-" COLOR_INFO " Edit the prompt text for a step" COLOR_EOL
        COLOR_ALT_TEXT_1 "  stepmatch <n> <pat> " COLOR_DECOR_1 "-" COLOR_INFO " Set the match pattern" COLOR_EOL
        COLOR_ALT_TEXT_1 "  save [json|olc]     " COLOR_DECOR_1 "-" COLOR_INFO " Save tutorials (uses config default when omitted)" COLOR_EOL
        COLOR_ALT_TEXT_1 "  done                " COLOR_DECOR_1 "-" COLOR_INFO " Exit the tutorial editor" COLOR_EOL;

    send_to_char(cmds, ch);
    return false;
}

TEDIT(tedit_help)
{
    do_help(ch, "tedit");
    return false;
}
