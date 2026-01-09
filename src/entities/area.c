////////////////////////////////////////////////////////////////////////////////
// entities/area.c
// Utilities to handle area
////////////////////////////////////////////////////////////////////////////////

#include "area.h"

#include <data/direction.h>

#include <lox/memory.h>

#include <olc/olc.h>

#include <comm.h>
#include <config.h>
#include <db.h>
#include <handler.h>
#include <stdlib.h>

int area_data_perm_count;
ValueArray global_areas = { 0 };
AreaData* area_data_free;

int area_count = 0;
int area_data_count = 0;
int area_perm_count;
Area* area_free;

Area* new_area(AreaData* area_data)
{
    LIST_ALLOC_PERM(area, Area);

    gc_protect(OBJ_VAL(area));

    init_header(&area->header, OBJ_AREA);

    init_table(&area->rooms);
    SET_LOX_FIELD(&area->header, &area->rooms, rooms);

    SET_NAME(area, NAME_FIELD(area_data));
    VNUM_FIELD(area) = VNUM_FIELD(area_data);

    area->data = area_data;
    area->empty = true;
    area->empty_timer = 0;
    area->owner_list = str_empty;
    area->teardown_in_progress = false;
    area->reset_timer = area->data->reset_thresh;

    copy_spawn_array(&area->gather_spawns, &area_data->gather_spawns);

    return area;
}

void free_area(Area* area)
{
    // Set flag to skip expensive inbound exit cleanup
    // since we're destroying all rooms anyway
    area->teardown_in_progress = true;
    free_gather_spawn_array(&area->gather_spawns);
    free_table(&area->header.fields);
    free_table(&area->rooms);

    LIST_FREE(area);
}

AreaData* new_area_data()
{
    char buf[MAX_INPUT_LENGTH];

    LIST_ALLOC_PERM(area_data, AreaData);

    gc_protect(OBJ_VAL(area_data));

    init_header(&area_data->header, OBJ_AREA_DATA);

    init_list(&area_data->instances);
    SET_LOX_FIELD(&area_data->header, &area_data->instances, instances);

    VNUM_FIELD(area_data) = global_areas.count;
    const char* def_fmt = cfg_get_default_format();
    const char* ext = (def_fmt && !str_cmp(def_fmt, "json")) ? ".json" : ".are";
    sprintf(buf, "area%"PRVNUM"%s", VNUM_FIELD(area_data), ext);

    area_data->area_flags = AREA_ADDED;
    area_data->security = 1;
    area_data->builders = str_empty;
    area_data->credits = str_empty;
    area_data->reset_thresh = 6;
    area_data->file_name = str_dup(buf);
    area_data->story_beats = NULL;
    area_data->checklist = NULL;
    area_data->periods = NULL;
    area_data->suppress_daycycle_messages = false;

    return area_data;
}

void free_area_data(AreaData* area_data)
{
    // area_data->name will get GC'd
    free_string(area_data->file_name);
    free_string(area_data->builders);
    free_string(area_data->credits);
    free_story_beats(area_data->story_beats);
    free_checklist(area_data->checklist);
    area_daycycle_period_clear(area_data);
    free_gather_spawn_array(&area_data->gather_spawns);

    remove_array_value(&global_areas, OBJ_VAL(area_data));

    LIST_FREE(area_data);
}

Area* create_area_instance(AreaData* area_data, bool create_exits)
{
    Area* area = new_area(area_data);

    list_push_back(&area_data->instances, OBJ_VAL(area));

    RoomData* room_data;
    FOR_EACH_GLOBAL_ROOM(room_data) {
        if (room_data->area_data == area_data) {
            // It's okay; it's still being added to area instances
            new_room(room_data, area);
        }
    }

    // Rooms are now all made. Time to populate exits.
    if (create_exits)
        create_instance_exits(area);

    return area;
}

void create_instance_exits(Area* area)
{
    Room* room;

    FOR_EACH_AREA_ROOM(room, area) {
        for (int i = 0; i < DIR_MAX; ++i) {
            if (room->data->exit_data[i])
                room->exit[i] = new_room_exit(room->data->exit_data[i], room);
        }
    }
}

