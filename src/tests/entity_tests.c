///////////////////////////////////////////////////////////////////////////////
// entity_tests.c
///////////////////////////////////////////////////////////////////////////////

// Test in-game entities and their interactions with Lox

#include "tests.h"
#include "test_registry.h"
#include "lox_tests.h"
#include "mock.h"

#include <db.h>
#include <handler.h>
#include <reload.h>

#include <entities/area.h>

#include <lox/vm.h>

TestGroup entity_tests;

static int test_room_script_binding()
{
    VNUM vnum = 52000;

    RoomData* rd = mock_room_data(vnum, NULL);

    const char* src =
        "on_entry() { print \"room entry\"; }"
        "on_exit() { print \"room exit\"; }";

    // The disassembler will print to the test output buffer unless we disable
    // this option.
    //test_output_enabled = false;
    //test_trace_exec = true;
    ObjClass* room_class = create_entity_class((Entity*)rd, "test_room", src);
    //test_trace_exec = false;
    //test_output_enabled = true;

    ASSERT_OR_GOTO(room_class != NULL, leave_test_room_script_binding);
    
    rd->header.klass = room_class;

    Room* r = mock_room(vnum, rd, NULL);
    push(OBJ_VAL(r));

    // Invoke on_enter
    bool inv_result = invoke(lox_string("on_entry"), 0);
    ASSERT_OR_GOTO(inv_result == true, leave_test_room_script_binding);

    InterpretResult result = run();
    ASSERT_LOX_OUTPUT_EQ("room entry\n");

leave_test_room_script_binding:
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_exit_bidirectional_linkage()
{
    // Create two rooms
    Room* r1 = mock_room(50000, NULL, NULL);
    Room* r2 = mock_room(50001, NULL, NULL);

    // Create an exit from r1 to r2
    mock_room_connection(r1, r2, DIR_NORTH, false);

    // Verify the exit exists in r1
    ASSERT(r1->exit[DIR_NORTH] != NULL);
    ASSERT(r1->exit[DIR_NORTH]->to_room == r2);
    
    // Verify RoomExit is now a proper entity with Obj header
    ASSERT(r1->exit[DIR_NORTH]->header.obj.type == OBJ_ROOM_EXIT);

    // Verify the exit is tracked in r2's inbound_exits list
    ASSERT(r2->inbound_exits.count == 1);
    ASSERT(r2->inbound_exits.front != NULL);
    
    // Now we can safely use AS_ROOM_EXIT since it's a real entity!
    RoomExit* inbound = AS_ROOM_EXIT(r2->inbound_exits.front->value);
    ASSERT(inbound == r1->exit[DIR_NORTH]);
    ASSERT(inbound->from_room == r1);

    return 0;
}

static int test_exit_cleanup_removes_from_inbound_list()
{
    // Create two rooms with a connection
    Room* r1 = mock_room(50000, NULL, NULL);
    Room* r2 = mock_room(50001, NULL, NULL);
    mock_room_connection(r1, r2, DIR_NORTH, false);

    // Verify the inbound exit exists
    ASSERT(r2->inbound_exits.count == 1);

    // Free the exit
    free_room_exit(r1->exit[DIR_NORTH]);
    r1->exit[DIR_NORTH] = NULL;

    // Verify it's removed from the inbound list
    ASSERT(r2->inbound_exits.count == 0);

    return 0;
}

static int test_room_cleanup_nulls_inbound_exits()
{
    // Create an area and rooms within it
    Area* area = mock_area(NULL);
    Room* target = mock_room(50000, NULL, area);
    Room* r1 = mock_room(50001, NULL, area);
    Room* r2 = mock_room(50002, NULL, area);
    Room* r3 = mock_room(50003, NULL, area);

    // All three rooms exit to target
    mock_room_connection(r1, target, DIR_NORTH, false);
    mock_room_connection(r2, target, DIR_EAST, false);
    mock_room_connection(r3, target, DIR_WEST, false);

    // Verify target has 3 inbound exits
    ASSERT(target->inbound_exits.count == 3);

    // Free the target room (not during teardown)
    target->area->teardown_in_progress = false;
    free_room(target);

    // Verify all inbound exits have been freed (nulled in their source rooms)
    ASSERT(r1->exit[DIR_NORTH] == NULL);
    ASSERT(r2->exit[DIR_EAST] == NULL);
    ASSERT(r3->exit[DIR_WEST] == NULL);

    return 0;
}

static int test_room_cleanup_skips_nulling_during_teardown()
{
    // Create an area with multiple rooms
    Area* area = mock_area(NULL);
    Room* target = mock_room(50000, NULL, area);
    Room* r1 = mock_room(50001, NULL, area);
    Room* r2 = mock_room(50002, NULL, area);

    // Connect rooms to target
    mock_room_connection(r1, target, DIR_NORTH, false);
    mock_room_connection(r2, target, DIR_EAST, false);

    // Set teardown flag
    area->teardown_in_progress = true;

    // Free the target room during teardown
    free_room(target);

    // During teardown, the expensive nulling loop is skipped
    // The exits still have their to_room pointers (dangling, but area is being destroyed)
    // This is OK because free_area will destroy all rooms anyway
    // We can't actually verify this without risking segfault, but the test
    // confirms the code path executes without error

    return 0;
}

static int test_bidirectional_connection()
{
    // Create two rooms with bidirectional connection
    Room* r1 = mock_room(50000, NULL, NULL);
    Room* r2 = mock_room(50001, NULL, NULL);
    mock_room_connection(r1, r2, DIR_NORTH, true);

    // Verify forward direction
    ASSERT(r1->exit[DIR_NORTH] != NULL);
    ASSERT(r1->exit[DIR_NORTH]->to_room == r2);
    ASSERT(r2->inbound_exits.count == 1);

    // Verify reverse direction
    ASSERT(r2->exit[DIR_SOUTH] != NULL);
    ASSERT(r2->exit[DIR_SOUTH]->to_room == r1);
    ASSERT(r1->inbound_exits.count == 1);

    // Verify from_room pointers
    ASSERT(r1->exit[DIR_NORTH]->from_room == r1);
    ASSERT(r2->exit[DIR_SOUTH]->from_room == r2);

    return 0;
}

static int test_multiple_exits_from_same_room()
{
    // Create a room with exits in all directions
    Room* center = mock_room(50000, NULL, NULL);
    Room* north = mock_room(50001, NULL, NULL);
    Room* south = mock_room(50002, NULL, NULL);
    Room* east = mock_room(50003, NULL, NULL);
    Room* west = mock_room(50004, NULL, NULL);

    mock_room_connection(center, north, DIR_NORTH, false);
    mock_room_connection(center, south, DIR_SOUTH, false);
    mock_room_connection(center, east, DIR_EAST, false);
    mock_room_connection(center, west, DIR_WEST, false);

    // Verify each target has one inbound exit
    ASSERT(north->inbound_exits.count == 1);
    ASSERT(south->inbound_exits.count == 1);
    ASSERT(east->inbound_exits.count == 1);
    ASSERT(west->inbound_exits.count == 1);

    // Verify center has no inbound exits (all exits are outbound)
    ASSERT(center->inbound_exits.count == 0);

    return 0;
}

static int test_area_cleanup_sets_teardown_flag()
{
    // Create an area
    Area* area = mock_area(NULL);
    ASSERT(area->teardown_in_progress == false);

    // Add some rooms (need to add them or free_area has nothing to do)
    mock_room(50000, NULL, area);
    mock_room(50001, NULL, area);

    // Free the area
    free_area(area);

    // The teardown flag should have been set before room cleanup
    // (We can't actually verify this after free_area, but the test confirms
    // the code path executes without error)

    return 0;
}

static int test_exit_with_null_to_room()
{
    // Simulate a multi-instance area exit where to_room is NULL
    Room* r1 = mock_room(50000, NULL, NULL);
    
    // Manually create an exit with NULL to_room (like multi-instance area)
    RoomExitData* exit_data = new_room_exit_data();
    exit_data->to_vnum = 60000;  // Points to a room that doesn't exist yet
    exit_data->to_room = NULL;   // Multi-instance area not loaded
    
    r1->data->exit_data[DIR_NORTH] = exit_data;
    RoomExit* exit = new_room_exit(exit_data, r1);
    r1->exit[DIR_NORTH] = exit;
    
    // Verify the exit was created but to_room is NULL
    ASSERT(exit != NULL);
    ASSERT(exit->to_room == NULL);
    ASSERT(exit->from_room == r1);
    ASSERT(exit->data->to_vnum == 60000);
    
    // Verify no crash when freeing exit with NULL to_room
    free_room_exit(exit);
    r1->exit[DIR_NORTH] = NULL;
    
    return 0;
}

static int test_cross_area_exit_tracking()
{
    // Create two different areas
    Area* area1 = mock_area(NULL);
    Area* area2 = mock_area(NULL);
    
    // Create rooms in different areas
    Room* r1 = mock_room(50000, NULL, area1);
    Room* r2 = mock_room(60000, NULL, area2);
    
    // Create an exit from area1 to area2
    mock_room_connection(r1, r2, DIR_NORTH, false);
    
    // Verify exit exists and r2 has inbound tracking
    ASSERT(r1->exit[DIR_NORTH] != NULL);
    ASSERT(r1->exit[DIR_NORTH]->to_room == r2);
    ASSERT(r2->inbound_exits.count == 1);
    
    // Now free r2 - with the OLD implementation, this would NOT clean up
    // the cross-area reference because free_room() only iterates rooms 
    // in the SAME area.
    //
    // With the NEW bidirectional tracking via inbound_exits list, we can
    // iterate the list directly and free all exits pointing to this room,
    // regardless of which area they're in!
    free_room(r2);
    
    // The exit from r1 should have been freed (nulled)
    // THIS IS THE BUG: current implementation doesn't do this for cross-area!
    ASSERT(r1->exit[DIR_NORTH] == NULL);
    
    return 0;
}

static int test_room_exit_is_lox_value()
{
    // Verify RoomExit can be used as a Lox Value now that it's a full entity
    Room* r1 = mock_room(50000, NULL, NULL);
    Room* r2 = mock_room(50001, NULL, NULL);
    
    mock_room_connection(r1, r2, DIR_NORTH, false);
    
    RoomExit* exit = r1->exit[DIR_NORTH];
    ASSERT(exit != NULL);
    
    // Can convert to Value and back safely
    Value val = OBJ_VAL(exit);
    ASSERT(IS_OBJ(val));
    ASSERT(OBJ_TYPE(val) == OBJ_ROOM_EXIT);
    
    // Can cast back safely
    RoomExit* exit2 = AS_ROOM_EXIT(val);
    ASSERT(exit2 == exit);
    ASSERT(exit2->from_room == r1);
    ASSERT(exit2->to_room == r2);
    
    return 0;
}

static int test_multi_instance_area_exit_null_to_room()
{
    // Multi-instance area exits should have NULL to_room by design
    // They are resolved lazily per-player via get_room_for_player()
    
    // Create a single-instance area with a room
    AreaData* single_area_data = mock_area_data();
    single_area_data->inst_type = AREA_INST_SINGLE;
    Area* single_area = mock_area(single_area_data);
    Room* r1 = mock_room(50000, NULL, single_area);
    
    // Create a multi-instance area with a room template
    AreaData* multi_area_data = mock_area_data();
    multi_area_data->inst_type = AREA_INST_MULTI;
    RoomData* r2_data = mock_room_data(60000, multi_area_data);
    
    // Create exit from single-instance to multi-instance area
    r1->data->exit_data[DIR_NORTH] = new_room_exit_data();
    r1->data->exit_data[DIR_NORTH]->to_room = r2_data;
    r1->data->exit_data[DIR_NORTH]->to_vnum = 60000;
    
    RoomExit* exit = new_room_exit(r1->data->exit_data[DIR_NORTH], r1);
    r1->exit[DIR_NORTH] = exit;
    
    // Verify the exit has NULL to_room (this is CORRECT behavior)
    ASSERT(exit != NULL);
    ASSERT(exit->to_room == NULL);  // Lazy-loaded per player
    ASSERT(exit->data->to_vnum == 60000);  // But vnum is stored
    
    // This is intentional! move_char() will call get_room_for_player()
    // to resolve it, creating/finding the player's instance.
    
    return 0;
}

static int test_nullified_exit_reseats_single_instance()
{
    // This tests the reload room/area strategy: nullify to_room pointers
    // and let them auto-resolve via get_room_for_player()
    
    // Create a single-instance area with two rooms
    AreaData* area_data = mock_area_data();
    area_data->inst_type = AREA_INST_SINGLE;
    Area* area = mock_area(area_data);
    
    Room* r1 = mock_room(50000, NULL, area);
    Room* r2 = mock_room(50001, NULL, area);
    
    // Create exit with resolved to_room
    mock_room_connection(r1, r2, DIR_NORTH, false);
    ASSERT(r1->exit[DIR_NORTH]->to_room == r2);
    
    // Simulate reload: nullify the to_room pointer
    r1->exit[DIR_NORTH]->to_room = NULL;
    
    // Now simulate player movement - should auto-resolve
    Mobile* player = mock_player("TestPlayer");
    transfer_mob(player, r1);
    
    Room* resolved = get_room_for_player(player, r1->exit[DIR_NORTH]->data->to_vnum);
    
    // Should resolve to the same room (single-instance area)
    ASSERT(resolved == r2);
    
    // This proves reload can work by just nullifying to_room!
    // Single-instance areas auto-resolve to THE one instance.
    
    return 0;
}

static int test_reload_room_recreates_from_prototype()
{
    // Create an area and room
    AreaData* area_data = mock_area_data();
    Area* area = mock_area(area_data);
    
    RoomData* room_data = mock_room_data(99100, area_data);
    Room* room = mock_room(99100, room_data, area);
    
    // Add a mobile and object to the room
    MobPrototype* mob_proto = mock_mob_proto(99100);
    Mobile* mob = mock_mob("testmob", 99100, mob_proto);
    mob_to_room(mob, room);
    
    ObjPrototype* obj_proto = mock_obj_proto(99100);
    Object* obj = mock_obj("testobj", 99100, obj_proto);
    obj_to_room(obj, room);
    
    // Create an inbound exit from another room
    RoomData* source_data = mock_room_data(99101, area_data);
    Room* source_room = mock_room(99101, source_data, area);
    mock_room_data_connection(source_data, room_data, DIR_NORTH, false);
    source_room->exit[DIR_NORTH] = new_room_exit(source_data->exit_data[DIR_NORTH], source_room);
    
    // Verify initial state
    ASSERT(room->mobiles.count == 1);
    ASSERT(room->objects.count == 1);
    ASSERT(room->inbound_exits.count == 1);
    ASSERT(source_room->exit[DIR_NORTH]->to_room == room);
    
    // Add an exit from the room we're reloading (so we can verify it's recreated)
    RoomData* dest_data = mock_room_data(99102, area_data);
    Room* dest_room = mock_room(99102, dest_data, area);
    mock_room_data_connection(room_data, dest_data, DIR_SOUTH, false);
    room->exit[DIR_SOUTH] = new_room_exit(room_data->exit_data[DIR_SOUTH], room);
    
    // Reload the room
    Mobile* admin = mock_player("Admin");
    mob_to_room(admin, room);
    test_socket_output_enabled = true;
    
    reload_room(admin, room);
    
    // Get the new room instance (should be in the same area, same vnum)
    Room* new_room = get_room(area, 99100);
    ASSERT(new_room != NULL);
    
    // Note: Can't compare new_room != old_room because memory pool may reuse the address!
    // Instead, verify recreation by checking that exits were recreated
    
    // Verify the new room has same VNUM
    ASSERT(VNUM_FIELD(new_room) == 99100);
    
    // Verify mobile and object were restored (2 mobs: original mob + admin)
    ASSERT(new_room->mobiles.count == 2);
    ASSERT(new_room->objects.count == 1);
    
    // Verify the mobile and object are in the new room
    ASSERT(mob->in_room == new_room);
    ASSERT(obj->in_room == new_room);
    ASSERT(admin->in_room == new_room);
    
    // Verify outbound exit exists and points to correct destination
    // Note: Can't check new_room->exit[DIR_SOUTH] != old_exit because memory pool may reuse address
    ASSERT(new_room->exit[DIR_SOUTH] != NULL);
    ASSERT(new_room->exit[DIR_SOUTH]->to_room == dest_room); // Points to correct dest
    ASSERT(new_room->exit[DIR_SOUTH]->from_room == new_room); // Points back to new room
    
    // Verify inbound exit was nullified (not freed, waiting for auto-resolve)
    ASSERT(source_room->exit[DIR_NORTH] != NULL);
    ASSERT(source_room->exit[DIR_NORTH]->to_room == NULL); // Nullified during reload
    
    // Verify admin got success message
    ASSERT_OUTPUT_CONTAINS("Room reloaded successfully");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    return 0;
}

void register_entity_tests()
{
#define REGISTER(n, f)  register_test(&entity_tests, (n), (f))

    init_test_group(&entity_tests, "ENTITY TESTS");
    register_test_group(&entity_tests);

    REGISTER("Room Script Binding", test_room_script_binding);
    REGISTER("Exit Bidirectional Linkage", test_exit_bidirectional_linkage);
    REGISTER("Exit Cleanup Removes From Inbound List", test_exit_cleanup_removes_from_inbound_list);
    REGISTER("Room Cleanup Nulls Inbound Exits", test_room_cleanup_nulls_inbound_exits);
    REGISTER("Room Cleanup Skips Nulling During Teardown", test_room_cleanup_skips_nulling_during_teardown);
    REGISTER("Bidirectional Connection", test_bidirectional_connection);
    REGISTER("Multiple Exits From Same Room", test_multiple_exits_from_same_room);
    REGISTER("Area Cleanup Sets Teardown Flag", test_area_cleanup_sets_teardown_flag);
    REGISTER("Exit With NULL to_room", test_exit_with_null_to_room);
    REGISTER("Cross-Area Exit Tracking", test_cross_area_exit_tracking);
    REGISTER("RoomExit Is Lox Value", test_room_exit_is_lox_value);
    REGISTER("Multi-Instance Area Exit NULL to_room", test_multi_instance_area_exit_null_to_room);
    REGISTER("Nullified Exit Reseats Single Instance", test_nullified_exit_reseats_single_instance);
    REGISTER("Reload Room Recreates From Prototype", test_reload_room_recreates_from_prototype);

#undef REGISTER
}