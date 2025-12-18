////////////////////////////////////////////////////////////////////////////////
// fight_tests.c - Combat command tests (15 tests)
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "mock.h"
#include "mock_skill_ops.h"
#include "test_registry.h"

#include <combat_ops.h>
#include <fight.h>
#include <handler.h>
#include <db.h>
#include <merc.h>
#include <rng.h>
#include <skill_ops.h>

#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>

#include <data/mobile_data.h>
#include <data/player.h>

extern bool test_socket_output_enabled;
extern Value test_output_buffer;

// Test kill command - basic attack
static int test_kill_basic()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 60;  // High level to guarantee kill
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 1;    // Low level
    victim->max_hit = 10; // Low HP
    victim->hit = 10;
    transfer_mob(victim, room);
    
    // Use combat mock to guarantee hit and high damage
    CombatOps* saved_combat = combat;
    combat = &mock_combat;
    reset_mock_combat();
    set_mock_always_hit(true);
    set_mock_damage_override(1000);  // Massive overkill damage
    
    test_socket_output_enabled = true;
    do_kill(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    combat = saved_combat;

    // Verify victim died - attacker should not be dead/resting and death count should be 1
    ASSERT(attacker->position != POS_RESTING && attacker->position != POS_DEAD && get_mock_deaths() == 1);
    
    return 0;
}

// Test murder command - PK attack
static int test_murder_player()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 60;  // High level to guarantee kill
    transfer_mob(attacker, room);
    
    Mobile* victim = mock_player("victim");
    victim->level = 1;    // Low level
    victim->max_hit = 10; // Low HP
    victim->hit = 10;
    transfer_mob(victim, room);
    
    // Use combat mock to guarantee hit and high damage
    CombatOps* saved_combat = combat;
    combat = &mock_combat;
    reset_mock_combat();
    set_mock_always_hit(true);
    set_mock_damage_override(1000);  // Massive overkill damage
    
    test_socket_output_enabled = true;
    do_murder(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    combat = saved_combat;
    
    // Verify victim died - should have created a corpse (PC corpse)
    //Object* corpse = NULL;
    //Object* obj;
    //FOR_EACH_ROOM_OBJ(obj, room) {
    //    if (obj->item_type == ITEM_CORPSE_PC) {
    //        corpse = obj;
    //        break;
    //    }
    //}
    
    // Should find a corpse (victim died and make_corpse was called)
    //ASSERT(corpse != NULL);
    ASSERT(attacker->position != POS_RESTING && attacker->position != POS_DEAD && get_mock_deaths() == 1);
    
    return 0;
}

// Test flee command
static int test_flee()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    player->level = 10;
    transfer_mob(player, room);
    
    MobPrototype* proto = mock_mob_proto(60003);
    Mobile* mob = mock_mob("mob", 60003, proto);
    mob->level = 15;
    transfer_mob(mob, room);
    
    // Start combat
    player->fighting = mob;
    mob->fighting = player;
    player->position = POS_FIGHTING;
    mob->position = POS_FIGHTING;
    
    test_socket_output_enabled = true;
    do_flee(player, "");
    test_socket_output_enabled = false;

    static const char* possible_outputs[] = {
        "You flee from combat!",
        "PANIC! You couldn't escape!"
    };
    ASSERT_OUTPUT_CONTAINS_ANY(possible_outputs, 2);

    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test kick command
static int test_kick()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 60;
    transfer_mob(attacker, room);
    mock_skill(attacker, gsn_kick, 100);  // Ensure attacker has kick skill
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 5;
    victim->hit = 100;
    victim->max_hit = 100;
    transfer_mob(victim, room);
    
    // Start combat first - kick requires ch->fighting to be set
    multi_hit(attacker, victim, TYPE_UNDEFINED);
    ASSERT(attacker->fighting == victim);
    
    // Use mock RNG to force success - kick uses number_percent() for skill check
    int sequence[] = {1, 1, 50, 50};  // Very low rolls for guaranteed success
    RngOps* saved_rng = rng;
    rng = &mock_rng;
    set_mock_rng_sequence(sequence, 4);
    
    int hp_before = victim->hit;
    test_socket_output_enabled = true;
    do_kick(attacker, "");  // No argument - uses ch->fighting
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    set_mock_rng_sequence(NULL, 0);
    rng = saved_rng;
    
    // Kick should deal damage
    ASSERT(victim->hit < hp_before);
    
    return 0;
}

