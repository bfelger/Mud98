////////////////////////////////////////////////////////////////////////////////
// metrics.c
////////////////////////////////////////////////////////////////////////////////

#include "metrics.h"

#include <merc.h>

#include <stdio.h>

void sim_metrics_init(SimAggregateMetrics* metrics)
{
    if (!metrics) {
        return;
    }

    *metrics = (SimAggregateMetrics){ 0 };
}

void sim_metrics_add_run(SimAggregateMetrics* metrics, const SimRunMetrics* run)
{
    if (!metrics || !run) {
        return;
    }

    metrics->runs++;
    metrics->total_ticks += run->ticks;
    metrics->total_damage += run->total_damage;
    metrics->total_spells_cast += run->spells_cast;
    metrics->total_mana_spent += run->mana_spent;
    metrics->total_attack_rolls += run->attack_rolls;
    metrics->total_hits += run->hit_count;
    metrics->total_misses += run->miss_count;
    metrics->total_parries += run->parry_count;
    metrics->total_dodges += run->dodge_count;
    metrics->total_blocks += run->block_count;

    if (run->timed_out) {
        metrics->timeouts++;
    }
    else {
        metrics->completed++;
        metrics->total_ticks_completed += run->ticks;
    }

    if (run->attacker_won) {
        metrics->attacker_wins++;
    }
    else if (run->defender_won) {
        metrics->defender_wins++;
    }
}

void sim_metrics_print_summary(const SimAggregateMetrics* metrics, FILE* out)
{
    if (!metrics || !out) {
        return;
    }

    int ties = metrics->runs - metrics->attacker_wins - metrics->defender_wins - metrics->timeouts;
    if (ties < 0) {
        ties = 0;
    }

    double avg_damage_per_tick = 0.0;
    if (metrics->total_ticks > 0) {
        avg_damage_per_tick = (double)metrics->total_damage / (double)metrics->total_ticks;
    }

    double avg_ticks_to_kill = 0.0;
    double avg_seconds_to_kill = 0.0;
    if (metrics->completed > 0) {
        avg_ticks_to_kill = (double)metrics->total_ticks_completed / (double)metrics->completed;
        avg_seconds_to_kill = avg_ticks_to_kill / (double)PULSE_PER_SECOND;
    }

    double avg_spells_cast = 0.0;
    double avg_mana_spent = 0.0;
    if (metrics->runs > 0) {
        avg_spells_cast = (double)metrics->total_spells_cast / (double)metrics->runs;
        avg_mana_spent = (double)metrics->total_mana_spent / (double)metrics->runs;
    }

    double hit_rate = 0.0;
    double miss_rate = 0.0;
    double parry_rate = 0.0;
    double dodge_rate = 0.0;
    double block_rate = 0.0;
    if (metrics->total_attack_rolls > 0) {
        hit_rate = (double)metrics->total_hits / (double)metrics->total_attack_rolls;
        miss_rate = (double)metrics->total_misses / (double)metrics->total_attack_rolls;
        parry_rate = (double)metrics->total_parries / (double)metrics->total_attack_rolls;
        dodge_rate = (double)metrics->total_dodges / (double)metrics->total_attack_rolls;
        block_rate = (double)metrics->total_blocks / (double)metrics->total_attack_rolls;
    }

    fprintf(out, "Runs: %d (completed: %d, timeouts: %d)\n",
            metrics->runs, metrics->completed, metrics->timeouts);
    fprintf(out, "Wins: attacker=%d defender=%d ties=%d\n",
            metrics->attacker_wins, metrics->defender_wins, ties);

    if (metrics->completed > 0) {
        fprintf(out, "Avg time to kill: %.2f ticks (%.2f sec)\n",
                avg_ticks_to_kill, avg_seconds_to_kill);
    }
    else {
        fprintf(out, "Avg time to kill: n/a\n");
    }

    fprintf(out, "Avg damage per tick: %.2f\n", avg_damage_per_tick);
    fprintf(out, "Avg spells cast: %.2f mana spent: %.2f\n", avg_spells_cast, avg_mana_spent);
    if (metrics->total_attack_rolls > 0) {
        fprintf(out, "Rates: hit=%.2f miss=%.2f parry=%.2f dodge=%.2f block=%.2f\n",
                hit_rate, miss_rate, parry_rate, dodge_rate, block_rate);
    }
}
