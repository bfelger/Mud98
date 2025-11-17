////////////////////////////////////////////////////////////////////////////////
// tests/event_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <entities/event.h>

#include <lox/object.h>

#include <act_obj.h>
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
    //char* out = AS_STRING(test_output_buffer)->chars;
    //size_t len = AS_STRING(test_output_buffer)->length;
    //for (size_t i = 0; i < len; i++)
    //    if (out[i] != expected[i]) {
    //        printf("First diff @ character %d: '%c' (%02x) != '%c' (%02x)\n", (int)i, out[i], (int)out[i], expected[i], (int)expected[i]);
    //        printf("%s\n", &out[i]);
    //        break;
    //    }

    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_bribe_event()
{
    // Mock up a room with mob and player
    Room* room = mock_room(54000, NULL, NULL);

    Mobile* mob = mock_mob("BribeMob", 54001, NULL);
    transfer_mob(mob, room);

    Mobile* pc = mock_player("Briber");
    pc->gold = 1000;
    transfer_mob(pc, room);

    // Attach an 'on_bribe' event to the mob.
    const char* event_src =  "on_bribe(briber, amount) { "
                             "   print briber.name + \" bribed me \" + amount / 100 + \" gold!\"; "
                             "}";
    ObjClass* mob_class = create_entity_class((Entity*)mob, 
        "mob_54001", event_src);
    mob->header.klass = mob_class;

    Event* bribe_event = new_event();
    bribe_event->trigger = TRIG_BRIBE;
    bribe_event->method_name = lox_string("on_bribe");
    bribe_event->criteria = INT_VAL(50);
    add_event((Entity*)mob, bribe_event);

    // Have the player bribe the mob organically using `do_give`.
    interpret(pc, "give 100 gold to BribeMob");

    char* expected = "Briber bribed me 100 gold!\n";

    ASSERT_OUTPUT_EQ(expected);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_death_event()
{
    // Mock up a room with mob and player
    Room* room = mock_room(54000, NULL, NULL);

    Mobile* mob = mock_mob("Joe", 54001, NULL);
    transfer_mob(mob, room);

    // Give Bob a weapon to kill Joe with
    Mobile* pc = mock_player("Bob");
    pc->level = 49;
    pc->hitroll = 100;
    SKNUM sknum = skill_lookup("sword");
    pc->pcdata->learned[sknum] = 100;

    Object* sword = mock_sword("sword", 54002, 49, 30, 30);
    obj_to_char(sword, pc);
    equip_char(pc, sword, WEAR_WIELD);
    transfer_mob(pc, room);

    // Attach an 'on_death' event to the mob.
    const char* event_src = "on_death(killer) { "
        "   print killer.name + \" killed me!\"; "
        "}";
    ObjClass* mob_class = create_entity_class((Entity*)mob,
        "mob_54001", event_src);
    mob->header.klass = mob_class;

    Event* death_event = new_event();
    death_event->trigger = TRIG_DEATH;
    death_event->method_name = lox_string("on_death");
    add_event((Entity*)mob, death_event);

    // Have the player kill the mob organically using `kill`.
    interpret(pc, "kill Joe");

    char* expected = "Bob killed me!\n";

    ASSERT_OUTPUT_EQ(expected);

    test_output_buffer = NIL_VAL;
    return 0;
}

void register_event_tests()
{
#define REGISTER(n, f)  register_test(&event_tests, (n), (f))

    init_test_group(&event_tests, "EVENT TESTS");
    register_test_group(&event_tests);

    REGISTER("TRIG_ACT: Mob->Mob", test_act_event);
    REGISTER("TRIG_BRIBE: Mob->Mob", test_bribe_event);
    REGISTER("TRIG_DEATH: Mob->Mob", test_death_event);

#undef REGISTER
}