Area* get_area_for_player(Mobile* ch, AreaData* area_data)
{
    Area* area = NULL;

    for (Node* node = area_data->instances.front; node != NULL; node = node->next) {
        area = AS_AREA(node->value);
        if (is_name(NAME_STR(ch), area->owner_list))
            return area;
    }

    return NULL;
}

/*
 * OLC
 * Use these macros to load any new area formats that you choose to
 * support on your MUD.  See the load_area format below for
 * a short example.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                \
    if (!str_cmp(word, literal)) {                  \
        field  = value;                             \
        break;                                      \
    }

#define SKEY( string, field )                       \
    if (!str_cmp(word, string)) {                   \
        free_string(field);                         \
        field = fread_string(fp);                   \
        break;                                      \
    }

#define KEYLS(literal, entity, field, value)                                   \
    if (!str_cmp(word, literal)) {                                             \
        (entity)->header.field = (value);                                      \
        SET_LOX_FIELD(&((entity)->header), (entity)->header.field, field);     \
        break;                                                                 \
    }

// Boot routines

void load_area(FILE* fp)
{
    AreaData* area_data;
    char* word;
#ifdef _MSC_VER
    int version = 1;
#else
    int __attribute__((unused)) version;
#endif

    area_data = new_area_data();
    VNUM_FIELD(area_data) = global_areas.count;
    area_data->reset_thresh = 6;
    area_data->file_name = str_dup(fpArea);
    area_data->security = 9;
    area_data->story_beats = NULL;
    area_data->checklist = NULL;

    for (; ; ) {
        word = feof(fp) ? "End" : fread_word(fp);

        switch (UPPER(word[0])) {
        case 'A':
            KEY("AlwaysReset", area_data->always_reset, (bool)fread_number(fp));
            break;
        case 'B':
            SKEY("Builders", area_data->builders);
            break;
        case 'C':
            SKEY("Credits", area_data->credits);
            break;
        case 'E':
            if (!str_cmp(word, "End")) {
                write_value_array(&global_areas, OBJ_VAL(area_data));
                if (global_areas.count > 0)
                    LAST_AREA_DATA->next = area_data;
                area_data->next = NULL;
                current_area_data = area_data;
                gc_protect_clear();
                return;
            }
            break;
        case 'G':
            if (!str_cmp(word, "GatherSpawn")) {
                Sector sector = fread_number(fp);
                VNUM vnum = fread_number(fp);
                int quantity = fread_number(fp);
                int respawn_timer = fread_number(fp);
                add_gather_spawn(&area_data->gather_spawns, sector, vnum, 
                    quantity, respawn_timer);
            }
            break;
        case 'H':
            KEY("High", area_data->high_range, (LEVEL)fread_number(fp));
            break;
        case 'I':
            KEY("InstType", area_data->inst_type, fread_number(fp));
            break;
        case 'L':
            KEY("Low", area_data->low_range, (LEVEL)fread_number(fp));
            SKEY("LootTable", area_data->loot_table);
            break;
        case 'N':
            KEYLS("Name", area_data, name, fread_lox_string(fp));
            break;
        case 'R':
            KEY("Reset", area_data->reset_thresh, (int16_t)fread_number(fp));
            break;
        case 'S':
            KEY("Security", area_data->security, fread_number(fp));
            KEY("Sector", area_data->sector, fread_number(fp));
            break;
        case 'V':
            if (!str_cmp(word, "VNUMs")) {
                area_data->min_vnum = fread_number(fp);
                area_data->max_vnum = fread_number(fp);
                break;
            }
            KEY("Version", version, fread_number(fp));
            break;
        // End switch
        }
    }
}

void load_story_beats(FILE* fp)
{
    if (current_area_data == NULL)
        return;

    for (;;) {
        char letter = fread_letter(fp);
        if (letter == 'S')
            break;
        if (letter != 'B') {
            fread_to_eol(fp);
            continue;
        }
        char* title = fread_string(fp);
        char* desc = fread_string(fp);
        add_story_beat(current_area_data, title, desc);
        free_string(title);
        free_string(desc);
    }
}

void load_checklist(FILE* fp)
{
    if (current_area_data == NULL)
        return;

    for (;;) {
        char letter = fread_letter(fp);
        if (letter == 'S')
            break;
        if (letter != 'C') {
            fread_to_eol(fp);
            continue;
        }
        int status = fread_number(fp);
        char* title = fread_string(fp);
        char* desc = fread_string(fp);
        ChecklistStatus st = (ChecklistStatus)status;
        if (st < CHECK_TODO || st > CHECK_DONE)
            st = CHECK_TODO;
        add_checklist_item(current_area_data, title, desc, st);
        free_string(title);
        free_string(desc);
    }
}

void load_area_daycycle(FILE* fp)
{
    if (current_area_data == NULL)
        return;

    for (;;) {
        char letter = fread_letter(fp);
        if (letter == 'S')
            break;

        if (letter == 'P') {
            const char* name = fread_word(fp);
            int start = fread_number(fp);
            int end = fread_number(fp);
            DayCyclePeriod* period = area_daycycle_period_add(current_area_data, name, start, end);
            if (period == NULL) {
                bug("load_area_daycycle: failed to add time period '%s'.", name ? name : "");
                exit(1);
            }
            free_string(period->description);
            period->description = fread_string(fp);
        }
        else if (letter == 'B') {
            const char* name = fread_word(fp);
            DayCyclePeriod* period = area_daycycle_period_find(current_area_data, name);
            if (period == NULL) {
                bug("load_area_daycycle: period '%s' not found for enter message.", name ? name : "");
                exit(1);
            }
            free_string(period->enter_message);
            period->enter_message = fread_string(fp);
        }
        else if (letter == 'A') {
            const char* name = fread_word(fp);
            DayCyclePeriod* period = area_daycycle_period_find(current_area_data, name);
            if (period == NULL) {
                bug("load_area_daycycle: period '%s' not found for exit message.", name ? name : "");
                exit(1);
            }
            free_string(period->exit_message);
            period->exit_message = fread_string(fp);
        }
        else if (letter == 'W') {
            current_area_data->suppress_daycycle_messages = fread_number(fp) != 0;
        }
        else {
            fread_to_eol(fp);
        }
    }
}

StoryBeat* add_story_beat(AreaData* area_data, const char* title, const char* description)
{
    StoryBeat* beat = malloc(sizeof(StoryBeat));
    if (!beat)
        return NULL;
    beat->title = str_dup(title ? title : "");
    beat->description = str_dup(description ? description : "");
    beat->next = NULL;

    if (!area_data->story_beats)
        area_data->story_beats = beat;
    else {
        StoryBeat* tail = area_data->story_beats;
        while (tail->next)
            tail = tail->next;
        tail->next = beat;
    }
    return beat;
}

ChecklistItem* add_checklist_item(AreaData* area_data, const char* title, const char* description, ChecklistStatus status)
{
    ChecklistItem* item = malloc(sizeof(ChecklistItem));
    if (!item)
        return NULL;
    item->title = str_dup(title ? title : "");
    item->description = str_dup(description ? description : "");
    item->status = status;
    item->next = NULL;

    if (!area_data->checklist)
        area_data->checklist = item;
    else {
        ChecklistItem* tail = area_data->checklist;
        while (tail->next)
            tail = tail->next;
        tail->next = item;
    }
    return item;
}

void free_story_beats(StoryBeat* head)
{
    StoryBeat* beat = head;
    while (beat) {
        StoryBeat* next = beat->next;
        free_string(beat->title);
        free_string(beat->description);
        free(beat);
        beat = next;
    }
}

void free_checklist(ChecklistItem* head)
{
    ChecklistItem* item = head;
    while (item) {
        ChecklistItem* next = item->next;
        free_string(item->title);
        free_string(item->description);
        free(item);
        item = next;
    }
}
