////////////////////////////////////////////////////////////////////////////////
// sim.c
////////////////////////////////////////////////////////////////////////////////

#include "sim.h"

#include <combat_metrics.h>
#include <combat_ops.h>
#include <db.h>
#include <fight.h>
#include <handler.h>
#include <lookup.h>
#include <lox/array.h>
#include <magic.h>
#include <special.h>
#include <stringutils.h>

#include <data/item.h>
#include <data/skill.h>

#include <entities/descriptor.h>
#include <entities/player_data.h>

#include <data/direction.h>

#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

static CombatOps sim_combat_ops;
static CombatOps* sim_prev_combat = NULL;
static CombatMetricsOps sim_metrics_ops;
static CombatMetricsOps* sim_prev_metrics = NULL;
static SimRunMetrics* sim_active_metrics = NULL;

typedef struct sim_ai_runtime_t {
    int spell_cooldowns[SIM_MAX_SPELLS];
} SimAiRuntime;

static void sim_on_attack_roll(Mobile* attacker, Mobile* victim, bool hit)
{
    if (!sim_active_metrics) {
        return;
    }

    sim_active_metrics->attack_rolls++;
    if (hit) {
        sim_active_metrics->hit_count++;
    }
    else {
        sim_active_metrics->miss_count++;
    }

    (void)attacker;
    (void)victim;
}

static void sim_on_parry(Mobile* attacker, Mobile* victim)
{
    if (sim_active_metrics) {
        sim_active_metrics->parry_count++;
    }
    (void)attacker;
    (void)victim;
}

static void sim_on_dodge(Mobile* attacker, Mobile* victim)
{
    if (sim_active_metrics) {
        sim_active_metrics->dodge_count++;
    }
    (void)attacker;
    (void)victim;
}

static void sim_on_shield_block(Mobile* attacker, Mobile* victim)
{
    if (sim_active_metrics) {
        sim_active_metrics->block_count++;
    }
    (void)attacker;
    (void)victim;
}

static void sim_handle_death(Mobile* victim)
{
    if (!victim) {
        return;
    }

    stop_fighting(victim, true);
    victim->position = POS_DEAD;
}

static void sim_advance_wait(Mobile* ch)
{
    if (!ch || ch->desc == NULL) {
        return;
    }

    if (ch->wait > 0) {
        ch->wait = (int16_t)(ch->wait > PULSE_VIOLENCE ? ch->wait - PULSE_VIOLENCE : 0);
    }
    if (ch->daze > 0) {
        ch->daze = (int16_t)(ch->daze > PULSE_VIOLENCE ? ch->daze - PULSE_VIOLENCE : 0);
    }
}

static bool attach_sim_descriptor(Mobile* ch, char* err, size_t err_len)
{
    if (!ch) {
        snprintf(err, err_len, "invalid player descriptor");
        return false;
    }
    if (ch->desc != NULL) {
        return true;
    }

    Descriptor* desc = new_descriptor();
    if (!desc) {
        snprintf(err, err_len, "failed to allocate descriptor");
        return false;
    }

    desc->character = ch;
    desc->connected = CON_PLAYING;
    ch->desc = desc;
    return true;
}

static void detach_sim_descriptor(Mobile* ch)
{
    if (!ch || !ch->desc) {
        return;
    }
    if (ch->desc->client != NULL || ch->desc->mth != NULL) {
        return;
    }

    Descriptor* desc = ch->desc;
    ch->desc = NULL;
    desc->character = NULL;
    free_descriptor(desc);
}

static bool apply_spec_to_mobile(Mobile* ch, const SimCombatantConfig* cfg,
                                 char* err, size_t err_len)
{
    if (!cfg || !cfg->set_spec) {
        return true;
    }
    if (!ch || !IS_NPC(ch)) {
        snprintf(err, err_len, "spec is only supported for NPCs");
        return false;
    }

    SpecFunc* spec = spec_lookup(cfg->spec_name);
    if (!spec) {
        snprintf(err, err_len, "unknown spec: %s", cfg->spec_name);
        return false;
    }

    ch->spec_fun = spec;
    return true;
}

static void sim_ai_tick_cooldowns(const SimCombatantConfig* cfg, SimAiRuntime* runtime)
{
    if (!cfg || !runtime || cfg->ai.spell_count <= 0) {
        return;
    }

    for (int i = 0; i < cfg->ai.spell_count; i++) {
        if (runtime->spell_cooldowns[i] > 0) {
            runtime->spell_cooldowns[i]--;
        }
    }
}

