////////////////////////////////////////////////////////////////////////////////
// entities/room_exit.c
// Utilities to handle room exits
////////////////////////////////////////////////////////////////////////////////

#include "room_exit.h"

#include <db.h>


int room_exit_count;
int room_exit_perm_count;
RoomExit* room_exit_free;

int room_exit_data_count;
int room_exit_data_perm_count;
RoomExitData* room_exit_data_free;

RoomExit* new_room_exit(RoomExitData* room_exit_data, Room* from)
{
    LIST_ALLOC_PERM(room_exit, RoomExit);

    room_exit->data = room_exit_data;
    room_exit->exit_flags = room_exit_data->exit_reset_flags;

    if (room_exit_data->to_room == NULL || room_exit_data->to_room->instances.front == NULL) {
        room_exit->to_room = NULL;
        return room_exit;
    }
    
    if (from->area->data == room_exit_data->to_room->area_data) {
        room_exit->to_room = get_room(from->area, room_exit_data->to_vnum);
    }
    else if (room_exit_data->to_room->area_data->inst_type != AREA_INST_MULTI) {
        // There is only one possible room.
        // It could be NULL.
        // Check before entering.
        room_exit->to_room = AS_ROOM(room_exit_data->to_room->instances.front->value);
        // DO NOT CREATE INSTANCED-TO-INSTANCED ROOM EXITS BETWEEN DIFFERENT
        // AREAS!
        // TODO: Add OLC code to prevent this from happening. Require a non-
        // instanced "buffer" between multi-instanced areas.
    }

    return room_exit;
}

void free_room_exit(RoomExit* room_exit)
{
    if (room_exit == NULL)
        return;

    LIST_FREE(room_exit);
}

RoomExitData* new_room_exit_data()
{
    LIST_ALLOC_PERM(room_exit_data, RoomExitData);

    room_exit_data->keyword = &str_empty[0];
    room_exit_data->description = &str_empty[0];

    return room_exit_data;
}

void free_room_exit_data(RoomExitData* room_exit_data)
{
    if (room_exit_data == NULL)
        return;

    free_string(room_exit_data->keyword);
    free_string(room_exit_data->description);

    LIST_FREE(room_exit_data);
}
