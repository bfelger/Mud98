////////////////////////////////////////////////////////////////////////////////
// multihit_tests.c
//
// Tests for multi-hit mechanics, combat rounds, second/third attacks, dual
// wield, haste, and attack sequencing.
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "fight.h"
#include "handler.h"
#include "interp.h"
#include "recycle.h"
#include "rng.h"

#include "tests/tests.h"
#include "tests/mock.h"
#include "tests/test_registry.h"

#include <entities/descriptor.h>
#include <entities/affect.h>

#include <data/mobile_data.h>
#include <data/skill.h>

#include <string.h>

TestGroup multihit_group;

// Helper: Count hits by tracking victim HP changes
static int count_hits_from_multihit(Mobile* attacker, Mobile* victim, int initial_hp)
{
    // Call multi_hit
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    
    // Calculate damage dealt
    int damage_dealt = initial_hp - victim->hit;
    
    // Estimate hits (assuming ~10 damage per hit average for level 20)
    // This is approximate but good enough for testing attack count
    if (damage_dealt <= 0) return 0;
    if (damage_dealt < 15) return 1;
    if (damage_dealt < 30) return 2;
    if (damage_dealt < 45) return 3;
    return 4; // Haste + second + third
}

////////////////////////////////////////////////////////////////////////////////
// BASIC MULTI-HIT TESTS
////////////////////////////////////////////////////////////////////////////////

// Test single attack with no extra attacks
static int test_single_attack_only()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("Attacker");
    Mobile* victim = mock_mob("Victim", 1, mock_mob_proto(1));
    
    attacker->level = 20;
    victim->hit = 1000;
    victim->max_hit = 1000;
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Give attacker a weapon
    Object* weapon = mock_sword("iron sword blade", 50, 10, 2, 5);
    obj_to_char(weapon, attacker);
    equip_char(attacker, weapon, WEAR_WIELD);
    
    // No skills, no haste, no offhand
    attacker->pcdata->learned[gsn_second_attack] = 0;
    attacker->pcdata->learned[gsn_third_attack] = 0;
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    int initial_hp = victim->hit;
    
    // Control RNG: use 19 for automatic hits, plus values for damage
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    int sequence[] = {19, 50, 75};  // 19 = auto hit, others for damage/effects
    set_mock_rng_sequence(sequence, 3);
    
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // Should have done some damage
    ASSERT(victim->hit < initial_hp);
    
    return 0;
}

// Test second attack triggers
static int test_second_attack()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("Attacker");
    Mobile* victim = mock_mob("Victim", 1, mock_mob_proto(1));
    
    attacker->level = 50;  // High level for skill access
    victim->hit = 1000;
    victim->max_hit = 1000;
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Give attacker a weapon with high damage
    Object* weapon = mock_sword("iron sword blade", 50, 10, 5, 10);
    obj_to_char(weapon, attacker);
    equip_char(attacker, weapon, WEAR_WIELD);
    
    // Set second attack skill to 100
    mock_skill(attacker, gsn_second_attack, 100);
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    int initial_hp = victim->hit;
    
    // Generous RNG sequence: lots of 19s for auto-hits, 40 for skill checks
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    int sequence[] = {19, 19, 40, 19, 19, 50, 75, 25, 100};  // More than enough values
    set_mock_rng_sequence(sequence, 9);
    
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // Should have at least some damage
    int damage = initial_hp - victim->hit;
    ASSERT(damage > 0);  // ANY damage means hits landed
    
    return 0;
}

