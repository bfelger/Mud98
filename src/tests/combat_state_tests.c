////////////////////////////////////////////////////////////////////////////////
// combat_state_tests.c
//
// Tests combat state transitions: initiation, fighting, fleeing, stopping,
// and death mechanics. Covers the lifecycle of combat from start to finish.
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
#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>

#include <data/mobile_data.h>
#include <data/skill.h>

#include <lox/value.h>

#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// COMBAT INITIATION TESTS
////////////////////////////////////////////////////////////////////////////////

// Test set_fighting initializes combat state correctly
static int test_set_fighting_basic()
{
    Room* room = mock_room(50000, NULL, NULL);
    MobPrototype* proto = mock_mob_proto(50001);
    proto->level = 10;
    Mobile* attacker = mock_mob("Attacker", 50001, proto);
    Mobile* victim = mock_mob("Victim", 50002, proto);
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Initially not fighting
    ASSERT(attacker->fighting == NULL);
    ASSERT(attacker->position != POS_FIGHTING);
    
    // Set fighting
    set_fighting(attacker, victim);
    
    // Now in combat state
    ASSERT(attacker->fighting == victim);
    ASSERT(attacker->position == POS_FIGHTING);
    
    return 0;
}

// Test set_fighting strips sleep affect
static int test_set_fighting_wakes_sleeper()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("Sleeper");
    Mobile* victim = mock_mob("Victim", 50001, NULL);
    ch->level = 50;
    
    transfer_mob(ch, room);
    transfer_mob(victim, room);
    
    // Apply sleep affect
    Affect af = { 0 };
    af.where = TO_AFFECTS;
    af.type = gsn_sleep;
    af.level = 10;
    af.duration = 5;
    af.bitvector = AFF_SLEEP;
    affect_to_mob(ch, &af);
    
    ASSERT(IS_AFFECTED(ch, AFF_SLEEP));
    
    // Set fighting should strip sleep
    set_fighting(ch, victim);
    
    ASSERT(!IS_AFFECTED(ch, AFF_SLEEP));
    ASSERT(ch->fighting == victim);
    
    return 0;
}

// Test set_fighting rejects already-fighting mob
static int test_set_fighting_already_fighting()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* attacker = mock_mob("Attacker", 50001, NULL);
    Mobile* victim1 = mock_mob("Victim1", 50002, NULL);
    Mobile* victim2 = mock_mob("Victim2", 50003, NULL);
    
    transfer_mob(attacker, room);
    transfer_mob(victim1, room);
    transfer_mob(victim2, room);
    
    // Set fighting first victim
    set_fighting(attacker, victim1);
    ASSERT(attacker->fighting == victim1);
    
    // Try to set fighting second victim - should fail silently
    set_fighting(attacker, victim2);
    
    // Still fighting first victim
    ASSERT(attacker->fighting == victim1);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// COMBAT STOP TESTS
////////////////////////////////////////////////////////////////////////////////

// Test stop_fighting ends combat for one side
static int test_stop_fighting_one_side()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* attacker = mock_mob("Attacker", 50001, NULL);
    Mobile* victim = mock_mob("Victim", 50002, NULL);
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    ASSERT(attacker->fighting == victim);
    ASSERT(victim->fighting == attacker);
    
    // Stop attacker only
    stop_fighting(attacker, false);
    
    // Attacker stops, victim still fighting
    ASSERT(attacker->fighting == NULL);
    ASSERT(attacker->position == POS_STANDING);
    ASSERT(victim->fighting == attacker);
    
    return 0;
}

// Test stop_fighting ends combat for both sides
static int test_stop_fighting_both_sides()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* attacker = mock_mob("Attacker", 50001, NULL);
    Mobile* victim = mock_mob("Victim", 50002, NULL);
    
    transfer_mob(attacker, room);
    transfer_mob(victim, room);
    
    // Start combat
    set_fighting(attacker, victim);
    set_fighting(victim, attacker);
    
    // Stop both
    stop_fighting(attacker, true);
    
    // Both stop fighting
    ASSERT(attacker->fighting == NULL);
    ASSERT(victim->fighting == NULL);
    ASSERT(attacker->position == POS_STANDING);
    ASSERT(victim->position == POS_STANDING);
    
    return 0;
}

