////////////////////////////////////////////////////////////////////////////////
// act_move_tests.c
//
// Tests for act_move.c movement and position commands.
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "lox_tests.h"
#include "mock.h"

#include <act_move.h>
#include <comm.h>
#include <handler.h>

#include <data/mobile_data.h>
#include <entities/room.h>

TestGroup act_move_tests;

static int test_north_no_exit()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_north(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(ch->in_room == room);  // Didn't move
    ASSERT_OUTPUT_CONTAINS("Alas, you cannot go that way");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_north_with_exit()
{
    Room* room1 = mock_room(50000, NULL, NULL);
    Room* room2 = mock_room(50001, NULL, NULL);
    mock_room_connection(room1, room2, DIR_NORTH, true);
    
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room1);
    
    do_north(ch, "");
    
    ASSERT(ch->in_room == room2);
    
    return 0;
}

static int test_stand_from_sitting()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    ch->position = POS_SITTING;
    
    test_socket_output_enabled = true;
    do_stand(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(ch->position == POS_STANDING);
    ASSERT_OUTPUT_CONTAINS("You stand up");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_stand_already_standing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    ch->position = POS_STANDING;
    
    test_socket_output_enabled = true;
    do_stand(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You are already standing");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_sit_from_standing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    ch->position = POS_STANDING;
    
    test_socket_output_enabled = true;
    do_sit(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(ch->position == POS_SITTING);
    ASSERT_OUTPUT_CONTAINS("You sit down");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_sit_already_sitting()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    ch->position = POS_SITTING;
    
    test_socket_output_enabled = true;
    do_sit(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You are already sitting");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_rest_from_standing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    ch->position = POS_STANDING;
    
    test_socket_output_enabled = true;
    do_rest(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(ch->position == POS_RESTING);
    ASSERT_OUTPUT_CONTAINS("You rest");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_rest_already_resting()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    ch->position = POS_RESTING;
    
    test_socket_output_enabled = true;
    do_rest(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You are already resting");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_sleep_from_standing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    ch->position = POS_STANDING;
    
    test_socket_output_enabled = true;
    do_sleep(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(ch->position == POS_SLEEPING);
    ASSERT_OUTPUT_CONTAINS("You go to sleep");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_sleep_already_sleeping()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    ch->position = POS_SLEEPING;
    
    test_socket_output_enabled = true;
    do_sleep(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You are already sleeping");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_wake_from_sleeping()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    ch->position = POS_SLEEPING;
    
    test_socket_output_enabled = true;
    do_wake(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(ch->position == POS_STANDING);  // Wake goes to standing
    ASSERT_OUTPUT_CONTAINS("You wake and stand up");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_wake_already_awake()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    ch->position = POS_STANDING;
    
    test_socket_output_enabled = true;
    do_wake(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You are already standing");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_visible_removes_invis()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    SET_BIT(ch->affect_flags, AFF_INVISIBLE);
    
    test_socket_output_enabled = true;
    do_visible(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_AFFECTED(ch, AFF_INVISIBLE));
    ASSERT_OUTPUT_CONTAINS("Ok");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

void register_act_move_tests()
{
#define REGISTER(n, f)  register_test(&act_move_tests, (n), (f))

    init_test_group(&act_move_tests, "ACT MOVE TESTS");
    register_test_group(&act_move_tests);

    REGISTER("Cmd: North no exit", test_north_no_exit);
    REGISTER("Cmd: North with exit", test_north_with_exit);
    REGISTER("Cmd: Stand from sitting", test_stand_from_sitting);
    REGISTER("Cmd: Stand already standing", test_stand_already_standing);
    REGISTER("Cmd: Sit from standing", test_sit_from_standing);
    REGISTER("Cmd: Sit already sitting", test_sit_already_sitting);
    REGISTER("Cmd: Rest from standing", test_rest_from_standing);
    REGISTER("Cmd: Rest already resting", test_rest_already_resting);
    REGISTER("Cmd: Sleep from standing", test_sleep_from_standing);
    REGISTER("Cmd: Sleep already sleeping", test_sleep_already_sleeping);
    REGISTER("Cmd: Wake from sleeping", test_wake_from_sleeping);
    REGISTER("Cmd: Wake already awake", test_wake_already_awake);
    REGISTER("Cmd: Visible removes invis", test_visible_removes_invis);

#undef REGISTER
}
