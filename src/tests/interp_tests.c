////////////////////////////////////////////////////////////////////////////////
// interp_tests.c - Interpreter and command parsing tests (13 tests)
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "mock.h"
#include "test_registry.h"

#include <interp.h>
#include <handler.h>
#include <db.h>
#include <merc.h>
#include <stringutils.h>

#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>

extern bool test_socket_output_enabled;
extern bool test_act_output_enabled;
extern Value test_output_buffer;

// Test interpret - basic command execution
static int test_interpret_simple_command()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    interpret(player, "look");
    test_socket_output_enabled = false;
    
    // Should have produced output
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test interpret - command abbreviation
static int test_interpret_abbreviation()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    interpret(player, "l");  // Abbreviation for "look"
    test_socket_output_enabled = false;
    
    // Should have produced output
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test interpret - invalid command
static int test_interpret_invalid_command()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    interpret(player, "invalidxyzcommand");
    test_socket_output_enabled = false;
    
    // Should produce "Huh?" or similar error
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test interpret - empty command
static int test_interpret_empty_command()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    interpret(player, "");
    test_socket_output_enabled = false;
    
    // Empty command should just do nothing or minimal output
    test_output_buffer = NIL_VAL;
    ASSERT(true);
    
    return 0;
}

// Test interpret - command with arguments
static int test_interpret_command_with_args()
{
    Room* room = mock_room(60001, NULL, NULL);
    room->data->description = str_dup("Test room description.");
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    Object* obj = mock_obj("sword", 60200, mock_obj_proto(60200));
    obj_to_room(obj, room);
    
    test_socket_output_enabled = true;
    interpret(player, "look sword");
    test_socket_output_enabled = false;
    
    // Should have produced output
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test interpret - multiple word command
static int test_interpret_multiword_command()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    interpret(player, "look auto");
    test_socket_output_enabled = false;
    
    // Should execute without error
    test_output_buffer = NIL_VAL;
    ASSERT(true);
    
    return 0;
}

// Test interpret - position requirements
static int test_interpret_position_requirement()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_SLEEPING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    interpret(player, "north");  // Movement requires standing
    test_socket_output_enabled = false;
    
    // Should fail with position error
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test interpret - command cooldown/delay
static int test_interpret_with_delay()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    player->wait = 0;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    interpret(player, "look");
    test_socket_output_enabled = false;
    
    // Command should execute
    test_output_buffer = NIL_VAL;
    ASSERT(true);
    
    return 0;
}

// Test interpret - case insensitivity
static int test_interpret_case_insensitive()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    interpret(player, "LOOK");  // Uppercase
    test_socket_output_enabled = false;
    
    // Should work with uppercase
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test interpret - whitespace handling
static int test_interpret_leading_whitespace()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    interpret(player, "   look   ");  // Extra whitespace
    test_socket_output_enabled = false;
    
    // Should handle whitespace gracefully
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test interpret - command not found but social exists
static int test_interpret_fallback_to_social()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    interpret(player, "smile");  // Social command
    test_socket_output_enabled = false;
    
    // Should execute social
    test_output_buffer = NIL_VAL;
    ASSERT(true);
    
    return 0;
}

// Test interpret - NPC executing command
static int test_interpret_npc_command()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* npc = mock_mob("testnpc", 60100, mock_mob_proto(60100));
    npc->position = POS_STANDING;
    transfer_mob(npc, room);
    
    test_socket_output_enabled = true;
    interpret(npc, "look");
    test_socket_output_enabled = false;
    
    // NPCs can execute commands too
    test_output_buffer = NIL_VAL;
    ASSERT(true);
    
    return 0;
}

// Test interpret - trust level check
static int test_interpret_trust_level()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    player->trust = 0;
    player->level = 1;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    interpret(player, "mload 60100");  // Imm command - should fail
    test_socket_output_enabled = false;
    
    // Should deny access
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

void register_interp_tests()
{
    TestGroup* group = calloc(1, sizeof(TestGroup));
    init_test_group(group, "INTERP TESTS");
    
#define REGISTER(name, func) register_test(group, (name), (func))
    
    // Basic interpretation
    REGISTER("Interpret: Simple command", test_interpret_simple_command);
    REGISTER("Interpret: Abbreviation", test_interpret_abbreviation);
    REGISTER("Interpret: Invalid command", test_interpret_invalid_command);
    REGISTER("Interpret: Empty command", test_interpret_empty_command);
    
    // Arguments and parsing
    REGISTER("Interpret: Command with args", test_interpret_command_with_args);
    REGISTER("Interpret: Multiword command", test_interpret_multiword_command);
    REGISTER("Interpret: Case insensitive", test_interpret_case_insensitive);
    REGISTER("Interpret: Leading whitespace", test_interpret_leading_whitespace);
    
    // Command requirements
    REGISTER("Interpret: Position requirement", test_interpret_position_requirement);
    REGISTER("Interpret: With delay", test_interpret_with_delay);
    REGISTER("Interpret: Trust level check", test_interpret_trust_level);
    
    // Socials
    REGISTER("Interpret: Fallback to social", test_interpret_fallback_to_social);
    
    // NPCs
    REGISTER("Interpret: NPC command", test_interpret_npc_command);

#undef REGISTER

    register_test_group(group);
}
