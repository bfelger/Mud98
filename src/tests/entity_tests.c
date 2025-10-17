///////////////////////////////////////////////////////////////////////////////
// entity_tests.c
///////////////////////////////////////////////////////////////////////////////

// Test in-game entities and their interactions with Lox

#include "tests.h"
#include "test_registry.h"
#include "lox_tests.h"
#include "mock.h"

#include <db.h>

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

    ObjClass* room_class = create_entity_class("test_room", src);
    ASSERT(room_class != NULL);
    rd->header.klass = room_class;

    Room* r = mock_room(vnum, rd, NULL);
    push(OBJ_VAL(r));

    // Invoke on_enter
    bool inv_result = invoke(lox_string("on_entry"), 0);
    ASSERT(inv_result == true);

    InterpretResult result = run();
    ASSERT_OUTPUT_EQ("room entry\n");

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