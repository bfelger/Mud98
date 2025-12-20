////////////////////////////////////////////////////////////////////////////////
// persist/player/json/player_persist_json.c
// JSON player persistence implementation.
////////////////////////////////////////////////////////////////////////////////

#include <persist/player/player_persist.h>

#include <persist/json/persist_json.h>
#include <persist/persist_io_adapters.h>

#include <color.h>
#include <comm.h>
#include <config.h>
#include <db.h>
#include <digest.h>
#include <handler.h>
#include <lookup.h>
#include <magic.h>
#include <data/quest.h>
#include <data/race.h>
#include <data/tutorial.h>
#include <recycle.h>
#include <skills.h>
#include <stringutils.h>
#include <tables.h>
#include <vt.h>

#include <entities/faction.h>
#include <entities/object.h>

#include <jansson.h>
#include <lox/lox.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define PLAYER_JSON_FORMAT_VERSION 1

static const char* color_mode_to_string(ColorMode mode)
{
    switch (mode) {
    case COLOR_MODE_16: return "ansi";
    case COLOR_MODE_256: return "256";
    case COLOR_MODE_RGB: return "rgb";
    case COLOR_MODE_PAL_IDX: return "palette";
    default: return NULL;
    }
}

static bool color_mode_from_string(const char* name, ColorMode* out_mode)
{
    if (!name || !out_mode)
        return false;

    if (!str_cmp(name, "ansi"))
        *out_mode = COLOR_MODE_16;
    else if (!str_cmp(name, "256"))
        *out_mode = COLOR_MODE_256;
    else if (!str_cmp(name, "rgb"))
        *out_mode = COLOR_MODE_RGB;
    else if (!str_cmp(name, "palette"))
        *out_mode = COLOR_MODE_PAL_IDX;
    else
        return false;

    return true;
}

static int color_code_count(ColorMode mode)
{
    switch (mode) {
    case COLOR_MODE_RGB: return 3;
    case COLOR_MODE_16: return 2;
    default: return 1;
    }
}

static json_t* color_to_json(const Color* color)
{
    if (!color)
        return NULL;

    const char* mode_name = color_mode_to_string(color->mode);
    if (!mode_name)
        return NULL;

    json_t* obj = json_object();
    JSON_SET_STRING(obj, "mode", mode_name);
    json_t* code = json_array();
    int count = color_code_count(color->mode);
    for (int i = 0; i < count; ++i)
        json_array_append_new(code, json_integer(color->code[i]));
    json_object_set_new(obj, "code", code);
    return obj;
}

static bool color_from_json(json_t* obj, Color* out_color)
{
    if (!json_is_object(obj) || !out_color)
        return false;

    const char* mode_name = JSON_STRING(obj, "mode");
    ColorMode mode = COLOR_MODE_16;
    if (!color_mode_from_string(mode_name, &mode))
        return false;

    json_t* code = json_object_get(obj, "code");
    if (!json_is_array(code))
        return false;

    out_color->mode = mode;
    out_color->code[0] = out_color->code[1] = out_color->code[2] = 0;
    int expected = color_code_count(mode);
    size_t arr_size = json_array_size(code);
    for (int i = 0; i < expected && (size_t)i < arr_size; ++i) {
        json_t* val = json_array_get(code, (size_t)i);
        if (!json_is_integer(val))
            return false;
        out_color->code[i] = (uint8_t)json_integer_value(val);
    }
    out_color->cache = NULL;
    out_color->xterm = NULL;
    return true;
}

static json_t* theme_to_json(const ColorTheme* theme)
{
    if (!theme)
        return NULL;

    json_t* obj = json_object();
    JSON_SET_STRING(obj, "name", theme->name);
    switch (theme->type) {
    case COLOR_THEME_TYPE_SYSTEM:
        JSON_SET_STRING(obj, "type", "system");
        break;
    case COLOR_THEME_TYPE_SYSTEM_COPY:
        JSON_SET_STRING(obj, "type", "system_copy");
        break;
    case COLOR_THEME_TYPE_CUSTOM:
    default:
        JSON_SET_STRING(obj, "type", "custom");
        break;
    }
    JSON_SET_STRING(obj, "mode", color_mode_to_string(theme->mode));
    json_object_set_new(obj, "isPublic", json_boolean(theme->is_public));
    json_object_set_new(obj, "isChanged", json_boolean(theme->is_changed));
    if (theme->banner && theme->banner[0] != '\0')
        JSON_SET_STRING(obj, "banner", theme->banner);

    json_t* palette = json_array();
    for (int i = 0; i < theme->palette_max && i < PALETTE_SIZE; ++i) {
        json_t* color = color_to_json(&theme->palette[i]);
        if (!color)
            continue;
        json_array_append_new(palette, color);
    }
    json_object_set_new(obj, "palette", palette);

    json_t* channels = json_object();
    for (int i = 0; i < COLOR_SLOT_COUNT; ++i) {
        json_t* color = color_to_json(&theme->channels[i]);
        if (!color)
            continue;
        json_object_set_new(channels, color_slot_entries[i].name, color);
    }
    json_object_set_new(obj, "channels", channels);
    return obj;
}

static ColorTheme* theme_from_json(json_t* obj)
{
    if (!json_is_object(obj))
        return NULL;

    ColorTheme* theme = new_color_theme();
    if (!theme)
        return NULL;

    const char* name = JSON_STRING(obj, "name");
    if (name && name[0])
        theme->name = str_dup(name);

    const char* type = JSON_STRING(obj, "type");
    if (type && !str_cmp(type, "system"))
        theme->type = COLOR_THEME_TYPE_SYSTEM;
    else if (type && !str_cmp(type, "system_copy"))
        theme->type = COLOR_THEME_TYPE_SYSTEM_COPY;
    else
        theme->type = COLOR_THEME_TYPE_CUSTOM;

    const char* mode_name = JSON_STRING(obj, "mode");
    ColorMode mode = COLOR_MODE_16;
    if (color_mode_from_string(mode_name, &mode))
        theme->mode = mode;

    theme->is_public = json_is_true(json_object_get(obj, "isPublic"));
    theme->is_changed = json_is_true(json_object_get(obj, "isChanged"));

    const char* banner = JSON_STRING(obj, "banner");
    if (banner)
        theme->banner = str_dup(banner);

    json_t* palette = json_object_get(obj, "palette");
    if (json_is_array(palette)) {
        size_t count = json_array_size(palette);
        theme->palette_max = (int)UMAX(UMIN((int)count, PALETTE_SIZE), 0);
        for (size_t i = 0; i < count && i < PALETTE_SIZE; ++i) {
            json_t* entry = json_array_get(palette, i);
            color_from_json(entry, &theme->palette[i]);
        }
    }

    json_t* channels = json_object_get(obj, "channels");
    if (json_is_object(channels)) {
        for (int i = 0; i < COLOR_SLOT_COUNT; ++i) {
            json_t* entry = json_object_get(channels, color_slot_entries[i].name);
            if (!entry)
                continue;
            color_from_json(entry, &theme->channels[i]);
        }
    }

    return theme;
}