static void build_spell_argument(const char* name, char* out, size_t out_len)
{
    if (!out || out_len == 0) {
        return;
    }
    if (!name) {
        out[0] = '\0';
        return;
    }

    if (strchr(name, ' ') != NULL) {
        snprintf(out, out_len, "'%s'", name);
    }
    else {
        snprintf(out, out_len, "%s", name);
    }
}

static bool sim_ai_try_cast_spell(Mobile* ch, Mobile* opponent,
                                  const SimCombatantConfig* cfg,
                                  SimAiRuntime* runtime)
{
    if (!ch || !cfg || !runtime || !cfg->ai.use_spells || cfg->ai.spell_count <= 0) {
        return false;
    }
    if (!IS_AWAKE(ch) || ch->wait > 0 || ch->daze > 0) {
        return false;
    }
    if (ch->fighting == NULL) {
        ch->fighting = opponent;
    }

    for (int i = 0; i < cfg->ai.spell_count; i++) {
        const SimSpellConfig* spell = &cfg->ai.spells[i];
        if (runtime->spell_cooldowns[i] > 0) {
            continue;
        }
        if (spell->chance < 100 && number_percent() > spell->chance) {
            continue;
        }

        char cast_arg[128];
        int pre_mana = ch->mana;
        build_spell_argument(spell->name, cast_arg, sizeof(cast_arg));
        do_cast(ch, cast_arg);
        int spent = pre_mana - ch->mana;
        if (spent > 0 && sim_active_metrics) {
            sim_active_metrics->spells_cast++;
            sim_active_metrics->mana_spent += spent;
        }
        runtime->spell_cooldowns[i] = spell->cooldown_ticks;
        return true;
    }

    return false;
}

static void sim_ai_run_spec(Mobile* ch, const SimCombatantConfig* cfg)
{
    if (!ch || !cfg || !cfg->ai.use_spec || !IS_NPC(ch)) {
        return;
    }
    if (ch->spec_fun == NULL) {
        return;
    }

    (void)ch->spec_fun(ch);
}

static bool is_terminal_state(const Mobile* ch)
{
    if (!ch) {
        return true;
    }
    if (!IS_VALID(ch)) {
        return true;
    }
    if (ch->hit <= 0) {
        return true;
    }
    return ch->position <= POS_STUNNED;
}

static bool sim_handle_terminal(Mobile* victim)
{
    if (!is_terminal_state(victim)) {
        return false;
    }

    if (combat && combat->handle_death) {
        combat->handle_death(victim);
    }
    return true;
}

static VNUM find_free_room_vnum(VNUM start)
{
    VNUM vnum = start;
    while (get_room_data(vnum) != NULL) {
        vnum++;
    }
    return vnum;
}

static VNUM find_free_mob_vnum(VNUM start)
{
    VNUM vnum = start;
    while (global_mob_proto_get(vnum) != NULL) {
        vnum++;
    }
    return vnum;
}

static void proto_set_name(MobPrototype* proto, const char* name)
{
    char long_descr[128];

    SET_NAME(proto, lox_string(name));

    free_string(proto->short_descr);
    free_string(proto->long_descr);
    free_string(proto->description);

    proto->short_descr = str_dup(name);
    snprintf(long_descr, sizeof(long_descr), "%s is here.\n\r", name);
    proto->long_descr = str_dup(long_descr);
    proto->description = str_dup("A combat simulation target.\n\r");
}

static void proto_set_fixed_hitpoints(MobPrototype* proto, int hitpoints)
{
    int hp = hitpoints;
    if (hp < 1) {
        hp = 1;
    }

    proto->hit[DICE_NUMBER] = 1;
    proto->hit[DICE_TYPE] = 1;
    proto->hit[DICE_BONUS] = (int16_t)(hp - 1);
}

static void proto_set_damage(MobPrototype* proto, const SimDamageConfig* damage)
{
    if (!damage) {
        return;
    }

    proto->damage[DICE_NUMBER] = (int16_t)damage->dice;
    proto->damage[DICE_TYPE] = (int16_t)damage->size;
    proto->damage[DICE_BONUS] = (int16_t)damage->bonus;
}

static void proto_set_armor(MobPrototype* proto, int armor)
{
    for (int i = 0; i < AC_COUNT; i++) {
        proto->ac[i] = (int16_t)armor;
    }
}

