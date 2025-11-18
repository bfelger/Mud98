////////////////////////////////////////////////////////////////////////////////
// tests/event_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <entities/area.h>
#include <entities/event.h>

#include <lox/object.h>

#include <act_obj.h>
#include <comm.h>
#include <fight.h>

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
    const char* event_src =  
        "on_act(actor, msg) {"
        "    print \"Heard: \" + msg;"
        "}";

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
    const char* event_src =  
        "on_bribe(briber, amount) { "
        "   print \"${briber.name} bribed me ${amount/100} gold!\"; "
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
    const char* event_src = 
        "on_death(killer) {"
        "   print \"${killer.name} killed me!\"; "
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

static int test_entry_event()
{
    Room* south_room = mock_room(55000, NULL, NULL);
    Room* north_room = mock_room(55001, NULL, NULL);

    mock_room_connection(south_room, north_room, DIR_NORTH, true);

    Mobile* mob = mock_mob("Mover", 55002, NULL);
    transfer_mob(mob, south_room);

    // Attach an 'on_entry' event to the mob.
    const char* event_src = 
        "on_entry() {"
        "   print \"${this.name} has moved rooms!\";"
        "}";
    ObjClass* mob_class = create_entity_class((Entity*)mob,
        "mob_55002", event_src);
    mob->header.klass = mob_class;

    Event* entry_event = new_event();
    entry_event->trigger = TRIG_ENTRY;
    entry_event->method_name = lox_string("on_entry");
    entry_event->criteria = INT_VAL(100);   // 100 percent chance
    add_event((Entity*)mob, entry_event);

    interpret(mob, "north");

    char* expected = "Mover has moved rooms!\n";

    ASSERT_OUTPUT_EQ(expected);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_fight_event()
{
    // Create a room with an attacker and victim 
    Room* room = mock_room(56000, NULL, NULL);
    Mobile* attacker = mock_mob("Attacker", 56001, NULL);
    transfer_mob(attacker, room);
    Mobile* victim = mock_mob("Victim", 56002, NULL);
    victim->hit = 100;
    victim->max_hit = 100;
    transfer_mob(victim, room);

    // Arm the attacker with a very weak weapon so the first hit doesn't kill.
    Object* sword = mock_sword("sword", 56003, 1, 1, 1);
    obj_to_char(sword, attacker);
    equip_char(attacker, sword, WEAR_WIELD);

    // Attach an 'on_fight' event to the attacker.
    const char* event_src = 
        "on_fight(victim) {"
        "   print \"Attacking ${victim.name}!\";"
        "}";

    ObjClass* attacker_class = create_entity_class((Entity*)attacker,
        "mob_56001", event_src);
    attacker->header.klass = attacker_class;

    Event* fight_event = new_event();
    fight_event->trigger = TRIG_FIGHT;
    fight_event->method_name = lox_string("on_fight");
    fight_event->criteria = INT_VAL(100);   // 100 percent chance
    add_event((Entity*)attacker, fight_event);

    // Have the attacker attack the victim organically using `do_kill`.
    interpret(attacker, "kill Victim");

    // Run violence_update to process the fight round.
    violence_update();

    char* expected = "Attacking Victim!\n";
    ASSERT_OUTPUT_EQ(expected);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_give_event_vnum()
{
    // Mock up a room with mob and player
    Room* room = mock_room(57000, NULL, NULL);

    Mobile* mob = mock_mob("Receiver", 57001, NULL);
    transfer_mob(mob, room);

    Mobile* pc = mock_player("Giver");
    transfer_mob(pc, room);

    // Create an object to give
    Object* obj = mock_obj("SpecialItem", 57002, NULL);
    obj_to_char(obj, pc);

    // Attach an 'on_give' event to the mob.
    const char* event_src =  
        "on_give(giver, item) {"
        "   print \"${giver.name} gave me ${item.name}!\";"
        "}";
    ObjClass* mob_class = create_entity_class((Entity*)mob, 
        "mob_57001", event_src);
    mob->header.klass = mob_class;

    Event* give_event = new_event();
    give_event->trigger = TRIG_GIVE;
    give_event->method_name = lox_string("on_give");
    give_event->criteria = INT_VAL(57002); // VNUM of object to trigger on
    add_event((Entity*)mob, give_event);

    // Have the player give the object to the mob organically using `do_give`.
    interpret(pc, "give SpecialItem to Receiver");

    char* expected = "Giver gave me SpecialItem!\n";

    ASSERT_OUTPUT_EQ(expected); 

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_give_event_name()
{
    // Mock up a room with mob and player
    Room* room = mock_room(57000, NULL, NULL);

    Mobile* mob = mock_mob("Receiver", 57001, NULL);
    transfer_mob(mob, room);

    Mobile* pc = mock_player("Giver");
    transfer_mob(pc, room);

    // Create an object to give
    Object* obj = mock_obj("SpecialItem", 57002, NULL);
    obj_to_char(obj, pc);

    // Attach an 'on_give' event to the mob.
    const char* event_src =  
        "on_give(giver, item) {"
        "   print \"${giver.name} gave me ${item.name}!\" "
        "}";
    ObjClass* mob_class = create_entity_class((Entity*)mob, 
        "mob_57001", event_src);
    mob->header.klass = mob_class;

    Event* give_event = new_event();
    give_event->trigger = TRIG_GIVE;
    give_event->method_name = lox_string("on_give");
    give_event->criteria = OBJ_VAL(lox_string("SpecialItem")); // Name of object to trigger on
    add_event((Entity*)mob, give_event);

    // Have the player give the object to the mob organically using `do_give`.
    interpret(pc, "give SpecialItem to Receiver");

    char* expected = "Giver gave me SpecialItem!\n";

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
    REGISTER("TRIG_ENTRY: Mob", test_entry_event);
    REGISTER("TRIG_FIGHT: Mob->Mob", test_fight_event);
    REGISTER("TRIG_GIVE: Mob->Mob: Obj by VNUM", test_give_event_vnum);
    REGISTER("TRIG_GIVE: Mob->Mob: Obj by Name", test_give_event_name);

#undef REGISTER
}