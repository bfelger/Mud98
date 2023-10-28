////////////////////////////////////////////////////////////////////////////////
// area.c
// Utilities to handle area
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "area.h"

#include "db.h"

#include "data/direction.h"

int area_data_count;
int area_data_perm_count;
AreaData* area_data_list;
AreaData* area_data_last;
AreaData* area_data_free;

int area_count;
int area_perm_count;
Area* area_free;

Area* new_area(AreaData* area_data)
{
    LIST_ALLOC_PERM(area, Area);

    area->data = area_data;
    area->empty = true;
    area->reset_timer = area->data->reset_thresh;

    return area;
}

void free_area(Area* area)
{
    ResetCounter* counter = NULL;
    while ((counter = area->obj_counts) != NULL) {
        NEXT_LINK(area->obj_counts);
        free_reset_counter(counter);
    }

    while ((counter = area->mob_counts) != NULL) {
        NEXT_LINK(area->mob_counts);
        free_reset_counter(counter);
    }

    LIST_FREE(area);
}

AreaData* new_area_data()
{
    char buf[MAX_INPUT_LENGTH];

    LIST_ALLOC_PERM(area_data, AreaData);

    area_data->name = str_empty;
    area_data->area_flags = AREA_ADDED;
    area_data->security = 1;
    area_data->builders = str_empty;
    area_data->credits = str_empty;
    area_data->reset_thresh = 6;
    area_data->vnum = area_data_count - 1;
    sprintf(buf, "area%"PRVNUM".are", area_data->vnum);
    area_data->file_name = str_dup(buf);

    return area_data;
}

void free_area_data(AreaData* area_data)
{
    free_string(area_data->name);
    free_string(area_data->file_name);
    free_string(area_data->builders);
    free_string(area_data->credits);

    LIST_FREE(area_data);
}

Area* create_area_instance(AreaData* area_data, bool create_exits)
{
    Area* area = new_area(area_data);
    area->next = area_data->instances;
    area_data->instances = area;

    Room* room;
    RoomData* room_data;
    for (int hash = 0; hash < MAX_KEY_HASH; ++hash) {
        FOR_EACH(room_data, room_data_hash[hash]) {
            if (room_data->area_data == area_data) {
                room = new_room(room_data, area);
            }
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
    for (int hash = 0; hash < AREA_ROOM_VNUM_HASH_SIZE; ++hash) {
        FOR_EACH(room, area->rooms[hash]) {
            for (int i = 0; i < DIR_MAX; ++i) {
                if (room->data->exit_data[i])
                    room->exit[i] = new_room_exit(room->data->exit_data[i], room);
            }
        }
    }
}







