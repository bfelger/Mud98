////////////////////////////////////////////////////////////////////////////////
// persist/json/class_persist_json.c
////////////////////////////////////////////////////////////////////////////////

#include "class_persist_json.h"

#include <persist/class/class_persist.h>
#include <persist/json/persist_json.h>
#include <persist/persist_io_adapters.h>

#include <merc.h>
#include <comm.h>
#include <lookup.h>

#include <data/class.h>
#include <tables.h>

#include <string.h>
#include <stdlib.h>

#ifdef HAS_JSON_AREAS
#include <jansson.h>
#endif

const ClassPersistFormat CLASS_PERSIST_JSON = { 
    .name = "json", 
    .load = class_persist_json_load, 
    .save = class_persist_json_save 
};

#ifdef HAVE_JSON_AREAS
static bool writer_write_all(const PersistWriter* writer, const char* data, size_t len)
{
    if (writer->ops->write)
        return writer->ops->write(data, len, writer->ctx) == len;
    for (size_t i = 0; i < len; i++) {
        if (writer->ops->putc(data[i], writer->ctx) == EOF)
            return false;
    }
    return true;
}
#endif 

PersistResult class_persist_json_save(const PersistWriter* writer, const char* filename)
{
#ifdef HAVE_JSON_AREAS
    if (!writer)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "class JSON save: missing writer", -1 };

    json_t* root = json_object();
    json_object_set_new(root, "formatVersion", json_integer(1));
    json_t* classes = json_array();
    for (int i = 0; class_table && !IS_NULLSTR(class_table[i].name); i++) {
        Class* cls = &class_table[i];
        json_t* obj = json_object();
        json_object_set_new(obj, "name", json_string(cls->name));
        if (cls->who_name && cls->who_name[0] != '\0')
            json_object_set_new(obj, "whoName", json_string(cls->who_name));
        if (cls->base_group)
            json_object_set_new(obj, "baseGroup", json_string(cls->base_group));
        if (cls->default_group)
            json_object_set_new(obj, "defaultGroup", json_string(cls->default_group));
        json_object_set_new(obj, "weaponVnum", json_integer(cls->weapon));
        json_t* guilds = json_array();
        for (int g = 0; g < MAX_GUILD; g++) {
            json_array_append_new(guilds, json_integer(cls->guild[g]));
        }
        json_object_set_new(obj, "guilds", guilds);
        if (cls->prime_stat >= 0 && cls->prime_stat < STAT_COUNT)
            json_object_set_new(obj, "primeStat", json_string(stat_table[cls->prime_stat].name));
        json_object_set_new(obj, "skillCap", json_integer(cls->skill_cap));
        json_object_set_new(obj, "thac0_00", json_integer(cls->thac0_00));
        json_object_set_new(obj, "thac0_32", json_integer(cls->thac0_32));
        json_object_set_new(obj, "hpMin", json_integer(cls->hp_min));
        json_object_set_new(obj, "hpMax", json_integer(cls->hp_max));
        json_object_set_new(obj, "manaUser", json_boolean(cls->fMana));
        json_object_set_new(obj, "startLoc", json_integer(cls->start_loc));
        json_t* titles = json_array();
        for (int lvl = 0; lvl <= MAX_LEVEL; lvl++) {
            json_t* pair = json_array();
            const char* male = cls->titles[lvl][0];
            const char* female = cls->titles[lvl][1];
            json_array_append_new(pair, json_string(male ? male : ""));
            json_array_append_new(pair, json_string(female ? female : ""));
            json_array_append_new(titles, pair);
        }
        json_object_set_new(obj, "titles", titles);
        json_array_append_new(classes, obj);
    }
    json_object_set_new(root, "classes", classes);

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "class JSON save: dump failed", -1 };

    size_t len = strlen(dump);
    bool ok = writer_write_all(writer, dump, len);
    free(dump);
    if (!ok)
        return (PersistResult){ PERSIST_ERR_IO, "class JSON save: write failed", -1 };
    if (writer->ops->flush)
        writer->ops->flush(writer->ctx);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
#else
    (void)writer;
    (void)filename;
    return json_not_supported("JSON support not built");
#endif
}

