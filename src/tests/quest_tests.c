////////////////////////////////////////////////////////////////////////////////
// tests/quest_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <data/quest.h>

#include <config.h>
#include <db.h>
#include <handler.h>

#include <entities/faction.h>
#include <entities/object.h>
#include <entities/obj_prototype.h>

#include <lox/list.h>
#include <lox/value.h>

static TestGroup quest_tests;

static AreaData* quest_tests_area()
{
    static bool initialized = false;
    static AreaData* area = NULL;

    if (!initialized) {
        area = mock_area_data();
        area->min_vnum = 800000;
        area->max_vnum = 801000;
        write_value_array(&global_areas, OBJ_VAL(area));
        initialized = true;
    }

    area->quests = NULL;
    return area;
}

static Quest* make_quest(
    QuestType type, VNUM vnum, VNUM target, VNUM target_upper, VNUM end, int16_t amount)
{
    Quest* quest = new_quest();
    quest->area_data = quest_tests_area();
    quest->vnum = vnum;
    quest->target = target;
    quest->target_upper = target_upper;
    quest->end = end;
    quest->amount = amount;
    quest->type = type;
    quest->level = 30;
    quest->xp = 0;

    ORDERED_INSERT(Quest, quest, quest->area_data->quests, vnum);

    return quest;
}

static int test_quest_add_targets()
{
    Mobile* player = mock_player("Questor");
    Quest* quest = make_quest(QUEST_KILL_MOB, 800100, 9000, 9002, 9100, 3);

    add_quest_to_log(player->pcdata->quest_log, quest, QSTAT_ACCEPTED, 1);

    QuestStatus* status = player->pcdata->quest_log->quests;
    ASSERT(status != NULL);
    ASSERT(status->quest == quest);
    ASSERT(status->state == QSTAT_ACCEPTED);
    ASSERT(status->progress == 1);
    ASSERT(status->amount == quest->amount);

    QuestTarget* end = player->pcdata->quest_log->target_ends;
    ASSERT(end != NULL);
    ASSERT(end->quest_vnum == quest->vnum);
    ASSERT(end->target_vnum == quest->end);

    int count = 0;
    for (QuestTarget* target = player->pcdata->quest_log->target_mobs;
         target != NULL; target = target->next) {
        ASSERT(target->quest_vnum == quest->vnum);
        ASSERT(target->type == quest->type);
        ASSERT(target->target_vnum == quest->target + count);
        count++;
    }
    ASSERT(count == quest->target_upper - quest->target + 1);

    return 0;
}

static int test_get_quest_target_mob()
{
    Mobile* player = mock_player("Tracker");
    Quest* quest = make_quest(QUEST_KILL_MOB, 800110, 9200, 9203, 9300, 4);
    add_quest_to_log(player->pcdata->quest_log, quest, QSTAT_ACCEPTED, 0);

    QuestTarget* found = get_quest_targ_mob(player, 9202);
    ASSERT(found != NULL);
    ASSERT(found->quest_vnum == quest->vnum);
    ASSERT(found->target_vnum == 9202);

    QuestTarget* missing = get_quest_targ_mob(player, 9199);
    ASSERT(missing == NULL);

    return 0;
}

static int test_get_quest_target_obj()
{
    Mobile* player = mock_player("Collector");
    Quest* quest = make_quest(QUEST_GET_OBJ, 800120, 9400, 9401, 9500, 2);
    add_quest_to_log(player->pcdata->quest_log, quest, QSTAT_ACCEPTED, 0);

    QuestTarget* found = get_quest_targ_obj(player, 9401);
    ASSERT(found != NULL);
    ASSERT(found->quest_vnum == quest->vnum);

    QuestTarget* missing = get_quest_targ_obj(player, 9501);
    ASSERT(missing == NULL);

    return 0;
}

static int test_quest_level_req()
{
    Mobile* player = mock_player("Newbie");
    Quest* quest = make_quest(QUEST_KILL_MOB, 800130, 9600, 9601, 9700, 2);

    player->level = quest->level - 1;
    ASSERT(!can_quest(player, quest->vnum));

    player->level = quest->level;
    ASSERT(can_quest(player, quest->vnum));

    add_quest_to_log(player->pcdata->quest_log, quest, QSTAT_ACCEPTED, 0);
    ASSERT(!can_quest(player, quest->vnum));

    return 0;
}

static int test_quest_completion()
{
    Mobile* player = mock_player("Artemis");
    Quest* quest = make_quest(QUEST_KILL_MOB, 800140, 9800, 9801, 9900, 2);

    ASSERT(!has_quest(player, quest->vnum));

    add_quest_to_log(player->pcdata->quest_log, quest, QSTAT_ACCEPTED, 0);
    ASSERT(has_quest(player, quest->vnum));

    QuestStatus* status = get_quest_status(player, quest->vnum);
    ASSERT(status != NULL);
    status->state = QSTAT_COMPLETE;
    ASSERT(!has_quest(player, quest->vnum));

    return 0;
}

