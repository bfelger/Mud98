////////////////////////////////////////////////////////////////////////////////
// mud98CombatSim: A combat simulator for MUD98.
////////////////////////////////////////////////////////////////////////////////

#include "metrics.h"
#include "sim.h"
#include "sim_config.h"

#include <config.h>
#include <db.h>
#include <fileutils.h>
#include <pcg_basic.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <stdint.h>
#include <io.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

extern bool test_output_enabled;
extern char str_boot_time[MAX_INPUT_LENGTH];
extern time_t boot_time;
extern time_t current_time;

#ifdef _MSC_VER
struct timezone {
    int dummy;
};

int gettimeofday(struct timeval* tp, struct timezone* unused);
#endif

static void print_usage(const char* argv0)
{
    fprintf(stderr,
        "Usage: %s [--dir=<rundir>] [--area-dir=<areadir>] <config.json>\n",
        argv0);
}

static void seed_rng(uint64_t seed)
{
    uint64_t seq = 54u;
    pcg32_srandom(seed, seq);
}

static const char* sweep_target_name(SimSweepTarget target)
{
    switch (target) {
    case SWEEP_TARGET_ATTACKER:
        return "attacker";
    case SWEEP_TARGET_DEFENDER:
        return "defender";
    default:
        return "none";
    }
}

static const char* sweep_stat_name(SimSweepStat stat)
{
    switch (stat) {
    case SWEEP_STAT_LEVEL:
        return "level";
    case SWEEP_STAT_HITROLL:
        return "hitroll";
    case SWEEP_STAT_HITPOINTS:
        return "hitpoints";
    case SWEEP_STAT_ARMOR:
        return "armor";
    case SWEEP_STAT_DAMAGE_DICE:
        return "damage_dice";
    case SWEEP_STAT_DAMAGE_SIZE:
        return "damage_size";
    case SWEEP_STAT_DAMAGE_BONUS:
        return "damage_bonus";
    default:
        return "none";
    }
}

static void compute_rates(long long rolls, long long hits, long long misses,
                          long long parries, long long dodges, long long blocks,
                          double* hit_rate, double* miss_rate, double* parry_rate,
                          double* dodge_rate, double* block_rate)
{
    if (rolls <= 0) {
        *hit_rate = 0.0;
        *miss_rate = 0.0;
        *parry_rate = 0.0;
        *dodge_rate = 0.0;
        *block_rate = 0.0;
        return;
    }

    *hit_rate = (double)hits / (double)rolls;
    *miss_rate = (double)misses / (double)rolls;
    *parry_rate = (double)parries / (double)rolls;
    *dodge_rate = (double)dodges / (double)rolls;
    *block_rate = (double)blocks / (double)rolls;
}

static void write_run_csv(FILE* csv, int run_index, int sweep_value, bool include_sweep,
                          const SimRunMetrics* run)
{
    double seconds = (double)run->ticks / (double)PULSE_PER_SECOND;
    double damage_per_tick = 0.0;
    if (run->ticks > 0) {
        damage_per_tick = (double)run->total_damage / (double)run->ticks;
    }

    double hit_rate = 0.0;
    double miss_rate = 0.0;
    double parry_rate = 0.0;
    double dodge_rate = 0.0;
    double block_rate = 0.0;
    compute_rates(run->attack_rolls, run->hit_count, run->miss_count,
                  run->parry_count, run->dodge_count, run->block_count,
                  &hit_rate, &miss_rate, &parry_rate, &dodge_rate, &block_rate);

    if (include_sweep) {
        fprintf(csv, "%d,", sweep_value);
    }
    fprintf(csv,
        "%d,%d,%.4f,%d,%.4f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.4f,%.4f,%.4f,%.4f,%.4f\n",
        run_index,
        run->ticks,
        seconds,
        run->total_damage,
        damage_per_tick,
        run->attacker_won ? 1 : 0,
        run->defender_won ? 1 : 0,
        run->timed_out ? 1 : 0,
        run->spells_cast,
        run->mana_spent,
        run->attack_rolls,
        run->hit_count,
        run->miss_count,
        run->parry_count,
        run->dodge_count,
        run->block_count,
        hit_rate,
        miss_rate,
        parry_rate,
        dodge_rate,
        block_rate);
}

