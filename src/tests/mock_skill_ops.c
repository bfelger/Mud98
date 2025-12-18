////////////////////////////////////////////////////////////////////////////////
// mock_skill_ops.c - Mock skill operations for deterministic testing
////////////////////////////////////////////////////////////////////////////////

#include "mock_skill_ops.h"

#include "skill_ops.h"
#include "data/skill.h"
#include "merc.h"

#include <stdbool.h>
#include <string.h>

// Maximum number of skills (generous upper bound)
#define MOCK_MAX_SKILLS 1000

// Storage for skill check results (simple array indexed by skill number)
bool skill_check_results[MOCK_MAX_SKILLS];
static bool initialized = false;

// Forward declarations
bool mock_skill_check(Mobile* ch, SKNUM sn, Mobile* victim);
bool mock_skill_check_simple(Mobile* ch, SKNUM sn);
bool mock_skill_check_modified(Mobile* ch, SKNUM sn, int chance);

// Mock skill operations implementation
SkillOps mock_skill_ops = {
    .check = mock_skill_check,
    .check_simple = mock_skill_check_simple,
    .check_modified = mock_skill_check_modified,
};

// Initialize the skill check results array
static void ensure_initialized(void)
{
    if (!initialized) {
        memset(skill_check_results, 0, sizeof(skill_check_results));
        initialized = true;
    }
}

// Set expected result for a skill check
void set_skill_check_result(SKNUM sn, bool succeeds)
{
    ensure_initialized();
    if (sn >= 0 && sn < MOCK_MAX_SKILLS) {
        skill_check_results[sn] = succeeds;
    }
}

// Clear all skill check results
void clear_skill_check_results(void)
{
    if (initialized) {
        memset(skill_check_results, 0, sizeof(skill_check_results));
    }
}

// Mock implementations
bool mock_skill_check(Mobile* ch, SKNUM sn, Mobile* victim)
{
    (void)ch;
    (void)victim;
    
    ensure_initialized();
    
    if (sn >= 0 && sn < MOCK_MAX_SKILLS) {
        return skill_check_results[sn];
    }
    
    return false;
}

bool mock_skill_check_simple(Mobile* ch, SKNUM sn)
{
    return mock_skill_check(ch, sn, NULL);
}

bool mock_skill_check_modified(Mobile* ch, SKNUM sn, int chance)
{
    // Ignore the calculated chance - use our predetermined result
    (void)chance;
    return mock_skill_check(ch, sn, NULL);
}
