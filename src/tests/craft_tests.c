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

#include <entities/mobile.h>
#include <entities/mob_prototype.h>
#include <entities/object.h>
#include <entities/obj_prototype.h>
#include <entities/room.h>

#include <fight.h>

#include <stdlib.h>
#include <string.h>

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

static int test_recipe_lookup_by_name()
{
    // Create recipes with names using mock helper
    Recipe* r1 = mock_recipe("leather strips", 80010);
    r1->product_vnum = 70100;
    
    Recipe* r2 = mock_recipe("iron ingot", 80011);
    r2->product_vnum = 70101;
    
    // Exact match
    Recipe* found = get_recipe_by_name("leather strips");
    ASSERT(found == r1);
    
    // Prefix match
    found = get_recipe_by_name("iron");
    ASSERT(found == r2);
    
    // Case-insensitive
    found = get_recipe_by_name("LEATHER STRIPS");
    ASSERT(found == r1);
    
    // Clean up - remove from global table
    remove_recipe(80010);
    remove_recipe(80011);
    
    return 0;
}

static int test_recipe_lookup_not_found()
{
    // VNUM that doesn't exist
    Recipe* found = get_recipe(99999);
    ASSERT(found == NULL);
    
    // Invalid VNUM
    found = get_recipe(VNUM_NONE);
    ASSERT(found == NULL);
    
    // Name that doesn't exist
    found = get_recipe_by_name("nonexistent recipe");
    ASSERT(found == NULL);
    
    // Empty name
    found = get_recipe_by_name("");
    ASSERT(found == NULL);
    
    // NULL name
    found = get_recipe_by_name(NULL);
    ASSERT(found == NULL);
    
    return 0;
}

static int test_recipe_free()
{
    // Create a recipe with ingredients
    Recipe* recipe = new_recipe();
    recipe->header.vnum = 80020;
    recipe->product_vnum = 70200;
    recipe->product_quantity = 2;
    recipe->station_type = WORK_TANNERY;
    recipe->min_level = 5;
    recipe->required_skill = 10;  // arbitrary skill number
    recipe->min_skill_pct = 25;
    
    recipe_add_ingredient(recipe, 70001, 3);
    recipe_add_ingredient(recipe, 70002, 1);
    
    ASSERT(recipe->ingredient_count == 2);
    
    // Free should not crash
    free_recipe(recipe);
    
    return 0;
}

