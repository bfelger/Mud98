////////////////////////////////////////////////////////////////////////////////
// act_enter_tests.c - Tests for portal/enter commands (14 tests)
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"

#include "act_enter.h"
#include "handler.h"
#include "interp.h"
#include "mock.h"
#include "test_registry.h"

#include <data/item.h>
#include <entities/descriptor.h>
#include <entities/object.h>

// Test: do_enter with no argument
static int test_enter_no_argument()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    
    do_enter(ch, "");
    
    ASSERT_OUTPUT_EQ("Nope, can't do it.\n\r");
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: do_enter with object that doesn't exist
static int test_enter_object_not_found()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    
    do_enter(ch, "portal");
    
    ASSERT_OUTPUT_EQ("You don't see that here.\n\r");
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: do_enter with non-portal object
static int test_enter_not_a_portal()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room);
    
    // Create a sword (not a portal)
    Object* sword = mock_sword("sword blade", 50100, 1, 2, 4);
    obj_to_room(sword, room);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    
    do_enter(ch, "sword");
    
    ASSERT_OUTPUT_EQ("You can't seem to find a way in.\n\r");
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: do_enter with closed portal
static int test_enter_closed_portal()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Room* room2 = mock_room(50001, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room1);
    
    // Create a portal
    ObjPrototype* proto = mock_obj_proto(50200);
    proto->item_type = ITEM_PORTAL;
    
    Object* portal = mock_obj("portal gate", 50200, proto);
    portal->portal.charges = 10;
    portal->portal.exit_flags = EX_CLOSED;  // Closed!
    portal->portal.gate_flags = 0;
    portal->portal.destination = VNUM_FIELD(room2);
    obj_to_room(portal, room1);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    
    do_enter(ch, "portal");
    
    ASSERT_OUTPUT_EQ("You can't seem to find a way in.\n\r");
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: do_enter with cursed player and non-NOCURSE portal
static int test_enter_cursed_blocked()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Room* room2 = mock_room(50001, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room1);
    
    // Apply curse
    SET_BIT(ch->affect_flags, AFF_CURSE);
    
    // Create a portal without NOCURSE flag
    ObjPrototype* proto = mock_obj_proto(50200);
    proto->item_type = ITEM_PORTAL;
    
    Object* portal = mock_obj("portal gate", 50200, proto);
    portal->portal.charges = 10;
    portal->portal.exit_flags = 0;
    portal->portal.gate_flags = 0;  // No PORTAL_NOCURSE
    portal->portal.destination = VNUM_FIELD(room2);
    obj_to_room(portal, room1);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    
    do_enter(ch, "portal");
    
    ASSERT_OUTPUT_EQ("Something prevents you from leaving...\n\r");
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: do_enter successfully through portal
static int test_enter_successful()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Room* room2 = mock_room(50001, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room1);
    
    // Create a portal
    ObjPrototype* proto = mock_obj_proto(50200);
    proto->item_type = ITEM_PORTAL;
    
    Object* portal = mock_obj("portal gate", 50200, proto);
    portal->portal.charges = 10;
    portal->portal.exit_flags = 0;
    portal->portal.gate_flags = 0;
    portal->portal.destination = VNUM_FIELD(room2);
    obj_to_room(portal, room1);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    
    do_enter(ch, "portal");
    
    // Verify player moved to room2
    ASSERT(ch->in_room == room2);
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: do_enter with PORTAL_NORMAL_EXIT flag
static int test_enter_normal_exit()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Room* room2 = mock_room(50001, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room1);
    
    // Create a portal with NORMAL_EXIT flag
    ObjPrototype* proto = mock_obj_proto(50200);
    proto->item_type = ITEM_PORTAL;
    
    Object* portal = mock_obj("portal gate", 50200, proto);
    portal->portal.charges = 10;
    portal->portal.exit_flags = 0;
    portal->portal.gate_flags = PORTAL_NORMAL_EXIT;
    portal->portal.destination = VNUM_FIELD(room2);
    obj_to_room(portal, room1);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    
    do_enter(ch, "portal");
    
    // Verify player moved
    ASSERT(ch->in_room == room2);
    
    // With NORMAL_EXIT, output should include "You enter"
    ASSERT_OUTPUT_CONTAINS("You enter");
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: do_enter depletes portal charges
static int test_enter_depletes_charges()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Room* room2 = mock_room(50001, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room1);
    
    // Create a portal with 1 charge
    ObjPrototype* proto = mock_obj_proto(50200);
    proto->item_type = ITEM_PORTAL;
    
    Object* portal = mock_obj("portal gate", 50200, proto);
    portal->portal.charges = 1;  // Only 1 charge
    portal->portal.exit_flags = 0;
    portal->portal.gate_flags = 0;
    portal->portal.destination = VNUM_FIELD(room2);
    obj_to_room(portal, room1);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    
    do_enter(ch, "portal");
    
    // Verify player moved
    ASSERT(ch->in_room == room2);
    
    // Portal should be set to -1 (exhausted) and then extracted
    // Since portal is extracted, we can't check its value, but we can
    // verify the output mentions it fading
    ASSERT_OUTPUT_CONTAINS("fades out of existence");
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: do_enter with zero charges (infinite portal)
static int test_enter_zero_charges_infinite()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Room* room2 = mock_room(50001, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room1);
    
    // Create a portal with 0 charges (infinite)
    ObjPrototype* proto = mock_obj_proto(50200);
    proto->item_type = ITEM_PORTAL;
    
    Object* portal = mock_obj("portal gate", 50200, proto);
    portal->portal.charges = 0;  // 0 = infinite
    portal->portal.exit_flags = 0;
    portal->portal.gate_flags = 0;
    portal->portal.destination = VNUM_FIELD(room2);
    obj_to_room(portal, room1);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    
    do_enter(ch, "portal");
    
    // Verify player moved
    ASSERT(ch->in_room == room2);
    
    // Portal should still exist (not extracted)
    ASSERT(portal->in_room == room1);
    ASSERT(portal->portal.charges == 0);  // Still 0
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: do_enter with PORTAL_GOWITH flag
static int test_enter_portal_goes_with()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Room* room2 = mock_room(50001, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room1);
    
    // Create a portal with GOWITH flag
    ObjPrototype* proto = mock_obj_proto(50200);
    proto->item_type = ITEM_PORTAL;
    
    Object* portal = mock_obj("portal gate", 50200, proto);
    portal->portal.charges = 5;
    portal->portal.exit_flags = 0;
    portal->portal.gate_flags = PORTAL_GOWITH;
    portal->portal.destination = VNUM_FIELD(room2);
    obj_to_room(portal, room1);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    
    do_enter(ch, "portal");
    
    // Verify player moved
    ASSERT(ch->in_room == room2);
    
    // Portal should have moved to room2 as well
    ASSERT(portal->in_room == room2);
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: do_enter with invalid destination
static int test_enter_invalid_destination()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room1);
    
    // Create a portal with invalid destination
    ObjPrototype* proto = mock_obj_proto(50200);
    proto->item_type = ITEM_PORTAL;
    
    Object* portal = mock_obj("portal gate", 50200, proto);
    portal->portal.charges = 10;
    portal->portal.exit_flags = 0;
    portal->portal.gate_flags = 0;
    portal->portal.destination = 99999;  // Invalid vnum
    obj_to_room(portal, room1);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    
    do_enter(ch, "portal");
    
    // Should fail with "doesn't seem to go anywhere"
    ASSERT_OUTPUT_CONTAINS("doesn't seem to go anywhere");
    
    // Player should still be in room1
    ASSERT(ch->in_room == room1);
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: do_enter while fighting
static int test_enter_while_fighting()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Room* room2 = mock_room(50001, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room1);
    
    MobPrototype* mob_proto = mock_mob_proto(50300);
    Mobile* mob = mock_mob("Joe", 50300, mob_proto);
    transfer_mob(mob, room1);
    
    // Set fighting state
    ch->fighting = mob;
    
    // Create a portal
    ObjPrototype* proto = mock_obj_proto(50200);
    proto->item_type = ITEM_PORTAL;
    
    Object* portal = mock_obj("portal gate", 50200, proto);
    portal->portal.charges = 10;
    portal->portal.exit_flags = 0;
    portal->portal.gate_flags = 0;
    portal->portal.destination = VNUM_FIELD(room2);
    obj_to_room(portal, room1);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    
    do_enter(ch, "portal");
    
    // Should not move (no output, no movement)
    ASSERT(ch->in_room == room1);
    // When fighting, do_enter returns early without output
    ASSERT(IS_NIL(test_output_buffer) || 
           (IS_STRING(test_output_buffer) && AS_CSTRING(test_output_buffer)[0] == '\0'));
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: Followers follow through portal
static int test_enter_followers_follow()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Room* room2 = mock_room(50001, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room1);
    
    // Create a charmed follower
    MobPrototype* mob_proto = mock_mob_proto(50300);
    Mobile* pet = mock_mob("pet", 50300, mob_proto);
    transfer_mob(pet, room1);
    pet->master = ch;
    pet->position = POS_STANDING;
    SET_BIT(pet->affect_flags, AFF_CHARM);
    
    // Create a portal
    ObjPrototype* proto = mock_obj_proto(50200);
    proto->item_type = ITEM_PORTAL;
    
    Object* portal = mock_obj("portal gate", 50200, proto);
    portal->portal.charges = 10;
    portal->portal.exit_flags = 0;
    portal->portal.gate_flags = 0;
    portal->portal.destination = VNUM_FIELD(room2);
    obj_to_room(portal, room1);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    
    do_enter(ch, "portal");
    
    // Both should be in room2
    ASSERT(ch->in_room == room2);
    ASSERT(pet->in_room == room2);
    
    test_output_buffer = NIL_VAL;
    return 0;
}

