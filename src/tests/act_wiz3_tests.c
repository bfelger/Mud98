////////////////////////////////////////////////////////////////////////////////
// act_wiz3_tests.c - More wizard command tests (stat, find, admin commands)
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

// Test do_mstat - show mob stats
static int test_mstat_self()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_mstat(wiz, "self");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Name: Wizard");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_ostat - show object stats
static int test_ostat_valid()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Object* obj = mock_obj("testobj", 60200, mock_obj_proto(60200));
    obj_to_char(obj, wiz);
    
    test_socket_output_enabled = true;
    do_ostat(wiz, "testobj");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Name(s):");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_rstat - show room stats
static int test_rstat()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_rstat(wiz, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Vnum: 60001");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_mfind - find mobs by name
static int test_mfind_valid()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    MobPrototype* mp = mock_mob_proto(60100);
    mp->header.name = AS_STRING(mock_str("test mob"));
    mp->short_descr = str_dup("a test mob");
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_mfind(wiz, "test");
    test_socket_output_enabled = false;
    
    // Check that output was produced (it's a Lox string)
    ASSERT(!IS_NIL(test_output_buffer));
    ASSERT(IS_STRING(test_output_buffer));
    ASSERT_OUTPUT_CONTAINS("60100");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_ofind - find objects by name
static int test_ofind_valid()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    ObjPrototype* op = mock_obj_proto(60200);
    op->header.name = AS_STRING(mock_str("test object"));
    op->short_descr = str_dup("a test object");
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_ofind(wiz, "test");
    test_socket_output_enabled = false;
    
    // Check that output was produced (it's a Lox string)
    ASSERT(!IS_NIL(test_output_buffer));
    ASSERT(IS_STRING(test_output_buffer));
    ASSERT_OUTPUT_CONTAINS("60200");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_mwhere - find mobs in world
static int test_mwhere()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* mob = mock_mob("testmob", 60100, mock_mob_proto(60100));
    transfer_mob(mob, room);
    
    test_socket_output_enabled = true;
    do_mwhere(wiz, "testmob");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("[60001]");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_owhere - find objects in world
static int test_owhere()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Object* obj = mock_obj("testobj", 60200, mock_obj_proto(60200));
    obj_to_room(obj, room);
    
    test_socket_output_enabled = true;
    do_owhere(wiz, "testobj");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Room 60001");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_vnum - lookup vnums
static int test_vnum_mob()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    MobPrototype* mp = mock_mob_proto(60100);
    mp->header.name = AS_STRING(mock_str("test mob"));
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    test_socket_output_enabled = true;
    do_vnum(wiz, "mob test");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("[60100]");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test do_freeze - freeze a player
static int test_freeze()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    transfer_mob(player, room);
    
    ASSERT(!IS_SET(player->act_flags, PLR_FREEZE));
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_freeze(wiz, "testplayer");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(IS_SET(player->act_flags, PLR_FREEZE));
    
    return 0;
}

// Test do_log - set log flag on player
static int test_log()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    transfer_mob(player, room);
    
    ASSERT(!IS_SET(player->act_flags, PLR_LOG));
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_log(wiz, "testplayer");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(IS_SET(player->act_flags, PLR_LOG));
    
    return 0;
}

// Test do_noemote - prevent player from emoting
static int test_noemote()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    transfer_mob(player, room);
    
    ASSERT(!IS_SET(player->comm_flags, COMM_NOEMOTE));
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_noemote(wiz, "testplayer");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(IS_SET(player->comm_flags, COMM_NOEMOTE));
    
    return 0;
}

// Test do_noshout - prevent player from shouting
static int test_noshout()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    transfer_mob(player, room);
    
    ASSERT(!IS_SET(player->comm_flags, COMM_NOSHOUT));
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_noshout(wiz, "testplayer");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(IS_SET(player->comm_flags, COMM_NOSHOUT));
    
    return 0;
}

// Test do_notell - prevent player from telling
static int test_notell()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    transfer_mob(player, room);
    
    ASSERT(!IS_SET(player->comm_flags, COMM_NOTELL));
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_notell(wiz, "testplayer");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(IS_SET(player->comm_flags, COMM_NOTELL));
    
    return 0;
}

// Test do_trust - set trust level
static int test_trust()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    player->trust = 0;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_trust(wiz, "testplayer 52");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(player->trust == 52);
    
    return 0;
}

// Test do_protect - set protected flag
static int test_protect()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* wiz = mock_imm("Wizard");
    transfer_mob(wiz, room);
    
    Mobile* player = mock_player("testplayer");
    transfer_mob(player, room);
    
    ASSERT(!IS_SET(player->comm_flags, COMM_SNOOP_PROOF));
    
    test_socket_output_enabled = true;
    test_act_output_enabled = true;
    do_protect(wiz, "testplayer");
    test_socket_output_enabled = false;
    test_act_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    ASSERT(IS_SET(player->comm_flags, COMM_SNOOP_PROOF));
    
    return 0;
}

void register_act_wiz3_tests()
{
    TestGroup* group = calloc(1, sizeof(TestGroup));
    init_test_group(group, "ACT WIZ3 TESTS");
    
#define REGISTER(name, func) register_test(group, (name), (func))
    
    // Stat commands
    REGISTER("Cmd: Mstat self", test_mstat_self);
    REGISTER("Cmd: Ostat valid", test_ostat_valid);
    REGISTER("Cmd: Rstat", test_rstat);
    
    // Find commands
    REGISTER("Cmd: Mfind valid", test_mfind_valid);
    REGISTER("Cmd: Ofind valid", test_ofind_valid);
    REGISTER("Cmd: Mwhere", test_mwhere);
    REGISTER("Cmd: Owhere", test_owhere);
    REGISTER("Cmd: Vnum mob", test_vnum_mob);
    
    // Player management
    REGISTER("Cmd: Freeze", test_freeze);
    REGISTER("Cmd: Log", test_log);
    REGISTER("Cmd: Noemote", test_noemote);
    REGISTER("Cmd: Noshout", test_noshout);
    REGISTER("Cmd: Notell", test_notell);
    REGISTER("Cmd: Trust", test_trust);
    REGISTER("Cmd: Protect", test_protect);

#undef REGISTER

    register_test_group(group);
}
