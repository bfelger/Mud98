////////////////////////////////////////////////////////////////////////////////
// act_wiz2_tests.c - Additional wizard command tests
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "mock.h"
#include "test_registry.h"

#include <act_wiz.h>
#include <handler.h>
#include <merc.h>

#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>

#include <data/mobile_data.h>

extern bool test_socket_output_enabled;
extern bool test_act_output_enabled;
extern Value test_output_buffer;

// Test do_mload - load a mobile by VNUM
static int test_mload_valid()
{
    Room* room = mock_room(60001, NULL, NULL);
    MobPrototype* mp = mock_mob_proto(60100);
    mp->header.name = AS_STRING(mock_str("test mob"));
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    int mob_count_before = room->mobiles.count;
    
    test_socket_output_enabled = true;
    do_mload(wiz, "60100");
    test_socket_output_enabled = false;
    
    ASSERT(room->mobiles.count == mob_count_before + 1);
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_mload - invalid VNUM
static int test_mload_invalid()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_mload(wiz, "99999");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("No mob has that vnum");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_oload - load an object by VNUM
static int test_oload_valid()
{
    Room* room = mock_room(60001, NULL, NULL);
    ObjPrototype* op = mock_obj_proto(60200);
    op->header.name = AS_STRING(mock_str("test object"));
    op->wear_flags = ITEM_TAKE;  // Make it takeable so it goes to inventory
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    int obj_count_before = wiz->objects.count;
    
    test_socket_output_enabled = true;
    do_oload(wiz, "60200");
    test_socket_output_enabled = false;
    
    ASSERT(wiz->objects.count == obj_count_before + 1);
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_oload - invalid VNUM
static int test_oload_invalid()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_oload(wiz, "99999");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("No object has that vnum");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_purge - purge all entities in room
static int test_purge_room()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    // Add some test mobs and objects
    Mobile* mob = mock_mob("TestMob", 60100, mock_mob_proto(60100));
    transfer_mob(mob, room);
    
    Object* obj = mock_obj("TestObj", 60200, mock_obj_proto(60200));
    obj_to_room(obj, room);
    
    ASSERT(room->mobiles.count == 2);  // wiz + mob
    ASSERT(room->objects.count == 1);
    
    do_purge(wiz, "");
    
    // Should purge mob and object, but not the wizard
    ASSERT(room->mobiles.count == 1);  // just wiz
    ASSERT(room->objects.count == 0);
    
    return 0;
}

// Test do_purge - purge specific mob
static int test_purge_target()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* mob = mock_mob("testmob", 60100, mock_mob_proto(60100));
    transfer_mob(mob, room);
    
    ASSERT(room->mobiles.count == 2);
    
    do_purge(wiz, "testmob");
    
    ASSERT(room->mobiles.count == 1);  // just wiz
    
    return 0;
}

// Test do_restore - restore a player
static int test_restore_player()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->hit = 10;
    player->max_hit = 100;
    player->mana = 5;
    player->max_mana = 50;
    player->move = 1;
    player->max_move = 100;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_restore(wiz, "testplayer");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(player->hit == player->max_hit);
    ASSERT(player->mana == player->max_mana);
    ASSERT(player->move == player->max_move);
    
    return 0;
}

// Test do_peace - stop all fighting in room
static int test_peace()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* mob1 = mock_mob("mob1", 60100, mock_mob_proto(60100));
    transfer_mob(mob1, room);
    
    Mobile* mob2 = mock_mob("mob2", 60101, mock_mob_proto(60101));
    transfer_mob(mob2, room);
    
    // Make them fight
    mob1->fighting = mob2;
    mob2->fighting = mob1;
    
    ASSERT(mob1->fighting != NULL);
    ASSERT(mob2->fighting != NULL);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_peace(wiz, "");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(mob1->fighting == NULL);
    ASSERT(mob2->fighting == NULL);
    
    return 0;
}

// Test do_at - execute command at another location
static int test_at_valid_location()
{
    Room* room1 = mock_room(60001, NULL, NULL);
    Room* room2 = mock_room(60002, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room1);
    
    Mobile* target = mock_mob("targetmob", 60100, mock_mob_proto(60100));
    transfer_mob(target, room2);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_at(wiz, "60002 look");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Wizard should still be in room1
    ASSERT(wiz->in_room == room1);
    
    return 0;
}

// Test do_clone - clone an object
static int test_clone_object()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Object* original = mock_obj("testobj", 60200, mock_obj_proto(60200));
    obj_to_char(original, wiz);
    
    int obj_count_before = wiz->objects.count;
    
    test_socket_output_enabled = true;
    do_clone(wiz, "testobj");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    // Should have original + clone
    ASSERT(wiz->objects.count == obj_count_before + 1);
    
    return 0;
}

void register_act_wiz2_tests()
{
    TestGroup* group = calloc(1, sizeof(TestGroup));
    init_test_group(group, "ACT WIZ2 TESTS");
    
#define REGISTER(name, func) register_test(group, (name), (func))
    
    // Loading commands
    REGISTER("Cmd: Mload valid", test_mload_valid);
    REGISTER("Cmd: Mload invalid", test_mload_invalid);
    REGISTER("Cmd: Oload valid", test_oload_valid);
    REGISTER("Cmd: Oload invalid", test_oload_invalid);
    
    // Entity management
    REGISTER("Cmd: Purge room", test_purge_room);
    REGISTER("Cmd: Purge target", test_purge_target);
    
    // Player management
    REGISTER("Cmd: Restore player", test_restore_player);
    
    // Combat management
    REGISTER("Cmd: Peace", test_peace);
    
    // Remote execution
    REGISTER("Cmd: At valid location", test_at_valid_location);
    
    // Object manipulation
    REGISTER("Cmd: Clone object", test_clone_object);

#undef REGISTER

    register_test_group(group);
}