static void write_csv_runs_header(FILE* csv, bool with_sweep)
{
    if (with_sweep) {
        fprintf(csv, "sweep_value,");
    }
    fprintf(csv,
        "run,ticks,seconds,total_damage,damage_per_tick,"
        "attacker_won,defender_won,timed_out,spells_cast,mana_spent,"
        "attack_rolls,hits,misses,parries,dodges,blocks,"
        "hit_rate,miss_rate,parry_rate,dodge_rate,block_rate\n");
}

static void write_csv_summary_header(FILE* csv)
{
    fprintf(csv,
        "sweep_target,sweep_stat,sweep_value,runs,completed,timeouts,"
        "attacker_wins,defender_wins,ties,avg_ticks,avg_seconds,avg_damage_per_tick,avg_spells_cast,avg_mana_spent,"
        "hit_rate,miss_rate,parry_rate,dodge_rate,block_rate\n");
}

static bool apply_sweep_value(SimConfig* cfg, int value, char* err, size_t err_len)
{
    if (!cfg) {
        snprintf(err, err_len, "invalid sweep config");
        return false;
    }

    SimCombatantConfig* target = NULL;
    if (cfg->sweep.target == SWEEP_TARGET_ATTACKER) {
        target = &cfg->attacker;
    }
    else if (cfg->sweep.target == SWEEP_TARGET_DEFENDER) {
        target = &cfg->defender;
    }

    if (!target) {
        snprintf(err, err_len, "sweep target is not set");
        return false;
    }

    switch (cfg->sweep.stat) {
    case SWEEP_STAT_LEVEL:
        if (value <= 0) {
            snprintf(err, err_len, "sweep level must be > 0");
            return false;
        }
        target->level = value;
        target->set_level = true;
        return true;
    case SWEEP_STAT_HITROLL:
        target->hitroll = value;
        target->set_hitroll = true;
        return true;
    case SWEEP_STAT_HITPOINTS:
        if (value <= 0) {
            snprintf(err, err_len, "sweep hitpoints must be > 0");
            return false;
        }
        target->hitpoints = value;
        target->set_hitpoints = true;
        return true;
    case SWEEP_STAT_ARMOR:
        target->armor = value;
        target->set_armor = true;
        return true;
    case SWEEP_STAT_DAMAGE_DICE:
        if (value <= 0) {
            snprintf(err, err_len, "sweep damage_dice must be > 0");
            return false;
        }
        target->damage.dice = value;
        target->set_damage = true;
        return true;
    case SWEEP_STAT_DAMAGE_SIZE:
        if (value <= 0) {
            snprintf(err, err_len, "sweep damage_size must be > 0");
            return false;
        }
        target->damage.size = value;
        target->set_damage = true;
        return true;
    case SWEEP_STAT_DAMAGE_BONUS:
        target->damage.bonus = value;
        target->set_damage = true;
        return true;
    default:
        snprintf(err, err_len, "unsupported sweep stat");
        return false;
    }
}

