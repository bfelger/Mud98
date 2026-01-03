////////////////////////////////////////////////////////////////////////////////
// persist/json/area_persist_json.c
// Stub JSON backend using jansson (gated by ENABLE_JSON_AREAS).
////////////////////////////////////////////////////////////////////////////////

#include "area_persist_json.h"

#include <persist/json/persist_json.h>
#include <persist/loot/json/loot_persist_json.h>

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
#include <comm.h>
#include <mob_prog.h>
#include <db.h>

#include <jansson.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define AREA_JSON_FORMAT_VERSION 1

const AreaPersistFormat AREA_PERSIST_JSON = {
    .name = "json",
    .load = json_load,
    .save = json_save,
};

static void json_set_flags_impl(json_t* obj, const char* key, FLAGS flags,
    const struct flag_type* table, const struct flag_type* defaults)
{
    json_t* arr = defaults ? flags_to_array_with_defaults(flags, defaults, table)
                           : flags_to_array(flags, table);
    if (arr && json_array_size(arr) > 0)
        json_object_set_new(obj, key, arr);
    else
        json_decref(arr);
}

static void json_set_flags_if(json_t* obj, const char* key, FLAGS flags, const struct flag_type* table)
{
    json_set_flags_impl(obj, key, flags, table, NULL);
}

