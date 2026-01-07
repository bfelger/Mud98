////////////////////////////////////////////////////////////////////////////////
// sim_config.c
////////////////////////////////////////////////////////////////////////////////

#include "sim_config.h"

#include <jansson.h>

#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

static void set_name(char* dest, size_t dest_size, const char* name)
{
    if (!dest || dest_size == 0) {
        return;
    }
    if (!name) {
        dest[0] = '\0';
        return;
    }
    snprintf(dest, dest_size, "%s", name);
}

static bool json_get_int(json_t* obj, const char* key, int* out)
{
    json_t* value = json_object_get(obj, key);
    if (!value) {
        return false;
    }
    if (!json_is_integer(value)) {
        return false;
    }
    *out = (int)json_integer_value(value);
    return true;
}

static bool json_get_uint64(json_t* obj, const char* key, uint64_t* out)
{
    json_t* value = json_object_get(obj, key);
    if (!value) {
        return false;
    }
    if (!json_is_integer(value)) {
        return false;
    }
    json_int_t raw = json_integer_value(value);
    if (raw < 0) {
        return false;
    }
    *out = (uint64_t)raw;
    return true;
}

static bool json_get_string(json_t* obj, const char* key, char* out, size_t out_len)
{
    json_t* value = json_object_get(obj, key);
    if (!value) {
        return false;
    }
    if (!json_is_string(value)) {
        return false;
    }
    snprintf(out, out_len, "%s", json_string_value(value));
    return true;
}

static bool json_get_bool(json_t* obj, const char* key, bool* out)
{
    json_t* value = json_object_get(obj, key);
    if (!value) {
        return false;
    }
    if (!json_is_boolean(value)) {
        return false;
    }
    *out = json_is_true(value);
    return true;
}

static SimCombatantType parse_combatant_type(const char* value)
{
    if (!value || value[0] == '\0') {
        return COMBATANT_SIM;
    }
    if (!strcasecmp(value, "player")) {
        return COMBATANT_PLAYER;
    }
    if (!strcasecmp(value, "mob") || !strcasecmp(value, "npc")) {
        return COMBATANT_MOB;
    }
    if (!strcasecmp(value, "sim")) {
        return COMBATANT_SIM;
    }
    return COMBATANT_SIM;
}

static bool parse_stats(json_t* node, SimCombatantConfig* cfg, char* err, size_t err_len)
{
    json_t* stats = json_object_get(node, "stats");
    if (!stats) {
        return true;
    }
    if (!json_is_array(stats)) {
        snprintf(err, err_len, "stats must be an array");
        return false;
    }
    if (json_array_size(stats) != STAT_COUNT) {
        snprintf(err, err_len, "stats must have %d entries", STAT_COUNT);
        return false;
    }

    for (size_t i = 0; i < STAT_COUNT; i++) {
        json_t* entry = json_array_get(stats, i);
        if (!json_is_integer(entry)) {
            snprintf(err, err_len, "stats entries must be integers");
            return false;
        }
        cfg->stats[i] = (int)json_integer_value(entry);
    }

    cfg->has_stats = true;
    return true;
}

static bool parse_skills(json_t* node, SimCombatantConfig* cfg, char* err, size_t err_len)
{
    json_t* skills = json_object_get(node, "skills");
    if (!skills) {
        return true;
    }
    if (!json_is_object(skills)) {
        snprintf(err, err_len, "skills must be an object");
        return false;
    }

    cfg->skill_count = 0;
    void* iter = json_object_iter(skills);
    while (iter != NULL) {
        if (cfg->skill_count >= SIM_MAX_SKILLS) {
            snprintf(err, err_len, "skills exceeds max (%d)", SIM_MAX_SKILLS);
            return false;
        }
        const char* key = json_object_iter_key(iter);
        json_t* value = json_object_iter_value(iter);
        if (!json_is_integer(value)) {
            snprintf(err, err_len, "skill '%s' must be an integer", key ? key : "");
            return false;
        }
        snprintf(cfg->skills[cfg->skill_count].name,
            sizeof(cfg->skills[cfg->skill_count].name), "%s", key ? key : "");
        cfg->skills[cfg->skill_count].rating = (int)json_integer_value(value);
        cfg->skill_count++;
        iter = json_object_iter_next(skills, iter);
    }

    cfg->has_skills = cfg->skill_count > 0;
    return true;
}

