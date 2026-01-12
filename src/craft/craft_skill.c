////////////////////////////////////////////////////////////////////////////////
// craft_skill.c
//
// Crafting skill check system implementation
////////////////////////////////////////////////////////////////////////////////

#include "craft/craft_skill.h"

#include <handler.h>
#include <rng.h>
#include <skill.h>

////////////////////////////////////////////////////////////////////////////////
// Quality Tier Functions
////////////////////////////////////////////////////////////////////////////////

static const char* quality_tier_names[] = {
    "poor",
    "normal",
    "fine",
    "exceptional",
    "masterwork"
};

const char* quality_tier_name(QualityTier tier)
{
    if (tier < 0 || tier > QUALITY_MASTERWORK)
        return "unknown";
    return quality_tier_names[tier];
}

QualityTier quality_tier_from_margin(int margin)
{
    if (margin < 0)
        return QUALITY_POOR;  // Failed, but treat as poor if somehow used
    if (margin < 20)
        return QUALITY_POOR;
    if (margin < 40)
        return QUALITY_NORMAL;
    if (margin < 60)
        return QUALITY_FINE;
    if (margin < 80)
        return QUALITY_EXCEPTIONAL;
    return QUALITY_MASTERWORK;
}

////////////////////////////////////////////////////////////////////////////////
// Skill Check Functions
////////////////////////////////////////////////////////////////////////////////

// Pure evaluation function - testable with injected roll
CraftCheckResult evaluate_craft_check(
    int skill_pct,
    int player_level,
    int recipe_level,
    int roll)
{
    // Level bonus: +2% per level above recipe requirement
    int bonus = (player_level - recipe_level) * 2;
    int target = skill_pct + bonus;
    
    // Clamp target to 5-95 range (always some chance of failure/success)
    target = URANGE(5, target, 95);
    
    CraftCheckResult result = {
        .success = roll <= target,
        .quality = target - roll,
        .roll = roll,
        .target = target,
        .skill_used = -1  // Set by caller if needed
    };
    
    return result;
}

// Calculate effective skill percentage for crafting
int craft_effective_skill(Mobile* ch, SKNUM sn)
{
    if (ch == NULL || sn < 0)
        return 0;
    
    return get_skill(ch, sn);
}

// Perform skill check for recipe (uses game RNG)
CraftCheckResult craft_skill_check(Mobile* ch, Recipe* recipe)
{
    CraftCheckResult result = {0};
    
    if (ch == NULL || recipe == NULL) {
        result.success = false;
        result.skill_used = -1;
        return result;
    }
    
    int skill = craft_effective_skill(ch, recipe->required_skill);
    int roll = number_percent();
    
    result = evaluate_craft_check(skill, ch->level, recipe->min_level, roll);
    result.skill_used = recipe->required_skill;
    
    return result;
}

// Get skill improvement multiplier based on recipe difficulty
int craft_improve_multiplier(Recipe* recipe, Mobile* ch)
{
    if (recipe == NULL || ch == NULL)
        return CRAFT_IMPROVE_MEDIUM;
    
    int level_diff = recipe->min_level - ch->level;
    
    if (level_diff < -10)
        return CRAFT_IMPROVE_EASY;   // Easy - slower improvement (4)
    if (level_diff < 5)
        return CRAFT_IMPROVE_MEDIUM; // Medium - normal improvement (3)
    return CRAFT_IMPROVE_HARD;       // Hard - faster improvement (2)
}

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////

bool has_crafting_tool(Mobile* ch, WeaponType required_tool)
{
    Object* wielded = get_eq_char(ch, WEAR_WIELD);
    if (wielded && wielded->item_type == ITEM_WEAPON) {
        WeaponType wielded_type = wielded->value[0];
        if (wielded_type == required_tool)
            return true;
    }

    return false;
}