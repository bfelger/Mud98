////////////////////////////////////////////////////////////////////////////////
// tutorial.c
////////////////////////////////////////////////////////////////////////////////

#include "tutorial.h"

#include <entities/mobile.h>
#include <entities/player_data.h>

#include <comm.h>
#include <config.h>
#include <db.h>
#include <tablesave.h>

#include <stdbool.h>

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
            COLOR_ALT_TEXT_1 "%s" COLOR_CLEAR "\n\r", t->name, t->blurb);
        match = true;
    }

    if (!match)
        printf_to_char(ch, "     No available tutorials found." 
            COLOR_CLEAR "\n\r");
    printf_to_char(ch, "\n\r");
}

void show_tutorial_step(Mobile* ch, Tutorial* t, int step)
{
    if (step >= t->step_count) {
        bug("Tutorial '%s' with %d steps does not have step %d.",
            t->name, t->step_count, step + 1);
        return;
    }

    printf_to_char(ch, "\n\r^jTutorial: %s (Step %d/%d)\n\r%s^x\n\r", t->name,
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
        printf_to_char(ch, "Tutorial '%s' not found\n\r");
        return;
    }

    if (t->min_level > ch->level) {
        printf_to_char(ch, "Your level is too low to begin that tutorial.\n\r");
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

Tutorial temp_t;
TutorialStep temp_s;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const SaveTableEntry tutorial_save_table[] = {
    { "name",           FIELD_STRING,           U(&temp_t.name),	        0,		            0   },
    { "blurb",          FIELD_STRING,           U(&temp_t.blurb),           0,		            0   },
    { "finish",         FIELD_STRING,           U(&temp_t.finish),          0,                  0   },
    { "min_level",      FIELD_INT,              U(&temp_t.min_level),       0,                  0   },
    { "step_count",     FIELD_INT,              U(&temp_t.step_count),      0,                  0   },
    { NULL,             0,                      0,                          0,                  0   }
};

const SaveTableEntry step_save_table[] = {
    { "prompt",         FIELD_STRING,           U(&temp_s.prompt),          0,                  0   },
    { "match",          FIELD_STRING,           U(&temp_s.match),           0,		            0   },
    { NULL,             0,                      0,                          0,                  0   }
};

static void load_tutorial(FILE* fp, int index)
{
    char* word;

    Tutorial* t = (Tutorial*)malloc(sizeof(Tutorial));
    if (t == NULL) {
        perror("load_tutorial(): Could not allocate tutorial!");
        exit(-1);
    }
    tutorials[index] = t;

    word = fread_word(fp);
    if (str_cmp(word, "#TUTORIAL")) {
        bugf("load_tutorial(): Expected '#TUTORIAL', got '%s'.", word);
        close_file(fp);
        return;
    }

    load_struct(fp, U(&temp_t), tutorial_save_table, U(t));

    TutorialStep* steps = (TutorialStep*)malloc(sizeof(TutorialStep) 
        * t->step_count);
    if (steps == NULL) {
        fprintf(stderr, "load_tutorial(): Could not allocate %d tutorial steps!",
            t->step_count);
        exit(-1);
    }

    t->steps = steps;

    for (int i = 0; i < t->step_count; i++) {
        word = fread_word(fp);
        if (str_cmp(word, "#STEP")) {
            bugf("load_tutorial(): Expected '#STEP', got '%s'.", word);
            close_file(fp);
            return;
        }
        load_struct(fp, U(&temp_s), step_save_table, U(&steps[i]));
    }
}

void load_tutorials()
{
    FILE* fp;

    OPEN_OR_DIE(fp = open_read_tutorials_file());

    tutorial_count = fread_number(fp);
    tutorials = (Tutorial**)malloc(sizeof(Tutorial*) * tutorial_count);
    if (tutorials == NULL) {
        fprintf(stderr, "load_tutorials(): Could not allocate %d tutorials!", 
            tutorial_count);
        exit(-1);
    }

    for (int i = 0; i < tutorial_count; i++)
        load_tutorial(fp, i);

    printf_log("Tutorials loaded.");
    close_file(fp);
}

void save_tutorials()
{
    FILE* fp;

    char temptutorials_file[256];
    sprintf(temptutorials_file, "%s%s", cfg_get_data_dir(), "temptutorials");

    OPEN_OR_RETURN(fp = open_write_file(temptutorials_file));

    fprintf(fp, "%d\n\n", tutorial_count);

    int i;
    for (i = 0; i < tutorial_count; ++i) {
        Tutorial* t = tutorials[i];

        fprintf(fp, "#TUTORIAL\n");
        save_struct(fp, U(&temp_t), tutorial_save_table, U(t));
        fprintf(fp, "#ENDTUTORIAL\n");

        for (int j = 0; j < t->step_count; j++) {
            TutorialStep* s = &t->steps[j];

            fprintf(fp, "#STEP\n");
            save_struct(fp, U(&temp_s), step_save_table, U(s));
            fprintf(fp, "#ENDSTEP\n");
        }

        fprintf(fp, "\n");
    }

    close_file(fp);

    char tutorials_file[256];
    sprintf(tutorials_file, "%s%s", cfg_get_data_dir(), cfg_get_tutorials_file());

#ifdef _MSC_VER
    if (!MoveFileExA(temptutorials_file, tutorials_file, MOVEFILE_REPLACE_EXISTING)) {
        bugf("save_tutorials(): Could not rename %s to %s!", temptutorials_file, tutorials_file);
        perror(tutorials_file);
    }
#else
    if (rename(temptutorials_file, tutorials_file) != 0) {
        bugf("save_tutorials(): Could not rename %s to %s!", temptutorials_file, tutorials_file);
        perror(tutorials_file);
    }
#endif
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif
