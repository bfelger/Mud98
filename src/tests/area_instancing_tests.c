////////////////////////////////////////////////////////////////////////////////
// area_instancing_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <command.h>
#include <db.h>
#include <handler.h>

#include <entities/area.h>
#include <entities/room_exit.h>

#include <data/direction.h>

TestGroup area_instancing_tests;

typedef struct InstancedDoorFixture {
    AreaData* area_data;
    Area* area;
    RoomData* room_from_data;
    RoomData* room_to_data;
    Room* room_from;
    Room* room_to;
    VNUM key_vnum;
} InstancedDoorFixture;

static InstancedDoorFixture setup_multi_instance_door(VNUM from_vnum, VNUM to_vnum)
{
    InstancedDoorFixture fixture = { 0 };
    fixture.key_vnum = 60010;

    fixture.area_data = mock_area_data();
    fixture.area_data->inst_type = AREA_INST_MULTI;

    fixture.room_from_data = mock_room_data(from_vnum, fixture.area_data);
    fixture.room_to_data = mock_room_data(to_vnum, fixture.area_data);

    RoomExitData* exit_east = new_room_exit_data();
    exit_east->orig_dir = DIR_EAST;
    exit_east->to_vnum = VNUM_FIELD(fixture.room_to_data);
    exit_east->to_room = fixture.room_to_data;
    exit_east->key = fixture.key_vnum;
    exit_east->exit_reset_flags = EX_ISDOOR | EX_CLOSED | EX_LOCKED | EX_PICKPROOF;
    exit_east->keyword = str_dup("door");
    exit_east->description = str_dup("A simple barrier.\n\r");
    fixture.room_from_data->exit_data[DIR_EAST] = exit_east;

    RoomExitData* exit_west = new_room_exit_data();
    exit_west->orig_dir = DIR_WEST;
    exit_west->to_vnum = VNUM_FIELD(fixture.room_from_data);
    exit_west->to_room = fixture.room_from_data;
    exit_west->key = fixture.key_vnum;
    exit_west->exit_reset_flags = EX_ISDOOR | EX_CLOSED;
    exit_west->keyword = str_dup("door");
    exit_west->description = str_dup("A simple barrier.\n\r");
    fixture.room_to_data->exit_data[DIR_WEST] = exit_west;

    fixture.area = create_area_instance(fixture.area_data, true);
    reset_area(fixture.area);

    fixture.room_from = get_room(fixture.area, from_vnum);
    fixture.room_to = get_room(fixture.area, to_vnum);

    return fixture;
}

static Object* setup_key(Mobile* ch, VNUM key_vnum)
{
    ObjPrototype* key_proto = mock_obj_proto(key_vnum);
    key_proto->item_type = ITEM_KEY;

    Object* key = mock_obj("key", key_vnum, key_proto);
    obj_to_char(key, ch);

    return key;
}

static int test_multi_instance_exit_flags_and_keyword()
{
    InstancedDoorFixture fixture = setup_multi_instance_door(60000, 60001);

    RoomExit* exit = fixture.room_from->exit[DIR_EAST];
    ASSERT(exit != NULL);
    ASSERT(exit->to_room == fixture.room_to);
    ASSERT(IS_SET(exit->exit_flags, EX_ISDOOR));
    ASSERT(IS_SET(exit->exit_flags, EX_CLOSED));
    ASSERT(IS_SET(exit->exit_flags, EX_LOCKED));
    ASSERT(IS_SET(exit->exit_flags, EX_PICKPROOF));
    ASSERT_STR_EQ("door", exit->data->keyword);

    return 0;
}

static int test_multi_instance_door_visibility_in_exits()
{
    InstancedDoorFixture fixture = setup_multi_instance_door(60002, 60003);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, fixture.room_from);
    setup_key(ch, fixture.key_vnum);

    test_socket_output_enabled = true;
    do_exits(ch, "auto");
    test_socket_output_enabled = false;

    ASSERT_OUTPUT_CONTAINS("none");
    test_output_buffer = NIL_VAL;

    test_socket_output_enabled = true;
    do_unlock(ch, "east");
    do_open(ch, "east");
    do_exits(ch, "auto");
    test_socket_output_enabled = false;

    ASSERT_OUTPUT_CONTAINS("east");
    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_multi_instance_look_shows_closed_keyword()
{
    InstancedDoorFixture fixture = setup_multi_instance_door(60004, 60005);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, fixture.room_from);

    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_look(ch, "east");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;

    ASSERT_OUTPUT_CONTAINS("door");
    ASSERT_OUTPUT_CONTAINS("closed");
    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_lazy_multi_instance_exit_creates_instance()
{
    VNUM source_vnum = 60010;
    VNUM target_vnum = 60011;
    VNUM key_vnum = 60012;

    AreaData* source_area = mock_area_data();
    source_area->inst_type = AREA_INST_SINGLE;
    RoomData* source_room_data = mock_room_data(source_vnum, source_area);

    AreaData* target_area = mock_area_data();
    target_area->inst_type = AREA_INST_MULTI;
    RoomData* target_room_data = mock_room_data(target_vnum, target_area);

    RoomExitData* exit_east = new_room_exit_data();
    exit_east->orig_dir = DIR_EAST;
    exit_east->to_vnum = target_vnum;
    exit_east->to_room = target_room_data;
    exit_east->key = key_vnum;
    exit_east->exit_reset_flags = EX_ISDOOR | EX_CLOSED | EX_LOCKED;
    exit_east->keyword = str_dup("door");
    source_room_data->exit_data[DIR_EAST] = exit_east;

    Area* source_instance = create_area_instance(source_area, true);
    reset_area(source_instance);

    Room* source_room = get_room(source_instance, source_vnum);
    ASSERT(source_room != NULL);
    ASSERT(source_room->exit[DIR_EAST] != NULL);
    ASSERT(source_room->exit[DIR_EAST]->to_room == NULL);
    ASSERT(target_area->instances.count == 0);

    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, source_room);
    setup_key(ch, key_vnum);

    test_socket_output_enabled = true;
    do_unlock(ch, "east");
    do_open(ch, "east");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;

    do_east(ch, "");

    ASSERT(ch->in_room != NULL);
    ASSERT(ch->in_room->area->data == target_area);
    ASSERT(VNUM_FIELD(ch->in_room) == target_vnum);
    ASSERT(target_area->instances.count == 1);

    return 0;
}

void register_area_instancing_tests()
{
#define REGISTER(name, fn) register_test(&area_instancing_tests, (name), (fn))

    init_test_group(&area_instancing_tests, "AREA INSTANCING TESTS");
    register_test_group(&area_instancing_tests);

    REGISTER("Multi-Instance Exit Flags And Keyword", test_multi_instance_exit_flags_and_keyword);
    REGISTER("Multi-Instance Door Visibility In Exits", test_multi_instance_door_visibility_in_exits);
    REGISTER("Multi-Instance Look Shows Closed Keyword", test_multi_instance_look_shows_closed_keyword);
    REGISTER("Lazy Multi-Instance Exit Creation", test_lazy_multi_instance_exit_creates_instance);

#undef REGISTER
}
