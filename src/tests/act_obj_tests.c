////////////////////////////////////////////////////////////////////////////////
// act_obj_tests.c
//
// Tests for act_obj.c object manipulation commands.
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "lox_tests.h"
#include "mock.h"

#include <act_obj.h>
#include <comm.h>
#include <handler.h>
#include <lookup.h>

#include <data/class.h>
#include <data/item.h>
#include <data/mobile_data.h>
#include <data/race.h>
#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/player_data.h>
#include <entities/room.h>

TestGroup act_obj_tests;

static int test_get_nothing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_get(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Get what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_get_object_from_room()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    Object* sword = mock_sword("sword", 1, 1, 2, 4);
    sword->wear_flags = ITEM_TAKE;
    transfer_mob(ch, room);
    obj_to_room(sword, room);
    
    test_socket_output_enabled = true;
    do_get(ch, "sword");
    test_socket_output_enabled = false;
    
    ASSERT(sword->carried_by == ch);
    ASSERT_OUTPUT_CONTAINS("You get");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_get_not_found()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_get(ch, "nonexistent");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("I see no");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_drop_nothing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_drop(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Drop what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_drop_object()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    Object* sword = mock_sword("sword", 1, 1, 2, 4);
    transfer_mob(ch, room);
    obj_to_char(sword, ch);
    
    test_socket_output_enabled = true;
    do_drop(ch, "sword");
    test_socket_output_enabled = false;
    
    ASSERT(sword->in_room == room);
    ASSERT_OUTPUT_CONTAINS("You drop");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_wear_nothing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_wear(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Wear, wield, or hold what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_wear_sword()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    Object* sword = mock_sword("sword", 1, 1, 2, 4);
    sword->wear_flags = ITEM_TAKE | ITEM_WIELD;
    transfer_mob(ch, room);
    obj_to_char(sword, ch);
    
    test_socket_output_enabled = true;
    do_wear(ch, "sword");
    test_socket_output_enabled = false;
    
    ASSERT(sword->wear_loc == WEAR_WIELD);
    ASSERT_OUTPUT_CONTAINS("You wield");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_wear_armor_requires_proficiency()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    ch->level = 10;
    ch->ch_class = class_lookup("mage");

    Object* armor = mock_obj("plate armor", 60001, NULL);
    armor->item_type = ITEM_ARMOR;
    armor->wear_flags = ITEM_TAKE | ITEM_WEAR_BODY;
    armor->armor.ac_pierce = -5;
    armor->armor.ac_bash = -5;
    armor->armor.ac_slash = -5;
    armor->armor.ac_exotic = -5;
    armor->armor.armor_type = ARMOR_HEAVY;
    obj_to_char(armor, ch);

    test_socket_output_enabled = true;
    do_wear(ch, "plate");
    test_socket_output_enabled = false;

    ASSERT(armor->wear_loc == WEAR_UNHELD);
    ASSERT_OUTPUT_CONTAINS("You are not trained to wear that kind of armor.");
    test_output_buffer = NIL_VAL;

    return 0;
}

static int test_armor_prof_race_overrides_class()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);

    int class_idx = class_lookup("mage");
    int16_t race_idx = race_lookup("human");
    ASSERT(class_idx >= 0);
    ASSERT(race_idx >= 0);
    ASSERT(class_table != NULL);
    ASSERT(race_table != NULL);
    ASSERT(class_table[class_idx].name != NULL);
    ASSERT(race_table[race_idx].name != NULL);

    ArmorTier old_class_prof = class_table[class_idx].armor_prof;
    ArmorTier old_race_prof = race_table[race_idx].armor_prof;

    class_table[class_idx].armor_prof = ARMOR_CLOTH;
    race_table[race_idx].armor_prof = ARMOR_HEAVY;

    ch->ch_class = class_idx;
    ch->race = race_idx;
    ch->pcdata->armor_prof = ARMOR_OLD_STYLE;

    grant_armor_prof(ch, class_table[class_idx].armor_prof);
    grant_armor_prof(ch, race_table[race_idx].armor_prof);

    ArmorTier prof = get_armor_prof(ch);

    class_table[class_idx].armor_prof = old_class_prof;
    race_table[race_idx].armor_prof = old_race_prof;

    ASSERT(prof == ARMOR_HEAVY);

    return 0;
}