static void json_set_int_if(json_t* obj, const char* key, int64_t value, int64_t def)
{
    if (value != def)
        JSON_SET_INT(obj, key, value);
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

static bool sector_value_is_known(Sector value)
{
    return value >= SECT_INSIDE && value <= SECT_DESERT;
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
            JSON_SET_STRING(obj, "trigger", trig_name);
        else
            JSON_SET_INT(obj, "triggerValue", ev->trigger);
        if (ev->method_name && ev->method_name->chars)
            JSON_SET_STRING(obj, "callback", ev->method_name->chars);
        if (IS_INT(ev->criteria))
            JSON_SET_INT(obj, "criteria", AS_INT(ev->criteria));
        else if (IS_STRING(ev->criteria))
            JSON_SET_STRING(obj, "criteria", AS_STRING(ev->criteria)->chars);
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
        const char* trig_name = JSON_STRING(e, "trigger");
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

        const char* cb = JSON_STRING(e, "callback");
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
    JSON_SET_INT(obj, "version", AREA_VERSION);
    JSON_SET_STRING(obj, "name", NAME_STR(area));
    JSON_SET_STRING(obj, "builders", area->builders ? area->builders : "");
    json_t* vnums = json_array();
    json_array_append_new(vnums, json_integer(area->min_vnum));
    json_array_append_new(vnums, json_integer(area->max_vnum));
    json_object_set_new(obj, "vnumRange", vnums);
    JSON_SET_STRING(obj, "credits", area->credits ? area->credits : "");
    JSON_SET_INT(obj, "security", area->security);
    const char* sector_name = flag_string(sector_flag_table, area->sector);
    if (sector_name && sector_name[0] != '\0')
        JSON_SET_STRING(obj, "sector", sector_name);
    JSON_SET_INT(obj, "lowLevel", area->low_range);
    JSON_SET_INT(obj, "highLevel", area->high_range);
    JSON_SET_INT(obj, "reset", area->reset_thresh);
    json_object_set_new(obj, "alwaysReset", json_boolean(area->always_reset));
    if (area->inst_type == AREA_INST_MULTI)
        JSON_SET_STRING(obj, "instType", "multi");
    if (!IS_NULLSTR(area->loot_table))
        JSON_SET_STRING(obj, "lootTable", area->loot_table);
    return obj;
}

static json_t* build_story_beats(const AreaData* area)
{
    json_t* arr = json_array();
    for (StoryBeat* beat = area->story_beats; beat != NULL; beat = beat->next) {
        json_t* obj = json_object();
        if (beat->title)
            JSON_SET_STRING(obj, "title", beat->title);
        if (beat->description)
            JSON_SET_STRING(obj, "description", beat->description);
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
            JSON_SET_STRING(obj, "title", item->title);
        if (item->description && item->description[0] != '\0')
            JSON_SET_STRING(obj, "description", item->description);
        const char* status = checklist_status_name(item->status);
        if (status)
            JSON_SET_STRING(obj, "status", status);
        else
            JSON_SET_INT(obj, "statusValue", item->status);
        json_array_append_new(arr, obj);
    }
    return arr;
}

static json_t* build_exit(const RoomExitData* ex)
{
    json_t* obj = json_object();
    const char* dir = dir_name_from_enum(ex->orig_dir);
    if (dir)
        JSON_SET_STRING(obj, "dir", dir);
    JSON_SET_INT(obj, "toVnum", ex->to_vnum);
    if (ex->key > 0)
        JSON_SET_INT(obj, "key", ex->key);
    json_set_flags_if(obj, "flags", ex->exit_reset_flags, exit_flag_table);
    if (ex->description && ex->description[0] != '\0')
        JSON_SET_STRING(obj, "description", ex->description);
    if (ex->keyword && ex->keyword[0] != '\0')
        JSON_SET_STRING(obj, "keyword", ex->keyword);
    return obj;
}

static json_t* build_extra_descs(ExtraDesc* ed_list)
{
    json_t* arr = json_array();
    for (ExtraDesc* ed = ed_list; ed != NULL; ed = ed->next) {
        json_t* obj = json_object();
        JSON_SET_STRING(obj, "keyword", ed->keyword);
        JSON_SET_STRING(obj, "description", ed->description);
        json_array_append_new(arr, obj);
    }
    return arr;
}

static json_t* build_time_periods(const DayCyclePeriod* head, bool include_description)
{
    json_t* arr = json_array();
    for (const DayCyclePeriod* period = head; period != NULL; period = period->next) {
        json_t* obj = json_object();
        JSON_SET_STRING(obj, "name", period->name ? period->name : "");
        JSON_SET_INT(obj, "fromHour", period->start_hour);
        JSON_SET_INT(obj, "toHour", period->end_hour);
        if (include_description && period->description && period->description[0] != '\0')
            JSON_SET_STRING(obj, "description", period->description);
        if (period->enter_message && period->enter_message[0] != '\0')
            JSON_SET_STRING(obj, "enterMessage", period->enter_message);
        if (period->exit_message && period->exit_message[0] != '\0')
            JSON_SET_STRING(obj, "exitMessage", period->exit_message);
        json_array_append_new(arr, obj);
    }
    return arr;
}

static json_t* build_room_periods(const RoomData* room)
{
    if (!room)
        return json_array();
    return build_time_periods(room->periods, true);
}

static json_t* build_area_periods(const AreaData* area)
{
    if (!area)
        return json_array();
    return build_time_periods(area->periods, false);
}

static void parse_area_periods(AreaData* area, json_t* periods);

static json_t* build_area_daycycle(const AreaData* area)
{
    if (!area)
        return NULL;

    json_t* periods = build_area_periods(area);
    size_t period_count = json_array_size(periods);
    bool has_flag = area->suppress_daycycle_messages;
    if (!has_flag && period_count == 0) {
        json_decref(periods);
        return NULL;
    }

    json_t* obj = json_object();
    if (area->suppress_daycycle_messages)
        json_object_set_new(obj, "suppressDaycycleMessages", json_true());
    if (period_count > 0)
        json_object_set_new(obj, "periods", periods);
    else
        json_decref(periods);
    return obj;
}

static json_t* build_rooms(const AreaData* area)
{
    json_t* arr = json_array();
    FLAGS known_room_mask = 0;
    for (int i = 0; room_flag_table[i].name != NULL; ++i)
        known_room_mask |= room_flag_table[i].bit;

    RoomData* room_data;
    FOR_EACH_GLOBAL_ROOM(room_data) {
        if (room_data->area_data != area)
            continue;
        json_t* obj = json_object();
        JSON_SET_INT(obj, "vnum", VNUM_FIELD(room_data));
        JSON_SET_STRING(obj, "name", NAME_STR(room_data));
        JSON_SET_STRING(obj, "description", room_data->description ? room_data->description : "");
        FLAGS room_flags = room_data->room_flags;
        json_set_flags_if(obj, "roomFlags", room_flags, room_flag_table);
        FLAGS unknown_room_bits = room_flags & ~known_room_mask;
        if (unknown_room_bits != 0)
            JSON_SET_INT(obj, "roomFlagsValue", room_flags);
        const char* sector_name = flag_string(sector_flag_table, room_data->sector_type);
        if (!sector_value_is_known(room_data->sector_type)) {
            fprintf(stderr, "json_save_area: room %"PRVNUM" in %s has unknown sector %d\n",
                VNUM_FIELD(room_data),
                area && area->file_name ? area->file_name : "<unknown>",
                room_data->sector_type);
        }
        if (sector_name && sector_name[0] != '\0')
            JSON_SET_STRING(obj, "sectorType", sector_name);
        if (room_data->mana_rate != 100)
            JSON_SET_INT(obj, "manaRate", room_data->mana_rate);
        if (room_data->heal_rate != 100)
            JSON_SET_INT(obj, "healRate", room_data->heal_rate);
        if (room_data->clan > 0)
            JSON_SET_INT(obj, "clan", room_data->clan);
        if (room_data->owner && room_data->owner[0] != '\0')
            JSON_SET_STRING(obj, "owner", room_data->owner);
        if (room_data->suppress_daycycle_messages)
            json_object_set_new(obj, "suppressDaycycleMessages", json_true());
        json_t* periods = build_room_periods(room_data);
        if (json_array_size(periods) > 0)
            json_object_set_new(obj, "timePeriods", periods);
        else
            json_decref(periods);

        json_t* exits = json_array();
        for (int dir = 0; dir < DIR_MAX; dir++) {
            if (room_data->exit_data[dir])
                json_array_append_new(exits, build_exit(room_data->exit_data[dir]));
        }
        if (json_array_size(exits) > 0)
            json_object_set_new(obj, "exits", exits);
        else
            json_decref(exits);

        if (room_data->extra_desc)
            json_object_set_new(obj, "extraDescs", build_extra_descs(room_data->extra_desc));

        Entity* ent = (Entity*)room_data;
        if (ent->script && ent->script->chars && ent->script->length > 0)
            JSON_SET_STRING(obj, "loxScript", ent->script->chars);
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
        const char* title = JSON_STRING(sb, "title");
        const char* desc = JSON_STRING(sb, "description");
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
        const char* title = JSON_STRING(it, "title");
        const char* desc = JSON_STRING(it, "description");
        const char* status_name = JSON_STRING(it, "status");
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
    area->file_name = boot_intern_string(params->file_name ? params->file_name : "area.json");

    const char* name = JSON_STRING(areadata, "name");
    if (name)
        SET_NAME(area, lox_string(name));

    const char* builders = JSON_STRING(areadata, "builders");
    JSON_INTERN(builders, area->builders)

    json_t* vnums = json_object_get(areadata, "vnumRange");
    if (json_is_array(vnums) && json_array_size(vnums) >= 2) {
        area->min_vnum = (VNUM)json_integer_value(json_array_get(vnums, 0));
        area->max_vnum = (VNUM)json_integer_value(json_array_get(vnums, 1));
    }

    const char* credits = JSON_STRING(areadata, "credits");
    JSON_INTERN(credits, area->credits)

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
    area->suppress_daycycle_messages = json_bool_or_default(areadata, "suppressDaycycleMessages", area->suppress_daycycle_messages);
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

    const char* loot_table = JSON_STRING(areadata, "lootTable");
    JSON_INTERN(loot_table, area->loot_table)

    parse_story_beats(json_object_get(root, "storyBeats"), area);
    parse_checklist(json_object_get(root, "checklist"), area);
    json_t* daycycle = json_object_get(root, "daycycle");
    if (json_is_object(daycycle)) {
        area->suppress_daycycle_messages = json_bool_or_default(daycycle, "suppressDaycycleMessages", area->suppress_daycycle_messages);
        parse_area_periods(area, json_object_get(daycycle, "periods"));
    }

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
        const char* dir_name = JSON_STRING(ex, "dir");
        int dir = dir_enum_from_name(dir_name);
        if (dir < 0 || dir >= DIR_MAX)
            continue;

        RoomExitData* ex_data = new_room_exit_data();
        ex_data->orig_dir = dir;
        ex_data->to_vnum = (VNUM)json_int_or_default(ex, "toVnum", 0);
        ex_data->key = (int16_t)json_int_or_default(ex, "key", -1);
        if (ex_data->key == 0) {
            fprintf(stderr, "json_load_area: room %"PRVNUM" exit %s in %s has key vnum 0; treating as -1\n",
                VNUM_FIELD(room), dir_name ? dir_name : "?",
                room->area_data && room->area_data->file_name ? room->area_data->file_name : "<unknown>");
            ex_data->key = -1;
        }
        ex_data->exit_reset_flags = (SHORT_FLAGS)JSON_FLAGS(ex, "flags", exit_flag_table);

        const char* desc = JSON_STRING(ex, "description");
        ex_data->description = desc ? boot_intern_string(desc) : &str_empty[0];
        const char* kw = JSON_STRING(ex, "keyword");
        ex_data->keyword = kw ? boot_intern_string(kw) : &str_empty[0];

        room->exit_data[dir] = ex_data;
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static void parse_room_periods(RoomData* room, json_t* periods)
{
    if (!room || !json_is_array(periods))
        return;

    size_t count = json_array_size(periods);
    for (size_t i = 0; i < count; i++) {
        json_t* entry = json_array_get(periods, i);
        if (!json_is_object(entry))
            continue;

        const char* name = JSON_STRING(entry, "name");
        int start = (int)json_int_or_default(entry, "fromHour", 0);
        int end = (int)json_int_or_default(entry, "toHour", start);
        DayCyclePeriod* period = room_daycycle_period_add(room, name ? name : "", start, end);
        if (!period)
            continue;

        free_string(period->name);
        period->name = boot_intern_string(name ? name : "");
        const char* desc = JSON_STRING(entry, "description");
        if (desc) {
            free_string(period->description);
            period->description = boot_intern_string(desc);
        }
        const char* enter_msg = JSON_STRING(entry, "enterMessage");
        if (enter_msg) {
            free_string(period->enter_message);
            period->enter_message = boot_intern_string(enter_msg);
        }
        const char* exit_msg = JSON_STRING(entry, "exitMessage");
        if (exit_msg) {
            free_string(period->exit_message);
            period->exit_message = boot_intern_string(exit_msg);
        }
    }
}

static void parse_area_periods(AreaData* area, json_t* periods)
{
    if (!area || !json_is_array(periods))
        return;

    size_t count = json_array_size(periods);
    for (size_t i = 0; i < count; i++) {
        json_t* entry = json_array_get(periods, i);
        if (!json_is_object(entry))
            continue;

        const char* name = JSON_STRING(entry, "name");
        int start = (int)json_int_or_default(entry, "fromHour", 0);
        int end = (int)json_int_or_default(entry, "toHour", start);
        DayCyclePeriod* period = area_daycycle_period_add(area, name ? name : "", start, end);
        if (!period)
            continue;

        free_string(period->name);
        period->name = boot_intern_string(name ? name : "");
        const char* desc = JSON_STRING(entry, "description");
        if (desc) {
            free_string(period->description);
            period->description = boot_intern_string(desc);
        }
        const char* enter_msg = JSON_STRING(entry, "enterMessage");
        if (enter_msg) {
            free_string(period->enter_message);
            period->enter_message = boot_intern_string(enter_msg);
        }
        const char* exit_msg = JSON_STRING(entry, "exitMessage");
        if (exit_msg) {
            free_string(period->exit_message);
            period->exit_message = boot_intern_string(exit_msg);
        }
    }
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

        VNUM vnum = (VNUM)json_int_or_default(r, "vnum", VNUM_NONE);
        VNUM_FIELD(room) = vnum;

        const char* name = JSON_STRING(r, "name");
        if (name)
            SET_NAME(room, lox_string(name));
        const char* desc = JSON_STRING(r, "description");
        JSON_INTERN(desc, room->description)

        room->room_flags = JSON_FLAGS(r, "roomFlags", room_flag_table);
        room->room_flags = (FLAGS)json_int_or_default(r, "roomFlagsValue", room->room_flags);
        json_t* sector_val = json_object_get(r, "sectorType");
        if (json_is_string(sector_val)) {
            FLAGS s = flag_lookup(json_string_value(sector_val), sector_flag_table);
            if (s != NO_FLAG)
                room->sector_type = (Sector)s;
        }
        else
            room->sector_type = (Sector)json_int_or_default(r, "sectorType", room->sector_type);
        room->mana_rate = (int16_t)json_int_or_default(r, "manaRate", room->mana_rate);
        room->heal_rate = (int16_t)json_int_or_default(r, "healRate", room->heal_rate);
        room->clan = (int16_t)json_int_or_default(r, "clan", room->clan);
        const char* owner = JSON_STRING(r, "owner");
        JSON_INTERN(owner, room->owner)
        bool suppress = json_bool_or_default(r, "suppressDaycycleMessages", room->suppress_daycycle_messages);
        suppress = json_bool_or_default(r, "suppressWeatherMessages", suppress);
        room->suppress_daycycle_messages = suppress;
        parse_room_periods(room, json_object_get(r, "timePeriods"));

        global_room_set(room);
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
                ed->keyword = boot_intern_string(JSON_STRING(e, "keyword"));
                ed->description = boot_intern_string(JSON_STRING(e, "description"));
                ADD_EXTRA_DESC(room, ed)
            }
        }

        const char* script = JSON_STRING(r, "loxScript");
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
        JSON_SET_STRING(obj, "type", skill_table[af->type].name);
    else if (af->type != -1)
        JSON_SET_INT(obj, "type", af->type);

    const char* where = flag_string(apply_types, af->where);
    if (where && where[0] != '\0')
        JSON_SET_STRING(obj, "where", where);
    const char* loc_name = flag_string(apply_flag_table, af->location);
    if (loc_name && loc_name[0] != '\0')
        JSON_SET_STRING(obj, "location", loc_name);
    else
        JSON_SET_INT(obj, "location", af->location);

    JSON_SET_INT(obj, "level", af->level);
    JSON_SET_INT(obj, "duration", af->duration);
    JSON_SET_INT(obj, "modifier", af->modifier);

    const struct bit_type* bv_type = &bitvector_type[af->where];
    if (bv_type && bv_type->table) {
        json_t* bits = json_array();
        FLAGS value = af->bitvector;
        for (int i = 0; bv_type->table[i].name != NULL; ++i) {
            if (IS_SET(value, bv_type->table[i].bit)) {
                json_array_append_new(bits, json_string(bv_type->table[i].name));
                REMOVE_BIT(value, bv_type->table[i].bit);
            }
        }
        if (json_array_size(bits) > 0)
            json_object_set_new(obj, "bitvector", bits);
        else
            json_decref(bits);
        if (value != 0)
            JSON_SET_INT(obj, "bitvectorValue", value);
    }
    else if (af->bitvector != 0) {
        JSON_SET_INT(obj, "bitvectorValue", af->bitvector);
    }

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

    Affect* new_head = NULL;
    Affect** new_tail = &new_head;

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

        const char* where = JSON_STRING(a, "where");
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
        FLAGS bv_value = 0;
        if (bv && bv_type && bv_type->table)
            bv_value = flags_from_array(bv, bv_type->table);
        FLAGS bv_extra = (FLAGS)json_int_or_default(a, "bitvectorValue", 0);
        af->bitvector = (int)(bv_value | bv_extra);

        af->next = NULL;
        *new_tail = af;
        new_tail = &af->next;
    }

    if (new_head == NULL)
        return;

    if (*out_list == NULL)
        *out_list = new_head;
    else {
        Affect* tail = *out_list;
        while (tail->next)
            tail = tail->next;
        tail->next = new_head;
    }
}