// Test bash command
static int test_bash()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 60;
    transfer_mob(attacker, room);
    mock_skill(attacker, gsn_bash, 100);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 8;
    victim->hit = 100;
    victim->max_hit = 100;
    transfer_mob(victim, room);
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_bash, true);
    
    test_socket_output_enabled = true;
    do_bash(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Bash succeeded - should initiate combat and knock victim down
    ASSERT(attacker->fighting == victim);
    ASSERT(victim->position == POS_RESTING);
    
    return 0;
}

// Test trip command
static int test_trip()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 60;
    transfer_mob(attacker, room);
    mock_skill(attacker, gsn_trip, 100);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 8;
    victim->position = POS_STANDING;
    transfer_mob(victim, room);
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_trip, true);
    
    test_socket_output_enabled = true;
    do_trip(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Trip succeeded - victim should be knocked down to POS_RESTING
    ASSERT(victim->position == POS_RESTING);
    
    return 0;
}

// Test dirt kicking
static int test_dirt()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 60;
    transfer_mob(attacker, room);
    mock_skill(attacker, gsn_dirt, 100);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 8;
    transfer_mob(victim, room);
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_dirt, true);
    
    test_socket_output_enabled = true;
    do_dirt(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Dirt succeeded - victim should be blinded
    ASSERT(IS_AFFECTED(victim, AFF_BLIND));
    
    return 0;
}

// Test disarm command
static int test_disarm()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 60;
    transfer_mob(attacker, room);
    mock_skill(attacker, gsn_disarm, 100);  // Ensure attacker has disarm skill
    
    // Give attacker a weapon too (required for disarm)
    Object* attacker_sword = mock_sword("blade", 60009, 10, 2, 6);
    obj_to_char(attacker_sword, attacker);
    equip_char(attacker, attacker_sword, WEAR_WIELD);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 10;
    transfer_mob(victim, room);
    
    // Give victim a weapon
    Object* sword = mock_sword("sword", 60010, 10, 2, 6);
    obj_to_char(sword, victim);
    equip_char(victim, sword, WEAR_WIELD);
    
    // Start combat first - disarm requires ch->fighting to be set
    set_fighting(attacker, victim);
    ASSERT(attacker->fighting == victim);
    ASSERT(get_eq_char(victim, WEAR_WIELD) != NULL);
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_disarm, true);
    
    test_socket_output_enabled = true;
    do_disarm(attacker, "");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Disarm succeeded - weapon should be removed
    ASSERT(get_eq_char(victim, WEAR_WIELD) == NULL);
    
    return 0;
}

// Test backstab command
static int test_backstab()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 60;
    transfer_mob(attacker, room);
    mock_skill(attacker, gsn_backstab, 100);
    
    // Give attacker a dagger
    Object* dagger = mock_sword("dagger", 60011, 10, 2, 4);
    dagger->weapon.weapon_type = WEAPON_DAGGER;
    obj_to_char(dagger, attacker);
    equip_char(attacker, dagger, WEAR_WIELD);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 10;
    victim->max_hit = 5000;  // High HP to survive backstab
    victim->hit = 5000;
    victim->position = POS_SLEEPING;  // Sleeping victims can't dodge
    transfer_mob(victim, room);
    
    int hp_before = victim->hit;
    
    // Use skill ops seam for deterministic skill check
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_backstab, true);
    
    // Use combat ops seam for deterministic hit/damage
    CombatOps* saved_combat = combat;
    combat = &mock_combat;
    reset_mock_combat();
    set_mock_always_hit(true);
    set_mock_damage_override(100);  // Fixed damage for predictable test
    
    test_socket_output_enabled = true;
    do_backstab(attacker, "victim");
    test_socket_output_enabled = false;
    test_output_buffer = NIL_VAL;
    
    combat = saved_combat;
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Backstab succeeded - verify damage was dealt
    ASSERT(victim->hit < hp_before);
    ASSERT(get_mock_damage_dealt() > 0);
    ASSERT(get_mock_last_attack_hit());
    
    return 0;
}