static json_t* json_from_affect(const Affect* aff)
{
    if (!aff || aff->type < 0 || aff->type >= skill_count)
        return NULL;
    json_t* obj = json_object();
    JSON_SET_STRING(obj, "skill", skill_table[aff->type].name);
    JSON_SET_INT(obj, "where", aff->where);
    JSON_SET_INT(obj, "level", aff->level);
    JSON_SET_INT(obj, "duration", aff->duration);
    JSON_SET_INT(obj, "modifier", aff->modifier);
    JSON_SET_INT(obj, "location", aff->location);
    JSON_SET_INT(obj, "bitvector", aff->bitvector);
    return obj;
}

static void apply_affect_to_list(Affect** head, Affect* affect)
{
    affect->next = *head;
    *head = affect;
}

static void affects_from_json(json_t* arr, Affect** head)
{
    if (!json_is_array(arr) || !head)
        return;

    size_t count = json_array_size(arr);
    for (size_t i = 0; i < count; ++i) {
        json_t* obj = json_array_get(arr, i);
        if (!json_is_object(obj))
            continue;
        const char* skill_name = JSON_STRING(obj, "skill");
        if (!skill_name)
            continue;
        SKNUM sn = skill_lookup(skill_name);
        if (sn < 0)
            continue;
        Affect* affect = new_affect();
        affect->type = sn;
        affect->where = (int16_t)json_int_or_default(obj, "where", TO_AFFECTS);
        affect->level = (LEVEL)json_int_or_default(obj, "level", 0);
        affect->duration = (int16_t)json_int_or_default(obj, "duration", 0);
        affect->modifier = (int16_t)json_int_or_default(obj, "modifier", 0);
        affect->location = (int16_t)json_int_or_default(obj, "location", 0);
        affect->bitvector = (int)json_int_or_default(obj, "bitvector", 0);
        apply_affect_to_list(head, affect);
    }
}

static void json_add_aliases(json_t* root, PlayerData* pcdata)
{
    if (!pcdata)
        return;
    json_t* aliases = json_array();
    for (int i = 0; i < MAX_ALIAS; ++i) {
        if (!pcdata->alias[i] || !pcdata->alias_sub[i])
            break;
        json_t* obj = json_object();
        JSON_SET_STRING(obj, "alias", pcdata->alias[i]);
        JSON_SET_STRING(obj, "sub", pcdata->alias_sub[i]);
        json_array_append_new(aliases, obj);
    }
    json_object_set_new(root, "aliases", aliases);
}

static void aliases_from_json(json_t* arr, PlayerData* pcdata)
{
    if (!json_is_array(arr) || !pcdata)
        return;
    for (int i = 0; i < MAX_ALIAS; ++i) {
        free_string(pcdata->alias[i]);
        free_string(pcdata->alias_sub[i]);
        pcdata->alias[i] = NULL;
        pcdata->alias_sub[i] = NULL;
    }
    size_t count = json_array_size(arr);
    size_t idx = 0;
    for (size_t i = 0; i < count && idx < MAX_ALIAS; ++i) {
        json_t* obj = json_array_get(arr, i);
        if (!json_is_object(obj))
            continue;
        const char* name = JSON_STRING(obj, "alias");
        const char* sub = JSON_STRING(obj, "sub");
        if (!name || !sub)
            continue;
        free_string(pcdata->alias[idx]);
        free_string(pcdata->alias_sub[idx]);
        pcdata->alias[idx] = str_dup(name);
        pcdata->alias_sub[idx] = str_dup(sub);
        ++idx;
    }
}

static json_t* json_from_reputations(const PlayerData* pcdata)
{
    json_t* arr = json_array();
    if (!pcdata || !pcdata->reputations.entries)
        return arr;
    for (size_t i = 0; i < pcdata->reputations.count; ++i) {
        FactionReputation* entry = &pcdata->reputations.entries[i];
        if (entry->vnum == 0)
            continue;
        json_t* obj = json_object();
        JSON_SET_INT(obj, "vnum", entry->vnum);
        JSON_SET_INT(obj, "value", entry->value);
        json_array_append_new(arr, obj);
    }
    return arr;
}

static void reputations_from_json(json_t* arr, PlayerData* pcdata)
{
    if (!json_is_array(arr) || !pcdata)
        return;
    size_t count = json_array_size(arr);
    for (size_t i = 0; i < count; ++i) {
        json_t* obj = json_array_get(arr, i);
        if (!json_is_object(obj))
            continue;
        VNUM vnum = (VNUM)json_int_or_default(obj, "vnum", 0);
        int value = (int)json_int_or_default(obj, "value", 0);
        if (vnum != 0)
            faction_set(pcdata, vnum, value);
    }
}

static json_t* json_from_skills(const PlayerData* pcdata)
{
    json_t* arr = json_array();
    if (!pcdata || !pcdata->learned)
        return arr;
    for (SKNUM sn = 0; sn < skill_count; ++sn) {
        if (skill_table[sn].name == NULL || pcdata->learned[sn] <= 0)
            continue;
        json_t* obj = json_object();
        JSON_SET_STRING(obj, "skill", skill_table[sn].name);
        JSON_SET_INT(obj, "value", pcdata->learned[sn]);
        json_array_append_new(arr, obj);
    }
    return arr;
}

static void skills_from_json(json_t* arr, PlayerData* pcdata)
{
    if (!json_is_array(arr) || !pcdata || !pcdata->learned)
        return;
    size_t count = json_array_size(arr);
    for (size_t i = 0; i < count; ++i) {
        json_t* obj = json_array_get(arr, i);
        if (!json_is_object(obj))
            continue;
        const char* name = JSON_STRING(obj, "skill");
        if (!name)
            continue;
        SKNUM sn = skill_lookup(name);
        if (sn < 0)
            continue;
        pcdata->learned[sn] = (int16_t)json_int_or_default(obj, "value", 0);
    }
}

static json_t* json_from_groups(const PlayerData* pcdata)
{
    json_t* arr = json_array();
    if (!pcdata || !pcdata->group_known)
        return arr;
    for (SKNUM gn = 0; gn < skill_group_count; ++gn) {
        if (!pcdata->group_known[gn] || skill_group_table[gn].name == NULL)
            continue;
        json_array_append_new(arr, json_string(skill_group_table[gn].name));
    }
    return arr;
}

