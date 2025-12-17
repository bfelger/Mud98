////////////////////////////////////////////////////////////////////////////////
// damage_tests.c - Core damage application tests
// These tests characterize existing damage behavior to provide a safety net
// for future combat seam refactoring.
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "mock.h"
#include "test_registry.h"

#include <fight.h>
#include <handler.h>
#include <db.h>
#include <merc.h>
#include <rng.h>

#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>
#include <entities/event.h>

#include <data/mobile_data.h>
#include <data/player.h>

extern bool test_socket_output_enabled;
extern Value test_output_buffer;
extern RngOps mock_rng;
extern void reset_mock_rng(void);

// Test basic damage application
static int test_basic_damage()
{
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();
    
    Room* room = mock_room(1, NULL, NULL);
    
    Mobile* attacker = mock_player("Attacker");
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(2);
    Mobile* victim = mock_mob("Victim", 2, proto);
    victim->hit = 100;
    victim->max_hit = 100;
    transfer_mob(victim, room);

    test_socket_output_enabled = true;
    bool result = damage(attacker, victim, 50, TYPE_HIT, DAM_BASH, false);
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;

    // DEBUG: Print actual HP
    if (victim->hit != 58) {
        printf("DEBUG: Expected HP=58, got HP=%d\n", victim->hit);
    }
    
    ASSERT(result == true);  // Damage applied successfully
    ASSERT(victim->hit == 58);  // HP reduced (some damage avoided via dodge/parry)
    // Note: position may be POS_FIGHTING if combat was initiated

    rng = saved_rng;
    return 0;
}

// Test damage cannot reduce HP below zero
static int test_damage_exceeds_hp()
{
    Room* room = mock_room(1, NULL, NULL);
    
    Mobile* attacker = mock_player("Attacker");
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(2);
    Mobile* victim = mock_mob("Victim", 2, proto);
    victim->hit = 30;
    victim->max_hit = 100;
    transfer_mob(victim, room);

    test_socket_output_enabled = true;
    bool result = damage(attacker, victim, 100, TYPE_HIT, DAM_BASH, false);
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;

    ASSERT(result == true);  // Damage applied
    ASSERT(victim->hit <= 0);  // HP at or below zero (death)
    ASSERT(victim->position == POS_DEAD);  // Victim is dead

    return 0;
}

// Test damage to already-dead mob is rejected
static int test_damage_already_dead()
{
    Room* room = mock_room(1, NULL, NULL);
    
    Mobile* attacker = mock_player("Attacker");
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(2);
    Mobile* victim = mock_mob("Victim", 2, proto);
    victim->hit = 0;
    victim->max_hit = 100;
    victim->position = POS_DEAD;
    transfer_mob(victim, room);

    test_socket_output_enabled = true;
    bool result = damage(attacker, victim, 50, TYPE_HIT, DAM_BASH, false);
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;

    ASSERT(result == false);  // Damage rejected
    ASSERT(victim->hit == 0);  // HP unchanged

    return 0;
}

// Test zero damage
static int test_zero_damage()
{
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();
    
    Room* room = mock_room(1, NULL, NULL);
    
    Mobile* attacker = mock_player("Attacker");
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(2);
    Mobile* victim = mock_mob("Victim", 2, proto);
    victim->hit = 100;
    victim->max_hit = 100;
    transfer_mob(victim, room);

    test_socket_output_enabled = true;
    bool result = damage(attacker, victim, 0, TYPE_HIT, DAM_BASH, false);
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;

    // DEBUG
    if (result) {
        printf("DEBUG: Zero damage returned true (expected false)\n");
    }
    
    // Zero damage doesn't initiate combat, so damage() returns false
    ASSERT(result == false);
    ASSERT(victim->hit == 100);  // HP unchanged

    rng = saved_rng;
    return 0;
}

