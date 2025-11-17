////////////////////////////////////////////////////////////////////////////////
// tests/event_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <entities/event.h>

#include <lox/object.h>

#include <comm.h>

TestGroup event_tests;

static int test_act_event()
{
    // Mock up a room with speaker and listener
    Room* room = mock_room(53000, NULL, NULL);

    Mobile* speaker = mock_mob("Speaker", 53001, NULL);
    transfer_mob(speaker, room);

    Mobile* listener = mock_mob("Listener", 53002, NULL);
    transfer_mob(listener, room);
   
    // Attach an 'on_act' event to the listener.
    const char* event_src =  "on_act(actor, msg) { print \"Heard: \" + msg; }";

    ObjClass* listener_class = create_entity_class((Entity*)listener, 
        "mob_53002", event_src);
    listener->header.klass = listener_class;

    Event* act_event = new_event();
    act_event->trigger = TRIG_ACT;
    act_event->method_name = lox_string("on_act");
    act_event->criteria = OBJ_VAL(lox_string("hello"));
    add_event((Entity*)listener, act_event);

    // Have the speaker 'act', and verify the listener's event was triggered.
    act("$n waves hello.", speaker, NULL, listener, TO_VICT);

    char* expected = "Heard: Speaker waves hello.\n\r\n";

    ASSERT_OUTPUT_EQ(expected);

    // If a test fails, and you want to know where, uncomment the code below.
    char* out = AS_STRING(test_output_buffer)->chars;
    size_t len = AS_STRING(test_output_buffer)->length;
    for (size_t i = 0; i < len; i++)
        if (out[i] != expected[i]) {
            printf("First diff @ character %d: '%c' (%02x) != '%c' (%02x)\n", (int)i, out[i], (int)out[i], expected[i], (int)expected[i]);
            printf("%s\n", &out[i]);
            break;
        }

    test_output_buffer = NIL_VAL;

    return 0;
}

void register_event_tests()
{
#define REGISTER(n, f)  register_test(&event_tests, (n), (f))

    init_test_group(&event_tests, "EVENT TESTS");
    register_test_group(&event_tests);

    REGISTER("TRIG_ACT: Mob->Mob", test_act_event);

#undef REGISTER
}