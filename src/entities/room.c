////////////////////////////////////////////////////////////////////////////////
// room.c
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

#include "room.h"

#include "comm.h"
#include "db.h"

#include "room_exit.h"
#include "extra_desc.h"
#include "reset.h"

int room_count;
int room_perm_count;
Room* room_free;

int room_data_count;
int room_data_perm_count;
RoomData* room_data_free;
RoomData* room_data_hash[MAX_KEY_HASH];

VNUM top_vnum_room;

Room* new_room(RoomData* room_data, Area* area)
{
    LIST_ALLOC_PERM(room, Room);

    room->data = room_data;
    room->next_instance = room_data->instances;
    room_data->instances = room;

    room->area = area;
    room->vnum = room_data->vnum;

    int hash = room->vnum % AREA_ROOM_VNUM_HASH_SIZE;

    ORDERED_INSERT(Room, room, area->rooms[hash], vnum);

    //room->next = area->rooms[hash];
    //area->rooms[hash] = room;
    return room;
}

void free_room(Room* room)
{
    if (room == room->data->instances)
        room->data->instances = room->next_instance;

    int hash = room->vnum % AREA_ROOM_VNUM_HASH_SIZE;
    Area* area = room->area;

    UNORDERED_REMOVE(Room, room, area->rooms[hash], vnum, room->vnum);

    //if (room == area->rooms[hash])
    //    area->rooms[hash] = room->next;
    //else {
    //    Room* iter;
    //    for (iter = area->rooms[hash]; iter && iter->next != room; NEXT_LINK(iter))
    //        ;
    //    if (!iter) {
    //        bugf("Could not delete room %s (%d); not present in area",
    //            room->data->name, room->data->vnum);
    //        return;
    //    }
    //    iter->next = room->next;
    //}


    LIST_FREE(room);
}

RoomData* new_room_data()
{
    LIST_ALLOC_PERM(room_data, RoomData);

    room_data->name = &str_empty[0];
    room_data->description = &str_empty[0];
    room_data->owner = &str_empty[0];
    room_data->heal_rate = 100;
    room_data->mana_rate = 100;

    return room_data;
}

void free_room_data(RoomData* room_data)
{
    ExtraDesc* extra;
    Reset* reset;
    int i;

    free_string(room_data->name);
    free_string(room_data->description);
    free_string(room_data->owner);

    for (i = 0; i < DIR_MAX; i++)
        free_room_exit_data(room_data->exit_data[i]);

    while((extra = room_data->extra_desc) != NULL) {
        NEXT_LINK(room_data->extra_desc);
        free_extra_desc(extra);
    }

    while((reset = room_data->reset_first) != NULL) {
        NEXT_LINK(room_data->reset_first);
        free_reset(reset);
    }

    LIST_FREE(room_data);
    return;
}

/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
RoomData* get_room_data(VNUM vnum)
{
    RoomData* room_data;

    for (room_data = room_data_hash[vnum % MAX_KEY_HASH];
        room_data != NULL;
        NEXT_LINK(room_data)) {
        if (room_data->vnum == vnum)
            return room_data;
    }

    if (fBootDb) {
        bug("get_room_data: bad vnum %"PRVNUM".", vnum);
        exit(1);
    }

    return NULL;
}

Room* get_room(Area* search_context, VNUM vnum)
{
    Area* search_area = NULL;
    RoomData* room_data = get_room_data(vnum);

    if (!room_data) {
        if (/*fBootDb*/1) {
            bug("get_room: bad vnum %"PRVNUM".", vnum);
            exit(1);
        }
        return NULL;
    }

    if (search_context && room_data->area_data == search_context->data) {
        // The current area and the given VNUM share the same AreaData; find the 
        // room in the same instance.
        search_area = search_context;
    }
    else if (room_data->area_data->inst_type == AREA_INST_NONE) {
        // The target area isn't instanced; it only has one Area object.
        search_area = room_data->area_data->instances;
    }
    else {
        // TODO: Lookup available instances for room_data
        return NULL;
    }

    int hash = vnum % AREA_ROOM_VNUM_HASH_SIZE;
    Room* room = NULL;
    ORDERED_GET(Room, room, search_area->rooms[hash], vnum, vnum);

    //FOR_EACH(room, search_area->rooms[vnum % AREA_ROOM_VNUM_HASH_SIZE]) {
    //    if (room->vnum == vnum)
    //        return room;
    //}

    if (fBootDb && !room) {
        bug("get_room: bad vnum %"PRVNUM".", vnum);
        exit(1);
    }

    return room;
}