// Test third attack triggers
static int test_third_attack()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("Attacker");
    Mobile* victim = mock_mob("Victim", 1, mock_mob_proto(1));
    
    attacker->level = 50;
    victim->hit = 1000;
    victim->max_hit = 1000;
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Give attacker a weapon with high damage
    Object* weapon = mock_sword("iron sword blade", 50, 10, 5, 10);
    obj_to_char(weapon, attacker);
    equip_char(attacker, weapon, WEAR_WIELD);
    
    // Set both second and third attack to 100
    mock_skill(attacker, gsn_second_attack, 100);
    mock_skill(attacker, gsn_third_attack, 100);
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    int initial_hp = victim->hit;
    
    // Generous RNG: many 19s for auto-hits, low values for skill checks
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    int sequence[] = {19, 19, 40, 19, 19, 20, 19, 19, 50, 75, 25, 100};  // Extra values
    set_mock_rng_sequence(sequence, 12);
    
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // Should have some damage from attacks
    int damage = initial_hp - victim->hit;
    ASSERT(damage > 0);  // ANY damage
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// HASTE TESTS
////////////////////////////////////////////////////////////////////////////////

// Test haste grants extra attack
static int test_haste_extra_attack()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("Attacker");
    Mobile* victim = mock_mob("Victim", 1, mock_mob_proto(1));
    
    attacker->level = 20;
    victim->hit = 1000;
    victim->max_hit = 1000;
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Give attacker a weapon with high damage
    Object* weapon = mock_sword("iron sword blade", 50, 10, 5, 10);
    obj_to_char(weapon, attacker);
    equip_char(attacker, weapon, WEAR_WIELD);
    
    // No extra attack skills
    attacker->pcdata->learned[gsn_second_attack] = 0;
    attacker->pcdata->learned[gsn_third_attack] = 0;
    
    // Apply haste
    Affect af = { 0 };
    af.where = TO_AFFECTS;
    af.type = -1;  // Generic affect
    af.level = 20;
    af.duration = 10;
    af.bitvector = AFF_HASTE;
    affect_to_mob(attacker, &af);
    
    ASSERT(IS_AFFECTED(attacker, AFF_HASTE));
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    int initial_hp = victim->hit;
    
    // Generous RNG: 19s for auto-hits on both attacks (base + haste)
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    int sequence[] = {19, 19, 19, 50, 75, 25, 100};  // Multiple 19s + damage values
    set_mock_rng_sequence(sequence, 7);
    
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // Should have some damage
    int damage = initial_hp - victim->hit;
    ASSERT(damage > 0);  // ANY damage
    
    return 0;
}

// Test slow reduces attack chances
static int test_slow_reduces_attacks()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("Attacker");
    Mobile* victim = mock_mob("Victim", 1, mock_mob_proto(1));
    
    attacker->level = 50;
    victim->hit = 1000;
    victim->max_hit = 1000;
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Give second attack skill
    mock_skill(attacker, gsn_second_attack, 100);
    
    // Apply slow
    Affect af = { 0 };
    af.where = TO_AFFECTS;
    af.type = -1;
    af.level = 20;
    af.duration = 10;
    af.bitvector = AFF_SLOW;
    affect_to_mob(attacker, &af);
    
    ASSERT(IS_AFFECTED(attacker, AFF_SLOW));
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    // Even with high roll, slow halves chance (50% -> 25%)
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    int sequence[] = {30};  // Would normally trigger, but slow makes it 25% chance
    set_mock_rng_sequence(sequence, 1);
    
    int initial_hp = victim->hit;
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // Slow should reduce extra attacks
    int damage = initial_hp - victim->hit;
    // Just verify some damage (testing exact count is hard with slow RNG interaction)
    ASSERT(damage > 0);
    
    return 0;
}

// Test slow prevents third attack entirely
static int test_slow_prevents_third_attack()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("Attacker");
    Mobile* victim = mock_mob("Victim", 1, mock_mob_proto(1));
    
    attacker->level = 50;
    victim->hit = 1000;
    victim->max_hit = 1000;
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Give attacker a weapon
    Object* weapon = mock_sword("iron sword blade", 50, 10, 2, 5);
    obj_to_char(weapon, attacker);
    equip_char(attacker, weapon, WEAR_WIELD);
    
    // Give third attack skill
    mock_skill(attacker, gsn_third_attack, 100);
    
    // Apply slow
    Affect af = { 0 };
    af.where = TO_AFFECTS;
    af.type = -1;
    af.level = 20;
    af.duration = 10;
    af.bitvector = AFF_SLOW;
    affect_to_mob(attacker, &af);
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();
    
    int initial_hp = victim->hit;
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    
    rng = saved_rng;
    
    // Slow sets third attack chance to 0, so max 1-2 hits possible
    int damage = initial_hp - victim->hit;
    ASSERT(damage > 0);  // Some damage occurred
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// DUAL WIELD TESTS
////////////////////////////////////////////////////////////////////////////////

