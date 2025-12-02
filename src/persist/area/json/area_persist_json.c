////////////////////////////////////////////////////////////////////////////////
// persist/json/area_persist_json.c
// Stub JSON backend using jansson (gated by ENABLE_JSON_AREAS).
////////////////////////////////////////////////////////////////////////////////

#include "area_persist_json.h"

#include <persist/json/persist_json.h>

#include <data/damage.h>
#include <data/direction.h>
#include <data/events.h>
#include <data/item.h>
#include <data/mobile_data.h>
#include <data/quest.h>
#include <data/race.h>
#include <data/skill.h>

#include <entities/area.h>
#include <entities/mob_prototype.h>
#include <entities/obj_prototype.h>
#include <entities/room.h>
#include <entities/room_exit.h>
#include <entities/extra_desc.h>
#include <entities/reset.h>
#include <entities/event.h>
#include <entities/faction.h>
#include <entities/help_data.h>
#include <entities/shop_data.h>

#include <olc/bit.h>

#include <lookup.h>
#include <tables.h>
#include <handler.h>
#include <mob_prog.h>
#include <db.h>

#include <jansson.h>

#include <stdlib.h>
#include <string.h>

#define AREA_JSON_FORMAT_VERSION 1

const AreaPersistFormat AREA_PERSIST_JSON = {
    .name = "json",
    .load = json_load,
    .save = json_save,
};

static void json_set_flags_if(json_t* obj, const char* key, FLAGS flags, const struct flag_type* table)
{
    json_t* arr = flags_to_array(flags, table);
    if (arr && json_array_size(arr) > 0)
        json_object_set_new(obj, key, arr);
    else
        json_decref(arr);
}

static void json_set_int_if(json_t* obj, const char* key, int64_t value, int64_t def)
{
    if (value != def)
        json_object_set_new(obj, key, json_integer(value));
}

static const char* dir_name_from_enum(int dir)
{
    if (dir < 0 || dir >= DIR_MAX)
        return NULL;
    return dir_list[dir].name;
}

static int dir_enum_from_name(const char* name)
{
    if (!name)
        return -1;
    return (int)get_direction(name);
}

static const char* checklist_status_name(ChecklistStatus status)
{
    switch (status) {
    case CHECK_TODO: return "todo";
    case CHECK_IN_PROGRESS: return "inProgress";
    case CHECK_DONE: return "done";
    default: return NULL;
    }
}

static ChecklistStatus checklist_status_from_name(const char* name, ChecklistStatus def)
{
    if (!name)
        return def;
    if (!str_cmp(name, "todo"))
        return CHECK_TODO;
    if (!str_cmp(name, "inProgress") || !str_cmp(name, "in_progress"))
        return CHECK_IN_PROGRESS;
    if (!str_cmp(name, "done"))
        return CHECK_DONE;
    return def;
}

static void ensure_entity_class(Entity* ent, const char* prefix)
{
    if (!ent || !ent->script || ent->klass)
        return;
    char class_name[MIL];
    snprintf(class_name, sizeof(class_name), "%s_%" PRVNUM, prefix, ent->vnum);
    ObjClass* klass = create_entity_class(ent, class_name, ent->script->chars);
    if (klass)
        ent->klass = klass;
}

static const EventTypeInfo* trigger_info_from_name(const char* name)
{
    if (!name)
        return NULL;
    for (int i = 0; i < TRIG_COUNT; i++) {
        const EventTypeInfo* info = &event_type_info_table[i];
        if (info->name && str_cmp(info->name, name) == 0)
            return info;
    }
    return NULL;
}

static json_t* build_events(const Entity* ent)
{
    json_t* arr = json_array();
    if (!ent || ent->events.count == 0)
        return arr;

    for (Node* node = ent->events.front; node != NULL; node = node->next) {
        Event* ev = AS_EVENT(node->value);
        if (!ev)
            continue;
        json_t* obj = json_object();
        const char* trig_name = get_event_name((EventTrigger)ev->trigger);
        if (trig_name && trig_name[0] != '\0')
            json_object_set_new(obj, "trigger", json_string(trig_name));
        else
            json_object_set_new(obj, "triggerValue", json_integer(ev->trigger));
        if (ev->method_name && ev->method_name->chars)
            json_object_set_new(obj, "callback", json_string(ev->method_name->chars));
        if (IS_INT(ev->criteria))
            json_object_set_new(obj, "criteria", json_integer(AS_INT(ev->criteria)));
        else if (IS_STRING(ev->criteria))
            json_object_set_new(obj, "criteria", json_string(AS_STRING(ev->criteria)->chars));
        if (json_object_size(obj) > 0)
            json_array_append_new(arr, obj);
        else
            json_decref(obj);
    }
    return arr;
}

static void parse_events(json_t* arr, Entity* ent, EventEnts ent_type)
{
    if (!json_is_array(arr) || !ent)
        return;

    size_t count = json_array_size(arr);
    for (size_t i = 0; i < count; i++) {
        json_t* e = json_array_get(arr, i);
        if (!json_is_object(e))
            continue;

        FLAGS trig = 0;
        const EventTypeInfo* info = NULL;
        const char* trig_name = json_string_value(json_object_get(e, "trigger"));
        if (trig_name) {
            info = trigger_info_from_name(trig_name);
            if (info)
                trig = info->trigger;
        }
        if (trig == 0 && json_is_integer(json_object_get(e, "triggerValue")))
            trig = (FLAGS)json_integer_value(json_object_get(e, "triggerValue"));
        if (trig == 0)
            continue;

        if (info == NULL)
            info = get_event_type_info((EventTrigger)trig);
        if (info && !(info->valid_ents & ent_type))
            continue; // not valid for this entity type

        const char* cb = json_string_value(json_object_get(e, "callback"));
        if (!cb && info)
            cb = info->default_callback;

        Value criteria = NIL_VAL;
        json_t* crit = json_object_get(e, "criteria");
        if (json_is_integer(crit))
            criteria = INT_VAL((int)json_integer_value(crit));
        else if (json_is_string(crit))
            criteria = OBJ_VAL(lox_string(json_string_value(crit)));

        Event* ev = new_event();
        ev->trigger = (EventTrigger)trig;
        if (cb)
            ev->method_name = lox_string(cb);
        ev->criteria = criteria;
        add_event(ent, ev);
    }
}

static json_t* build_areadata(const AreaData* area)
{
    json_t* obj = json_object();
    json_object_set_new(obj, "version", json_integer(AREA_VERSION));
    json_object_set_new(obj, "name", json_string(NAME_STR(area)));
    json_object_set_new(obj, "builders", json_string(area->builders ? area->builders : ""));
    json_t* vnums = json_array();
    json_array_append_new(vnums, json_integer(area->min_vnum));
    json_array_append_new(vnums, json_integer(area->max_vnum));
    json_object_set_new(obj, "vnumRange", vnums);
    json_object_set_new(obj, "credits", json_string(area->credits ? area->credits : ""));
    json_object_set_new(obj, "security", json_integer(area->security));
    const char* sector_name = flag_string(sector_flag_table, area->sector);
    if (sector_name && sector_name[0] != '\0')
        json_object_set_new(obj, "sector", json_string(sector_name));
    json_object_set_new(obj, "lowLevel", json_integer(area->low_range));
    json_object_set_new(obj, "highLevel", json_integer(area->high_range));
    json_object_set_new(obj, "reset", json_integer(area->reset_thresh));
    json_object_set_new(obj, "alwaysReset", json_boolean(area->always_reset));
    if (area->inst_type == AREA_INST_MULTI)
        json_object_set_new(obj, "instType", json_string("multi"));
    return obj;
}

static json_t* build_story_beats(const AreaData* area)
{
    json_t* arr = json_array();
    for (StoryBeat* beat = area->story_beats; beat != NULL; beat = beat->next) {
        json_t* obj = json_object();
        if (beat->title)
            json_object_set_new(obj, "title", json_string(beat->title));
        if (beat->description)
            json_object_set_new(obj, "description", json_string(beat->description));
        json_array_append_new(arr, obj);
    }
    return arr;
}

static json_t* build_checklist(const AreaData* area)
{
    json_t* arr = json_array();
    for (ChecklistItem* item = area->checklist; item != NULL; item = item->next) {
        json_t* obj = json_object();
        if (item->title)
            json_object_set_new(obj, "title", json_string(item->title));
        if (item->description && item->description[0] != '\0')
            json_object_set_new(obj, "description", json_string(item->description));
        const char* status = checklist_status_name(item->status);
        if (status)
            json_object_set_new(obj, "status", json_string(status));
        else
            json_object_set_new(obj, "statusValue", json_integer(item->status));
        json_array_append_new(arr, obj);
    }
    return arr;
}

static json_t* build_exit(const RoomExitData* ex)
{
    json_t* obj = json_object();
    const char* dir = dir_name_from_enum(ex->orig_dir);
    if (dir)
        json_object_set_new(obj, "dir", json_string(dir));
    json_object_set_new(obj, "toVnum", json_integer(ex->to_vnum));
    if (ex->key != 0)
        json_object_set_new(obj, "key", json_integer(ex->key));
    json_set_flags_if(obj, "flags", ex->exit_reset_flags, exit_flag_table);
    if (ex->description && ex->description[0] != '\0')
        json_object_set_new(obj, "description", json_string(ex->description));
    if (ex->keyword && ex->keyword[0] != '\0')
        json_object_set_new(obj, "keyword", json_string(ex->keyword));
    return obj;
}

static json_t* build_extra_descs(ExtraDesc* ed_list)
{
    json_t* arr = json_array();
    for (ExtraDesc* ed = ed_list; ed != NULL; ed = ed->next) {
        json_t* obj = json_object();
        json_object_set_new(obj, "keyword", json_string(ed->keyword));
        json_object_set_new(obj, "description", json_string(ed->description));
        json_array_append_new(arr, obj);
    }
    return arr;
}

