////////////////////////////////////////////////////////////////////////////////
// craft_skill.h
//
// Crafting skill check system - determines success and quality of crafting
////////////////////////////////////////////////////////////////////////////////

#ifndef MUD98_CRAFT_CRAFT_SKILL_H
#define MUD98_CRAFT_CRAFT_SKILL_H

#include <merc.h>
#include <data/skill.h>

#include "recipe.h"

////////////////////////////////////////////////////////////////////////////////
// Skill Improvement Rate Constants
////////////////////////////////////////////////////////////////////////////////

// Multiplier for check_improve - lower = faster improvement
#define CRAFT_IMPROVE_EASY    4   // Simple recipes (10+ levels below player)
#define CRAFT_IMPROVE_MEDIUM  3   // Standard recipes (within 5 levels)
#define CRAFT_IMPROVE_HARD    2   // Complex recipes (5+ levels above player)

////////////////////////////////////////////////////////////////////////////////
// Quality Tiers
////////////////////////////////////////////////////////////////////////////////

typedef enum {
    QUALITY_POOR = 0,
    QUALITY_NORMAL = 1,
    QUALITY_FINE = 2,
    QUALITY_EXCEPTIONAL = 3,
    QUALITY_MASTERWORK = 4
} QualityTier;

const char* quality_tier_name(QualityTier tier);
QualityTier quality_tier_from_margin(int margin);

////////////////////////////////////////////////////////////////////////////////
// Crafting Skill Check Result
////////////////////////////////////////////////////////////////////////////////

typedef struct craft_check_result_t {
    bool success;           // Did the check pass?
    int quality;            // Quality margin (0 = min, higher = better)
    int roll;               // Actual roll (for debugging/display)
    int target;             // Target number
    SKNUM skill_used;       // Which skill was checked
} CraftCheckResult;

////////////////////////////////////////////////////////////////////////////////
// Crafting Skill Functions
////////////////////////////////////////////////////////////////////////////////

// Perform skill check for recipe (uses game RNG)
CraftCheckResult craft_skill_check(Mobile* ch, Recipe* recipe);

// Calculate effective skill percentage
int craft_effective_skill(Mobile* ch, SKNUM sn);

// Get skill improvement multiplier based on recipe difficulty
// Lower = faster improvement (hard recipes improve faster)
int craft_improve_multiplier(Recipe* recipe, Mobile* ch);

// Pure evaluation function (testable - injects roll)
CraftCheckResult evaluate_craft_check(
    int skill_pct,      // Player's skill percentage
    int player_level,   // Player's level
    int recipe_level,   // Recipe's minimum level
    int roll            // The d100 roll (injected for testing)
);

// Check to see if character has required crafting tool equipped
bool has_crafting_tool(Mobile* ch, WeaponType required_tool);

#endif // MUD98_CRAFT_CRAFT_SKILL_H
