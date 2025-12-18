////////////////////////////////////////////////////////////////////////////////
// tests/mock_combat.c
// Mock combat implementation for unit testing.
//
// Provides deterministic, observable combat behavior for tests:
// - Tracks damage/healing without actually modifying HP
// - Configurable hit/miss behavior
// - Records combat events for assertions
// - Can be reset between tests
//
// This allows tests to verify combat logic without dealing with RNG or
// complex state management.
////////////////////////////////////////////////////////////////////////////////

#include "mock.h"

#include "combat_ops.h"
#include "fight.h"
#include "handler.h"

#include <entities/mobile.h>
#include <entities/object.h>

#include <stdio.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// MOCK STATE
////////////////////////////////////////////////////////////////////////////////

// Mock configuration
static bool mock_always_hit = true;     // Whether attacks always hit
static bool mock_always_crit = false;   // Whether attacks always crit
static int mock_damage_override = -1;   // Override damage amount (-1 = use real calculation)
static bool mock_prevent_death = false; // Prevent actual death (for death testing)

// Mock tracking
static int mock_damage_dealt = 0;       // Total damage dealt this session
static int mock_healing_done = 0;       // Total healing done
static int mock_deaths = 0;             // Number of deaths
static bool mock_last_attack_hit = false;

// Reset mock state between tests
void reset_mock_combat(void)
{
    mock_always_hit = true;
    mock_always_crit = false;
    mock_damage_override = -1;
    mock_prevent_death = false;
    mock_damage_dealt = 0;
    mock_healing_done = 0;
    mock_deaths = 0;
    mock_last_attack_hit = false;
}

// Configuration functions
void set_mock_always_hit(bool value) { mock_always_hit = value; }
void set_mock_damage_override(int damage) { mock_damage_override = damage; }
void set_mock_prevent_death(bool value) { mock_prevent_death = value; }

// Query functions
int get_mock_damage_dealt(void) { return mock_damage_dealt; }
int get_mock_healing_done(void) { return mock_healing_done; }
int get_mock_deaths(void) { return mock_deaths; }
bool get_mock_last_attack_hit(void) { return mock_last_attack_hit; }

////////////////////////////////////////////////////////////////////////////////
// MOCK COMBAT OPERATIONS
////////////////////////////////////////////////////////////////////////////////

static bool mock_apply_damage(Mobile* ch, Mobile* victim, int dam, 
                               int16_t dt, DamageType dam_type, bool show)
{
    // Track damage
    mock_damage_dealt += dam;
    
    // Apply damage normally (tests need actual HP changes for assertions)
    // But track it for test verification
    victim->hit -= (int16_t)dam;
    
    // Immortals don't die
    if (!IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL && victim->hit < 1)
        victim->hit = 1;
    
    // Update position
    update_pos(victim);
    
    // Check for death
    if (victim->hit <= 0 && !mock_prevent_death) {
        victim->position = POS_DEAD;
        mock_deaths++;
        
        // Don't actually call death handler in mock - tests may want to verify state
        // Tests can call raw_kill() explicitly if needed
        return false;
    }
    
    return true;
}

static void mock_apply_healing(Mobile* ch, int amount)
{
    if (amount <= 0)
        return;
    
    mock_healing_done += amount;
    
    ch->hit += (int16_t)amount;
    if (ch->hit > ch->max_hit)
        ch->hit = ch->max_hit;
    
    update_pos(ch);
}

static void mock_apply_regen(Mobile* ch, int hp_gain, int mana_gain, int move_gain)
{
    // Track healing
    if (hp_gain > 0)
        mock_healing_done += hp_gain;
    
    // Apply regen
    if (hp_gain > 0) {
        ch->hit += (int16_t)hp_gain;
        if (ch->hit > ch->max_hit)
            ch->hit = ch->max_hit;
    }
    
    if (mana_gain > 0) {
        ch->mana += (int16_t)mana_gain;
        if (ch->mana > ch->max_mana)
            ch->mana = ch->max_mana;
    }
    
    if (move_gain > 0) {
        ch->move += (int16_t)move_gain;
        if (ch->move > ch->max_move)
            ch->move = ch->max_move;
    }
    
    update_pos(ch);
}

static void mock_drain_mana(Mobile* ch, int amount)
{
    ch->mana -= (int16_t)amount;
    if (ch->mana < 0)
        ch->mana = 0;
}

static void mock_drain_move(Mobile* ch, int amount)
{
    ch->move -= (int16_t)amount;
    if (ch->move < 0)
        ch->move = 0;
}

static bool mock_check_hit(Mobile* ch, Mobile* victim, Object* weapon, int16_t dt)
{
    // Mock can override hit behavior
    if (mock_always_hit) {
        mock_last_attack_hit = true;
        return true;
    }
    
    // Otherwise use real THAC0 calculation (from combat_rom.c)
    // For now, just delegate to ROM implementation
    extern CombatOps combat_rom;
    bool hit = combat_rom.check_hit(ch, victim, weapon, dt);
    mock_last_attack_hit = hit;
    return hit;
}

static int mock_calculate_damage(Mobile* ch, Mobile* victim, Object* weapon, 
                                 int16_t dt, bool is_offhand)
{
    // Override damage if configured
    if (mock_damage_override >= 0)
        return mock_damage_override;
    
    // Otherwise use real calculation
    extern CombatOps combat_rom;
    return combat_rom.calculate_damage(ch, victim, weapon, dt, is_offhand);
}

static void mock_handle_death(Mobile* victim)
{
    mock_deaths++;
    
    // For tests, we often want to inspect state before death handler runs
    // So just mark as dead, don't actually extract/create corpse
    // Tests can call the real death handler if needed
    stop_fighting(victim, true);
    victim->position = POS_DEAD;
    victim->hit = -10;  // Clearly dead
}

////////////////////////////////////////////////////////////////////////////////
// MOCK COMBAT OPS TABLE
////////////////////////////////////////////////////////////////////////////////

CombatOps mock_combat = {
    .apply_damage = mock_apply_damage,
    .apply_healing = mock_apply_healing,
    .apply_regen = mock_apply_regen,
    .drain_mana = mock_drain_mana,
    .drain_move = mock_drain_move,
    .check_hit = mock_check_hit,
    .calculate_damage = mock_calculate_damage,
    .handle_death = mock_handle_death,
};
