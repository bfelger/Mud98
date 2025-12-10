////////////////////////////////////////////////////////////////////////////////
// tests/money_tests.c
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <act_obj.h>
#include <comm.h>
#include <db.h>
#include <handler.h>
#include <interp.h>
#include <fight.h>
#include <entities/shop_data.h>

#include <string.h>

static TestGroup money_tests;

static ShopData* setup_basic_weapon_shop(Mobile* keeper)
{
    ShopData* shop = new_shop_data();
    keeper->prototype->pShop = shop;
    shop->profit_buy = 100;
    shop->profit_sell = 100;
    shop->open_hour = 0;
    shop->close_hour = 23;
    for (int i = 0; i < MAX_TRADE; ++i)
        shop->buy_type[i] = ITEM_WEAPON;
    return shop;
}

static Object* find_room_corpse(Room* room)
{
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC)
            return obj;
    }
    return NULL;
}

static Mobile* setup_healer(Room* room)
{
    Mobile* healer = mock_mob("Healer", 4000, NULL);
    SET_BIT(healer->act_flags, ACT_IS_HEALER);
    transfer_mob(healer, room);
    return healer;
}


static int test_get_collects_money_object()
{
    Room* room = mock_room(9700, NULL, NULL);
    Mobile* player = mock_player("Banker");
    transfer_mob(player, room);

    Object* coins = create_money(2, 3, 4);
    obj_to_room(coins, room);

    player->gold = 0;
    player->silver = 0;
    player->copper = 0;

    get_obj(player, coins, NULL);

    ASSERT(player->gold == 2);
    ASSERT(player->silver == 3);
    ASSERT(player->copper == 4);

    return 0;
}

static int test_give_copper_transfers_money()
{
    Room* room = mock_room(9800, NULL, NULL);
    Mobile* giver = mock_player("Giver");
    Mobile* receiver = mock_player("Receiver");
    transfer_mob(giver, room);
    transfer_mob(receiver, room);

    giver->copper = 10;
    receiver->copper = 0;

    interpret(giver, safe_arg("give 5 copper Receiver"));

    ASSERT(giver->copper == 5);
    ASSERT(receiver->copper == 5);

    return 0;
}

static int test_drop_copper_creates_money_object()
{
    Room* room = mock_room(9900, NULL, NULL);
    Mobile* dropper = mock_player("Dropper");
    transfer_mob(dropper, room);

    dropper->gold = 0;
    dropper->silver = 0;
    dropper->copper = 10;

    interpret(dropper, safe_arg("drop 5 copper"));

    ASSERT(dropper->copper == 5);

    Object* money = NULL;
    FOR_EACH_ROOM_OBJ(money, room) {
        break;
    }

    ASSERT(money != NULL);
    ASSERT(money->item_type == ITEM_MONEY);
    ASSERT(money->value[MONEY_VALUE_COPPER] == 5);
    ASSERT(money->value[MONEY_VALUE_SILVER] == 0);
    ASSERT(money->value[MONEY_VALUE_GOLD] == 0);

    return 0;
}

static int test_split_divides_money_between_group()
{
    Room* room = mock_room(9950, NULL, NULL);
    Mobile* leader = mock_player("Leader");
    Mobile* member = mock_player("Member");
    transfer_mob(leader, room);
    transfer_mob(member, room);

    member->leader = leader;

    leader->gold = 1;
    leader->silver = 5;
    leader->copper = 10;

    member->gold = 0;
    member->silver = 0;
    member->copper = 0;

    interpret(leader, safe_arg("split 1 gold 5 silver 10 copper"));

    ASSERT(leader->gold == 0);
    ASSERT(leader->silver == 52);
    ASSERT(leader->copper == 55);

    ASSERT(member->gold == 0);
    ASSERT(member->silver == 52);
    ASSERT(member->copper == 55);

    return 0;
}


