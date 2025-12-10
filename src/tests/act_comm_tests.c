////////////////////////////////////////////////////////////////////////////////
// act_comm_tests.c
//
// Tests for act_comm.c communication commands.
//
// Standard command test pattern:
//   1. Create room: Room* room = mock_room(vnum, NULL, NULL);
//   2. Create entity: Mobile* ch = mock_player("Name");
//   3. Place in world: transfer_mob(ch, room);
//   4. Enable output capture: test_socket_output_enabled = true;
//   5. Execute command: do_command(ch, "args");
//   6. Disable capture: test_socket_output_enabled = false;
//   7. Assert output: ASSERT_OUTPUT_CONTAINS("expected");
//   8. Reset buffer: test_output_buffer = NIL_VAL;
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "lox_tests.h"
#include "mock.h"

#include <act_comm.h>
#include <comm.h>
#include <handler.h>

#include <data/mobile_data.h>
#include <entities/room.h>

TestGroup act_comm_tests;

static int test_deaf_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->comm_flags, COMM_DEAF);
    test_socket_output_enabled = true;
    do_deaf(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->comm_flags, COMM_DEAF));
    ASSERT_OUTPUT_CONTAINS("you won't hear tells");
    test_output_buffer = NIL_VAL;
    
    test_socket_output_enabled = true;
    do_deaf(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_SET(ch->comm_flags, COMM_DEAF));
    ASSERT_OUTPUT_CONTAINS("You can now hear tells again");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_quiet_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->comm_flags, COMM_QUIET);
    test_socket_output_enabled = true;
    do_quiet(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->comm_flags, COMM_QUIET));
    ASSERT_OUTPUT_CONTAINS("you will only hear says and emotes");
    test_output_buffer = NIL_VAL;
    
    test_socket_output_enabled = true;
    do_quiet(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_SET(ch->comm_flags, COMM_QUIET));
    ASSERT_OUTPUT_CONTAINS("Quiet mode removed");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_afk_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->comm_flags, COMM_AFK);
    test_socket_output_enabled = true;
    do_afk(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->comm_flags, COMM_AFK));
    ASSERT_OUTPUT_CONTAINS("You are now in AFK mode");
    test_output_buffer = NIL_VAL;
    
    test_socket_output_enabled = true;
    do_afk(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_SET(ch->comm_flags, COMM_AFK));
    ASSERT_OUTPUT_CONTAINS("AFK mode removed");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_replay_empty()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_replay(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You have no tells to replay");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_say_empty()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_say(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Say what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_say_message()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_say(ch, "Hello world");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You say");
    ASSERT_OUTPUT_CONTAINS("Hello world");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_yell_empty()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->comm_flags, COMM_NOSHOUT);
    
    test_socket_output_enabled = true;
    do_yell(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Yell what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_yell_message()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->comm_flags, COMM_NOSHOUT);
    
    test_socket_output_enabled = true;
    do_yell(ch, "Help!");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You yell");
    ASSERT_OUTPUT_CONTAINS("Help!");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_emote_empty()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_emote(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Emote what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_emote_action()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_emote(ch, "smiles happily.");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("TestPlayer smiles happily");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

void register_act_comm_tests()
{
#define REGISTER(n, f)  register_test(&act_comm_tests, (n), (f))

    init_test_group(&act_comm_tests, "ACT COMM TESTS");
    register_test_group(&act_comm_tests);

    REGISTER("Cmd: Deaf toggle", test_deaf_toggle);
    REGISTER("Cmd: Quiet toggle", test_quiet_toggle);
    REGISTER("Cmd: AFK toggle", test_afk_toggle);
    REGISTER("Cmd: Replay empty", test_replay_empty);
    REGISTER("Cmd: Say empty", test_say_empty);
    REGISTER("Cmd: Say message", test_say_message);
    REGISTER("Cmd: Yell empty", test_yell_empty);
    REGISTER("Cmd: Yell message", test_yell_message);
    REGISTER("Cmd: Emote empty", test_emote_empty);
    REGISTER("Cmd: Emote action", test_emote_action);

#undef REGISTER
}
