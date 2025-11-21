////////////////////////////////////////////////////////////////////////////////
// tests/faction_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <act_info.h>
#include <interp.h>
#include <update.h>

#include <entities/faction.h>
#include <entities/mob_prototype.h>
#include <entities/mobile.h>

#include <db.h>

void aggr_update();

static void add_player_to_list(PlayerData* pcdata)
{
    pcdata->next = player_data_list;
    player_data_list = pcdata;
}

static void remove_player_from_list(PlayerData* pcdata)
{
    PlayerData** link = &player_data_list;
    while (*link != NULL) {
        if (*link == pcdata) {
            *link = pcdata->next;
            pcdata->next = NULL;
            break;
        }
        link = &(*link)->next;
    }
}

static int test_reputation_command()
{
    Room* room = mock_room(9100, NULL, NULL);
    Mobile* player = mock_player("Tester");
    transfer_mob(player, room);

    Faction* friendly = faction_create(9101);
    SET_NAME(friendly, lox_string("Stormwind"));
    faction_set(player->pcdata, VNUM_FIELD(friendly), 3500);

    Faction* hostile = faction_create(9102);
    SET_NAME(hostile, lox_string("Scourge"));
    faction_set(player->pcdata, VNUM_FIELD(hostile), -5000);

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    interpret(player, "reputation");
    test_socket_output_enabled = false;

    ASSERT(IS_STRING(test_output_buffer));
    char* out = "";
    if (IS_STRING(test_output_buffer) && AS_STRING(test_output_buffer) != NULL) 
        out = AS_STRING(test_output_buffer)->chars;
    ASSERT(strstr(out, "Stormwind") != NULL);
    ASSERT(strstr(out, "Friendly") != NULL);
    ASSERT(strstr(out, "Scourge") != NULL);
    ASSERT(strstr(out, "Hostile") != NULL);
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_reputation_adjust_on_kill()
{
    Room* room = mock_room(9200, NULL, NULL);
    Mobile* player = mock_player("Slayer");
    transfer_mob(player, room);

    Faction* legion = faction_create(9201);
    SET_NAME(legion, lox_string("Legion"));

    Faction* alliance = faction_create(9202);
    SET_NAME(alliance, lox_string("Alliance"));
    write_value_array(&legion->enemies, INT_VAL(VNUM_FIELD(alliance)));

    MobPrototype* mp = mock_mob_proto(9203);
    mp->faction_vnum = VNUM_FIELD(legion);
    Mobile* victim = mock_mob("Grunt", 9203, mp);
    transfer_mob(victim, room);

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    faction_handle_kill(player, victim);
    test_socket_output_enabled = false;

    ASSERT(faction_get_value(player, legion, false) == -5);
    ASSERT(faction_get_value(player, alliance, false) == 5);
    ASSERT(IS_STRING(test_output_buffer));
    char* out = "";
    if (IS_STRING(test_output_buffer) && AS_STRING(test_output_buffer) != NULL)
        out = AS_STRING(test_output_buffer)->chars;
    ASSERT(strstr(out, "Legion") != NULL);
    ASSERT(strstr(out, "Alliance") != NULL);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_reputation_blocks_attack()
{
    Room* room = mock_room(9300, NULL, NULL);
    Mobile* player = mock_player("Guardian");
    transfer_mob(player, room);

    Faction* wardens = faction_create(9301);
    SET_NAME(wardens, lox_string("Wardens"));
    faction_set(player->pcdata, VNUM_FIELD(wardens), 4000);

    MobPrototype* mp = mock_mob_proto(9302);
    mp->faction_vnum = VNUM_FIELD(wardens);
    Mobile* warden = mock_mob("Warden", 9302, mp);
    transfer_mob(warden, room);

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    interpret(player, "kill Warden");
    test_socket_output_enabled = false;    
    ASSERT(IS_STRING(test_output_buffer));
    char* out = "";
    if (IS_STRING(test_output_buffer) && AS_STRING(test_output_buffer) != NULL)
        out = AS_STRING(test_output_buffer)->chars;
    ASSERT(strstr(out, "refuse") != NULL);
    ASSERT(player->fighting == NULL);
    ASSERT(warden->fighting == NULL);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_hostile_mobs_auto_aggress()
{
    Room* room = mock_room(9400, NULL, NULL);
    Mobile* player = mock_player("Target");
    transfer_mob(player, room);
    add_player_to_list(player->pcdata);

    Faction* cult = faction_create(9401);
    SET_NAME(cult, lox_string("Cult"));
    faction_set(player->pcdata, VNUM_FIELD(cult), -5000);

    MobPrototype* mp = mock_mob_proto(9402);
    mp->faction_vnum = VNUM_FIELD(cult);
    Mobile* cultist = mock_mob("Cultist", 9402, mp);
    REMOVE_BIT(cultist->act_flags, ACT_AGGRESSIVE);
    transfer_mob(cultist, room);

    aggr_update();

    ASSERT(cultist->fighting == player);
    ASSERT(player->fighting == cultist);

    remove_player_from_list(player->pcdata);
    return 0;
}

static TestGroup faction_tests;

void register_faction_tests()
{
#define REGISTER(name, fn) register_test(&faction_tests, (name), (fn))

    init_test_group(&faction_tests, "FACTION TESTS");
    register_test_group(&faction_tests);

    REGISTER("Reputation Command Lists Entries", test_reputation_command);
    REGISTER("Reputation Adjusts On Kill", test_reputation_adjust_on_kill);
    REGISTER("Friendly Factions Cannot Be Attacked", test_reputation_blocks_attack);
    REGISTER("Hostile Factions Attack On Sight", test_hostile_mobs_auto_aggress);

#undef REGISTER
}
