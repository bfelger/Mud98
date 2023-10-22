////////////////////////////////////////////////////////////////////////////////
// room_exit.c
// Utilities to handle room exits
////////////////////////////////////////////////////////////////////////////////

#include "room_exit.h"

#include "db.h"

int room_exit_count;
RoomExit* room_exit_free;

RoomExit* new_room_exit()
{
    RoomExit zero = { 0 };
    RoomExit* room_exit;

    if (!room_exit_free) {
        room_exit = alloc_perm(sizeof(*room_exit));
        room_exit_count++;
    }
    else {
        room_exit = room_exit_free;
        NEXT_LINK(room_exit_free);
    }

    *room_exit = zero;
    room_exit->keyword = &str_empty[0];
    room_exit->description = &str_empty[0];

    return room_exit;
}

void free_room_exit(RoomExit* room_exit)
{
    if (room_exit == NULL)
        return;

    free_string(room_exit->keyword);
    free_string(room_exit->description);

    room_exit->next = room_exit_free;
    room_exit_free = room_exit;

    return;
}
