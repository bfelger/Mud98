////////////////////////////////////////////////////////////////////////////////
// combat_metrics.h
// Optional combat metric hooks used by tools like combat simulators.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__COMBAT_METRICS_H
#define MUD98__COMBAT_METRICS_H

#include <stdbool.h>

typedef struct mobile_t Mobile;

typedef struct combat_metrics_ops_t {
    void (*on_attack_roll)(Mobile* attacker, Mobile* victim, bool hit);
    void (*on_parry)(Mobile* attacker, Mobile* victim);
    void (*on_dodge)(Mobile* attacker, Mobile* victim);
    void (*on_shield_block)(Mobile* attacker, Mobile* victim);
} CombatMetricsOps;

extern CombatMetricsOps* combat_metrics;

static inline void combat_metrics_attack_roll(Mobile* attacker, Mobile* victim, bool hit)
{
    if (combat_metrics && combat_metrics->on_attack_roll) {
        combat_metrics->on_attack_roll(attacker, victim, hit);
    }
}

static inline void combat_metrics_parry(Mobile* attacker, Mobile* victim)
{
    if (combat_metrics && combat_metrics->on_parry) {
        combat_metrics->on_parry(attacker, victim);
    }
}

static inline void combat_metrics_dodge(Mobile* attacker, Mobile* victim)
{
    if (combat_metrics && combat_metrics->on_dodge) {
        combat_metrics->on_dodge(attacker, victim);
    }
}

static inline void combat_metrics_shield_block(Mobile* attacker, Mobile* victim)
{
    if (combat_metrics && combat_metrics->on_shield_block) {
        combat_metrics->on_shield_block(attacker, victim);
    }
}

#endif // MUD98__COMBAT_METRICS_H
