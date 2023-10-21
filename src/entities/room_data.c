////////////////////////////////////////////////////////////////////////////////
// room_data.h
// Utilities to handle navigable rooms
////////////////////////////////////////////////////////////////////////////////

#include "room_data.h"

#include "db.h"

#include "exit_data.h"
#include "extra_desc.h"
#include "reset_data.h"

int room_count;
VNUM top_vnum_room;
RoomData* room_index_free;
RoomData* room_index_hash[MAX_KEY_HASH];

void free_room_index(RoomData* pRoom)
{
    ExtraDesc* pExtra;
    ResetData* pReset;
    int i;

    free_string(pRoom->name);
    free_string(pRoom->description);
    free_string(pRoom->owner);

    for (i = 0; i < DIR_MAX; i++)
        free_exit(pRoom->exit[i]);

    FOR_EACH(pExtra, pRoom->extra_desc) {
        free_extra_desc(pExtra);
    }

    FOR_EACH(pReset, pRoom->reset_first) {
        free_reset_data(pReset);
    }

    pRoom->next = room_index_free;
    room_index_free = pRoom;
    return;
}

/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
RoomData* get_room_data(VNUM vnum)
{
    RoomData* pRoomIndex;

    for (pRoomIndex = room_index_hash[vnum % MAX_KEY_HASH]; pRoomIndex != NULL;
        NEXT_LINK(pRoomIndex)) {
        if (pRoomIndex->vnum == vnum) 
            return pRoomIndex;
    }

    if (fBootDb) {
        bug("Get_room_index: bad vnum %"PRVNUM".", vnum);
        exit(1);
    }

    return NULL;
}

RoomData* new_room_index()
{
    static RoomData rZero;
    RoomData* pRoom;

    if (!room_index_free) {
        pRoom = alloc_perm(sizeof(*pRoom));
        room_count++;
    }
    else {
        pRoom = room_index_free;
        NEXT_LINK(room_index_free);
    }

    *pRoom = rZero;

    pRoom->name = &str_empty[0];
    pRoom->description = &str_empty[0];
    pRoom->owner = &str_empty[0];
    pRoom->heal_rate = 100;
    pRoom->mana_rate = 100;

    return pRoom;
}
