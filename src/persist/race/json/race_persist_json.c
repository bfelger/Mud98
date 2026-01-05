////////////////////////////////////////////////////////////////////////////////
// persist/json/race_persist_json.c
////////////////////////////////////////////////////////////////////////////////

#include "race_persist_json.h"

#include <persist/json/persist_json.h>
#include <persist/persist_io_adapters.h>
#include <persist/race/race_persist.h>

#include <merc.h>
#include <comm.h>
#include <lookup.h>
#include <array.h>

#include <data/race.h>
#include <data/class.h>
#include <tables.h>

#include <string.h>
#include <stdlib.h>

#include <jansson.h>

const RacePersistFormat RACE_PERSIST_JSON = { 
    .name = "json", 
    .load = race_persist_json_load, 
    .save = race_persist_json_save 
};

static json_t* build_class_mult(const Race* race)
{
    json_t* obj = json_object();
    bool has_value = false;
    for (int i = 0; i < class_count; i++) {
        int16_t mult = GET_ELEM(&race->class_mult, i);
        if (mult != 0)
            has_value = true;
        JSON_SET_INT(obj, class_table[i].name, mult);
    }
    if (!has_value) {
        json_decref(obj);
        return NULL;
    }
    return obj;
}

static json_t* build_class_start(const Race* race)
{
    json_t* obj = json_object();
    bool has_value = false;
    for (int i = 0; i < class_count; i++) {
        VNUM vnum = GET_ELEM(&race->class_start, i);
        if (vnum != 0) {
            has_value = true;
            JSON_SET_INT(obj, class_table[i].name, vnum);
        }
    }
    if (!has_value) {
        json_decref(obj);
        return NULL;
    }
    return obj;
}

static bool stats_have_values(const int16_t* stats)
{
    for (int i = 0; i < STAT_COUNT; i++) {
        if (stats[i] != 0)
            return true;
    }
    return false;
}

static void add_stats_if_needed(json_t* obj, const char* key, const int16_t* stats)
{
    if (!stats_have_values(stats))
        return;
    json_t* block = json_object();
    for (int s = 0; stat_table[s].name != NULL; s++)
        JSON_SET_INT(block, stat_table[s].name, stats[s]);
    json_object_set_new(obj, key, block);
}

