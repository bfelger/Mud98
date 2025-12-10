////////////////////////////////////////////////////////////////////////////////
// act_tests.c
//
// Tests for act_*.c command functions and act() system.
//
// CRITICAL PATTERN: All entities (players, mobs, objects) MUST be placed in
// rooms via transfer_mob() or obj_to_* functions before executing commands.
// Commands assume world structure exists and will segfault without it.
//
// Standard command test pattern:
//   1. Create room: Room* room = mock_room(vnum, NULL, NULL);
//   2. Create entity: Mobile* ch = mock_player("Name");
//   3. Place in world: transfer_mob(ch, room);
//   4. Enable output capture: test_socket_output_enabled = true;
//   5. Execute command: do_command(ch, "args");
//   6. Disable capture: test_socket_output_enabled = false;
//   7. Assert output: ASSERT_OUTPUT_CONTAINS("expected");
//   8. Reset buffer: test_output_buffer = NIL_VAL;
//
// NOTE: send_to_char() and act() use different output paths. Most commands
// use send_to_char(); act() is for third-person messages.
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "lox_tests.h"
#include "mock.h"

#include <act_info.h>
#include <comm.h>
#include <handler.h>

#include <data/mobile_data.h>
#include <entities/room.h>

TestGroup act_tests;

static int test_to_vch()
{
    Room* room = mock_room(50000, NULL, NULL);

    Mobile* ch = mock_mob("Bob", 50001, NULL);
    transfer_mob(ch, room);

    Mobile* pc = mock_player("Jim");
    transfer_mob(pc, room);

    test_socket_output_enabled = true;

    const char* msg = "$n waves at you.";
    act(msg, ch, NULL, pc, TO_VICT);

    test_socket_output_enabled = false;

    ASSERT_OUTPUT_EQ("Bob waves at you.\n\r");
    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_to_room()
{
    Room* room = mock_room(50000, NULL, NULL);

    Mobile* ch = mock_mob("Bob", 50001, NULL);
    transfer_mob(ch, room);

    Mobile* vch = mock_mob("Alice", 50002, NULL);
    vch->sex = SEX_FEMALE;
    transfer_mob(vch, room);

    Mobile* pc = mock_player("Jim");
    transfer_mob(pc, room);

    test_socket_output_enabled = true;

    const char* msg = "$n looks at $N and tries to steal $S lunch. But $E is "
        "having none of $s shenanigans and bops $m. $n glowers at $M.";
    act(msg, ch, room, vch, TO_ROOM);

    test_socket_output_enabled = false;

    ASSERT_OUTPUT_EQ("Bob looks at Alice and tries to steal her lunch. But "
        "she is having none of his shenanigans and bops him. Bob glowers at "
        "her.\n\r");
    test_output_buffer = NIL_VAL;

    return 0;
}

// =============================================================================
// Command Tests (act_info.c)
// =============================================================================

static int test_autoassist_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    // Test enabling
    REMOVE_BIT(ch->act_flags, PLR_AUTOASSIST);
    test_socket_output_enabled = true;
    do_autoassist(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->act_flags, PLR_AUTOASSIST));
    ASSERT_OUTPUT_CONTAINS("You will now assist when needed");
    test_output_buffer = NIL_VAL;
    
    // Test disabling
    test_socket_output_enabled = true;
    do_autoassist(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_SET(ch->act_flags, PLR_AUTOASSIST));
    ASSERT_OUTPUT_CONTAINS("Autoassist removed");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_autoexit_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->act_flags, PLR_AUTOEXIT);
    test_socket_output_enabled = true;
    do_autoexit(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->act_flags, PLR_AUTOEXIT));
    ASSERT_OUTPUT_CONTAINS("Exits will now be displayed");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_autogold_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->act_flags, PLR_AUTOGOLD);
    test_socket_output_enabled = true;
    do_autogold(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->act_flags, PLR_AUTOGOLD));
    ASSERT_OUTPUT_CONTAINS("Automatic gold looting set");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_autoloot_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->act_flags, PLR_AUTOLOOT);
    test_socket_output_enabled = true;
    do_autoloot(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->act_flags, PLR_AUTOLOOT));
    ASSERT_OUTPUT_CONTAINS("Automatic corpse looting set");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_autosac_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->act_flags, PLR_AUTOSAC);
    test_socket_output_enabled = true;
    do_autosac(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->act_flags, PLR_AUTOSAC));
    ASSERT_OUTPUT_CONTAINS("Automatic corpse sacrificing set");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_autosplit_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->act_flags, PLR_AUTOSPLIT);
    test_socket_output_enabled = true;
    do_autosplit(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->act_flags, PLR_AUTOSPLIT));
    ASSERT_OUTPUT_CONTAINS("Automatic gold splitting set");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_brief_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->comm_flags, COMM_BRIEF);
    test_socket_output_enabled = true;
    do_brief(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->comm_flags, COMM_BRIEF));
    ASSERT_OUTPUT_CONTAINS("Short descriptions activated");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_compact_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->comm_flags, COMM_COMPACT);
    test_socket_output_enabled = true;
    do_compact(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->comm_flags, COMM_COMPACT));
    ASSERT_OUTPUT_CONTAINS("Compact mode set");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_inventory_empty()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_inventory(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You are carrying:");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_inventory_with_items()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    Object* sword = mock_sword("a sharp sword", 1001, 10, 2, 4);
    obj_to_char(sword, ch);
    
    test_socket_output_enabled = true;
    do_inventory(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You are carrying:");
    ASSERT_OUTPUT_CONTAINS("sword");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_equipment_nothing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_equipment(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You are using:");
    ASSERT_OUTPUT_CONTAINS("Nothing");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_worth_player()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    ch->gold = 5;
    ch->silver = 50;
    ch->copper = 25;
    ch->exp = 1000;
    ch->level = 5;
    
    test_socket_output_enabled = true;
    do_worth(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You have");
    ASSERT_OUTPUT_CONTAINS("gold");
    ASSERT_OUTPUT_CONTAINS("experience");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_worth_npc()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* npc = mock_mob("TestMob", 1001, NULL);
    transfer_mob(npc, room);
    npc->gold = 10;
    npc->silver = 0;
    npc->copper = 0;
    npc->desc = mock_descriptor();
    
    test_socket_output_enabled = true;
    do_worth(npc, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("You have");
    ASSERT_OUTPUT_CONTAINS("gold");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_time()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_time(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("o'clock");
    ASSERT_OUTPUT_CONTAINS("Day of");
    ASSERT_OUTPUT_CONTAINS("Mud98 started up at");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_weather_indoors()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    // Ensure room is indoors
    SET_BIT(room->data->room_flags, ROOM_INDOORS);
    room->data->sector_type = SECT_INSIDE;
    
    test_socket_output_enabled = true;
    do_weather(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("can't see the weather indoors");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_exits_none()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_exits(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Obvious exits:");
    ASSERT_OUTPUT_CONTAINS("None");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_exits_with_connections()
{
    Room* room1 = mock_room(50001, NULL, NULL);
    Room* room2 = mock_room(50002, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    
    mock_room_connection(room1, room2, DIR_NORTH, false);
    transfer_mob(ch, room1);
    
    test_socket_output_enabled = true;
    do_exits(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Obvious exits:");
    ASSERT_OUTPUT_CONTAINS("North");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_exits_auto()
{
    Room* room1 = mock_room(50001, NULL, NULL);
    Room* room2 = mock_room(50002, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    
    mock_room_connection(room1, room2, DIR_EAST, false);
    transfer_mob(ch, room1);
    
    test_socket_output_enabled = true;
    do_exits(ch, "auto");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("[Exits:");
    ASSERT_OUTPUT_CONTAINS("east");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_combine_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->comm_flags, COMM_COMBINE);
    test_socket_output_enabled = true;
    do_combine(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->comm_flags, COMM_COMBINE));
    ASSERT_OUTPUT_CONTAINS("Combined inventory selected");
    test_output_buffer = NIL_VAL;
    
    test_socket_output_enabled = true;
    do_combine(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_SET(ch->comm_flags, COMM_COMBINE));
    ASSERT_OUTPUT_CONTAINS("Long inventory selected");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_noloot_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->act_flags, PLR_CANLOOT);
    test_socket_output_enabled = true;
    do_noloot(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->act_flags, PLR_CANLOOT));
    ASSERT_OUTPUT_CONTAINS("Your corpse may now be looted");
    test_output_buffer = NIL_VAL;
    
    test_socket_output_enabled = true;
    do_noloot(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_SET(ch->act_flags, PLR_CANLOOT));
    ASSERT_OUTPUT_CONTAINS("Your corpse is now safe from thieves");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_nofollow_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->act_flags, PLR_NOFOLLOW);
    test_socket_output_enabled = true;
    do_nofollow(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->act_flags, PLR_NOFOLLOW));
    ASSERT_OUTPUT_CONTAINS("You no longer accept followers");
    test_output_buffer = NIL_VAL;
    
    test_socket_output_enabled = true;
    do_nofollow(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_SET(ch->act_flags, PLR_NOFOLLOW));
    ASSERT_OUTPUT_CONTAINS("You now accept followers");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_nosummon_toggle()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    REMOVE_BIT(ch->act_flags, PLR_NOSUMMON);
    test_socket_output_enabled = true;
    do_nosummon(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(IS_SET(ch->act_flags, PLR_NOSUMMON));
    ASSERT_OUTPUT_CONTAINS("You are now immune to summoning");
    test_output_buffer = NIL_VAL;
    
    test_socket_output_enabled = true;
    do_nosummon(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_SET(ch->act_flags, PLR_NOSUMMON));
    ASSERT_OUTPUT_CONTAINS("You are no longer immune to summon");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_autolist()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    SET_BIT(ch->act_flags, PLR_AUTOASSIST);
    REMOVE_BIT(ch->act_flags, PLR_AUTOEXIT);
    
    test_socket_output_enabled = true;
    do_autolist(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("action");
    ASSERT_OUTPUT_CONTAINS("status");
    ASSERT_OUTPUT_CONTAINS("autoassist");
    ASSERT_OUTPUT_CONTAINS("autoexit");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_report()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    ch->hit = 100;
    ch->max_hit = 150;
    ch->mana = 50;
    ch->max_mana = 100;
    ch->move = 75;
    ch->max_move = 120;
    ch->exp = 1000;
    
    test_socket_output_enabled = true;
    do_report(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("100/150 hp");
    ASSERT_OUTPUT_CONTAINS("50/100 mana");
    ASSERT_OUTPUT_CONTAINS("75/120 mv");
    ASSERT_OUTPUT_CONTAINS("1000 xp");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_wimpy_default()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    ch->max_hit = 100;
    ch->wimpy = 0;
    
    test_socket_output_enabled = true;
    do_wimpy(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(ch->wimpy == 20);  // max_hit / 5
    ASSERT_OUTPUT_CONTAINS("Wimpy set to 20 hit points");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_wimpy_explicit()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    ch->max_hit = 100;
    ch->wimpy = 0;
    
    test_socket_output_enabled = true;
    do_wimpy(ch, "30");
    test_socket_output_enabled = false;
    
    ASSERT(ch->wimpy == 30);
    ASSERT_OUTPUT_CONTAINS("Wimpy set to 30 hit points");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_wimpy_too_high()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    ch->max_hit = 100;
    ch->wimpy = 0;
    
    test_socket_output_enabled = true;
    do_wimpy(ch, "60");
    test_socket_output_enabled = false;
    
    ASSERT(ch->wimpy == 0);  // Should not change
    ASSERT_OUTPUT_CONTAINS("Such cowardice ill becomes you");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_compare_same_object()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    Object* sword = mock_sword("sword", 1, 1, 2, 4);
    transfer_mob(ch, room);
    obj_to_char(sword, ch);
    
    test_socket_output_enabled = true;
    do_compare(ch, "sword sword");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("compare");
    ASSERT_OUTPUT_CONTAINS("itself");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_compare_weapons()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    Object* sword1 = mock_sword("sword1", 1, 1, 2, 4);
    Object* sword2 = mock_sword("sword2", 2, 1, 2, 4);
    transfer_mob(ch, room);
    obj_to_char(sword1, ch);
    obj_to_char(sword2, ch);
    
    // Make sword1 better
    sword1->value[1] = 5;  // num dice
    sword1->value[2] = 10; // size dice
    sword2->value[1] = 2;
    sword2->value[2] = 4;
    
    // IMPORTANT: Keywords in object->header.name must match what get_obj_carry()
    // searches for. mock_sword() sets header.name from the name parameter, so
    // we use "sword1" and "sword2" here to match the object keywords.
    test_socket_output_enabled = true;
    do_compare(ch, "sword1 sword2");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("looks better");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_consider_easy()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    MobPrototype* mp = mock_mob_proto(1);
    Mobile* victim = mock_mob("TestMob", 1, mp);
    transfer_mob(ch, room);
    transfer_mob(victim, room);
    
    ch->level = 20;
    victim->level = 10;
    
    test_socket_output_enabled = true;
    do_consider(ch, "TestMob");
    test_socket_output_enabled = false;
    
    // do_consider calculates diff = victim->level - ch->level
    // Diff of -10 triggers: "You can kill $N naked and weaponless."
    ASSERT_OUTPUT_CONTAINS("naked and weaponless");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_consider_hard()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    MobPrototype* mp = mock_mob_proto(1);
    Mobile* victim = mock_mob("TestMob", 1, mp);
    transfer_mob(ch, room);
    transfer_mob(victim, room);
    
    ch->level = 10;
    victim->level = 20;
    
    test_socket_output_enabled = true;
    do_consider(ch, "TestMob");
    test_socket_output_enabled = false;
    
    // Diff of 10 or more triggers: "Death will thank you for your gift."
    ASSERT_OUTPUT_CONTAINS("Death will thank you");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_credits()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    // do_credits just calls do_help with "diku"
    // We can't easily test the help system, but we can ensure no crash
    test_socket_output_enabled = true;
    do_credits(ch, "");
    test_socket_output_enabled = false;
    
    // Just verify it didn't crash
    test_output_buffer = NIL_VAL;
    
    return 0;
}

void register_act_tests()
{
#define REGISTER(n, f)  register_test(&act_tests, (n), (f))

    init_test_group(&act_tests, "ACT TESTS");
    register_test_group(&act_tests);

    REGISTER("Act: To Victim", test_to_vch);
    REGISTER("Act: To Room", test_to_room);
    
    // Command Tests
    REGISTER("Cmd: Autoassist toggle", test_autoassist_toggle);
    REGISTER("Cmd: Autoexit toggle", test_autoexit_toggle);
    REGISTER("Cmd: Autogold toggle", test_autogold_toggle);
    REGISTER("Cmd: Autoloot toggle", test_autoloot_toggle);
    REGISTER("Cmd: Autosac toggle", test_autosac_toggle);
    REGISTER("Cmd: Autosplit toggle", test_autosplit_toggle);
    REGISTER("Cmd: Brief toggle", test_brief_toggle);
    REGISTER("Cmd: Compact toggle", test_compact_toggle);
    REGISTER("Cmd: Inventory empty", test_inventory_empty);
    REGISTER("Cmd: Inventory with items", test_inventory_with_items);
    REGISTER("Cmd: Equipment nothing", test_equipment_nothing);
    REGISTER("Cmd: Worth player", test_worth_player);
    REGISTER("Cmd: Worth NPC", test_worth_npc);
    REGISTER("Cmd: Time", test_time);
    REGISTER("Cmd: Weather indoors", test_weather_indoors);
    REGISTER("Cmd: Exits none", test_exits_none);
    REGISTER("Cmd: Exits with connections", test_exits_with_connections);
    REGISTER("Cmd: Exits auto format", test_exits_auto);
    REGISTER("Cmd: Combine toggle", test_combine_toggle);
    REGISTER("Cmd: Noloot toggle", test_noloot_toggle);
    REGISTER("Cmd: Nofollow toggle", test_nofollow_toggle);
    REGISTER("Cmd: Nosummon toggle", test_nosummon_toggle);
    REGISTER("Cmd: Autolist", test_autolist);
    REGISTER("Cmd: Report", test_report);
    REGISTER("Cmd: Wimpy default", test_wimpy_default);
    REGISTER("Cmd: Wimpy explicit", test_wimpy_explicit);
    REGISTER("Cmd: Wimpy too high", test_wimpy_too_high);
    REGISTER("Cmd: Compare same object", test_compare_same_object);
    REGISTER("Cmd: Compare weapons", test_compare_weapons);
    REGISTER("Cmd: Consider easy", test_consider_easy);
    REGISTER("Cmd: Consider hard", test_consider_hard);
    REGISTER("Cmd: Credits", test_credits);

#undef REGISTER
}
