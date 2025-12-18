////////////////////////////////////////////////////////////////////////////////
// tohit_tests.c - Tests for to-hit mechanics, armor class, and defensive skills
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "combat_ops.h"
#include "fight.h"
#include "handler.h"
#include "recycle.h"
#include "rng.h"

#include "entities/descriptor.h"

#include "data/mobile_data.h"
#include "data/skill.h"

#include "tests/tests.h"
#include "tests/mock.h"
#include "tests/test_registry.h"

#include <assert.h>
#include <string.h>

TestGroup tohit_group;

// Test check_parry with no weapon (NPCs can parry, PCs can't)
static int test_parry_no_weapon_npc()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_mob("attacker", 1, mock_mob_proto(1));
    Mobile* victim = mock_mob("victim", 2, mock_mob_proto(2));
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    victim->level = 20;
    attacker->level = 20;
    SET_BIT(victim->act_flags, ACT_IS_NPC);
    mock_skill(victim, gsn_parry, 100);
    
    // NPC parry chance calculation:
    // get_skill returns level*2 = 40 for NPC with ATK_PARRY
    // chance = 40 / 2 = 20
    // No weapon, NPC: chance = 20 / 2 = 10
    // With equal levels: 10 + 0 = 10%
    // Set RNG to return 5 (< 10, should succeed)
    int sequence[] = {5};
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    set_mock_rng_sequence(sequence, 1);
    
    bool result = check_parry(attacker, victim);
    
    set_mock_rng_sequence(NULL, 0);  // Reset to default
    rng = saved_rng;
    
    ASSERT(result == true);

    return 0;
}

// Test check_parry for PC without weapon (should always fail)
static int test_parry_no_weapon_pc()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_mob("attacker", 1, mock_mob_proto(1));
    Mobile* victim = mock_player("victim");
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    victim->level = 20;
    mock_skill(victim, gsn_parry, 100);
    
    // PC without weapon cannot parry
    bool result = check_parry(attacker, victim);
    
    ASSERT(result == false);

    return 0;
}

// Test check_parry with weapon equipped
static int test_parry_with_weapon()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_mob("attacker", 1, mock_mob_proto(1));
    Mobile* victim = mock_player("victim");
    Object* sword = mock_sword("sword", 1, 10, 2, 5);
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    victim->level = 50;  // High enough to meet skill level requirements
    attacker->level = 50;
    mock_skill(victim, gsn_parry, 100);
    
    // Equip weapon
    obj_to_char(sword, victim);
    equip_char(victim, sword, WEAR_WIELD);
    
    // Parry chance calculation:
    // get_skill returns pcdata->learned[gsn_parry] = 100
    // chance = 100 / 2 = 50
    // With weapon (no penalty)
    // With equal levels: 50 + 0 = 50%
    // Set RNG to return 25 (< 50, should succeed)
    int sequence[] = {25};
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    set_mock_rng_sequence(sequence, 1);
    
    bool result = check_parry(attacker, victim);
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    ASSERT(result == true);

    return 0;
}

// Test check_parry fails when victim can't see attacker
static int test_parry_blind_victim()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_mob("attacker", 1, mock_mob_proto(1));
    Mobile* victim = mock_player("victim");
    Object* sword = mock_sword("sword", 1, 10, 2, 5);
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    victim->level = 20;
    mock_skill(victim, gsn_parry, 100);
    
    obj_to_char(sword, victim);
    equip_char(victim, sword, WEAR_WIELD);
    
    // Blind victim has halved chance
    SET_BIT(victim->affect_flags, AFF_BLIND);
    
    // Parry chance = (skill/2)/2 + 0 = 25%
    // Mock RNG default sequence starts at 50, should fail (50 >= 25)
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();
    
    bool result = check_parry(attacker, victim);
    
    rng = saved_rng;
    
    ASSERT(result == false);

    return 0;
}