// Test stop_fighting resets to default_pos for NPCs
static int test_stop_fighting_npc_default_pos()
{
    Room* room = mock_room(50000, NULL, NULL);
    MobPrototype* proto = mock_mob_proto(50001);
    proto->default_pos = POS_SITTING;
    Mobile* npc = mock_mob("Sitter", 50001, proto);
    Mobile* attacker = mock_player("Player");
    
    transfer_mob(npc, room);
    transfer_mob(attacker, room);
    
    // Start combat
    set_fighting(npc, attacker);
    set_fighting(attacker, npc);
    
    ASSERT(npc->position == POS_FIGHTING);
    
    // Stop combat
    stop_fighting(npc, true);
    
    // NPC returns to default position
    ASSERT(npc->position == POS_SITTING);
    ASSERT(attacker->position == POS_STANDING); // Player stands
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// FLEE TESTS
////////////////////////////////////////////////////////////////////////////////

// Test flee succeeds when exit available
static int test_flee_success()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Room* room2 = mock_room(50001, NULL, NULL);
    mock_room_connection(room1, room2, DIR_NORTH, false);
    
    Mobile* ch = mock_player("Fleeter");
    Mobile* victim = mock_mob("Attacker", 50002, NULL);
    ch->level = 50;
    
    transfer_mob(ch, room1);
    transfer_mob(victim, room1);
    
    // Start combat
    set_fighting(ch, victim);
    set_fighting(victim, ch);
    
    ASSERT(ch->fighting == victim);
    ASSERT(ch->in_room == room1);
    
    // Capture output
    test_socket_output_enabled = true;
    test_output_buffer = NIL_VAL;
    
    // Mock RNG to ensure flee succeeds (door selection + daze check)
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    int sequence[] = {DIR_NORTH, 0}; // Pick north, pass daze check
    set_mock_rng_sequence(sequence, 2);
    
    // Flee
    do_flee(ch, "");
    
    // Restore RNG
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // Should have fled to room2
    ASSERT(ch->in_room == room2);
    ASSERT(ch->fighting == NULL);
    ASSERT_OUTPUT_CONTAINS("You flee from combat!");
    
    test_socket_output_enabled = false;
    
    return 0;
}

// Test flee fails when no exits available
static int test_flee_no_exits()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("Trapped");
    Mobile* victim = mock_mob("Attacker", 50001, NULL);
    
    transfer_mob(ch, room);
    transfer_mob(victim, room);
    
    // Start combat
    set_fighting(ch, victim);
    
    ASSERT(ch->fighting == victim);
    
    // Capture output
    test_socket_output_enabled = true;
    test_output_buffer = NIL_VAL;
    
    // Flee (will try all directions and fail)
    do_flee(ch, "");
    
    // Verify still in same room and still fighting
    ASSERT(ch->in_room == room);
    ASSERT(ch->fighting == victim);
    ASSERT_OUTPUT_CONTAINS("PANIC! You couldn't escape!");
    
    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = false;
    
    return 0;
}

// Test flee fails when not fighting
static int test_flee_not_fighting()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("Peaceful");
    
    transfer_mob(ch, room);
    
    ASSERT(ch->fighting == NULL);
    
    // Capture output
    test_socket_output_enabled = true;
    test_output_buffer = NIL_VAL;
    
    // Try to flee when not fighting
    do_flee(ch, "");
    
    ASSERT_OUTPUT_CONTAINS("You aren't fighting anyone.");
    
    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = false;
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// DEATH MECHANICS TESTS
////////////////////////////////////////////////////////////////////////////////

