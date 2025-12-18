////////////////////////////////////////////////////////////////////////////////
// mock_skill_ops.h - Mock skill operations for testing
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98_MOCK_SKILL_OPS_H
#define MUD98_MOCK_SKILL_OPS_H

#include "data/skill.h"

#include <stdbool.h>

// Forward declaration
typedef struct skill_ops_t SkillOps;

// Mock skill operations instance
extern SkillOps mock_skill_ops;

// Set expected result for a specific skill check
// sn: Skill number
// succeeds: true if skill should succeed, false if it should fail
void set_skill_check_result(SKNUM sn, bool succeeds);

// Clear all skill check results (reset to default fail)
void clear_skill_check_results(void);

#endif // MUD98_MOCK_SKILL_OPS_H
