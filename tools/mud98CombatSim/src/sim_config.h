////////////////////////////////////////////////////////////////////////////////
// sim_config.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98_COMBAT_SIM_CONFIG_H
#define MUD98_COMBAT_SIM_CONFIG_H

#include <data/stats.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SIM_MAX_SKILLS 32
#define SIM_MAX_SPELLS 16
#define SIM_NAME_LEN 64
#define SIM_TOKEN_LEN 32

typedef struct sim_damage_config_t {
    int dice;
    int size;
    int bonus;
} SimDamageConfig;

typedef enum sim_combatant_type_t {
    COMBATANT_SIM = 0,
    COMBATANT_PLAYER = 1,
    COMBATANT_MOB = 2,
} SimCombatantType;

typedef struct sim_skill_config_t {
    char name[SIM_TOKEN_LEN];
    int rating;
} SimSkillConfig;

typedef struct sim_spell_config_t {
    char name[SIM_TOKEN_LEN];
    int cooldown_ticks;
    int chance;
} SimSpellConfig;

typedef struct sim_ai_config_t {
    bool auto_attack;
    bool use_spec;
    bool use_spells;
    int spell_count;
    SimSpellConfig spells[SIM_MAX_SPELLS];
} SimAiConfig;

typedef struct sim_combatant_config_t {
    SimCombatantType type;
    char name[SIM_NAME_LEN];
    int mob_vnum;
    char spec_name[SIM_TOKEN_LEN];
    char class_name[SIM_TOKEN_LEN];
    char race_name[SIM_TOKEN_LEN];
    int level;
    int hitpoints;
    int hitroll;
    int armor;
    SimDamageConfig damage;
    int weapon_vnum;
    int stats[STAT_COUNT];
    int skill_count;
    SimSkillConfig skills[SIM_MAX_SKILLS];
    SimAiConfig ai;
    bool has_stats;
    bool has_skills;
    bool set_spec;
    bool set_level;
    bool set_hitpoints;
    bool set_hitroll;
    bool set_armor;
    bool set_damage;
    bool set_weapon;
} SimCombatantConfig;

typedef enum sim_sweep_target_t {
    SWEEP_TARGET_NONE = 0,
    SWEEP_TARGET_ATTACKER = 1,
    SWEEP_TARGET_DEFENDER = 2,
} SimSweepTarget;

typedef enum sim_sweep_stat_t {
    SWEEP_STAT_NONE = 0,
    SWEEP_STAT_LEVEL = 1,
    SWEEP_STAT_HITROLL = 2,
    SWEEP_STAT_HITPOINTS = 3,
    SWEEP_STAT_ARMOR = 4,
    SWEEP_STAT_DAMAGE_DICE = 5,
    SWEEP_STAT_DAMAGE_SIZE = 6,
    SWEEP_STAT_DAMAGE_BONUS = 7,
} SimSweepStat;

typedef struct sim_sweep_config_t {
    bool enabled;
    SimSweepTarget target;
    SimSweepStat stat;
    int start;
    int end;
    int step;
} SimSweepConfig;

typedef struct sim_config_t {
    int runs;
    int max_ticks;
    bool use_seed;
    uint64_t seed;
    char csv_path[256];
    char csv_runs_path[256];
    SimCombatantConfig attacker;
    SimCombatantConfig defender;
    SimSweepConfig sweep;
} SimConfig;

void sim_config_init(SimConfig* cfg);
bool sim_config_load(const char* path, SimConfig* out, char* err, size_t err_len);

#endif // MUD98_COMBAT_SIM_CONFIG_H
