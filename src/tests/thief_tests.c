////////////////////////////////////////////////////////////////////////////////
// thief_tests.c - Tests for thief skills (6 tests)
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "mock.h"
#include "mock_skill_ops.h"
#include "test_registry.h"

#include <act_info.h>
#include <act_move.h>
#include <act_obj.h>
#include <handler.h>
#include <db.h>
#include <merc.h>
#include <skill_ops.h>

#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>

#include <data/mobile_data.h>
#include <data/player.h>

extern bool test_socket_output_enabled;
extern Value test_output_buffer;

// Test hide skill
static int test_hide()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* thief = mock_player("thief");
    thief->position = POS_STANDING;
    thief->level = 20;
    transfer_mob(thief, room);
    mock_skill(thief, gsn_hide, 100);
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_hide, true);
    
    test_socket_output_enabled = true;
    do_hide(thief, "");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Hide succeeded - should have AFF_HIDE flag
    ASSERT(IS_AFFECTED(thief, AFF_HIDE));
    
    return 0;
}

// Test hide failure
static int test_hide_fail()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* thief = mock_player("thief");
    thief->position = POS_STANDING;
    thief->level = 20;
    transfer_mob(thief, room);
    mock_skill(thief, gsn_hide, 100);
    
    // Use skill ops seam to force failure
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_hide, false);
    
    test_socket_output_enabled = true;
    do_hide(thief, "");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Hide failed - should not have AFF_HIDE flag
    ASSERT(!IS_AFFECTED(thief, AFF_HIDE));
    
    return 0;
}

// Test hide blocked by medium armor
static int test_hide_blocked_by_medium_armor()
{
    Room* room = mock_room(60001, NULL, NULL);

    Mobile* thief = mock_player("thief");
    thief->position = POS_STANDING;
    thief->level = 20;
    transfer_mob(thief, room);
    mock_skill(thief, gsn_hide, 100);

    Object* armor = mock_shield("medium shield", 60020, 1);
    armor->armor.armor_type = ARMOR_MEDIUM;
    obj_to_char(armor, thief);
    equip_char(thief, armor, WEAR_SHIELD);

    test_socket_output_enabled = true;
    do_hide(thief, "");
    test_socket_output_enabled = false;
    ASSERT_OUTPUT_CONTAINS("Your armor is too bulky to hide.");
    test_output_buffer = NIL_VAL;

    ASSERT(!IS_AFFECTED(thief, AFF_HIDE));

    return 0;
}

// Test sneak skill
static int test_sneak()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* thief = mock_player("thief");
    thief->position = POS_STANDING;
    thief->level = 20;
    transfer_mob(thief, room);
    mock_skill(thief, gsn_sneak, 100);
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_sneak, true);
    
    test_socket_output_enabled = true;
    do_sneak(thief, "");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Sneak succeeded - should have AFF_SNEAK affect
    ASSERT(IS_AFFECTED(thief, AFF_SNEAK));
    
    return 0;
}

// Test sneak blocked by heavy armor
static int test_sneak_blocked_by_heavy_armor()
{
    Room* room = mock_room(60001, NULL, NULL);

    Mobile* thief = mock_player("thief");
    thief->position = POS_STANDING;
    thief->level = 20;
    transfer_mob(thief, room);
    mock_skill(thief, gsn_sneak, 100);

    Object* armor = mock_shield("heavy shield", 60021, 1);
    armor->armor.armor_type = ARMOR_HEAVY;
    obj_to_char(armor, thief);
    equip_char(thief, armor, WEAR_SHIELD);

    test_socket_output_enabled = true;
    do_sneak(thief, "");
    test_socket_output_enabled = false;
    ASSERT_OUTPUT_CONTAINS("Your armor is too bulky to move silently.");
    test_output_buffer = NIL_VAL;

    ASSERT(!IS_AFFECTED(thief, AFF_SNEAK));

    return 0;
}

// Test picking a lock on a chest
static int test_pick_lock()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* thief = mock_player("thief");
    thief->position = POS_STANDING;
    thief->level = 10;
    transfer_mob(thief, room);
    mock_skill(thief, gsn_pick_lock, 100);
    
    // Create a locked chest
    ObjPrototype* proto = mock_obj_proto(60010);
    proto->item_type = ITEM_CONTAINER;
    proto->container.flags = CONT_CLOSED | CONT_LOCKED;
    proto->container.key_vnum = 0;  // No key required (pickable)
    
    Object* chest = mock_obj("chest", 60010, proto);
    obj_to_room(chest, room);

    ASSERT(IS_SET(chest->container.flags, CONT_LOCKED));
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_pick_lock, true);
    
    test_socket_output_enabled = true;
    do_pick(thief, "chest");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Pick lock executed successfully
    ASSERT(!IS_SET(chest->container.flags, CONT_LOCKED));
    
    return 0;
}

// Test steal gold
static int test_steal_gold()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* thief = mock_player("thief");
    thief->position = POS_STANDING;
    thief->level = 30;
    thief->clan = 1;  // Must be in a clan to steal
    transfer_mob(thief, room);
    mock_skill(thief, gsn_steal, 100);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 10;
    victim->gold = 100;
    victim->silver = 50;
    victim->copper = 25;
    victim->position = POS_SLEEPING;  // Sleeping makes steal easier
    transfer_mob(victim, room);
    
    int thief_copper_before = mobile_total_copper(thief);
    int victim_copper_before = mobile_total_copper(victim);
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_steal, true);
    
    test_socket_output_enabled = true;
    do_steal(thief, "gold victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Steal succeeded - thief should have more money, victim should have less
    ASSERT(mobile_total_copper(thief) > thief_copper_before);
    ASSERT(mobile_total_copper(victim) < victim_copper_before);
    
    return 0;
}

// Test peek skill (looking at someone's inventory)
static int test_peek()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* thief = mock_player("thief");
    thief->position = POS_STANDING;
    thief->level = 20;
    transfer_mob(thief, room);
    mock_skill(thief, gsn_peek, 100);
    
    Mobile* victim = mock_player("victim");
    victim->position = POS_STANDING;
    victim->level = 10;
    transfer_mob(victim, room);
    
    // Give victim an object
    Object* sword = mock_sword("sword", 60010, 10, 2, 6);
    obj_to_char(sword, victim);
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_peek, true);
    
    test_socket_output_enabled = true;
    do_look(thief, "victim");
    test_socket_output_enabled = false;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Peek succeeded - output should contain victim's inventory
    ASSERT_OUTPUT_CONTAINS("peek");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

void register_thief_tests()
{
    TestGroup* group = calloc(1, sizeof(TestGroup));
    init_test_group(group, "THIEF TESTS");

#define REGISTER(name, func) register_test(group, (name), (func))

    REGISTER("Hide: Success", test_hide);
    REGISTER("Hide: Failure", test_hide_fail);
    REGISTER("Hide: Blocked by medium armor", test_hide_blocked_by_medium_armor);
    REGISTER("Sneak: Success", test_sneak);
    REGISTER("Sneak: Blocked by heavy armor", test_sneak_blocked_by_heavy_armor);
    REGISTER("Pick Lock: Chest", test_pick_lock);
    REGISTER("Steal: Gold from victim", test_steal_gold);
    REGISTER("Peek: View inventory", test_peek);

#undef REGISTER

    register_test_group(group);
}