static void groups_from_json(json_t* arr, Mobile* ch)
{
    if (!json_is_array(arr) || !ch)
        return;
    size_t count = json_array_size(arr);
    for (size_t i = 0; i < count; ++i) {
        const char* name = json_string_value(json_array_get(arr, i));
        if (!name)
            continue;
        SKNUM gn = group_lookup(name);
        if (gn >= 0)
            gn_add(ch, gn);
    }
}

static json_t* json_from_money(const Mobile* ch)
{
    json_t* obj = json_object();
    JSON_SET_INT(obj, "gold", ch->gold);
    JSON_SET_INT(obj, "silver", ch->silver);
    JSON_SET_INT(obj, "copper", ch->copper);
    return obj;
}

static json_t* json_from_hmv(const Mobile* ch)
{
    json_t* obj = json_object();
    JSON_SET_INT(obj, "hit", ch->hit);
    JSON_SET_INT(obj, "maxHit", ch->max_hit);
    JSON_SET_INT(obj, "mana", ch->mana);
    JSON_SET_INT(obj, "maxMana", ch->max_mana);
    JSON_SET_INT(obj, "move", ch->move);
    JSON_SET_INT(obj, "maxMove", ch->max_move);
    return obj;
}

static json_t* json_from_perm_stats(const Mobile* ch)
{
    json_t* arr = json_array();
    for (int i = 0; i < STAT_COUNT; ++i)
        json_array_append_new(arr, json_integer(ch->perm_stat[i]));
    return arr;
}

static json_t* json_from_mod_stats(const Mobile* ch)
{
    json_t* arr = json_array();
    for (int i = 0; i < STAT_COUNT; ++i)
        json_array_append_new(arr, json_integer(ch->mod_stat[i]));
    return arr;
}

static json_t* json_from_armor(const Mobile* ch)
{
    json_t* arr = json_array();
    for (int i = 0; i < AC_COUNT; ++i)
        json_array_append_new(arr, json_integer(ch->armor[i]));
    return arr;
}

static json_t* json_from_theme_config(const PlayerData* pcdata)
{
    json_t* obj = json_object();
    json_object_set_new(obj, "hide24bit", json_boolean(pcdata->theme_config.hide_24bit));
    json_object_set_new(obj, "hide256", json_boolean(pcdata->theme_config.hide_256));
    json_object_set_new(obj, "xterm", json_boolean(pcdata->theme_config.xterm));
    json_object_set_new(obj, "hideRgbHelp", json_boolean(pcdata->theme_config.hide_rgb_help));
    return obj;
}

static void theme_config_from_json(json_t* obj, PlayerData* pcdata)
{
    if (!json_is_object(obj) || !pcdata)
        return;
    pcdata->theme_config.hide_24bit = json_is_true(json_object_get(obj, "hide24bit"));
    pcdata->theme_config.hide_256 = json_is_true(json_object_get(obj, "hide256"));
    pcdata->theme_config.xterm = json_is_true(json_object_get(obj, "xterm"));
    pcdata->theme_config.hide_rgb_help = json_is_true(json_object_get(obj, "hideRgbHelp"));
}

static json_t* json_from_personal_themes(const PlayerData* pcdata)
{
    json_t* arr = json_array();
    if (!pcdata)
        return arr;
    for (int i = 0; i < MAX_THEMES; ++i) {
        ColorTheme* theme = pcdata->color_themes[i];
        if (!theme)
            continue;
        json_t* obj = theme_to_json(theme);
        if (obj)
            json_array_append_new(arr, obj);
    }
    return arr;
}

static void personal_themes_from_json(json_t* arr, PlayerData* pcdata)
{
    if (!json_is_array(arr) || !pcdata)
        return;
    size_t count = json_array_size(arr);
    size_t idx = 0;
    for (size_t i = 0; i < count && idx < MAX_THEMES; ++i) {
        json_t* obj = json_array_get(arr, i);
        if (!json_is_object(obj))
            continue;
        ColorTheme* theme = theme_from_json(obj);
        if (!theme)
            continue;
        if (pcdata->color_themes[idx])
            free_color_theme(pcdata->color_themes[idx]);
        pcdata->color_themes[idx++] = theme;
    }
    for (; idx < MAX_THEMES; ++idx) {
        if (pcdata->color_themes[idx]) {
            free_color_theme(pcdata->color_themes[idx]);
            pcdata->color_themes[idx] = NULL;
        }
    }
}

static json_t* json_from_quest_log(const PlayerData* pcdata)
{
    json_t* arr = json_array();
    if (!pcdata || !pcdata->quest_log)
        return arr;

    QuestStatus* qs;
    FOR_EACH(qs, pcdata->quest_log->quests) {
        json_t* obj = json_object();
        JSON_SET_INT(obj, "vnum", qs->vnum);
        JSON_SET_INT(obj, "progress", qs->progress);
        JSON_SET_INT(obj, "state", qs->state);
        json_array_append_new(arr, obj);
    }
    return arr;
}

static void quest_log_from_json(json_t* arr, Mobile* ch)
{
    if (!json_is_array(arr) || !ch || !ch->pcdata || !ch->pcdata->quest_log)
        return;
    size_t count = json_array_size(arr);
    for (size_t i = 0; i < count; ++i) {
        json_t* obj = json_array_get(arr, i);
        if (!json_is_object(obj))
            continue;
        VNUM vnum = (VNUM)json_int_or_default(obj, "vnum", 0);
        Quest* quest = get_quest(vnum);
        if (!quest)
            continue;
        int progress = (int)json_int_or_default(obj, "progress", 0);
        QuestState state = (QuestState)json_int_or_default(obj, "state", QSTAT_ACCEPTED);
        add_quest_to_log(ch->pcdata->quest_log, quest, state, progress);
    }
}