PersistResult class_persist_json_load(const PersistReader* reader, const char* filename)
{
#ifdef HAVE_JSON_AREAS
    if (!reader)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "class JSON load: missing reader", -1 };

    ReaderBuffer buf = { 0 };
    if (!reader_fill_buffer(reader, &buf))
        return (PersistResult){ PERSIST_ERR_IO, "class JSON load: failed to read", -1 };

    json_error_t error;
    json_t* root = json_loadb(buf.data, buf.len, 0, &error);
    free(buf.data);
    if (!root) {
        static char msg[256];
        snprintf(msg, sizeof(msg), "class JSON parse error at line %d: %s", error.line, error.text);
        return (PersistResult){ PERSIST_ERR_FORMAT, msg, (int)error.line };
    }
    json_t* fmtv = json_object_get(root, "formatVersion");
    if (json_is_integer(fmtv) && json_integer_value(fmtv) != 1) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "class JSON: unsupported formatVersion", -1 };
    }

    json_t* classes = json_object_get(root, "classes");
    if (!json_is_array(classes)) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_FORMAT, "class JSON: missing classes array", -1 };
    }

    size_t count = json_array_size(classes);
    if (class_table)
        free(class_table);
    class_table = calloc(count + 1, sizeof(Class));
    if (!class_table) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_INTERNAL, "class JSON: alloc failed", -1 };
    }

    for (size_t i = 0; i < count; i++) {
        json_t* c = json_array_get(classes, i);
        if (!json_is_object(c))
            continue;
        Class* cls = &class_table[i];
        const char* name = json_string_value(json_object_get(c, "name"));
        if (name)
            cls->name = str_dup(name);
        const char* who = json_string_value(json_object_get(c, "whoName"));
        if (who)
            cls->who_name = str_dup(who);
        const char* base = json_string_value(json_object_get(c, "baseGroup"));
        if (base)
            cls->base_group = str_dup(base);
        const char* dflt = json_string_value(json_object_get(c, "defaultGroup"));
        if (dflt)
            cls->default_group = str_dup(dflt);
        cls->weapon = (VNUM)json_int_or_default(c, "weaponVnum", cls->weapon);
        json_t* guilds = json_object_get(c, "guilds");
        if (json_is_array(guilds)) {
            size_t gsz = json_array_size(guilds);
            for (size_t g = 0; g < gsz && g < MAX_GUILD; g++)
                cls->guild[g] = (VNUM)json_integer_value(json_array_get(guilds, g));
        }
        const char* prime = json_string_value(json_object_get(c, "primeStat"));
        if (prime) {
            FLAGS bit = flag_lookup(prime, stat_table);
            if (bit != NO_FLAG && bit < STAT_COUNT)
                cls->prime_stat = (int16_t)bit;
        }
        cls->skill_cap = (int16_t)json_int_or_default(c, "skillCap", cls->skill_cap);
        cls->thac0_00 = (int16_t)json_int_or_default(c, "thac0_00", cls->thac0_00);
        cls->thac0_32 = (int16_t)json_int_or_default(c, "thac0_32", cls->thac0_32);
        cls->hp_min = (int16_t)json_int_or_default(c, "hpMin", cls->hp_min);
        cls->hp_max = (int16_t)json_int_or_default(c, "hpMax", cls->hp_max);
        cls->fMana = json_is_true(json_object_get(c, "manaUser"));
        cls->start_loc = (VNUM)json_int_or_default(c, "startLoc", cls->start_loc);
        json_t* titles = json_object_get(c, "titles");
        if (json_is_array(titles)) {
            size_t tsz = json_array_size(titles);
            for (int lvl = 0; lvl <= MAX_LEVEL && (size_t)lvl < tsz; lvl++) {
                json_t* pair = json_array_get(titles, lvl);
                if (json_is_array(pair) && json_array_size(pair) >= 2) {
                    const char* male = json_string_value(json_array_get(pair, 0));
                    const char* female = json_string_value(json_array_get(pair, 1));
                    if (male)
                        cls->titles[lvl][0] = str_dup(male);
                    if (female)
                        cls->titles[lvl][1] = str_dup(female);
                }
            }
        }
    }

    class_count = (int)count;
    class_table[count].name = NULL;
    json_decref(root);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
#else
    (void)reader;
    (void)filename;
    return json_not_supported("JSON support not built");
#endif
}
