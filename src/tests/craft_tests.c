////////////////////////////////////////////////////////////////////////////////
// craft_tests.c
//
// Tests for the crafting system (src/craft/)
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <craft/craft.h>
#include <craft/recipe.h>

#include <data/item.h>

#include <entities/object.h>
#include <entities/room.h>

TestGroup craft_tests;

////////////////////////////////////////////////////////////////////////////////
// Issue #1: Crafting Enum Tests
////////////////////////////////////////////////////////////////////////////////

static int test_craft_mat_type_names()
{
    // Test that all material types have valid names
    ASSERT_STR_EQ("none", craft_mat_type_name(MAT_NONE));
    ASSERT_STR_EQ("hide", craft_mat_type_name(MAT_HIDE));
    ASSERT_STR_EQ("leather", craft_mat_type_name(MAT_LEATHER));
    ASSERT_STR_EQ("fur", craft_mat_type_name(MAT_FUR));
    ASSERT_STR_EQ("scale", craft_mat_type_name(MAT_SCALE));
    ASSERT_STR_EQ("bone", craft_mat_type_name(MAT_BONE));
    ASSERT_STR_EQ("meat", craft_mat_type_name(MAT_MEAT));
    ASSERT_STR_EQ("ore", craft_mat_type_name(MAT_ORE));
    ASSERT_STR_EQ("ingot", craft_mat_type_name(MAT_INGOT));
    ASSERT_STR_EQ("gem", craft_mat_type_name(MAT_GEM));
    ASSERT_STR_EQ("cloth", craft_mat_type_name(MAT_CLOTH));
    ASSERT_STR_EQ("wood", craft_mat_type_name(MAT_WOOD));
    ASSERT_STR_EQ("herb", craft_mat_type_name(MAT_HERB));
    ASSERT_STR_EQ("essence", craft_mat_type_name(MAT_ESSENCE));
    ASSERT_STR_EQ("component", craft_mat_type_name(MAT_COMPONENT));
    
    return 0;
}

static int test_craft_mat_type_lookup()
{
    // Test string to enum lookup (case-insensitive)
    ASSERT(craft_mat_type_lookup("hide") == MAT_HIDE);
    ASSERT(craft_mat_type_lookup("HIDE") == MAT_HIDE);
    ASSERT(craft_mat_type_lookup("Hide") == MAT_HIDE);
    ASSERT(craft_mat_type_lookup("leather") == MAT_LEATHER);
    ASSERT(craft_mat_type_lookup("ingot") == MAT_INGOT);
    ASSERT(craft_mat_type_lookup("wood") == MAT_WOOD);
    
    // Invalid lookup returns MAT_NONE
    ASSERT(craft_mat_type_lookup("invalid") == MAT_NONE);
    ASSERT(craft_mat_type_lookup("") == MAT_NONE);
    ASSERT(craft_mat_type_lookup(NULL) == MAT_NONE);
    
    return 0;
}

static int test_workstation_type_names()
{
    // Test workstation flag names
    ASSERT_STR_EQ("none", workstation_type_name(WORK_NONE));
    ASSERT_STR_EQ("forge", workstation_type_name(WORK_FORGE));
    ASSERT_STR_EQ("smelter", workstation_type_name(WORK_SMELTER));
    ASSERT_STR_EQ("tannery", workstation_type_name(WORK_TANNERY));
    ASSERT_STR_EQ("loom", workstation_type_name(WORK_LOOM));
    ASSERT_STR_EQ("alchemy", workstation_type_name(WORK_ALCHEMY));
    ASSERT_STR_EQ("cooking", workstation_type_name(WORK_COOKING));
    ASSERT_STR_EQ("enchant", workstation_type_name(WORK_ENCHANT));
    ASSERT_STR_EQ("woodwork", workstation_type_name(WORK_WOODWORK));
    ASSERT_STR_EQ("jeweler", workstation_type_name(WORK_JEWELER));
    
    return 0;
}