static void set_flags_if_not_empty(json_t* obj, const char* key, FLAGS flags, const struct flag_type* table, const struct flag_type* defaults)
{
    json_t* arr = defaults ? flags_to_array_with_defaults(flags, defaults, table)
                           : flags_to_array(flags, table);
    if (json_array_size(arr) == 0) {
        json_decref(arr);
        return;
    }
    json_object_set_new(obj, key, arr);
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

PersistResult race_persist_json_save(const PersistWriter* writer, const char* filename)
{
    if (!writer)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "race JSON save: missing writer", -1 };

    json_t* root = json_object();
    JSON_SET_INT(root, "formatVersion", 1);
    json_t* races = json_array();
    for (int i = 0; race_table && !IS_NULLSTR(race_table[i].name); i++) {
        Race* race = &race_table[i];
        json_t* obj = json_object();
        JSON_SET_STRING(obj, "name", race->name);
        if (!IS_NULLSTR(race->who_name))
            JSON_SET_STRING(obj, "whoName", race->who_name);
        if (race->pc_race)
            json_object_set_new(obj, "pc", json_true());
        if (race->points != 0)
            JSON_SET_INT(obj, "points", race->points);
        JSON_SET_STRING(obj, "size", size_name(race->size));
        ArmorTier armor_type = armor_type_from_value(race->armor_prof);
        if (armor_type != ARMOR_OLD_STYLE)
            JSON_SET_STRING(obj, "armorProf", armor_type_name(armor_type));
        add_stats_if_needed(obj, "stats", race->stats);
        add_stats_if_needed(obj, "maxStats", race->max_stats);
        set_flags_if_not_empty(obj, "actFlags", race->act_flags, act_flag_table, NULL);
        set_flags_if_not_empty(obj, "affectFlags", race->aff, affect_flag_table, NULL);
        set_flags_if_not_empty(obj, "offFlags", race->off, off_flag_table, NULL);
        set_flags_if_not_empty(obj, "immFlags", race->imm, imm_flag_table, NULL);
        set_flags_if_not_empty(obj, "resFlags", race->res, res_flag_table, NULL);
        set_flags_if_not_empty(obj, "vulnFlags", race->vuln, vuln_flag_table, NULL);
        set_flags_if_not_empty(obj, "formFlags", race->form, form_flag_table, form_defaults_flag_table);
        set_flags_if_not_empty(obj, "partFlags", race->parts, part_flag_table, part_defaults_flag_table);
        if (race->pc_race) {
            json_t* class_mult = build_class_mult(race);
            if (class_mult)
                json_object_set_new(obj, "classMult", class_mult);
            if (race->start_loc != 0)
                JSON_SET_INT(obj, "startLoc", race->start_loc);
            json_t* class_start = build_class_start(race);
            if (class_start)
                json_object_set_new(obj, "classStart", class_start);
        }
        json_t* skills = json_array();
        for (int s = 0; s < RACE_NUM_SKILLS; s++) {
            if (race->skills[s] && race->skills[s][0] != '\0')
                json_array_append_new(skills, json_string(race->skills[s]));
        }
        if (json_array_size(skills) > 0)
            json_object_set_new(obj, "skills", skills);
        else
            json_decref(skills);
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
        const char* name = JSON_STRING(r, "name");
        if (name)
            race->name = boot_intern_string(name);
        const char* who = JSON_STRING(r, "whoName");
        if (who)
            race->who_name = boot_intern_string(who);
        race->pc_race = json_is_true(json_object_get(r, "pc"));
        race->points = (int16_t)json_int_or_default(r, "points", race->points);
        const char* size = JSON_STRING(r, "size");
        if (size) {
            int s = size_lookup(size);
            if (s >= 0)
                race->size = (MobSize)s;
        }
        json_t* armor_type = json_object_get(r, "armorProf");
        if (json_is_string(armor_type)) {
            int value = armor_type_lookup(json_string_value(armor_type));
            if (value >= 0)
                race->armor_prof = (ArmorTier)value;
        }
        else if (json_is_integer(armor_type)) {
            race->armor_prof = armor_type_from_value((int)json_integer_value(armor_type));
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
        race->act_flags = JSON_FLAGS(r, "actFlags", act_flag_table);
        race->aff = JSON_FLAGS(r, "affectFlags", affect_flag_table);
        race->off = JSON_FLAGS(r, "offFlags", off_flag_table);
        race->imm = JSON_FLAGS(r, "immFlags", imm_flag_table);
        race->res = JSON_FLAGS(r, "resFlags", res_flag_table);
        race->vuln = JSON_FLAGS(r, "vulnFlags", vuln_flag_table);
        race->form = flags_from_array_with_defaults(json_object_get(r, "formFlags"), form_defaults_flag_table, form_flag_table);
        race->parts = flags_from_array_with_defaults(json_object_get(r, "partFlags"), part_defaults_flag_table, part_flag_table);
        apply_class_mult(race, json_object_get(r, "classMult"));
        race->start_loc = (VNUM)json_int_or_default(r, "startLoc", race->start_loc);
        apply_class_start(race, json_object_get(r, "classStart"));
        json_t* skills = json_object_get(r, "skills");
        if (json_is_array(skills)) {
            size_t sz = json_array_size(skills);
            for (size_t s = 0; s < sz && s < RACE_NUM_SKILLS; s++) {
                const char* sk = json_string_value(json_array_get(skills, s));
                if (sk)
                    race->skills[s] = boot_intern_string(sk);
            }
        }
    }

    race_count = (int)count;
    race_table[count].name = NULL;
    init_race_table_lox();
    json_decref(root);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
