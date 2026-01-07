////////////////////////////////////////////////////////////////////////////////
// metrics.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98_COMBAT_SIM_METRICS_H
#define MUD98_COMBAT_SIM_METRICS_H

#include <stdbool.h>
#include <stdio.h>

typedef struct sim_run_metrics_t {
    int ticks;
    int total_damage;
    bool attacker_won;
    bool defender_won;
    bool timed_out;
    int spells_cast;
    int mana_spent;
    int attack_rolls;
    int hit_count;
    int miss_count;
    int parry_count;
    int dodge_count;
    int block_count;
} SimRunMetrics;

typedef struct sim_aggregate_metrics_t {
    int runs;
    int completed;
    int timeouts;
    int attacker_wins;
    int defender_wins;
    long long total_ticks;
    long long total_ticks_completed;
    long long total_damage;
    long long total_spells_cast;
    long long total_mana_spent;
    long long total_attack_rolls;
    long long total_hits;
    long long total_misses;
    long long total_parries;
    long long total_dodges;
    long long total_blocks;
} SimAggregateMetrics;

void sim_metrics_init(SimAggregateMetrics* metrics);
void sim_metrics_add_run(SimAggregateMetrics* metrics, const SimRunMetrics* run);
void sim_metrics_print_summary(const SimAggregateMetrics* metrics, FILE* out);

#endif // MUD98_COMBAT_SIM_METRICS_H