static int test_workstation_type_lookup()
{
    // Test string to flag lookup (case-insensitive)
    ASSERT(workstation_type_lookup("forge") == WORK_FORGE);
    ASSERT(workstation_type_lookup("FORGE") == WORK_FORGE);
    ASSERT(workstation_type_lookup("Forge") == WORK_FORGE);
    ASSERT(workstation_type_lookup("smelter") == WORK_SMELTER);
    ASSERT(workstation_type_lookup("tannery") == WORK_TANNERY);
    
    // Invalid lookup returns WORK_NONE
    ASSERT(workstation_type_lookup("invalid") == WORK_NONE);
    ASSERT(workstation_type_lookup("") == WORK_NONE);
    ASSERT(workstation_type_lookup(NULL) == WORK_NONE);
    
    return 0;
}

static int test_discovery_type_names()
{
    // Test discovery type names
    ASSERT_STR_EQ("known", discovery_type_name(DISC_KNOWN));
    ASSERT_STR_EQ("trainer", discovery_type_name(DISC_TRAINER));
    ASSERT_STR_EQ("scroll", discovery_type_name(DISC_SCROLL));
    ASSERT_STR_EQ("discovery", discovery_type_name(DISC_DISCOVERY));
    ASSERT_STR_EQ("quest", discovery_type_name(DISC_QUEST));
    
    return 0;
}