// Test damage caps at 1200 (anti-cheat protection)
static int test_damage_cap()
{
    Room* room = mock_room(1, NULL, NULL);
    
    Mobile* attacker = mock_player("Attacker");
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(2);
    Mobile* victim = mock_mob("Victim", 2, proto);
    victim->hit = 2000;  // High HP to survive capped damage
    victim->max_hit = 2000;
    transfer_mob(victim, room);

    test_socket_output_enabled = true;
    bool result = damage(attacker, victim, 5000, TYPE_HIT, DAM_BASH, false);
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;

    ASSERT(result == true);
    // Damage is capped at 1200, then damage reduction applies
    // Original: dam > 35: (dam - 35) / 2 + 35
    //           dam > 80: (dam - 80) / 2 + 80
    // For 1200: (1200 - 35) / 2 + 35 = 617.5
    //           (617.5 - 80) / 2 + 80 = 348.75
    // Expected final: ~349 damage
    ASSERT(victim->hit > 1500);  // Damage was significantly reduced

    return 0;
}

// Test damage initiates combat
static int test_damage_starts_combat()
{
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();
    
    Room* room = mock_room(1, NULL, NULL);
    
    Mobile* attacker = mock_player("Attacker");
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(2);
    Mobile* victim = mock_mob("Victim", 2, proto);
    victim->hit = 100;
    victim->max_hit = 100;
    victim->level = 10;
    transfer_mob(victim, room);

    ASSERT(attacker->fighting == NULL);
    ASSERT(victim->fighting == NULL);

    test_socket_output_enabled = true;
    damage(attacker, victim, 10, TYPE_HIT, DAM_BASH, false);
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;

    ASSERT(attacker->fighting == victim);  // Attacker now fighting victim
    ASSERT(victim->fighting == attacker);  // Victim fights back
    ASSERT(victim->position == POS_FIGHTING);  // Victim in combat stance

    rng = saved_rng;
    return 0;
}

// Test sanctuary halves damage
static int test_sanctuary_reduction()
{
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();
    
    Room* room = mock_room(1, NULL, NULL);
    
    Mobile* attacker = mock_player("Attacker");
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(2);
    Mobile* victim = mock_mob("Victim", 2, proto);
    victim->hit = 100;
    victim->max_hit = 100;
    victim->affect_flags = AFF_SANCTUARY;
    transfer_mob(victim, room);

    test_socket_output_enabled = true;
    damage(attacker, victim, 40, TYPE_HIT, DAM_BASH, false);
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;

    // DEBUG
    if (victim->hit != 82) {
        printf("DEBUG: Sanctuary test expected HP=82, got HP=%d\n", victim->hit);
    }
    
    // Sanctuary halves damage: 40 / 2 = 20, but some avoided
    ASSERT(victim->hit == 82);

    rng = saved_rng;
    return 0;
}

// Test damage reduction for high values
static int test_damage_scaling()
{
    Room* room = mock_room(1, NULL, NULL);
    
    Mobile* attacker = mock_player("Attacker");
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(2);
    Mobile* victim = mock_mob("Victim", 2, proto);
    victim->hit = 500;
    victim->max_hit = 500;
    transfer_mob(victim, room);

    test_socket_output_enabled = true;
    damage(attacker, victim, 100, TYPE_HIT, DAM_BASH, false);
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;

    // Damage reduction for dam > 80:
    // (100 - 35) / 2 + 35 = 67.5
    // (67.5 - 80) / 2 + 80 = ... wait, 67.5 < 80, so only first reduction
    // Expected: 67 or 68 damage
    ASSERT(victim->hit > 430);  // Verify reduction applied
    ASSERT(victim->hit < 435);

    return 0;
}

// Test damage to self
static int test_self_damage()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* victim = mock_player("Victim");
    victim->hit = 100;
    victim->max_hit = 100;
    transfer_mob(victim, room);

    test_socket_output_enabled = true;
    damage(victim, victim, 20, TYPE_HIT, DAM_BASH, false);
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;

    ASSERT(victim->hit == 80);  // Self-damage applies
    ASSERT(victim->fighting == NULL);  // Self-damage doesn't start combat

    return 0;
}