static json_t* build_rooms(const AreaData* area)
{
    json_t* arr = json_array();
    RoomData* room_data;
    FOR_EACH_GLOBAL_ROOM(room_data) {
        if (room_data->area_data != area)
            continue;
        json_t* obj = json_object();
        json_object_set_new(obj, "vnum", json_integer(VNUM_FIELD(room_data)));
        json_object_set_new(obj, "name", json_string(NAME_STR(room_data)));
        json_object_set_new(obj, "description", json_string(room_data->description ? room_data->description : ""));
        json_set_flags_if(obj, "roomFlags", room_data->room_flags, room_flag_table);
        const char* sector_name = flag_string(sector_flag_table, room_data->sector_type);
        if (sector_name && sector_name[0] != '\0')
            json_object_set_new(obj, "sectorType", json_string(sector_name));
        if (room_data->mana_rate != 100)
            json_object_set_new(obj, "manaRate", json_integer(room_data->mana_rate));
        if (room_data->heal_rate != 100)
            json_object_set_new(obj, "healRate", json_integer(room_data->heal_rate));
        if (room_data->clan > 0)
            json_object_set_new(obj, "clan", json_integer(room_data->clan));
        if (room_data->owner && room_data->owner[0] != '\0')
            json_object_set_new(obj, "owner", json_string(room_data->owner));

        json_t* exits = json_array();
        for (int i = 0; i < DIR_MAX; i++) {
            if (room_data->exit_data[i])
                json_array_append_new(exits, build_exit(room_data->exit_data[i]));
        }
        if (json_array_size(exits) > 0)
            json_object_set_new(obj, "exits", exits);
        else
            json_decref(exits);

        if (room_data->extra_desc)
            json_object_set_new(obj, "extraDescs", build_extra_descs(room_data->extra_desc));

        Entity* ent = (Entity*)room_data;
        if (ent->script && ent->script->chars && ent->script->length > 0)
            json_object_set_new(obj, "loxScript", json_string(ent->script->chars));
        json_t* ev = build_events(ent);
        if (json_array_size(ev) > 0)
            json_object_set_new(obj, "events", ev);
        else
            json_decref(ev);

        json_array_append_new(arr, obj);
    }
    return arr;
}

bool json_bool_or_default(json_t* obj, const char* key, bool def)
{
    json_t* val = json_object_get(obj, key);
    if (json_is_boolean(val))
        return json_is_true(val);
    return def;
}

static void parse_story_beats(json_t* arr, AreaData* area)
{
    if (!json_is_array(arr) || !area)
        return;
    size_t count = json_array_size(arr);
    for (size_t i = 0; i < count; i++) {
        json_t* sb = json_array_get(arr, i);
        if (!json_is_object(sb))
            continue;
        const char* title = json_string_value(json_object_get(sb, "title"));
        const char* desc = json_string_value(json_object_get(sb, "description"));
        add_story_beat(area, title ? title : "", desc ? desc : "");
    }
}

static void parse_checklist(json_t* arr, AreaData* area)
{
    if (!json_is_array(arr) || !area)
        return;
    size_t count = json_array_size(arr);
    for (size_t i = 0; i < count; i++) {
        json_t* it = json_array_get(arr, i);
        if (!json_is_object(it))
            continue;
        const char* title = json_string_value(json_object_get(it, "title"));
        const char* desc = json_string_value(json_object_get(it, "description"));
        const char* status_name = json_string_value(json_object_get(it, "status"));
        ChecklistStatus status = checklist_status_from_name(status_name, CHECK_TODO);
        if (status_name == NULL && json_is_integer(json_object_get(it, "statusValue")))
            status = (ChecklistStatus)json_integer_value(json_object_get(it, "statusValue"));
        if (status < CHECK_TODO || status > CHECK_DONE)
            status = CHECK_TODO;
        add_checklist_item(area, title ? title : "", desc ? desc : "", status);
    }
}