static int test_mock_recipe()
{
    // Test the mock helper
    Recipe* recipe = mock_recipe("test mock recipe", 80030);
    ASSERT(recipe != NULL);
    ASSERT(recipe->header.vnum == 80030);
    ASSERT_STR_EQ("test mock recipe", NAME_STR(recipe));
    
    // Should be in global table
    Recipe* found = get_recipe(80030);
    ASSERT(found == recipe);
    
    // Clean up
    remove_recipe(80030);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #4: Mob Prototype Craft Mats Tests
////////////////////////////////////////////////////////////////////////////////

static int test_mob_proto_craft_mats_field()
{
    // Create a mob prototype with craft_mats using mock helper
    MobPrototype* mp = mock_mob_proto(70010);
    ASSERT(mp != NULL);
    ASSERT(mp->craft_mats == NULL);
    ASSERT(mp->craft_mat_count == 0);
    
    // Add some craft_mats
    mp->craft_mat_count = 3;
    mp->craft_mats = malloc(sizeof(VNUM) * 3);
    mp->craft_mats[0] = 1001;  // Example material VNUM
    mp->craft_mats[1] = 1002;
    mp->craft_mats[2] = 1003;
    
    ASSERT(mp->craft_mat_count == 3);
    ASSERT(mp->craft_mats[0] == 1001);
    ASSERT(mp->craft_mats[1] == 1002);
    ASSERT(mp->craft_mats[2] == 1003);
    
    // Note: Don't manually free - test framework manages via mocks()
    
    return 0;
}

static int test_mob_craft_mats_from_prototype()
{
    // Create a mob prototype with craft_mats
    MobPrototype* mp = mock_mob_proto(70001);
    ASSERT(mp != NULL);
    
    // Add craft_mats to prototype
    mp->craft_mat_count = 2;
    mp->craft_mats = malloc(sizeof(VNUM) * 2);
    mp->craft_mats[0] = 2001;
    mp->craft_mats[1] = 2002;
    
    // Create mob from prototype
    Mobile* mob = create_mobile(mp);
    ASSERT(mob != NULL);
    ASSERT(mob->prototype == mp);
    
    // Mob accesses craft_mats through its prototype pointer
    ASSERT(mob->prototype->craft_mat_count == 2);
    ASSERT(mob->prototype->craft_mats[0] == 2001);
    ASSERT(mob->prototype->craft_mats[1] == 2002);
    
    return 0;
}

static int test_corpse_inherits_craft_mats()
{
    // Set up room for corpse to be placed in
    Room* room = mock_room(70002, NULL, NULL);
    ASSERT(room != NULL);
    
    // Create a mob prototype with craft_mats
    MobPrototype* mp = mock_mob_proto(70003);
    ASSERT(mp != NULL);
    
    mp->craft_mat_count = 2;
    mp->craft_mats = malloc(sizeof(VNUM) * 2);
    mp->craft_mats[0] = 3001;
    mp->craft_mats[1] = 3002;
    
    // Create mob from prototype
    Mobile* mob = create_mobile(mp);
    ASSERT(mob != NULL);
    
    // Place mob in room (required for make_corpse)
    transfer_mob(mob, room);
    
    // Make corpse (this should copy craft_mats)
    make_corpse(mob);
    
    // Find the corpse in the room (corpse is ITEM_CORPSE_NPC, not ITEM_CONTAINER)
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    // Verify corpse has craft_mats from the mob
    ASSERT(corpse->craft_mat_count == 2);
    ASSERT(corpse->craft_mats != NULL);
    ASSERT(corpse->craft_mats[0] == 3001);
    ASSERT(corpse->craft_mats[1] == 3002);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #5: Object Prototype Salvage Mats Tests
////////////////////////////////////////////////////////////////////////////////

static int test_obj_proto_salvage_mats_field()
{
    // Create an object prototype with salvage_mats using mock helper
    ObjPrototype* op = mock_obj_proto(80010);
    ASSERT(op != NULL);
    ASSERT(op->salvage_mats == NULL);
    ASSERT(op->salvage_mat_count == 0);
    
    // Add some salvage_mats
    op->salvage_mat_count = 2;
    op->salvage_mats = malloc(sizeof(VNUM) * 2);
    op->salvage_mats[0] = 4001;  // Example material VNUM
    op->salvage_mats[1] = 4002;
    
    ASSERT(op->salvage_mat_count == 2);
    ASSERT(op->salvage_mats[0] == 4001);
    ASSERT(op->salvage_mats[1] == 4002);
    
    return 0;
}

static int test_obj_salvage_mats_inheritance()
{
    // Create an object prototype with salvage_mats
    ObjPrototype* op = mock_obj_proto(80011);
    ASSERT(op != NULL);
    
    op->salvage_mat_count = 3;
    op->salvage_mats = malloc(sizeof(VNUM) * 3);
    op->salvage_mats[0] = 5001;
    op->salvage_mats[1] = 5002;
    op->salvage_mats[2] = 5003;
    
    // Create object from prototype
    Object* obj = create_object(op, 1);
    ASSERT(obj != NULL);
    ASSERT(obj->prototype == op);
    
    // Object should have copied salvage_mats from prototype
    ASSERT(obj->salvage_mat_count == 3);
    ASSERT(obj->salvage_mats != NULL);
    ASSERT(obj->salvage_mats[0] == 5001);
    ASSERT(obj->salvage_mats[1] == 5002);
    ASSERT(obj->salvage_mats[2] == 5003);
    
    // Verify it's a copy, not a pointer to prototype's array
    ASSERT(obj->salvage_mats != op->salvage_mats);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #6: Corpse Extraction Flags Tests
////////////////////////////////////////////////////////////////////////////////

static int test_corpse_flags_init()
{
    // Create a mobile and kill it to generate a corpse
    Room* room = mock_room(90001, NULL, NULL);
    MobPrototype* proto = mock_mob_proto(90001);
    Mobile* mob = mock_mob("test corpse victim", 90001, proto);
    transfer_mob(mob, room);
    
    // Kill the mob to create a corpse
    mob->hit = -10;
    raw_kill(mob);
    
    // Find the corpse
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    
    ASSERT(corpse != NULL);
    
    // New corpse should have extraction_flags = 0
    ASSERT(corpse->corpse.extraction_flags == 0);
    ASSERT(!IS_SET(corpse->corpse.extraction_flags, CORPSE_SKINNED));
    ASSERT(!IS_SET(corpse->corpse.extraction_flags, CORPSE_BUTCHERED));
    
    return 0;
}

static int test_corpse_skinned_flag()
{
    // Create a corpse
    Room* room = mock_room(90002, NULL, NULL);
    MobPrototype* proto = mock_mob_proto(90002);
    Mobile* mob = mock_mob("skinnable creature", 90002, proto);
    transfer_mob(mob, room);
    
    mob->hit = -10;
    raw_kill(mob);
    
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    // Set the SKINNED flag (simulating a skin command)
    SET_BIT(corpse->corpse.extraction_flags, CORPSE_SKINNED);
    
    ASSERT(IS_SET(corpse->corpse.extraction_flags, CORPSE_SKINNED));
    ASSERT(!IS_SET(corpse->corpse.extraction_flags, CORPSE_BUTCHERED));
    
    return 0;
}

static int test_corpse_butchered_flag()
{
    // Create a corpse
    Room* room = mock_room(90003, NULL, NULL);
    MobPrototype* proto = mock_mob_proto(90003);
    Mobile* mob = mock_mob("butcherable creature", 90003, proto);
    transfer_mob(mob, room);
    
    mob->hit = -10;
    raw_kill(mob);
    
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    // Set the BUTCHERED flag (simulating a butcher command)
    SET_BIT(corpse->corpse.extraction_flags, CORPSE_BUTCHERED);
    
    ASSERT(!IS_SET(corpse->corpse.extraction_flags, CORPSE_SKINNED));
    ASSERT(IS_SET(corpse->corpse.extraction_flags, CORPSE_BUTCHERED));
    
    return 0;
}

static int test_corpse_flags_independent()
{
    // Create a corpse
    Room* room = mock_room(90004, NULL, NULL);
    MobPrototype* proto = mock_mob_proto(90004);
    Mobile* mob = mock_mob("multi-extract creature", 90004, proto);
    transfer_mob(mob, room);
    
    mob->hit = -10;
    raw_kill(mob);
    
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    // Set SKINNED first
    SET_BIT(corpse->corpse.extraction_flags, CORPSE_SKINNED);
    ASSERT(IS_SET(corpse->corpse.extraction_flags, CORPSE_SKINNED));
    ASSERT(!IS_SET(corpse->corpse.extraction_flags, CORPSE_BUTCHERED));
    
    // Set BUTCHERED second - both should now be set
    SET_BIT(corpse->corpse.extraction_flags, CORPSE_BUTCHERED);
    ASSERT(IS_SET(corpse->corpse.extraction_flags, CORPSE_SKINNED));
    ASSERT(IS_SET(corpse->corpse.extraction_flags, CORPSE_BUTCHERED));
    
    // Corpse can be both skinned AND butchered
    ASSERT(corpse->corpse.extraction_flags == (CORPSE_SKINNED | CORPSE_BUTCHERED));
    
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
    REGISTER("Recipe: Lookup By Name", test_recipe_lookup_by_name);
    REGISTER("Recipe: Lookup Not Found", test_recipe_lookup_not_found);
    REGISTER("Recipe: Free", test_recipe_free);
    REGISTER("Recipe: Mock Helper", test_mock_recipe);
    
    // Issue #4: Mob Prototype Craft Mats
    REGISTER("MobProto: CraftMats Field", test_mob_proto_craft_mats_field);
    REGISTER("Mob: CraftMats From Proto", test_mob_craft_mats_from_prototype);
    REGISTER("Corpse: Inherits CraftMats", test_corpse_inherits_craft_mats);
    
    // Issue #5: Object Prototype Salvage Mats
    REGISTER("ObjProto: SalvageMats Field", test_obj_proto_salvage_mats_field);
    REGISTER("Object: SalvageMats Inheritance", test_obj_salvage_mats_inheritance);
    
    // Issue #6: Corpse Extraction Flags
    REGISTER("Corpse: Flags Init", test_corpse_flags_init);
    REGISTER("Corpse: Skinned Flag", test_corpse_skinned_flag);
    REGISTER("Corpse: Butchered Flag", test_corpse_butchered_flag);
    REGISTER("Corpse: Flags Independent", test_corpse_flags_independent);

#undef REGISTER
}