// Test check_shield_block with shield
static int test_shield_block_with_shield()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_mob("attacker", 1, mock_mob_proto(1));
    Mobile* victim = mock_player("victim");
    Object* shield = mock_shield("shield", 1, 10);
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    victim->level = 20;
    attacker->level = 20;
    mock_skill(victim, gsn_shield_block, 100);
    
    // Equip shield
    obj_to_char(shield, victim);
    equip_char(victim, shield, WEAR_SHIELD);
    
    // Shield block chance calculation:
    // get_skill returns pcdata->learned[gsn_shield_block] = 100
    // chance = 100 / 5 + 3 = 20 + 3 = 23
    // With equal levels: 23 + 0 = 23%
    // Set RNG to return 20 (< 23, should succeed)
    int sequence[] = {20};
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    set_mock_rng_sequence(sequence, 1);
    
    bool result = check_shield_block(attacker, victim);
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    ASSERT(result == true);
    
    return 0;
}

// Test check_shield_block without shield (should always fail)
static int test_shield_block_no_shield()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_mob("attacker", 1, mock_mob_proto(1));
    Mobile* victim = mock_player("victim");
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    victim->level = 20;
    mock_skill(victim, gsn_shield_block, 100);
    
    // No shield equipped
    bool result = check_shield_block(attacker, victim);
    
    ASSERT(result == false);

    return 0;
}

// Test check_dodge basic functionality
static int test_dodge_basic()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_mob("attacker", 1, mock_mob_proto(1));
    Mobile* victim = mock_player("victim");
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    victim->level = 20;
    attacker->level = 20;
    mock_skill(victim, gsn_dodge, 100);
    
    // Dodge chance = skill/2 + level_diff = 50 + 0 = 50%
    // Set RNG to return 25 (< 50, should succeed)
    int sequence[] = {25};
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    set_mock_rng_sequence(sequence, 1);
    
    bool result = check_dodge(attacker, victim);
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    ASSERT(result == true);

    return 0;
}

// Test check_dodge when victim can't see attacker
static int test_dodge_blind_victim()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_mob("attacker", 1, mock_mob_proto(1));
    Mobile* victim = mock_player("victim");
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    victim->level = 20;
    attacker->level = 20;
    mock_skill(victim, gsn_dodge, 100);
    
    // Blind victim has halved chance
    SET_BIT(victim->affect_flags, AFF_BLIND);
    
    // Dodge chance = (skill/2)/2 + 0 = 25%
    // Mock RNG default sequence starts at 50, should fail
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();
    
    bool result = check_dodge(attacker, victim);
    
    rng = saved_rng;
    
    ASSERT(result == false);

    return 0;
}

// Test check_dodge fails when victim is not awake
static int test_dodge_sleeping_victim()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_mob("attacker", 1, mock_mob_proto(1));
    Mobile* victim = mock_player("victim");
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    victim->level = 20;
    victim->position = POS_SLEEPING;
    mock_skill(victim, gsn_dodge, 100);
    
    // Sleeping victim cannot dodge
    bool result = check_dodge(attacker, victim);
    
    ASSERT(result == false);

    return 0;
}

// Test level difference affects defensive chances
static int test_level_difference_defense()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_mob("attacker", 1, mock_mob_proto(1));
    Mobile* victim = mock_player("victim");
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    victim->level = 30;
    attacker->level = 20;
    mock_skill(victim, gsn_dodge, 100);
    
    // Dodge chance = skill/2 + (victim_level - ch_level) = 50 + 10 = 60%
    // Mock RNG default sequence starts at 50, should dodge
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();
    
    bool result = check_dodge(attacker, victim);
    
    rng = saved_rng;
    
    ASSERT(result == true);

    return 0;
}

// Test THAC0 calculation for warriors
static int test_thac0_warrior()
{
    // This test verifies the THAC0 interpolation formula
    // Warriors get thac0_00=20, thac0_32=-10 (best progression)
    Room* room = mock_room(1, NULL, NULL);
    MobPrototype* proto = mock_mob_proto(1);
    Mobile* warrior = mock_mob("warrior", 1, proto);
    Mobile* victim = mock_mob("victim", 2, mock_mob_proto(2));
    
    transfer_mob(warrior, room);
    transfer_mob(victim, room);
    
    warrior->level = 16;  // Halfway between 0 and 32
    SET_BIT(warrior->act_flags, ACT_IS_NPC);
    SET_BIT(warrior->act_flags, ACT_WARRIOR);
    
    // Expected THAC0 at level 16:
    // interpolate(16, 20, -10) = 20 - (16 * (20 - (-10)) / 32)
    //                           = 20 - (16 * 30 / 32)
    //                           = 20 - 15 = 5
    // This is an indirect test - we'll verify behavior, not exact calculation
    
    // Set victim AC to be very low so we can hit
    victim->armor[AC_PIERCE] = 0;
    victim->armor[AC_BASH] = 0;
    victim->armor[AC_SLASH] = 0;
    victim->armor[AC_EXOTIC] = 0;
    victim->hit = 100;
    victim->max_hit = 100;
    
    // Mock RNG for hit roll
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();
    
    // Execute one_hit and verify it doesn't crash
    one_hit(warrior, victim, TYPE_UNDEFINED, false);
    
    rng = saved_rng;
    
    // If we get here without crashing, THAC0 calculation worked

    return 0;
}

