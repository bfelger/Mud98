////////////////////////////////////////////////////////////////////////////////
// craft/craft.h
// Crafting system enums and shared types.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__CRAFT__CRAFT_H
#define MUD98__CRAFT__CRAFT_H

#include "merc.h"

////////////////////////////////////////////////////////////////////////////////
// Material Type Enumeration
////////////////////////////////////////////////////////////////////////////////

// Types of crafting materials (harvested or processed)
typedef enum craft_mat_type_t {
    MAT_NONE = 0,       // Invalid/unset
    MAT_HIDE,           // Raw animal skin (from skinning)
    MAT_LEATHER,        // Processed hide
    MAT_FUR,            // Pelts with fur intact
    MAT_SCALE,          // Reptile/fish scales
    MAT_BONE,           // Bones and horns
    MAT_MEAT,           // Edible flesh
    MAT_ORE,            // Unprocessed metal ore
    MAT_INGOT,          // Smelted metal bars
    MAT_GEM,            // Precious stones
    MAT_CLOTH,          // Woven fabric
    MAT_WOOD,           // Lumber
    MAT_HERB,           // Alchemical plants
    MAT_ESSENCE,        // Magical essence
    MAT_COMPONENT,      // Spell components
    CRAFT_MAT_TYPE_COUNT
} CraftMatType;

////////////////////////////////////////////////////////////////////////////////
// Workstation Type Flags
////////////////////////////////////////////////////////////////////////////////

// Workstation capabilities (bit flags - stations can have multiple)
typedef enum workstation_type_t {
    WORK_NONE     = 0,
    WORK_FORGE    = (1 << 0),   // Blacksmithing
    WORK_SMELTER  = (1 << 1),   // Ore to ingot
    WORK_TANNERY  = (1 << 2),   // Hide to leather
    WORK_LOOM     = (1 << 3),   // Cloth weaving
    WORK_ALCHEMY  = (1 << 4),   // Potion brewing
    WORK_COOKING  = (1 << 5),   // Food preparation
    WORK_ENCHANT  = (1 << 6),   // Magical enhancement
    WORK_WOODWORK = (1 << 7),   // Carpentry
    WORK_JEWELER  = (1 << 8),   // Jewelry crafting
} WorkstationType;

////////////////////////////////////////////////////////////////////////////////
// Corpse Extraction Flags
////////////////////////////////////////////////////////////////////////////////

// Flags stored on corpse objects to track which extraction commands applied
#define CORPSE_SKINNED      BIT(0)
#define CORPSE_BUTCHERED    BIT(1)

////////////////////////////////////////////////////////////////////////////////
// Recipe Discovery Type
////////////////////////////////////////////////////////////////////////////////

// How a recipe becomes known to a player
typedef enum discovery_type_t {
    DISC_KNOWN = 0,     // Always known (default/tutorial recipes)
    DISC_TRAINER,       // Learned from NPC trainer
    DISC_SCROLL,        // Learned from recipe scroll item
    DISC_DISCOVERY,     // Discovered through experimentation
    DISC_QUEST,         // Unlocked by quest completion
    DISCOVERY_TYPE_COUNT
} DiscoveryType;

////////////////////////////////////////////////////////////////////////////////
// Lookup Functions
////////////////////////////////////////////////////////////////////////////////

// Material type name/lookup
const char* craft_mat_type_name(CraftMatType type);
CraftMatType craft_mat_type_lookup(const char* name);

// Workstation type name/lookup (handles bit flags)
const char* workstation_type_name(WorkstationType type);
WorkstationType workstation_type_lookup(const char* name);
void workstation_type_flags_string(WorkstationType flags, char* buf, size_t buflen);

// Discovery type name/lookup
const char* discovery_type_name(DiscoveryType type);
DiscoveryType discovery_type_lookup(const char* name);

#endif // MUD98__CRAFT__CRAFT_H