// Test raw_kill for NPC creates corpse and extracts
static int test_raw_kill_npc()
{
    Room* room = mock_room(50000, NULL, NULL);
    MobPrototype* proto = mock_mob_proto(50001);
    proto->killed = 0;
    Mobile* victim = mock_mob("Victim", 50001, proto);
    Mobile* attacker = mock_player("Killer");
    
    transfer_mob(victim, room);
    transfer_mob(attacker, room);
    
    // Start combat
    set_fighting(victim, attacker);
    set_fighting(attacker, victim);
    
    ASSERT(victim->fighting == attacker);
    ASSERT(proto->killed == 0);
    
    // Count objects in room before death
    int obj_count_before = 0;
    Object* obj;
    FOR_EACH_ROOM_OBJ(obj, room) {
        obj_count_before++;
    }
    
    // Kill victim
    raw_kill(victim);
    
    // Combat stopped
    ASSERT(attacker->fighting == NULL);
    
    // Corpse should be in room
    int obj_count_after = 0;
    Object* corpse = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        obj_count_after++;
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
        }
    }
    
    ASSERT(obj_count_after == obj_count_before + 1);
    ASSERT(corpse != NULL);
    ASSERT(corpse->item_type == ITEM_CORPSE_NPC);
    
    // Kill counter incremented
    ASSERT(proto->killed == 1);
    
    return 0;
}

// Test raw_kill for PC creates corpse and resets stats
static int test_raw_kill_pc()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* victim = mock_player("Victim");
    Mobile* attacker = mock_mob("Killer", 50001, NULL);
    victim->level = 50;
    
    transfer_mob(victim, room);
    transfer_mob(attacker, room);
    
    // Set some stats
    victim->hit = 100;
    victim->mana = 100;
    victim->move = 100;
    victim->position = POS_FIGHTING;
    
    // Start combat
    set_fighting(victim, attacker);
    
    // Count objects in room before death
    int obj_count_before = 0;
    Object* obj;
    FOR_EACH_ROOM_OBJ(obj, room) {
        obj_count_before++;
    }
    
    // Kill victim
    raw_kill(victim);
    
    // Victim moves to death room (clan hall), not extracted
    ASSERT(victim->in_room != NULL);
    ASSERT(victim->in_room != room); // Moved away from combat room
    
    // Stats reset
    ASSERT(victim->hit >= 1);
    ASSERT(victim->mana >= 1);
    ASSERT(victim->move >= 1);
    ASSERT(victim->position == POS_RESTING);
    
    // Armor reset to 100
    ASSERT(victim->armor[AC_PIERCE] == 100);
    ASSERT(victim->armor[AC_BASH] == 100);
    ASSERT(victim->armor[AC_SLASH] == 100);
    ASSERT(victim->armor[AC_EXOTIC] == 100);
    
    // PC corpse should be in room
    int obj_count_after = 0;
    Object* corpse = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        obj_count_after++;
        if (obj->item_type == ITEM_CORPSE_PC) {
            corpse = obj;
        }
    }
    
    ASSERT(obj_count_after == obj_count_before + 1);
    ASSERT(corpse != NULL);
    ASSERT(corpse->item_type == ITEM_CORPSE_PC);
    
    return 0;
}

// Test corpse contains NPC money
static int test_corpse_contains_npc_money()
{
    Room* room = mock_room(50000, NULL, NULL);
    MobPrototype* proto = mock_mob_proto(50001);
    Mobile* victim = mock_mob("Rich Victim", 50001, proto);
    
    transfer_mob(victim, room);
    
    // Give victim some money
    victim->gold = 100;
    victim->silver = 50;
    victim->copper = 25;
    
    ASSERT(mobile_total_copper(victim) > 0);
    
    // Kill victim
    raw_kill(victim);
    
    // Find corpse
    Object* corpse = NULL;
    Object* obj;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    
    ASSERT(corpse != NULL);
    
    // Check corpse contents for money
    bool found_money = false;
    FOR_EACH_OBJ_CONTENT(obj, corpse) {
        if (obj->item_type == ITEM_MONEY) {
            found_money = true;
            // Money object should have value
            ASSERT(obj->value[0] > 0 || obj->value[1] > 0 || obj->value[2] > 0);
            break;
        }
    }
    
    ASSERT(found_money);
    
    return 0;
}

