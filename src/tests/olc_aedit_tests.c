////////////////////////////////////////////////////////////////////////////////
// tests/olc_aedit_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <craft/recipe.h>

#include <data/quest.h>

#include <entities/faction.h>
#include <entities/reset.h>

#include <olc/olc.h>

TestGroup olc_aedit_tests;

////////////////////////////////////////////////////////////////////////////////
// AEdit MoveVnums Tests
////////////////////////////////////////////////////////////////////////////////

static int test_aedit_movevnums_shifts_references()
{
    AreaData* area_a = mock_area_data();
    area_a->min_vnum = 900000;
    area_a->max_vnum = 900009;
    write_value_array(&global_areas, OBJ_VAL(area_a));

    AreaData* area_b = mock_area_data();
    area_b->min_vnum = 910000;
    area_b->max_vnum = 910009;
    write_value_array(&global_areas, OBJ_VAL(area_b));

    Area* inst_a = mock_area(area_a);
    Area* inst_b = mock_area(area_b);

    Room* room_a = mock_room(900000, NULL, inst_a);
    Room* room_b = mock_room(910000, NULL, inst_b);

    mock_room_connection(room_b, room_a, DIR_NORTH, false);
    room_b->data->exit_data[DIR_NORTH]->key = 900003;

    Mobile* ch = mock_player("AEditMover1");
    ch->pcdata->security = 9;
    transfer_mob(ch, room_b);

    MobPrototype* mob_proto = mock_mob_proto(900001);
    mob_proto->area = area_a;
    mob_proto->faction_vnum = 900004;
    mob_proto->craft_mat_count = 1;
    mob_proto->craft_mats = malloc(sizeof(VNUM));
    mob_proto->craft_mats[0] = 900002;

    ObjPrototype* obj_proto = mock_obj_proto(900002);
    obj_proto->area = area_a;
    obj_proto->item_type = ITEM_CONTAINER;
    obj_proto->container.key_vnum = 900003;

    ObjPrototype* key_proto = mock_obj_proto(900003);
    key_proto->area = area_a;

    Faction* faction = faction_create(900004);
    faction->area = area_a;

    Recipe* recipe = mock_recipe("movevnums recipe", 910005);
    recipe->area = area_b;
    recipe->station_vnum = 900000;
    recipe->product_vnum = 900002;
    recipe_add_ingredient(recipe, 900003, 1);

    Quest* quest = new_quest();
    quest->area_data = area_b;
    quest->vnum = 910006;
    quest->end = 900000;
    quest->target = 900001;
    quest->target_upper = 900001;
    quest->reward_faction_vnum = 900004;
    quest->reward_obj_vnum[0] = 900002;
    quest->reward_obj_count[0] = 1;
    ORDERED_INSERT(Quest, quest, area_b->quests, vnum);

    Reset* reset = new_reset();
    reset->command = 'M';
    reset->arg1 = 900001;
    reset->arg2 = 1;
    reset->arg3 = VNUM_FIELD(room_b->data);
    reset->arg4 = 1;
    reset->next = NULL;
    room_b->data->reset_first = reset;
    room_b->data->reset_last = reset;

    set_editor(ch->desc, ED_AREA, (uintptr_t)area_a);

    test_socket_output_enabled = true;
    aedit(ch, "movevnums 920000");
    test_socket_output_enabled = false;

    ASSERT(area_a->min_vnum == 920000);
    ASSERT(area_a->max_vnum == 920009);
    ASSERT(VNUM_FIELD(room_a->data) == 920000);
    ASSERT(VNUM_FIELD(room_a) == 920000);
    ASSERT(get_room_data(900000) == NULL);
    ASSERT(get_room_data(920000) == room_a->data);
    ASSERT(room_b->data->exit_data[DIR_NORTH]->to_vnum == 920000);
    ASSERT(room_b->data->exit_data[DIR_NORTH]->key == 920003);
    ASSERT(reset->arg1 == 920001);
    ASSERT(VNUM_FIELD(mob_proto) == 920001);
    ASSERT(mob_proto->faction_vnum == 920004);
    ASSERT(mob_proto->craft_mats[0] == 920002);
    ASSERT(VNUM_FIELD(obj_proto) == 920002);
    ASSERT(obj_proto->container.key_vnum == 920003);
    ASSERT(get_object_prototype(900002) == NULL);
    ASSERT(get_object_prototype(920002) == obj_proto);
    ASSERT(get_mob_prototype(900001) == NULL);
    ASSERT(get_mob_prototype(920001) == mob_proto);
    ASSERT(get_faction(900004) == NULL);
    ASSERT(get_faction(920004) == faction);
    ASSERT(recipe->station_vnum == 920000);
    ASSERT(recipe->product_vnum == 920002);
    ASSERT(recipe->ingredients[0].mat_vnum == 920003);
    ASSERT(quest->end == 920000);
    ASSERT(quest->target == 920001);
    ASSERT(quest->reward_obj_vnum[0] == 920002);
    ASSERT(quest->reward_faction_vnum == 920004);
    ASSERT_OUTPUT_CONTAINS("Area VNUMs moved");

    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

static int test_aedit_movevnums_rejects_overlap()
{
    Room* room = mock_room(940020, NULL, NULL);
    Mobile* ch = mock_player("AEditMover2");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);

    AreaData* area_a = mock_area_data();
    area_a->min_vnum = 940000;
    area_a->max_vnum = 940009;
    write_value_array(&global_areas, OBJ_VAL(area_a));

    AreaData* area_b = mock_area_data();
    area_b->min_vnum = 940010;
    area_b->max_vnum = 940019;
    write_value_array(&global_areas, OBJ_VAL(area_b));

    set_editor(ch->desc, ED_AREA, (uintptr_t)area_a);

    test_socket_output_enabled = true;
    aedit(ch, "movevnums 940010");
    test_socket_output_enabled = false;

    ASSERT(area_a->min_vnum == 940000);
    ASSERT(area_a->max_vnum == 940009);
    ASSERT_OUTPUT_CONTAINS("overlaps");

    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Test Registration
////////////////////////////////////////////////////////////////////////////////

void register_olc_aedit_tests()
{
#define REGISTER(name, func) register_test(&olc_aedit_tests, name, func)

    init_test_group(&olc_aedit_tests, "OLC AEdit TESTS");
    register_test_group(&olc_aedit_tests);

    REGISTER("AEdit: MoveVnums Shifts References", test_aedit_movevnums_shifts_references);
    REGISTER("AEdit: MoveVnums Overlap Rejected", test_aedit_movevnums_rejects_overlap);
 
#undef REGISTER
}
