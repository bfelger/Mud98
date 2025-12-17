////////////////////////////////////////////////////////////////////////////////
// combat_rom.c
// ROM 2.4 / THAC0 combat implementation.
//
// This wraps the existing fight.c implementation in the CombatOps interface.
// Initially these are pass-through stubs; we'll gradually extract logic as we
// refactor fight.c to use the ops table.
////////////////////////////////////////////////////////////////////////////////

#include "combat_ops.h"

#include "fight.h"
#include "handler.h"

#include <entities/mobile.h>
#include <entities/object.h>

// Forward declarations of fight.c functions we're wrapping
extern bool damage(Mobile* ch, Mobile* victim, int dam, int16_t dt, DamageType dam_type, bool show);
extern void raw_kill(Mobile* victim);
extern void update_pos(Mobile* victim);

////////////////////////////////////////////////////////////////////////////////
// ROM COMBAT OPERATIONS (Pass-through stubs for now)
////////////////////////////////////////////////////////////////////////////////

static bool rom_apply_damage(Mobile* ch, Mobile* victim, int dam, 
                              int16_t dt, DamageType dam_type, bool show)
{
    // Delegate to existing damage() function in fight.c
    return damage(ch, victim, dam, dt, dam_type, show);
}

static void rom_apply_healing(Mobile* ch, int amount)
{
    if (amount <= 0)
        return;
    
    ch->hit += (int16_t)amount;
    if (ch->hit > ch->max_hit)
        ch->hit = ch->max_hit;
    
    update_pos(ch);
}

static void rom_apply_regen(Mobile* ch, int hp_gain, int mana_gain, int move_gain)
{
    // HP regeneration
    if (hp_gain > 0) {
        ch->hit += (int16_t)hp_gain;
        if (ch->hit > ch->max_hit)
            ch->hit = ch->max_hit;
    }
    
    // Mana regeneration
    if (mana_gain > 0) {
        ch->mana += (int16_t)mana_gain;
        if (ch->mana > ch->max_mana)
            ch->mana = ch->max_mana;
    }
    
    // Move regeneration
    if (move_gain > 0) {
        ch->move += (int16_t)move_gain;
        if (ch->move > ch->max_move)
            ch->move = ch->max_move;
    }
    
    update_pos(ch);
}

static void rom_drain_mana(Mobile* ch, int amount)
{
    ch->mana -= (int16_t)amount;
    if (ch->mana < 0)
        ch->mana = 0;
}

static void rom_drain_move(Mobile* ch, int amount)
{
    ch->move -= (int16_t)amount;
    if (ch->move < 0)
        ch->move = 0;
}

static bool rom_check_hit(Mobile* ch, Mobile* victim, Object* weapon, int16_t sn)
{
    // TODO: Extract THAC0 logic from one_hit() in fight.c
    // For now, return true (we're not changing behavior yet)
    (void)ch;
    (void)victim;
    (void)weapon;
    (void)sn;
    return true;
}

static int rom_calculate_damage(Mobile* ch, Mobile* victim, Object* weapon, 
                                int16_t sn, bool is_offhand)
{
    // TODO: Extract damage calculation from one_hit() in fight.c
    // For now, return 0 (we're not changing behavior yet)
    (void)ch;
    (void)victim;
    (void)weapon;
    (void)sn;
    (void)is_offhand;
    return 0;
}

static void rom_handle_death(Mobile* victim)
{
    // Delegate to existing raw_kill() function in fight.c
    raw_kill(victim);
}

////////////////////////////////////////////////////////////////////////////////
// ROM COMBAT OPS TABLE
////////////////////////////////////////////////////////////////////////////////

CombatOps combat_rom = {
    .apply_damage = rom_apply_damage,
    .apply_healing = rom_apply_healing,
    .apply_regen = rom_apply_regen,
    .drain_mana = rom_drain_mana,
    .drain_move = rom_drain_move,
    .check_hit = rom_check_hit,
    .calculate_damage = rom_calculate_damage,
    .handle_death = rom_handle_death,
};

// Global combat pointer (defaults to ROM implementation)
CombatOps* combat = &combat_rom;