// Test: NOCURSE flag allows cursed player to enter
static int test_enter_nocurse_allows_cursed()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Room* room2 = mock_room(50001, NULL, NULL);
    Mobile* ch = mock_player("Bob");
    transfer_mob(ch, room1);
    
    // Apply curse
    SET_BIT(ch->affect_flags, AFF_CURSE);
    
    // Create a portal WITH NOCURSE flag
    ObjPrototype* proto = mock_obj_proto(50200);
    proto->item_type = ITEM_PORTAL;
    
    Object* portal = mock_obj("portal gate", 50200, proto);
    portal->portal.charges = 10;
    portal->portal.exit_flags = 0;
    portal->portal.gate_flags = PORTAL_NOCURSE;  // Allows cursed
    portal->portal.destination = VNUM_FIELD(room2);
    obj_to_room(portal, room1);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    
    do_enter(ch, "portal");
    
    // Cursed player should still be able to enter
    ASSERT(ch->in_room == room2);
    
    test_output_buffer = NIL_VAL;
    return 0;
}

void register_act_enter_tests()
{
    TestGroup* group = calloc(1, sizeof(TestGroup));
    init_test_group(group, "ACT_ENTER TESTS");
    
#define REGISTER(name, func) register_test(group, (name), (func))
    
    REGISTER("Enter: No argument", test_enter_no_argument);
    REGISTER("Enter: Object not found", test_enter_object_not_found);
    REGISTER("Enter: Not a portal", test_enter_not_a_portal);
    REGISTER("Enter: Closed portal", test_enter_closed_portal);
    REGISTER("Enter: Cursed player blocked", test_enter_cursed_blocked);
    REGISTER("Enter: Successful", test_enter_successful);
    REGISTER("Enter: Normal exit flag", test_enter_normal_exit);
    REGISTER("Enter: Depletes charges", test_enter_depletes_charges);
    REGISTER("Enter: Zero charges infinite", test_enter_zero_charges_infinite);
    REGISTER("Enter: Portal goes with", test_enter_portal_goes_with);
    REGISTER("Enter: Invalid destination", test_enter_invalid_destination);
    REGISTER("Enter: While fighting", test_enter_while_fighting);
    REGISTER("Enter: Followers follow", test_enter_followers_follow);
    REGISTER("Enter: NOCURSE allows cursed", test_enter_nocurse_allows_cursed);
    
#undef REGISTER
    
    register_test_group(group);
}
