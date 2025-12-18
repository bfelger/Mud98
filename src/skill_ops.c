////////////////////////////////////////////////////////////////////////////////
// skill_ops.c - ROM skill check implementation
////////////////////////////////////////////////////////////////////////////////

#include "skill_ops.h"

#include "data/skill.h"
#include "entities/mobile.h"
#include "lookup.h"
#include "rng.h"

#include <stdbool.h>

// Forward declarations for ROM skill check implementations
static bool rom_skill_check(Mobile* ch, SKNUM sn, Mobile* victim);
static bool rom_skill_check_simple(Mobile* ch, SKNUM sn);
static bool rom_skill_check_modified(Mobile* ch, SKNUM sn, int chance);

// Global skill operations pointer (defaults to ROM)
SkillOps* skill_ops = &rom_skill_ops;

// ROM skill operations implementation
SkillOps rom_skill_ops = {
    .check = rom_skill_check,
    .check_simple = rom_skill_check_simple,
    .check_modified = rom_skill_check_modified,
};

////////////////////////////////////////////////////////////////////////////////
// ROM Skill Check Implementation
//
// Encapsulates the full skill check calculation:
// 1. Get base skill percentage from character
// 2. Apply modifiers based on skill type and context
// 3. Roll against final chance
////////////////////////////////////////////////////////////////////////////////

static bool rom_skill_check(Mobile* ch, SKNUM sn, Mobile* victim)
{
    int chance;
    
    if (ch == NULL || sn < 0 || sn >= skill_count)
        return false;
    
    // Base chance is character's skill percentage
    chance = get_skill(ch, sn);
    
    if (chance <= 0)
        return false;
    
    // Apply skill-specific modifiers based on skill type
    // For now, we'll use a simplified version - full modifiers will be
    // moved here when we refactor each skill
    
    // Level difference bonus (common across most skills)
    if (victim != NULL) {
        chance += (ch->level - victim->level) * 2;
    }
    
    // Dex modifier (common for physical skills)
    // This is a simplified version - specific skills have their own formulas
    if (victim != NULL) {
        chance += get_curr_stat(ch, STAT_DEX) / 2;
        chance -= get_curr_stat(victim, STAT_DEX) / 3;
    }
    
    // Size modifier for appropriate skills
    if (victim != NULL && sn == gsn_trip) {
        if (ch->size < victim->size)
            chance += (ch->size - victim->size) * 10;
    }
    
    // Speed modifiers
    if (victim != NULL) {
        if (IS_AFFECTED(ch, AFF_HASTE))
            chance += 10;
        if (IS_AFFECTED(victim, AFF_HASTE))
            chance -= 20;
    }
    
    // Roll against final chance
    return number_percent() < chance;
}

static bool rom_skill_check_simple(Mobile* ch, SKNUM sn)
{
    int chance;
    
    if (ch == NULL || sn < 0 || sn >= skill_count)
        return false;
    
    chance = get_skill(ch, sn);
    
    if (chance <= 0)
        return false;
    
    return number_percent() < chance;
}

// Check skill with pre-calculated chance
// This allows skills to maintain their unique modifier calculations
// while still using the seam for RNG testability
static bool rom_skill_check_modified(Mobile* ch, SKNUM sn, int chance)
{
    (void)ch;  // May be used for logging/debugging in future
    (void)sn;  // May be used for logging/debugging in future
    
    if (chance <= 0)
        return false;
    
    return number_percent() < chance;
}