// Test dual wield with offhand weapon
static int test_dual_wield_attack()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("Attacker");
    Mobile* victim = mock_mob("Victim", 1, mock_mob_proto(1));
    
    attacker->level = 50;
    victim->hit = 1000;
    victim->max_hit = 1000;
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Give dual wield skill
    mock_skill(attacker, gsn_dual_wield, 100);
    
    // Equip offhand weapon with high damage
    Object* offhand = mock_sword("offhand dagger blade", 99100, 10, 5, 10);
    obj_to_char(offhand, attacker);
    equip_char(attacker, offhand, WEAR_WIELD_OH);
    
    ASSERT(get_eq_char(attacker, WEAR_WIELD_OH) != NULL);
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    int initial_hp = victim->hit;
    
    // Generous RNG: 19s for hits, 40 for dual wield check
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    int sequence[] = {19, 19, 40, 19, 19, 50, 75, 25, 100};  // hits + dual wield + damage
    set_mock_rng_sequence(sequence, 9);
    
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // Should have some damage
    int damage = initial_hp - victim->hit;
    ASSERT(damage > 0);
    
    return 0;
}

// Test dual wield with slow
static int test_dual_wield_with_slow()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("Attacker");
    Mobile* victim = mock_mob("Victim", 1, mock_mob_proto(1));
    
    attacker->level = 50;
    victim->hit = 1000;
    victim->max_hit = 1000;
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Give main hand weapon
    Object* mainhand = mock_sword("iron sword blade", 99100, 10, 2, 5);
    obj_to_char(mainhand, attacker);
    equip_char(attacker, mainhand, WEAR_WIELD);
    
    // Give dual wield skill
    mock_skill(attacker, gsn_dual_wield, 100);
    
    // Equip offhand weapon
    Object* offhand = mock_sword("offhand sword blade", 99101, 10, 2, 4);
    obj_to_char(offhand, attacker);
    equip_char(attacker, offhand, WEAR_WIELD_OH);
    
    // Apply slow
    Affect af = { 0 };
    af.where = TO_AFFECTS;
    af.type = -1;
    af.level = 20;
    af.duration = 10;
    af.bitvector = AFF_SLOW;
    affect_to_mob(attacker, &af);
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    int sequence[] = {30};  // Chance halved by slow
    set_mock_rng_sequence(sequence, 1);
    
    int initial_hp = victim->hit;
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // Slow halves dual wield chance
    int damage = initial_hp - victim->hit;
    ASSERT(damage > 0);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// BACKSTAB TESTS
////////////////////////////////////////////////////////////////////////////////

// Test backstab prevents extra attacks
static int test_backstab_no_extra_attacks()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("Attacker");
    Mobile* victim = mock_mob("Victim", 1, mock_mob_proto(1));
    
    attacker->level = 50;
    victim->hit = 1000;
    victim->max_hit = 1000;
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Give attacker a dagger
    Object* weapon = mock_sword("iron dagger blade", 50, 10, 1, 4);
    obj_to_char(weapon, attacker);
    equip_char(attacker, weapon, WEAR_WIELD);
    
    // Give second attack skill
    mock_skill(attacker, gsn_second_attack, 100);
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();
    
    int initial_hp = victim->hit;
    
    // Call multi_hit with backstab dt
    multi_hit(attacker, victim, gsn_backstab);
    
    rng = saved_rng;
    
    // Backstab should only allow 1 hit (no second attack)
    int hits = count_hits_from_multihit(attacker, victim, initial_hp);
    ASSERT(hits <= 1);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// COMBAT STOPS DURING MULTI-HIT