static void apply_combatant_proto(MobPrototype* proto, const SimCombatantConfig* cfg)
{
    if (!proto || !cfg) {
        return;
    }

    if (cfg->set_level) {
        proto->level = (int16_t)cfg->level;
    }
    if (cfg->set_hitroll) {
        proto->hitroll = (int16_t)cfg->hitroll;
    }
    if (cfg->set_hitpoints) {
        proto_set_fixed_hitpoints(proto, cfg->hitpoints);
    }
    if (cfg->set_damage) {
        proto_set_damage(proto, &cfg->damage);
    }
    if (cfg->set_armor) {
        proto_set_armor(proto, cfg->armor);
    }
}

static void apply_stats_to_mobile(Mobile* ch, const SimCombatantConfig* cfg)
{
    if (!ch || !cfg || !cfg->has_stats) {
        return;
    }

    for (int i = 0; i < STAT_COUNT; i++) {
        ch->perm_stat[i] = (int16_t)cfg->stats[i];
        ch->mod_stat[i] = 0;
    }
}

static bool apply_skills_to_player(Mobile* ch, const SimCombatantConfig* cfg, char* err, size_t err_len)
{
    if (!ch || !cfg) {
        snprintf(err, err_len, "invalid skill config");
        return false;
    }
    if (!cfg->has_skills) {
        return true;
    }

    for (int i = 0; i < cfg->skill_count; i++) {
        SKNUM sn = skill_lookup(cfg->skills[i].name);
        if (sn < 0) {
            snprintf(err, err_len, "unknown skill: %s", cfg->skills[i].name);
            return false;
        }
        int rating = cfg->skills[i].rating;
        if (rating < 0) {
            rating = 0;
        }
        if (rating > 100) {
            rating = 100;
        }
        ch->pcdata->learned[sn] = (int16_t)rating;
    }

    return true;
}

static bool equip_weapon_vnum(Mobile* ch, int vnum, char* err, size_t err_len)
{
    if (!ch || vnum <= 0) {
        return true;
    }

    ObjPrototype* proto = get_object_prototype((VNUM)vnum);
    if (!proto) {
        snprintf(err, err_len, "weapon vnum %d not found", vnum);
        return false;
    }

    Object* obj = create_object(proto, ch->level);
    if (obj->item_type != ITEM_WEAPON) {
        snprintf(err, err_len, "object %d is not a weapon", vnum);
        extract_obj(obj);
        return false;
    }

    obj_to_char(obj, ch);
    equip_char(ch, obj, WEAR_WIELD);
    return true;
}

static void apply_combatant_instance(Mobile* ch, const SimCombatantConfig* cfg)
{
    if (!ch || !cfg) {
        return;
    }

    if (cfg->set_level) {
        ch->level = (int16_t)cfg->level;
    }
    if (cfg->set_hitroll) {
        ch->hitroll = (int16_t)cfg->hitroll;
    }
    if (cfg->set_hitpoints) {
        ch->max_hit = (int16_t)cfg->hitpoints;
        ch->hit = ch->max_hit;
    }
    if (cfg->set_armor) {
        for (int i = 0; i < AC_COUNT; i++) {
            ch->armor[i] = (int16_t)cfg->armor;
        }
    }
    if (cfg->set_damage) {
        ch->damage[DICE_NUMBER] = (int16_t)cfg->damage.dice;
        ch->damage[DICE_TYPE] = (int16_t)cfg->damage.size;
        ch->damage[DICE_BONUS] = (int16_t)cfg->damage.bonus;
        ch->damroll = (int16_t)cfg->damage.bonus;
    }

    apply_stats_to_mobile(ch, cfg);
}

static MobPrototype* create_sim_proto(const SimCombatantConfig* cfg, VNUM vnum)
{
    MobPrototype* proto = new_mob_prototype();

    VNUM_FIELD(proto) = vnum;
    SET_BIT(proto->act_flags, ACT_IS_NPC);
    proto_set_name(proto, cfg->name);
    apply_combatant_proto(proto, cfg);
    proto->mana[DICE_NUMBER] = 1;
    proto->mana[DICE_TYPE] = 1;
    proto->mana[DICE_BONUS] = 0;

    return proto;
}