static json_t* json_from_object(const Object* obj)
{
    if (!obj || !obj->prototype)
        return NULL;
    json_t* node = json_object();
    JSON_SET_INT(node, "vnum", VNUM_FIELD(obj->prototype));
    json_object_set_new(node, "enchanted", json_boolean(obj->enchanted));
    JSON_SET_STRING(node, "name", NAME_STR(obj));
    JSON_SET_STRING(node, "shortDescr", obj->short_descr);
    JSON_SET_STRING(node, "description", obj->description);
    JSON_SET_STRING(node, "material", obj->material ? obj->material : "");
    json_object_set_new(node, "extraFlags", json_integer(obj->extra_flags));
    json_object_set_new(node, "wearFlags", json_integer(obj->wear_flags));
    json_object_set_new(node, "itemType", json_integer(obj->item_type));
    json_object_set_new(node, "weight", json_integer(obj->weight));
    json_object_set_new(node, "condition", json_integer(obj->condition));
    json_object_set_new(node, "wearLoc", json_integer(obj->wear_loc));
    json_object_set_new(node, "level", json_integer(obj->level));
    json_object_set_new(node, "timer", json_integer(obj->timer));
    json_object_set_new(node, "cost", json_integer(obj->cost));

    json_t* values = json_array();
    for (int i = 0; i < 5; ++i)
        json_array_append_new(values, json_integer(obj->value[i]));
    json_object_set_new(node, "values", values);

    json_t* affects = json_array();
    for (Affect* affect = obj->affected; affect != NULL; affect = affect->next) {
        json_t* aff = json_from_affect(affect);
        if (aff)
            json_array_append_new(affects, aff);
    }
    json_object_set_new(node, "affects", affects);

    json_t* extra = json_array();
    for (ExtraDesc* ed = obj->extra_desc; ed != NULL; ed = ed->next) {
        if (!ed->keyword || !ed->description)
            continue;
        json_t* desc = json_object();
        JSON_SET_STRING(desc, "keyword", ed->keyword);
        JSON_SET_STRING(desc, "description", ed->description);
        json_array_append_new(extra, desc);
    }
    json_object_set_new(node, "extraDescriptions", extra);

    json_t* spells = json_array();
    if (obj->item_type == ITEM_POTION || obj->item_type == ITEM_SCROLL
        || obj->item_type == ITEM_PILL) {
        for (int i = 1; i <= 3; ++i) {
            int sn = obj->value[i];
            if (sn <= 0 || sn >= skill_count || skill_table[sn].name == NULL)
                continue;
            json_t* spec = json_object();
            JSON_SET_INT(spec, "slot", i);
            JSON_SET_STRING(spec, "name", skill_table[sn].name);
            json_array_append_new(spells, spec);
        }
    }
    else if (obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND) {
        int sn = obj->value[3];
        if (sn > 0 && sn < skill_count && skill_table[sn].name != NULL) {
            json_t* spec = json_object();
            JSON_SET_INT(spec, "slot", 3);
            JSON_SET_STRING(spec, "name", skill_table[sn].name);
            json_array_append_new(spells, spec);
        }
    }
    json_object_set_new(node, "spells", spells);

    json_t* contents = json_array();
    for (Node* node_it = obj->objects.back; node_it != NULL; node_it = node_it->prev) {
        Object* content = AS_OBJECT(node_it->value);
        json_t* entry = json_from_object(content);
        if (entry)
            json_array_append_new(contents, entry);
    }
    json_object_set_new(node, "contents", contents);
    return node;
}

static Object* object_from_json(json_t* node)
{
    if (!json_is_object(node))
        return NULL;

    VNUM vnum = (VNUM)json_int_or_default(node, "vnum", 0);
    Object* obj = NULL;
    ObjPrototype* proto = get_object_prototype(vnum);
    if (proto)
        obj = create_object(proto, -1);
    else
        obj = new_object();

    if (!obj)
        return NULL;

    const char* name = JSON_STRING(node, "name");
    if (name)
        SET_NAME(obj, lox_string(name));

    const char* short_descr = JSON_STRING(node, "shortDescr");
    if (short_descr) {
        free_string(obj->short_descr);
        obj->short_descr = str_dup(short_descr);
    }

    const char* description = JSON_STRING(node, "description");
    if (description) {
        free_string(obj->description);
        obj->description = str_dup(description);
    }

    const char* material = JSON_STRING(node, "material");
    if (material) {
        free_string(obj->material);
        obj->material = str_dup(material);
    }

    obj->enchanted = json_is_true(json_object_get(node, "enchanted"));
    obj->extra_flags = (FLAGS)json_int_or_default(node, "extraFlags", obj->extra_flags);
    obj->wear_flags = (FLAGS)json_int_or_default(node, "wearFlags", obj->wear_flags);
    obj->item_type = (ItemType)json_int_or_default(node, "itemType", obj->item_type);
    obj->weight = (int16_t)json_int_or_default(node, "weight", obj->weight);
    obj->condition = (int16_t)json_int_or_default(node, "condition", obj->condition);
    obj->wear_loc = (WearLocation)json_int_or_default(node, "wearLoc", obj->wear_loc);
    obj->level = (LEVEL)json_int_or_default(node, "level", obj->level);
    obj->timer = (int16_t)json_int_or_default(node, "timer", obj->timer);
    obj->cost = (int)json_int_or_default(node, "cost", obj->cost);

    json_t* values = json_object_get(node, "values");
    if (json_is_array(values)) {
        size_t count = json_array_size(values);
        for (size_t i = 0; i < count && i < 5; ++i)
            obj->value[i] = (int)json_integer_value(json_array_get(values, i));
    }

    json_t* affects = json_object_get(node, "affects");
    affects_from_json(affects, &obj->affected);

    json_t* extra = json_object_get(node, "extraDescriptions");
    if (json_is_array(extra)) {
        size_t count = json_array_size(extra);
        for (size_t i = 0; i < count; ++i) {
            json_t* entry = json_array_get(extra, i);
            if (!json_is_object(entry))
                continue;
            const char* keyword = JSON_STRING(entry, "keyword");
            const char* desc = JSON_STRING(entry, "description");
            if (!keyword || !desc)
                continue;
            ExtraDesc* ed = new_extra_desc();
            ed->keyword = str_dup(keyword);
            ed->description = str_dup(desc);
            ed->next = obj->extra_desc;
            obj->extra_desc = ed;
        }
    }

    json_t* spells = json_object_get(node, "spells");
    if (json_is_array(spells)) {
        size_t count = json_array_size(spells);
        for (size_t i = 0; i < count; ++i) {
            json_t* entry = json_array_get(spells, i);
            if (!json_is_object(entry))
                continue;
            int slot = (int)json_int_or_default(entry, "slot", -1);
            const char* spell_name = JSON_STRING(entry, "name");
            if (!spell_name || slot < 0 || slot > 4)
                continue;
            SKNUM sn = skill_lookup(spell_name);
            if (sn >= 0)
                obj->value[slot] = sn;
        }
    }

    json_t* contents = json_object_get(node, "contents");
    if (json_is_array(contents)) {
        size_t count = json_array_size(contents);
        for (size_t i = 0; i < count; ++i) {
            json_t* entry = json_array_get(contents, i);
            Object* child = object_from_json(entry);
            if (child)
                obj_to_obj(child, obj);
        }
    }

    return obj;
}

static void inventory_from_json(json_t* arr, Mobile* ch)
{
    if (!json_is_array(arr) || !ch)
        return;
    size_t count = json_array_size(arr);
    for (size_t i = 0; i < count; ++i) {
        json_t* entry = json_array_get(arr, i);
        Object* obj = object_from_json(entry);
        if (obj)
            obj_to_char(obj, ch);
    }
}

