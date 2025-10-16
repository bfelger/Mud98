///////////////////////////////////////////////////////////////////////////////
// entity_tests.c
///////////////////////////////////////////////////////////////////////////////

// Test in-game entities and their interactions with Lox

#include "tests.h"
#include "test_registry.h"

#include "lox_tests.h"

#include <db.h>

#include <entities/area.h>

#include <lox/vm.h>

TestGroup entity_tests;

static int test_room_script_binding()
{
    // Mock AreaData
    AreaData* ad = new_area_data();
    push(OBJ_VAL(ad));

    // Mock RoomData
    RoomData* rd = new_room_data();
    push(OBJ_VAL(rd));
    rd->area_data = ad;
    VNUM_FIELD(rd) = 1000;
    table_set_vnum(&global_rooms, VNUM_FIELD(rd), OBJ_VAL(rd));

    const char* src =
        "on_enter() { print \"room entry\"; }"
        "on_exit() { print \"room exit\"; }";

    ObjClass* room_class = create_entity_class("test_room", src);
    ASSERT(room_class != NULL);
    rd->header.klass = room_class;

    // Mock Area
    Area* area = create_area_instance(ad, false);
    push(OBJ_VAL(area));

    // Mock Room
    Room* r = new_room(rd, area);
    push(OBJ_VAL(r));

    // Invoke on_enter
    bool inv_result = invoke(lox_string("on_enter"), 0);
    ASSERT(inv_result == true);

    InterpretResult result = run();
    ASSERT_OUTPUT_EQ("room entry\n");

    //print_value(test_output_buffer);

    pop(); // room
    pop(); // area
    pop(); // room data
    pop(); // area data

    test_output_buffer = NIL_VAL;
    return 0;
}

void register_entity_tests()
{
#define REGISTER(n, f)  register_test(&entity_tests, (n), (f))

    init_test_group(&entity_tests, "ENTITY TESTS");
    register_test_group(&entity_tests);

    REGISTER("Room Script Binding", test_room_script_binding);

#undef REGISTER
}