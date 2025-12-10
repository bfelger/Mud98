////////////////////////////////////////////////////////////////////////////////
// persist/skill/json/skill_persist_json.c
////////////////////////////////////////////////////////////////////////////////

#include "skill_persist_json.h"

#include <persist/json/persist_json.h>

#include <data/skill.h>
#include <data/class.h>

#include <comm.h>
#include <db.h>
#include <handler.h>
#include <lookup.h>

#include <tables.h>

#include <lox/vm.h>

#include <jansson.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

DECLARE_SPELL_FUN(spell_null);

const SkillPersistFormat SKILL_PERSIST_JSON = {
    .name = "json",
    .load = skill_persist_json_load,
    .save = skill_persist_json_save,
};

const SkillGroupPersistFormat SKILL_GROUP_PERSIST_JSON = {
    .name = "json",
    .load = skill_group_persist_json_load,
    .save = skill_group_persist_json_save,
};

static const char* flag_name_from_value(FLAGS value, const struct flag_type* table)
{
    if (!table)
        return NULL;
    for (int i = 0; table[i].name != NULL; ++i) {
        if (table[i].bit == value)
            return table[i].name;
    }
    return NULL;
}

static FLAGS flag_value_from_name(const char* name, const struct flag_type* table, FLAGS def)
{
    if (!name)
        return def;
    FLAGS bit = flag_lookup(name, table);
    if (bit == NO_FLAG)
        return def;
    return bit;
}

static const char* position_name_safe(Position pos)
{
    if (pos < 0 || pos >= POS_MAX)
        return position_table[POS_DEAD].name;
    return position_table[pos].name;
}

static void append_class_values(json_t* obj, const ARRAY(LEVEL)* arr)
{
    extern Class* class_table;
    extern int class_count;
    if (!class_table)
        return;

    for (int i = 0; i < class_count; ++i) {
        if (IS_NULLSTR(class_table[i].name))
            continue;
        LEVEL value = GET_ELEM(arr, i);
        JSON_SET_INT(obj, class_table[i].name, value);
    }
}

static void append_class_rating(json_t* obj, const ARRAY(SkillRating)* arr)
{
    extern Class* class_table;
    extern int class_count;
    if (!class_table)
        return;

    for (int i = 0; i < class_count; ++i) {
        if (IS_NULLSTR(class_table[i].name))
            continue;
        SkillRating value = GET_ELEM(arr, i);
        JSON_SET_INT(obj, class_table[i].name, value);
    }
}

static void apply_class_int_map(json_t* map, ARRAY(LEVEL)* arr)
{
    if (!json_is_object(map))
        return;
    const char* key;
    json_t* val;
    json_object_foreach(map, key, val) {
        if (!json_is_integer(val))
            continue;
        int class_index = class_lookup(key);
        if (class_index < 0)
            continue;
        while ((size_t)class_index >= arr->count)
            CREATE_ELEM(*arr);
        arr->elems[class_index] = (LEVEL)json_integer_value(val);
    }
}

static void apply_class_rating_map(json_t* map, ARRAY(SkillRating)* arr)
{
    if (!json_is_object(map))
        return;
    const char* key;
    json_t* val;
    json_object_foreach(map, key, val) {
        if (!json_is_integer(val))
            continue;
        int class_index = class_lookup(key);
        if (class_index < 0)
            continue;
        while ((size_t)class_index >= arr->count)
            CREATE_ELEM(*arr);
        arr->elems[class_index] = (SkillRating)json_integer_value(val);
    }
}

static bool set_lox_closure_from_name(Skill* skill, const char* name)
{
    if (IS_NULLSTR(name))
        return true;

    skill->lox_spell_name = lox_string(name);
    Value value;
    if (!table_get(&vm.globals, skill->lox_spell_name, &value)) {
        bugf("skill JSON load: unknown Lox symbol '%s'", name);
        return false;
    }
    if (!IS_CLOSURE(value)) {
        bugf("skill JSON load: '%s' is not callable", name);
        return false;
    }
    skill->lox_closure = AS_CLOSURE(value);
    return true;
}