static json_t* json_from_inventory(const Mobile* ch)
{
    json_t* arr = json_array();
    if (!ch)
        return arr;
    for (Node* node = ch->objects.back; node != NULL; node = node->prev) {
        Object* obj = AS_OBJECT(node->value);
        json_t* entry = json_from_object(obj);
        if (entry)
            json_array_append_new(arr, entry);
    }
    return arr;
}

static json_t* json_from_pet(const Mobile* pet)
{
    if (!pet || !pet->prototype)
        return NULL;
    json_t* obj = json_object();
    JSON_SET_INT(obj, "vnum", VNUM_FIELD(pet->prototype));
    JSON_SET_STRING(obj, "name", NAME_STR(pet));
    JSON_SET_STRING(obj, "shortDescr", pet->short_descr);
    JSON_SET_STRING(obj, "longDescr", pet->long_descr);
    JSON_SET_STRING(obj, "description", pet->description);
    JSON_SET_STRING(obj, "race", race_table[pet->race].name);
    if (pet->clan && clan_table[pet->clan].name)
        JSON_SET_STRING(obj, "clan", clan_table[pet->clan].name);
    json_object_set_new(obj, "sex", json_integer(pet->sex));
    json_object_set_new(obj, "level", json_integer(pet->level));
    json_object_set_new(obj, "hmv", json_from_hmv(pet));
    json_object_set_new(obj, "gold", json_integer(pet->gold));
    json_object_set_new(obj, "silver", json_integer(pet->silver));
    json_object_set_new(obj, "copper", json_integer(pet->copper));
    json_object_set_new(obj, "exp", json_integer(pet->exp));
    json_object_set_new(obj, "actFlags", json_integer(pet->act_flags));
    json_object_set_new(obj, "affectFlags", json_integer(pet->affect_flags));
    json_object_set_new(obj, "commFlags", json_integer(pet->comm_flags));
    json_object_set_new(obj, "position", json_integer(pet->position));
    json_object_set_new(obj, "savingThrow", json_integer(pet->saving_throw));
    json_object_set_new(obj, "alignment", json_integer(pet->alignment));
    json_object_set_new(obj, "hitroll", json_integer(pet->hitroll));
    json_object_set_new(obj, "damroll", json_integer(pet->damroll));
    json_object_set_new(obj, "armor", json_from_armor(pet));
    json_object_set_new(obj, "permStats", json_from_perm_stats(pet));
    json_object_set_new(obj, "modStats", json_from_mod_stats(pet));
    json_t* affects = json_array();
    for (Affect* affect = pet->affected; affect != NULL; affect = affect->next) {
        json_t* entry = json_from_affect(affect);
        if (entry)
            json_array_append_new(affects, entry);
    }
    json_object_set_new(obj, "affects", affects);
    return obj;
}

static void pet_from_json(json_t* obj, Mobile* ch)
{
    if (!json_is_object(obj) || !ch)
        return;

    VNUM vnum = (VNUM)json_int_or_default(obj, "vnum", 0);
    MobPrototype* proto = get_mob_prototype(vnum);
    Mobile* pet = NULL;
    if (proto)
        pet = create_mobile(proto);
    else
        pet = create_mobile(get_mob_prototype(MOB_VNUM_FIDO));

    if (!pet)
        return;

    const char* name = JSON_STRING(obj, "name");
    if (name)
        SET_NAME(pet, lox_string(name));

    const char* short_descr = JSON_STRING(obj, "shortDescr");
    if (short_descr) {
        free_string(pet->short_descr);
        pet->short_descr = str_dup(short_descr);
    }

    const char* long_descr = JSON_STRING(obj, "longDescr");
    if (long_descr) {
        free_string(pet->long_descr);
        pet->long_descr = str_dup(long_descr);
    }

    const char* description = JSON_STRING(obj, "description");
    if (description) {
        free_string(pet->description);
        pet->description = str_dup(description);
    }

    const char* race_name = JSON_STRING(obj, "race");
    if (race_name)
        pet->race = race_lookup(race_name);

    const char* clan_name = JSON_STRING(obj, "clan");
    if (clan_name)
        pet->clan = (int16_t)clan_lookup(clan_name);

    pet->sex = (Sex)json_int_or_default(obj, "sex", pet->sex);
    pet->level = (LEVEL)json_int_or_default(obj, "level", pet->level);
    json_t* hmv = json_object_get(obj, "hmv");
    if (json_is_object(hmv)) {
        pet->hit = (int16_t)json_int_or_default(hmv, "hit", pet->hit);
        pet->max_hit = (int16_t)json_int_or_default(hmv, "maxHit", pet->max_hit);
        pet->mana = (int16_t)json_int_or_default(hmv, "mana", pet->mana);
        pet->max_mana = (int16_t)json_int_or_default(hmv, "maxMana", pet->max_mana);
        pet->move = (int16_t)json_int_or_default(hmv, "move", pet->move);
        pet->max_move = (int16_t)json_int_or_default(hmv, "maxMove", pet->max_move);
    }
    pet->gold = (int16_t)json_int_or_default(obj, "gold", pet->gold);
    pet->silver = (int16_t)json_int_or_default(obj, "silver", pet->silver);
    pet->copper = (int16_t)json_int_or_default(obj, "copper", pet->copper);
    pet->exp = (int)json_int_or_default(obj, "exp", pet->exp);
    pet->act_flags = (FLAGS)json_int_or_default(obj, "actFlags", pet->act_flags);
    pet->affect_flags = (FLAGS)json_int_or_default(obj, "affectFlags", pet->affect_flags);
    pet->comm_flags = (FLAGS)json_int_or_default(obj, "commFlags", pet->comm_flags);
    pet->position = (Position)json_int_or_default(obj, "position", pet->position);
    pet->saving_throw = (int16_t)json_int_or_default(obj, "savingThrow", pet->saving_throw);
    pet->alignment = (int16_t)json_int_or_default(obj, "alignment", pet->alignment);
    pet->hitroll = (int16_t)json_int_or_default(obj, "hitroll", pet->hitroll);
    pet->damroll = (int16_t)json_int_or_default(obj, "damroll", pet->damroll);

    json_t* armor = json_object_get(obj, "armor");
    if (json_is_array(armor)) {
        size_t count = json_array_size(armor);
        for (size_t i = 0; i < count && i < AC_COUNT; ++i)
            pet->armor[i] = (int16_t)json_integer_value(json_array_get(armor, i));
    }

    json_t* perm = json_object_get(obj, "permStats");
    if (json_is_array(perm)) {
        size_t count = json_array_size(perm);
        for (size_t i = 0; i < count && i < STAT_COUNT; ++i)
            pet->perm_stat[i] = (int16_t)json_integer_value(json_array_get(perm, i));
    }

    json_t* mod = json_object_get(obj, "modStats");
    if (json_is_array(mod)) {
        size_t count = json_array_size(mod);
        for (size_t i = 0; i < count && i < STAT_COUNT; ++i)
            pet->mod_stat[i] = (int16_t)json_integer_value(json_array_get(mod, i));
    }

    affects_from_json(json_object_get(obj, "affects"), &pet->affected);

    pet->leader = ch;
    pet->master = ch;
    ch->pet = pet;
}

