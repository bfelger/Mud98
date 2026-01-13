////////////////////////////////////////////////////////////////////////////////
// craft/gather.c
////////////////////////////////////////////////////////////////////////////////

#include "gather.h"

#include <entities/area.h>

#include <olc/bit.h>
#include <olc/olc.h>

#include <comm.h>
#include <db.h>

////////////////////////////////////////////////////////////////////////////////
// Gather Types
////////////////////////////////////////////////////////////////////////////////

const GatherInfo gather_type_table[GATHER_TYPE_COUNT] = {
    { GATHER_NONE,          "none"      },
    { GATHER_ORE,           "ore"       },
    { GATHER_HERB,          "herbs"     },
};

const char* gather_type_name(GatherType type)
{
    for (int i = 0; i < GATHER_TYPE_COUNT; i++) {
        if (gather_type_table[i].type == type)
            return gather_type_table[i].name;
    }
    return "unknown";
}

GatherType gather_lookup(const char* name)
{
    for (int type = 0; type < GATHER_TYPE_COUNT; type++) {
        if (LOWER(name[0]) == LOWER(gather_type_table[type].name[0])
            && !str_prefix(name, gather_type_table[type].name))
            return gather_type_table[type].type;
    }

    return GATHER_NONE;
}

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////

void add_gather_spawn(GatherSpawnArray* array, Sector sector, VNUM vnum, 
    int quantity, int respawn_timer)
{
    if (array->count >= array->capacity) {
        size_t new_capacity = array->capacity == 0 ? 4 : array->capacity * 2;
        array->spawns = realloc_mem(array->spawns, array->capacity * sizeof(GatherSpawn), new_capacity * sizeof(GatherSpawn));
        array->capacity = new_capacity;
    }
    
    GatherSpawn* spawn = &array->spawns[array->count++];
    spawn->spawn_sector = sector;
    spawn->vnum = vnum;
    spawn->quantity = quantity;
    spawn->respawn_timer = respawn_timer;
}

void free_gather_spawn_array(GatherSpawnArray* array)
{
    if (array->spawns) {
        free_mem(array->spawns, array->capacity * sizeof(GatherSpawn));
        array->spawns = NULL;
    }
    array->count = 0;
    array->capacity = 0;
}

void copy_spawn_array(GatherSpawnArray* dest, const GatherSpawnArray* src)
{
    if (dest->spawns) {
        free_gather_spawn_array(dest);
    }

    if (src->count == 0) {
        return;
    }

    dest->spawns = realloc_mem(NULL, 0, src->capacity * sizeof(GatherSpawn));
    memcpy(dest->spawns, src->spawns, src->count * sizeof(GatherSpawn));
    dest->count = src->count;
    dest->capacity = src->capacity;
}

void reset_area_gather_spawns(Area* area)
{
    for (size_t i = 0; i < area->gather_spawns.count; i++) {
        GatherSpawn* spawn = &area->gather_spawns.spawns[i];
        GatherSpawn* spawn_data = &area->data->gather_spawns.spawns[i];
        if (spawn->respawn_timer++ >= spawn_data->respawn_timer)
            reset_gather_spawn(area, spawn);
    }
}

// If you change the gather spawns in an area, call this to reset them all.
// Useful for OLC area editing.
void force_reset_area_gather_spawns(Area* area)
{
    AreaData* area_data = area->data;
    free_gather_spawn_array(&area->gather_spawns);
    copy_spawn_array(&area->gather_spawns, &area_data->gather_spawns);
    for (size_t i = 0; i < area->gather_spawns.count; i++) {
        GatherSpawn* spawn = &area->gather_spawns.spawns[i];
        reset_gather_spawn(area, spawn);
    }
}

void reset_gather_spawn(Area* area, GatherSpawn* spawn)
{
    #define MAX_RESERVOIR_ROOMS 100
    // Use reservoir sampling to randomly select rooms of the correct sector
    // type to place gather nodes in. If we have more than 100 matching rooms, 
    // we limit ourselves to placing in 100 rooms max to avoid excessive memory
    // use.
    Room* reservoir[MAX_RESERVOIR_ROOMS];
    size_t reservoir_count = 0;

    ObjPrototype* obj_proto = get_object_prototype(spawn->vnum);
    if (!obj_proto) {
        bugf("reset_gather_spawn: Invalid gather spawn vnum %d in area %d",
            spawn->vnum, NAME_STR(area));
        spawn->respawn_timer = 0;
        return;
    }

    // Remove existing objects of this gather spawn type, and add sector-
    // matching rooms to reservoir
    Room* room;
    FOR_EACH_AREA_ROOM(room, area) {
        if (room->data->sector_type != spawn->spawn_sector)
            continue;

        // If we've reached max reservoir size, stop adding more. If you want
        // to increase this limit, increase MAX_RESERVOIR_ROOMS above.
        if (reservoir_count < MAX_RESERVOIR_ROOMS) {
            reservoir[reservoir_count++] = room;
        }

        Object* obj;
        FOR_EACH_ROOM_OBJ(obj, room) {
            if (VNUM_FIELD(obj) == spawn->vnum)
                extract_obj(obj);
        }
    }

    if (spawn->quantity <= 0 || reservoir_count == 0) {
        // Nothing to spawn
        spawn->respawn_timer = 0;
        return;
    }

    // Place gather nodes in randomly selected rooms from reservoir
    for (int i = 0; i < spawn->quantity && reservoir_count > 0; ) {
        size_t index = number_range(0, reservoir_count - 1);

        Room* target_room = reservoir[index];
        if (target_room) {
            Object* new_obj = create_object(obj_proto, 1);
            obj_to_room(new_obj, target_room);
            i++;
        }

        // Remove selected room from reservoir
        reservoir[index] = reservoir[--reservoir_count];
    }

    // Reset the respawn timer
    spawn->respawn_timer = 0;
}

void olc_show_gather_spawns(Mobile* ch, AreaData* area_data)
{
    if (area_data->gather_spawns.count == 0) {
        olc_print_str_box(ch, "Gather Spawns", "(none)", "Type '" COLOR_TITLE "GATHER ADD" COLOR_ALT_TEXT_2 "' to add one.");
        return;
    }
    send_to_char("Gather Spawns:\n\r", ch);
    send_to_char(COLOR_TITLE "     Idx  Sector        Qty   Timer    VNUM" COLOR_EOL, ch);
    send_to_char(COLOR_DECOR_2 "    ===== ============ ====== ===== ===========" COLOR_EOL, ch);
    for (size_t i = 0; i < area_data->gather_spawns.count; ++i) {
        GatherSpawn* spawn = &area_data->gather_spawns.spawns[i];
        const char* sector_name = flag_string(sector_flag_table, spawn->spawn_sector);
        
        ObjPrototype* obj_proto = get_object_prototype(spawn->vnum);
        printf_to_char(ch, PRETTY_IDX " %-12s " COLOR_DECOR_1 
            "[ " COLOR_ALT_TEXT_1 "%2d" COLOR_DECOR_1 " ] "
            "[ " COLOR_ALT_TEXT_1 "%1d" COLOR_DECOR_1 " ] "
            "[ " COLOR_ALT_TEXT_1 "%7d" COLOR_DECOR_1 " ] " 
            COLOR_ALT_TEXT_2 "%-20.20s" COLOR_EOL,
            i + 1,
            sector_name ? sector_name : "Unknown",
            spawn->quantity,
            spawn->respawn_timer,
            spawn->vnum,
            obj_proto ? obj_proto->short_descr : "Unknown");
    }
}
