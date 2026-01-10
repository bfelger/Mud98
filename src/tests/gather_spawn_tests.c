////////////////////////////////////////////////////////////////////////////////
// gather_spawn_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <db.h>
#include <command.h>
#include <gsn.h>
#include <craft/craft.h>
#include <craft/gather.h>
#include <entities/object.h>
#include <entities/room.h>
#include <handler.h>

TestGroup gather_spawn_tests;

static int count_room_objects_by_vnum(Room* room, VNUM vnum)
{
    int count = 0;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (VNUM_FIELD(obj) == vnum)
            count++;
    }
    return count;
}

static int count_area_objects_by_vnum(Area* area, VNUM vnum)
{
    int count = 0;
    Room* room = NULL;
    FOR_EACH_AREA_ROOM(room, area) {
        count += count_room_objects_by_vnum(room, vnum);
    }
    return count;
}

static int count_mob_objects_by_vnum(Mobile* ch, VNUM vnum)
{
    int count = 0;
    Object* obj = NULL;
    FOR_EACH_MOB_OBJ(obj, ch) {
        if (VNUM_FIELD(obj) == vnum)
            count++;
    }
    return count;
}

static Object* setup_mining_node(Room* room, VNUM node_vnum, VNUM mat_vnum,
    int quantity, int min_skill)
{
    ObjPrototype* mat_proto = mock_obj_proto(mat_vnum);
    mat_proto->header.name = AS_STRING(mock_str("ore"));
    mat_proto->short_descr = str_dup("ore");
    mat_proto->item_type = ITEM_MAT;
    mat_proto->craft_mat.mat_type = MAT_ORE;
    mat_proto->craft_mat.amount = 1;
    mat_proto->craft_mat.quality = 50;

    ObjPrototype* node_proto = mock_obj_proto(node_vnum);
    node_proto->header.name = AS_STRING(mock_str("ore node"));
    node_proto->short_descr = str_dup("ore node");
    node_proto->item_type = ITEM_GATHER;
    node_proto->gather.gather_type = GATHER_ORE;
    node_proto->gather.mat_vnum = mat_vnum;
    node_proto->gather.quantity = quantity;
    node_proto->gather.min_skill = min_skill;

    Object* node = mock_obj("ore node", node_vnum, node_proto);
    obj_to_room(node, room);
    return node;
}

static int test_reset_gather_spawn_places_nodes_in_matching_rooms()
{
    VNUM node_vnum = 61000;
    ObjPrototype* proto = mock_obj_proto(node_vnum);
    proto->item_type = ITEM_GATHER;

    AreaData* area_data = mock_area_data();
    add_gather_spawn(&area_data->gather_spawns, SECT_MOUNTAIN, node_vnum, 2, 5);

    Area* area = mock_area(area_data);

    Room* room_one = mock_room(61001, NULL, area);
    Room* room_two = mock_room(61002, NULL, area);
    Room* room_three = mock_room(61003, NULL, area);

    room_one->data->sector_type = SECT_MOUNTAIN;
    room_two->data->sector_type = SECT_MOUNTAIN;
    room_three->data->sector_type = SECT_FOREST;

    GatherSpawn* spawn = &area->gather_spawns.spawns[0];

    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();

    reset_gather_spawn(area, spawn);

    rng = saved_rng;

    ASSERT(count_room_objects_by_vnum(room_one, node_vnum) == 1);
    ASSERT(count_room_objects_by_vnum(room_two, node_vnum) == 1);
    ASSERT(count_room_objects_by_vnum(room_three, node_vnum) == 0);
    ASSERT(count_area_objects_by_vnum(area, node_vnum) == 2);
    ASSERT(spawn->respawn_timer == 0);

    return 0;
}

static int test_reset_gather_spawn_clears_matching_rooms_with_zero_quantity()
{
    VNUM node_vnum = 61010;
    ObjPrototype* proto = mock_obj_proto(node_vnum);
    proto->item_type = ITEM_GATHER;

    AreaData* area_data = mock_area_data();
    add_gather_spawn(&area_data->gather_spawns, SECT_FIELD, node_vnum, 0, 5);

    Area* area = mock_area(area_data);

    Room* room = mock_room(61011, NULL, area);
    room->data->sector_type = SECT_FIELD;

    Object* existing = mock_obj("gather node", node_vnum, proto);
    obj_to_room(existing, room);
    ASSERT(count_room_objects_by_vnum(room, node_vnum) == 1);

    GatherSpawn* spawn = &area->gather_spawns.spawns[0];
    reset_gather_spawn(area, spawn);

    ASSERT(count_room_objects_by_vnum(room, node_vnum) == 0);
    ASSERT(spawn->respawn_timer == 0);

    return 0;
}

