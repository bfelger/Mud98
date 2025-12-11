////////////////////////////////////////////////////////////////////////////////
// persist/tutorial/rom-olc/tutorial_persist_rom_olc.c
////////////////////////////////////////////////////////////////////////////////

#include "tutorial_persist_rom_olc.h"

#include <persist/persist_io_adapters.h>

#include <tablesave.h>

#include <comm.h>
#include <db.h>

#include <data/tutorial.h>

#include <stdlib.h>

#ifdef U
#undef U
#endif
#define U(x) (uintptr_t)(x)

static Tutorial temp_t;
static TutorialStep temp_s;

static const SaveTableEntry tutorial_save_table[] = {
    { "name",       FIELD_STRING,   U(&temp_t.name),      0, 0 },
    { "blurb",      FIELD_STRING,   U(&temp_t.blurb),     0, 0 },
    { "finish",     FIELD_STRING,   U(&temp_t.finish),    0, 0 },
    { "min_level",  FIELD_INT,      U(&temp_t.min_level), 0, 0 },
    { "step_count", FIELD_INT,      U(&temp_t.step_count),0, 0 },
    { NULL,         0,              0,                    0, 0 }
};

static const SaveTableEntry step_save_table[] = {
    { "prompt",     FIELD_STRING,   U(&temp_s.prompt),    0, 0 },
    { "match",      FIELD_STRING,   U(&temp_s.match),     0, 0 },
    { NULL,         0,              0,                    0, 0 }
};

const TutorialPersistFormat TUTORIAL_PERSIST_ROM_OLC = {
    .name = "rom-olc",
    .load = tutorial_persist_rom_olc_load,
    .save = tutorial_persist_rom_olc_save,
};

static bool load_tutorial_entry(FILE* fp, Tutorial** out)
{
    char* word = fread_word(fp);
    if (str_cmp(word, "#TUTORIAL")) {
        bugf("load_tutorial(): Expected '#TUTORIAL', got '%s'.", word);
        return false;
    }

    Tutorial* tut = malloc(sizeof(Tutorial));
    if (!tut) {
        perror("load_tutorial(): Could not allocate tutorial!");
        return false;
    }
    load_struct(fp, U(&temp_t), tutorial_save_table, U(tut));

    TutorialStep* steps = malloc(sizeof(TutorialStep) * tut->step_count);
    if (!steps) {
        fprintf(stderr, "load_tutorial(): Could not allocate %d tutorial steps!", tut->step_count);
        free(tut);
        return false;
    }
    tut->steps = steps;

    for (int i = 0; i < tut->step_count; ++i) {
        word = fread_word(fp);
        if (str_cmp(word, "#STEP")) {
            bugf("load_tutorial(): Expected '#STEP', got '%s'.", word);
            free(steps);
            free(tut);
            return false;
        }
        load_struct(fp, U(&temp_s), step_save_table, U(&steps[i]));
    }

    *out = tut;
    return true;
}

PersistResult tutorial_persist_rom_olc_load(const PersistReader* reader, const char* filename)
{
    if (!reader || reader->ops != &PERSIST_FILE_STREAM_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "tutorial ROM load requires FILE reader", -1 };
    FILE* fp = (FILE*)reader->ctx;
    extern Tutorial** tutorials;
    extern int tutorial_count;
    if (tutorials) {
        for (int i = 0; i < tutorial_count; ++i) {
            if (!tutorials[i])
                continue;
            free(tutorials[i]->steps);
            free(tutorials[i]);
        }
        free(tutorials);
        tutorials = NULL;
    }

    if (fscanf(fp, "%d\n", &tutorial_count) < 1)
        return (PersistResult){ PERSIST_ERR_FORMAT, "tutorial ROM load: missing count", -1 };

    tutorials = malloc(sizeof(Tutorial*) * tutorial_count);
    if (!tutorials)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "tutorial ROM load: alloc failed", -1 };

    for (int i = 0; i < tutorial_count; ++i) {
        if (!load_tutorial_entry(fp, &tutorials[i]))
            return (PersistResult){ PERSIST_ERR_FORMAT, "tutorial ROM load: parse error", -1 };
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult tutorial_persist_rom_olc_save(const PersistWriter* writer, const char* filename)
{
    if (!writer || writer->ops != &PERSIST_FILE_WRITER_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "tutorial ROM save requires FILE writer", -1 };
    FILE* fp = (FILE*)writer->ctx;
    extern Tutorial** tutorials;
    extern int tutorial_count;

    fprintf(fp, "%d\n\n", tutorial_count);

    for (int i = 0; i < tutorial_count; ++i) {
        Tutorial* tut = tutorials[i];

        // Guard against NULL steps
        int alloc_step_count = tut->step_count;
        for (int j = 0; j < tut->step_count; ++j) {
            if (!tut->steps[j].prompt || !tut->steps[j].match) {
                --tut->step_count;
            }
        }

        fprintf(fp, "#TUTORIAL\n");
        save_struct(fp, U(&temp_t), tutorial_save_table, U(tut));
        fprintf(fp, "#ENDTUTORIAL\n");

        for (int j = 0; j < alloc_step_count; ++j) {
            if (!tut->steps[j].prompt || !tut->steps[j].match)
                continue;
            fprintf(fp, "#STEP\n");
            save_struct(fp, U(&temp_s), step_save_table, U(&tut->steps[j]));
            fprintf(fp, "#ENDSTEP\n");
        }

        // Restore original step count in case we were in the middle of editing
        tut->step_count = alloc_step_count;

        fprintf(fp, "\n");
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

#ifdef U
#undef U
#endif
