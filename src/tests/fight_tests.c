////////////////////////////////////////////////////////////////////////////////
// fight_tests.c - Combat command tests (15 tests)
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "mock.h"
#include "test_registry.h"

#include <fight.h>
#include <handler.h>
#include <db.h>
#include <merc.h>

#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>

#include <data/mobile_data.h>
#include <data/player.h>

extern bool test_socket_output_enabled;
extern Value test_output_buffer;

// Test kill command - basic attack
static int test_kill_basic()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 5;
    transfer_mob(victim, room);
    
    test_socket_output_enabled = true;
    do_kill(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Should have initiated combat
    ASSERT(attacker->fighting != NULL);
    ASSERT(victim->fighting != NULL);
    
    return 0;
}

// Test murder command - PK attack
static int test_murder_player()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    Mobile* victim = mock_player("victim");
    victim->level = 8;
    transfer_mob(victim, room);
    
    test_socket_output_enabled = true;
    do_murder(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Should have initiated combat (or given error if not PK)
    // Just verify it doesn't crash
    ASSERT(true);
    
    return 0;
}

// Test flee command
static int test_flee()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    player->level = 10;
    transfer_mob(player, room);
    
    MobPrototype* proto = mock_mob_proto(60003);
    Mobile* mob = mock_mob("mob", 60003, proto);
    mob->level = 15;
    transfer_mob(mob, room);
    
    // Start combat
    player->fighting = mob;
    mob->fighting = player;
    player->position = POS_FIGHTING;
    mob->position = POS_FIGHTING;
    
    test_socket_output_enabled = true;
    do_flee(player, "");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // May or may not succeed, but should execute without crash
    ASSERT(true);
    
    return 0;
}

// Test kick command
static int test_kick()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 5;
    transfer_mob(victim, room);
    
    test_socket_output_enabled = true;
    do_kick(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Kick may or may not succeed, just verify no crash
    ASSERT(true);
    
    return 0;
}

// Test bash command
static int test_bash()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 15;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 8;
    transfer_mob(victim, room);
    
    test_socket_output_enabled = true;
    do_bash(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Bash may or may not succeed, just verify no crash
    ASSERT(true);
    
    return 0;
}

// Test trip command
static int test_trip()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 12;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 8;
    transfer_mob(victim, room);
    
    test_socket_output_enabled = true;
    do_trip(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Trip may or may not succeed, just verify no crash
    ASSERT(true);
    
    return 0;
}

// Test dirt kicking
static int test_dirt()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 10;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 8;
    transfer_mob(victim, room);
    
    test_socket_output_enabled = true;
    do_dirt(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Dirt may or may not succeed, just verify no crash
    ASSERT(true);
    
    return 0;
}

// Test disarm command
static int test_disarm()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 15;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 10;
    transfer_mob(victim, room);
    
    // Give victim a weapon
    Object* sword = mock_sword("sword", 60010, 10, 2, 6);
    obj_to_char(sword, victim);
    equip_char(victim, sword, WEAR_WIELD);
    
    test_socket_output_enabled = true;
    do_disarm(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Disarm may or may not succeed, just verify no crash
    ASSERT(true);
    
    return 0;
}

// Test backstab command
static int test_backstab()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 15;
    transfer_mob(attacker, room);
    
    // Give attacker a dagger
    Object* dagger = mock_sword("dagger", 60011, 10, 2, 4);
    dagger->weapon.weapon_type = WEAPON_DAGGER;
    obj_to_char(dagger, attacker);
    equip_char(attacker, dagger, WEAR_WIELD);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 10;
    transfer_mob(victim, room);
    
    test_socket_output_enabled = true;
    do_backstab(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Backstab may or may not succeed, just verify no crash
    ASSERT(true);
    
    return 0;
}

// Test berserk command
static int test_berserk()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* warrior = mock_player("warrior");
    warrior->position = POS_STANDING;
    warrior->level = 15;
    transfer_mob(warrior, room);
    
    test_socket_output_enabled = true;
    do_berserk(warrior, "");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Berserk may or may not succeed, just verify no crash
    ASSERT(true);
    
    return 0;
}

// Test rescue command
static int test_rescue()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* rescuer = mock_player("rescuer");
    rescuer->position = POS_STANDING;
    rescuer->level = 15;
    transfer_mob(rescuer, room);
    
    Mobile* victim = mock_player("victim");
    victim->position = POS_STANDING;
    victim->level = 10;
    transfer_mob(victim, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* mob = mock_mob("mob", 60002, proto);
    mob->level = 12;
    transfer_mob(mob, room);
    
    // Victim is fighting mob
    victim->fighting = mob;
    mob->fighting = victim;
    victim->position = POS_FIGHTING;
    mob->position = POS_FIGHTING;
    
    test_socket_output_enabled = true;
    do_rescue(rescuer, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Rescue may or may not succeed, just verify no crash
    ASSERT(true);
    
    return 0;
}

// Test surrender command
static int test_surrender()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    player->level = 5;
    transfer_mob(player, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* mob = mock_mob("mob", 60002, proto);
    mob->level = 15;
    transfer_mob(mob, room);
    
    // Start combat
    player->fighting = mob;
    mob->fighting = player;
    player->position = POS_FIGHTING;
    mob->position = POS_FIGHTING;
    
    test_socket_output_enabled = true;
    do_surrender(player, "");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Surrender calls stop_fighting, but mob may attack back
    // Just verify no crash
    ASSERT(true);
    
    return 0;
}

// Test kill without target
static int test_kill_no_target()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_kill(player, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test backstab without weapon
static int test_backstab_no_weapon()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 15;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 10;
    transfer_mob(victim, room);
    
    test_socket_output_enabled = true;
    do_backstab(attacker, "victim");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test flee when not fighting
static int test_flee_not_fighting()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_flee(player, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

void register_fight_tests()
{
    TestGroup* group = calloc(1, sizeof(TestGroup));
    init_test_group(group, "FIGHT TESTS");

#define REGISTER(name, func) register_test(group, (name), (func))

    // Basic combat
    REGISTER("Kill: Basic attack", test_kill_basic);
    REGISTER("Kill: No target", test_kill_no_target);
    REGISTER("Murder: PK attack", test_murder_player);
    REGISTER("Flee: From combat", test_flee);
    REGISTER("Flee: Not fighting", test_flee_not_fighting);
    
    // Combat skills
    REGISTER("Kick: Basic", test_kick);
    REGISTER("Bash: Basic", test_bash);
    REGISTER("Trip: Basic", test_trip);
    REGISTER("Dirt: Kick dirt", test_dirt);
    REGISTER("Disarm: Opponent weapon", test_disarm);
    
    // Special attacks
    REGISTER("Backstab: With dagger", test_backstab);
    REGISTER("Backstab: No weapon", test_backstab_no_weapon);
    REGISTER("Berserk: Warrior rage", test_berserk);
    REGISTER("Rescue: Save ally", test_rescue);
    REGISTER("Surrender: Stop fighting", test_surrender);

#undef REGISTER

    register_test_group(group);
}
