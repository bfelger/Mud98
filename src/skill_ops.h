////////////////////////////////////////////////////////////////////////////////
// skill_ops.h - Skill check operation seam for testability
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98_SKILL_OPS_H
#define MUD98_SKILL_OPS_H

#include "merc.h"

// Forward declarations
typedef struct mobile_t Mobile;
typedef int16_t SKNUM;

////////////////////////////////////////////////////////////////////////////////
// Skill Check Operations
//
// Provides a seam for skill success checks, allowing tests to control
// skill outcomes without manipulating RNG, stats, or complex modifiers.
//
// Production: Uses full ROM skill calculation (chance + modifiers vs RNG)
// Testing: Direct control over pass/fail via mock implementation
////////////////////////////////////////////////////////////////////////////////

typedef struct skill_ops_t {
    // Check skill with opponent (combat/target skills)
    // Returns true if skill check succeeds
    // ch: Character performing the skill
    // sn: Skill number
    // victim: Target of the skill (for opposed checks)
    bool (*check)(Mobile* ch, SKNUM sn, Mobile* victim);
    
    // Check skill without opponent (simple success check)
    // Used for non-combat skills like hide, sneak, etc.
    // ch: Character performing the skill
    // sn: Skill number
    bool (*check_simple)(Mobile* ch, SKNUM sn);
    
    // Check skill with pre-calculated chance (for skills with unique modifiers)
    // Skills with custom modifier logic calculate their own chance value,
    // then pass it here for the RNG check. This allows skills to preserve
    // their unique calculations while still being testable.
    // ch: Character performing the skill
    // sn: Skill number
    // chance: Pre-calculated percentage chance (0-100+)
    bool (*check_modified)(Mobile* ch, SKNUM sn, int chance);
} SkillOps;

// Global skill operations pointer (defaults to ROM implementation)
extern SkillOps* skill_ops;

// ROM implementation (production)
extern SkillOps rom_skill_ops;

// Mock implementation (testing) - defined in tests/mock_skill_ops.c
extern SkillOps mock_skill_ops;

#endif // MUD98_SKILL_OPS_H
