////////////////////////////////////////////////////////////////////////////////
// data/tutorial.c
////////////////////////////////////////////////////////////////////////////////

#include "tutorial.h"

#include <entities/mobile.h>
#include <entities/player_data.h>

#include <comm.h>
#include <config.h>
#include <db.h>
#include <persist/tutorial/tutorial_persist.h>

#include <stdbool.h>

extern bool test_output_enabled;

Tutorial** tutorials;
int tutorial_count;

static void show_tutorial_list(Mobile* ch)
{
    bool match = false;

    printf_to_char(ch, COLOR_INFO "\n\rThe following tutorials are available to "
        "you:\n\r\n\r");
    for (int i = 0; i < tutorial_count; i++) {
        Tutorial* t = tutorials[i];
        if (t->min_level > ch->level)
            continue;
        printf_to_char(ch, COLOR_TITLE "%-20s " COLOR_INFO "- "
            COLOR_ALT_TEXT_1 "%s" COLOR_EOL, t->name, t->blurb);
        match = true;
    }

    if (!match)
        printf_to_char(ch, "     No available tutorials found." 
            COLOR_EOL);
    printf_to_char(ch, "\n\r");
}

void show_tutorial_step(Mobile* ch, Tutorial* t, int step)
{
    if (step >= t->step_count) {
        bug("Tutorial '%s' with %d steps does not have step %d.",
            t->name, t->step_count, step + 1);
        return;
    }

    printf_to_char(ch, COLOR_INFO "\n\rTutorial: %s (Step %d/%d)\n\r%s" COLOR_EOL, t->name,
       step + 1, t->step_count, t->steps[step].prompt);
}

void advance_tutorial_step(Mobile* ch)
{
    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL)
        return;

    Tutorial* t = ch->pcdata->tutorial;
    int s = ++(ch->pcdata->tutorial_step);
    
    if (s < t->step_count) {
        show_tutorial_step(ch, t, s);
    }
    else {
        printf_to_char(ch, "\n\r^jTutorial: %s\n\r%s^x\n\r", t->name, t->finish);
        ch->pcdata->tutorial = NULL;
        ch->pcdata->tutorial_step = 0;
    }
}

void do_tutorial(Mobile* ch, char* argument)
{
    if (IS_NPC(ch) || ch->pcdata == NULL)
        return;

    if (argument[0] == '\0') {
        Tutorial* t = ch->pcdata->tutorial;
        if (t != NULL)
            show_tutorial_step(ch, t, ch->pcdata->tutorial_step);
        else
            show_tutorial_list(ch);
        return;
    }

    Tutorial* t = get_tutorial(argument);

    if (t == NULL) {
        printf_to_char(ch, COLOR_INFO"Tutorial '%s' not found" COLOR_EOL, argument);
        return;
    }

    if (t->min_level > ch->level) {
        printf_to_char(ch, COLOR_INFO "Your level is too low to begin that tutorial." COLOR_EOL);
        return;
    }

    ch->pcdata->tutorial = t;
    ch->pcdata->tutorial_step = 0;

    show_tutorial_step(ch, t, 0);
}

Tutorial* get_tutorial(const char* name)
{
    for (int i = 0; i < tutorial_count; i++) {
        if (!str_prefix(name, tutorials[i]->name))
            return tutorials[i];
    }

    return NULL;
}

// SAVE/LOAD ROUTINES //////////////////////////////////////////////////////////

void load_tutorials()
{
    PersistResult res = tutorial_persist_load(cfg_get_tutorials_file());
    if (!persist_succeeded(res)) {
        bugf("load_tutorials: failed to load tutorials (%s)",
            res.message ? res.message : "unknown error");
        exit(1);
    }
    
    if (!test_output_enabled)   
        printf_log("Tutorials loaded (%d tutorials).", tutorial_count);
}

void save_tutorials()
{
    PersistResult res = tutorial_persist_save(cfg_get_tutorials_file());
    if (!persist_succeeded(res))
        bugf("save_tutorials: failed to save tutorials (%s)",
            res.message ? res.message : "unknown error");
}
