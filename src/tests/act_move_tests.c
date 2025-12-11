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

// Door manipulation tests
static int test_open_door()
{
    Room* room1 = mock_room(50100, NULL, NULL);
    Room* room2 = mock_room(50101, NULL, NULL);
    mock_room_connection(room1, room2, DIR_NORTH, true);
    
    // Set up a closed door
    room1->exit[DIR_NORTH]->exit_flags = EX_ISDOOR | EX_CLOSED;
    room1->exit[DIR_NORTH]->data->keyword = str_dup("door");
    
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room1);
    
    test_socket_output_enabled = true;
    do_open(ch, "north");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_SET(room1->exit[DIR_NORTH]->exit_flags, EX_CLOSED));
    ASSERT_OUTPUT_CONTAINS("Ok");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_open_already_open_door()
{
    Room* room1 = mock_room(50100, NULL, NULL);
    Room* room2 = mock_room(50101, NULL, NULL);
    mock_room_connection(room1, room2, DIR_NORTH, true);
    
    room1->exit[DIR_NORTH]->exit_flags = EX_ISDOOR;  // Open door
    room1->exit[DIR_NORTH]->data->keyword = str_dup("door");
    
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room1);
    
    test_socket_output_enabled = true;
    do_open(ch, "north");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("already open");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_close_door()
{
    Room* room1 = mock_room(50100, NULL, NULL);
    Room* room2 = mock_room(50101, NULL, NULL);
    mock_room_connection(room1, room2, DIR_NORTH, true);
    
    // Set up an open door
    room1->exit[DIR_NORTH]->exit_flags = EX_ISDOOR;
    room1->exit[DIR_NORTH]->data->keyword = str_dup("door");
    
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room1);
    
    test_socket_output_enabled = true;
    do_close(ch, "north");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(room1->exit[DIR_NORTH]->exit_flags, EX_CLOSED));
    ASSERT_OUTPUT_CONTAINS("Ok");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_close_already_closed_door()
{
    Room* room1 = mock_room(50100, NULL, NULL);
    Room* room2 = mock_room(50101, NULL, NULL);
    mock_room_connection(room1, room2, DIR_NORTH, true);
    
    room1->exit[DIR_NORTH]->exit_flags = EX_ISDOOR | EX_CLOSED;
    room1->exit[DIR_NORTH]->data->keyword = str_dup("door");
    
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room1);
    
    test_socket_output_enabled = true;
    do_close(ch, "north");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("already closed");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_lock_door_with_key()
{
    Room* room1 = mock_room(50100, NULL, NULL);
    Room* room2 = mock_room(50101, NULL, NULL);
    mock_room_connection(room1, room2, DIR_NORTH, true);
    
    // Set up a closed, unlocked door with key vnum 50110 (tests VNUM support)
    room1->exit[DIR_NORTH]->exit_flags = EX_ISDOOR | EX_CLOSED;
    room1->exit[DIR_NORTH]->data->keyword = str_dup("door");
    room1->exit[DIR_NORTH]->data->key = 50110;
    
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room1);
    
    // Give player the key
    Object* key = mock_obj("key", 50110, NULL);
    obj_to_char(key, ch);
    
    test_socket_output_enabled = true;
    do_lock(ch, "north");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(room1->exit[DIR_NORTH]->exit_flags, EX_LOCKED));
    ASSERT_OUTPUT_CONTAINS("*Click*");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_lock_door_without_key()
{
    Room* room1 = mock_room(50100, NULL, NULL);
    Room* room2 = mock_room(50101, NULL, NULL);
    mock_room_connection(room1, room2, DIR_NORTH, true);
    
    room1->exit[DIR_NORTH]->exit_flags = EX_ISDOOR | EX_CLOSED;
    room1->exit[DIR_NORTH]->data->keyword = str_dup("door");
    room1->exit[DIR_NORTH]->data->key = 50110;
    
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room1);
    // No key in inventory
    
    test_socket_output_enabled = true;
    do_lock(ch, "north");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_SET(room1->exit[DIR_NORTH]->exit_flags, EX_LOCKED));
    ASSERT_OUTPUT_CONTAINS("You lack the key");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_unlock_door_with_key()
{
    Room* room1 = mock_room(50100, NULL, NULL);
    Room* room2 = mock_room(50101, NULL, NULL);
    mock_room_connection(room1, room2, DIR_NORTH, true);
    
    // Set up a locked door
    room1->exit[DIR_NORTH]->exit_flags = EX_ISDOOR | EX_CLOSED | EX_LOCKED;
    room1->exit[DIR_NORTH]->data->keyword = str_dup("door");
    room1->exit[DIR_NORTH]->data->key = 50110;
    
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room1);
    
    // Give player the key
    Object* key = mock_obj("key", 50110, NULL);
    obj_to_char(key, ch);
    
    test_socket_output_enabled = true;
    do_unlock(ch, "north");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_SET(room1->exit[DIR_NORTH]->exit_flags, EX_LOCKED));
    ASSERT_OUTPUT_CONTAINS("*Click*");  // Doors send "*Click*", containers send "You unlock"
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_open_container()
{
    Room* room = mock_room(50100, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    ObjPrototype* proto = mock_obj_proto(50120);
    proto->item_type = ITEM_CONTAINER;
    proto->container.flags = CONT_CLOSEABLE | CONT_CLOSED;
    
    Object* chest = mock_obj("chest", 50120, proto);
    obj_to_room(chest, room);
    
    test_socket_output_enabled = true;
    do_open(ch, "chest");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_SET(chest->container.flags, CONT_CLOSED));
    ASSERT_OUTPUT_CONTAINS("You open");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_close_container()
{
    Room* room = mock_room(50100, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    ObjPrototype* proto = mock_obj_proto(50120);
    proto->item_type = ITEM_CONTAINER;
    proto->container.flags = CONT_CLOSEABLE;  // Open
    
    Object* chest = mock_obj("chest", 50120, proto);
    obj_to_room(chest, room);
    
    test_socket_output_enabled = true;
    do_close(ch, "chest");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(chest->container.flags, CONT_CLOSED));
    ASSERT_OUTPUT_CONTAINS("You close");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_lock_container_with_key()
{
    Room* room = mock_room(50100, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    ObjPrototype* proto = mock_obj_proto(50120);
    proto->item_type = ITEM_CONTAINER;
    proto->container.flags = CONT_CLOSEABLE | CONT_CLOSED;
    proto->container.key_vnum = 50121;  // Tests full VNUM range support
    
    Object* chest = mock_obj("chest", 50120, proto);
    obj_to_room(chest, room);
    
    // Give player the key
    Object* key = mock_obj("key", 50121, NULL);
    obj_to_char(key, ch);
    
    test_socket_output_enabled = true;
    do_lock(ch, "chest");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(chest->container.flags, CONT_LOCKED));
    ASSERT_OUTPUT_CONTAINS("You lock");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

void register_act_move_tests()
{
#define REGISTER(n, f)  register_test(&act_move_tests, (n), (f))

    init_test_group(&act_move_tests, "ACT MOVE TESTS");
    register_test_group(&act_move_tests);

    // Movement tests
    REGISTER("Cmd: North no exit", test_north_no_exit);
    REGISTER("Cmd: North with exit", test_north_with_exit);
    
    // Position tests
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
    
    // Door manipulation tests
    REGISTER("Cmd: Open door", test_open_door);
    REGISTER("Cmd: Open already open door", test_open_already_open_door);
    REGISTER("Cmd: Close door", test_close_door);
    REGISTER("Cmd: Close already closed door", test_close_already_closed_door);
    REGISTER("Cmd: Lock door with key", test_lock_door_with_key);
    REGISTER("Cmd: Lock door without key", test_lock_door_without_key);
    REGISTER("Cmd: Unlock door with key", test_unlock_door_with_key);
    
    // Container manipulation tests
    REGISTER("Cmd: Open container", test_open_container);
    REGISTER("Cmd: Close container", test_close_container);
    REGISTER("Cmd: Lock container with key", test_lock_container_with_key);

#undef REGISTER
}
