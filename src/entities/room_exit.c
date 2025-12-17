////////////////////////////////////////////////////////////////////////////////
// entities/room_exit.c
// Utilities to handle room exits
////////////////////////////////////////////////////////////////////////////////

#include "room_exit.h"

#include <db.h>

#include <lox/vm.h>


int room_exit_count;
int room_exit_perm_count;
RoomExit* room_exit_free;

int room_exit_data_count;
int room_exit_data_perm_count;
RoomExitData* room_exit_data_free;

RoomExit* new_room_exit(RoomExitData* room_exit_data, Room* from)
{
    LIST_ALLOC_PERM(room_exit, RoomExit);

    gc_protect(OBJ_VAL(room_exit));
    init_header(&room_exit->header, OBJ_ROOM_EXIT);

    room_exit->data = room_exit_data;
    room_exit->from_room = from;
    room_exit->exit_flags = room_exit_data->exit_reset_flags;

    if (room_exit_data->to_room == NULL || room_exit_data->to_room->header.obj.type != OBJ_ROOM_DATA || room_exit_data->to_room->instances.front == NULL) {
        room_exit->to_room = NULL;
        gc_protect_clear();
        return room_exit;
    }
    
    if (from->area->data == room_exit_data->to_room->area_data) {
        room_exit->to_room = get_room(from->area, room_exit_data->to_vnum);
    }
    else if (room_exit_data->to_room->area_data->inst_type != AREA_INST_MULTI) {
        // Different area, but single-instance: there's only one possible room
        room_exit->to_room = AS_ROOM(room_exit_data->to_room->instances.front->value);
    }
    // else: Different area AND multi-instance: leave to_room = NULL
    // This is INTENTIONAL! Multi-instance area exits are resolved lazily
    // per-player via get_room_for_player() when move_char() is called.
    // Each player gets their own instance of the target area.
    // The to_vnum is stored in room_exit_data for lazy resolution.

    // Add to target room's inbound exits list
    if (room_exit->to_room) {
        list_push_back(&room_exit->to_room->inbound_exits, OBJ_VAL(room_exit));
    }

    gc_protect_clear();
    return room_exit;
}

void free_room_exit(RoomExit* room_exit)
{
    if (room_exit == NULL)
        return;

    // Remove from target room's inbound exits list
    if (room_exit->to_room) {
        list_remove_value(&room_exit->to_room->inbound_exits, OBJ_VAL(room_exit));
    }

    LIST_FREE(room_exit);
}

RoomExitData* new_room_exit_data()
{
    LIST_ALLOC_PERM(room_exit_data, RoomExitData);

    gc_protect(OBJ_VAL(room_exit_data));
    init_header(&room_exit_data->header, OBJ_ROOM_EXIT_DATA);

    room_exit_data->keyword = &str_empty[0];
    room_exit_data->description = &str_empty[0];
    room_exit_data->key = -1;

    gc_protect_clear();
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