static json_t* dice_to_json(const int16_t dice[3])
{
    json_t* obj = json_object();
    JSON_SET_INT(obj, "number", dice[DICE_NUMBER]);
    JSON_SET_INT(obj, "type", dice[DICE_TYPE]);
    if (dice[DICE_BONUS] != 0)
        JSON_SET_INT(obj, "bonus", dice[DICE_BONUS]);
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
    if (sn <= 0)
        return NULL;
    if (sn < skill_count && skill_table[sn].name && skill_table[sn].name[0] != '\0')
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
    MobPrototype* mob;
    FOR_EACH_MOB_PROTO(mob) {
        if (mob->area != area)
            continue;

        json_t* obj = json_object();
        JSON_SET_INT(obj, "vnum", VNUM_FIELD(mob));
        JSON_SET_STRING(obj, "name", NAME_STR(mob));
        JSON_SET_STRING(obj, "shortDescr", mob->short_descr);
        JSON_SET_STRING(obj, "longDescr", mob->long_descr);
        JSON_SET_STRING(obj, "description", mob->description ? mob->description : "");
        JSON_SET_STRING(obj, "race", race_table[mob->race].name);

        const Race* mob_race = (mob->race >= 0 && mob->race < race_count) ? &race_table[mob->race] : NULL;
        FLAGS race_form = mob_race ? mob_race->form : 0;
        FLAGS race_parts = mob_race ? mob_race->parts : 0;

        FLAGS act_flags = mob->act_flags;
        json_set_flags_if(obj, "actFlags", act_flags, act_flag_table);
        FLAGS known_act_mask = 0;
        for (int i = 0; act_flag_table[i].name != NULL; ++i)
            known_act_mask |= act_flag_table[i].bit;
        if ((act_flags & ~known_act_mask) != 0)
            JSON_SET_INT(obj, "actFlagsValue", act_flags);
        json_set_flags_if(obj, "affectFlags", mob->affect_flags, affect_flag_table);
        json_set_flags_if(obj, "atkFlags", mob->atk_flags, off_flag_table);
        json_set_flags_if(obj, "immFlags", mob->imm_flags, imm_flag_table);
        json_set_flags_if(obj, "resFlags", mob->res_flags, res_flag_table);
        json_set_flags_if(obj, "vulnFlags", mob->vuln_flags, vuln_flag_table);
        if (!mob_race || mob->form != race_form)
            json_set_flags_impl(obj, "formFlags", mob->form, form_flag_table, form_defaults_flag_table);
        if (!mob_race || mob->parts != race_parts)
            json_set_flags_impl(obj, "partFlags", mob->parts, part_flag_table, part_defaults_flag_table);

        JSON_SET_INT(obj, "alignment", mob->alignment);
        JSON_SET_INT(obj, "group", mob->group);
        JSON_SET_INT(obj, "level", mob->level);
        JSON_SET_INT(obj, "hitroll", mob->hitroll);
        json_object_set_new(obj, "hitDice", dice_to_json(mob->hit));
        json_object_set_new(obj, "manaDice", dice_to_json(mob->mana));
        json_object_set_new(obj, "damageDice", dice_to_json(mob->damage));
        JSON_SET_STRING(obj, "damType", attack_table[mob->dam_type].name);

        json_t* ac = json_object();
        JSON_SET_INT(ac, "pierce", mob->ac[AC_PIERCE] / 10);
        JSON_SET_INT(ac, "bash", mob->ac[AC_BASH] / 10);
        JSON_SET_INT(ac, "slash", mob->ac[AC_SLASH] / 10);
        JSON_SET_INT(ac, "exotic", mob->ac[AC_EXOTIC] / 10);
        json_object_set_new(obj, "ac", ac);

        JSON_SET_STRING(obj, "startPos", position_name(mob->start_pos));
        JSON_SET_STRING(obj, "defaultPos", position_name(mob->default_pos));
        JSON_SET_STRING(obj, "sex", sex_name(mob->sex));
        JSON_SET_INT(obj, "wealth", mob->wealth);
        JSON_SET_STRING(obj, "size", size_name(mob->size));
        JSON_SET_STRING(obj, "material", mob->material ? mob->material : "");
        if (mob->faction_vnum != 0)
            JSON_SET_INT(obj, "factionVnum", mob->faction_vnum);
        if (!IS_NULLSTR(mob->loot_table))
            JSON_SET_STRING(obj, "lootTable", mob->loot_table);
        if (mob->craft_mats != NULL && mob->craft_mat_count > 0) {
            json_t* mats = json_array();
            for (int i = 0; i < mob->craft_mat_count; i++) {
                json_array_append_new(mats, json_integer(mob->craft_mats[i]));
            }
            json_object_set_new(obj, "craftMats", mats);
        }

        Entity* ent = (Entity*)mob;
        if (ent->script && ent->script->chars && ent->script->length > 0)
            JSON_SET_STRING(obj, "loxScript", ent->script->chars);
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

        VNUM vnum = (VNUM)json_int_or_default(m, "vnum", VNUM_NONE);
        VNUM_FIELD(mob) = vnum;

        const char* name = JSON_STRING(m, "name");
        if (name)
            SET_NAME(mob, lox_string(name));
        const char* sd = JSON_STRING(m, "shortDescr");
        JSON_INTERN(sd, mob->short_descr)
        const char* ld = JSON_STRING(m, "longDescr");
        JSON_INTERN(ld, mob->long_descr)
        const char* desc = JSON_STRING(m, "description");
        JSON_INTERN(desc, mob->description)

        const char* race_name = JSON_STRING(m, "race");
        if (race_name) {
            int r = race_lookup(race_name);
            if (r >= 0)
                mob->race = (int16_t)r;
        }

        mob->act_flags = JSON_FLAGS(m, "actFlags", act_flag_table);
        mob->act_flags = (FLAGS)json_int_or_default(m, "actFlagsValue", mob->act_flags);
        mob->affect_flags = JSON_FLAGS(m, "affectFlags", affect_flag_table);
        mob->atk_flags = JSON_FLAGS(m, "atkFlags", off_flag_table);
        mob->imm_flags = JSON_FLAGS(m, "immFlags", imm_flag_table);
        mob->res_flags = JSON_FLAGS(m, "resFlags", res_flag_table);
        mob->vuln_flags = JSON_FLAGS(m, "vulnFlags", vuln_flag_table);
        const Race* mob_race = (mob->race >= 0 && mob->race < race_count) ? &race_table[mob->race] : NULL;
        json_t* form_json = json_object_get(m, "formFlags");
        if (json_is_array(form_json))
            mob->form = flags_from_array_with_defaults(form_json, form_defaults_flag_table, form_flag_table);
        else if (mob_race)
            mob->form = mob_race->form;
        json_t* parts_json = json_object_get(m, "partFlags");
        if (json_is_array(parts_json))
            mob->parts = flags_from_array_with_defaults(parts_json, part_defaults_flag_table, part_flag_table);
        else if (mob_race)
            mob->parts = mob_race->parts;

        mob->alignment = (int16_t)json_int_or_default(m, "alignment", mob->alignment);
        mob->group = (int16_t)json_int_or_default(m, "group", mob->group);
        mob->level = (int16_t)json_int_or_default(m, "level", mob->level);
        mob->hitroll = (int16_t)json_int_or_default(m, "hitroll", mob->hitroll);
        json_to_dice(json_object_get(m, "hitDice"), mob->hit);
        json_to_dice(json_object_get(m, "manaDice"), mob->mana);
        json_to_dice(json_object_get(m, "damageDice"), mob->damage);

        const char* dam = JSON_STRING(m, "damType");
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

        const char* startPos = JSON_STRING(m, "startPos");
        if (startPos)
            mob->start_pos = (int16_t)position_lookup(startPos);
        const char* defPos = JSON_STRING(m, "defaultPos");
        if (defPos)
            mob->default_pos = (int16_t)position_lookup(defPos);
        const char* sex = JSON_STRING(m, "sex");
        if (sex)
            mob->sex = (Sex)sex_lookup(sex);

        mob->wealth = (int)json_int_or_default(m, "wealth", mob->wealth);
        const char* size = JSON_STRING(m, "size");
        if (size)
            mob->size = (int16_t)size_lookup(size);
        const char* mat = JSON_STRING(m, "material");
        JSON_INTERN(mat, mob->material)
        mob->faction_vnum = (VNUM)json_int_or_default(m, "factionVnum", mob->faction_vnum);
        const char* loot_table = JSON_STRING(m, "lootTable");
        JSON_INTERN(loot_table, mob->loot_table)

        json_t* craft_mats = json_object_get(m, "craftMats");
        if (json_is_array(craft_mats)) {
            size_t count = json_array_size(craft_mats);
            if (count > 0) {
                mob->craft_mats = malloc(sizeof(VNUM) * count);
                if (mob->craft_mats != NULL) {
                    mob->craft_mat_count = (int)count;
                    for (size_t i = 0; i < count; i++) {
                        mob->craft_mats[i] = (VNUM)json_integer_value(json_array_get(craft_mats, i));
                    }
                }
            }
        }

        const char* script = JSON_STRING(m, "loxScript");
        if (script && script[0] != '\0')
            ((Entity*)mob)->script = lox_string(script);
        parse_events(json_object_get(m, "events"), (Entity*)mob, ENT_MOB);
        ensure_entity_class((Entity*)mob, "mob");

        global_mob_proto_set(mob);
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;
        assign_area_vnum(vnum);
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

static json_t* build_shops(const AreaData* area)
{
    json_t* arr = json_array();
    MobPrototype* mob;
    FOR_EACH_MOB_PROTO(mob) {
        if (mob->area != area || mob->pShop == NULL)
            continue;

        ShopData* shop = mob->pShop;
        json_t* obj = json_object();
        JSON_SET_INT(obj, "keeper", VNUM_FIELD(mob));
        json_t* buy = json_array();
        for (int i = 0; i < MAX_TRADE; i++)
            json_array_append_new(buy, json_integer(shop->buy_type[i]));
        json_object_set_new(obj, "buyTypes", buy);
        JSON_SET_INT(obj, "profitBuy", shop->profit_buy);
        JSON_SET_INT(obj, "profitSell", shop->profit_sell);
        JSON_SET_INT(obj, "openHour", shop->open_hour);
        JSON_SET_INT(obj, "closeHour", shop->close_hour);
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
        if (shop)
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
    const char* wclass = flag_string(weapon_class, obj->weapon.weapon_type);
    if (wclass && wclass[0] != '\0' && obj->weapon.weapon_type != 0)
        JSON_SET_STRING(w, "class", wclass);
    if (obj->weapon.num_dice != 0 || obj->weapon.size_dice != 0) {
        json_t* dice = json_array();
        json_array_append_new(dice, json_integer(obj->weapon.num_dice));
        json_array_append_new(dice, json_integer(obj->weapon.size_dice));
        json_object_set_new(w, "dice", dice);
    }
    if (obj->weapon.damage_type != 0)
        JSON_SET_STRING(w, "damageType", attack_table[obj->weapon.damage_type].name);
    json_set_flags_if(w, "flags", obj->weapon.flags, weapon_type2);
    return w;
}

static json_t* build_container(const ObjPrototype* obj)
{
    json_t* c = json_object();
    json_set_int_if(c, "capacity", obj->container.capacity, 0);
    json_set_flags_if(c, "flags", obj->container.flags, container_flag_table);
    json_set_int_if(c, "keyVnum", obj->container.key_vnum, 0);
    json_set_int_if(c, "maxWeight", obj->container.max_item_weight, 0);
    json_set_int_if(c, "weightMult", obj->container.weight_mult, 0);
    return c;
}

static void apply_weapon(ObjPrototype* obj, json_t* weapon)
{
    if (!json_is_object(weapon))
        return;
    const char* wclass = JSON_STRING(weapon, "class");
    if (wclass) {
        FLAGS val = flag_lookup(wclass, weapon_class);
        if (val != NO_FLAG)
            obj->weapon.weapon_type = (int16_t)val;
    }
    const char* dtype = JSON_STRING(weapon, "damageType");
    if (dtype) {
        int dt = attack_lookup(dtype);
        if (dt >= 0)
            obj->weapon.damage_type = dt;
    }
    json_t* dice = json_object_get(weapon, "dice");
    if (json_is_array(dice) && json_array_size(dice) >= 2) {
        obj->weapon.num_dice = (int16_t)json_integer_value(json_array_get(dice, 0));
        obj->weapon.size_dice = (int16_t)json_integer_value(json_array_get(dice, 1));
    }
    obj->weapon.flags = (int16_t)JSON_FLAGS(weapon, "flags", weapon_type2);
}

static void apply_container(ObjPrototype* obj, json_t* container)
{
    if (!json_is_object(container))
        return;
    obj->container.capacity = (int16_t)json_int_or_default(container, "capacity", obj->container.capacity);
    obj->container.flags = (int16_t)JSON_FLAGS(container, "flags", container_flag_table);
    obj->container.key_vnum = (VNUM)json_int_or_default(container, "keyVnum", obj->container.key_vnum);
    obj->container.max_item_weight = (int16_t)json_int_or_default(container, "maxWeight", obj->container.max_item_weight);
    obj->container.weight_mult = (int16_t)json_int_or_default(container, "weightMult", obj->container.weight_mult);
}

static json_t* build_light(const ObjPrototype* obj)
{
    json_t* l = json_object();
    json_set_int_if(l, "hours", obj->light.hours, 0);
    return l;
}

static json_t* build_armor(const ObjPrototype* obj)
{
    json_t* a = json_object();
    json_set_int_if(a, "acPierce", obj->armor.ac_pierce, 0);
    json_set_int_if(a, "acBash", obj->armor.ac_bash, 0);
    json_set_int_if(a, "acSlash", obj->armor.ac_slash, 0);
    json_set_int_if(a, "acExotic", obj->armor.ac_exotic, 0);
    return a;
}

static json_t* build_drink(const ObjPrototype* obj)
{
    json_t* d = json_object();
    json_set_int_if(d, "capacity", obj->drink_con.capacity, 0);
    json_set_int_if(d, "remaining", obj->drink_con.current, 0);
    if (obj->drink_con.liquid_type > 0 && obj->drink_con.liquid_type < LIQ_COUNT)
        JSON_SET_STRING(d, "liquid", liquid_table[obj->drink_con.liquid_type].name);
    if (obj->drink_con.poisoned != 0)
        json_object_set_new(d, "poisoned", json_boolean(true));
    return d;
}

static json_t* build_fountain(const ObjPrototype* obj)
{
    json_t* f = json_object();
    json_set_int_if(f, "capacity", obj->fountain.capacity, 0);
    json_set_int_if(f, "remaining", obj->fountain.current, 0);
    if (obj->fountain.liquid_type > 0 && obj->fountain.liquid_type < LIQ_COUNT)
        JSON_SET_STRING(f, "liquid", liquid_table[obj->fountain.liquid_type].name);
    return f;
}

static json_t* build_food(const ObjPrototype* obj)
{
    json_t* f = json_object();
    json_set_int_if(f, "foodHours", obj->food.hours_full, 0);
    json_set_int_if(f, "fullHours", obj->food.hours_hunger, 0);
    if (obj->food.poisoned != 0)
        json_object_set_new(f, "poisoned", json_boolean(true));
    return f;
}

static json_t* build_money(const ObjPrototype* obj)
{
    json_t* m = json_object();
    json_set_int_if(m, "copper", obj->money.copper, 0);
    json_set_int_if(m, "silver", obj->money.silver, 0);
    json_set_int_if(m, "gold", obj->money.gold, 0);
    return m;
}

static json_t* build_wandstaff(const ObjPrototype* obj)
{
    json_t* w = json_object();
    json_set_int_if(w, "level", obj->wand.level, 0);
    json_set_int_if(w, "chargesTotal", obj->wand.max_charges, 0);
    if (obj->wand.charges != obj->wand.max_charges)
        json_set_int_if(w, "chargesRemaining", obj->wand.charges, 0);
    const char* spell = skill_name_from_sn((SKNUM)obj->wand.spell);
    if (spell && obj->wand.spell >= 0)
        JSON_SET_STRING(w, "spell", spell);
    return w;
}

static json_t* build_scroll_potion_pill(const ObjPrototype* obj)
{
    json_t* s = json_object();
    json_set_int_if(s, "level", obj->scroll.level, 0);
    json_t* spells = json_array();
    const char* spell1 = skill_name_from_sn((SKNUM)obj->scroll.spell1);
    const char* spell2 = skill_name_from_sn((SKNUM)obj->scroll.spell2);
    const char* spell3 = skill_name_from_sn((SKNUM)obj->scroll.spell3);
    json_array_append_new(spells, json_string(spell1 ? spell1 : ""));
    json_array_append_new(spells, json_string(spell2 ? spell2 : ""));
    json_array_append_new(spells, json_string(spell3 ? spell3 : ""));
    json_object_set_new(s, "spells", spells);
    return s;
}

static json_t* build_portal(const ObjPrototype* obj)
{
    json_t* p = json_object();
    json_set_int_if(p, "charges", obj->portal.charges, 0);
    json_set_flags_if(p, "exitFlags", obj->portal.exit_flags, exit_flag_table);
    json_set_flags_if(p, "portalFlags", obj->portal.gate_flags, portal_flag_table);
    json_set_int_if(p, "toVnum", obj->portal.destination, 0);
    json_set_int_if(p, "keyVnum", obj->portal.key_vnum, 0);
    return p;
}

static json_t* build_furniture(const ObjPrototype* obj)
{
    json_t* f = json_object();
    json_set_int_if(f, "maxPeople", obj->furniture.max_people, 0);
    json_set_int_if(f, "maxWeight", obj->furniture.max_weight, 0);
    json_set_flags_if(f, "flags", obj->furniture.flags, furniture_flag_table);
    json_set_int_if(f, "healBonus", obj->furniture.heal_rate, 0);
    json_set_int_if(f, "manaBonus", obj->furniture.mana_rate, 0);
    return f;
}

static void apply_light(ObjPrototype* obj, json_t* light)
{
    if (!json_is_object(light))
        return;
    obj->light.hours = (int16_t)json_int_or_default(light, "hours", obj->light.hours);
}

static void apply_armor(ObjPrototype* obj, json_t* armor)
{
    if (!json_is_object(armor))
        return;
    obj->armor.ac_pierce = (int16_t)json_int_or_default(armor, "acPierce", obj->armor.ac_pierce);
    obj->armor.ac_bash = (int16_t)json_int_or_default(armor, "acBash", obj->armor.ac_bash);
    obj->armor.ac_slash = (int16_t)json_int_or_default(armor, "acSlash", obj->armor.ac_slash);
    obj->armor.ac_exotic = (int16_t)json_int_or_default(armor, "acExotic", obj->armor.ac_exotic);
}

static void apply_drink(ObjPrototype* obj, json_t* drink)
{
    if (!json_is_object(drink))
        return;
    obj->drink_con.capacity = (int)json_int_or_default(drink, "capacity", obj->drink_con.capacity);
    obj->drink_con.current = (int)json_int_or_default(drink, "remaining", obj->drink_con.current);
    const char* liquid = JSON_STRING(drink, "liquid");
    if (liquid) {
        int l = liquid_lookup(liquid);
        if (l >= 0)
            obj->drink_con.liquid_type = l;
    }
    obj->drink_con.poisoned = json_bool_or_default(drink, "poisoned", obj->drink_con.poisoned != 0) ? 1 : 0;
}

static void apply_fountain(ObjPrototype* obj, json_t* fountain)
{
    if (!json_is_object(fountain))
        return;
    obj->fountain.capacity = (int)json_int_or_default(fountain, "capacity", obj->fountain.capacity);
    obj->fountain.current = (int)json_int_or_default(fountain, "remaining", obj->fountain.current);
    const char* liquid = JSON_STRING(fountain, "liquid");
    if (liquid) {
        int l = liquid_lookup(liquid);
        if (l >= 0)
            obj->fountain.liquid_type = l;
    }
}

static void apply_food(ObjPrototype* obj, json_t* food)
{
    if (!json_is_object(food))
        return;
    obj->food.hours_full = (int16_t)json_int_or_default(food, "foodHours", obj->food.hours_full);
    obj->food.hours_hunger = (int16_t)json_int_or_default(food, "fullHours", obj->food.hours_hunger);
    obj->food.poisoned = json_bool_or_default(food, "poisoned", obj->food.poisoned != 0) ? 1 : 0;
}

static void apply_money(ObjPrototype* obj, json_t* money)
{
    if (!json_is_object(money))
        return;
    obj->money.copper = (int)json_int_or_default(money, "copper", obj->money.copper);
    obj->money.silver = (int)json_int_or_default(money, "silver", obj->money.silver);
    obj->money.gold = (int)json_int_or_default(money, "gold", obj->money.gold);
}

static void apply_wandstaff(ObjPrototype* obj, json_t* wand)
{
    if (!json_is_object(wand))
        return;
    obj->wand.level = (int16_t)json_int_or_default(wand, "level", obj->wand.level);
    obj->wand.max_charges = (int16_t)json_int_or_default(wand, "chargesTotal", obj->wand.max_charges);
    json_t* rem = json_object_get(wand, "chargesRemaining");
    if (json_is_integer(rem))
        obj->wand.charges = (int16_t)json_integer_value(rem);
    else
        obj->wand.charges = obj->wand.max_charges;
    const char* spell = JSON_STRING(wand, "spell");
    SKNUM sn = spell_lookup_name(spell);
    if (sn >= 0)
        obj->wand.spell = sn;
}

static void apply_scroll_potion_pill(ObjPrototype* obj, json_t* sp)
{
    if (!json_is_object(sp))
        return;
    obj->scroll.level = (int16_t)json_int_or_default(sp, "level", obj->scroll.level);
    apply_spell_array(json_object_get(sp, "spells"), obj, 1, 3);
}

static void apply_portal(ObjPrototype* obj, json_t* portal)
{
    if (!json_is_object(portal))
        return;
    obj->portal.charges = (int16_t)json_int_or_default(portal, "charges", obj->portal.charges);
    obj->portal.exit_flags = (int16_t)JSON_FLAGS(portal, "exitFlags", exit_flag_table);
    obj->portal.gate_flags = (int16_t)JSON_FLAGS(portal, "portalFlags", portal_flag_table);
    obj->portal.destination = (int16_t)json_int_or_default(portal, "toVnum", obj->portal.destination);
    obj->portal.key_vnum = (VNUM)json_int_or_default(portal, "keyVnum", obj->portal.key_vnum);
}

static void apply_furniture(ObjPrototype* obj, json_t* furniture)
{
    if (!json_is_object(furniture))
        return;
    obj->furniture.max_people = (int16_t)json_int_or_default(furniture, "maxPeople", obj->furniture.max_people);
    obj->furniture.max_weight = (int16_t)json_int_or_default(furniture, "maxWeight", obj->furniture.max_weight);
    obj->furniture.flags = (int16_t)JSON_FLAGS(furniture, "flags", furniture_flag_table);
    obj->furniture.heal_rate = (int16_t)json_int_or_default(furniture, "healBonus", obj->furniture.heal_rate);
    obj->furniture.mana_rate = (int16_t)json_int_or_default(furniture, "manaBonus", obj->furniture.mana_rate);
}


static int compare_obj_proto(const void* lhs, const void* rhs)
{
    const ObjPrototype* a = *(const ObjPrototype* const*)lhs;
    const ObjPrototype* b = *(const ObjPrototype* const*)rhs;
    if (VNUM_FIELD(a) < VNUM_FIELD(b))
        return -1;
    if (VNUM_FIELD(a) > VNUM_FIELD(b))
        return 1;
    return 0;
}

static json_t* build_objects(const AreaData* area)
{
    json_t* arr = json_array();
    int total = global_obj_proto_count();
    if (total <= 0)
        return arr;

    ObjPrototype** list = malloc((size_t)total * sizeof(*list));
    if (!list)
        return arr;

    size_t count = 0;
    ObjPrototype* obj;
    FOR_EACH_OBJ_PROTO(obj) {
        if (obj->area != area)
            continue;
        list[count++] = obj;
    }

    if (count == 0) {
        free(list);
        return arr;
    }

    qsort(list, count, sizeof(*list), compare_obj_proto);

    for (size_t i = 0; i < count; ++i) {
        obj = list[i];

        json_t* o = json_object();
        JSON_SET_INT(o, "vnum", VNUM_FIELD(obj));
        JSON_SET_STRING(o, "name", NAME_STR(obj));
        JSON_SET_STRING(o, "shortDescr", obj->short_descr);
        JSON_SET_STRING(o, "description", obj->description);
        if (obj->material && obj->material[0] != '\0')
            JSON_SET_STRING(o, "material", obj->material);
        JSON_SET_STRING(o, "itemType", flag_string(type_flag_table, obj->item_type));
        json_set_flags_if(o, "extraFlags", obj->extra_flags, extra_flag_table);
        json_set_flags_if(o, "wearFlags", obj->wear_flags, wear_flag_table);
        bool has_value = false;
        for (int j = 0; j < 5; j++) {
            if (obj->value[j] != 0) {
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
            JSON_SET_STRING(o, "loxScript", ent->script->chars);
        json_t* ev = build_events(ent);
        if (json_array_size(ev) > 0)
            json_object_set_new(o, "events", ev);
        else
            json_decref(ev);

        json_array_append_new(arr, o);
    }

    free(list);
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

        VNUM vnum = (VNUM)json_int_or_default(o, "vnum", VNUM_NONE);
        VNUM_FIELD(obj) = vnum;

        const char* name = JSON_STRING(o, "name");
        if (name)
            SET_NAME(obj, lox_string(name));
        const char* sd = JSON_STRING(o, "shortDescr");
        JSON_INTERN(sd, obj->short_descr)
        const char* desc = JSON_STRING(o, "description");
        JSON_INTERN(desc, obj->description)
        const char* mat = JSON_STRING(o, "material");
        JSON_INTERN(mat, obj->material)

        const char* itype = JSON_STRING(o, "itemType");
        if (itype) {
            FLAGS val = flag_lookup(itype, type_flag_table);
            if (val != NO_FLAG)
                obj->item_type = (ItemType)val;
        }

        obj->extra_flags = JSON_FLAGS(o, "extraFlags", extra_flag_table);
        obj->wear_flags = JSON_FLAGS(o, "wearFlags", wear_flag_table);
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
                ed->keyword = boot_intern_string(JSON_STRING(e, "keyword"));
                ed->description = boot_intern_string(JSON_STRING(e, "description"));
                ADD_EXTRA_DESC(obj, ed)
            }
        }
        parse_affects(json_object_get(o, "affects"), &obj->affected);

        const char* script = JSON_STRING(o, "loxScript");
        if (script && script[0] != '\0')
            ((Entity*)obj)->script = lox_string(script);
        parse_events(json_object_get(o, "events"), (Entity*)obj, ENT_OBJ);
        ensure_entity_class((Entity*)obj, "obj");

        if (!obj->material || obj->material[0] == '\0') {
            free_string(obj->material);
            obj->material = boot_intern_string("");
        }
        global_obj_proto_set(obj);
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
                JSON_SET_INT(obj, "vnum", vnum);
                JSON_SET_STRING(obj, "code", prog->code);
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
        const char* code = JSON_STRING(p, "code");
        if (vnum <= 0 || !code)
            continue;
        MobProgCode* prog = new_mob_prog_code();
        prog->vnum = vnum;
        prog->code = boot_intern_string(code);
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
        JSON_SET_INT(obj, "vnum", q->vnum);
        JSON_SET_STRING(obj, "name", q->name ? q->name : "");
        JSON_SET_STRING(obj, "entry", q->entry ? q->entry : "");
        JSON_SET_STRING(obj, "type", flag_string(quest_type_table, q->type));
        JSON_SET_INT(obj, "xp", q->xp);
        JSON_SET_INT(obj, "level", q->level);
        JSON_SET_INT(obj, "end", q->end);
        JSON_SET_INT(obj, "target", q->target);
        JSON_SET_INT(obj, "upper", q->target_upper);
        JSON_SET_INT(obj, "count", q->amount);
        JSON_SET_INT(obj, "rewardFaction", q->reward_faction_vnum);
        JSON_SET_INT(obj, "rewardReputation", q->reward_reputation);
        JSON_SET_INT(obj, "rewardGold", q->reward_gold);
        JSON_SET_INT(obj, "rewardSilver", q->reward_silver);
        JSON_SET_INT(obj, "rewardCopper", q->reward_copper);
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

        const char* name = JSON_STRING(q, "name");
        JSON_INTERN(name, quest->name)
        const char* entry = JSON_STRING(q, "entry");
        JSON_INTERN(entry, quest->entry)
        const char* type = JSON_STRING(q, "type");
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
            JSON_SET_INT(obj, "mobVnum", VNUM_FIELD(mob));
            JSON_SET_STRING(obj, "spec", spec_name(mob->spec_fun));
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
        const char* spec = JSON_STRING(s, "spec");
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
        JSON_SET_INT(obj, "mobVnum", reset->arg1);
        JSON_SET_INT(obj, "maxInArea", reset->arg2);
        JSON_SET_INT(obj, "roomVnum", reset->arg3);
        if (reset->arg4 != 0)
            JSON_SET_INT(obj, "maxInRoom", reset->arg4);
        break;
    case 'O':
        name = "placeObj";
        JSON_SET_INT(obj, "objVnum", reset->arg1);
        JSON_SET_INT(obj, "roomVnum", reset->arg3);
        break;
    case 'P':
        name = "putObj";
        JSON_SET_INT(obj, "objVnum", reset->arg1);
        if (reset->arg2 != 0)
            JSON_SET_INT(obj, "count", reset->arg2);
        JSON_SET_INT(obj, "containerVnum", reset->arg3);
        if (reset->arg4 != 0)
            JSON_SET_INT(obj, "maxInContainer", reset->arg4);
        break;
    case 'G':
        name = "giveObj";
        JSON_SET_INT(obj, "objVnum", reset->arg1);
        break;
    case 'E':
        name = "equipObj";
        JSON_SET_INT(obj, "objVnum", reset->arg1);
        JSON_SET_STRING(obj, "wearLoc", flag_string(wear_loc_strings, reset->arg3));
        break;
    case 'D':
        name = "setDoor";
        JSON_SET_INT(obj, "roomVnum", reset->arg1);
        JSON_SET_STRING(obj, "direction", dir_name_from_enum(reset->arg2));
        JSON_SET_INT(obj, "state", reset->arg3);
        break;
    case 'R':
        name = "randomizeExits";
        JSON_SET_INT(obj, "roomVnum", reset->arg1);
        JSON_SET_INT(obj, "exits", reset->arg2);
        break;
    default:
        break;
    }

    JSON_SET_INT(obj, "roomVnum", room_vnum);
    if (name)
        JSON_SET_STRING(obj, "commandName", name);
    else {
        JSON_SET_STRING(obj, "command", cmd);
        JSON_SET_INT(obj, "arg1", reset->arg1);
        JSON_SET_INT(obj, "arg2", reset->arg2);
        JSON_SET_INT(obj, "arg3", reset->arg3);
        JSON_SET_INT(obj, "arg4", reset->arg4);
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
        const char* cmd_str = JSON_STRING(r, "command");
        const char* cmd_name = JSON_STRING(r, "commandName");
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
                const char* wear = JSON_STRING(r, "wearLoc");
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
                const char* dir = JSON_STRING(r, "direction");
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
    ha->filename = boot_intern_string(area->file_name ? area->file_name : "area.json");
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
        help->keyword = boot_intern_string(JSON_STRING(h, "keyword"));
        help->text = boot_intern_string(JSON_STRING(h, "text"));
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
        JSON_SET_INT(obj, "vnum", VNUM_FIELD(faction));
        JSON_SET_STRING(obj, "name", NAME_STR(faction));
        JSON_SET_INT(obj, "defaultStanding", faction->default_standing);
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

        const char* name = JSON_STRING(f, "name");
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
        JSON_SET_INT(obj, "level", h->level);
        JSON_SET_STRING(obj, "keyword", h->keyword);
        JSON_SET_STRING(obj, "text", h->text);
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
    
    // Parse loot groups and tables for this area
    if (area_persist_succeeded(res)) {
        json_t* loot = json_object_get(root, "loot");
        if (loot)
            loot_persist_json_parse(loot, &current_area_data->header);
    }

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
    JSON_SET_INT(root, "formatVersion", AREA_JSON_FORMAT_VERSION);
    json_object_set_new(root, "areadata", build_areadata(params->area));
    json_object_set_new(root, "storyBeats", build_story_beats(params->area));
    json_object_set_new(root, "checklist", build_checklist(params->area));
    json_t* daycycle = build_area_daycycle(params->area);
    if (daycycle)
        json_object_set_new(root, "daycycle", daycycle);
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
    
    // Build and add loot groups/tables for this area
    json_t* loot = loot_persist_json_build(&params->area->header);
    if (loot)
        json_object_set_new(root, "loot", loot);

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