static int test_buy_from_shop_updates_money()
{
    Room* room = mock_room(9960, NULL, NULL);
    Mobile* buyer = mock_player("Buyer");
    Mobile* keeper = mock_mob("Merchant", 2000, NULL);
    transfer_mob(buyer, room);
    transfer_mob(keeper, room);

    ShopData* shop = setup_basic_weapon_shop(keeper);

    Object* sword = mock_sword("practice sword", 2001, 1, 1, 4);
    sword->cost = 10;
    obj_to_char(sword, keeper);

    long initial_player = convert_money_to_copper(1, 0, 0);
    mobile_set_money_from_copper(buyer, initial_player);
    long initial_keeper = mobile_total_copper(keeper);

    int expected_silver = sword->cost * shop->profit_buy / 100;
    long expected_copper = (long)expected_silver * COPPER_PER_SILVER;

    do_buy(buyer, safe_arg("practice"));

    ASSERT(mobile_total_copper(buyer) == initial_player - expected_copper);
    ASSERT(mobile_total_copper(keeper) == initial_keeper + expected_copper);
    ASSERT(get_obj_carry(buyer, "practice sword") != NULL);

    return 0;
}

static int test_buy_multiple_non_inventory_items()
{
    Room* room = mock_room(9965, NULL, NULL);
    Mobile* buyer = mock_player("BulkBuyer");
    Mobile* keeper = mock_mob("Merchant", 2002, NULL);
    transfer_mob(buyer, room);
    transfer_mob(keeper, room);

    setup_basic_weapon_shop(keeper);

    Object* sword1 = mock_sword("bulk sword", 2002, 1, 1, 4);
    Object* sword2 = new_object();
    clone_object(sword1, sword2);
    sword1->cost = 5;
    sword2->cost = 5;
    obj_to_char(sword1, keeper);
    obj_to_char(sword2, keeper);

    mobile_set_money_from_copper(buyer, convert_money_to_copper(0, 20, 0));

    do_buy(buyer, safe_arg("2 bulk"));

    int count = 0;
    Object* item;
    FOR_EACH_MOB_OBJ(item, buyer) {
        if (is_name("bulk", NAME_STR(item)))
            count++;
    }
    ASSERT(count == 2);
    ASSERT(get_obj_carry(keeper, "bulk sword") == NULL);

    return 0;
}

static int test_sell_to_shop_updates_money()
{
    Room* room = mock_room(9970, NULL, NULL);
    Mobile* seller = mock_player("Seller");
    Mobile* keeper = mock_mob("Merchant", 2100, NULL);
    transfer_mob(seller, room);
    transfer_mob(keeper, room);

    ShopData* shop = setup_basic_weapon_shop(keeper);

    Object* sword = mock_sword("merchant sword", 2101, 1, 1, 4);
    sword->cost = 12;
    obj_to_char(sword, seller);

    long keeper_initial = convert_money_to_copper(5, 0, 0);
    mobile_set_money_from_copper(keeper, keeper_initial);
    long seller_initial = mobile_total_copper(seller);

    int expected_silver = sword->cost * shop->profit_sell / 100;
    ASSERT(expected_silver > 0);
    long expected_copper = (long)expected_silver * COPPER_PER_SILVER;

    do_sell(seller, "merchant");

    ASSERT(mobile_total_copper(seller) == seller_initial + expected_copper);
    ASSERT(mobile_total_copper(keeper) == keeper_initial - expected_copper);
    ASSERT(get_obj_carry(seller, "merchant sword") == NULL);

    return 0;
}