static bool parse_spells(json_t* node, SimCombatantConfig* cfg,
                         bool has_ai_use_spells, char* err, size_t err_len)
{
    json_t* spells = json_object_get(node, "spells");
    if (!spells) {
        return true;
    }
    if (!json_is_array(spells)) {
        snprintf(err, err_len, "spells must be an array");
        return false;
    }

    cfg->ai.spell_count = 0;
    size_t count = json_array_size(spells);
    for (size_t i = 0; i < count; i++) {
        if (cfg->ai.spell_count >= SIM_MAX_SPELLS) {
            snprintf(err, err_len, "spells exceeds max (%d)", SIM_MAX_SPELLS);
            return false;
        }

        SimSpellConfig* spell = &cfg->ai.spells[cfg->ai.spell_count];
        *spell = (SimSpellConfig){ 0 };
        spell->chance = 100;
        spell->cooldown_ticks = 0;

        json_t* entry = json_array_get(spells, i);
        if (json_is_string(entry)) {
            snprintf(spell->name, sizeof(spell->name), "%s", json_string_value(entry));
        }
        else if (json_is_object(entry)) {
            if (!json_get_string(entry, "name", spell->name, sizeof(spell->name))) {
                snprintf(err, err_len, "spell entry %zu missing name", i);
                return false;
            }
            (void)json_get_int(entry, "cooldown", &spell->cooldown_ticks);
            (void)json_get_int(entry, "chance", &spell->chance);
        }
        else {
            snprintf(err, err_len, "spell entry %zu must be string or object", i);
            return false;
        }

        if (spell->name[0] == '\0') {
            snprintf(err, err_len, "spell entry %zu has empty name", i);
            return false;
        }
        if (spell->cooldown_ticks < 0) {
            snprintf(err, err_len, "spell '%s' cooldown must be >= 0", spell->name);
            return false;
        }
        if (spell->chance < 0 || spell->chance > 100) {
            snprintf(err, err_len, "spell '%s' chance must be 0-100", spell->name);
            return false;
        }

        cfg->ai.spell_count++;
    }

    if (!has_ai_use_spells && cfg->ai.spell_count > 0) {
        cfg->ai.use_spells = true;
    }

    return true;
}

static bool parse_ai(json_t* node, SimCombatantConfig* cfg,
                     bool* has_use_spells, char* err, size_t err_len)
{
    json_t* ai = json_object_get(node, "ai");
    if (!ai) {
        return true;
    }
    if (!json_is_object(ai)) {
        snprintf(err, err_len, "ai must be an object");
        return false;
    }

    (void)json_get_bool(ai, "auto_attack", &cfg->ai.auto_attack);
    (void)json_get_bool(ai, "use_spec", &cfg->ai.use_spec);
    if (json_get_bool(ai, "use_spells", &cfg->ai.use_spells)) {
        if (has_use_spells) {
            *has_use_spells = true;
        }
    }

    return true;
}

static SimSweepTarget parse_sweep_target(const char* value)
{
    if (!value) {
        return SWEEP_TARGET_NONE;
    }
    if (!strcasecmp(value, "attacker")) {
        return SWEEP_TARGET_ATTACKER;
    }
    if (!strcasecmp(value, "defender")) {
        return SWEEP_TARGET_DEFENDER;
    }
    return SWEEP_TARGET_NONE;
}

static SimSweepStat parse_sweep_stat(const char* value)
{
    if (!value) {
        return SWEEP_STAT_NONE;
    }
    if (!strcasecmp(value, "level")) {
        return SWEEP_STAT_LEVEL;
    }
    if (!strcasecmp(value, "hitroll")) {
        return SWEEP_STAT_HITROLL;
    }
    if (!strcasecmp(value, "hitpoints") || !strcasecmp(value, "hp")) {
        return SWEEP_STAT_HITPOINTS;
    }
    if (!strcasecmp(value, "armor") || !strcasecmp(value, "ac")) {
        return SWEEP_STAT_ARMOR;
    }
    if (!strcasecmp(value, "damage_dice")) {
        return SWEEP_STAT_DAMAGE_DICE;
    }
    if (!strcasecmp(value, "damage_size")) {
        return SWEEP_STAT_DAMAGE_SIZE;
    }
    if (!strcasecmp(value, "damage_bonus")) {
        return SWEEP_STAT_DAMAGE_BONUS;
    }
    return SWEEP_STAT_NONE;
}