static bool apply_player_config(Mobile* ch, const SimCombatantConfig* cfg,
                                char* err, size_t err_len)
{
    if (!ch || !cfg) {
        snprintf(err, err_len, "invalid player config");
        return false;
    }

    ch->pcdata = new_player_data();

    if (cfg->name[0] != '\0') {
        SET_NAME(ch, lox_string(cfg->name));
        free_string(ch->short_descr);
        ch->short_descr = str_dup(cfg->name);
    }

    int16_t class_id = class_lookup(cfg->class_name);
    if (class_id < 0) {
        snprintf(err, err_len, "unknown class: %s", cfg->class_name);
        return false;
    }

    int16_t race_id = race_lookup(cfg->race_name);
    if (race_id == 0 && strcasecmp(cfg->race_name, "human") != 0) {
        snprintf(err, err_len, "unknown race: %s", cfg->race_name);
        return false;
    }

    ch->ch_class = class_id;
    ch->race = race_id;

    apply_combatant_instance(ch, cfg);

    if (!apply_skills_to_player(ch, cfg, err, err_len)) {
        return false;
    }

    if (cfg->set_weapon) {
        if (!equip_weapon_vnum(ch, cfg->weapon_vnum, err, err_len)) {
            return false;
        }
    }

    if (!attach_sim_descriptor(ch, err, err_len)) {
        return false;
    }

    return true;
}

static Mobile* create_combatant(const SimCombatantConfig* cfg, MobPrototype* proto,
                                char* err, size_t err_len)
{
    if (!cfg) {
        snprintf(err, err_len, "invalid combatant config");
        return NULL;
    }

    if (cfg->type == COMBATANT_PLAYER) {
        Mobile* ch = new_mobile();
        REMOVE_BIT(ch->act_flags, ACT_IS_NPC);
        if (!apply_player_config(ch, cfg, err, err_len)) {
            free_mobile(ch);
            return NULL;
        }
        if (!apply_spec_to_mobile(ch, cfg, err, err_len)) {
            free_mobile(ch);
            return NULL;
        }
        return ch;
    }

    if (!proto) {
        snprintf(err, err_len, "missing mob prototype");
        return NULL;
    }

    Mobile* ch = create_mobile(proto);
    apply_combatant_instance(ch, cfg);

    if (!apply_spec_to_mobile(ch, cfg, err, err_len)) {
        extract_char(ch, true);
        return NULL;
    }

    if (cfg->set_weapon) {
        if (!equip_weapon_vnum(ch, cfg->weapon_vnum, err, err_len)) {
            extract_char(ch, true);
            return NULL;
        }
    }

    if (cfg->ai.use_spells) {
        if (!attach_sim_descriptor(ch, err, err_len)) {
            extract_char(ch, true);
            return NULL;
        }
    }

    return ch;
}

bool sim_init(SimContext* ctx, const SimConfig* cfg, char* err, size_t err_len)
{
    if (!ctx || !cfg) {
        snprintf(err, err_len, "invalid sim init input");
        return false;
    }

    memset(ctx, 0, sizeof(*ctx));

    VNUM room_vnum = find_free_room_vnum(900000);

    AreaData* area_data = new_area_data();
    SET_NAME(area_data, lox_string("CombatSim"));
    area_data->min_vnum = room_vnum;
    area_data->max_vnum = room_vnum;

    write_value_array(&global_areas, OBJ_VAL(area_data));
    area_data->next = NULL;

    RoomData* room_data = new_room_data();
    SET_NAME(room_data, lox_string("CombatSim Arena"));
    room_data->description = str_dup("A quiet space for combat simulations.\n\r");
    room_data->area_data = area_data;
    room_data->room_flags = 0;
    room_data->sector_type = SECT_INSIDE;
    VNUM_FIELD(room_data) = room_vnum;

    if (!global_room_set(room_data)) {
        snprintf(err, err_len, "failed to register sim room");
        return false;
    }

    Area* area = create_area_instance(area_data, false);
    Room* room = get_room(area, room_vnum);
    if (!room) {
        snprintf(err, err_len, "failed to create sim room instance");
        return false;
    }

    VNUM attacker_vnum = find_free_mob_vnum(910000);
    VNUM defender_vnum = find_free_mob_vnum(attacker_vnum + 1);

    MobPrototype* attacker_proto = NULL;
    MobPrototype* defender_proto = NULL;

    if (cfg->attacker.type == COMBATANT_SIM) {
        attacker_proto = create_sim_proto(&cfg->attacker, attacker_vnum);
    }
    else if (cfg->attacker.type == COMBATANT_MOB) {
        attacker_proto = get_mob_prototype((VNUM)cfg->attacker.mob_vnum);
        if (!attacker_proto) {
            snprintf(err, err_len, "attacker mob vnum %d not found", cfg->attacker.mob_vnum);
            return false;
        }
    }

    if (cfg->defender.type == COMBATANT_SIM) {
        defender_proto = create_sim_proto(&cfg->defender, defender_vnum);
    }
    else if (cfg->defender.type == COMBATANT_MOB) {
        defender_proto = get_mob_prototype((VNUM)cfg->defender.mob_vnum);
        if (!defender_proto) {
            snprintf(err, err_len, "defender mob vnum %d not found", cfg->defender.mob_vnum);
            return false;
        }
    }

    sim_prev_combat = combat;
    sim_combat_ops = *combat;
    sim_combat_ops.handle_death = sim_handle_death;
    combat = &sim_combat_ops;

    sim_prev_metrics = combat_metrics;
    sim_metrics_ops = (CombatMetricsOps){
        .on_attack_roll = sim_on_attack_roll,
        .on_parry = sim_on_parry,
        .on_dodge = sim_on_dodge,
        .on_shield_block = sim_on_shield_block,
    };
    combat_metrics = &sim_metrics_ops;

    ctx->area_data = area_data;
    ctx->room_data = room_data;
    ctx->area = area;
    ctx->room = room;
    ctx->attacker_proto = attacker_proto;
    ctx->defender_proto = defender_proto;

    return true;
}