static int resolve_room_vnum(const Mobile* ch)
{
    if (ch->in_room == get_room(NULL, ROOM_VNUM_LIMBO) && ch->was_in_room != NULL)
        return (int)VNUM_FIELD(ch->was_in_room);
    if (ch->in_room == NULL)
        return ch->pcdata ? ch->pcdata->recall : cfg_get_default_recall();
    return (int)VNUM_FIELD(ch->in_room);
}

static json_t* json_from_pcdata(const Mobile* ch)
{
    PlayerData* pc = ch->pcdata;
    json_t* obj = json_object();
    if (!pc)
        return obj;

    JSON_SET_INT(obj, "security", pc->security);
    JSON_SET_INT(obj, "recall", pc->recall);
    JSON_SET_INT(obj, "points", pc->points);
    JSON_SET_INT(obj, "trueSex", pc->true_sex);
    JSON_SET_INT(obj, "lastLevel", pc->last_level);
    JSON_SET_INT(obj, "permHit", pc->perm_hit);
    JSON_SET_INT(obj, "permMana", pc->perm_mana);
    JSON_SET_INT(obj, "permMove", pc->perm_move);
    json_t* cond = json_array();
    for (int i = 0; i < COND_MAX; ++i)
        json_array_append_new(cond, json_integer(pc->condition[i]));
    json_object_set_new(obj, "conditions", cond);
    JSON_SET_INT(obj, "lastNote", pc->last_note);
    JSON_SET_INT(obj, "lastIdea", pc->last_idea);
    JSON_SET_INT(obj, "lastPenalty", pc->last_penalty);
    JSON_SET_INT(obj, "lastNews", pc->last_news);
    JSON_SET_INT(obj, "lastChanges", pc->last_changes);
    if (pc->theme_config.current_theme_name)
        JSON_SET_STRING(obj, "currentTheme", pc->theme_config.current_theme_name);
    if (pc->tutorial && pc->tutorial->name) {
        json_t* tut = json_object();
        JSON_SET_STRING(tut, "name", pc->tutorial->name);
        JSON_SET_INT(tut, "step", pc->tutorial_step);
        json_object_set_new(obj, "tutorial", tut);
    }

    if (pc->pwd_digest_hex && pc->pwd_digest_hex[0] != '\0')
        JSON_SET_STRING(obj, "pwdDigest", pc->pwd_digest_hex);
    else if (pc->pwd_digest && pc->pwd_digest_len > 0) {
        char hex[256];
        bin_to_hex(hex, pc->pwd_digest, pc->pwd_digest_len);
        JSON_SET_STRING(obj, "pwdDigest", hex);
    }

    JSON_SET_STRING(obj, "bamfin", pc->bamfin);
    JSON_SET_STRING(obj, "bamfout", pc->bamfout);
    JSON_SET_STRING(obj, "title", pc->title);

    json_object_set_new(obj, "themeConfig", json_from_theme_config(pc));
    json_add_aliases(obj, pc);
    json_object_set_new(obj, "skills", json_from_skills(pc));
    json_object_set_new(obj, "groups", json_from_groups(pc));
    json_object_set_new(obj, "reputations", json_from_reputations(pc));
    json_object_set_new(obj, "themeConfig", json_from_theme_config(pc));
    json_object_set_new(obj, "personalThemes", json_from_personal_themes(pc));
    return obj;
}

static void pcdata_from_json(json_t* obj, Mobile* ch)
{
    if (!json_is_object(obj) || !ch || !ch->pcdata)
        return;

    PlayerData* pc = ch->pcdata;
    pc->security = (int)json_int_or_default(obj, "security", pc->security);
    pc->recall = (int)json_int_or_default(obj, "recall", pc->recall);
    pc->points = (int16_t)json_int_or_default(obj, "points", pc->points);
    pc->true_sex = (Sex)json_int_or_default(obj, "trueSex", pc->true_sex);
    pc->last_level = (LEVEL)json_int_or_default(obj, "lastLevel", pc->last_level);
    pc->perm_hit = (int16_t)json_int_or_default(obj, "permHit", pc->perm_hit);
    pc->perm_mana = (int16_t)json_int_or_default(obj, "permMana", pc->perm_mana);
    pc->perm_move = (int16_t)json_int_or_default(obj, "permMove", pc->perm_move);
    json_t* cond = json_object_get(obj, "conditions");
    if (json_is_array(cond)) {
        size_t count = json_array_size(cond);
        for (size_t i = 0; i < count && i < COND_MAX; ++i)
            pc->condition[i] = (int16_t)json_integer_value(json_array_get(cond, i));
    }
    pc->last_note = (time_t)json_int_or_default(obj, "lastNote", pc->last_note);
    pc->last_idea = (time_t)json_int_or_default(obj, "lastIdea", pc->last_idea);
    pc->last_penalty = (time_t)json_int_or_default(obj, "lastPenalty", pc->last_penalty);
    pc->last_news = (time_t)json_int_or_default(obj, "lastNews", pc->last_news);
    pc->last_changes = (time_t)json_int_or_default(obj, "lastChanges", pc->last_changes);

    const char* curr_theme = JSON_STRING(obj, "currentTheme");
    if (curr_theme) {
        free_string(pc->theme_config.current_theme_name);
        pc->theme_config.current_theme_name = str_dup(curr_theme);
    }

    json_t* tutorial = json_object_get(obj, "tutorial");
    if (json_is_object(tutorial)) {
        const char* tut_name = JSON_STRING(tutorial, "name");
        if (tut_name) {
            Tutorial* t = get_tutorial(tut_name);
            if (t) {
                pc->tutorial = t;
                pc->tutorial_step = (int)json_int_or_default(tutorial, "step", pc->tutorial_step);
            }
        }
    }

    const char* digest = JSON_STRING(obj, "pwdDigest");
    if (digest)
        decode_digest(pc, digest);

    const char* bamfin = JSON_STRING(obj, "bamfin");
    if (bamfin) {
        free_string(pc->bamfin);
        pc->bamfin = str_dup(bamfin);
    }

    const char* bamfout = JSON_STRING(obj, "bamfout");
    if (bamfout) {
        free_string(pc->bamfout);
        pc->bamfout = str_dup(bamfout);
    }

    const char* title = JSON_STRING(obj, "title");
    if (title) {
        free_string(pc->title);
        pc->title = str_dup(title);
    }

    theme_config_from_json(json_object_get(obj, "themeConfig"), pc);

    json_t* aliases = json_object_get(obj, "aliases");
    aliases_from_json(aliases, pc);

    json_t* skills = json_object_get(obj, "skills");
    skills_from_json(skills, pc);

    json_t* groups = json_object_get(obj, "groups");
    groups_from_json(groups, ch);

    json_t* reps = json_object_get(obj, "reputations");
    reputations_from_json(reps, pc);

    json_t* themes = json_object_get(obj, "personalThemes");
    personal_themes_from_json(themes, pc);
}

