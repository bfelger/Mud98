////////////////////////////////////////////////////////////////////////////////
// persist/json/race_persist_json.c
////////////////////////////////////////////////////////////////////////////////

#include "race_persist_json.h"

#include <persist/persist_io_adapters.h>

#include <merc.h>
#include <comm.h>
#include <lookup.h>
#include <array.h>

#include <data/race.h>
#include <data/class.h>
#include <tables.h>

#include <jansson.h>
#include <string.h>
#include <stdlib.h>

typedef struct reader_buffer_t {
    char* data;
    size_t len;
    size_t cap;
} ReaderBuffer;

static bool reader_fill_buffer(const PersistReader* reader, ReaderBuffer* out)
{
    ReaderBuffer buf = { 0 };
    buf.cap = 1024;
    buf.data = malloc(buf.cap);
    if (!buf.data)
        return false;

    for (;;) {
        int ch = reader->ops->getc(reader->ctx);
        if (ch == EOF)
            break;
        if (buf.len + 1 > buf.cap) {
            size_t new_cap = buf.cap * 2;
            char* tmp = realloc(buf.data, new_cap);
            if (!tmp) {
                free(buf.data);
                return false;
            }
            buf.data = tmp;
            buf.cap = new_cap;
        }
        buf.data[buf.len++] = (char)ch;
    }

    out->data = buf.data;
    out->len = buf.len;
    out->cap = buf.cap;
    return true;
}

static json_t* flags_to_array(FLAGS flags, const struct flag_type* table)
{
    json_t* arr = json_array();
    if (!table)
        return arr;
    for (int i = 0; table[i].name != NULL; i++) {
        if (IS_SET(flags, table[i].bit)) {
            json_array_append_new(arr, json_string(table[i].name));
        }
    }
    return arr;
}

static FLAGS flags_from_array(json_t* arr, const struct flag_type* table)
{
    FLAGS flags = 0;
    if (!json_is_array(arr) || !table)
        return flags;
    size_t size = json_array_size(arr);
    for (size_t i = 0; i < size; i++) {
        const char* name = json_string_value(json_array_get(arr, i));
        if (!name)
            continue;
        FLAGS bit = flag_lookup(name, table);
        if (bit != NO_FLAG)
            SET_BIT(flags, bit);
    }
    return flags;
}

static int64_t json_int_or_default(json_t* obj, const char* key, int64_t def)
{
    json_t* val = json_object_get(obj, key);
    if (json_is_integer(val))
        return json_integer_value(val);
    return def;
}

static json_t* build_class_mult(const Race* race)
{
    json_t* obj = json_object();
    for (int i = 0; i < class_count; i++) {
        int16_t mult = GET_ELEM(&race->class_mult, i);
        json_object_set_new(obj, class_table[i].name, json_integer(mult));
    }
    return obj;
}

static json_t* build_class_start(const Race* race)
{
    json_t* obj = json_object();
    for (int i = 0; i < class_count; i++) {
        VNUM vnum = GET_ELEM(&race->class_start, i);
        if (vnum != 0)
            json_object_set_new(obj, class_table[i].name, json_integer(vnum));
    }
    return obj;
}

static void apply_class_mult(Race* race, json_t* arr)
{
    if (!json_is_object(arr) && !json_is_array(arr))
        return;
    if (json_is_object(arr)) {
        const char* key;
        json_t* val;
        json_object_foreach(arr, key, val) {
            int idx = class_lookup(key);
            if (idx < 0)
                continue;
            while ((int)race->class_mult.count <= idx)
                CREATE_ELEM(race->class_mult);
            race->class_mult.elems[idx] = (int16_t)json_integer_value(val);
        }
    }
    else {
        size_t sz = json_array_size(arr);
        for (size_t i = 0; i < sz; i++)
            *CREATE_ELEM(race->class_mult) = (int16_t)json_integer_value(json_array_get(arr, i));
    }
}

