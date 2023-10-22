////////////////////////////////////////////////////////////////////////////////
// room.h
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

#include "room.h"

#include "db.h"

#include "room_exit.h"
#include "extra_desc.h"
#include "reset.h"

int room_count;
VNUM top_vnum_room;
Room* room_free;
Room* room_vnum_hash[MAX_KEY_HASH];

void free_room(Room* room)
{
    ExtraDesc* extra;
    Reset* reset;
    int i;

    free_string(room->name);
    free_string(room->description);
    free_string(room->owner);

    for (i = 0; i < DIR_MAX; i++)
        free_room_exit(room->exit[i]);

    FOR_EACH(extra, room->extra_desc) {
        free_extra_desc(extra);
    }

    FOR_EACH(reset, room->reset_first) {
        free_reset(reset);
    }

    room->next = room_free;
    room_free = room;
    return;
}

/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
Room* get_room(VNUM vnum)
{
    Room* room;

    for (room = room_vnum_hash[vnum % MAX_KEY_HASH];
        room != NULL;
        NEXT_LINK(room)) {
        if (room->vnum == vnum)
            return room;
    }

    if (fBootDb) {
        bug("get_room: bad vnum %"PRVNUM".", vnum);
        exit(1);
    }

    return NULL;
}

Room* new_room()
{
    static Room zero = { 0 };
    Room* room;

    if (!room_free) {
        room = alloc_perm(sizeof(*room));
        room_count++;
    }
    else {
        room = room_free;
        NEXT_LINK(room_free);
    }

    *room = zero;

    room->name = &str_empty[0];
    room->description = &str_empty[0];
    room->owner = &str_empty[0];
    room->heal_rate = 100;
    room->mana_rate = 100;

    return room;
}