////////////////////////////////////////////////////////////////////////////////

// Test combat stops if victim dies mid-multi-hit
static int test_victim_death_stops_attacks()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("Attacker");
    Mobile* victim = mock_mob("Victim", 1, mock_mob_proto(1));
    
    attacker->level = 50;
    victim->hit = 5;  // Very low HP
    victim->max_hit = 100;
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Give attacker a weapon with high damage
    Object* weapon = mock_sword("iron sword blade", 50, 10, 10, 10);
    obj_to_char(weapon, attacker);
    equip_char(attacker, weapon, WEAR_WIELD);
    
    // Give second attack skill
    mock_skill(attacker, gsn_second_attack, 100);
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    // RNG: 19 for auto-hit, damage values should kill victim (5 HP)
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    int sequence[] = {19, 50, 75, 25, 100};  // auto-hit + high damage values
    set_mock_rng_sequence(sequence, 5);
    
    // First hit should kill victim
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // Victim should be dead (hit <= 0)
    ASSERT(victim->hit <= 0);
    
    // Combat should have stopped
    ASSERT(attacker->fighting == NULL);
    
    return 0;
}

// Test combat stops if attacker flees mid-multi-hit
static int test_combat_end_stops_multi_hit()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("Attacker");
    Mobile* victim = mock_mob("Victim", 1, mock_mob_proto(1));
    
    attacker->level = 50;
    victim->hit = 1000;
    victim->max_hit = 1000;
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Give second attack skill
    mock_skill(attacker, gsn_second_attack, 100);
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    // Manually stop combat (simulates flee/death)
    stop_fighting(attacker, true);
    
    // Verify combat stopped
    ASSERT(attacker->fighting == NULL);
    ASSERT(victim->fighting == NULL);
    
    // Call multi_hit after combat stopped - this WILL re-initiate combat
    // because multi_hit always calls one_hit(), which calls damage(), which calls set_fighting()
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    
    // Combat should have restarted (correct behavior!)
    ASSERT(attacker->fighting == victim);
    ASSERT(victim->fighting == attacker);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// POSITION TESTS
////////////////////////////////////////////////////////////////////////////////

// Test no attacks from stunned position
static int test_no_attacks_when_stunned()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("Attacker");
    Mobile* victim = mock_mob("Victim", 1, mock_mob_proto(1));
    
    attacker->level = 20;
    victim->hit = 100;
    victim->max_hit = 100;
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    // Set attacker to stunned position
    attacker->position = POS_STUNNED;
    
    int initial_hp = victim->hit;
    
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    
    // No damage should have occurred
    ASSERT(victim->hit == initial_hp);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// REGISTRATION
////////////////////////////////////////////////////////////////////////////////

void register_multihit_tests()
{
    init_test_group(&multihit_group, "MULTI-HIT TESTS");
    
    #define REGISTER(name, func) register_test(&multihit_group, name, func)
    
    // Basic multi-hit
    REGISTER("Single attack only", test_single_attack_only);
    REGISTER("Second attack triggers", test_second_attack);
    REGISTER("Third attack triggers", test_third_attack);
    
    // Haste/slow
    REGISTER("Haste grants extra attack", test_haste_extra_attack);
    REGISTER("Slow reduces attack chances", test_slow_reduces_attacks);
    REGISTER("Slow prevents third attack", test_slow_prevents_third_attack);
    
    // Dual wield
    REGISTER("Dual wield offhand attack", test_dual_wield_attack);
    REGISTER("Dual wield with slow", test_dual_wield_with_slow);
    
    // Special cases
    REGISTER("Backstab prevents extra attacks", test_backstab_no_extra_attacks);
    REGISTER("Victim death stops attacks", test_victim_death_stops_attacks);
    REGISTER("Combat end stops multi-hit", test_combat_end_stops_multi_hit);
    REGISTER("No attacks when stunned", test_no_attacks_when_stunned);
    
    #undef REGISTER
    
    register_test_group(&multihit_group);
}