static bool parse_sweep(json_t* root, SimSweepConfig* out, char* err, size_t err_len)
{
    json_t* node = json_object_get(root, "sweep");
    if (!node) {
        out->enabled = false;
        return true;
    }

    if (!json_is_object(node)) {
        snprintf(err, err_len, "sweep must be an object");
        return false;
    }

    char target_name[64] = { 0 };
    char stat_name[64] = { 0 };
    if (!json_get_string(node, "target", target_name, sizeof(target_name))) {
        snprintf(err, err_len, "sweep.target is required");
        return false;
    }
    if (!json_get_string(node, "stat", stat_name, sizeof(stat_name))) {
        snprintf(err, err_len, "sweep.stat is required");
        return false;
    }

    int start = 0;
    int end = 0;
    int step = 0;
    if (!json_get_int(node, "start", &start)) {
        snprintf(err, err_len, "sweep.start is required");
        return false;
    }
    if (!json_get_int(node, "end", &end)) {
        snprintf(err, err_len, "sweep.end is required");
        return false;
    }
    if (!json_get_int(node, "step", &step) || step == 0) {
        snprintf(err, err_len, "sweep.step must be a non-zero integer");
        return false;
    }

    SimSweepTarget target = parse_sweep_target(target_name);
    if (target == SWEEP_TARGET_NONE) {
        snprintf(err, err_len, "sweep.target must be attacker or defender");
        return false;
    }

    SimSweepStat stat = parse_sweep_stat(stat_name);
    if (stat == SWEEP_STAT_NONE) {
        snprintf(err, err_len, "sweep.stat is not supported");
        return false;
    }

    if (start < end && step < 0) {
        snprintf(err, err_len, "sweep.step must be positive for increasing ranges");
        return false;
    }
    if (start > end && step > 0) {
        snprintf(err, err_len, "sweep.step must be negative for decreasing ranges");
        return false;
    }

    out->enabled = true;
    out->target = target;
    out->stat = stat;
    out->start = start;
    out->end = end;
    out->step = step;
    return true;
}

static bool parse_damage(json_t* node, SimDamageConfig* out, char* err, size_t err_len)
{
    if (!json_is_object(node)) {
        snprintf(err, err_len, "damage must be an object");
        return false;
    }

    SimDamageConfig cfg = *out;
    (void)json_get_int(node, "dice", &cfg.dice);
    (void)json_get_int(node, "size", &cfg.size);
    (void)json_get_int(node, "bonus", &cfg.bonus);

    if (cfg.dice <= 0 || cfg.size <= 0) {
        snprintf(err, err_len, "damage dice and size must be > 0");
        return false;
    }

    *out = cfg;
    return true;
}

