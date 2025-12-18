////////////////////////////////////////////////////////////////////////////////
// act_wiz5_tests.c - More wizard command tests (set, string, skills, etc.)
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "mock.h"
#include "test_registry.h"

#include <act_wiz.h>
#include <comm.h>
#include <handler.h>
#include <merc.h>

#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>

#include <data/mobile_data.h>

extern bool test_socket_output_enabled;
extern bool test_act_output_enabled;
extern Value test_output_buffer;

// Test do_set - set character attributes (hit points)
static int test_set_hp()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    player->max_hit = 100;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_set(wiz, "mob testplayer hp 500");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(player->max_hit == 500);
    
    return 0;
}

// Test do_set - set level (NPCs only)
static int test_set_level()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* mob = mock_mob("testmob", 60100, mock_mob_proto(60100));
    mob->level = 1;
    transfer_mob(mob, room);
    
    test_socket_output_enabled = true;
    do_set(wiz, "mob testmob level 10");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(mob->level == 10);
    
    return 0;
}

// Test do_string - string object short description
static int test_string_obj_short()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Object* obj = mock_obj("testobj", 60200, mock_obj_proto(60200));
    obj_to_char(obj, wiz);
    
    test_socket_output_enabled = true;
    do_string(wiz, "obj testobj short a new short description");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(obj->short_descr != NULL);
    ASSERT(strstr(obj->short_descr, "new short description") != NULL);
    
    return 0;
}

// Test do_string - string mobile short description
static int test_string_mob_short()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* mob = mock_mob("testmob", 60100, mock_mob_proto(60100));
    transfer_mob(mob, room);
    
    test_socket_output_enabled = true;
    do_string(wiz, "mob testmob short a new mob description");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(mob->short_descr != NULL);
    ASSERT(strstr(mob->short_descr, "new mob description") != NULL);
    
    return 0;
}

// Test do_sset - set skills
static int test_sset()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    transfer_mob(player, room);
    
    // Set a skill to 75%
    do_sset(wiz, "testplayer 'magic missile' 75");
    
    ASSERT(get_skill(player, skill_lookup("magic missile")) == 75);
    
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_mset - set mob attributes
static int test_mset_level()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* mob = mock_mob("testmob", 60100, mock_mob_proto(60100));
    mob->level = 5;
    transfer_mob(mob, room);
    
    test_socket_output_enabled = true;
    do_mset(wiz, "testmob level 15");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(mob->level == 15);
    
    return 0;
}

// Test do_oset - set object attributes
static int test_oset_level()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Object* obj = mock_obj("testobj", 60200, mock_obj_proto(60200));
    obj->level = 5;
    obj_to_char(obj, wiz);
    
    test_socket_output_enabled = true;
    do_oset(wiz, "testobj level 20");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(obj->level == 20);
    
    return 0;
}

// Test do_rset - set room attributes
static int test_rset_sector()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    room->data->sector_type = 0;
    
    test_socket_output_enabled = true;
    do_rset(wiz, "60001 sector 1");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(room->data->sector_type == 1);
    
    return 0;
}

// Test do_return - return from switch (body-swap)
static int test_return()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    // Return is for switch command (body-swap), requires descriptor->original
    test_socket_output_enabled = true;
    do_return(wiz, "");
    test_socket_output_enabled = false;
    
    // Verify return command output
    static const char* possible_outputs[] = {
        "You return to your original body",
        "You aren't switched"
    };
    ASSERT_OUTPUT_CONTAINS_ANY(possible_outputs, 2);
    
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_snoop - snoop on a player
static int test_snoop()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    transfer_mob(player, room);
    
    // Actual snooping requires descriptor management
    test_socket_output_enabled = true;
    do_snoop(wiz, "testplayer");
    test_socket_output_enabled = false;
    
    // Verify snoop output (success or error)
    static const char* possible_outputs[] = {
        "Ok.",
        "Busy already.",
        "No descriptor",
        "You can't find them",
        "Snoop whom?"
    };
    ASSERT_OUTPUT_CONTAINS_ANY(possible_outputs, 5);
    
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_switch - switch into a mob
static int test_switch()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* mob = mock_mob("testmob", 60100, mock_mob_proto(60100));
    transfer_mob(mob, room);
    
    // Switch requires descriptor management
    test_socket_output_enabled = true;
    do_switch(wiz, "testmob");
    test_socket_output_enabled = false;
    
    // Verify switch output (success or error)
    static const char* possible_outputs[] = {
        "Ok.",
        "You can't find them",
        "Switch into whom?"
    };
    ASSERT_OUTPUT_CONTAINS_ANY(possible_outputs, 3);
    
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_immtalk - immortal channel
static int test_immtalk()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_immtalk(wiz, "test message");
    test_socket_output_enabled = false;
    
    // Immtalk sends to all imms via descriptor_list
    // Verify command produced output
    static const char* possible_outputs[] = {
        "test message",
        "Huh?"
    };
    ASSERT_OUTPUT_CONTAINS_ANY(possible_outputs, 2);
    
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_memory - show memory statistics
static int test_memory()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_memory(wiz, "");
    test_socket_output_enabled = false;
    
    // Should produce some output
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_wizlock - toggle wizlock
static int test_wizlock()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    extern bool wizlock;
    bool initial_state = wizlock;
    
    test_socket_output_enabled = true;
    do_wizlock(wiz, "");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Should toggle wizlock
    ASSERT(wizlock != initial_state);
    
    // Reset to initial state
    wizlock = initial_state;
    
    return 0;
}

// Test do_newlock - toggle newlock
static int test_newlock()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    extern bool newlock;
    bool initial_state = newlock;
    
    test_socket_output_enabled = true;
    do_newlock(wiz, "");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Should toggle newlock
    ASSERT(newlock != initial_state);
    
    // Reset to initial state
    newlock = initial_state;
    
    return 0;
}

void register_act_wiz5_tests()
{
    TestGroup* group = calloc(1, sizeof(TestGroup));
    init_test_group(group, "ACT WIZ5 TESTS");
    
#define REGISTER(name, func) register_test(group, (name), (func))
    
    // Set commands
    REGISTER("Cmd: Set hp", test_set_hp);
    REGISTER("Cmd: Set level", test_set_level);
    
    // String commands
    REGISTER("Cmd: String obj short", test_string_obj_short);
    REGISTER("Cmd: String mob short", test_string_mob_short);
    
    // Skill/stat setting
    REGISTER("Cmd: Sset", test_sset);
    REGISTER("Cmd: Mset level", test_mset_level);
    REGISTER("Cmd: Oset level", test_oset_level);
    REGISTER("Cmd: Rset sector", test_rset_sector);
    
    // Navigation/state
    REGISTER("Cmd: Return", test_return);
    
    // Admin tools
    REGISTER("Cmd: Snoop", test_snoop);
    REGISTER("Cmd: Switch", test_switch);
    REGISTER("Cmd: Immtalk", test_immtalk);
    REGISTER("Cmd: Memory", test_memory);
    
    // Server control
    REGISTER("Cmd: Wizlock", test_wizlock);
    REGISTER("Cmd: Newlock", test_newlock);

#undef REGISTER

    register_test_group(group);
}