// Test corpse contains victim's equipment
static int test_corpse_contains_equipment()
{
    Room* room = mock_room(50000, NULL, NULL);
    MobPrototype* proto = mock_mob_proto(50001);
    Mobile* victim = mock_mob("Armed Victim", 50001, proto);
    
    transfer_mob(victim, room);
    
    // Give victim a sword
    Object* sword = mock_sword("iron sword blade", 50100, 10, 2, 5);
    obj_to_char(sword, victim);
    
    // Kill victim
    raw_kill(victim);
    
    // Find corpse
    Object* corpse = NULL;
    Object* obj;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    
    ASSERT(corpse != NULL);
    
    // Check corpse contains sword
    bool found_sword = false;
    FOR_EACH_OBJ_CONTENT(obj, corpse) {
        if (obj->item_type == ITEM_WEAPON) {
            found_sword = true;
            break;
        }
    }
    
    ASSERT(found_sword);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// MULTI-COMBAT TESTS
////////////////////////////////////////////////////////////////////////////////

// Test multiple attackers on one victim
static int test_multiple_attackers()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* victim = mock_mob("Victim", 50001, NULL);
    Mobile* attacker1 = mock_mob("Attacker1", 50002, NULL);
    Mobile* attacker2 = mock_mob("Attacker2", 50003, NULL);
    
    transfer_mob(victim, room);
    transfer_mob(attacker1, room);
    transfer_mob(attacker2, room);
    
    // Both attackers target same victim
    set_fighting(attacker1, victim);
    set_fighting(attacker2, victim);
    set_fighting(victim, attacker1); // Victim fights back at first attacker
    
    ASSERT(attacker1->fighting == victim);
    ASSERT(attacker2->fighting == victim);
    ASSERT(victim->fighting == attacker1);
    
    // Stop victim's combat (both sides)
    stop_fighting(victim, true);
    
    // All combat should stop
    ASSERT(attacker1->fighting == NULL);
    ASSERT(attacker2->fighting == NULL);
    ASSERT(victim->fighting == NULL);
    
    return 0;
}

// Test victim can only fight one attacker at a time
static int test_victim_fights_one_at_a_time()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* victim = mock_mob("Victim", 50001, NULL);
    Mobile* attacker1 = mock_mob("Attacker1", 50002, NULL);
    Mobile* attacker2 = mock_mob("Attacker2", 50003, NULL);
    
    transfer_mob(victim, room);
    transfer_mob(attacker1, room);
    transfer_mob(attacker2, room);
    
    // Set victim fighting first attacker
    set_fighting(victim, attacker1);
    ASSERT(victim->fighting == attacker1);
    
    // Try to set victim fighting second attacker
    set_fighting(victim, attacker2);
    
    // Victim still fighting first attacker
    ASSERT(victim->fighting == attacker1);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// REGISTRATION
////////////////////////////////////////////////////////////////////////////////

TestGroup combat_state_group;

void register_combat_state_tests()
{
    init_test_group(&combat_state_group, "COMBAT-STATE");
    
    // Combat initiation
    register_test(&combat_state_group, "set_fighting basic", test_set_fighting_basic);
    register_test(&combat_state_group, "set_fighting wakes sleeper", test_set_fighting_wakes_sleeper);
    register_test(&combat_state_group, "set_fighting already fighting", test_set_fighting_already_fighting);
    
    // Combat stop
    register_test(&combat_state_group, "stop_fighting one side", test_stop_fighting_one_side);
    register_test(&combat_state_group, "stop_fighting both sides", test_stop_fighting_both_sides);
    register_test(&combat_state_group, "stop_fighting NPC default_pos", test_stop_fighting_npc_default_pos);
    
    // Flee mechanics
    register_test(&combat_state_group, "flee success", test_flee_success);
    register_test(&combat_state_group, "flee no exits", test_flee_no_exits);
    register_test(&combat_state_group, "flee not fighting", test_flee_not_fighting);
    
    // Death mechanics
    register_test(&combat_state_group, "raw_kill NPC", test_raw_kill_npc);
    register_test(&combat_state_group, "raw_kill PC", test_raw_kill_pc);
    register_test(&combat_state_group, "corpse contains NPC money", test_corpse_contains_npc_money);
    register_test(&combat_state_group, "corpse contains equipment", test_corpse_contains_equipment);
    
    // Multi-combat
    register_test(&combat_state_group, "multiple attackers", test_multiple_attackers);
    register_test(&combat_state_group, "victim fights one at a time", test_victim_fights_one_at_a_time);
    
    register_test_group(&combat_state_group);
}
