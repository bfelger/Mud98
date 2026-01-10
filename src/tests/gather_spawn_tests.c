////////////////////////////////////////////////////////////////////////////////
// gather_spawn_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

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

#undef REGISTER
}