// Test negative damage (should be treated as zero)
static int test_negative_damage()
{
    Room* room = mock_room(1, NULL, NULL);
    
    Mobile* attacker = mock_player("Attacker");
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    Mobile* victim = mock_player("Victim");
    victim->hit = 100;
    victim->max_hit = 100;
    transfer_mob(victim, room);

    test_socket_output_enabled = true;
    damage(attacker, victim, -50, TYPE_HIT, DAM_BASH, false);
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;

    // Negative damage might heal or be ignored
    // This test characterizes current behavior
    ASSERT(victim->hit >= 100);  // HP not reduced

    return 0;
}

// Test damage with different damage types
static int test_damage_types()
{
    Room* room = mock_room(1, NULL, NULL);
    
    // Test each major damage type to ensure they work
    DamageType types[] = {
        DAM_BASH, DAM_PIERCE, DAM_SLASH, DAM_FIRE, DAM_COLD,
        DAM_LIGHTNING, DAM_ACID, DAM_POISON, DAM_NEGATIVE, DAM_HOLY,
        DAM_ENERGY, DAM_MENTAL, DAM_DISEASE, DAM_DROWNING, DAM_LIGHT
    };
    
    Mobile* attacker = mock_player("Attacker");
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(2);
    
    for (int i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        Mobile* victim = mock_mob("Victim", 2, proto);
        victim->hit = 100;
        victim->max_hit = 100;
        transfer_mob(victim, room);
        
        test_socket_output_enabled = true;
        bool result = damage(attacker, victim, 10, TYPE_HIT, types[i], false);
        test_socket_output_enabled = false;
        test_output_buffer = NIL_VAL;
        
        ASSERT(result == true);  // All types should work
        ASSERT(victim->hit == 90);  // Standard damage applied
    }

    return 0;
}

// Test stop_fighting clears combat state
static int test_stop_fighting()
{
    Room* room = mock_room(1, NULL, NULL);
    
    Mobile* ch = mock_player("Fighter");
    transfer_mob(ch, room);
    
    Mobile* victim = mock_player("Victim");
    transfer_mob(victim, room);
    
    // Set up combat
    ch->fighting = victim;
    ch->position = POS_FIGHTING;
    victim->fighting = ch;
    victim->position = POS_FIGHTING;
    
    // Stop fighting
    stop_fighting(ch, false);
    
    ASSERT(ch->fighting == NULL);
    ASSERT(ch->position == POS_STANDING);
    // Victim still fighting (one-sided stop)
    ASSERT(victim->fighting == ch);

    return 0;
}

// Note: set_fighting is not exported in fight.h, so we test it indirectly through damage()

void register_damage_tests()
{
    TestGroup* group = calloc(1, sizeof(TestGroup));
    init_test_group(group, "DAMAGE TESTS");

#define REGISTER(name, func) register_test(group, (name), (func))

    // Basic damage mechanics
    REGISTER("Damage: Basic application", test_basic_damage);
    REGISTER("Damage: Exceeds HP", test_damage_exceeds_hp);
    REGISTER("Damage: Already dead", test_damage_already_dead);
    REGISTER("Damage: Zero damage", test_zero_damage);
    REGISTER("Damage: Negative damage", test_negative_damage);
    REGISTER("Damage: Cap at 1200", test_damage_cap);
    REGISTER("Damage: Scaling reduction", test_damage_scaling);
    
    // Damage modifiers
    REGISTER("Damage: Sanctuary halves", test_sanctuary_reduction);
    REGISTER("Damage: All damage types", test_damage_types);
    
    // Combat state
    REGISTER("Damage: Starts combat", test_damage_starts_combat);
    REGISTER("Damage: Self damage", test_self_damage);
    REGISTER("Combat: Stop fighting", test_stop_fighting);

#undef REGISTER

    register_test_group(group);
}