void sim_shutdown(SimContext* ctx)
{
    if (sim_prev_combat) {
        combat = sim_prev_combat;
        sim_prev_combat = NULL;
    }

    if (sim_prev_metrics) {
        combat_metrics = sim_prev_metrics;
        sim_prev_metrics = NULL;
    }

    sim_active_metrics = NULL;

    (void)ctx;
}

static int sum_positive_damage(int before, int after)
{
    if (before > after) {
        return before - after;
    }
    return 0;
}

static void accumulate_damage(Mobile* attacker, Mobile* defender,
                              int* attacker_mark, int* defender_mark,
                              int* total_damage)
{
    if (!attacker || !defender || !attacker_mark || !defender_mark || !total_damage) {
        return;
    }

    *total_damage += sum_positive_damage(*attacker_mark, attacker->hit);
    *total_damage += sum_positive_damage(*defender_mark, defender->hit);
    *attacker_mark = attacker->hit;
    *defender_mark = defender->hit;
}

bool sim_run(SimContext* ctx, const SimConfig* cfg, SimRunMetrics* out)
{
    if (!ctx || !cfg || !out) {
        return false;
    }

    *out = (SimRunMetrics){ 0 };
    sim_active_metrics = out;

    if (cfg->attacker.type == COMBATANT_SIM) {
        apply_combatant_proto(ctx->attacker_proto, &cfg->attacker);
    }
    if (cfg->defender.type == COMBATANT_SIM) {
        apply_combatant_proto(ctx->defender_proto, &cfg->defender);
    }

    char err[256] = { 0 };
    Mobile* attacker = create_combatant(&cfg->attacker, ctx->attacker_proto, err, sizeof(err));
    if (!attacker) {
        fprintf(stderr, "attacker setup failed: %s\n", err);
        sim_active_metrics = NULL;
        return false;
    }
    Mobile* defender = create_combatant(&cfg->defender, ctx->defender_proto, err, sizeof(err));
    if (!defender) {
        fprintf(stderr, "defender setup failed: %s\n", err);
        detach_sim_descriptor(attacker);
        extract_char(attacker, true);
        sim_active_metrics = NULL;
        return false;
    }

    if (cfg->attacker.set_hitpoints) {
        attacker->max_hit = (int16_t)cfg->attacker.hitpoints;
        attacker->hit = attacker->max_hit;
    }
    if (cfg->defender.set_hitpoints) {
        defender->max_hit = (int16_t)cfg->defender.hitpoints;
        defender->hit = defender->max_hit;
    }

    attacker->position = POS_STANDING;
    defender->position = POS_STANDING;

    mob_to_room(attacker, ctx->room);
    mob_to_room(defender, ctx->room);

    set_fighting(attacker, defender);
    set_fighting(defender, attacker);

    int ticks = 0;
    int total_damage = 0;
    int pulse_accum = 0;
    bool attacker_dead = false;
    bool defender_dead = false;

    int att_proto_kill_count = attacker->prototype ? attacker->prototype->killed : 0;
    int def_proto_kill_count = defender->prototype ? defender->prototype->killed : 0;

    SimAiRuntime attacker_ai = { 0 };
    SimAiRuntime defender_ai = { 0 };

    for (ticks = 1; ticks <= cfg->max_ticks; ticks++) {
        int attacker_mark = attacker->hit;
        int defender_mark = defender->hit;

        sim_advance_wait(attacker);
        sim_advance_wait(defender);

        sim_ai_tick_cooldowns(&cfg->attacker, &attacker_ai);
        sim_ai_tick_cooldowns(&cfg->defender, &defender_ai);

        pulse_accum += PULSE_VIOLENCE;
        if (pulse_accum >= PULSE_MOBILE) {
            pulse_accum -= PULSE_MOBILE;

            sim_ai_run_spec(attacker, &cfg->attacker);
            accumulate_damage(attacker, defender, &attacker_mark, &defender_mark, &total_damage);
            if (def_proto_kill_count < (defender->prototype ? defender->prototype->killed : 0)
                || sim_handle_terminal(defender)) {
                defender_dead = true;
                break;
            }
            if (att_proto_kill_count < (attacker->prototype ? attacker->prototype->killed : 0)
                || sim_handle_terminal(attacker)) {
                attacker_dead = true;
                break;
            }

            sim_ai_run_spec(defender, &cfg->defender);
            accumulate_damage(attacker, defender, &attacker_mark, &defender_mark, &total_damage);
            if (att_proto_kill_count < (attacker->prototype ? attacker->prototype->killed : 0)
                || sim_handle_terminal(attacker)) {
                attacker_dead = true;
                break;
            }
            if (def_proto_kill_count < (defender->prototype ? defender->prototype->killed : 0)
                || sim_handle_terminal(defender)) {
                defender_dead = true;
                break;
            }
        }

        if (sim_ai_try_cast_spell(attacker, defender, &cfg->attacker, &attacker_ai)) {
            accumulate_damage(attacker, defender, &attacker_mark, &defender_mark, &total_damage);
            if (def_proto_kill_count < (defender->prototype ? defender->prototype->killed : 0)
                || sim_handle_terminal(defender)) {
                defender_dead = true;
                break;
            }
            if (att_proto_kill_count < (attacker->prototype ? attacker->prototype->killed : 0)
                || sim_handle_terminal(attacker)) {
                attacker_dead = true;
                break;
            }
        }

        if (sim_ai_try_cast_spell(defender, attacker, &cfg->defender, &defender_ai)) {
            accumulate_damage(attacker, defender, &attacker_mark, &defender_mark, &total_damage);
            if (att_proto_kill_count < (attacker->prototype ? attacker->prototype->killed : 0)
                || sim_handle_terminal(attacker)) {
                attacker_dead = true;
                break;
            }
            if (def_proto_kill_count < (defender->prototype ? defender->prototype->killed : 0)
                || sim_handle_terminal(defender)) {
                defender_dead = true;
                break;
            }
        }

        if (cfg->attacker.ai.auto_attack) {
            multi_hit(attacker, defender, TYPE_UNDEFINED);
            accumulate_damage(attacker, defender, &attacker_mark, &defender_mark, &total_damage);

            if (def_proto_kill_count < (defender->prototype ? defender->prototype->killed : 0)
                || sim_handle_terminal(defender)) {
                defender_dead = true;
                break;
            }

            if (att_proto_kill_count < (attacker->prototype ? attacker->prototype->killed : 0)
                || sim_handle_terminal(attacker)) {
                attacker_dead = true;
                break;
            }
        }

        if (cfg->defender.ai.auto_attack) {
            multi_hit(defender, attacker, TYPE_UNDEFINED);
            accumulate_damage(attacker, defender, &attacker_mark, &defender_mark, &total_damage);

            if (att_proto_kill_count < (attacker->prototype ? attacker->prototype->killed : 0)
                || sim_handle_terminal(attacker)) {
                attacker_dead = true;
                break;
            }

            if (def_proto_kill_count < (defender->prototype ? defender->prototype->killed : 0)
                || sim_handle_terminal(defender)) {
                defender_dead = true;
                break;
            }
        }
    }

    bool timed_out = false;
    if (!attacker_dead && !defender_dead) {
        if (ticks > cfg->max_ticks) {
            ticks = cfg->max_ticks;
        }
        timed_out = true;
    }

    out->ticks = ticks;
    out->total_damage = total_damage;
    out->attacker_won = defender_dead && !attacker_dead;
    out->defender_won = attacker_dead && !defender_dead;
    out->timed_out = timed_out;

    detach_sim_descriptor(attacker);
    detach_sim_descriptor(defender);

    extract_char(attacker, true);
    extract_char(defender, true);

    sim_active_metrics = NULL;

    return true;
}
