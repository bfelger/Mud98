////////////////////////////////////////////////////////////////////////////////
// craft/act_craft.h
// Crafting command declarations and helper functions.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__CRAFT__ACT_CRAFT_H
#define MUD98__CRAFT__ACT_CRAFT_H

#include "craft.h"
#include "craft_skill.h"
#include "recipe.h"

#include <entities/mobile.h>
#include <entities/object.h>

#include <stdbool.h>

////////////////////////////////////////////////////////////////////////////////
// Material Type Classification
////////////////////////////////////////////////////////////////////////////////

// Check if a material type can be obtained via skinning
bool is_skinnable_type(CraftMatType type);

// Check if a material type can be obtained via butchering
bool is_butcherable_type(CraftMatType type);

////////////////////////////////////////////////////////////////////////////////
// Corpse Extraction Helpers
////////////////////////////////////////////////////////////////////////////////

// Result of evaluating an extraction operation
typedef struct extraction_result_t {
    bool success;           // Can extraction proceed?
    bool skill_passed;      // Did skill check pass? (only valid if success)
    const char* error_msg;  // Error message if !success
    VNUM* mat_vnums;        // Array of material VNUMs to extract
    int mat_count;          // Number of materials
    SKNUM skill_used;       // Skill used for the check
    int skill_roll;         // The roll made (for testing)
    int skill_target;       // The target number (for testing)
} ExtractionResult;

// Evaluate whether skinning can proceed (does not modify world state)
ExtractionResult evaluate_skin(Mobile* ch, Object* corpse);

// Evaluate whether butchering can proceed (does not modify world state)
ExtractionResult evaluate_butcher(Mobile* ch, Object* corpse);

// Free resources allocated by evaluate_* functions
void free_extraction_result(ExtractionResult* result);

// Apply extraction results (create materials, set flags, send messages)
void apply_extraction(Mobile* ch, Object* corpse, ExtractionResult* result,
                      int corpse_flag, const char* verb);

////////////////////////////////////////////////////////////////////////////////
// Salvage Helpers
////////////////////////////////////////////////////////////////////////////////

// Result of evaluating a salvage operation
typedef struct salvage_result_t {
    bool success;           // Can salvage proceed?
    bool skill_passed;      // Did skill check pass? (only valid if has_skill)
    bool has_skill;         // Does player have the relevant skill?
    const char* error_msg;  // Error message if !success
    VNUM* mat_vnums;        // Array of material VNUMs recovered
    int mat_count;          // Number of materials (may be random)
    SKNUM skill_used;       // Skill used for the check (-1 if none)
    int skill_pct;          // Player's skill percentage
    int skill_roll;         // The roll made (for testing)
} SalvageResult;

// Determine which skill is used for salvaging an object
// Returns -1 if no skill is required
SKNUM get_salvage_skill(Object* obj);

// Evaluate whether salvage can proceed
SalvageResult evaluate_salvage(Mobile* ch, Object* obj);

// Free resources allocated by evaluate_salvage
void free_salvage_result(SalvageResult* result);

////////////////////////////////////////////////////////////////////////////////
// Crafting Helpers
////////////////////////////////////////////////////////////////////////////////

// Result of evaluating a craft operation
typedef struct craft_result_t {
    bool can_attempt;       // Can crafting be attempted?
    bool success;           // Did the skill check succeed?
    const char* error_msg;  // Error message if !can_attempt
    Recipe* recipe;         // The matched recipe (if found)
    CraftCheckResult check; // Detailed skill check result
} CraftResult;

// Evaluate whether crafting can proceed
CraftResult evaluate_craft(Mobile* ch, const char* recipe_name);

// Check if character has all required materials
bool has_materials(Mobile* ch, Recipe* recipe);

// Consume materials for a recipe (modifies world state)
void consume_materials(Mobile* ch, Recipe* recipe);

// Consume partial materials (50% of ingredient types) on failure
void consume_materials_partial(Mobile* ch, Recipe* recipe);

// Calculate output quality based on skill check margin
int calculate_craft_quality(Mobile* ch, Recipe* recipe);

////////////////////////////////////////////////////////////////////////////////
// Recipe Knowledge
////////////////////////////////////////////////////////////////////////////////

// Check if character knows a recipe (based on discovery type)
bool knows_recipe(Mobile* ch, Recipe* recipe);

// Find a recipe by name that the character knows
Recipe* find_known_recipe(Mobile* ch, const char* name);

#endif // MUD98__CRAFT__ACT_CRAFT_H