static PersistResult parse_areadata(json_t* root, const AreaPersistLoadParams* params)
{
    json_t* areadata = json_object_get(root, "areadata");
    if (!json_is_object(areadata))
        return (PersistResult){ PERSIST_ERR_FORMAT, "JSON area load: missing areadata", -1 };

    AreaData* area = new_area_data();
    area->file_name = str_dup(params->file_name ? params->file_name : "area.json");

    const char* name = json_string_value(json_object_get(areadata, "name"));
    if (name)
        SET_NAME(area, lox_string(name));

    const char* builders = json_string_value(json_object_get(areadata, "builders"));
    if (builders) {
        free_string(area->builders);
        area->builders = str_dup(builders);
    }

    json_t* vnums = json_object_get(areadata, "vnumRange");
    if (json_is_array(vnums) && json_array_size(vnums) >= 2) {
        area->min_vnum = (VNUM)json_integer_value(json_array_get(vnums, 0));
        area->max_vnum = (VNUM)json_integer_value(json_array_get(vnums, 1));
    }

    const char* credits = json_string_value(json_object_get(areadata, "credits"));
    if (credits) {
        free_string(area->credits);
        area->credits = str_dup(credits);
    }

    area->security = (int)json_int_or_default(areadata, "security", area->security);
    json_t* sector_val = json_object_get(areadata, "sector");
    if (json_is_string(sector_val)) {
        FLAGS s = flag_lookup(json_string_value(sector_val), sector_flag_table);
        if (s != NO_FLAG)
            area->sector = (Sector)s;
    }
    else
        area->sector = (Sector)json_int_or_default(areadata, "sector", area->sector);
    area->low_range = (LEVEL)json_int_or_default(areadata, "lowLevel", area->low_range);
    area->high_range = (LEVEL)json_int_or_default(areadata, "highLevel", area->high_range);
    area->reset_thresh = (int16_t)json_int_or_default(areadata, "reset", area->reset_thresh);
    area->always_reset = json_bool_or_default(areadata, "alwaysReset", area->always_reset);
    json_t* inst = json_object_get(areadata, "instType");
    if (json_is_string(inst)) {
        const char* istr = json_string_value(inst);
        if (istr && !str_cmp(istr, "multi"))
            area->inst_type = AREA_INST_MULTI;
        else
            area->inst_type = AREA_INST_SINGLE;
    }
    else
        area->inst_type = (InstanceType)json_int_or_default(areadata, "instType", area->inst_type);

    parse_story_beats(json_object_get(root, "storyBeats"), area);
    parse_checklist(json_object_get(root, "checklist"), area);

    write_value_array(&global_areas, OBJ_VAL(area));
    if (global_areas.count > 0)
        LAST_AREA_DATA->next = area;
    area->next = NULL;
    current_area_data = area;

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static PersistResult parse_exits(RoomData* room, json_t* exits)
{
    if (!json_is_array(exits))
        return (PersistResult){ PERSIST_OK, NULL, -1 };

    size_t count = json_array_size(exits);
    for (size_t i = 0; i < count; i++) {
        json_t* ex = json_array_get(exits, i);
        if (!json_is_object(ex))
            continue;
        const char* dir_name = json_string_value(json_object_get(ex, "dir"));
        int dir = dir_enum_from_name(dir_name);
        if (dir < 0 || dir >= DIR_MAX)
            continue;

        RoomExitData* ex_data = new_room_exit_data();
        ex_data->orig_dir = dir;
        ex_data->to_vnum = (VNUM)json_int_or_default(ex, "toVnum", 0);
        ex_data->key = (int16_t)json_int_or_default(ex, "key", 0);
        ex_data->exit_reset_flags = flags_from_array(json_object_get(ex, "flags"), exit_flag_table);

        const char* desc = json_string_value(json_object_get(ex, "description"));
        ex_data->description = desc ? str_dup(desc) : &str_empty[0];
        const char* kw = json_string_value(json_object_get(ex, "keyword"));
        ex_data->keyword = kw ? str_dup(kw) : &str_empty[0];

        room->exit_data[dir] = ex_data;
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static PersistResult parse_rooms(json_t* root, AreaData* area)
{
    json_t* rooms = json_object_get(root, "rooms");
    if (!json_is_array(rooms))
        return (PersistResult){ PERSIST_OK, NULL, -1 };

    size_t count = json_array_size(rooms);
    for (size_t i = 0; i < count; i++) {
        json_t* r = json_array_get(rooms, i);
        if (!json_is_object(r))
            continue;
        RoomData* room = new_room_data();
        room->area_data = area;

        const char* name = json_string_value(json_object_get(r, "name"));
        if (name)
            SET_NAME(room, lox_string(name));
        const char* desc = json_string_value(json_object_get(r, "description"));
        if (desc) {
            free_string(room->description);
            room->description = str_dup(desc);
        }

        room->room_flags = flags_from_array(json_object_get(r, "roomFlags"), room_flag_table);
        room->sector_type = (Sector)json_int_or_default(r, "sectorType", room->sector_type);
        room->mana_rate = (int16_t)json_int_or_default(r, "manaRate", room->mana_rate);
        room->heal_rate = (int16_t)json_int_or_default(r, "healRate", room->heal_rate);
        room->clan = (int16_t)json_int_or_default(r, "clan", room->clan);
        const char* owner = json_string_value(json_object_get(r, "owner"));
        if (owner) {
            free_string(room->owner);
            room->owner = str_dup(owner);
        }

        VNUM vnum = (VNUM)json_int_or_default(r, "vnum", VNUM_NONE);
        VNUM_FIELD(room) = vnum;
        table_set_vnum(&global_rooms, vnum, OBJ_VAL(room));
        top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room;
        assign_area_vnum(vnum);

        parse_exits(room, json_object_get(r, "exits"));

        json_t* extra = json_object_get(r, "extraDescs");
        if (json_is_array(extra)) {
            size_t ed_count = json_array_size(extra);
            for (size_t j = 0; j < ed_count; j++) {
                json_t* e = json_array_get(extra, j);
                if (!json_is_object(e))
                    continue;
                ExtraDesc* ed = new_extra_desc();
                ed->keyword = str_dup(json_string_value(json_object_get(e, "keyword")));
                ed->description = str_dup(json_string_value(json_object_get(e, "description")));
                ADD_EXTRA_DESC(room, ed)
            }
        }

        const char* script = json_string_value(json_object_get(r, "loxScript"));
        if (script && script[0] != '\0')
            ((Entity*)room)->script = lox_string(script);
        parse_events(json_object_get(r, "events"), (Entity*)room, ENT_ROOM);
        ensure_entity_class((Entity*)room, "room");
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static const char* position_name(Position pos)
{
    if (pos < 0 || pos >= POS_MAX)
        return "standing";
    return position_table[pos].name;
}

static const char* sex_name(Sex sex)
{
    if (sex < 0 || sex >= SEX_COUNT)
        return sex_table[SEX_NEUTRAL].name;
    return sex_table[sex].name;
}

static json_t* build_affect(const Affect* af)
{
    json_t* obj = json_object();
    if (af->type >= 0 && af->type < skill_count && skill_table[af->type].name)
        json_object_set_new(obj, "type", json_string(skill_table[af->type].name));
    else
        json_object_set_new(obj, "type", json_integer(af->type));

    const char* where_name = flag_string(apply_types, af->where);
    if (where_name && where_name[0] != '\0')
        json_object_set_new(obj, "where", json_string(where_name));
    const char* loc_name = flag_string(apply_flag_table, af->location);
    if (loc_name && loc_name[0] != '\0')
        json_object_set_new(obj, "location", json_string(loc_name));
    else
        json_object_set_new(obj, "location", json_integer(af->location));

    json_object_set_new(obj, "level", json_integer(af->level));
    json_object_set_new(obj, "duration", json_integer(af->duration));
    json_object_set_new(obj, "modifier", json_integer(af->modifier));

    const struct bit_type* bv_type = &bitvector_type[af->where];
    if (bv_type && bv_type->table)
        json_set_flags_if(obj, "bitvector", af->bitvector, bv_type->table);
    else if (af->bitvector != 0)
        json_object_set_new(obj, "bitvectorValue", json_integer(af->bitvector));

    return obj;
}

static json_t* build_affects(const Affect* list)
{
    json_t* arr = json_array();
    for (const Affect* af = list; af != NULL; af = af->next)
        json_array_append_new(arr, build_affect(af));
    return arr;
}

static void parse_affects(json_t* arr, Affect** out_list)
{
    if (!json_is_array(arr) || !out_list)
        return;

    size_t count = json_array_size(arr);
    for (size_t i = 0; i < count; i++) {
        json_t* a = json_array_get(arr, i);
        if (!json_is_object(a))
            continue;
        Affect* af = new_affect();
        json_t* type = json_object_get(a, "type");
        if (json_is_string(type)) {
            SKNUM sn = skill_lookup(json_string_value(type));
            af->type = sn >= 0 ? sn : -1;
        }
        else
            af->type = (int16_t)json_int_or_default(a, "type", -1);

        const char* where = json_string_value(json_object_get(a, "where"));
        if (where) {
            FLAGS w = flag_lookup(where, apply_types);
            if (w != NO_FLAG)
                af->where = (int16_t)w;
        }
        json_t* loc = json_object_get(a, "location");
        if (json_is_string(loc)) {
            FLAGS l = flag_lookup(json_string_value(loc), apply_flag_table);
            if (l != NO_FLAG)
                af->location = (int16_t)l;
        }
        else
            af->location = (int16_t)json_int_or_default(a, "location", af->location);

        af->level = (int16_t)json_int_or_default(a, "level", af->level);
        af->duration = (int16_t)json_int_or_default(a, "duration", af->duration);
        af->modifier = (int16_t)json_int_or_default(a, "modifier", af->modifier);

        const struct bit_type* bv_type = &bitvector_type[af->where];
        json_t* bv = json_object_get(a, "bitvector");
        if (bv && bv_type && bv_type->table)
            af->bitvector = (int16_t)flags_from_array(bv, bv_type->table);
        else
            af->bitvector = (int16_t)json_int_or_default(a, "bitvectorValue", af->bitvector);

        af->next = *out_list;
        *out_list = af;
    }
}

static json_t* dice_to_json(const int16_t dice[3])
{
    json_t* obj = json_object();
    json_object_set_new(obj, "number", json_integer(dice[DICE_NUMBER]));
    json_object_set_new(obj, "type", json_integer(dice[DICE_TYPE]));
    if (dice[DICE_BONUS] != 0)
        json_object_set_new(obj, "bonus", json_integer(dice[DICE_BONUS]));
    return obj;
}

static void json_to_dice(json_t* src, int16_t dice[3])
{
    if (json_is_object(src)) {
        dice[DICE_NUMBER] = (int16_t)json_int_or_default(src, "number", dice[DICE_NUMBER]);
        dice[DICE_TYPE] = (int16_t)json_int_or_default(src, "type", dice[DICE_TYPE]);
        dice[DICE_BONUS] = (int16_t)json_int_or_default(src, "bonus", dice[DICE_BONUS]);
        return;
    }
    if (json_is_array(src) && json_array_size(src) >= 3) {
        dice[DICE_NUMBER] = (int16_t)json_integer_value(json_array_get(src, 0));
        dice[DICE_TYPE] = (int16_t)json_integer_value(json_array_get(src, 1));
        dice[DICE_BONUS] = (int16_t)json_integer_value(json_array_get(src, 2));
    }
}

static const char* skill_name_from_sn(SKNUM sn)
{
    if (sn >= 0 && sn < skill_count && skill_table[sn].name)
        return skill_table[sn].name;
    return NULL;
}

static SKNUM spell_lookup_name(const char* name)
{
    if (!name || !name[0])
        return -1;
    return skill_lookup(name);
}

static SKNUM parse_spell_entry(json_t* entry)
{
    if (json_is_integer(entry))
        return (SKNUM)json_integer_value(entry);
    const char* name = json_string_value(entry);
    return spell_lookup_name(name);
}

static void apply_spell_array(json_t* arr, ObjPrototype* obj, int start, int count)
{
    if (!json_is_array(arr))
        return;
    size_t sz = json_array_size(arr);
    for (size_t i = 0; i < sz && (int)i < count; i++) {
        SKNUM sn = parse_spell_entry(json_array_get(arr, i));
        if (sn >= 0)
            obj->value[start + (int)i] = sn;
    }
}

static json_t* build_mobiles(const AreaData* area)
{
    json_t* arr = json_array();
    if (mob_protos.capacity == 0 || mob_protos.entries == NULL)
        return arr;

    for (int idx = 0; idx < mob_protos.capacity; ++idx) {
        Entry* entry = &mob_protos.entries[idx];
        if (IS_NIL(entry->value) || !IS_MOB_PROTO(entry->value))
            continue;
        MobPrototype* mob = AS_MOB_PROTO(entry->value);
        if (mob->area != area)
            continue;

        json_t* obj = json_object();
        json_object_set_new(obj, "vnum", json_integer(VNUM_FIELD(mob)));
        json_object_set_new(obj, "name", json_string(NAME_STR(mob)));
        json_object_set_new(obj, "shortDescr", json_string(mob->short_descr));
        json_object_set_new(obj, "longDescr", json_string(mob->long_descr));
        json_object_set_new(obj, "description", json_string(mob->description ? mob->description : ""));
        json_object_set_new(obj, "race", json_string(race_table[mob->race].name));

        json_set_flags_if(obj, "actFlags", mob->act_flags, act_flag_table);
        json_set_flags_if(obj, "affectFlags", mob->affect_flags, affect_flag_table);
        json_set_flags_if(obj, "atkFlags", mob->atk_flags, off_flag_table);
        json_set_flags_if(obj, "immFlags", mob->imm_flags, imm_flag_table);
        json_set_flags_if(obj, "resFlags", mob->res_flags, res_flag_table);
        json_set_flags_if(obj, "vulnFlags", mob->vuln_flags, vuln_flag_table);
        json_set_flags_if(obj, "formFlags", mob->form, form_flag_table);
        json_set_flags_if(obj, "partFlags", mob->parts, part_flag_table);

        json_object_set_new(obj, "alignment", json_integer(mob->alignment));
        json_object_set_new(obj, "group", json_integer(mob->group));
        json_object_set_new(obj, "level", json_integer(mob->level));
        json_object_set_new(obj, "hitroll", json_integer(mob->hitroll));
        json_object_set_new(obj, "hitDice", dice_to_json(mob->hit));
        json_object_set_new(obj, "manaDice", dice_to_json(mob->mana));
        json_object_set_new(obj, "damageDice", dice_to_json(mob->damage));
        json_object_set_new(obj, "damType", json_string(attack_table[mob->dam_type].name));

        json_t* ac = json_object();
        json_object_set_new(ac, "pierce", json_integer(mob->ac[AC_PIERCE] / 10));
        json_object_set_new(ac, "bash", json_integer(mob->ac[AC_BASH] / 10));
        json_object_set_new(ac, "slash", json_integer(mob->ac[AC_SLASH] / 10));
        json_object_set_new(ac, "exotic", json_integer(mob->ac[AC_EXOTIC] / 10));
        json_object_set_new(obj, "ac", ac);

        json_object_set_new(obj, "startPos", json_string(position_name(mob->start_pos)));
        json_object_set_new(obj, "defaultPos", json_string(position_name(mob->default_pos)));
        json_object_set_new(obj, "sex", json_string(sex_name(mob->sex)));
        json_object_set_new(obj, "wealth", json_integer(mob->wealth));
        json_object_set_new(obj, "size", json_string(size_name(mob->size)));
        json_object_set_new(obj, "material", json_string(mob->material ? mob->material : ""));
        if (mob->faction_vnum != 0)
            json_object_set_new(obj, "factionVnum", json_integer(mob->faction_vnum));

        Entity* ent = (Entity*)mob;
        if (ent->script && ent->script->chars && ent->script->length > 0)
            json_object_set_new(obj, "loxScript", json_string(ent->script->chars));
        json_t* ev = build_events(ent);
        if (json_array_size(ev) > 0)
            json_object_set_new(obj, "events", ev);
        else
            json_decref(ev);

        json_array_append_new(arr, obj);
    }

    return arr;
}

static PersistResult parse_mobiles(json_t* root, AreaData* area)
{
    json_t* mobs = json_object_get(root, "mobiles");
    if (!json_is_array(mobs))
        return (PersistResult){ PERSIST_OK, NULL, -1 };

    size_t count = json_array_size(mobs);
    for (size_t i = 0; i < count; i++) {
        json_t* m = json_array_get(mobs, i);
        if (!json_is_object(m))
            continue;
        MobPrototype* mob = new_mob_prototype();
        mob->area = area;

        const char* name = json_string_value(json_object_get(m, "name"));
        if (name)
            SET_NAME(mob, lox_string(name));
        const char* sd = json_string_value(json_object_get(m, "shortDescr"));
        if (sd) {
            free_string(mob->short_descr);
            mob->short_descr = str_dup(sd);
        }
        const char* ld = json_string_value(json_object_get(m, "longDescr"));
        if (ld) {
            free_string(mob->long_descr);
            mob->long_descr = str_dup(ld);
        }
        const char* desc = json_string_value(json_object_get(m, "description"));
        if (desc) {
            free_string(mob->description);
            mob->description = str_dup(desc);
        }

        const char* race_name = json_string_value(json_object_get(m, "race"));
        if (race_name) {
            int r = race_lookup(race_name);
            if (r >= 0)
                mob->race = (int16_t)r;
        }

        mob->act_flags = flags_from_array(json_object_get(m, "actFlags"), act_flag_table);
        mob->affect_flags = flags_from_array(json_object_get(m, "affectFlags"), affect_flag_table);
        mob->atk_flags = flags_from_array(json_object_get(m, "atkFlags"), off_flag_table);
        mob->imm_flags = flags_from_array(json_object_get(m, "immFlags"), imm_flag_table);
        mob->res_flags = flags_from_array(json_object_get(m, "resFlags"), res_flag_table);
        mob->vuln_flags = flags_from_array(json_object_get(m, "vulnFlags"), vuln_flag_table);
        mob->form = flags_from_array(json_object_get(m, "formFlags"), form_flag_table);
        mob->parts = flags_from_array(json_object_get(m, "partFlags"), part_flag_table);

        mob->alignment = (int16_t)json_int_or_default(m, "alignment", mob->alignment);
        mob->group = (int16_t)json_int_or_default(m, "group", mob->group);
        mob->level = (int16_t)json_int_or_default(m, "level", mob->level);
        mob->hitroll = (int16_t)json_int_or_default(m, "hitroll", mob->hitroll);
        json_to_dice(json_object_get(m, "hitDice"), mob->hit);
        json_to_dice(json_object_get(m, "manaDice"), mob->mana);
        json_to_dice(json_object_get(m, "damageDice"), mob->damage);

        const char* dam = json_string_value(json_object_get(m, "damType"));
        if (dam) {
            int dt = attack_lookup(dam);
            if (dt >= 0)
                mob->dam_type = (int16_t)dt;
        }

        json_t* ac = json_object_get(m, "ac");
        if (json_is_object(ac)) {
            mob->ac[AC_PIERCE] = (int16_t)json_int_or_default(ac, "pierce", mob->ac[AC_PIERCE] / 10) * 10;
            mob->ac[AC_BASH] = (int16_t)json_int_or_default(ac, "bash", mob->ac[AC_BASH] / 10) * 10;
            mob->ac[AC_SLASH] = (int16_t)json_int_or_default(ac, "slash", mob->ac[AC_SLASH] / 10) * 10;
            mob->ac[AC_EXOTIC] = (int16_t)json_int_or_default(ac, "exotic", mob->ac[AC_EXOTIC] / 10) * 10;
        }
        else if (json_is_array(ac) && json_array_size(ac) >= 4) {
            mob->ac[AC_PIERCE] = (int16_t)json_integer_value(json_array_get(ac, 0)) * 10;
            mob->ac[AC_BASH] = (int16_t)json_integer_value(json_array_get(ac, 1)) * 10;
            mob->ac[AC_SLASH] = (int16_t)json_integer_value(json_array_get(ac, 2)) * 10;
            mob->ac[AC_EXOTIC] = (int16_t)json_integer_value(json_array_get(ac, 3)) * 10;
        }

        const char* startPos = json_string_value(json_object_get(m, "startPos"));
        if (startPos)
            mob->start_pos = (int16_t)position_lookup(startPos);
        const char* defPos = json_string_value(json_object_get(m, "defaultPos"));
        if (defPos)
            mob->default_pos = (int16_t)position_lookup(defPos);
        const char* sex = json_string_value(json_object_get(m, "sex"));
        if (sex)
            mob->sex = (Sex)sex_lookup(sex);

        mob->wealth = (int)json_int_or_default(m, "wealth", mob->wealth);
        const char* size = json_string_value(json_object_get(m, "size"));
        if (size)
            mob->size = (int16_t)size_lookup(size);
        const char* mat = json_string_value(json_object_get(m, "material"));
        if (mat) {
            free_string(mob->material);
            mob->material = str_dup(mat);
        }
        mob->faction_vnum = (VNUM)json_int_or_default(m, "factionVnum", mob->faction_vnum);

        const char* script = json_string_value(json_object_get(m, "loxScript"));
        if (script && script[0] != '\0')
            ((Entity*)mob)->script = lox_string(script);
        parse_events(json_object_get(m, "events"), (Entity*)mob, ENT_MOB);
        ensure_entity_class((Entity*)mob, "mob");

        VNUM vnum = (VNUM)json_int_or_default(m, "vnum", VNUM_NONE);
        VNUM_FIELD(mob) = vnum;
        table_set_vnum(&mob_protos, vnum, OBJ_VAL(mob));
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;
        assign_area_vnum(vnum);
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static json_t* build_shops(const AreaData* area)
{
    json_t* arr = json_array();
    if (mob_protos.capacity == 0 || mob_protos.entries == NULL)
        return arr;

    for (int idx = 0; idx < mob_protos.capacity; ++idx) {
        Entry* entry = &mob_protos.entries[idx];
        if (IS_NIL(entry->value) || !IS_MOB_PROTO(entry->value))
            continue;
        MobPrototype* mob = AS_MOB_PROTO(entry->value);
        if (mob->area != area || mob->pShop == NULL)
            continue;

        ShopData* shop = mob->pShop;
        json_t* obj = json_object();
        json_object_set_new(obj, "keeper", json_integer(VNUM_FIELD(mob)));
        json_t* buy = json_array();
        for (int i = 0; i < MAX_TRADE; i++)
            json_array_append_new(buy, json_integer(shop->buy_type[i]));
        json_object_set_new(obj, "buyTypes", buy);
        json_object_set_new(obj, "profitBuy", json_integer(shop->profit_buy));
        json_object_set_new(obj, "profitSell", json_integer(shop->profit_sell));
        json_object_set_new(obj, "openHour", json_integer(shop->open_hour));
        json_object_set_new(obj, "closeHour", json_integer(shop->close_hour));
        json_array_append_new(arr, obj);
    }

    return arr;
}

static PersistResult parse_shops(json_t* root)
{
    json_t* shops = json_object_get(root, "shops");
    if (!json_is_array(shops))
        return (PersistResult){ PERSIST_OK, NULL, -1 };

    size_t count = json_array_size(shops);
    for (size_t i = 0; i < count; i++) {
        json_t* s = json_array_get(shops, i);
        if (!json_is_object(s))
            continue;
        VNUM keeper = (VNUM)json_int_or_default(s, "keeper", 0);
        MobPrototype* mob = get_mob_prototype(keeper);
        if (!mob)
            continue;
        ShopData* shop = new_shop_data();
        shop->keeper = (int16_t)keeper;
        for (int j = 0; j < MAX_TRADE; j++)
            shop->buy_type[j] = 0;

        json_t* buy = json_object_get(s, "buyTypes");
        if (json_is_array(buy)) {
            size_t bsz = json_array_size(buy);
            for (size_t j = 0; j < bsz && j < MAX_TRADE; j++)
                shop->buy_type[j] = (int16_t)json_integer_value(json_array_get(buy, j));
        }
        shop->profit_buy = (int16_t)json_int_or_default(s, "profitBuy", shop->profit_buy);
        shop->profit_sell = (int16_t)json_int_or_default(s, "profitSell", shop->profit_sell);
        shop->open_hour = (int16_t)json_int_or_default(s, "openHour", shop->open_hour);
        shop->close_hour = (int16_t)json_int_or_default(s, "closeHour", shop->close_hour);

        mob->pShop = shop;
        if (shop_first == NULL)
            shop_first = shop;
        if (shop_last != NULL)
            shop_last->next = shop;
        shop_last = shop;
        shop->next = NULL;
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
static json_t* build_object_values(const ObjPrototype* obj)
{
    json_t* arr = json_array();
    for (int i = 0; i < 5; i++)
        json_array_append_new(arr, json_integer(obj->value[i]));
    return arr;
}

static void apply_object_values(ObjPrototype* obj, json_t* values)
{
    if (!json_is_array(values))
        return;
    size_t sz = json_array_size(values);
    for (size_t i = 0; i < sz && i < 5; i++) {
        obj->value[i] = (int16_t)json_integer_value(json_array_get(values, i));
    }
}

static json_t* build_weapon(const ObjPrototype* obj)
{
    json_t* w = json_object();
    const char* wclass = flag_string(weapon_class, obj->value[0]);
    if (wclass && wclass[0] != '\0' && obj->value[0] != 0)
        json_object_set_new(w, "class", json_string(wclass));
    if (obj->value[1] != 0 || obj->value[2] != 0) {
        json_t* dice = json_array();
        json_array_append_new(dice, json_integer(obj->value[1]));
        json_array_append_new(dice, json_integer(obj->value[2]));
        json_object_set_new(w, "dice", dice);
    }
    if (obj->value[3] != 0)
        json_object_set_new(w, "damageType", json_string(attack_table[obj->value[3]].name));
    json_set_flags_if(w, "flags", obj->value[4], weapon_type2);
    return w;
}

static json_t* build_container(const ObjPrototype* obj)
{
    json_t* c = json_object();
    json_set_int_if(c, "capacity", obj->value[0], 0);
    json_set_flags_if(c, "flags", obj->value[1], container_flag_table);
    json_set_int_if(c, "keyVnum", obj->value[2], 0);
    json_set_int_if(c, "maxWeight", obj->value[3], 0);
    json_set_int_if(c, "weightMult", obj->value[4], 0);
    return c;
}

static void apply_weapon(ObjPrototype* obj, json_t* weapon)
{
    if (!json_is_object(weapon))
        return;
    const char* wclass = json_string_value(json_object_get(weapon, "class"));
    if (wclass) {
        FLAGS val = flag_lookup(wclass, weapon_class);
        if (val != NO_FLAG)
            obj->value[0] = (int16_t)val;
    }
    const char* dtype = json_string_value(json_object_get(weapon, "damageType"));
    if (dtype) {
        int dt = attack_lookup(dtype);
        if (dt >= 0)
            obj->value[3] = dt;
    }
    json_t* dice = json_object_get(weapon, "dice");
    if (json_is_array(dice) && json_array_size(dice) >= 2) {
        obj->value[1] = (int16_t)json_integer_value(json_array_get(dice, 0));
        obj->value[2] = (int16_t)json_integer_value(json_array_get(dice, 1));
    }
    obj->value[4] = (int16_t)flags_from_array(json_object_get(weapon, "flags"), weapon_type2);
}

static void apply_container(ObjPrototype* obj, json_t* container)
{
    if (!json_is_object(container))
        return;
    obj->value[0] = (int16_t)json_int_or_default(container, "capacity", obj->value[0]);
    obj->value[1] = (int16_t)flags_from_array(json_object_get(container, "flags"), container_flag_table);
    obj->value[2] = (int16_t)json_int_or_default(container, "keyVnum", obj->value[2]);
    obj->value[3] = (int16_t)json_int_or_default(container, "maxWeight", obj->value[3]);
    obj->value[4] = (int16_t)json_int_or_default(container, "weightMult", obj->value[4]);
}

static json_t* build_light(const ObjPrototype* obj)
{
    json_t* l = json_object();
    json_set_int_if(l, "hours", obj->value[2], 0);
    return l;
}

static json_t* build_armor(const ObjPrototype* obj)
{
    json_t* a = json_object();
    json_set_int_if(a, "acPierce", obj->value[0], 0);
    json_set_int_if(a, "acBash", obj->value[1], 0);
    json_set_int_if(a, "acSlash", obj->value[2], 0);
    json_set_int_if(a, "acExotic", obj->value[3], 0);
    return a;
}

static json_t* build_drink(const ObjPrototype* obj)
{
    json_t* d = json_object();
    json_set_int_if(d, "capacity", obj->value[0], 0);
    json_set_int_if(d, "remaining", obj->value[1], 0);
    if (obj->value[2] > 0 && obj->value[2] < LIQ_COUNT)
        json_object_set_new(d, "liquid", json_string(liquid_table[obj->value[2]].name));
    if (obj->value[3] != 0)
        json_object_set_new(d, "poisoned", json_boolean(true));
    return d;
}

static json_t* build_fountain(const ObjPrototype* obj)
{
    json_t* f = json_object();
    json_set_int_if(f, "capacity", obj->value[0], 0);
    json_set_int_if(f, "remaining", obj->value[1], 0);
    if (obj->value[2] > 0 && obj->value[2] < LIQ_COUNT)
        json_object_set_new(f, "liquid", json_string(liquid_table[obj->value[2]].name));
    return f;
}

static json_t* build_food(const ObjPrototype* obj)
{
    json_t* f = json_object();
    json_set_int_if(f, "foodHours", obj->value[0], 0);
    json_set_int_if(f, "fullHours", obj->value[1], 0);
    if (obj->value[3] != 0)
        json_object_set_new(f, "poisoned", json_boolean(true));
    return f;
}

static json_t* build_money(const ObjPrototype* obj)
{
    json_t* m = json_object();
    json_set_int_if(m, "gold", obj->value[0], 0);
    json_set_int_if(m, "silver", obj->value[1], 0);
    return m;
}

static json_t* build_wandstaff(const ObjPrototype* obj)
{
    json_t* w = json_object();
    json_set_int_if(w, "level", obj->value[0], 0);
    json_set_int_if(w, "chargesTotal", obj->value[1], 0);
    if (obj->value[2] != obj->value[1])
        json_set_int_if(w, "chargesRemaining", obj->value[2], 0);
    const char* spell = skill_name_from_sn(obj->value[3]);
    if (spell && obj->value[3] >= 0)
        json_object_set_new(w, "spell", json_string(spell));
    return w;
}

static json_t* build_scroll_potion_pill(const ObjPrototype* obj)
{
    json_t* s = json_object();
    json_set_int_if(s, "level", obj->value[0], 0);
    json_t* spells = json_array();
    for (int i = 1; i <= 4; i++) {
        const char* name = skill_name_from_sn(obj->value[i]);
        if (name)
            json_array_append_new(spells, json_string(name));
    }
    if (json_array_size(spells) > 0)
        json_object_set_new(s, "spells", spells);
    else
        json_decref(spells);
    return s;
}

static json_t* build_portal(const ObjPrototype* obj)
{
    json_t* p = json_object();
    json_set_int_if(p, "charges", obj->value[0], 0);
    json_set_flags_if(p, "exitFlags", obj->value[1], exit_flag_table);
    json_set_flags_if(p, "portalFlags", obj->value[2], portal_flag_table);
    json_set_int_if(p, "toVnum", obj->value[3], 0);
    return p;
}

static json_t* build_furniture(const ObjPrototype* obj)
{
    json_t* f = json_object();
    json_set_int_if(f, "maxPeople", obj->value[0], 0);
    json_set_int_if(f, "maxWeight", obj->value[1], 0);
    json_set_flags_if(f, "flags", obj->value[2], furniture_flag_table);
    json_set_int_if(f, "healBonus", obj->value[3], 0);
    json_set_int_if(f, "manaBonus", obj->value[4], 0);
    return f;
}

static void apply_light(ObjPrototype* obj, json_t* light)
{
    if (!json_is_object(light))
        return;
    obj->value[2] = (int16_t)json_int_or_default(light, "hours", obj->value[2]);
}

static void apply_armor(ObjPrototype* obj, json_t* armor)
{
    if (!json_is_object(armor))
        return;
    obj->value[0] = (int16_t)json_int_or_default(armor, "acPierce", obj->value[0]);
    obj->value[1] = (int16_t)json_int_or_default(armor, "acBash", obj->value[1]);
    obj->value[2] = (int16_t)json_int_or_default(armor, "acSlash", obj->value[2]);
    obj->value[3] = (int16_t)json_int_or_default(armor, "acExotic", obj->value[3]);
}

static void apply_drink(ObjPrototype* obj, json_t* drink)
{
    if (!json_is_object(drink))
        return;
    obj->value[0] = (int16_t)json_int_or_default(drink, "capacity", obj->value[0]);
    obj->value[1] = (int16_t)json_int_or_default(drink, "remaining", obj->value[1]);
    const char* liquid = json_string_value(json_object_get(drink, "liquid"));
    if (liquid) {
        int l = liquid_lookup(liquid);
        if (l >= 0)
            obj->value[2] = (int16_t)l;
    }
    obj->value[3] = json_bool_or_default(drink, "poisoned", obj->value[3] != 0) ? 1 : 0;
}

static void apply_fountain(ObjPrototype* obj, json_t* fountain)
{
    if (!json_is_object(fountain))
        return;
    obj->value[0] = (int16_t)json_int_or_default(fountain, "capacity", obj->value[0]);
    obj->value[1] = (int16_t)json_int_or_default(fountain, "remaining", obj->value[1]);
    const char* liquid = json_string_value(json_object_get(fountain, "liquid"));
    if (liquid) {
        int l = liquid_lookup(liquid);
        if (l >= 0)
            obj->value[2] = (int16_t)l;
    }
}

static void apply_food(ObjPrototype* obj, json_t* food)
{
    if (!json_is_object(food))
        return;
    obj->value[0] = (int16_t)json_int_or_default(food, "foodHours", obj->value[0]);
    obj->value[1] = (int16_t)json_int_or_default(food, "fullHours", obj->value[1]);
    obj->value[3] = json_bool_or_default(food, "poisoned", obj->value[3] != 0) ? 1 : 0;
}

static void apply_money(ObjPrototype* obj, json_t* money)
{
    if (!json_is_object(money))
        return;
    obj->value[0] = (int16_t)json_int_or_default(money, "gold", obj->value[0]);
    obj->value[1] = (int16_t)json_int_or_default(money, "silver", obj->value[1]);
}

static void apply_wandstaff(ObjPrototype* obj, json_t* wand)
{
    if (!json_is_object(wand))
        return;
    obj->value[0] = (int16_t)json_int_or_default(wand, "level", obj->value[0]);
    obj->value[1] = (int16_t)json_int_or_default(wand, "chargesTotal", obj->value[1]);
    obj->value[2] = (int16_t)json_int_or_default(wand, "chargesRemaining", obj->value[2]);
    const char* spell = json_string_value(json_object_get(wand, "spell"));
    SKNUM sn = spell_lookup_name(spell);
    if (sn >= 0)
        obj->value[3] = sn;
}

static void apply_scroll_potion_pill(ObjPrototype* obj, json_t* sp)
{
    if (!json_is_object(sp))
        return;
    obj->value[0] = (int16_t)json_int_or_default(sp, "level", obj->value[0]);
    apply_spell_array(json_object_get(sp, "spells"), obj, 1, 4);
}

static void apply_portal(ObjPrototype* obj, json_t* portal)
{
    if (!json_is_object(portal))
        return;
    obj->value[0] = (int16_t)json_int_or_default(portal, "charges", obj->value[0]);
    obj->value[1] = (int16_t)flags_from_array(json_object_get(portal, "exitFlags"), exit_flag_table);
    obj->value[2] = (int16_t)flags_from_array(json_object_get(portal, "portalFlags"), portal_flag_table);
    obj->value[3] = (int16_t)json_int_or_default(portal, "toVnum", obj->value[3]);
}

static void apply_furniture(ObjPrototype* obj, json_t* furniture)
{
    if (!json_is_object(furniture))
        return;
    obj->value[0] = (int16_t)json_int_or_default(furniture, "maxPeople", obj->value[0]);
    obj->value[1] = (int16_t)json_int_or_default(furniture, "maxWeight", obj->value[1]);
    obj->value[2] = (int16_t)flags_from_array(json_object_get(furniture, "flags"), furniture_flag_table);
    obj->value[3] = (int16_t)json_int_or_default(furniture, "healBonus", obj->value[3]);
    obj->value[4] = (int16_t)json_int_or_default(furniture, "manaBonus", obj->value[4]);
}


static json_t* build_objects(const AreaData* area)
{
    json_t* arr = json_array();
    if (obj_protos.capacity == 0 || obj_protos.entries == NULL)
        return arr;

    for (int idx = 0; idx < obj_protos.capacity; ++idx) {
        Entry* entry = &obj_protos.entries[idx];
        if (IS_NIL(entry->value) || !IS_OBJ_PROTO(entry->value))
            continue;
        ObjPrototype* obj = AS_OBJ_PROTO(entry->value);
        if (obj->area != area)
            continue;

        json_t* o = json_object();
        json_object_set_new(o, "vnum", json_integer(VNUM_FIELD(obj)));
        json_object_set_new(o, "name", json_string(NAME_STR(obj)));
        json_object_set_new(o, "shortDescr", json_string(obj->short_descr));
        json_object_set_new(o, "description", json_string(obj->description));
        if (obj->material && obj->material[0] != '\0')
            json_object_set_new(o, "material", json_string(obj->material));
        json_object_set_new(o, "itemType", json_string(flag_string(type_flag_table, obj->item_type)));
        json_set_flags_if(o, "extraFlags", obj->extra_flags, extra_flag_table);
        json_set_flags_if(o, "wearFlags", obj->wear_flags, wear_flag_table);
        bool has_value = false;
        for (int i = 0; i < 5; i++) {
            if (obj->value[i] != 0) {
                has_value = true;
                break;
            }
        }
        bool typed_emitted = false;
        switch (obj->item_type) {
        case ITEM_WEAPON:
        {
            json_t* w = build_weapon(obj);
            if (json_object_size(w) > 0)
                json_object_set_new(o, "weapon", w);
            else
                json_decref(w);
            typed_emitted = true;
            break;
        }
        case ITEM_CONTAINER:
        {
            json_t* c = build_container(obj);
            if (json_object_size(c) > 0)
                json_object_set_new(o, "container", c);
            else
                json_decref(c);
            typed_emitted = true;
            break;
        }
        case ITEM_LIGHT:
        {
            json_t* l = build_light(obj);
            if (json_object_size(l) > 0)
                json_object_set_new(o, "light", l);
            else
                json_decref(l);
            typed_emitted = true;
            break;
        }
        case ITEM_ARMOR:
        {
            json_t* a = build_armor(obj);
            if (json_object_size(a) > 0)
                json_object_set_new(o, "armor", a);
            else
                json_decref(a);
            typed_emitted = true;
            break;
        }
        case ITEM_DRINK_CON:
        {
            json_t* d = build_drink(obj);
            if (json_object_size(d) > 0)
                json_object_set_new(o, "drink", d);
            else
                json_decref(d);
            typed_emitted = true;
            break;
        }
        case ITEM_FOUNTAIN:
        {
            json_t* f = build_fountain(obj);
            if (json_object_size(f) > 0)
                json_object_set_new(o, "fountain", f);
            else
                json_decref(f);
            typed_emitted = true;
            break;
        }
        case ITEM_FOOD:
        {
            json_t* f = build_food(obj);
            if (json_object_size(f) > 0)
                json_object_set_new(o, "food", f);
            else
                json_decref(f);
            typed_emitted = true;
            break;
        }
        case ITEM_MONEY:
        {
            json_t* m = build_money(obj);
            if (json_object_size(m) > 0)
                json_object_set_new(o, "money", m);
            else
                json_decref(m);
            typed_emitted = true;
            break;
        }
        case ITEM_WAND:
        case ITEM_STAFF:
        {
            json_t* w = build_wandstaff(obj);
            if (json_object_size(w) > 0)
                json_object_set_new(o, "wand", w);
            else
                json_decref(w);
            typed_emitted = true;
            break;
        }
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
        {
            json_t* s = build_scroll_potion_pill(obj);
            if (json_object_size(s) > 0)
                json_object_set_new(o, "spells", s);
            else
                json_decref(s);
            typed_emitted = true;
            break;
        }
        case ITEM_PORTAL:
        {
            json_t* p = build_portal(obj);
            if (json_object_size(p) > 0)
                json_object_set_new(o, "portal", p);
            else
                json_decref(p);
            typed_emitted = true;
            break;
        }
        case ITEM_FURNITURE:
        {
            json_t* f = build_furniture(obj);
            if (json_object_size(f) > 0)
                json_object_set_new(o, "furniture", f);
            else
                json_decref(f);
            typed_emitted = true;
            break;
        }
        default:
            break;
        }
        if (has_value && !typed_emitted)
            json_object_set_new(o, "values", build_object_values(obj));
        json_set_int_if(o, "level", obj->level, 0);
        json_set_int_if(o, "weight", obj->weight, 0);
        json_set_int_if(o, "cost", obj->cost, 0);
        json_set_int_if(o, "condition", obj->condition, 0);

        if (obj->extra_desc)
            json_object_set_new(o, "extraDescs", build_extra_descs(obj->extra_desc));
        if (obj->affected)
            json_object_set_new(o, "affects", build_affects(obj->affected));

        Entity* ent = (Entity*)obj;
        if (ent->script && ent->script->chars && ent->script->length > 0)
            json_object_set_new(o, "loxScript", json_string(ent->script->chars));
        json_t* ev = build_events(ent);
        if (json_array_size(ev) > 0)
            json_object_set_new(o, "events", ev);
        else
            json_decref(ev);

        json_array_append_new(arr, o);
    }

    return arr;
}

static PersistResult parse_objects(json_t* root, AreaData* area)
{
    json_t* objs = json_object_get(root, "objects");
    if (!json_is_array(objs))
        return (PersistResult){ PERSIST_OK, NULL, -1 };

    size_t count = json_array_size(objs);
    for (size_t i = 0; i < count; i++) {
        json_t* o = json_array_get(objs, i);
        if (!json_is_object(o))
            continue;
        ObjPrototype* obj = new_object_prototype();
        obj->area = area;

        const char* name = json_string_value(json_object_get(o, "name"));
        if (name)
            SET_NAME(obj, lox_string(name));
        const char* sd = json_string_value(json_object_get(o, "shortDescr"));
        if (sd) {
            free_string(obj->short_descr);
            obj->short_descr = str_dup(sd);
        }
        const char* desc = json_string_value(json_object_get(o, "description"));
        if (desc) {
            free_string(obj->description);
            obj->description = str_dup(desc);
        }
        const char* mat = json_string_value(json_object_get(o, "material"));
        if (mat) {
            free_string(obj->material);
            obj->material = str_dup(mat);
        }

        const char* itype = json_string_value(json_object_get(o, "itemType"));
        if (itype) {
            FLAGS val = flag_lookup(itype, type_flag_table);
            if (val != NO_FLAG)
                obj->item_type = (ItemType)val;
        }

        obj->extra_flags = flags_from_array(json_object_get(o, "extraFlags"), extra_flag_table);
        obj->wear_flags = flags_from_array(json_object_get(o, "wearFlags"), wear_flag_table);
        apply_object_values(obj, json_object_get(o, "values"));
        switch (obj->item_type) {
        case ITEM_WEAPON:
            apply_weapon(obj, json_object_get(o, "weapon"));
            break;
        case ITEM_CONTAINER:
            apply_container(obj, json_object_get(o, "container"));
            break;
        case ITEM_LIGHT:
            apply_light(obj, json_object_get(o, "light"));
            break;
        case ITEM_ARMOR:
            apply_armor(obj, json_object_get(o, "armor"));
            break;
        case ITEM_DRINK_CON:
            apply_drink(obj, json_object_get(o, "drink"));
            break;
        case ITEM_FOUNTAIN:
            apply_fountain(obj, json_object_get(o, "fountain"));
            break;
        case ITEM_FOOD:
            apply_food(obj, json_object_get(o, "food"));
            break;
        case ITEM_MONEY:
            apply_money(obj, json_object_get(o, "money"));
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            apply_wandstaff(obj, json_object_get(o, "wand"));
            break;
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            apply_scroll_potion_pill(obj, json_object_get(o, "spells"));
            break;
        case ITEM_PORTAL:
            apply_portal(obj, json_object_get(o, "portal"));
            break;
        case ITEM_FURNITURE:
            apply_furniture(obj, json_object_get(o, "furniture"));
            break;
        default:
            break;
        }
        obj->level = (int16_t)json_int_or_default(o, "level", obj->level);
        obj->weight = (int16_t)json_int_or_default(o, "weight", obj->weight);
        obj->cost = (int)json_int_or_default(o, "cost", obj->cost);
        obj->condition = (int16_t)json_int_or_default(o, "condition", obj->condition);

        json_t* extra = json_object_get(o, "extraDescs");
        if (json_is_array(extra)) {
            size_t ed_count = json_array_size(extra);
            for (size_t j = 0; j < ed_count; j++) {
                json_t* e = json_array_get(extra, j);
                if (!json_is_object(e))
                    continue;
                ExtraDesc* ed = new_extra_desc();
                ed->keyword = str_dup(json_string_value(json_object_get(e, "keyword")));
                ed->description = str_dup(json_string_value(json_object_get(e, "description")));
                ADD_EXTRA_DESC(obj, ed)
            }
        }
        parse_affects(json_object_get(o, "affects"), &obj->affected);

        const char* script = json_string_value(json_object_get(o, "loxScript"));
        if (script && script[0] != '\0')
            ((Entity*)obj)->script = lox_string(script);
        parse_events(json_object_get(o, "events"), (Entity*)obj, ENT_OBJ);
        ensure_entity_class((Entity*)obj, "obj");

        VNUM vnum = (VNUM)json_int_or_default(o, "vnum", VNUM_NONE);
        VNUM_FIELD(obj) = vnum;
        obj->material = obj->material ? obj->material : str_dup("unknown");
        table_set_vnum(&obj_protos, vnum, OBJ_VAL(obj));
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;
        assign_area_vnum(vnum);
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static json_t* build_mobprogs(const AreaData* area)
{
    json_t* arr = json_array();
    FOR_EACH_AREA(area) {
        for (VNUM vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
            MobProgCode* prog = get_mprog_index(vnum);
            if (prog) {
                json_t* obj = json_object();
                json_object_set_new(obj, "vnum", json_integer(vnum));
                json_object_set_new(obj, "code", json_string(prog->code));
                json_array_append_new(arr, obj);
            }
        }
    }
    return arr;
}

static PersistResult parse_mobprogs(json_t* root)
{
    json_t* progs = json_object_get(root, "mobprogs");
    if (!json_is_array(progs))
        return (PersistResult){ PERSIST_OK, NULL, -1 };

    size_t count = json_array_size(progs);
    for (size_t i = 0; i < count; i++) {
        json_t* p = json_array_get(progs, i);
        if (!json_is_object(p))
            continue;
        VNUM vnum = (VNUM)json_int_or_default(p, "vnum", 0);
        const char* code = json_string_value(json_object_get(p, "code"));
        if (vnum <= 0 || !code)
            continue;
        MobProgCode* prog = new_mob_prog_code();
        prog->vnum = vnum;
        prog->code = str_dup(code);
        ORDERED_INSERT(MobProgCode, prog, mprog_list, vnum);
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static json_t* build_quests(const AreaData* area)
{
    json_t* arr = json_array();
    Quest* q;
    FOR_EACH(q, area->quests) {
        json_t* obj = json_object();
        json_object_set_new(obj, "vnum", json_integer(q->vnum));
        json_object_set_new(obj, "name", json_string(q->name ? q->name : ""));
        json_object_set_new(obj, "entry", json_string(q->entry ? q->entry : ""));
        json_object_set_new(obj, "type", json_string(flag_string(quest_type_table, q->type)));
        json_object_set_new(obj, "xp", json_integer(q->xp));
        json_object_set_new(obj, "level", json_integer(q->level));
        json_object_set_new(obj, "end", json_integer(q->end));
        json_object_set_new(obj, "target", json_integer(q->target));
        json_object_set_new(obj, "upper", json_integer(q->target_upper));
        json_object_set_new(obj, "count", json_integer(q->amount));
        json_object_set_new(obj, "rewardFaction", json_integer(q->reward_faction_vnum));
        json_object_set_new(obj, "rewardReputation", json_integer(q->reward_reputation));
        json_object_set_new(obj, "rewardGold", json_integer(q->reward_gold));
        json_object_set_new(obj, "rewardSilver", json_integer(q->reward_silver));
        json_object_set_new(obj, "rewardCopper", json_integer(q->reward_copper));
        json_t* ro = json_array();
        json_t* rc = json_array();
        for (int i = 0; i < QUEST_MAX_REWARD_ITEMS; i++) {
            json_array_append_new(ro, json_integer(q->reward_obj_vnum[i]));
            json_array_append_new(rc, json_integer(q->reward_obj_count[i]));
        }
        json_object_set_new(obj, "rewardObjs", ro);
        json_object_set_new(obj, "rewardCounts", rc);
        json_array_append_new(arr, obj);
    }
    return arr;
}

static PersistResult parse_quests(json_t* root, AreaData* area)
{
    json_t* quests = json_object_get(root, "quests");
    if (!json_is_array(quests))
        return (PersistResult){ PERSIST_OK, NULL, -1 };

    size_t count = json_array_size(quests);
    for (size_t i = 0; i < count; i++) {
        json_t* q = json_array_get(quests, i);
        if (!json_is_object(q))
            continue;
        Quest* quest = new_quest();
        quest->area_data = area;

        const char* name = json_string_value(json_object_get(q, "name"));
        if (name) {
            free_string(quest->name);
            quest->name = str_dup(name);
        }
        const char* entry = json_string_value(json_object_get(q, "entry"));
        if (entry) {
            free_string(quest->entry);
            quest->entry = str_dup(entry);
        }
        const char* type = json_string_value(json_object_get(q, "type"));
        if (type) {
            FLAGS t = flag_lookup(type, quest_type_table);
            if (t != NO_FLAG)
                quest->type = (QuestType)t;
        }
        quest->xp = (int16_t)json_int_or_default(q, "xp", quest->xp);
        quest->level = (LEVEL)json_int_or_default(q, "level", quest->level);
        quest->end = (VNUM)json_int_or_default(q, "end", quest->end);
        quest->target = (VNUM)json_int_or_default(q, "target", quest->target);
        quest->target_upper = (VNUM)json_int_or_default(q, "upper", quest->target_upper);
        quest->amount = (int16_t)json_int_or_default(q, "count", quest->amount);
        quest->reward_faction_vnum = (VNUM)json_int_or_default(q, "rewardFaction", quest->reward_faction_vnum);
        quest->reward_reputation = (int16_t)json_int_or_default(q, "rewardReputation", quest->reward_reputation);
        quest->reward_gold = (int16_t)json_int_or_default(q, "rewardGold", quest->reward_gold);
        quest->reward_silver = (int16_t)json_int_or_default(q, "rewardSilver", quest->reward_silver);
        quest->reward_copper = (int16_t)json_int_or_default(q, "rewardCopper", quest->reward_copper);

        json_t* ro = json_object_get(q, "rewardObjs");
        json_t* rc = json_object_get(q, "rewardCounts");
        if (json_is_array(ro) && json_is_array(rc)) {
            for (int j = 0; j < QUEST_MAX_REWARD_ITEMS; j++) {
                if (j < (int)json_array_size(ro))
                    quest->reward_obj_vnum[j] = (VNUM)json_integer_value(json_array_get(ro, j));
                if (j < (int)json_array_size(rc))
                    quest->reward_obj_count[j] = (int16_t)json_integer_value(json_array_get(rc, j));
            }
        }

        quest->vnum = (VNUM)json_int_or_default(q, "vnum", quest->vnum);
        ORDERED_INSERT(Quest, quest, area->quests, vnum);
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static json_t* build_specials(const AreaData* area)
{
    json_t* arr = json_array();
    MobPrototype* mob;
    FOR_EACH_MOB_PROTO(mob) {
        if (mob && mob->area == area && mob->spec_fun) {
            json_t* obj = json_object();
            json_object_set_new(obj, "mobVnum", json_integer(VNUM_FIELD(mob)));
            json_object_set_new(obj, "spec", json_string(spec_name(mob->spec_fun)));
            json_array_append_new(arr, obj);
        }
    }
    return arr;
}

static PersistResult parse_specials(json_t* root)
{
    json_t* specials = json_object_get(root, "specials");
    if (!json_is_array(specials))
        return (PersistResult){ PERSIST_OK, NULL, -1 };

    size_t count = json_array_size(specials);
    for (size_t i = 0; i < count; i++) {
        json_t* s = json_array_get(specials, i);
        if (!json_is_object(s))
            continue;
        VNUM vnum = (VNUM)json_int_or_default(s, "mobVnum", 0);
        const char* spec = json_string_value(json_object_get(s, "spec"));
        if (!spec)
            continue;
        MobPrototype* mob = get_mob_prototype(vnum);
        if (!mob)
            continue;
        mob->spec_fun = spec_lookup(spec);
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
static json_t* build_reset(const Reset* reset, VNUM room_vnum)
{
    json_t* obj = json_object();
    char cmd[2] = { reset->command, '\0' };

    const char* name = NULL;
    switch (reset->command) {
    case 'M':
        name = "loadMob";
        json_object_set_new(obj, "mobVnum", json_integer(reset->arg1));
        json_object_set_new(obj, "maxInArea", json_integer(reset->arg2));
        json_object_set_new(obj, "roomVnum", json_integer(reset->arg3));
        if (reset->arg4 != 0)
            json_object_set_new(obj, "maxInRoom", json_integer(reset->arg4));
        break;
    case 'O':
        name = "placeObj";
        json_object_set_new(obj, "objVnum", json_integer(reset->arg1));
        json_object_set_new(obj, "roomVnum", json_integer(reset->arg3));
        break;
    case 'P':
        name = "putObj";
        json_object_set_new(obj, "objVnum", json_integer(reset->arg1));
        if (reset->arg2 != 0)
            json_object_set_new(obj, "count", json_integer(reset->arg2));
        json_object_set_new(obj, "containerVnum", json_integer(reset->arg3));
        if (reset->arg4 != 0)
            json_object_set_new(obj, "maxInContainer", json_integer(reset->arg4));
        break;
    case 'G':
        name = "giveObj";
        json_object_set_new(obj, "objVnum", json_integer(reset->arg1));
        break;
    case 'E':
        name = "equipObj";
        json_object_set_new(obj, "objVnum", json_integer(reset->arg1));
        json_object_set_new(obj, "wearLoc", json_string(flag_string(wear_loc_strings, reset->arg3)));
        break;
    case 'D':
        name = "setDoor";
        json_object_set_new(obj, "roomVnum", json_integer(reset->arg1));
        json_object_set_new(obj, "direction", json_string(dir_name_from_enum(reset->arg2)));
        json_object_set_new(obj, "state", json_integer(reset->arg3));
        break;
    case 'R':
        name = "randomizeExits";
        json_object_set_new(obj, "roomVnum", json_integer(reset->arg1));
        json_object_set_new(obj, "exits", json_integer(reset->arg2));
        break;
    default:
        break;
    }

    json_object_set_new(obj, "roomVnum", json_integer(room_vnum));
    if (name)
        json_object_set_new(obj, "commandName", json_string(name));
    else {
        json_object_set_new(obj, "command", json_string(cmd));
        json_object_set_new(obj, "arg1", json_integer(reset->arg1));
        json_object_set_new(obj, "arg2", json_integer(reset->arg2));
        json_object_set_new(obj, "arg3", json_integer(reset->arg3));
        json_object_set_new(obj, "arg4", json_integer(reset->arg4));
    }
    return obj;
}

static json_t* build_resets(const AreaData* area)
{
    json_t* arr = json_array();
    RoomData* room;
    FOR_EACH_GLOBAL_ROOM(room) {
        if (room->area_data != area)
            continue;
        Reset* reset;
        FOR_EACH(reset, room->reset_first) {
            json_array_append_new(arr, build_reset(reset, VNUM_FIELD(room)));
        }
    }
    return arr;
}

static void append_reset_room(RoomData* room, Reset* reset)
{
    reset->next = NULL;
    if (room->reset_first == NULL)
        room->reset_first = reset;
    if (room->reset_last != NULL)
        room->reset_last->next = reset;
    room->reset_last = reset;
}

static PersistResult parse_resets(json_t* root)
{
    json_t* resets = json_object_get(root, "resets");
    if (!json_is_array(resets))
        return (PersistResult){ PERSIST_OK, NULL, -1 };

    VNUM last_room_vnum = VNUM_NONE;
    size_t count = json_array_size(resets);
    for (size_t i = 0; i < count; i++) {
        json_t* r = json_array_get(resets, i);
        if (!json_is_object(r))
            continue;
        const char* cmd_str = json_string_value(json_object_get(r, "command"));
        const char* cmd_name = json_string_value(json_object_get(r, "commandName"));
        char cmd = 0;
        if (cmd_str && strlen(cmd_str) == 1)
            cmd = cmd_str[0];
        else if (cmd_name) {
            if (!str_cmp(cmd_name, "loadMob")) cmd = 'M';
            else if (!str_cmp(cmd_name, "placeObj")) cmd = 'O';
            else if (!str_cmp(cmd_name, "putObj")) cmd = 'P';
            else if (!str_cmp(cmd_name, "giveObj")) cmd = 'G';
            else if (!str_cmp(cmd_name, "equipObj")) cmd = 'E';
            else if (!str_cmp(cmd_name, "setDoor")) cmd = 'D';
            else if (!str_cmp(cmd_name, "randomizeExits")) cmd = 'R';
        }
        if (cmd == 0)
            continue;

        Reset* reset = new_reset();
        reset->command = cmd;

        switch (cmd) {
        case 'M':
            reset->arg1 = (int16_t)json_int_or_default(r, "mobVnum", 0);
            reset->arg2 = (int16_t)json_int_or_default(r, "maxInArea", 0);
            reset->arg3 = (int16_t)json_int_or_default(r, "roomVnum", 0);
            reset->arg4 = (int16_t)json_int_or_default(r, "maxInRoom", 0);
            if (reset->arg3 != 0)
                last_room_vnum = reset->arg3;
            break;
        case 'O':
            reset->arg1 = (int16_t)json_int_or_default(r, "objVnum", 0);
            reset->arg3 = (int16_t)json_int_or_default(r, "roomVnum", 0);
            if (reset->arg3 != 0)
                last_room_vnum = reset->arg3;
            break;
        case 'P':
            reset->arg1 = (int16_t)json_int_or_default(r, "objVnum", 0);
            reset->arg2 = (int16_t)json_int_or_default(r, "count", 0);
            reset->arg3 = (int16_t)json_int_or_default(r, "containerVnum", 0);
            reset->arg4 = (int16_t)json_int_or_default(r, "maxInContainer", reset->arg4);
            break;
        case 'G':
            reset->arg1 = (int16_t)json_int_or_default(r, "objVnum", 0);
            break;
        case 'E':
            reset->arg1 = (int16_t)json_int_or_default(r, "objVnum", 0);
            {
                const char* wear = json_string_value(json_object_get(r, "wearLoc"));
                if (wear) {
                    FLAGS wl = flag_lookup(wear, wear_loc_strings);
                    if (wl != NO_FLAG)
                        reset->arg3 = (int16_t)wl;
                }
            }
            break;
        case 'D':
            reset->arg1 = (int16_t)json_int_or_default(r, "roomVnum", 0);
            {
                const char* dir = json_string_value(json_object_get(r, "direction"));
                int d = dir_enum_from_name(dir);
                if (d >= 0)
                    reset->arg2 = (int16_t)d;
            }
            reset->arg3 = (int16_t)json_int_or_default(r, "state", 0);
            break;
        case 'R':
            reset->arg1 = (int16_t)json_int_or_default(r, "roomVnum", 0);
            reset->arg2 = (int16_t)json_int_or_default(r, "exits", 0);
            if (reset->arg1 != 0)
                last_room_vnum = reset->arg1;
            break;
        default:
            reset->arg1 = (int16_t)json_int_or_default(r, "arg1", 0);
            reset->arg2 = (int16_t)json_int_or_default(r, "arg2", 0);
            reset->arg3 = (int16_t)json_int_or_default(r, "arg3", 0);
            reset->arg4 = (int16_t)json_int_or_default(r, "arg4", 0);
            break;
        }

        VNUM room_vnum = (VNUM)json_int_or_default(r, "roomVnum", VNUM_NONE);
        VNUM vnum = room_vnum;
        if (vnum == VNUM_NONE) {
            switch (reset->command) {
            case 'M':
            case 'O':
                vnum = reset->arg3;
                break;
            case 'D':
            case 'R':
                vnum = reset->arg1;
                break;
            default:
                break;
            }
        }
        if (vnum == VNUM_NONE && last_room_vnum != VNUM_NONE) {
            vnum = last_room_vnum;
        }
        RoomData* room = (vnum != VNUM_NONE) ? get_room_data(vnum) : NULL;
        if (room)
            append_reset_room(room, reset);
        else
            free_reset(reset);
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static void ensure_help_area(AreaData* area)
{
    if (area->helps)
        return;
    HelpArea* ha = new_help_area();
    ha->area_data = area;
    ha->filename = str_dup(area->file_name ? area->file_name : "area.json");
    ha->first = ha->last = NULL;
    ha->changed = false;
    ha->next = help_area_list;
    help_area_list = ha;
    area->helps = ha;
}

static void append_help(HelpArea* ha, HelpData* help)
{
    if (!ha->first)
        ha->first = help;
    if (ha->last)
        ha->last->next_area = help;
    ha->last = help;
    help->next_area = NULL;

    if (!help_first)
        help_first = help;
    if (help_last)
        help_last->next = help;
    help_last = help;
    help->next = NULL;
}

static PersistResult parse_helps(json_t* root, AreaData* area)
{
    json_t* helps = json_object_get(root, "helps");
    if (!json_is_array(helps) || json_array_size(helps) == 0)
        return (PersistResult){ PERSIST_OK, NULL, -1 };

    ensure_help_area(area);
    HelpArea* ha = area->helps;

    size_t count = json_array_size(helps);
    for (size_t i = 0; i < count; i++) {
        json_t* h = json_array_get(helps, i);
        if (!json_is_object(h))
            continue;
        HelpData* help = new_help_data();
        help->level = (LEVEL)json_int_or_default(h, "level", 0);
        help->keyword = str_dup(json_string_value(json_object_get(h, "keyword")));
        help->text = str_dup(json_string_value(json_object_get(h, "text")));
        append_help(ha, help);
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static json_t* build_int_array(ValueArray* arr)
{
    json_t* out = json_array();
    if (!arr)
        return out;
    for (int i = 0; i < arr->count; i++) {
        Value v = arr->values[i];
        if (IS_INT(v))
            json_array_append_new(out, json_integer(AS_INT(v)));
    }
    return out;
}

static json_t* build_factions(const AreaData* area)
{
    json_t* arr = json_array();
    if (faction_table.capacity == 0 || faction_table.entries == NULL)
        return arr;
    for (int idx = 0; idx < faction_table.capacity; ++idx) {
        Entry* entry = &faction_table.entries[idx];
        if (IS_NIL(entry->value) || !IS_FACTION(entry->value))
            continue;
        Faction* faction = AS_FACTION(entry->value);
        if (faction->area != area)
            continue;
        json_t* obj = json_object();
        json_object_set_new(obj, "vnum", json_integer(VNUM_FIELD(faction)));
        json_object_set_new(obj, "name", json_string(NAME_STR(faction)));
        json_object_set_new(obj, "defaultStanding", json_integer(faction->default_standing));
        json_object_set_new(obj, "allies", build_int_array(&faction->allies));
        json_object_set_new(obj, "opposing", build_int_array(&faction->enemies));
        json_array_append_new(arr, obj);
    }
    return arr;
}

static PersistResult parse_factions(json_t* root)
{
    json_t* factions = json_object_get(root, "factions");
    if (!json_is_array(factions))
        return (PersistResult){ PERSIST_OK, NULL, -1 };

    size_t count = json_array_size(factions);
    for (size_t i = 0; i < count; i++) {
        json_t* f = json_array_get(factions, i);
        if (!json_is_object(f))
            continue;
        VNUM vnum = (VNUM)json_int_or_default(f, "vnum", VNUM_NONE);
        if (vnum == VNUM_NONE)
            continue;
        Faction* faction = faction_create(vnum);
        if (!faction)
            continue;

        const char* name = json_string_value(json_object_get(f, "name"));
        if (name)
            SET_NAME(faction, lox_string(name));
        faction->default_standing = (int)json_int_or_default(f, "defaultStanding", faction->default_standing);

        json_t* allies = json_object_get(f, "allies");
        if (json_is_array(allies)) {
            size_t asz = json_array_size(allies);
            for (size_t j = 0; j < asz; j++) {
                if (json_is_integer(json_array_get(allies, j)))
                    faction_add_ally(faction, (VNUM)json_integer_value(json_array_get(allies, j)));
            }
        }
        json_t* enemies = json_object_get(f, "opposing");
        if (json_is_array(enemies)) {
            size_t esz = json_array_size(enemies);
            for (size_t j = 0; j < esz; j++) {
                if (json_is_integer(json_array_get(enemies, j)))
                    faction_add_enemy(faction, (VNUM)json_integer_value(json_array_get(enemies, j)));
            }
        }
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static json_t* build_helps(const AreaData* area)
{
    json_t* arr = json_array();
    if (!area->helps || !area->helps->first)
        return arr;
    for (HelpData* h = area->helps->first; h != NULL; h = h->next_area) {
        json_t* obj = json_object();
        json_object_set_new(obj, "level", json_integer(h->level));
        json_object_set_new(obj, "keyword", json_string(h->keyword));
        json_object_set_new(obj, "text", json_string(h->text));
        json_array_append_new(arr, obj);
    }
    return arr;
}

PersistResult json_load(const AreaPersistLoadParams* params)
{
    if (!params || !params->reader)
        return json_not_supported("JSON area load: missing reader");

    ReaderBuffer buf = { 0 };
    if (!reader_fill_buffer(params->reader, &buf))
        return (PersistResult){ PERSIST_ERR_IO, "JSON area load: failed to read", -1 };

    json_error_t error;
    json_t* root = json_loadb(buf.data, buf.len, 0, &error);
    free(buf.data);
    if (!root) {
        static char msg[256];
        snprintf(msg, sizeof(msg), "JSON parse error at line %d: %s", error.line, error.text);
        return (PersistResult){ PERSIST_ERR_FORMAT, msg, (int)error.line };
    }

    json_t* fmtv = json_object_get(root, "formatVersion");
    if (json_is_integer(fmtv)) {
        int version = (int)json_integer_value(fmtv);
        if (version != AREA_JSON_FORMAT_VERSION) {
            json_decref(root);
            return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "JSON area load: unsupported formatVersion", -1 };
        }
    }

    PersistResult res = parse_areadata(root, params);
    if (area_persist_succeeded(res))
        res = parse_factions(root);
    if (area_persist_succeeded(res))
        res = parse_rooms(root, current_area_data);
    if (area_persist_succeeded(res))
        res = parse_mobiles(root, current_area_data);
    if (area_persist_succeeded(res))
        res = parse_objects(root, current_area_data);
    if (area_persist_succeeded(res))
        res = parse_shops(root);
    if (area_persist_succeeded(res))
        res = parse_specials(root);
    if (area_persist_succeeded(res))
        res = parse_mobprogs(root);
    if (area_persist_succeeded(res))
        res = parse_quests(root, current_area_data);
    if (area_persist_succeeded(res))
        res = parse_resets(root);
    if (area_persist_succeeded(res))
        res = parse_helps(root, current_area_data);
    if (area_persist_succeeded(res)
        && params->create_single_instance
        && current_area_data
        && current_area_data->inst_type == AREA_INST_SINGLE) {
        create_area_instance(current_area_data, false);
    }
    json_decref(root);
    return res;
}

PersistResult json_save(const AreaPersistSaveParams* params)
{
    if (!params || !params->writer || !params->area)
        return json_not_supported("JSON area save: missing params");

    json_t* root = json_object();
    json_object_set_new(root, "formatVersion", json_integer(AREA_JSON_FORMAT_VERSION));
    json_object_set_new(root, "areadata", build_areadata(params->area));
    json_object_set_new(root, "storyBeats", build_story_beats(params->area));
    json_object_set_new(root, "checklist", build_checklist(params->area));
    json_object_set_new(root, "rooms", build_rooms(params->area));
    json_object_set_new(root, "mobiles", build_mobiles(params->area));
    json_object_set_new(root, "objects", build_objects(params->area));
    json_object_set_new(root, "shops", build_shops(params->area));
    json_object_set_new(root, "specials", build_specials(params->area));
    json_object_set_new(root, "mobprogs", build_mobprogs(params->area));
    json_object_set_new(root, "resets", build_resets(params->area));
    json_object_set_new(root, "factions", build_factions(params->area));
    json_t* helps = build_helps(params->area);
    if (json_array_size(helps) > 0)
        json_object_set_new(root, "helps", helps);
    else
        json_decref(helps);
    json_object_set_new(root, "quests", build_quests(params->area));

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "JSON area save: dump failed", -1 };

    size_t len = strlen(dump);
    bool ok = writer_write_all(params->writer, dump, len);
    free(dump);
    if (!ok)
        return (PersistResult){ PERSIST_ERR_IO, "JSON area save: write failed", -1 };

    if (params->writer->ops->flush)
        params->writer->ops->flush(params->writer->ctx);

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