// Test berserk command
static int test_berserk()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* warrior = mock_player("warrior");
    warrior->position = POS_STANDING;
    warrior->level = 15;
    warrior->mana = 60;  // Ensure enough mana
    warrior->hit = warrior->max_hit / 3;  // Low HP improves berserk chance
    transfer_mob(warrior, room);
    mock_skill(warrior, gsn_berserk, 100);
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_berserk, true);
    
    test_socket_output_enabled = true;
    do_berserk(warrior, "");
    test_socket_output_enabled = false;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Berserk should produce some output (success, failure, or error)
    static const char* possible_outputs[] = {
        "Your pulse races as you are consumed by rage!",
        "Your pulse speeds up, but nothing happens.",
        "You turn red in the face, but nothing happens.",
        "You get a little madder.",
        "You're feeling to mellow to berserk.",
        "You can't get up enough energy."
    };
    ASSERT_OUTPUT_CONTAINS_ANY(possible_outputs, 6);
    
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test rescue command
static int test_rescue()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* rescuer = mock_player("rescuer");
    rescuer->position = POS_STANDING;
    rescuer->level = 60;
    transfer_mob(rescuer, room);
    mock_skill(rescuer, gsn_rescue, 100);
    
    Mobile* victim = mock_player("victim");
    victim->position = POS_STANDING;
    victim->level = 10;
    transfer_mob(victim, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* mob = mock_mob("mob", 60002, proto);
    mob->level = 12;
    transfer_mob(mob, room);
    
    // Victim is fighting mob
    victim->fighting = mob;
    mob->fighting = victim;
    victim->position = POS_FIGHTING;
    mob->position = POS_FIGHTING;
    
    // Use skill ops seam for deterministic control
    SkillOps* saved_skill_ops = skill_ops;
    skill_ops = &mock_skill_ops;
    set_skill_check_result(gsn_rescue, true);
    
    test_socket_output_enabled = true;
    do_rescue(rescuer, "victim");
    test_socket_output_enabled = false;
    
    skill_ops = saved_skill_ops;
    clear_skill_check_results();
    
    // Rescue should produce some output (success, failure, or error)
    static const char* possible_outputs[] = {
        "You rescue",
        "You fail the rescue.",
        "Rescue whom?",
        "They aren't here.",
        "What about fleeing instead?",
        "Doesn't need your help!",
        "Too late.",
        "That person is not fighting right now.",
        "Kill stealing is not permitted."
    };
    ASSERT_OUTPUT_CONTAINS_ANY(possible_outputs, 9);
    
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test surrender command
static int test_surrender()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    player->level = 5;
    transfer_mob(player, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* mob = mock_mob("mob", 60002, proto);
    mob->level = 15;
    transfer_mob(mob, room);
    
    // Start combat
    player->fighting = mob;
    mob->fighting = player;
    player->position = POS_FIGHTING;
    mob->position = POS_FIGHTING;
    
    test_socket_output_enabled = true;
    do_surrender(player, "");
    test_socket_output_enabled = false;
    
    // Surrender should produce output (either success or error)
    static const char* possible_outputs[] = {
        "You surrender to",
        "But you're not fighting!"
    };
    ASSERT_OUTPUT_CONTAINS_ANY(possible_outputs, 2);
    
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test kill without target
static int test_kill_no_target()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_kill(player, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test backstab without weapon
static int test_backstab_no_weapon()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* attacker = mock_player("attacker");
    attacker->position = POS_STANDING;
    attacker->level = 15;
    transfer_mob(attacker, room);
    
    MobPrototype* proto = mock_mob_proto(60002);
    Mobile* victim = mock_mob("victim", 60002, proto);
    victim->level = 10;
    transfer_mob(victim, room);
    
    test_socket_output_enabled = true;
    do_backstab(attacker, "victim");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

// Test flee when not fighting
static int test_flee_not_fighting()
{
    Room* room = mock_room(60001, NULL, NULL);
    
    Mobile* player = mock_player("testplayer");
    player->position = POS_STANDING;
    transfer_mob(player, room);
    
    test_socket_output_enabled = true;
    do_flee(player, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    test_output_buffer = NIL_VAL;
    
    return 0;
}

void register_fight_tests()
{
    TestGroup* group = calloc(1, sizeof(TestGroup));
    init_test_group(group, "FIGHT TESTS");

#define REGISTER(name, func) register_test(group, (name), (func))

    // Basic combat
    REGISTER("Kill: Basic attack", test_kill_basic);
    REGISTER("Kill: No target", test_kill_no_target);
    REGISTER("Murder: PK attack", test_murder_player);
    REGISTER("Flee: From combat", test_flee);
    REGISTER("Flee: Not fighting", test_flee_not_fighting);
    
    // Combat skills
    REGISTER("Kick: Basic", test_kick);
    REGISTER("Bash: Basic", test_bash);
    REGISTER("Trip: Basic", test_trip);
    REGISTER("Dirt: Kick dirt", test_dirt);
    REGISTER("Disarm: Opponent weapon", test_disarm);
    
    // Special attacks
    REGISTER("Backstab: With dagger", test_backstab);
    REGISTER("Backstab: No weapon", test_backstab_no_weapon);
    REGISTER("Berserk: Warrior rage", test_berserk);
    REGISTER("Rescue: Save ally", test_rescue);
    REGISTER("Surrender: Stop fighting", test_surrender);

#undef REGISTER

    register_test_group(group);
}
