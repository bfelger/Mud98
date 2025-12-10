////////////////////////////////////////////////////////////////////////////////
// act_wiz_tests.c
//
// Tests for act_wiz.c wizard/immortal commands.
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "lox_tests.h"
#include "mock.h"

#include <act_wiz.h>
#include <comm.h>
#include <handler.h>
#include <interp.h>

#include <data/mobile_data.h>
#include <entities/room.h>
#include <entities/descriptor.h>

TestGroup act_wiz_tests;

// Test do_echo - global echo command
static int test_echo_with_message()
{
    Room* room = mock_room(60000, NULL, NULL);
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    // Add the descriptor to the global list for the echo to work
    wiz->desc->next = descriptor_list;
    descriptor_list = wiz->desc;
    wiz->desc->connected = CON_PLAYING;
    
    test_socket_output_enabled = true;
    do_echo(wiz, "Test global message");
    test_socket_output_enabled = false;
    
    // Remove from descriptor list
    descriptor_list = wiz->desc->next;
    wiz->desc->next = NULL;
    
    ASSERT_OUTPUT_CONTAINS("global> Test global message");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_echo_without_message()
{
    Room* room = mock_room(60000, NULL, NULL);
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_echo(wiz, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Global echo what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_recho - room echo command
static int test_recho_with_message()
{
    Room* room = mock_room(60000, NULL, NULL);
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    // Add the descriptor to the global list for the recho to work
    wiz->desc->next = descriptor_list;
    descriptor_list = wiz->desc;
    wiz->desc->connected = CON_PLAYING;
    
    test_socket_output_enabled = true;
    do_recho(wiz, "Test room message");
    test_socket_output_enabled = false;
    
    // Remove from descriptor list
    descriptor_list = wiz->desc->next;
    wiz->desc->next = NULL;
    
    ASSERT_OUTPUT_CONTAINS("local> Test room message");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_recho_without_message()
{
    Room* room = mock_room(60000, NULL, NULL);
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_recho(wiz, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Local echo what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_goto - teleport to room by vnum
static int test_goto_by_vnum()
{
    Room* room1 = mock_room(60001, NULL, NULL);
    Room* room2 = mock_room(60002, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    wiz->pcdata->bamfin = str_dup("");   // Initialize bamf messages
    wiz->pcdata->bamfout = str_dup("");
    transfer_mob(wiz, room1);
    
    test_socket_output_enabled = true;  // Capture do_look output
    test_act_output_enabled = true;     // Capture act() messages
    do_goto(wiz, "60002");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(wiz->in_room == room2);
    
    return 0;
}

static int test_goto_invalid_location()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_goto(wiz, "99999");
    test_socket_output_enabled = false;
    
    ASSERT(wiz->in_room == room);  // Didn't move
    ASSERT_OUTPUT_CONTAINS("No such location");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_goto_no_argument()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_goto(wiz, "");
    test_socket_output_enabled = false;
    
    ASSERT(wiz->in_room == room);  // Didn't move
    ASSERT_OUTPUT_CONTAINS("Goto where?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_transfer - summon a player to your location
static int test_transfer_player()
{
    AreaData* ad = mock_area_data();
    ad->inst_type = AREA_INST_SINGLE;  // Ensure it's a single-instance area
    
    Room* room1 = mock_room(60010, mock_room_data(60010, ad), NULL);
    Room* room2 = mock_room(60011, mock_room_data(60011, ad), NULL);

    // Make sure the rooms are in the same area instance
    ASSERT(room1->area == room2->area);
    
    Mobile* wiz = mock_imm("Wizard");
    SET_BIT(wiz->act_flags, PLR_HOLYLIGHT);  // Ensure can see everything
    transfer_mob(wiz, room1);
    
    Mobile* player = mock_player("testplayer");
    transfer_mob(player, room2);
    
    // Note: descriptor_list is not needed for get_mob_world
    // It uses FOR_EACH_GLOBAL_MOB which iterates mob_list
    
    test_socket_output_enabled = true;  // Capture do_look and other output
    test_act_output_enabled = true;     // Capture act() messages
    do_transfer(wiz, "testplayer");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(player->in_room == room1);  // Moved to wizard's room
    
    return 0;
}

static int test_transfer_no_argument()
{
    Room* room = mock_room(60010, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_transfer(wiz, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Transfer whom");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_outfit - give player starting equipment
static int test_outfit_basic()
{
    Room* room = mock_room(60020, NULL, NULL);
    Mobile* player = mock_player("TestPlayer");
    player->level = 1;
    transfer_mob(player, room);
    
    // Count objects before
    int count_before = 0;
    Object* obj;
    FOR_EACH_MOB_OBJ(obj, player) {
        count_before++;
    }
    
    do_outfit(player, "");
    
    // Count objects after
    int count_after = 0;
    FOR_EACH_MOB_OBJ(obj, player) {
        count_after++;
    }
    
    ASSERT(count_after > count_before);  // Should have received items
    
    return 0;
}

static int test_outfit_too_high_level()
{
    Room* room = mock_room(60020, NULL, NULL);
    Mobile* player = mock_player("TestPlayer");
    player->level = 10;  // Too high for outfit
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_outfit(player, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Find it yourself");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_guild - change player's guild location
static int test_guild_requires_argument()
{
    Room* room = mock_room(60030, NULL, NULL);
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_guild(wiz, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Syntax");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

void register_act_wiz_tests()
{
#define REGISTER(n, f)  register_test(&act_wiz_tests, (n), (f))

    init_test_group(&act_wiz_tests, "ACT WIZ TESTS");
    register_test_group(&act_wiz_tests);

    // Echo commands
    REGISTER("Cmd: Echo with message", test_echo_with_message);
    REGISTER("Cmd: Echo without message", test_echo_without_message);
    REGISTER("Cmd: Recho with message", test_recho_with_message);
    REGISTER("Cmd: Recho without message", test_recho_without_message);
    
    // Teleportation commands
    REGISTER("Cmd: Goto by vnum", test_goto_by_vnum);
    REGISTER("Cmd: Goto invalid location", test_goto_invalid_location);
    REGISTER("Cmd: Goto no argument", test_goto_no_argument);
    
    // Transfer command
    REGISTER("Cmd: Transfer player", test_transfer_player);
    REGISTER("Cmd: Transfer no argument", test_transfer_no_argument);
    
    // Outfit command
    REGISTER("Cmd: Outfit basic", test_outfit_basic);
    REGISTER("Cmd: Outfit too high level", test_outfit_too_high_level);
    
    // Guild command
    REGISTER("Cmd: Guild requires argument", test_guild_requires_argument);

#undef REGISTER
}