PersistResult skill_persist_json_save(const PersistWriter* writer, const char* filename)
{
    if (!writer)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "skill JSON save: missing writer", -1 };

    extern Skill* skill_table;

    json_t* root = json_object();
    JSON_SET_INT(root, "formatVersion", 1);
    json_t* skills = json_array();

    for (int i = 0; skill_table && !IS_NULLSTR(skill_table[i].name); ++i) {
        Skill* skill = &skill_table[i];
        if (IS_NULLSTR(skill->name))
            continue;

        json_t* obj = json_object();
        JSON_SET_STRING(obj, "name", skill->name);

        json_t* levels = json_object();
        append_class_values(levels, &skill->skill_level);
        json_object_set_new(obj, "levels", levels);

        json_t* ratings = json_object();
        append_class_rating(ratings, &skill->rating);
        json_object_set_new(obj, "ratings", ratings);

        if (skill->spell_fun && skill->spell_fun != spell_null) {
            const char* spell = spell_name(skill->spell_fun);
            if (!IS_NULLSTR(spell))
                JSON_SET_STRING(obj, "spell", spell);
        }

        if (skill->lox_spell_name && skill->lox_spell_name->chars && skill->lox_spell_name->chars[0] != '\0')
            JSON_SET_STRING(obj, "loxSpell", skill->lox_spell_name->chars);

        const char* target_name = flag_name_from_value(skill->target, target_table);
        if (target_name)
            JSON_SET_STRING(obj, "target", target_name);

        JSON_SET_STRING(obj, "minPosition", position_name_safe(skill->minimum_position));

        if (skill->pgsn) {
            const char* gsn = gsn_name(skill->pgsn);
            if (!IS_NULLSTR(gsn))
                JSON_SET_STRING(obj, "gsn", gsn);
        }

        if (skill->slot != 0)
            JSON_SET_INT(obj, "slot", skill->slot);
        if (skill->min_mana != 0)
            JSON_SET_INT(obj, "minMana", skill->min_mana);
        if (skill->beats != 0)
            JSON_SET_INT(obj, "beats", skill->beats);

        if (!IS_NULLSTR(skill->noun_damage))
            JSON_SET_STRING(obj, "nounDamage", skill->noun_damage);
        if (!IS_NULLSTR(skill->msg_off))
            JSON_SET_STRING(obj, "msgOff", skill->msg_off);
        if (!IS_NULLSTR(skill->msg_obj))
            JSON_SET_STRING(obj, "msgObj", skill->msg_obj);

        json_array_append_new(skills, obj);
    }

    json_object_set_new(root, "skills", skills);

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "skill JSON save: dump failed", -1 };

    size_t len = strlen(dump);
    bool ok = writer_write_all(writer, dump, len);
    free(dump);
    if (!ok)
        return (PersistResult){ PERSIST_ERR_IO, "skill JSON save: write failed", -1 };
    if (writer->ops->flush)
        writer->ops->flush(writer->ctx);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult skill_persist_json_load(const PersistReader* reader, const char* filename)
{
    if (!reader)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "skill JSON load: missing reader", -1 };

    ReaderBuffer buf = { 0 };
    if (!reader_fill_buffer(reader, &buf))
        return (PersistResult){ PERSIST_ERR_IO, "skill JSON load: failed to read", -1 };

    json_error_t error;
    json_t* root = json_loadb(buf.data, buf.len, 0, &error);
    free(buf.data);
    if (!root) {
        static char msg[256];
        snprintf(msg, sizeof msg, "skill JSON parse error at line %d: %s", error.line, error.text);
        return (PersistResult){ PERSIST_ERR_FORMAT, msg, (int)error.line };
    }

    json_t* fmtv = json_object_get(root, "formatVersion");
    if (json_is_integer(fmtv) && json_integer_value(fmtv) != 1) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "skill JSON: unsupported formatVersion", -1 };
    }

    json_t* skills = json_object_get(root, "skills");
    if (!json_is_array(skills)) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_FORMAT, "skill JSON: missing skills array", -1 };
    }

    size_t count = json_array_size(skills);
    extern Skill* skill_table;
    extern int skill_count;

    if (skill_table)
        free(skill_table);
    skill_table = calloc(count + 1, sizeof(Skill));
    if (!skill_table) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_INTERNAL, "skill JSON: alloc failed", -1 };
    }

    for (size_t i = 0; i < count; ++i) {
        json_t* skill_obj = json_array_get(skills, i);
        if (!json_is_object(skill_obj))
            continue;
        Skill* skill = &skill_table[i];
        INIT_ARRAY(skill->skill_level, LEVEL);
        INIT_ARRAY(skill->rating, SkillRating);

        const char* name = JSON_STRING(skill_obj, "name");
        if (name)
            skill->name = boot_intern_string(name);

        json_t* levels = json_object_get(skill_obj, "levels");
        if (levels)
            apply_class_int_map(levels, &skill->skill_level);

        json_t* ratings = json_object_get(skill_obj, "ratings");
        if (ratings)
            apply_class_rating_map(ratings, &skill->rating);

        const char* spell = JSON_STRING(skill_obj, "spell");
        if (spell && spell[0] != '\0')
            skill->spell_fun = spell_function((char*)spell);
        else
            skill->spell_fun = spell_null;

        const char* lox = JSON_STRING(skill_obj, "loxSpell");
        if (lox && lox[0] != '\0')
            set_lox_closure_from_name(skill, lox);

        const char* target = JSON_STRING(skill_obj, "target");
        skill->target = (int16_t)flag_value_from_name(target, target_table, SKILL_TARGET_IGNORE);

        const char* pos = JSON_STRING(skill_obj, "minPosition");
        if (pos)
            skill->minimum_position = position_lookup(pos);
        else
            skill->minimum_position = POS_DEAD;

        const char* gsn = JSON_STRING(skill_obj, "gsn");
        if (gsn && gsn[0] != '\0')
            skill->pgsn = gsn_lookup((char*)gsn);

        skill->slot = (int)json_int_or_default(skill_obj, "slot", skill->slot);
        skill->min_mana = (int16_t)json_int_or_default(skill_obj, "minMana", skill->min_mana);
        skill->beats = (int16_t)json_int_or_default(skill_obj, "beats", skill->beats);

        const char* noun = JSON_STRING(skill_obj, "nounDamage");
        if (noun)
            skill->noun_damage = boot_intern_string(noun);
        const char* msg_off = JSON_STRING(skill_obj, "msgOff");
        if (msg_off)
            skill->msg_off = boot_intern_string(msg_off);
        const char* msg_obj = JSON_STRING(skill_obj, "msgObj");
        if (msg_obj)
            skill->msg_obj = boot_intern_string(msg_obj);
    }

    skill_table[count].name = NULL;
    skill_count = (int)count;
    json_decref(root);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult skill_group_persist_json_save(const PersistWriter* writer, const char* filename)
{
    if (!writer)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "skill group JSON save: missing writer", -1 };

    extern SkillGroup* skill_group_table;

    json_t* root = json_object();
    JSON_SET_INT(root, "formatVersion", 1);
    json_t* groups = json_array();

    for (int i = 0; skill_group_table && !IS_NULLSTR(skill_group_table[i].name); ++i) {
        SkillGroup* group = &skill_group_table[i];
        if (IS_NULLSTR(group->name))
            continue;
        json_t* obj = json_object();
        JSON_SET_STRING(obj, "name", group->name);

        json_t* ratings = json_object();
        append_class_rating(ratings, &group->rating);
        json_object_set_new(obj, "ratings", ratings);

        json_t* skills = json_array();
        for (int s = 0; s < MAX_IN_GROUP; ++s) {
            if (group->skills[s] && group->skills[s][0] != '\0')
                json_array_append_new(skills, json_string(group->skills[s]));
        }
        json_object_set_new(obj, "skills", skills);

        json_array_append_new(groups, obj);
    }

    json_object_set_new(root, "groups", groups);

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "skill group JSON save: dump failed", -1 };

    size_t len = strlen(dump);
    bool ok = writer_write_all(writer, dump, len);
    free(dump);
    if (!ok)
        return (PersistResult){ PERSIST_ERR_IO, "skill group JSON save: write failed", -1 };
    if (writer->ops->flush)
        writer->ops->flush(writer->ctx);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult skill_group_persist_json_load(const PersistReader* reader, const char* filename)
{
    if (!reader)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "skill group JSON load: missing reader", -1 };

    ReaderBuffer buf = { 0 };
    if (!reader_fill_buffer(reader, &buf))
        return (PersistResult){ PERSIST_ERR_IO, "skill group JSON load: failed to read", -1 };

    json_error_t error;
    json_t* root = json_loadb(buf.data, buf.len, 0, &error);
    free(buf.data);
    if (!root) {
        static char msg[256];
        snprintf(msg, sizeof msg, "skill group JSON parse error at line %d: %s", error.line, error.text);
        return (PersistResult){ PERSIST_ERR_FORMAT, msg, (int)error.line };
    }

    json_t* fmtv = json_object_get(root, "formatVersion");
    if (json_is_integer(fmtv) && json_integer_value(fmtv) != 1) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "skill group JSON: unsupported formatVersion", -1 };
    }

    json_t* groups = json_object_get(root, "groups");
    if (!json_is_array(groups)) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_FORMAT, "skill group JSON: missing groups array", -1 };
    }

    size_t count = json_array_size(groups);
    extern SkillGroup* skill_group_table;
    extern int skill_group_count;

    if (skill_group_table)
        free(skill_group_table);
    skill_group_table = calloc(count + 1, sizeof(SkillGroup));
    if (!skill_group_table) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_INTERNAL, "skill group JSON: alloc failed", -1 };
    }

    for (size_t i = 0; i < count; ++i) {
        json_t* group_obj = json_array_get(groups, i);
        if (!json_is_object(group_obj))
            continue;
        SkillGroup* group = &skill_group_table[i];
        INIT_ARRAY(group->rating, SkillRating);

        const char* name = JSON_STRING(group_obj, "name");
        if (name)
            group->name = boot_intern_string(name);

        json_t* ratings = json_object_get(group_obj, "ratings");
        if (ratings)
            apply_class_rating_map(ratings, &group->rating);

        json_t* skills = json_object_get(group_obj, "skills");
        if (json_is_array(skills)) {
            size_t sz = json_array_size(skills);
            size_t limit = sz < MAX_IN_GROUP ? sz : MAX_IN_GROUP;
            for (size_t s = 0; s < limit; ++s) {
                const char* skill_name = json_string_value(json_array_get(skills, s));
                if (skill_name)
                    group->skills[s] = boot_intern_string(skill_name);
            }
        }
    }

    skill_group_table[count].name = NULL;
    skill_group_count = (int)count;
    json_decref(root);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