static json_t* json_from_player(const Mobile* ch)
{
    json_t* player = json_object();
    JSON_SET_STRING(player, "name", NAME_STR(ch));
    JSON_SET_INT(player, "id", ch->id);
    JSON_SET_INT(player, "logoffTime", current_time);
    JSON_SET_INT(player, "version", ch->version > 0 ? ch->version : 5);
    JSON_SET_STRING(player, "shortDescr", ch->short_descr);
    JSON_SET_STRING(player, "longDescr", ch->long_descr);
    JSON_SET_STRING(player, "description", ch->description);
    if (ch->prompt
        && str_cmp(ch->prompt, "<%hhp %mm %vmv> ")
        && str_cmp(ch->prompt, "^p<%hhp %mm %vmv>" COLOR_CLEAR " "))
        JSON_SET_STRING(player, "prompt", ch->prompt);

    JSON_SET_STRING(player, "race", race_table[ch->race].name);
    if (ch->clan && clan_table[ch->clan].name)
        JSON_SET_STRING(player, "clan", clan_table[ch->clan].name);
    json_object_set_new(player, "sex", json_integer(ch->sex));
    json_object_set_new(player, "class", json_integer(ch->ch_class));
    json_object_set_new(player, "level", json_integer(ch->level));
    if (ch->trust != 0)
        json_object_set_new(player, "trust", json_integer(ch->trust));
    json_object_set_new(player, "security", json_integer(ch->pcdata->security));
    json_object_set_new(player, "played", json_integer((int)(ch->played + (current_time - ch->logon))));
    json_object_set_new(player, "screenLines", json_integer(ch->lines));
    json_object_set_new(player, "recall", json_integer(ch->pcdata->recall));
    json_object_set_new(player, "room", json_integer(resolve_room_vnum(ch)));
    json_object_set_new(player, "hmv", json_from_hmv(ch));
    json_object_set_new(player, "money", json_from_money(ch));
    json_object_set_new(player, "exp", json_integer(ch->exp));
    json_object_set_new(player, "actFlags", json_integer(ch->act_flags));
    json_object_set_new(player, "affectFlags", json_integer(ch->affect_flags));
    json_object_set_new(player, "commFlags", json_integer(ch->comm_flags));
    json_object_set_new(player, "wiznet", json_integer(ch->wiznet));
    json_object_set_new(player, "invisLevel", json_integer(ch->invis_level));
    json_object_set_new(player, "incogLevel", json_integer(ch->incog_level));
    json_object_set_new(player, "position", json_integer(ch->position == POS_FIGHTING ? POS_STANDING : ch->position));
    json_object_set_new(player, "practice", json_integer(ch->practice));
    json_object_set_new(player, "train", json_integer(ch->train));
    json_object_set_new(player, "savingThrow", json_integer(ch->saving_throw));
    json_object_set_new(player, "alignment", json_integer(ch->alignment));
    json_object_set_new(player, "hitroll", json_integer(ch->hitroll));
    json_object_set_new(player, "damroll", json_integer(ch->damroll));
    json_object_set_new(player, "armor", json_from_armor(ch));
    json_object_set_new(player, "wimpy", json_integer(ch->wimpy));
    json_object_set_new(player, "permStats", json_from_perm_stats(ch));
    json_object_set_new(player, "modStats", json_from_mod_stats(ch));

    json_t* affects = json_array();
    for (Affect* affect = ch->affected; affect != NULL; affect = affect->next) {
        json_t* entry = json_from_affect(affect);
        if (entry)
            json_array_append_new(affects, entry);
    }
    json_object_set_new(player, "affects", affects);

    json_object_set_new(player, "pcdata", json_from_pcdata(ch));
    return player;
}