static int test_remove_nothing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_remove(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Remove what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_remove_sword()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    Object* sword = mock_sword("sword", 1, 1, 2, 4);
    sword->wear_flags = ITEM_TAKE | ITEM_WIELD;
    transfer_mob(ch, room);
    obj_to_char(sword, ch);
    equip_char(ch, sword, WEAR_WIELD);
    
    ASSERT(sword->wear_loc == WEAR_WIELD);
    
    test_socket_output_enabled = true;
    do_remove(ch, "sword");
    test_socket_output_enabled = false;
    
    ASSERT(sword->wear_loc == WEAR_UNHELD);
    ASSERT_OUTPUT_CONTAINS("You stop using");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_sacrifice_nothing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_sacrifice(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Mota appreciates your offer");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_sacrifice_object()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    Object* obj = mock_obj("item", 1, NULL);
    obj->wear_flags = ITEM_TAKE;
    transfer_mob(ch, room);
    obj_to_room(obj, room);
    
    test_socket_output_enabled = true;
    do_sacrifice(ch, "item");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Mota gives you");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_put_nothing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_put(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Put what in what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_put_object_in_container()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    Object* container = mock_obj("bag", 1, NULL);
    Object* item = mock_obj("gem", 2, NULL);
    
    container->item_type = ITEM_CONTAINER;
    container->wear_flags = ITEM_TAKE;
    container->container.capacity = 100;         // Max weight capacity in pounds
    container->container.max_item_weight = 50;   // Max item weight in pounds
    container->container.weight_mult = 100;      // 100% weight (standard)
    
    item->wear_flags = ITEM_TAKE;
    item->weight = 10;  // 1 pound (weight is in tenths of pounds)
    
    transfer_mob(ch, room);
    obj_to_char(container, ch);
    obj_to_char(item, ch);
    
    test_socket_output_enabled = true;
    do_put(ch, "gem bag");
    test_socket_output_enabled = false;
    
    ASSERT(item->in_obj == container);
    ASSERT_OUTPUT_CONTAINS("You put");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_give_nothing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_give(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Give what to whom?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_give_to_mob()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    MobPrototype* mp = mock_mob_proto(1);
    Mobile* target = mock_mob("guard", 1, mp);
    Object* item = mock_obj("coin", 1, NULL);
    
    item->wear_flags = ITEM_TAKE;
    
    transfer_mob(ch, room);
    transfer_mob(target, room);
    obj_to_char(item, ch);
    
    test_socket_output_enabled = true;
    do_give(ch, "coin guard");
    test_socket_output_enabled = false;
    
    ASSERT(item->carried_by == target);
    ASSERT_OUTPUT_CONTAINS("You give");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_fill_nothing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_fill(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Fill what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_fill_from_fountain()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    Object* fountain = mock_obj("fountain", 1, NULL);
    Object* waterskin = mock_obj("waterskin", 2, NULL);
    
    fountain->item_type = ITEM_FOUNTAIN;
    fountain->fountain.capacity = 100;      // Max capacity
    fountain->fountain.current = 100;       // Current amount (full)
    fountain->fountain.liquid_type = LIQ_WATER;
    
    waterskin->item_type = ITEM_DRINK_CON;
    waterskin->drink_con.capacity = 10;     // Max capacity
    waterskin->drink_con.current = 0;       // Current amount (empty)
    waterskin->drink_con.liquid_type = LIQ_WATER;
    waterskin->wear_flags = ITEM_TAKE;
    
    transfer_mob(ch, room);
    obj_to_room(fountain, room);
    obj_to_char(waterskin, ch);
    
    test_socket_output_enabled = true;
    do_fill(ch, "waterskin fountain");
    test_socket_output_enabled = false;
    
    ASSERT(waterskin->drink_con.current == 10);  // Now full
    ASSERT_OUTPUT_CONTAINS("You fill");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_drink_nothing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_drink(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Drink what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_drink_from_container()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    Object* waterskin = mock_obj("waterskin", 1, NULL);
    
    waterskin->item_type = ITEM_DRINK_CON;
    waterskin->drink_con.capacity = 10;     // Max capacity
    waterskin->drink_con.current = 10;      // Current amount (full)
    waterskin->drink_con.liquid_type = LIQ_WATER;
    waterskin->wear_flags = ITEM_TAKE;
    
    ch->pcdata->condition[COND_THIRST] = 10;
    
    transfer_mob(ch, room);
    obj_to_char(waterskin, ch);
    
    test_socket_output_enabled = true;
    do_drink(ch, "waterskin");
    test_socket_output_enabled = false;
    
    ASSERT(waterskin->drink_con.current < 10);  // Some consumed
    ASSERT(ch->pcdata->condition[COND_THIRST] > 10);  // Less thirsty
    ASSERT_OUTPUT_CONTAINS("You drink");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_eat_nothing()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_eat(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Eat what?");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

static int test_eat_food()
{
    Room* room = mock_room(50000, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    Object* bread = mock_obj("bread", 1, NULL);
    
    bread->item_type = ITEM_FOOD;
    bread->food.hours_full = 5;      // Hours of fullness
    bread->food.hours_hunger = 5;    // Hours reduces hunger
    bread->food.poisoned = 0;        // Not poisoned
    bread->wear_flags = ITEM_TAKE;
    
    ch->pcdata->condition[COND_FULL] = 10;    // Not too full
    ch->pcdata->condition[COND_HUNGER] = 10;  // Somewhat hungry
    
    int hunger_before = ch->pcdata->condition[COND_HUNGER];
    
    transfer_mob(ch, room);
    obj_to_char(bread, ch);
    
    test_socket_output_enabled = true;
    do_eat(ch, "bread");
    test_socket_output_enabled = false;
    
    // Note: food object is destroyed after eating
    ASSERT(ch->pcdata->condition[COND_HUNGER] != hunger_before);  // Hunger changed
    ASSERT_OUTPUT_CONTAINS("You eat");
    test_output_buffer = NIL_VAL;
    
    return 0;
}

void register_act_obj_tests()
{
#define REGISTER(n, f)  register_test(&act_obj_tests, (n), (f))

    init_test_group(&act_obj_tests, "ACT OBJ TESTS");
    register_test_group(&act_obj_tests);

    REGISTER("Cmd: Get nothing", test_get_nothing);
    REGISTER("Cmd: Get object from room", test_get_object_from_room);
    REGISTER("Cmd: Get not found", test_get_not_found);
    REGISTER("Cmd: Drop nothing", test_drop_nothing);
    REGISTER("Cmd: Drop object", test_drop_object);
    REGISTER("Cmd: Wear nothing", test_wear_nothing);
    REGISTER("Cmd: Wear sword", test_wear_sword);
    REGISTER("Cmd: Wear armor requires proficiency", test_wear_armor_requires_proficiency);
    REGISTER("Armor prof: Race overrides class", test_armor_prof_race_overrides_class);
    REGISTER("Cmd: Remove nothing", test_remove_nothing);
    REGISTER("Cmd: Remove sword", test_remove_sword);
    REGISTER("Cmd: Sacrifice nothing", test_sacrifice_nothing);
    REGISTER("Cmd: Sacrifice object", test_sacrifice_object);
    REGISTER("Cmd: Put nothing", test_put_nothing);
    REGISTER("Cmd: Put object in container", test_put_object_in_container);
    REGISTER("Cmd: Give nothing", test_give_nothing);
    REGISTER("Cmd: Give to mob", test_give_to_mob);
    REGISTER("Cmd: Fill nothing", test_fill_nothing);
    REGISTER("Cmd: Fill from fountain", test_fill_from_fountain);
    REGISTER("Cmd: Drink nothing", test_drink_nothing);
    REGISTER("Cmd: Drink from container", test_drink_from_container);
    REGISTER("Cmd: Eat nothing", test_eat_nothing);
    REGISTER("Cmd: Eat food", test_eat_food);

#undef REGISTER
}
