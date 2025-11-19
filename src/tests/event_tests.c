////////////////////////////////////////////////////////////////////////////////
// tests/event_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"
#include "lox_tests.h"

#include <entities/area.h>
#include <entities/event.h>

#include <lox/object.h>

#include <act_obj.h>
#include <comm.h>
#include <fight.h>
#include <update.h>

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

static int test_attacked_event()
{
    Room* room = mock_room(59000, NULL, NULL);

    Mobile* victim = mock_mob("Victim", 59001, NULL);
    victim->hit = 100;
    victim->max_hit = 100;
    transfer_mob(victim, room);

    Mobile* attacker = mock_mob("Attacker", 59002, NULL);
    transfer_mob(attacker, room);

    // Arm the attacker with a very weak weapon so the first hit doesn't kill.
    Object* sword = mock_sword("sword", 59003, 1, 1, 1);
    obj_to_char(sword, attacker);
    equip_char(attacker, sword, WEAR_WIELD);

    // Attach an 'on_attacked' event to the victim.
    const char* event_src =
        "on_attacked(attacker) {"
        "   print \"${attacker.name} is attacking me!\";"
        "}";
    ObjClass* victim_class = create_entity_class((Entity*)victim,
        "mob_59001", event_src);
    victim->header.klass = victim_class;
    init_entity_class((Entity*)victim);

    Event* attacked_event = new_event();
    attacked_event->trigger = TRIG_ATTACKED;
    attacked_event->method_name = lox_string("on_attacked");
    attacked_event->criteria = INT_VAL(100);   // 100 percent chance
    add_event((Entity*)victim, attacked_event);

    // Simulate an attack
    interpret(attacker, "kill Victim");

    char* expected = "Attacker is attacking me!\n";

    ASSERT_OUTPUT_EQ(expected);

    extract_char(attacker, true);
    extract_char(victim, true);

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
    init_entity_class((Entity*)mob);

    Event* bribe_event = new_event();
    bribe_event->trigger = TRIG_BRIBE;
    bribe_event->method_name = lox_string("on_bribe");
    bribe_event->criteria = INT_VAL(50);
    add_event((Entity*)mob, bribe_event);

    // Have the player bribe the mob organically using `do_give`.
    interpret(pc, "give 100 gold to BribeMob");

    char* expected = "Briber bribed me 100 gold!\n";

    ASSERT_OUTPUT_EQ(expected);

    extract_char(pc, true);
    extract_char(mob, true);

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
    init_entity_class((Entity*)mob);

    Event* death_event = new_event();
    death_event->trigger = TRIG_DEATH;
    death_event->method_name = lox_string("on_death");
    add_event((Entity*)mob, death_event);

    // Have the player kill the mob organically using `kill`.
    interpret(pc, "kill Joe");

    char* expected = "Bob killed me!\n";

    ASSERT_OUTPUT_EQ(expected);

    extract_char(pc, true);
    extract_char(mob, true);

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
    init_entity_class((Entity*)mob);

    Event* entry_event = new_event();
    entry_event->trigger = TRIG_ENTRY;
    entry_event->method_name = lox_string("on_entry");
    entry_event->criteria = INT_VAL(100);   // 100 percent chance
    add_event((Entity*)mob, entry_event);

    interpret(mob, "north");

    char* expected = "Mover has moved rooms!\n";

    ASSERT_OUTPUT_EQ(expected);

    extract_char(mob, true);

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
    init_entity_class((Entity*)attacker);

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

    extract_char(attacker, true);
    extract_char(victim, true);

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
    init_entity_class((Entity*)mob);

    Event* give_event = new_event();
    give_event->trigger = TRIG_GIVE;
    give_event->method_name = lox_string("on_give");
    give_event->criteria = INT_VAL(57002); // VNUM of object to trigger on
    add_event((Entity*)mob, give_event);

    // Have the player give the object to the mob organically using `do_give`.
    interpret(pc, "give SpecialItem to Receiver");

    char* expected = "Giver gave me SpecialItem!\n";

    ASSERT_OUTPUT_EQ(expected); 

    extract_char(pc, true);
    extract_char(mob, true);

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
    init_entity_class((Entity*)mob);

    Event* give_event = new_event();
    give_event->trigger = TRIG_GIVE;
    give_event->method_name = lox_string("on_give");
    give_event->criteria = OBJ_VAL(lox_string("SpecialItem")); // Name of object to trigger on
    add_event((Entity*)mob, give_event);

    // Have the player give the object to the mob organically using `do_give`.
    interpret(pc, "give SpecialItem to Receiver");

    char* expected = "Giver gave me SpecialItem!\n";

    ASSERT_OUTPUT_EQ(expected); 

    extract_char(pc, true);
    extract_char(mob, true);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_greet_event_on_room()
{
    // TRIG_GREET can be applied to a room as well as a mob.
    Room* south_room = mock_room(58000, NULL, NULL);
    Room* north_room = mock_room(58001, NULL, NULL);
    mock_room_connection(south_room, north_room, DIR_NORTH, true);

    Mobile* pc = mock_player("Entrant");
    transfer_mob(pc, south_room);

    // Attach an 'on_greet' event to the north room.
    const char* event_src =  
        "on_greet(ch) {"
        "   print \"${ch.name} has entered the room!\";"
        "}";
    ObjClass* room_class = create_entity_class((Entity*)north_room, 
        "room_58001", event_src);
    north_room->header.klass = room_class;
    init_entity_class((Entity*)north_room);

    Event* greet_event = new_event();
    greet_event->trigger = TRIG_GREET;
    greet_event->method_name = lox_string("on_greet");
    add_event((Entity*)north_room, greet_event);

    // Have the player move north into the room.
    interpret(pc, "north");

    char* expected = "Entrant has entered the room!\n";

    ASSERT_OUTPUT_EQ(expected);

    extract_char(pc, true);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_greet_event_on_mob()
{
    // TRIG_GREET can be applied to a room as well as a mob.
    Room* south_room = mock_room(58000, NULL, NULL);
    Room* north_room = mock_room(58001, NULL, NULL);
    mock_room_connection(south_room, north_room, DIR_NORTH, true);

    Mobile* greeter = mock_mob("Greeter", 58002, NULL);
    transfer_mob(greeter, north_room);

    Mobile* pc = mock_player("Entrant");
    transfer_mob(pc, south_room);

    // Attach an 'on_greet' event to the greeter mob.
    const char* event_src =  
        "on_greet(ch) {"
        "   print \"${ch.name}, welcome to the room!\";"
        "}";
    ObjClass* mob_class = create_entity_class((Entity*)greeter, 
        "mob_58002", event_src);
    greeter->header.klass = mob_class;
    init_entity_class((Entity*)greeter);

    Event* greet_event = new_event();
    greet_event->trigger = TRIG_GREET;
    greet_event->method_name = lox_string("on_greet");
    add_event((Entity*)greeter, greet_event);

    // Have the player move north into the room.
    interpret(pc, "north");

    char* expected = "Entrant, welcome to the room!\n";

    ASSERT_OUTPUT_EQ(expected);

    extract_char(pc, true);
    extract_char(greeter, true);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_hpcnt_event()
{
        // Create a room with an attacker and victim 
    Room* room = mock_room(60000, NULL, NULL);

    Mobile* attacker = mock_mob("Attacker", 60001, NULL);
    attacker->level = 49;
    attacker->hitroll = 100;
    transfer_mob(attacker, room);

    Mobile* victim = mock_mob("Victim", 60002, NULL);
    victim->hit = 3;
    victim->max_hit = 3;
    transfer_mob(victim, room);

    // Arm the attacker with a very weak weapon so the first hit doesn't kill.
    Object* sword = mock_sword("sword", 60003, 1, 1, 1);
    obj_to_char(sword, attacker);
    equip_char(attacker, sword, WEAR_WIELD);

    // Attach an 'on_fight' event to the attacker.
    const char* event_src =
        "on_hpcnt(attacker) {"
        "   print \"${attacker.name} brought me to ${(this.hp*100)/100}% health!\";"
        "}";

    ObjClass* victim_class = create_entity_class((Entity*)victim,
        "mob_60002", event_src);
    victim->header.klass = victim_class;
    init_entity_class((Entity*)victim);

    Event* fight_event = new_event();
    fight_event->trigger = TRIG_HPCNT;
    fight_event->method_name = lox_string("on_hpcnt");
    fight_event->criteria = INT_VAL(80);   // 80 percent health
    add_event((Entity*)victim, fight_event);

    // Have the attacker attack the victim organically using `do_kill`.
    interpret(attacker, "kill Victim");

    // Run violence_update to process the fight round.
    violence_update();

    char* expected = "Attacker brought me to $N% health!\n";
    ASSERT_OUTPUT_MATCH(expected);

    extract_char(attacker, true);
    extract_char(victim, true);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_random_event()
{
    Room* room = mock_room(61000, NULL, NULL);

    Mobile* mob = mock_mob("Random", 61001, NULL);
    SET_BIT(mob->act_flags, ACT_UPDATE_ALWAYS);
    transfer_mob(mob, room);

    // Attach an 'on_random' event to the mob.
    const char* event_src =
        "on_random() {"
        "   print \"I am ${this.name}; I like to do ${dice(1,6)} random things a day.\";"
        "}";

    ObjClass* mob_class = create_entity_class((Entity*)mob,
        "mob_61001", event_src);
    mob->header.klass = mob_class;
    init_entity_class((Entity*)mob);

    Event* random_event = new_event();
    random_event->trigger = TRIG_RANDOM;
    random_event->method_name = lox_string("on_random");
    random_event->criteria = INT_VAL(100);   // 100 percent chance
    add_event((Entity*)mob, random_event);

    // Update all
    update_handler();

    char* expected = "I am Random; I like to do $N random things a day.\n";
    ASSERT_OUTPUT_MATCH(expected);

    extract_char(mob, true);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_speech_event()
{
    // Mock up a room with speaker and listener
    Room* room = mock_room(62000, NULL, NULL);

    Mobile* speaker = mock_player("Speaker");
    transfer_mob(speaker, room);

    Mobile* listener = mock_mob("Listener", 62002, NULL);
    transfer_mob(listener, room);

    // Attach an 'on_act' event to the listener.
    const char* event_src =
        "on_act(actor, msg) {"
        "    print \"Heard: ${msg}\""
        "}";

    ObjClass* listener_class = create_entity_class((Entity*)listener,
        "mob_62002", event_src);
    listener->header.klass = listener_class;
    init_entity_class((Entity*)listener);

    Event* act_event = new_event();
    act_event->trigger = TRIG_ACT;
    act_event->method_name = lox_string("on_act");
    act_event->criteria = OBJ_VAL(lox_string("hello"));
    add_event((Entity*)listener, act_event);

    interpret(speaker, "say hello there");

    char* expected = "Heard: ^6Speaker says '^7hello there^6'^x\n\r\n";

    ASSERT_OUTPUT_EQ(expected);

    // If a test fails, and you want to know where, uncomment the code below.
    //char* out = AS_STRING(test_output_buffer)->chars;
    //size_t len = AS_STRING(test_output_buffer)->length;
    //for (size_t i = 0; i < len; i++)
    //    if (out[i] != expected[i]) {
    //        printf("First diff @ character %d: '%c' (%02x) != '%c' (%02x)\n", (int)i, /*out[i]*/ ' ', (int)out[i], /*expected[i]*/ ' ', (int)expected[i]);
    //        printf("%s\n", &out[i]);
    //        break;
    //    }

    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_exit_event()
{
    // Put two players in a room with a northward exit. The room has an exit
    // event that will block one of the players from leaving.
    Room* south_room = mock_room(63000, NULL, NULL);
    Room* north_room = mock_room(63001, NULL, NULL);
    mock_room_connection(south_room, north_room, DIR_NORTH, true);

    Mobile* bob = mock_player("Bob");
    transfer_mob(bob, south_room);

    Mobile* alice = mock_player("Alice");
    transfer_mob(alice, south_room);

    // Attach an 'on_exit' event to the south room.
    const char* event_src =
        "on_exit_north(ch) {"
        "   if (ch.name == \"Bob\") {"
        "       print \"You cannot leave yet, Bob!\";"
        "       return true;"
        "   }"
        "   print \"You may leave, ${ch.name}.\";"
        "   return false;"
        "}";
    ObjClass* room_class = create_entity_class((Entity*)south_room,
        "room_63000", event_src);
    south_room->header.klass = room_class;
    init_entity_class((Entity*)south_room);

    Event* exit_event = new_event();
    exit_event->trigger = TRIG_EXIT;
    exit_event->method_name = lox_string("on_exit_north");
    exit_event->criteria = INT_VAL(DIR_NORTH);
    add_event((Entity*)south_room, exit_event);

    // Have Bob try to leave north.
    interpret(bob, "north");
    char* expected_bob = "You cannot leave yet, Bob!\n";
    ASSERT_OUTPUT_EQ(expected_bob);
    test_output_buffer = NIL_VAL;

    // Have Alice try to leave north.
    interpret(alice, "north");
    char* expected_alice = "You may leave, Alice.\n";
    ASSERT_OUTPUT_EQ(expected_alice);

    extract_char(bob, true);
    extract_char(alice, true);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_surrender_event()
{
    Room* room = mock_room(64000, NULL, NULL);

    Mobile* guard = mock_mob("Guard", 64001, NULL);
    guard->hit = 100;
    guard->max_hit = 100;
    transfer_mob(guard, room);

    Mobile* bob = mock_player("Bob");
    bob->hit = 100;
    bob->max_hit = 100;
    transfer_mob(bob, room);

    // Arm both with a very weak weapon so the first hit doesn't kill.
    Object* sword1 = mock_sword("sword", 60003, 1, 1, 1);
    obj_to_char(sword1, guard);
    equip_char(guard, sword1, WEAR_WIELD);

    Object* sword2 = mock_sword("sword", 60004, 1, 1, 1);
    obj_to_char(sword2, bob);
    equip_char(bob, sword2, WEAR_WIELD);

    // Attach an 'on_surr' event to the guard.
    const char* event_src =
        "init() {\n"
        //"   this.foes = [];\n"
        "   this.foe = nil;\n"
        "}\n"
        "on_surr(ch) {\n"
        //"   if (this.foes.contains(ch.name)) {\n"
        "   if (this.foe == ch.name) {\n"
        "       print \"I have already accepted your surrender once, ${ch.name}. Never again!\";\n"
        "       return false;\n"
        "   }\n"
        "   print \"You better keep your nose clean, ${ch.name}.\";\n"
        //"   this.foes.add(ch.name);\n"
        "   this.foe = ch.name;\n"
        "   return true;\n"
        "}";
    test_disassemble_on_test = true;
    ObjClass* guard_class = create_entity_class((Entity*)guard,
        "mob_64001", event_src);
    test_disassemble_on_test = false;
    guard->header.klass = guard_class;
    init_entity_class((Entity*)guard);

    Event* surr_event = new_event();
    surr_event->trigger = TRIG_SURR;
    surr_event->method_name = lox_string("on_surr");
    surr_event->criteria = INT_VAL(100);   // 100 percent chance
    add_event((Entity*)guard, surr_event);

    // Have Bob attack the guard organically using `do_kill`.
    interpret(bob, "kill Guard");
    violence_update();

    // Have Bob surrender.
    interpret(bob, "surrender");
    char* expected1 = "You better keep your nose clean, Bob.\n";
    ASSERT_OUTPUT_EQ(expected1);
    ASSERT(bob->fighting == NULL);
    test_output_buffer = NIL_VAL;

    // Have Bob attack the guard again.
    interpret(bob, "kill Guard");
    violence_update();

    // Have Bob surrender again.
    interpret(bob, "surrender");

    char* expected2 = "I have already accepted your surrender once, Bob. Never again!\n";
    ASSERT_OUTPUT_EQ(expected2);
    ASSERT(bob->fighting != NULL);

    extract_char(guard, true);
    extract_char(bob, true);

    test_output_buffer = NIL_VAL;
    return 0;
}

void register_event_tests()
{
#define REGISTER(n, f)  register_test(&event_tests, (n), (f))

    init_test_group(&event_tests, "EVENT TESTS");
    register_test_group(&event_tests);

    REGISTER("TRIG_ACT: Mob->Mob", test_act_event);
    REGISTER("TRIG_ATTACKED: Mob->Mob", test_attacked_event);
    REGISTER("TRIG_BRIBE: Mob->Mob", test_bribe_event);
    REGISTER("TRIG_DEATH: Mob->Mob", test_death_event);
    REGISTER("TRIG_ENTRY: Mob", test_entry_event);
    REGISTER("TRIG_FIGHT: Mob->Mob", test_fight_event);
    REGISTER("TRIG_GIVE: Mob->Mob: Obj by VNUM", test_give_event_vnum);
    REGISTER("TRIG_GIVE: Mob->Mob: Obj by Name", test_give_event_name);
    REGISTER("TRIG_GREET: Room->Mob", test_greet_event_on_room);
    REGISTER("TRIG_GREET: Mob->Mob", test_greet_event_on_mob);
    REGISTER("TRIG_HPCNT: Mob->Mob", test_hpcnt_event);
    REGISTER("TRIG_RANDOM: Mob", test_random_event);
    REGISTER("TRIG_SPEECH: Mob->Mob", test_speech_event);
    REGISTER("TRIG_EXIT: Room->Mob", test_exit_event);
    REGISTER("TRIG_SURR: Mob->Mob", test_surrender_event);

#undef REGISTER
}