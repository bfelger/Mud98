////////////////////////////////////////////////////////////////////////////////
// craft_olc.h
// Shared OLC helper functions for crafting material lists
////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef MUD98__CRAFT__CRAFT_OLC_H
#define MUD98__CRAFT__CRAFT_OLC_H

#include "craft.h"
#include "merc.h"

#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/area.h>

////////////////////////////////////////////////////////////////////////////////
// Material List Management
////////////////////////////////////////////////////////////////////////////////

// Add VNUM to a dynamic array, checking for ITEM_MAT type
// Returns true if added successfully
bool craft_olc_add_mat(VNUM** list, int16_t* count, VNUM vnum, Mobile* ch);

// Remove VNUM from list by VNUM or 1-based index string
// Returns true if removed successfully
bool craft_olc_remove_mat(VNUM** list, int16_t* count, const char* arg, Mobile* ch);

// Clear all VNUMs from list
void craft_olc_clear_mats(VNUM** list, int16_t* count);

// Display current mat list to character
void craft_olc_show_mats(VNUM* list, int16_t count, Mobile* ch, const char* label);

// List available ITEM_MAT objects in area with optional type filter
// If filter is MAT_NONE, lists all ITEM_MAT objects
void craft_olc_list_mats(AreaData* area, CraftMatType filter, Mobile* ch);

#endif // !MUD98__CRAFT__CRAFT_OLC_H
