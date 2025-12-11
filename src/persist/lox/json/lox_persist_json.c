////////////////////////////////////////////////////////////////////////////////
// persist/lox/json/lox_persist_json.c
////////////////////////////////////////////////////////////////////////////////

#include "lox_persist_json.h"

#include <persist/json/persist_json.h>

#include <lox/lox.h>

#include <comm.h>

#include <jansson.h>

#include <stdlib.h>
#include <string.h>

const LoxPersistFormat LOX_PERSIST_JSON = {
    .name = "json",
    .load = lox_persist_json_load,
    .save = lox_persist_json_save,
};

PersistResult lox_persist_json_save(const PersistWriter* writer, const char* filename)
{
    if (!writer)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "lox JSON save: missing writer", -1 };

    json_t* root = json_object();
    JSON_SET_INT(root, "formatVersion", 1);
    json_t* scripts = json_array();

    size_t count = lox_script_entry_count();
    for (size_t i = 0; i < count; ++i) {
        const LoxScriptEntry* entry = lox_script_entry_get(i);
        if (!entry)
            continue;

        json_t* obj = json_object();
        if (entry->category && entry->category[0] != '\0')
            JSON_SET_STRING(obj, "category", entry->category);
        if (entry->file && entry->file[0] != '\0')
            JSON_SET_STRING(obj, "file", entry->file);
        JSON_SET_STRING(obj, "when", lox_script_when_name(entry->when));
        json_array_append_new(scripts, obj);
    }

    json_object_set_new(root, "scripts", scripts);

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "lox JSON save: dump failed", -1 };

    size_t len = strlen(dump);
    bool wrote = writer_write_all(writer, dump, len);
    free(dump);
    if (!wrote)
        return (PersistResult){ PERSIST_ERR_IO, "lox JSON save: write failed", -1 };
    if (writer->ops && writer->ops->flush)
        writer->ops->flush(writer->ctx);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult lox_persist_json_load(const PersistReader* reader, const char* filename)
{
    if (!reader)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "lox JSON load: missing reader", -1 };

    ReaderBuffer buf = { 0 };
    if (!reader_fill_buffer(reader, &buf))
        return (PersistResult){ PERSIST_ERR_IO, "lox JSON load: failed to read file", -1 };

    json_error_t error;
    json_t* root = json_loadb(buf.data, buf.len, 0, &error);
    free(buf.data);
    if (!root) {
        static char msg[256];
        snprintf(msg, sizeof(msg), "lox JSON parse error at line %d: %s", error.line, error.text);
        return (PersistResult){ PERSIST_ERR_FORMAT, msg, (int)error.line };
    }

    json_t* fmtv = json_object_get(root, "formatVersion");
    if (json_is_integer(fmtv) && json_integer_value(fmtv) != 1) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "lox JSON: unsupported formatVersion", -1 };
    }

    json_t* scripts = json_object_get(root, "scripts");
    if (!json_is_array(scripts)) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_FORMAT, "lox JSON: missing scripts array", -1 };
    }

    size_t count = json_array_size(scripts);
    for (size_t i = 0; i < count; ++i) {
        json_t* script = json_array_get(scripts, i);
        if (!json_is_object(script))
            continue;

        const char* file = JSON_STRING(script, "file");
        if (!file || file[0] == '\0') {
            bug("lox JSON load: script missing file name", 0);
            continue;
        }

        const char* category = JSON_STRING(script, "category");
        const char* when_name = JSON_STRING(script, "when");
        LoxScriptWhen when = LOX_SCRIPT_WHEN_PRE;
        if (when_name && !lox_script_when_parse(when_name, &when)) {
            bugf("lox JSON load: invalid when '%s'", when_name);
            continue;
        }

        if (!lox_script_entry_append_loaded(category ? category : "", file, when)) {
            json_decref(root);
            return (PersistResult){ PERSIST_ERR_INTERNAL, "lox JSON load: allocation failed", -1 };
        }
    }

    json_decref(root);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