static void player_from_json(json_t* player_obj, Mobile* ch)
{
    if (!json_is_object(player_obj) || !ch)
        return;

    const char* name = JSON_STRING(player_obj, "name");
    if (name)
        SET_NAME(ch, lox_string(name));
    ch->id = (int)json_int_or_default(player_obj, "id", ch->id);
    ch->version = (int)json_int_or_default(player_obj, "version", ch->version);

    const char* short_descr = JSON_STRING(player_obj, "shortDescr");
    if (short_descr) {
        free_string(ch->short_descr);
        ch->short_descr = str_dup(short_descr);
    }

    const char* long_descr = JSON_STRING(player_obj, "longDescr");
    if (long_descr) {
        free_string(ch->long_descr);
        ch->long_descr = str_dup(long_descr);
    }

    const char* description = JSON_STRING(player_obj, "description");
    if (description) {
        free_string(ch->description);
        ch->description = str_dup(description);
    }

    const char* prompt = JSON_STRING(player_obj, "prompt");
    if (prompt) {
        free_string(ch->prompt);
        ch->prompt = str_dup(prompt);
    }

    const char* race_name = JSON_STRING(player_obj, "race");
    if (race_name)
        ch->race = race_lookup(race_name);

    const char* clan_name = JSON_STRING(player_obj, "clan");
    if (clan_name)
        ch->clan = (int16_t)clan_lookup(clan_name);

    ch->sex = (Sex)json_int_or_default(player_obj, "sex", ch->sex);
    ch->ch_class = (int16_t)json_int_or_default(player_obj, "class", ch->ch_class);
    ch->level = (LEVEL)json_int_or_default(player_obj, "level", ch->level);
    ch->trust = (int16_t)json_int_or_default(player_obj, "trust", ch->trust);
    ch->pcdata->security = (int)json_int_or_default(player_obj, "security", ch->pcdata->security);
    ch->played = json_int_or_default(player_obj, "played", ch->played);
    ch->lines = (int)json_int_or_default(player_obj, "screenLines", ch->lines);
    ch->pcdata->recall = (int)json_int_or_default(player_obj, "recall", ch->pcdata->recall);

    json_t* hmv = json_object_get(player_obj, "hmv");
    if (json_is_object(hmv)) {
        ch->hit = (int16_t)json_int_or_default(hmv, "hit", ch->hit);
        ch->max_hit = (int16_t)json_int_or_default(hmv, "maxHit", ch->max_hit);
        ch->mana = (int16_t)json_int_or_default(hmv, "mana", ch->mana);
        ch->max_mana = (int16_t)json_int_or_default(hmv, "maxMana", ch->max_mana);
        ch->move = (int16_t)json_int_or_default(hmv, "move", ch->move);
        ch->max_move = (int16_t)json_int_or_default(hmv, "maxMove", ch->max_move);
    }

    json_t* money = json_object_get(player_obj, "money");
    if (json_is_object(money)) {
        ch->gold = (int16_t)json_int_or_default(money, "gold", ch->gold);
        ch->silver = (int16_t)json_int_or_default(money, "silver", ch->silver);
        ch->copper = (int16_t)json_int_or_default(money, "copper", ch->copper);
    }

    ch->exp = (int)json_int_or_default(player_obj, "exp", ch->exp);
    ch->act_flags = (FLAGS)json_int_or_default(player_obj, "actFlags", ch->act_flags);
    ch->affect_flags = (FLAGS)json_int_or_default(player_obj, "affectFlags", ch->affect_flags);
    ch->comm_flags = (FLAGS)json_int_or_default(player_obj, "commFlags", ch->comm_flags);
    ch->wiznet = (FLAGS)json_int_or_default(player_obj, "wiznet", ch->wiznet);
    ch->invis_level = (int16_t)json_int_or_default(player_obj, "invisLevel", ch->invis_level);
    ch->incog_level = (int16_t)json_int_or_default(player_obj, "incogLevel", ch->incog_level);
    ch->position = (Position)json_int_or_default(player_obj, "position", ch->position);
    ch->practice = (int16_t)json_int_or_default(player_obj, "practice", ch->practice);
    ch->train = (int16_t)json_int_or_default(player_obj, "train", ch->train);
    ch->saving_throw = (int16_t)json_int_or_default(player_obj, "savingThrow", ch->saving_throw);
    ch->alignment = (int16_t)json_int_or_default(player_obj, "alignment", ch->alignment);
    ch->hitroll = (int16_t)json_int_or_default(player_obj, "hitroll", ch->hitroll);
    ch->damroll = (int16_t)json_int_or_default(player_obj, "damroll", ch->damroll);

    json_t* armor = json_object_get(player_obj, "armor");
    if (json_is_array(armor)) {
        size_t count = json_array_size(armor);
        for (size_t i = 0; i < count && i < AC_COUNT; ++i)
            ch->armor[i] = (int16_t)json_integer_value(json_array_get(armor, i));
    }

    ch->wimpy = (int16_t)json_int_or_default(player_obj, "wimpy", ch->wimpy);

    json_t* perm_stats = json_object_get(player_obj, "permStats");
    if (json_is_array(perm_stats)) {
        size_t count = json_array_size(perm_stats);
        for (size_t i = 0; i < count && i < STAT_COUNT; ++i)
            ch->perm_stat[i] = (int16_t)json_integer_value(json_array_get(perm_stats, i));
    }

    json_t* mod_stats = json_object_get(player_obj, "modStats");
    if (json_is_array(mod_stats)) {
        size_t count = json_array_size(mod_stats);
        for (size_t i = 0; i < count && i < STAT_COUNT; ++i)
            ch->mod_stat[i] = (int16_t)json_integer_value(json_array_get(mod_stats, i));
    }

    affects_from_json(json_object_get(player_obj, "affects"), &ch->affected);

    json_t* pcdata = json_object_get(player_obj, "pcdata");
    pcdata_from_json(pcdata, ch);

    int room_vnum = (int)json_int_or_default(player_obj, "room", ch->pcdata->recall);
    RoomData* room_data = get_room_data(room_vnum);
    if (room_data) {
        Area* area = get_area_for_player(ch, room_data->area_data);
        if (area || room_data->area_data->low_range == 1)
            ch->in_room = get_room_for_player(ch, VNUM_FIELD(room_data));
    }
}

PersistResult player_persist_json_save(const PlayerPersistSaveParams* params)
{
    if (!params || !params->writer || !params->ch)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "player JSON save: missing writer", -1 };

    json_t* root = json_object();
    JSON_SET_INT(root, "formatVersion", PLAYER_JSON_FORMAT_VERSION);
    json_object_set_new(root, "player", json_from_player(params->ch));
    json_object_set_new(root, "inventory", json_from_inventory(params->ch));
    if (params->ch->pet && params->ch->pet->in_room == params->ch->in_room) {
        json_t* pet = json_from_pet(params->ch->pet);
        if (pet)
            json_object_set_new(root, "pet", pet);
    }
    json_object_set_new(root, "questLog", json_from_quest_log(params->ch->pcdata));

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "player JSON save: failed to dump", -1 };

    size_t len = strlen(dump);
    bool ok = writer_write_all(params->writer, dump, len);
    free(dump);
    if (!ok)
        return (PersistResult){ PERSIST_ERR_IO, "player JSON save: write failed", -1 };
    if (params->writer->ops->flush)
        params->writer->ops->flush(params->writer->ctx);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

PersistResult player_persist_json_load(const PlayerPersistLoadParams* params)
{
    if (!params || !params->reader || !params->ch)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "player JSON load: missing reader", -1 };

    ReaderBuffer buf = { 0 };
    if (!reader_fill_buffer(params->reader, &buf))
        return (PersistResult){ PERSIST_ERR_IO, "player JSON load: read failed", -1 };

    json_error_t error;
    json_t* root = json_loadb(buf.data, buf.len, 0, &error);
    free(buf.data);
    if (!root) {
        static char msg[256];
        snprintf(msg, sizeof msg, "player JSON parse error at line %d: %s", error.line, error.text);
        return (PersistResult){ PERSIST_ERR_FORMAT, msg, (int)error.line };
    }

    json_t* version = json_object_get(root, "formatVersion");
    if (!json_is_integer(version) || json_integer_value(version) != PLAYER_JSON_FORMAT_VERSION) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "player JSON: unsupported formatVersion", -1 };
    }

    json_t* player_obj = json_object_get(root, "player");
    if (!json_is_object(player_obj)) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_FORMAT, "player JSON: missing player object", -1 };
    }

    player_from_json(player_obj, params->ch);

    json_t* inventory = json_object_get(root, "inventory");
    inventory_from_json(inventory, params->ch);

    json_t* pet_obj = json_object_get(root, "pet");
    if (pet_obj)
        pet_from_json(pet_obj, params->ch);

    json_t* quest_log = json_object_get(root, "questLog");
    quest_log_from_json(quest_log, params->ch);

    json_decref(root);
    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