static int test_reset_area_gather_spawns_respects_respawn_timer()
{
    VNUM node_vnum = 61020;
    ObjPrototype* proto = mock_obj_proto(node_vnum);
    proto->item_type = ITEM_GATHER;

    AreaData* area_data = mock_area_data();
    add_gather_spawn(&area_data->gather_spawns, SECT_HILLS, node_vnum, 1, 2);

    Area* area = mock_area(area_data);

    Room* room = mock_room(61021, NULL, area);
    room->data->sector_type = SECT_HILLS;

    GatherSpawn* spawn = &area->gather_spawns.spawns[0];
    //spawn->respawn_timer = 0;

    RngOps* saved_rng = rng;
    rng = &mock_rng;
    reset_mock_rng();

    ASSERT(count_room_objects_by_vnum(room, node_vnum) == 0);
    ASSERT(spawn->respawn_timer == 2);

    reset_area_gather_spawns(area);
    ASSERT(count_room_objects_by_vnum(room, node_vnum) == 1);
    ASSERT(spawn->respawn_timer == 0);

    reset_area_gather_spawns(area);
    ASSERT(count_room_objects_by_vnum(room, node_vnum) == 1);
    ASSERT(spawn->respawn_timer == 1);

    rng = saved_rng;

    return 0;
}

static int test_mine_success_creates_materials_and_depletes_node()
{
    Room* room = mock_room(62001, NULL, NULL);
    Mobile* ch = mock_player("Miner");
    ch->level = 5;
    transfer_mob(ch, room);
    mock_skill(ch, gsn_mining, 100);

    VNUM node_vnum = 62000;
    VNUM mat_vnum = 62010;
    Object* node = setup_mining_node(room, node_vnum, mat_vnum, 2, 0);

    test_socket_output_enabled = true;
    do_mine(ch, "node");
    test_socket_output_enabled = false;

    ASSERT(count_mob_objects_by_vnum(ch, mat_vnum) == 2);
    ASSERT(node->gather.quantity == 0);
    ASSERT_OUTPUT_CONTAINS("You carefully mine");
    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_mine_requires_skill()
{
    Room* room = mock_room(62002, NULL, NULL);
    Mobile* ch = mock_player("NoSkillMiner");
    transfer_mob(ch, room);

    VNUM node_vnum = 62002;
    VNUM mat_vnum = 62012;
    Object* node = setup_mining_node(room, node_vnum, mat_vnum, 1, 0);

    test_socket_output_enabled = true;
    do_mine(ch, "node");
    test_socket_output_enabled = false;

    ASSERT(count_mob_objects_by_vnum(ch, mat_vnum) == 0);
    ASSERT(node->gather.quantity == 1);
    ASSERT_OUTPUT_CONTAINS("don't know how to mine");
    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_mine_respects_min_skill()
{
    Room* room = mock_room(62003, NULL, NULL);
    Mobile* ch = mock_player("LowSkillMiner");
    ch->level = 5;
    transfer_mob(ch, room);
    mock_skill(ch, gsn_mining, 25);

    VNUM node_vnum = 62003;
    VNUM mat_vnum = 62013;
    Object* node = setup_mining_node(room, node_vnum, mat_vnum, 1, 50);

    test_socket_output_enabled = true;
    do_mine(ch, "node");
    test_socket_output_enabled = false;

    ASSERT(count_mob_objects_by_vnum(ch, mat_vnum) == 0);
    ASSERT(node->gather.quantity == 1);
    ASSERT_OUTPUT_CONTAINS("not skilled enough to mine");
    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_mine_already_mined()
{
    Room* room = mock_room(62004, NULL, NULL);
    Mobile* ch = mock_player("MinedOut");
    transfer_mob(ch, room);
    mock_skill(ch, gsn_mining, 80);

    VNUM node_vnum = 62004;
    VNUM mat_vnum = 62014;
    Object* node = setup_mining_node(room, node_vnum, mat_vnum, 0, 0);

    test_socket_output_enabled = true;
    do_mine(ch, "node");
    test_socket_output_enabled = false;

    ASSERT(count_mob_objects_by_vnum(ch, mat_vnum) == 0);
    ASSERT(node->gather.quantity == 0);
    ASSERT_OUTPUT_CONTAINS("already been mined");
    test_output_buffer = NIL_VAL;

    return 0;
}

void register_gather_spawn_tests()
{
#define REGISTER(name, fn) register_test(&gather_spawn_tests, (name), (fn))

    init_test_group(&gather_spawn_tests, "GATHER SPAWN TESTS");
    register_test_group(&gather_spawn_tests);

    REGISTER("Reset Gather Spawn Places Nodes In Matching Rooms",
        test_reset_gather_spawn_places_nodes_in_matching_rooms);
    REGISTER("Reset Gather Spawn Clears Matching Rooms With Zero Quantity",
        test_reset_gather_spawn_clears_matching_rooms_with_zero_quantity);
    REGISTER("Reset Area Gather Spawns Respects Respawn Timer",
        test_reset_area_gather_spawns_respects_respawn_timer);
    REGISTER("Mine Success Creates Materials And Depletes Node",
        test_mine_success_creates_materials_and_depletes_node);
    REGISTER("Mine Requires Skill",
        test_mine_requires_skill);
    REGISTER("Mine Respects Min Skill",
        test_mine_respects_min_skill);
    REGISTER("Mine Already Mined",
        test_mine_already_mined);

#undef REGISTER
}
