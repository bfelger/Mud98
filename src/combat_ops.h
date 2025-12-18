////////////////////////////////////////////////////////////////////////////////
// combat_ops.h
// Combat system abstraction layer.
//
// Provides a seam for swapping combat implementations (ROM/THAC0 vs d20, etc.)
// This enables:
// - Testing with mock combat (no actual HP modification)
// - Alternative combat systems without touching game logic
// - Clean separation between combat mechanics and game state
//
// Pattern follows RngOps and EventOps - global function pointer table that can
// be swapped at runtime.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__COMBAT_OPS_H
#define MUD98__COMBAT_OPS_H

#include "merc.h"

#include <data/damage.h>

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct mobile_t Mobile;
typedef struct object_t Object;

////////////////////////////////////////////////////////////////////////////////
// COMBAT OPERATIONS TABLE
////////////////////////////////////////////////////////////////////////////////

typedef struct combat_ops_t {
    // -------------------------------------------------------------------------
    // DAMAGE & HEALING
    // -------------------------------------------------------------------------
    
    // Apply damage to a victim.
    // This is the PRIMARY damage entry point - nearly all damage flows here.
    // 
    // Returns: true if victim survived, false if died
    //
    // Parameters:
    //   ch        - Attacker (NULL for environmental damage)
    //   victim    - Target receiving damage
    //   dam       - Raw damage amount (before modifiers)
    //   dt        - Damage type (skill number or TYPE_UNDEFINED/TYPE_HIT)
    //   dam_type  - Damage category (slash, pierce, bash, fire, etc.)
    //   show      - Whether to display damage messages
    bool (*apply_damage)(Mobile* ch, Mobile* victim, int dam, 
                         int16_t dt, DamageType dam_type, bool show);
    
    // Apply healing to a character.
    // Used by healing spells, potions, regeneration, etc.
    //
    // Parameters:
    //   ch     - Character being healed
    //   amount - HP to restore (will be capped at max_hit)
    void (*apply_healing)(Mobile* ch, int amount);
    
    // Apply natural regeneration (periodic tick healing).
    //
    // Parameters:
    //   ch        - Character regenerating
    //   hp_gain   - HP to restore
    //   mana_gain - Mana to restore
    //   move_gain - Movement to restore
    void (*apply_regen)(Mobile* ch, int hp_gain, int mana_gain, int move_gain);
    
    // -------------------------------------------------------------------------
    // RESOURCE MANAGEMENT
    // -------------------------------------------------------------------------
    
    // Drain mana from a character (skill costs, etc.)
    void (*drain_mana)(Mobile* ch, int amount);
    
    // Drain movement from a character (travel costs, etc.)
    void (*drain_move)(Mobile* ch, int amount);
    
    // -------------------------------------------------------------------------
    // TO-HIT MECHANICS (THAC0 / d20 / etc.)
    // -------------------------------------------------------------------------
    
    // Check if an attack hits.
    // Encapsulates to-hit calculation (THAC0 vs AC, d20 roll, etc.)
    //
    // Returns: true if attack hits, false if misses
    //
    // Parameters:
    //   ch        - Attacker
    //   victim    - Defender
    //   weapon    - Weapon being used (NULL for unarmed)
    //   dt        - Damage type/skill number (TYPE_HIT, gsn_backstab, etc.)
    bool (*check_hit)(Mobile* ch, Mobile* victim, Object* weapon, int16_t dt);
    
    // Calculate base damage for an attack.
    // Factors in weapon dice, skill %, strength, level, etc.
    // Does NOT apply armor/resistance - that happens in apply_damage.
    //
    // Returns: Base damage amount
    //
    // Parameters:
    //   ch         - Attacker
    //   victim     - Defender (for context, not damage reduction)
    //   weapon     - Weapon being used (NULL for unarmed)
    //   dt         - Damage type/skill number (TYPE_HIT, gsn_backstab, etc.)
    //   is_offhand - Whether this is an offhand/dual-wield attack
    int (*calculate_damage)(Mobile* ch, Mobile* victim, Object* weapon, 
                           int16_t dt, bool is_offhand);
    
    // -------------------------------------------------------------------------
    // DEATH & CORPSES
    // -------------------------------------------------------------------------
    
    // Handle character death.
    // Creates corpse, transfers equipment, handles PC vs NPC differences.
    //
    // Parameters:
    //   victim - Character who died
    void (*handle_death)(Mobile* victim);
    
} CombatOps;

// Global combat ops pointer.
// Defaults to ROM/THAC0 implementation (combat_rom.c).
// Tests can swap to mock implementations (tests/mock_combat.c).
extern CombatOps* combat;

// ROM/THAC0 combat ops table (default production implementation)
extern CombatOps combat_rom;

// Convenience macros for calling through combat ops.
// These provide drop-in compatibility with existing code patterns.
#define COMBAT_DAMAGE(ch, victim, dam, dt, dam_type, show) \
    (combat->apply_damage((ch), (victim), (dam), (dt), (dam_type), (show)))

#define COMBAT_HEAL(ch, amount) \
    (combat->apply_healing((ch), (amount)))

#define COMBAT_REGEN(ch, hp, mana, move) \
    (combat->apply_regen((ch), (hp), (mana), (move)))

#define COMBAT_DRAIN_MANA(ch, amount) \
    (combat->drain_mana((ch), (amount)))

#define COMBAT_DRAIN_MOVE(ch, amount) \
    (combat->drain_move((ch), (amount)))

#define COMBAT_CHECK_HIT(ch, victim, weapon, dt) \
    (combat->check_hit((ch), (victim), (weapon), (dt)))

#define COMBAT_CALC_DAMAGE(ch, victim, weapon, dt, offhand) \
    (combat->calculate_damage((ch), (victim), (weapon), (dt), (offhand)))

#define COMBAT_DEATH(victim) \
    (combat->handle_death((victim)))

#endif // !MUD98__COMBAT_OPS_H