int main(int argc, char** argv)
{
    struct timeval now_time = { 0 };
    char run_dir[256] = { 0 };
    char area_dir[256] = { 0 };
    const char* config_path = NULL;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (!strncmp(argv[i], "--dir=", 6)) {
                snprintf(run_dir, sizeof(run_dir), "%s", argv[i] + 6);
            }
            else if (!strncmp(argv[i], "--rundir=", 9)) {
                snprintf(run_dir, sizeof(run_dir), "%s", argv[i] + 9);
            }
            else if (!strcmp(argv[i], "-d")) {
                if (++i < argc) {
                    snprintf(run_dir, sizeof(run_dir), "%s", argv[i]);
                }
            }
            else if (!strncmp(argv[i], "--area-dir=", 11)) {
                snprintf(area_dir, sizeof(area_dir), "%s", argv[i] + 11);
            }
            else if (!strcmp(argv[i], "-a")) {
                if (++i < argc) {
                    snprintf(area_dir, sizeof(area_dir), "%s", argv[i]);
                }
            }
            else if (!strncmp(argv[i], "--config=", 9)) {
                config_path = argv[i] + 9;
            }
            else if (argv[i][0] == '-') {
                fprintf(stderr, "Unknown argument '%s'.\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
            else if (!config_path) {
                config_path = argv[i];
            }
            else {
                fprintf(stderr, "Unexpected argument '%s'.\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        }
    }

    if (!config_path) {
        print_usage(argv[0]);
        return 1;
    }

    if (area_dir[0]) {
        cfg_set_area_dir(area_dir);
    }

    if (run_dir[0]) {
        cfg_set_base_dir(run_dir);
    }

    test_output_enabled = true;

    gettimeofday(&now_time, NULL);
    boot_time = (time_t)now_time.tv_sec;
    current_time = boot_time;
    strcpy(str_boot_time, ctime(&boot_time));

    open_reserve_file();
    boot_db();

    SimConfig cfg;
    char err[256] = { 0 };
    if (!sim_config_load(config_path, &cfg, err, sizeof(err))) {
        fprintf(stderr, "Config error: %s\n", err);
        return 1;
    }

    SimContext ctx;
    if (!sim_init(&ctx, &cfg, err, sizeof(err))) {
        fprintf(stderr, "Sim init error: %s\n", err);
        return 1;
    }

    bool sweep_enabled = cfg.sweep.enabled;
    FILE* csv_summary = NULL;
    FILE* csv_runs = NULL;

    if (sweep_enabled) {
        if (cfg.csv_path[0] != '\0') {
            csv_summary = fopen(cfg.csv_path, "w");
            if (!csv_summary) {
                fprintf(stderr, "Failed to open CSV output: %s\n", cfg.csv_path);
                return 1;
            }
            write_csv_summary_header(csv_summary);
        }

        if (cfg.csv_runs_path[0] != '\0') {
            csv_runs = fopen(cfg.csv_runs_path, "w");
            if (!csv_runs) {
                fprintf(stderr, "Failed to open CSV runs output: %s\n", cfg.csv_runs_path);
                return 1;
            }
            write_csv_runs_header(csv_runs, true);
        }
    }
    else {
        if (cfg.csv_path[0] != '\0') {
            csv_runs = fopen(cfg.csv_path, "w");
            if (!csv_runs) {
                fprintf(stderr, "Failed to open CSV output: %s\n", cfg.csv_path);
                return 1;
            }
            write_csv_runs_header(csv_runs, false);
        }
    }

    if (sweep_enabled) {
        int sweep_index = 0;
        for (int value = cfg.sweep.start; ; value += cfg.sweep.step, sweep_index++) {
            if (cfg.sweep.step > 0 && value > cfg.sweep.end) {
                break;
            }
            if (cfg.sweep.step < 0 && value < cfg.sweep.end) {
                break;
            }

            SimConfig step_cfg = cfg;
            if (!apply_sweep_value(&step_cfg, value, err, sizeof(err))) {
                fprintf(stderr, "Sweep error: %s\n", err);
                return 1;
            }

            SimAggregateMetrics step_metrics;
            sim_metrics_init(&step_metrics);

            for (int run = 0; run < cfg.runs; run++) {
                if (cfg.use_seed) {
                    uint64_t offset = (uint64_t)(sweep_index * cfg.runs + run);
                    seed_rng(cfg.seed + offset);
                }

                SimRunMetrics run_metrics = { 0 };
                if (!sim_run(&ctx, &step_cfg, &run_metrics)) {
                    fprintf(stderr, "Sim run failed.\n");
                    return 1;
                }

                if (csv_runs) {
                    write_run_csv(csv_runs, run + 1, value, true, &run_metrics);
                }

                sim_metrics_add_run(&step_metrics, &run_metrics);
            }

            printf("Sweep target=%s stat=%s value=%d\n",
                sweep_target_name(cfg.sweep.target),
                sweep_stat_name(cfg.sweep.stat),
                value);
            sim_metrics_print_summary(&step_metrics, stdout);

            if (csv_summary) {
                int ties = step_metrics.runs - step_metrics.attacker_wins
                    - step_metrics.defender_wins - step_metrics.timeouts;
                if (ties < 0) {
                    ties = 0;
                }

                double avg_ticks = 0.0;
                double avg_seconds = 0.0;
                if (step_metrics.completed > 0) {
                    avg_ticks = (double)step_metrics.total_ticks_completed
                        / (double)step_metrics.completed;
                    avg_seconds = avg_ticks / (double)PULSE_PER_SECOND;
                }

                double avg_damage_per_tick = 0.0;
                if (step_metrics.total_ticks > 0) {
                    avg_damage_per_tick = (double)step_metrics.total_damage
                        / (double)step_metrics.total_ticks;
                }

                double avg_spells_cast = 0.0;
                double avg_mana_spent = 0.0;
                if (step_metrics.runs > 0) {
                    avg_spells_cast = (double)step_metrics.total_spells_cast
                        / (double)step_metrics.runs;
                    avg_mana_spent = (double)step_metrics.total_mana_spent
                        / (double)step_metrics.runs;
                }

                double hit_rate = 0.0;
                double miss_rate = 0.0;
                double parry_rate = 0.0;
                double dodge_rate = 0.0;
                double block_rate = 0.0;
                compute_rates(step_metrics.total_attack_rolls, step_metrics.total_hits,
                    step_metrics.total_misses, step_metrics.total_parries,
                    step_metrics.total_dodges, step_metrics.total_blocks,
                    &hit_rate, &miss_rate, &parry_rate, &dodge_rate, &block_rate);

                fprintf(csv_summary,
                    "%s,%s,%d,%d,%d,%d,%d,%d,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
                    sweep_target_name(cfg.sweep.target),
                    sweep_stat_name(cfg.sweep.stat),
                    value,
                    step_metrics.runs,
                    step_metrics.completed,
                    step_metrics.timeouts,
                    step_metrics.attacker_wins,
                    step_metrics.defender_wins,
                    ties,
                    avg_ticks,
                    avg_seconds,
                    avg_damage_per_tick,
                    avg_spells_cast,
                    avg_mana_spent,
                    hit_rate,
                    miss_rate,
                    parry_rate,
                    dodge_rate,
                    block_rate);
            }

            if (value == cfg.sweep.end) {
                break;
            }
        }
    }
    else {
        SimAggregateMetrics metrics;
        sim_metrics_init(&metrics);

        for (int run = 0; run < cfg.runs; run++) {
            if (cfg.use_seed) {
                seed_rng(cfg.seed + (uint64_t)run);
            }

            SimRunMetrics run_metrics = { 0 };
            if (!sim_run(&ctx, &cfg, &run_metrics)) {
                fprintf(stderr, "Sim run failed.\n");
                return 1;
            }

            if (csv_runs) {
                write_run_csv(csv_runs, run + 1, 0, false, &run_metrics);
            }

            sim_metrics_add_run(&metrics, &run_metrics);
        }

        sim_metrics_print_summary(&metrics, stdout);
    }

    sim_shutdown(&ctx);
    free_vm();

    if (csv_summary) {
        fclose(csv_summary);
        csv_summary = NULL;
    }
    if (csv_runs) {
        fclose(csv_runs);
        csv_runs = NULL;
    }

    return 0;
}

#ifdef _MSC_VER
////////////////////////////////////////////////////////////////////////////////
// This implementation taken from StackOverflow user Michaelangel007's example:
//    https://stackoverflow.com/a/26085827
//    "Here is a free implementation:"
////////////////////////////////////////////////////////////////////////////////
int gettimeofday(struct timeval* tp, struct timezone* unused)
{
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

    SYSTEMTIME system_time;
    FILETIME file_time;
    uint64_t time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
#endif