static int test_quest_complete_visit()
{
    Mobile* player = mock_player("Visitor");
    Room* room = mock_room(81000, NULL, NULL);
    mob_to_room(player, room);

    Quest* quest = make_quest(QUEST_VISIT_MOB, 800150, 10000, 10000, 10100, 1);
    add_quest_to_log(player->pcdata->quest_log, quest, QSTAT_ACCEPTED, 0);
    ASSERT(!can_finish_quest(player, quest->vnum));

    MobPrototype* proto = mock_mob_proto(quest->target);
    Mobile* target = mock_mob("Guide", quest->target, proto);
    mob_to_room(target, room);

    ASSERT(can_finish_quest(player, quest->vnum));

    return 0;
}

static int test_quest_complete_kill()
{
    Mobile* player = mock_player("Slayer");
    Quest* quest = make_quest(QUEST_KILL_MOB, 800160, 10200, 10202, 10300, 3);
    add_quest_to_log(player->pcdata->quest_log, quest, QSTAT_ACCEPTED, 0);

    QuestStatus* status = get_quest_status(player, quest->vnum);
    ASSERT(status != NULL);
    status->progress = quest->amount - 1;
    ASSERT(!can_finish_quest(player, quest->vnum));

    status->progress = quest->amount;
    ASSERT(can_finish_quest(player, quest->vnum));

    return 0;
}

static int test_quest_clear_targs()
{
    Mobile* player = mock_player("Finisher");
    Quest* quest = make_quest(QUEST_KILL_MOB, 800170, 10400, 10401, 10500, 2);
    add_quest_to_log(player->pcdata->quest_log, quest, QSTAT_ACCEPTED, quest->amount);

    QuestStatus* status = get_quest_status(player, quest->vnum);
    ASSERT(status != NULL);

    ASSERT(get_quest_targ_mob(player, quest->target) != NULL);
    ASSERT(get_quest_targ_end(player, quest->end) != NULL);

    finish_quest(player, quest, status);

    ASSERT(status->state == QSTAT_COMPLETE);
    ASSERT(get_quest_targ_mob(player, quest->target) == NULL);
    ASSERT(get_quest_targ_end(player, quest->end) == NULL);

    return 0;
}

static int test_quest_reward_currency_and_items()
{
    Mobile* player = mock_player("Rewarded");
    Quest* quest = make_quest(QUEST_KILL_MOB, 800180, 10650, 10651, 10750, 1);
    quest->reward_gold = 3;
    quest->reward_silver = 4;
    quest->reward_copper = 5;

    ObjPrototype* proto = mock_obj_proto(800181);
    proto->short_descr = str_dup("Reward Token");
    quest->reward_obj_vnum[0] = VNUM_FIELD(proto);
    quest->reward_obj_count[0] = 2;

    // We need to add the prototype to the global list so that finish_quest can 
    // find it.
    global_obj_proto_set(proto);

    add_quest_to_log(player->pcdata->quest_log, quest, QSTAT_ACCEPTED, quest->amount);
    QuestStatus* status = get_quest_status(player, quest->vnum);
    ASSERT(status != NULL);

    finish_quest(player, quest, status);

    ASSERT(player->gold == quest->reward_gold);
    ASSERT(player->silver == quest->reward_silver);
    ASSERT(player->copper == quest->reward_copper);
    ASSERT(player->objects.count == quest->reward_obj_count[0]);

    int seen = 0;
    for (Node* node = player->objects.front; node != NULL; node = node->next) {
        Object* obj = AS_OBJECT(node->value);
        ASSERT(obj->prototype == proto);
        seen++;
    }
    ASSERT(seen == quest->reward_obj_count[0]);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_quest_reward_reputation()
{
    Mobile* player = mock_player("Diplomat");
    Quest* quest = make_quest(QUEST_KILL_MOB, 800190, 10760, 10761, 10850, 1);

    Faction* faction = faction_create(800191);
    SET_NAME(faction, lox_string("Scholars"));
    quest->reward_faction_vnum = VNUM_FIELD(faction);
    quest->reward_reputation = 250;

    add_quest_to_log(player->pcdata->quest_log, quest, QSTAT_ACCEPTED, quest->amount);
    QuestStatus* status = get_quest_status(player, quest->vnum);
    ASSERT(status != NULL);

    finish_quest(player, quest, status);

    ASSERT(faction_get_standing(player, faction, false) == quest->reward_reputation);

    test_output_buffer = NIL_VAL;
    return 0;
}

void register_quest_tests()
{
#define REGISTER(n, f)  register_test(&quest_tests, (n), (f))

    cfg_set_player_dir("../temp/quest_tests/");
    cfg_set_gods_dir("../temp/quest_tests/");

    init_test_group(&quest_tests, "QUEST TESTS");
    register_test_group(&quest_tests);

    REGISTER("Add Targets To Quest Log", test_quest_add_targets);
    REGISTER("Get Target for Mobs", test_get_quest_target_mob);
    REGISTER("Get Target for Objects", test_get_quest_target_obj);
    REGISTER("Level Requirement", test_quest_level_req);
    REGISTER("Quest Completion", test_quest_completion);
    REGISTER("Quest Completion: Visit", test_quest_complete_visit);
    REGISTER("Quest Completion: Kill", test_quest_complete_kill);
    REGISTER("Qeust Clear Targets", test_quest_clear_targs);
    REGISTER("Quest Rewards Currency and Items", test_quest_reward_currency_and_items);
    REGISTER("Quest Rewards Reputation", test_quest_reward_reputation);

#undef REGISTER
}
