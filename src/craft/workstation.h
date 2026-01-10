////////////////////////////////////////////////////////////////////////////////
// craft/workstation.h
// Workstation detection and validation helpers.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__CRAFT__WORKSTATION_H
#define MUD98__CRAFT__WORKSTATION_H

#include "craft.h"
#include "recipe.h"

#include <entities/room.h>
#include <entities/object.h>

// Find any workstation in room matching type flags
// Returns first matching workstation or NULL if none found
Object* find_workstation(Room* room, WorkstationType type);

// Find specific workstation by VNUM
// Returns workstation with matching prototype VNUM or NULL
Object* find_workstation_by_vnum(Room* room, VNUM vnum);

// Check if room has required workstation for recipe
// Returns true if recipe has no workstation requirement, or room has matching station
// Optionally returns the matched workstation via out_station.
bool has_required_workstation(Room* room, Recipe* recipe, Object** out_station);

#endif // !MUD98__CRAFT__WORKSTATION_H