static bool parse_combatant(json_t* root, const char* key, SimCombatantConfig* out,
                            char* err, size_t err_len)
{
    json_t* node = json_object_get(root, key);
    if (!node) {
        snprintf(err, err_len, "missing %s", key);
        return false;
    }
    if (!json_is_object(node)) {
        snprintf(err, err_len, "%s must be an object", key);
        return false;
    }

    SimCombatantType type = COMBATANT_SIM;
    json_t* type_node = json_object_get(node, "type");
    if (type_node != NULL) {
        if (!json_is_string(type_node)) {
            snprintf(err, err_len, "%s.type must be a string", key);
            return false;
        }
        type = parse_combatant_type(json_string_value(type_node));
    }

    SimCombatantConfig cfg = *out;
    cfg.type = type;

    cfg.has_stats = false;
    cfg.has_skills = false;
    cfg.skill_count = 0;
    cfg.ai.spell_count = 0;
    cfg.set_spec = false;

    json_t* name = json_object_get(node, "name");
    if (name && json_is_string(name)) {
        set_name(cfg.name, sizeof(cfg.name), json_string_value(name));
    }

    if (json_get_string(node, "spec", cfg.spec_name, sizeof(cfg.spec_name))) {
        cfg.set_spec = true;
    }

    if (cfg.type == COMBATANT_MOB) {
        if (!json_get_int(node, "vnum", &cfg.mob_vnum) || cfg.mob_vnum <= 0) {
            snprintf(err, err_len, "%s.vnum is required for mob combatants", key);
            return false;
        }
    }

    if (cfg.type == COMBATANT_PLAYER) {
        if (!json_get_string(node, "class", cfg.class_name, sizeof(cfg.class_name))) {
            snprintf(err, err_len, "%s.class is required for player combatants", key);
            return false;
        }
        if (!json_get_string(node, "race", cfg.race_name, sizeof(cfg.race_name))) {
            snprintf(err, err_len, "%s.race is required for player combatants", key);
            return false;
        }
    }

    bool has_level = json_get_int(node, "level", &cfg.level);
    if (has_level && cfg.level <= 0) {
        snprintf(err, err_len, "%s level must be > 0", key);
        return false;
    }

    bool has_hitpoints = json_get_int(node, "hitpoints", &cfg.hitpoints);
    if (has_hitpoints && cfg.hitpoints <= 0) {
        snprintf(err, err_len, "%s hitpoints must be > 0", key);
        return false;
    }

    bool has_hitroll = json_get_int(node, "hitroll", &cfg.hitroll);
    bool has_armor = json_get_int(node, "armor", &cfg.armor);

    json_t* damage = json_object_get(node, "damage");
    if (damage != NULL) {
        if (!parse_damage(damage, &cfg.damage, err, err_len)) {
            return false;
        }
        cfg.set_damage = true;
    }
    else {
        cfg.set_damage = cfg.type != COMBATANT_MOB;
    }

    if (!parse_stats(node, &cfg, err, err_len)) {
        return false;
    }

    if (!parse_skills(node, &cfg, err, err_len)) {
        return false;
    }

    bool has_ai_use_spells = false;
    if (!parse_ai(node, &cfg, &has_ai_use_spells, err, err_len)) {
        return false;
    }
    if (!parse_spells(node, &cfg, has_ai_use_spells, err, err_len)) {
        return false;
    }

    if (json_get_int(node, "weapon_vnum", &cfg.weapon_vnum)) {
        cfg.set_weapon = cfg.weapon_vnum > 0;
    }
    else {
        cfg.set_weapon = false;
    }

    if (cfg.type == COMBATANT_MOB) {
        cfg.set_level = has_level;
        cfg.set_hitpoints = has_hitpoints;
        cfg.set_hitroll = has_hitroll;
        cfg.set_armor = has_armor;
        if (!damage) {
            cfg.set_damage = false;
        }
    }
    else {
        cfg.set_level = true;
        cfg.set_hitpoints = true;
        cfg.set_hitroll = true;
        cfg.set_armor = true;
        if (!damage) {
            cfg.set_damage = true;
        }
    }

    *out = cfg;
    return true;
}