static void apply_class_start(Race* race, json_t* arr)
{
    if (!json_is_object(arr) && !json_is_array(arr))
        return;
    if (json_is_object(arr)) {
        const char* key;
        json_t* val;
        json_object_foreach(arr, key, val) {
            int idx = class_lookup(key);
            if (idx < 0)
                continue;
            while ((int)race->class_start.count <= idx)
                CREATE_ELEM(race->class_start);
            race->class_start.elems[idx] = (VNUM)json_integer_value(val);
        }
    }
    else {
        size_t sz = json_array_size(arr);
        for (size_t i = 0; i < sz; i++)
            *CREATE_ELEM(race->class_start) = (VNUM)json_integer_value(json_array_get(arr, i));
    }
}

static const char* size_name(MobSize size)
{
    if (size < 0 || size >= MOB_SIZE_COUNT)
        return mob_size_table[SIZE_MEDIUM].name;
    return mob_size_table[size].name;
}

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

PersistResult race_persist_json_save(const PersistWriter* writer, const char* filename)
{
    if (!writer)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "race JSON save: missing writer", -1 };

    json_t* root = json_object();
    json_object_set_new(root, "formatVersion", json_integer(1));
    json_t* races = json_array();
    for (int i = 0; race_table && !IS_NULLSTR(race_table[i].name); i++) {
        Race* race = &race_table[i];
        json_t* obj = json_object();
        json_object_set_new(obj, "name", json_string(race->name));
        json_object_set_new(obj, "whoName", json_string(race->who_name ? race->who_name : ""));
        json_object_set_new(obj, "pc", json_boolean(race->pc_race));
        json_object_set_new(obj, "points", json_integer(race->points));
        json_object_set_new(obj, "size", json_string(size_name(race->size)));
        json_t* stats = json_object();
        json_t* maxs = json_object();
        for (int s = 0; stat_table[s].name != NULL; s++) {
            json_object_set_new(stats, stat_table[s].name, json_integer(race->stats[s]));
            json_object_set_new(maxs, stat_table[s].name, json_integer(race->max_stats[s]));
        }
        json_object_set_new(obj, "stats", stats);
        json_object_set_new(obj, "maxStats", maxs);
        json_object_set_new(obj, "actFlags", flags_to_array(race->act_flags, act_flag_table));
        json_object_set_new(obj, "affectFlags", flags_to_array(race->aff, affect_flag_table));
        json_object_set_new(obj, "offFlags", flags_to_array(race->off, off_flag_table));
        json_object_set_new(obj, "immFlags", flags_to_array(race->imm, imm_flag_table));
        json_object_set_new(obj, "resFlags", flags_to_array(race->res, res_flag_table));
        json_object_set_new(obj, "vulnFlags", flags_to_array(race->vuln, vuln_flag_table));
        json_object_set_new(obj, "formFlags", flags_to_array(race->form, form_flag_table));
        json_object_set_new(obj, "partFlags", flags_to_array(race->parts, part_flag_table));
        json_object_set_new(obj, "classMult", build_class_mult(race));
        json_object_set_new(obj, "startLoc", json_integer(race->start_loc));
        json_object_set_new(obj, "classStart", build_class_start(race));
        json_t* skills = json_array();
        for (int s = 0; s < RACE_NUM_SKILLS; s++) {
            if (race->skills[s] && race->skills[s][0] != '\0')
                json_array_append_new(skills, json_string(race->skills[s]));
        }
        json_object_set_new(obj, "skills", skills);
        json_array_append_new(races, obj);
    }
    json_object_set_new(root, "races", races);

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "race JSON save: dump failed", -1 };

    size_t len = strlen(dump);
    bool ok = writer_write_all(writer, dump, len);
    free(dump);
    if (!ok)
        return (PersistResult){ PERSIST_ERR_IO, "race JSON save: write failed", -1 };
    if (writer->ops->flush)
        writer->ops->flush(writer->ctx);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult race_persist_json_load(const PersistReader* reader, const char* filename)
{
    if (!reader)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "race JSON load: missing reader", -1 };

    ReaderBuffer buf = { 0 };
    if (!reader_fill_buffer(reader, &buf))
        return (PersistResult){ PERSIST_ERR_IO, "race JSON load: failed to read", -1 };

    json_error_t error;
    json_t* root = json_loadb(buf.data, buf.len, 0, &error);
    free(buf.data);
    if (!root) {
        static char msg[256];
        snprintf(msg, sizeof(msg), "race JSON parse error at line %d: %s", error.line, error.text);
        return (PersistResult){ PERSIST_ERR_FORMAT, msg, (int)error.line };
    }
    json_t* fmtv = json_object_get(root, "formatVersion");
    if (json_is_integer(fmtv) && json_integer_value(fmtv) != 1) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "race JSON: unsupported formatVersion", -1 };
    }

    json_t* races = json_object_get(root, "races");
    if (!json_is_array(races)) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_FORMAT, "race JSON: missing races array", -1 };
    }

    size_t count = json_array_size(races);
    if (race_table)
        free(race_table);
    race_table = calloc(count + 1, sizeof(Race));
    if (!race_table) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_INTERNAL, "race JSON: alloc failed", -1 };
    }

    for (size_t i = 0; i < count; i++) {
        json_t* r = json_array_get(races, i);
        if (!json_is_object(r))
            continue;
        Race* race = &race_table[i];
        INIT_ARRAY(race->class_mult, ClassMult);
        INIT_ARRAY(race->class_start, StartLoc);
        const char* name = json_string_value(json_object_get(r, "name"));
        if (name)
            race->name = str_dup(name);
        const char* who = json_string_value(json_object_get(r, "whoName"));
        if (who)
            race->who_name = str_dup(who);
        race->pc_race = json_is_true(json_object_get(r, "pc"));
        race->points = (int16_t)json_int_or_default(r, "points", race->points);
        const char* size = json_string_value(json_object_get(r, "size"));
        if (size) {
            int s = size_lookup(size);
            if (s >= 0)
                race->size = (MobSize)s;
        }
        json_t* stats = json_object_get(r, "stats");
        if (json_is_object(stats)) {
            const char* key;
            json_t* val;
            json_object_foreach(stats, key, val) {
                FLAGS bit = flag_lookup(key, stat_table);
                if (bit != NO_FLAG && bit < STAT_COUNT)
                    race->stats[bit] = (int16_t)json_integer_value(val);
            }
        }
        else if (json_is_array(stats) && json_array_size(stats) >= STAT_COUNT) {
            for (int s = 0; s < STAT_COUNT; s++)
                race->stats[s] = (int16_t)json_integer_value(json_array_get(stats, s));
        }
        json_t* maxs = json_object_get(r, "maxStats");
        if (json_is_object(maxs)) {
            const char* key;
            json_t* val;
            json_object_foreach(maxs, key, val) {
                FLAGS bit = flag_lookup(key, stat_table);
                if (bit != NO_FLAG && bit < STAT_COUNT)
                    race->max_stats[bit] = (int16_t)json_integer_value(val);
            }
        }
        else if (json_is_array(maxs) && json_array_size(maxs) >= STAT_COUNT) {
            for (int s = 0; s < STAT_COUNT; s++)
                race->max_stats[s] = (int16_t)json_integer_value(json_array_get(maxs, s));
        }
        race->act_flags = flags_from_array(json_object_get(r, "actFlags"), act_flag_table);
        race->aff = flags_from_array(json_object_get(r, "affectFlags"), affect_flag_table);
        race->off = flags_from_array(json_object_get(r, "offFlags"), off_flag_table);
        race->imm = flags_from_array(json_object_get(r, "immFlags"), imm_flag_table);
        race->res = flags_from_array(json_object_get(r, "resFlags"), res_flag_table);
        race->vuln = flags_from_array(json_object_get(r, "vulnFlags"), vuln_flag_table);
        race->form = flags_from_array(json_object_get(r, "formFlags"), form_flag_table);
        race->parts = flags_from_array(json_object_get(r, "partFlags"), part_flag_table);
        apply_class_mult(race, json_object_get(r, "classMult"));
        race->start_loc = (VNUM)json_int_or_default(r, "startLoc", race->start_loc);
        apply_class_start(race, json_object_get(r, "classStart"));
        json_t* skills = json_object_get(r, "skills");
        if (json_is_array(skills)) {
            size_t sz = json_array_size(skills);
            for (size_t s = 0; s < sz && s < RACE_NUM_SKILLS; s++) {
                const char* sk = json_string_value(json_array_get(skills, s));
                if (sk)
                    race->skills[s] = str_dup(sk);
            }
        }
    }

    race_count = (int)count;
    race_table[count].name = NULL;
    init_race_table_lox();
    json_decref(root);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
