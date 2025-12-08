////////////////////////////////////////////////////////////////////////////////
// persist/social/json/social_persist_json.c
////////////////////////////////////////////////////////////////////////////////

#include "social_persist_json.h"

#include <persist/json/persist_json.h>

#include <data/social.h>

#include <db.h>

#include <jansson.h>

#include <stdlib.h>
#include <string.h>

const SocialPersistFormat SOCIAL_PERSIST_JSON = {
    .name = "json",
    .load = social_persist_json_load,
    .save = social_persist_json_save,
};

PersistResult social_persist_json_save(const PersistWriter* writer, const char* filename)
{
    if (!writer)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "social JSON save: missing writer", -1 };

    extern Social* social_table;

    json_t* root = json_object();
    JSON_SET_INT(root, "formatVersion", 1);
    json_t* socials = json_array();

    for (int i = 0; social_table && social_table[i].name && social_table[i].name[0] != '\0'; ++i) {
        Social* soc = &social_table[i];
        if (!soc->name || soc->name[0] == '\0')
            continue;

        json_t* obj = json_object();
        JSON_SET_STRING(obj, "name", soc->name);
        if (soc->char_no_arg && soc->char_no_arg[0] != '\0')
            JSON_SET_STRING(obj, "charNoArg", soc->char_no_arg);
        if (soc->others_no_arg && soc->others_no_arg[0] != '\0')
            JSON_SET_STRING(obj, "othersNoArg", soc->others_no_arg);
        if (soc->char_found && soc->char_found[0] != '\0')
            JSON_SET_STRING(obj, "charFound", soc->char_found);
        if (soc->others_found && soc->others_found[0] != '\0')
            JSON_SET_STRING(obj, "othersFound", soc->others_found);
        if (soc->vict_found && soc->vict_found[0] != '\0')
            JSON_SET_STRING(obj, "victFound", soc->vict_found);
        if (soc->char_auto && soc->char_auto[0] != '\0')
            JSON_SET_STRING(obj, "charAuto", soc->char_auto);
        if (soc->others_auto && soc->others_auto[0] != '\0')
            JSON_SET_STRING(obj, "othersAuto", soc->others_auto);

        json_array_append_new(socials, obj);
    }

    json_object_set_new(root, "socials", socials);

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "social JSON save: dump failed", -1 };

    size_t len = strlen(dump);
    bool ok = writer_write_all(writer, dump, len);
    free(dump);
    if (!ok)
        return (PersistResult){ PERSIST_ERR_IO, "social JSON save: write failed", -1 };
    if (writer->ops->flush)
        writer->ops->flush(writer->ctx);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult social_persist_json_load(const PersistReader* reader, const char* filename)
{
    if (!reader)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "social JSON load: missing reader", -1 };

    ReaderBuffer buf = { 0 };
    if (!reader_fill_buffer(reader, &buf))
        return (PersistResult){ PERSIST_ERR_IO, "social JSON load: failed to read", -1 };

    json_error_t error;
    json_t* root = json_loadb(buf.data, buf.len, 0, &error);
    free(buf.data);
    if (!root) {
        static char msg[256];
        snprintf(msg, sizeof msg, "social JSON parse error at line %d: %s", error.line, error.text);
        return (PersistResult){ PERSIST_ERR_FORMAT, msg, (int)error.line };
    }

    json_t* fmtv = json_object_get(root, "formatVersion");
    if (json_is_integer(fmtv) && json_integer_value(fmtv) != 1) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "social JSON: unsupported formatVersion", -1 };
    }

    json_t* socials = json_object_get(root, "socials");
    if (!json_is_array(socials)) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_FORMAT, "social JSON: missing socials array", -1 };
    }

    size_t count = json_array_size(socials);
    extern Social* social_table;
    extern int social_count;

    if (social_table)
        free(social_table);
    social_table = calloc(count + 1, sizeof(Social));
    if (!social_table) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_INTERNAL, "social JSON: alloc failed", -1 };
    }

    for (size_t i = 0; i < count; ++i) {
        json_t* obj = json_array_get(socials, i);
        if (!json_is_object(obj))
            continue;
        Social* soc = &social_table[i];

        const char* name = JSON_STRING(obj, "name");
        if (name)
            soc->name = boot_intern_string(name);
        const char* char_no = JSON_STRING(obj, "charNoArg");
        if (char_no)
            soc->char_no_arg = boot_intern_string(char_no);
        const char* others_no = JSON_STRING(obj, "othersNoArg");
        if (others_no)
            soc->others_no_arg = boot_intern_string(others_no);
        const char* char_found = JSON_STRING(obj, "charFound");
        if (char_found)
            soc->char_found = boot_intern_string(char_found);
        const char* others_found = JSON_STRING(obj, "othersFound");
        if (others_found)
            soc->others_found = boot_intern_string(others_found);
        const char* vict = JSON_STRING(obj, "victFound");
        if (vict)
            soc->vict_found = boot_intern_string(vict);
        const char* char_auto = JSON_STRING(obj, "charAuto");
        if (char_auto)
            soc->char_auto = boot_intern_string(char_auto);
        const char* others_auto = JSON_STRING(obj, "othersAuto");
        if (others_auto)
            soc->others_auto = boot_intern_string(others_auto);
    }

    social_table[count].name = str_dup("");
    social_count = (int)count;
    json_decref(root);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
