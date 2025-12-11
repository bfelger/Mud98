////////////////////////////////////////////////////////////////////////////////
// persist/tutorial/json/tutorial_persist_json.c
////////////////////////////////////////////////////////////////////////////////

#include "tutorial_persist_json.h"

#include <persist/json/persist_json.h>

#include <data/tutorial.h>

#include <db.h>

#include <jansson.h>

#include <stdlib.h>
#include <string.h>

const TutorialPersistFormat TUTORIAL_PERSIST_JSON = {
    .name = "json",
    .load = tutorial_persist_json_load,
    .save = tutorial_persist_json_save,
};

static void write_tutorial_steps(json_t* steps_arr, const Tutorial* tut)
{
    for (int i = 0; i < tut->step_count; ++i) {
        TutorialStep* step = &tut->steps[i];
        if (!step->prompt && !step->match)
            continue;
        json_t* obj = json_object();
        if (step->prompt)
            JSON_SET_STRING(obj, "prompt", step->prompt);
        if (step->match && step->match[0] != '\0')
            JSON_SET_STRING(obj, "match", step->match);
        json_array_append_new(steps_arr, obj);
    }
}

PersistResult tutorial_persist_json_save(const PersistWriter* writer, const char* filename)
{
    if (!writer)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "tutorial JSON save: missing writer", -1 };

    extern Tutorial** tutorials;
    extern int tutorial_count;

    json_t* root = json_object();
    JSON_SET_INT(root, "formatVersion", 1);
    json_t* arr = json_array();

    for (int i = 0; i < tutorial_count; ++i) {
        Tutorial* tut = tutorials[i];
        if (!tut || !tut->name || tut->name[0] == '\0')
            continue;
        json_t* obj = json_object();
        JSON_SET_STRING(obj, "name", tut->name);
        if (tut->blurb && tut->blurb[0] != '\0')
            JSON_SET_STRING(obj, "blurb", tut->blurb);
        if (tut->finish && tut->finish[0] != '\0')
            JSON_SET_STRING(obj, "finish", tut->finish);
        JSON_SET_INT(obj, "minLevel", tut->min_level);

        json_t* steps = json_array();
        write_tutorial_steps(steps, tut);
        json_object_set_new(obj, "steps", steps);

        json_array_append_new(arr, obj);
    }

    json_object_set_new(root, "tutorials", arr);

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "tutorial JSON save: dump failed", -1 };

    size_t len = strlen(dump);
    bool ok = writer_write_all(writer, dump, len);
    free(dump);
    if (!ok)
        return (PersistResult){ PERSIST_ERR_IO, "tutorial JSON save: write failed", -1 };
    if (writer->ops->flush)
        writer->ops->flush(writer->ctx);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult tutorial_persist_json_load(const PersistReader* reader, const char* filename)
{
    if (!reader)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "tutorial JSON load: missing reader", -1 };

    ReaderBuffer buf = { 0 };
    if (!reader_fill_buffer(reader, &buf))
        return (PersistResult){ PERSIST_ERR_IO, "tutorial JSON load: failed to read", -1 };

    json_error_t error;
    json_t* root = json_loadb(buf.data, buf.len, 0, &error);
    free(buf.data);
    if (!root) {
        static char msg[256];
        snprintf(msg, sizeof msg, "tutorial JSON parse error at line %d: %s", error.line, error.text);
        return (PersistResult){ PERSIST_ERR_FORMAT, msg, (int)error.line };
    }

    json_t* fmtv = json_object_get(root, "formatVersion");
    if (json_is_integer(fmtv) && json_integer_value(fmtv) != 1) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "tutorial JSON: unsupported formatVersion", -1 };
    }

    json_t* tutorials_arr = json_object_get(root, "tutorials");
    if (!json_is_array(tutorials_arr)) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_FORMAT, "tutorial JSON: missing tutorials array", -1 };
    }

    size_t count = json_array_size(tutorials_arr);
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
    }

    tutorials = malloc(sizeof(Tutorial*) * count);
    if (!tutorials) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_INTERNAL, "tutorial JSON: alloc failed", -1 };
    }

    for (size_t i = 0; i < count; ++i) {
        json_t* obj = json_array_get(tutorials_arr, i);
        if (!json_is_object(obj))
            continue;

        Tutorial* tut = malloc(sizeof(Tutorial));
        if (!tut) {
            json_decref(root);
            return (PersistResult){ PERSIST_ERR_INTERNAL, "tutorial JSON: alloc failed", -1 };
        }
        memset(tut, 0, sizeof(Tutorial));

        const char* name = JSON_STRING(obj, "name");
        if (name)
            tut->name = boot_intern_string(name);
        const char* blurb = JSON_STRING(obj, "blurb");
        if (blurb)
            tut->blurb = boot_intern_string(blurb);
        const char* finish = JSON_STRING(obj, "finish");
        if (finish)
            tut->finish = boot_intern_string(finish);
        tut->min_level = (int)json_int_or_default(obj, "minLevel", tut->min_level);

        json_t* steps = json_object_get(obj, "steps");
        if (json_is_array(steps)) {
            tut->step_count = (int)json_array_size(steps);
            tut->steps = malloc(sizeof(TutorialStep) * tut->step_count);
            if (!tut->steps) {
                free(tut);
                json_decref(root);
                return (PersistResult){ PERSIST_ERR_INTERNAL, "tutorial JSON: step alloc failed", -1 };
            }
            for (int s = 0; s < tut->step_count; ++s) {
                json_t* step_obj = json_array_get(steps, (size_t)s);
                const char* prompt = JSON_STRING(step_obj, "prompt");
                if (prompt)
                    tut->steps[s].prompt = boot_intern_string(prompt);
                const char* match = JSON_STRING(step_obj, "match");
                if (match)
                    tut->steps[s].match = boot_intern_string(match);
            }
        }
        else {
            tut->step_count = 0;
            tut->steps = NULL;
        }

        tutorials[i] = tut;
    }

    tutorial_count = (int)count;
    json_decref(root);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
