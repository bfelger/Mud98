////////////////////////////////////////////////////////////////////////////////
// magic_tests.c - Tests for magic item usage (3 tests)
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "mock.h"
#include "mock_skill_ops.h"
#include "test_registry.h"

#include <act_obj.h>
#include <handler.h>
#include <db.h>
#include <magic.h>
#include <merc.h>
#include <skill_ops.h>

#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>

#include <data/mobile_data.h>
#include <data/player.h>
#include <data/skill.h>

extern bool test_socket_output_enabled;
extern Value test_output_buffer;

// Test reciting a scroll successfully
static int test_recite_scroll()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* reader = mock_player("reader");
    reader->position = POS_STANDING;
    reader->level = 10;
    transfer_mob(reader, room);
    mock_skill(reader, gsn_scrolls, 100);
    
    // Create a scroll with a simple spell
    SKNUM bless_sn = skill_lookup("bless");
    if (bless_sn < 0) {
        return 0;  // Skip if spell not found
    }
    
    ObjPrototype* proto = mock_obj_proto(60010);
    proto->item_type = ITEM_SCROLL;
    proto->level = 10;
    proto->value[0] = 10;  // Level
    proto->value[1] = bless_sn;
    proto->value[2] = -1;
    proto->value[3] = -1;
    proto->value[4] = -1;
    
    Object* scroll = mock_obj("scroll", 60010, proto);
    obj_to_char(scroll, reader);
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_scrolls, true);
    
    test_socket_output_enabled = true;
    do_recite(reader, "scroll");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Scroll should be consumed (extracted)
    ASSERT(get_obj_carry(reader, "scroll") == NULL);
    
    return 0;
}

// Test brandishing a staff
static int test_brandish_staff()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wielder = mock_player("wielder");
    wielder->position = POS_STANDING;
    wielder->level = 15;
    transfer_mob(wielder, room);
    mock_skill(wielder, gsn_staves, 100);
    
    // Create a staff with bless spell
    SKNUM bless_sn = skill_lookup("bless");
    if (bless_sn < 0) {
        return 0;  // Skip if spell not found
    }
    
    ObjPrototype* proto = mock_obj_proto(60011);
    proto->item_type = ITEM_STAFF;
    proto->level = 10;
    proto->staff.level = 10;  // Level
    proto->staff.max_charges = 3;   // Max charges
    proto->staff.charges = 2;   // Charges remaining
    proto->staff.spell = bless_sn;
    
    Object* staff = mock_obj("staff", 60011, proto);
    obj_to_char(staff, wielder);
    equip_char(wielder, staff, WEAR_HOLD);
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_staves, true);
    
    test_socket_output_enabled = true;
    do_brandish(wielder, "");
    test_socket_output_enabled = false;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Brandish executed successfully
    ASSERT_OUTPUT_CONTAINS("You brandish staff")

    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test zapping a wand
static int test_zap_wand()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* zapper = mock_player("zapper");
    zapper->position = POS_STANDING;
    zapper->level = 15;
    transfer_mob(zapper, room);
    mock_skill(zapper, gsn_wands, 100);
    
    // Create a target for the wand
    Mobile* target = mock_mob("target", 60020, NULL);
    target->position = POS_STANDING;
    target->level = 5;
    target->max_hit = 50;
    target->hit = 50;
    transfer_mob(target, room);
    
    // Create a wand with a simple offensive spell
    SKNUM mm_sn = skill_lookup("magic missile");
    ASSERT(mm_sn >= 0);
    if (mm_sn < 0) {
        return 0;  // Skip if spell not found
    }
    
    ObjPrototype* proto = mock_obj_proto(60012);
    proto->item_type = ITEM_WAND;
    proto->level = 10;
    proto->wand.level = 10;  // Level
    proto->wand.max_charges = 5;   // Max charges
    proto->wand.charges = 3;   // Charges remaining
    proto->wand.spell = mm_sn;
    
    Object* wand = mock_obj("wand", 60012, proto);
    obj_to_char(wand, zapper);
    equip_char(zapper, wand, WEAR_HOLD);
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_wands, true);
    
    test_socket_output_enabled = true;
    do_zap(zapper, "target");
    test_socket_output_enabled = false;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Zap executed successfully
    ASSERT_OUTPUT_CONTAINS("You zap target with wand")
    
    test_output_buffer = NIL_VAL;
    
    return 0;
}

void register_magic_tests()
{
    TestGroup* group = calloc(1, sizeof(TestGroup));
    init_test_group(group, "MAGIC TESTS");

#define REGISTER(name, func) register_test(group, (name), (func))

    REGISTER("Recite: Scroll", test_recite_scroll);
    REGISTER("Brandish: Staff", test_brandish_staff);
    REGISTER("Zap: Wand", test_zap_wand);

#undef REGISTER

    register_test_group(group);
}