static int test_discovery_type_lookup()
{
    // Test string to enum lookup (case-insensitive)
    ASSERT(discovery_type_lookup("known") == DISC_KNOWN);
    ASSERT(discovery_type_lookup("KNOWN") == DISC_KNOWN);
    ASSERT(discovery_type_lookup("trainer") == DISC_TRAINER);
    ASSERT(discovery_type_lookup("scroll") == DISC_SCROLL);
    ASSERT(discovery_type_lookup("discovery") == DISC_DISCOVERY);
    ASSERT(discovery_type_lookup("quest") == DISC_QUEST);
    
    // Invalid lookup returns DISC_KNOWN (default)
    ASSERT(discovery_type_lookup("invalid") == DISC_KNOWN);
    ASSERT(discovery_type_lookup("") == DISC_KNOWN);
    ASSERT(discovery_type_lookup(NULL) == DISC_KNOWN);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #2: Item Type Tests
////////////////////////////////////////////////////////////////////////////////

static int test_item_mat_creation()
{
    Object* mat = mock_mat("wolf hide", 70001, MAT_HIDE, 3, 75);
    
    ASSERT(mat != NULL);
    ASSERT(mat->item_type == ITEM_MAT);
    ASSERT(mat->craft_mat.mat_type == MAT_HIDE);
    ASSERT(mat->craft_mat.amount == 3);
    ASSERT(mat->craft_mat.quality == 75);
    
    return 0;
}

static int test_item_mat_type_name()
{
    // Verify ITEM_MAT is in the item type table
    const char* name = item_name(ITEM_MAT);
    ASSERT_STR_EQ("material", name);
    
    return 0;
}

static int test_item_workstation_creation()
{
    Object* ws = mock_workstation("blacksmith forge", 70002, WORK_FORGE, 5);
    
    ASSERT(ws != NULL);
    ASSERT(ws->item_type == ITEM_WORKSTATION);
    ASSERT(ws->workstation.station_flags == WORK_FORGE);
    ASSERT(ws->workstation.bonus == 5);
    
    return 0;
}

static int test_item_workstation_multi_flags()
{
    // Create a combo workstation (forge + smelter)
    Object* ws = mock_workstation("master forge", 70003, WORK_FORGE | WORK_SMELTER, 10);
    
    ASSERT(ws != NULL);
    ASSERT(ws->item_type == ITEM_WORKSTATION);
    ASSERT(IS_SET(ws->workstation.station_flags, WORK_FORGE));
    ASSERT(IS_SET(ws->workstation.station_flags, WORK_SMELTER));
    ASSERT(!IS_SET(ws->workstation.station_flags, WORK_TANNERY));
    ASSERT(ws->workstation.bonus == 10);
    
    return 0;
}

static int test_item_workstation_type_name()
{
    // Verify ITEM_WORKSTATION is in the item type table
    const char* name = item_name(ITEM_WORKSTATION);
    ASSERT_STR_EQ("workstation", name);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Recipe Tests (Basic OrderedTable functionality)
////////////////////////////////////////////////////////////////////////////////

static int test_recipe_lifecycle()
{
    // Test basic recipe creation and registration
    Recipe* recipe = new_recipe();
    ASSERT(recipe != NULL);
    
    // Set basic fields
    recipe->header.vnum = 80001;
    recipe->product_vnum = 70010;
    recipe->product_quantity = 1;
    recipe->station_type = WORK_FORGE;
    
    // Add to global table
    bool added = add_recipe(recipe);
    ASSERT(added);
    
    // Lookup by VNUM
    Recipe* found = get_recipe(80001);
    ASSERT(found == recipe);
    
    // Count should be 1
    ASSERT(recipe_count() >= 1);
    
    // Remove
    bool removed = remove_recipe(80001);
    ASSERT(removed);
    
    // Lookup should fail now
    found = get_recipe(80001);
    ASSERT(found == NULL);
    
    return 0;
}

static int test_recipe_ingredients()
{
    Recipe* recipe = new_recipe();
    recipe->header.vnum = 80002;
    
    // Add ingredients
    ASSERT(recipe_add_ingredient(recipe, 70001, 2));  // 2x hide
    ASSERT(recipe_add_ingredient(recipe, 70002, 1));  // 1x thread
    
    ASSERT(recipe->ingredient_count == 2);
    ASSERT(recipe->ingredients[0].mat_vnum == 70001);
    ASSERT(recipe->ingredients[0].quantity == 2);
    ASSERT(recipe->ingredients[1].mat_vnum == 70002);
    ASSERT(recipe->ingredients[1].quantity == 1);
    
    // Remove first ingredient
    ASSERT(recipe_remove_ingredient(recipe, 0));
    ASSERT(recipe->ingredient_count == 1);
    ASSERT(recipe->ingredients[0].mat_vnum == 70002);  // Thread shifted down
    
    // Clear all
    recipe_clear_ingredients(recipe);
    ASSERT(recipe->ingredient_count == 0);
    
    free_recipe(recipe);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Test Registration
////////////////////////////////////////////////////////////////////////////////

void register_craft_tests()
{
#define REGISTER(name, func) register_test(&craft_tests, name, func)

    init_test_group(&craft_tests, "CRAFT TESTS");
    register_test_group(&craft_tests);

    // Issue #1: Crafting Enums
    REGISTER("CraftMatType: Names", test_craft_mat_type_names);
    REGISTER("CraftMatType: Lookup", test_craft_mat_type_lookup);
    REGISTER("WorkstationType: Names", test_workstation_type_names);
    REGISTER("WorkstationType: Lookup", test_workstation_type_lookup);
    REGISTER("DiscoveryType: Names", test_discovery_type_names);
    REGISTER("DiscoveryType: Lookup", test_discovery_type_lookup);
    
    // Issue #2: Item Types
    REGISTER("ITEM_MAT: Creation", test_item_mat_creation);
    REGISTER("ITEM_MAT: Type Name", test_item_mat_type_name);
    REGISTER("ITEM_WORKSTATION: Creation", test_item_workstation_creation);
    REGISTER("ITEM_WORKSTATION: Multi Flags", test_item_workstation_multi_flags);
    REGISTER("ITEM_WORKSTATION: Type Name", test_item_workstation_type_name);
    
    // Recipe basics
    REGISTER("Recipe: Lifecycle", test_recipe_lifecycle);
    REGISTER("Recipe: Ingredients", test_recipe_ingredients);

#undef REGISTER
}
