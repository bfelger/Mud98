////////////////////////////////////////////////////////////////////////////////
// skills_tests.c - Skill learning and practice command tests (12 tests)
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "mock.h"
#include "test_registry.h"

#include <skills.h>
#include <handler.h>
#include <db.h>
#include <merc.h>

#include <entities/mobile.h>
#include <entities/room.h>

#include <data/mobile_data.h>
#include <data/player.h>

extern bool test_socket_output_enabled;
extern Value test_output_buffer;

// Test gain command - no trainer
static int test_gain_no_trainer()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_gain(player, "list");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test gain command - with trainer
static int test_gain_with_trainer()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* trainer = mock_mob("trainer", 60002, proto);
    SET_BIT(trainer->act_flags, ACT_GAIN);
    transfer_mob(trainer, room);
    
    test_socket_output_enabled = true;
    do_gain(player, "list");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test gain - empty argument with trainer
static int test_gain_empty_arg()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* trainer = mock_mob("trainer", 60002, proto);
    SET_BIT(trainer->act_flags, ACT_GAIN);
    transfer_mob(trainer, room);
    
    test_socket_output_enabled = true;
    do_gain(player, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test spells command - list spells
static int test_spells_list()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    player->level = 10;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_spells(player, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test spells command - by class
static int test_spells_by_class()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    player->level = 10;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_spells(player, "mage");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test skills command - list skills
static int test_skills_list()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    player->level = 10;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_skills(player, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test skills command - by class
static int test_skills_by_class()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    player->level = 10;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_skills(player, "warrior");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test groups command - list groups
static int test_groups_list()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    player->level = 10;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_groups(player, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test groups command - specific group
static int test_groups_specific()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    player->level = 10;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_groups(player, "attack");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test spells - NPC should return early
static int test_spells_npc()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* npc = mock_mob("testnpc", 60002, proto);
    transfer_mob(npc, room);
    
    test_socket_output_enabled = true;
    do_spells(npc, "");
    test_socket_output_enabled = false;
    
    // NPCs should not produce output
    ASSERT(IS_NIL(test_output_buffer));
    
    return 0;
}

// Test skills - NPC should return early
static int test_skills_npc()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* npc = mock_mob("testnpc", 60002, proto);
    transfer_mob(npc, room);
    
    test_socket_output_enabled = true;
    do_skills(npc, "");
    test_socket_output_enabled = false;
    
    // NPCs should not produce output
    ASSERT(IS_NIL(test_output_buffer));
    
    return 0;
}

// Test groups - NPC should return early
static int test_groups_npc()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* npc = mock_mob("testnpc", 60002, proto);
    transfer_mob(npc, room);
    
    test_socket_output_enabled = true;
    do_groups(npc, "");
    test_socket_output_enabled = false;
    
    // NPCs should not produce output
    ASSERT(IS_NIL(test_output_buffer));
    
    return 0;
}

void register_skills_tests()
{
    TestGroup* group = calloc(1, sizeof(TestGroup));
    init_test_group(group, "SKILLS TESTS");
    
#define REGISTER(name, func) register_test(group, (name), (func))
    
    // Gain command
    REGISTER("Gain: No trainer", test_gain_no_trainer);
    REGISTER("Gain: With trainer", test_gain_with_trainer);
    REGISTER("Gain: Empty argument", test_gain_empty_arg);
    
    // Spells command
    REGISTER("Spells: List all", test_spells_list);
    REGISTER("Spells: By class", test_spells_by_class);
    REGISTER("Spells: NPC check", test_spells_npc);
    
    // Skills command
    REGISTER("Skills: List all", test_skills_list);
    REGISTER("Skills: By class", test_skills_by_class);
    REGISTER("Skills: NPC check", test_skills_npc);
    
    // Groups command
    REGISTER("Groups: List all", test_groups_list);
    REGISTER("Groups: Specific group", test_groups_specific);
    REGISTER("Groups: NPC check", test_groups_npc);

#undef REGISTER

    register_test_group(group);
}