void sim_config_init(SimConfig* cfg)
{
    if (!cfg) {
        return;
    }

    memset(cfg, 0, sizeof(*cfg));

    cfg->runs = 1;
    cfg->max_ticks = 200;
    cfg->use_seed = false;
    cfg->seed = 0;
    cfg->csv_path[0] = '\0';
    cfg->csv_runs_path[0] = '\0';

    cfg->attacker.type = COMBATANT_SIM;
    cfg->attacker.mob_vnum = 0;
    cfg->attacker.spec_name[0] = '\0';
    cfg->attacker.class_name[0] = '\0';
    cfg->attacker.race_name[0] = '\0';
    cfg->attacker.weapon_vnum = 0;
    cfg->attacker.has_stats = false;
    cfg->attacker.has_skills = false;
    cfg->attacker.skill_count = 0;
    cfg->attacker.ai.auto_attack = true;
    cfg->attacker.ai.use_spec = true;
    cfg->attacker.ai.use_spells = false;
    cfg->attacker.ai.spell_count = 0;
    set_name(cfg->attacker.name, sizeof(cfg->attacker.name), "Attacker");
    cfg->attacker.level = 10;
    cfg->attacker.hitpoints = 100;
    cfg->attacker.hitroll = 0;
    cfg->attacker.armor = 100;
    cfg->attacker.damage.dice = 1;
    cfg->attacker.damage.size = 6;
    cfg->attacker.damage.bonus = 0;
    cfg->attacker.set_level = true;
    cfg->attacker.set_hitpoints = true;
    cfg->attacker.set_hitroll = true;
    cfg->attacker.set_armor = true;
    cfg->attacker.set_damage = true;
    cfg->attacker.set_weapon = false;
    cfg->attacker.set_spec = false;

    cfg->defender.type = COMBATANT_SIM;
    cfg->defender.mob_vnum = 0;
    cfg->defender.spec_name[0] = '\0';
    cfg->defender.class_name[0] = '\0';
    cfg->defender.race_name[0] = '\0';
    cfg->defender.weapon_vnum = 0;
    cfg->defender.has_stats = false;
    cfg->defender.has_skills = false;
    cfg->defender.skill_count = 0;
    cfg->defender.ai.auto_attack = true;
    cfg->defender.ai.use_spec = true;
    cfg->defender.ai.use_spells = false;
    cfg->defender.ai.spell_count = 0;
    set_name(cfg->defender.name, sizeof(cfg->defender.name), "Defender");
    cfg->defender.level = 10;
    cfg->defender.hitpoints = 100;
    cfg->defender.hitroll = 0;
    cfg->defender.armor = 100;
    cfg->defender.damage.dice = 1;
    cfg->defender.damage.size = 6;
    cfg->defender.damage.bonus = 0;
    cfg->defender.set_level = true;
    cfg->defender.set_hitpoints = true;
    cfg->defender.set_hitroll = true;
    cfg->defender.set_armor = true;
    cfg->defender.set_damage = true;
    cfg->defender.set_weapon = false;
    cfg->defender.set_spec = false;

    cfg->sweep.enabled = false;
    cfg->sweep.target = SWEEP_TARGET_NONE;
    cfg->sweep.stat = SWEEP_STAT_NONE;
    cfg->sweep.start = 0;
    cfg->sweep.end = 0;
    cfg->sweep.step = 0;
}

bool sim_config_load(const char* path, SimConfig* out, char* err, size_t err_len)
{
    json_error_t error;
    json_t* root = NULL;

    if (!path || !out) {
        snprintf(err, err_len, "invalid config input");
        return false;
    }

    sim_config_init(out);

    root = json_load_file(path, 0, &error);
    if (!root) {
        snprintf(err, err_len, "json parse error at line %d: %s", error.line, error.text);
        return false;
    }

    if (!json_is_object(root)) {
        snprintf(err, err_len, "config root must be an object");
        json_decref(root);
        return false;
    }

    if (json_get_uint64(root, "seed", &out->seed)) {
        out->use_seed = true;
    }

    if (json_get_int(root, "runs", &out->runs) && out->runs <= 0) {
        snprintf(err, err_len, "runs must be > 0");
        json_decref(root);
        return false;
    }

    if (json_get_int(root, "max_ticks", &out->max_ticks) && out->max_ticks <= 0) {
        snprintf(err, err_len, "max_ticks must be > 0");
        json_decref(root);
        return false;
    }

    (void)json_get_string(root, "csv_path", out->csv_path, sizeof(out->csv_path));
    (void)json_get_string(root, "csv_runs_path", out->csv_runs_path, sizeof(out->csv_runs_path));

    if (!parse_combatant(root, "attacker", &out->attacker, err, err_len)) {
        json_decref(root);
        return false;
    }

    if (!parse_combatant(root, "defender", &out->defender, err, err_len)) {
        json_decref(root);
        return false;
    }

    if (!parse_sweep(root, &out->sweep, err, err_len)) {
        json_decref(root);
        return false;
    }

    json_decref(root);
    return true;
}