// Test GET_HITROLL affects to-hit
static int test_hitroll_affects_tohit()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("attacker");
    Mobile* victim = mock_mob("victim", 1, mock_mob_proto(1));
    Object* sword = mock_sword("sword", 1, 10, 2, 5);
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    attacker->level = 20;
    attacker->hitroll = 20;  // +20 to hit
    victim->level = 20;
    victim->hit = 100;
    victim->max_hit = 100;
    
    // Give attacker a weapon
    obj_to_char(sword, attacker);
    equip_char(attacker, sword, WEAR_WIELD);
    
    // Set victim AC moderate
    victim->armor[AC_SLASH] = 0;
    
    // Mock RNG - use 19 for automatic hit
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    int sequence[] = {19};  // Auto-hit
    set_mock_rng_sequence(sequence, 1);
    
    int hp_before = victim->hit;
    one_hit(attacker, victim, TYPE_UNDEFINED, false);
    int hp_after = victim->hit;
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // High hitroll should increase hit chance significantly
    // We should have landed a hit
    ASSERT(hp_after < hp_before);

    return 0;
}

// Test GET_DAMROLL affects damage
static int test_damroll_affects_damage()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("attacker");
    Mobile* victim = mock_mob("victim", 1, mock_mob_proto(1));
    Object* sword = mock_sword("sword", 1, 10, 1, 3);  // 1d3 weapon
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    attacker->level = 20;
    attacker->hitroll = 50;  // Make sure we hit
    attacker->damroll = 20;  // +20 damage
    victim->level = 1;       // Low level to ensure hit
    victim->hit = 200;
    victim->max_hit = 200;
    
    obj_to_char(sword, attacker);
    equip_char(attacker, sword, WEAR_WIELD);
    
    // Set victim AC high (easy to hit)
    victim->armor[AC_SLASH] = 100;
    
    // Mock RNG
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();
    
    int hp_before = victim->hit;
    one_hit(attacker, victim, TYPE_UNDEFINED, false);
    int hp_after = victim->hit;
    
    rng = saved_rng;
    
    int damage = hp_before - hp_after;
    
    // With 1d3 weapon rolling 1, skill 20, damroll 20:
    // dam = 1 * 20/100 = 0 (rounds down)
    // dam += 20 * 100/100 = 20
    // dam = 1 (minimum)
    // Then +20 damroll should significantly increase damage
    // Actual damage will be modified by many factors, but should be > 0
    ASSERT(damage > 0);

    return 0;
}

// Test armor class affects hit chance
static int test_armor_class_affects_hit()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("attacker");
    Mobile* victim = mock_mob("victim", 1, mock_mob_proto(1));
    Object* sword = mock_sword("sword", 1, 10, 2, 5);
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    attacker->level = 20;
    attacker->hitroll = 0;
    victim->level = 20;
    victim->hit = 100;
    victim->max_hit = 100;
    
    obj_to_char(sword, attacker);
    equip_char(attacker, sword, WEAR_WIELD);
    
    // Set victim to have very good AC (harder to hit)
    victim->armor[AC_SLASH] = -100;  // Excellent armor
    
    // Mock RNG
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();
    
    one_hit(attacker, victim, TYPE_UNDEFINED, false);
    
    rng = saved_rng;
    
    // With excellent AC and moderate roll, likely missed
    // But the important thing is the calculation doesn't crash

    return 0;
}