static int test_worth_displays_all_denominations()
{
    Room* room = mock_room(9980, NULL, NULL);
    Mobile* player = mock_player("Investor");
    transfer_mob(player, room);

    player->gold = 1;
    player->silver = 2;
    player->copper = 3;
    player->level = 5;
    player->exp = 250;

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    interpret(player, "worth");
    test_socket_output_enabled = false;
    ASSERT(IS_STRING(test_output_buffer));
    ASSERT(strstr(AS_STRING(test_output_buffer)->chars,
        "You have 1 gold coin, 2 silver coins, and 3 copper coins, and 250 experience") != NULL);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_score_displays_all_denominations()
{
    Room* room = mock_room(9990, NULL, NULL);
    Mobile* player = mock_player("Scorer");
    transfer_mob(player, room);

    player->gold = 4;
    player->silver = 5;
    player->copper = 6;
    player->level = 3;
    player->exp = 123;

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    interpret(player, "score");
    test_socket_output_enabled = false;
    ASSERT(IS_STRING(test_output_buffer));
    ASSERT(strstr(AS_STRING(test_output_buffer)->chars,
        "You have scored 123 exp, and have 4 gold coins, 5 silver coins, and 6 copper coins.") != NULL);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_prompt_displays_all_denominations()
{
    Room* room = mock_room(9995, NULL, NULL);
    Mobile* player = mock_player("Prompted");
    transfer_mob(player, room);

    player->gold = 7;
    player->silver = 8;
    player->copper = 9;
    if (player->prompt != NULL)
        free_string(player->prompt);
    player->prompt = str_dup("%g gold %s silver %C copper>");

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    bust_a_prompt(player);
    test_socket_output_enabled = false;

    ASSERT(IS_STRING(test_output_buffer));
    const char* out = AS_STRING(test_output_buffer)->chars;
    ASSERT(strstr(out, "7 gold 8 silver 9 copper>") != NULL);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_heal_price_list_shows_formatted_money()
{
    Room* room = mock_room(10000, NULL, NULL);
    Mobile* player = mock_player("Patient");
    transfer_mob(player, room);
    setup_healer(room);

    test_output_buffer = NIL_VAL;
    test_socket_output_enabled = true;
    interpret(player, "heal");
    test_socket_output_enabled = false;

    ASSERT(IS_STRING(test_output_buffer));
    const char* out = AS_STRING(test_output_buffer)->chars;
    ASSERT(strstr(out, "cure light wounds") != NULL);
    ASSERT(strstr(out, "restore movement") != NULL);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_heal_light_deducts_money()
{
    Room* room = mock_room(10010, NULL, NULL);
    Mobile* player = mock_player("Patient");
    transfer_mob(player, room);
    Mobile* healer = setup_healer(room);

    long price = (long)10 * COPPER_PER_SILVER;
    mobile_set_money_from_copper(player, price * 2);

    test_output_buffer = NIL_VAL;
    interpret(player, "heal light");

    ASSERT(mobile_total_copper(player) == price);
    ASSERT(mobile_total_copper(healer) == price);

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_sacrifice_rewards_money()
{
    Room* room = mock_room(10020, NULL, NULL);
    Mobile* player = mock_player("Priest");
    transfer_mob(player, room);

    Object* idol = mock_obj("idol", 6000, NULL);
    idol->level = 10;
    idol->cost = 200;
    idol->wear_flags = ITEM_TAKE;
    obj_to_room(idol, room);
    int idol_level = idol->level;
    int idol_cost = idol->cost;

    mobile_set_money_from_copper(player, 0);
    long initial = mobile_total_copper(player);
    do_sacrifice(player, "idol");

    long reward_copper = UMIN(UMAX(1, idol_level * 3), idol_cost);
    long actual = mobile_total_copper(player);
    ASSERT(actual == initial + reward_copper);

    return 0;
}

static int test_make_corpse_npc_transfers_money()
{
    Room* room = mock_room(10100, NULL, NULL);
    Mobile* mob = mock_mob("Grunt", 5000, NULL);
    transfer_mob(mob, room);

    mob->gold = 2;
    mob->silver = 3;
    mob->copper = 4;

    long expected = convert_money_to_copper(mob->gold, mob->silver, mob->copper);

    make_corpse(mob);

    Object* corpse = find_room_corpse(room);
    ASSERT(corpse != NULL);

    Object* money = NULL;
    FOR_EACH_OBJ_CONTENT(money, corpse) {
        if (money->item_type == ITEM_MONEY)
            break;
    }

    ASSERT(money != NULL);
    long corpse_amount = convert_money_to_copper(
        money->value[MONEY_VALUE_GOLD],
        money->value[MONEY_VALUE_SILVER],
        money->value[MONEY_VALUE_COPPER]);

    ASSERT(corpse_amount == expected);
    ASSERT(mobile_total_copper(mob) == 0);

    return 0;
}

static int test_make_corpse_clan_player_drops_half()
{
    Room* room = mock_room(10110, NULL, NULL);
    Mobile* player = mock_player("ClanHero");
    transfer_mob(player, room);

    player->clan = 2; // non-independent clan

    long initial = convert_money_to_copper(4, 5, 6);
    mobile_set_money_from_copper(player, initial);

    make_corpse(player);

    Object* corpse = find_room_corpse(room);
    ASSERT(corpse != NULL);
    ASSERT(corpse->owner == NULL);

    Object* money = NULL;
    FOR_EACH_OBJ_CONTENT(money, corpse) {
        if (money->item_type == ITEM_MONEY)
            break;
    }

    long expected_drop = initial / 2;
    ASSERT(mobile_total_copper(player) == initial - expected_drop);
    if (expected_drop > 0) {
        ASSERT(money != NULL);
        long corpse_amount = convert_money_to_copper(
            money->value[MONEY_VALUE_GOLD],
            money->value[MONEY_VALUE_SILVER],
            money->value[MONEY_VALUE_COPPER]);
        ASSERT(corpse_amount == expected_drop);
    }

    return 0;
}

static int test_steal_coins_transfers_money()
{
    Room* room = mock_room(10030, NULL, NULL);
    Mobile* thief = mock_player("Sneak");
    Mobile* victim = mock_player("Victim");
    transfer_mob(thief, room);
    transfer_mob(victim, room);

    long victim_initial = convert_money_to_copper(5, 10, 0);
    mobile_set_money_from_copper(victim, victim_initial);
    mobile_set_money_from_copper(thief, 0);

    long stolen = convert_money_to_copper(1, 2, 3);
    steal_coins_transfer(thief, victim, stolen);

    ASSERT(mobile_total_copper(thief) == stolen);
    ASSERT(mobile_total_copper(victim) == victim_initial - stolen);

    return 0;
}

static int test_money_union_semantics()
{
    Room* room = mock_room(9950, NULL, NULL);
    Mobile* ch = mock_player("TestPlayer");
    transfer_mob(ch, room);
    
    // Create money object with specific values
    Object* money = create_money(50, 100, 250);  // 50g, 100s, 250c
    
    // Verify array access matches union access
    ASSERT(money->value[MONEY_VALUE_COPPER] == 250);
    ASSERT(money->value[MONEY_VALUE_SILVER] == 100);
    ASSERT(money->value[MONEY_VALUE_GOLD] == 50);
    
    // Verify union members match
    ASSERT(money->money.copper == 250);
    ASSERT(money->money.silver == 100);
    ASSERT(money->money.gold == 50);
    
    // Verify they're the same memory
    ASSERT(money->value[0] == money->money.copper);
    ASSERT(money->value[1] == money->money.silver);
    ASSERT(money->value[2] == money->money.gold);
    
    extract_obj(money);
    return 0;
}

void register_money_tests()
{
#define REGISTER(name, fn) register_test(&money_tests, (name), (fn))

    init_test_group(&money_tests, "MONEY TESTS");
    register_test_group(&money_tests);

    REGISTER("Money Union Semantics", test_money_union_semantics);
    REGISTER("Get Collects Money Object", test_get_collects_money_object);
    REGISTER("Give Copper Transfers Money", test_give_copper_transfers_money);
    REGISTER("Drop Copper Creates Money Object", test_drop_copper_creates_money_object);
    REGISTER("Split Divides Money Between Group", test_split_divides_money_between_group);
    REGISTER("Buy From Shop Updates Money", test_buy_from_shop_updates_money);
    REGISTER("Buy Multiple Non Inventory Items", test_buy_multiple_non_inventory_items);
    REGISTER("Sell To Shop Updates Money", test_sell_to_shop_updates_money);
    REGISTER("Worth Displays All Denominations", test_worth_displays_all_denominations);
    REGISTER("Score Displays All Denominations", test_score_displays_all_denominations);
    REGISTER("Prompt Displays All Denominations", test_prompt_displays_all_denominations);
    REGISTER("Heal Price List Shows Formatted Money", test_heal_price_list_shows_formatted_money);
    REGISTER("Heal Light Deducts Money", test_heal_light_deducts_money);
    REGISTER("Sacrifice Rewards Money", test_sacrifice_rewards_money);
    REGISTER("Make Corpse NPC Transfers Money", test_make_corpse_npc_transfers_money);
    REGISTER("Make Corpse Clan Player Drops Half", test_make_corpse_clan_player_drops_half);
    REGISTER("Steal Coins Transfers Money", test_steal_coins_transfers_money);

#undef REGISTER
}
