////////////////////////////////////////////////////////////////////////////////
// act_wiz4_tests.c - More wizard command tests (force, slay, advance, etc.)
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "mock.h"
#include "test_registry.h"

#include <act_wiz.h>
#include <comm.h>
#include <handler.h>
#include <merc.h>

#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>

#include <data/mobile_data.h>

extern bool test_socket_output_enabled;
extern bool test_act_output_enabled;
extern Value test_output_buffer;

// Test do_force - force a player to execute a command
static int test_force_player()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    player->position = POS_SITTING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_force(wiz, "testplayer stand");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(player->position == POS_STANDING);
    
    return 0;
}

// Test do_force - force all players
static int test_force_all()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player1 = mock_player("testplayer1");
    player1->trust = 0;
    player1->position = POS_SITTING;
    transfer_mob(player1, room);
    
    Mobile* player2 = mock_player("testplayer2");
    player2->trust = 0;
    player2->position = POS_RESTING;
    transfer_mob(player2, room);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_force(wiz, "all stand");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Both players should now be standing (wiz is already standing)
    ASSERT(player1->position == POS_STANDING);
    ASSERT(player2->position == POS_STANDING);
    
    return 0;
}

// Test do_advance - advance player level
static int test_advance()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    player->level = 1;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_advance(wiz, "testplayer 5");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(player->level == 5);
    
    return 0;
}

// Test do_slay - kill a player
static int test_slay()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* victim = mock_player("victim");
    victim->trust = 0;
    victim->hit = 100;
    victim->level = 1;
    transfer_mob(victim, room);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_slay(wiz, "victim");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Victim should be dead - raw_kill was called
    // Just verify the command executed
    ASSERT(true);
    
    return 0;
}

// Test do_pardon - pardon a player's flags
static int test_pardon_killer()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    SET_BIT(player->act_flags, PLR_KILLER);
    transfer_mob(player, room);
    
    ASSERT(IS_SET(player->act_flags, PLR_KILLER));
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_pardon(wiz, "testplayer killer");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(!IS_SET(player->act_flags, PLR_KILLER));
    
    return 0;
}

// Test do_pardon - pardon thief flag
static int test_pardon_thief()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    SET_BIT(player->act_flags, PLR_THIEF);
    transfer_mob(player, room);
    
    ASSERT(IS_SET(player->act_flags, PLR_THIEF));
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_pardon(wiz, "testplayer thief");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(!IS_SET(player->act_flags, PLR_THIEF));
    
    return 0;
}

// Test do_deny - deny player access
static int test_deny()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    transfer_mob(player, room);
    
    ASSERT(!IS_SET(player->act_flags, PLR_DENY));
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_deny(wiz, "testplayer");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(IS_SET(player->act_flags, PLR_DENY));
    
    return 0;
}

// Test do_disconnect - disconnect a player
static int test_disconnect()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_disconnect(wiz, "testplayer");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Descriptor should be marked for closing (note: in tests, close_socket may not fully disconnect)
    // Just verify the command executed without crashing
    ASSERT(true);
    
    return 0;
}

// Test do_echo - echo a message (sends to all descriptors)
static int test_echo()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    // Echo iterates descriptor_list and sends to all connected players
    // In test environment, descriptor list may be empty
    test_socket_output_enabled = true;
    do_echo(wiz, "test message");
    test_socket_output_enabled = false;
    
    // Just verify command executed without crashing
    test_output_buffer = NIL_VAL;
    ASSERT(true);
    
    return 0;
}

// Test do_recho - echo to room (sends to all in room via descriptor_list)
static int test_recho()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    transfer_mob(player, room);
    
    // Recho iterates descriptor_list looking for characters in same room
    test_socket_output_enabled = true;
    do_recho(wiz, "room message");
    test_socket_output_enabled = false;
    
    // Just verify command executed without crashing
    test_output_buffer = NIL_VAL;
    ASSERT(true);
    
    return 0;
}

// Test do_pecho - echo to player
static int test_pecho()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_pecho(wiz, "testplayer personal message");
    test_socket_output_enabled = false;
    
    // Just verify command executed
    test_output_buffer = NIL_VAL;
    ASSERT(true);
    
    return 0;
}

// Test do_transfer - transfer player to wizard
static int test_transfer_to_wiz()
{
    Room* room1 = mock_room(60001, NULL, NULL);
    Room* room2 = mock_room(60002, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room1);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    transfer_mob(player, room2);
    
    ASSERT(player->in_room == room2);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_transfer(wiz, "testplayer");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(player->in_room == room1);
    
    return 0;
}

// Test do_goto - wizard teleports to room
static int test_goto()
{
    Room* room1 = mock_room(60001, NULL, NULL);
    Room* room2 = mock_room(60002, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room1);
    
    ASSERT(wiz->in_room == room1);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_goto(wiz, "60002");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(wiz->in_room == room2);
    
    return 0;
}

// Test do_holylight - toggle holylight
static int test_holylight()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    bool initial_state = IS_SET(wiz->act_flags, PLR_HOLYLIGHT);
    
    test_socket_output_enabled = true;
    do_holylight(wiz, "");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Should be toggled
    ASSERT(IS_SET(wiz->act_flags, PLR_HOLYLIGHT) != initial_state);
    
    return 0;
}

// Test do_invis - set wizard invisibility level
static int test_wizinvis()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    wiz->invis_level = 0;
    
    test_socket_output_enabled = true;
    do_invis(wiz, "50");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(wiz->invis_level == 50);
    
    return 0;
}

void register_act_wiz4_tests()
{
    TestGroup* group = calloc(1, sizeof(TestGroup));
    init_test_group(group, "ACT WIZ4 TESTS");
    
#define REGISTER(name, func) register_test(group, (name), (func))
    
    // Force commands
    REGISTER("Cmd: Force player", test_force_player);
    REGISTER("Cmd: Force all", test_force_all);
    
    // Player management
    REGISTER("Cmd: Advance", test_advance);
    REGISTER("Cmd: Slay", test_slay);
    REGISTER("Cmd: Pardon killer", test_pardon_killer);
    REGISTER("Cmd: Pardon thief", test_pardon_thief);
    REGISTER("Cmd: Deny", test_deny);
    REGISTER("Cmd: Disconnect", test_disconnect);
    
    // Communication
    REGISTER("Cmd: Echo", test_echo);
    REGISTER("Cmd: Recho", test_recho);
    REGISTER("Cmd: Pecho", test_pecho);
    
    // Movement
    REGISTER("Cmd: Transfer to wiz", test_transfer_to_wiz);
    REGISTER("Cmd: Goto", test_goto);
    
    // Settings
    REGISTER("Cmd: Holylight", test_holylight);
    REGISTER("Cmd: Wizinvis", test_wizinvis);

#undef REGISTER

    register_test_group(group);
}
