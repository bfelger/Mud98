////////////////////////////////////////////////////////////////////////////////
// craft/workstation.c
// Workstation detection and validation implementations.
////////////////////////////////////////////////////////////////////////////////

#include "workstation.h"

#include <entities/entity.h>
#include <entities/object.h>
#include <entities/room.h>

#include <data/item.h>

Object* find_workstation(Room* room, WorkstationType type)
{
    if (room == NULL || type == WORK_NONE)
        return NULL;
    
    Object* obj;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_WORKSTATION
            && (obj->workstation.station_flags & type) == type) {
            return obj;
        }
    }
    return NULL;
}

Object* find_workstation_by_vnum(Room* room, VNUM vnum)
{
    if (room == NULL || vnum == VNUM_NONE)
        return NULL;
    
    Object* obj;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_WORKSTATION
            && obj->prototype
            && VNUM_FIELD(obj->prototype) == vnum) {
            return obj;
        }
    }
    return NULL;
}

bool has_required_workstation(Room* room, Recipe* recipe, Object** out_station)
{
    if (recipe == NULL)
        return false;

    if (out_station)
        *out_station = NULL;
    
    // Check VNUM-based requirement first
    if (recipe->station_vnum != VNUM_NONE && recipe->station_vnum != 0) {
        Object* station = find_workstation_by_vnum(room, recipe->station_vnum);
        if (out_station)
            *out_station = station;
        return station != NULL;
    }
    
    // Check type-based requirement
    if (recipe->station_type != WORK_NONE) {
        Object* station = find_workstation(room, recipe->station_type);
        if (out_station)
            *out_station = station;
        return station != NULL;
    }
    
    // No workstation required
    return true;
}