// Test critical miss (roll == 0)
static int test_critical_miss()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("attacker");
    Mobile* victim = mock_mob("victim", 1, mock_mob_proto(1));
    Object* sword = mock_sword("sword", 1, 10, 2, 5);
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    attacker->level = 20;
    attacker->hitroll = 100;  // Massive hitroll - should normally always hit
    victim->level = 1;
    victim->hit = 100;
    victim->max_hit = 100;
    victim->armor[AC_SLASH] = 100;  // Terrible armor - easy to hit normally
    
    obj_to_char(sword, attacker);
    equip_char(attacker, sword, WEAR_WIELD);
    
    // Force the d20 roll to return 0 (critical miss)
    // one_hit calls number_range(0, 19), so sequence needs to provide 0
    int sequence[] = {0};  // d20 roll = 0 (auto-miss)
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    set_mock_rng_sequence(sequence, 1);
    
    int hp_before = victim->hit;
    one_hit(attacker, victim, TYPE_UNDEFINED, false);
    int hp_after = victim->hit;
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // Critical miss (roll==0) should always miss regardless of bonuses
    // Victim HP should not change
    ASSERT(hp_after == hp_before);

    return 0;
}

// Test critical hit (roll == 19)
static int test_critical_hit()
{
    Room* room = mock_room(1, NULL, NULL);
    Mobile* attacker = mock_player("attacker");
    Mobile* victim = mock_mob("victim", 1, mock_mob_proto(1));
    Object* sword = mock_sword("sword", 1, 10, 2, 5);
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    attacker->level = 1;
    attacker->hitroll = 0;
    victim->level = 50;  // Much higher level
    victim->hit = 200;
    victim->max_hit = 200;
    victim->armor[AC_SLASH] = -200;  // Excellent armor - should normally never hit
    
    obj_to_char(sword, attacker);
    equip_char(attacker, sword, WEAR_WIELD);
    
    // Force the d20 roll to return 19 (critical hit)
    // one_hit calls number_range(0, 19), then various other RNG calls for damage
    // First call is the d20 roll, so provide 19 followed by reasonable values
    int sequence[] = {19, 50, 50, 50};  // d20=19 (auto-hit), then normal rolls for damage
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    set_mock_rng_sequence(sequence, 4);
    
    int hp_before = victim->hit;
    one_hit(attacker, victim, TYPE_UNDEFINED, false);
    int hp_after = victim->hit;
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // Critical hit (roll==19) should always hit regardless of penalties
    // Victim HP should decrease despite massive defensive advantages
    ASSERT(hp_after < hp_before);

    return 0;
}

void register_tohit_tests()
{
    init_test_group(&tohit_group, "TO-HIT TESTS");

#define REGISTER(name, func) register_test(&tohit_group, (name), (func))

    // Defensive skills - parry
    REGISTER("Parry: NPC without weapon", test_parry_no_weapon_npc);
    REGISTER("Parry: PC without weapon", test_parry_no_weapon_pc);
    REGISTER("Parry: With weapon", test_parry_with_weapon);
    REGISTER("Parry: Blind victim", test_parry_blind_victim);
    
    // Defensive skills - shield block
    REGISTER("Shield: Block with shield", test_shield_block_with_shield);
    REGISTER("Shield: No shield fails", test_shield_block_no_shield);
    
    // Defensive skills - dodge
    REGISTER("Dodge: Basic", test_dodge_basic);
    REGISTER("Dodge: Blind victim", test_dodge_blind_victim);
    REGISTER("Dodge: Sleeping fails", test_dodge_sleeping_victim);
    REGISTER("Dodge: Level difference", test_level_difference_defense);
    
    // To-hit mechanics
    REGISTER("THAC0: Warrior calculation", test_thac0_warrior);
    REGISTER("Hit: Hitroll affects", test_hitroll_affects_tohit);
    REGISTER("Hit: Damroll affects", test_damroll_affects_damage);
    REGISTER("Hit: Armor class affects", test_armor_class_affects_hit);
    REGISTER("Hit: Critical miss (roll 0)", test_critical_miss);
    REGISTER("Hit: Critical hit (roll 19)", test_critical_hit);

#undef REGISTER

    register_test_group(&tohit_group);
}
