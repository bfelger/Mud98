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
#include <craft/workstation.h>
#include <craft/act_craft.h>
#include <craft/craft_olc.h>

#include <persist/recipe/json/recipe_persist_json.h>
#include <persist/recipe/rom-olc/recipe_persist_rom_olc.h>

#include <data/item.h>
#include <data/skill.h>

#include <entities/area.h>
#include <entities/mobile.h>
#include <entities/mob_prototype.h>
#include <entities/object.h>
#include <entities/obj_prototype.h>
#include <entities/room.h>

#include <fight.h>
#include <comm.h>
#include <handler.h>

#include <jansson.h>

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
// Issue #8: Recipe Persistence Tests
////////////////////////////////////////////////////////////////////////////////

static int test_recipe_json_build()
{
    // Create a test recipe
    Recipe* recipe = new_recipe();
    VNUM_FIELD(recipe) = 99001;
    recipe->header.name = AS_STRING(OBJ_VAL(copy_string("test leather armor", 18)));
    recipe->required_skill = gsn_second_attack;  // Use any available skill
    recipe->min_skill_pct = 50;
    recipe->min_level = 10;
    recipe->station_type = WORK_TANNERY | WORK_FORGE;
    recipe->station_vnum = VNUM_NONE;
    recipe->discovery = DISC_TRAINER;
    recipe_add_ingredient(recipe, 100, 3);  // 3x vnum 100
    recipe_add_ingredient(recipe, 101, 1);  // 1x vnum 101
    recipe->product_vnum = 200;
    recipe->product_quantity = 1;
    
    // Add to global table
    add_recipe(recipe);
    
    // Build JSON
    json_t* arr = recipe_persist_json_build(NULL);
    ASSERT(arr != NULL);
    ASSERT(json_is_array(arr));
    ASSERT(json_array_size(arr) >= 1);
    
    // Find our recipe in the array
    bool found = false;
    for (size_t i = 0; i < json_array_size(arr); i++) {
        json_t* obj = json_array_get(arr, i);
        json_int_t vnum = json_integer_value(json_object_get(obj, "vnum"));
        if (vnum == 99001) {
            found = true;
            
            // Verify fields
            ASSERT_STR_EQ("test leather armor", json_string_value(json_object_get(obj, "name")));
            ASSERT(json_integer_value(json_object_get(obj, "minSkillPct")) == 50);
            ASSERT(json_integer_value(json_object_get(obj, "minLevel")) == 10);
            ASSERT(json_integer_value(json_object_get(obj, "outputVnum")) == 200);
            
            // Verify inputs array
            json_t* inputs = json_object_get(obj, "inputs");
            ASSERT(json_is_array(inputs));
            ASSERT(json_array_size(inputs) == 2);
            
            break;
        }
    }
    ASSERT(found);
    
    json_decref(arr);
    remove_recipe(99001);
    
    return 0;
}

static int test_recipe_json_parse()
{
    // Create JSON for a recipe
    json_t* arr = json_array();
    json_t* obj = json_object();
    
    json_object_set_new(obj, "vnum", json_integer(99002));
    json_object_set_new(obj, "name", json_string("parsed recipe"));
    json_object_set_new(obj, "skill", json_string("second attack"));  // Use available skill
    json_object_set_new(obj, "minSkillPct", json_integer(75));
    json_object_set_new(obj, "minLevel", json_integer(15));
    json_object_set_new(obj, "discovery", json_string("quest"));
    json_object_set_new(obj, "outputVnum", json_integer(300));
    json_object_set_new(obj, "outputQuantity", json_integer(2));
    
    // Workstation type array
    json_t* stations = json_array();
    json_array_append_new(stations, json_string("alchemy"));
    json_object_set_new(obj, "stationType", stations);
    
    // Inputs array
    json_t* inputs = json_array();
    json_t* input1 = json_object();
    json_object_set_new(input1, "vnum", json_integer(150));
    json_object_set_new(input1, "quantity", json_integer(5));
    json_array_append_new(inputs, input1);
    json_object_set_new(obj, "inputs", inputs);
    
    json_array_append_new(arr, obj);
    
    // Parse it
    recipe_persist_json_parse(arr, NULL);
    
    // Verify recipe was created
    Recipe* recipe = get_recipe(99002);
    ASSERT(recipe != NULL);
    ASSERT_STR_EQ("parsed recipe", NAME_STR(recipe));
    ASSERT(recipe->min_skill_pct == 75);
    ASSERT(recipe->min_level == 15);
    ASSERT(recipe->discovery == DISC_QUEST);
    ASSERT(recipe->station_type == WORK_ALCHEMY);
    ASSERT(recipe->product_vnum == 300);
    ASSERT(recipe->product_quantity == 2);
    ASSERT(recipe->ingredient_count == 1);
    ASSERT(recipe->ingredients[0].mat_vnum == 150);
    ASSERT(recipe->ingredients[0].quantity == 5);
    
    json_decref(arr);
    remove_recipe(99002);
    
    return 0;
}

static int test_recipe_json_roundtrip()
{
    // Create a recipe with all fields
    Recipe* recipe = new_recipe();
    VNUM_FIELD(recipe) = 99003;
    recipe->header.name = AS_STRING(OBJ_VAL(copy_string("roundtrip recipe", 16)));
    recipe->required_skill = gsn_second_attack;
    recipe->min_skill_pct = 60;
    recipe->min_level = 20;
    recipe->station_type = WORK_FORGE | WORK_SMELTER;
    recipe->station_vnum = 5000;
    recipe->discovery = DISC_DISCOVERY;
    recipe_add_ingredient(recipe, 500, 2);
    recipe_add_ingredient(recipe, 501, 4);
    recipe_add_ingredient(recipe, 502, 1);
    recipe->product_vnum = 600;
    recipe->product_quantity = 3;
    
    // Add to global table
    add_recipe(recipe);
    
    // Build JSON and immediately parse it back
    json_t* arr = recipe_persist_json_build(NULL);
    
    // Find and extract our recipe's JSON
    json_t* recipe_json = NULL;
    for (size_t i = 0; i < json_array_size(arr); i++) {
        json_t* obj = json_array_get(arr, i);
        if (json_integer_value(json_object_get(obj, "vnum")) == 99003) {
            recipe_json = json_incref(obj);
            break;
        }
    }
    ASSERT(recipe_json != NULL);
    
    // Remove original recipe
    remove_recipe(99003);
    ASSERT(get_recipe(99003) == NULL);
    
    // Parse the JSON back
    json_t* parse_arr = json_array();
    json_array_append(parse_arr, recipe_json);
    recipe_persist_json_parse(parse_arr, NULL);
    
    // Verify round-trip
    Recipe* loaded = get_recipe(99003);
    ASSERT(loaded != NULL);
    ASSERT_STR_EQ("roundtrip recipe", NAME_STR(loaded));
    ASSERT(loaded->min_skill_pct == 60);
    ASSERT(loaded->min_level == 20);
    ASSERT(loaded->station_type == (WORK_FORGE | WORK_SMELTER));
    ASSERT(loaded->station_vnum == 5000);
    ASSERT(loaded->discovery == DISC_DISCOVERY);
    ASSERT(loaded->ingredient_count == 3);
    ASSERT(loaded->ingredients[0].mat_vnum == 500);
    ASSERT(loaded->ingredients[0].quantity == 2);
    ASSERT(loaded->ingredients[1].mat_vnum == 501);
    ASSERT(loaded->ingredients[1].quantity == 4);
    ASSERT(loaded->ingredients[2].mat_vnum == 502);
    ASSERT(loaded->ingredients[2].quantity == 1);
    ASSERT(loaded->product_vnum == 600);
    ASSERT(loaded->product_quantity == 3);
    
    json_decref(arr);
    json_decref(recipe_json);
    json_decref(parse_arr);
    remove_recipe(99003);
    
    return 0;
}

static int test_recipe_rom_olc_save()
{
    // Create a mock area and recipe
    AreaData* area = new_area_data();
    area->header.name = AS_STRING(OBJ_VAL(copy_string("Test Area", 9)));
    area->min_vnum = 99000;
    area->max_vnum = 99999;
    
    Recipe* recipe = new_recipe();
    VNUM_FIELD(recipe) = 99004;
    recipe->header.name = AS_STRING(OBJ_VAL(copy_string("rom olc recipe", 14)));
    recipe->area = area;
    recipe->required_skill = gsn_second_attack;
    recipe->min_skill_pct = 40;
    recipe->min_level = 5;
    recipe->station_type = WORK_COOKING;
    recipe->discovery = DISC_TRAINER;
    recipe_add_ingredient(recipe, 700, 2);
    recipe->product_vnum = 800;
    recipe->product_quantity = 1;
    
    // Add to global table
    add_recipe(recipe);
    
    // Save to temp file
    FILE* fp = tmpfile();
    ASSERT(fp != NULL);
    
    save_recipes(fp, area);
    
    // Read back and verify format
    rewind(fp);
    char buffer[4096];
    size_t read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[read] = '\0';
    fclose(fp);
    
    // Verify output contains expected content
    ASSERT(strstr(buffer, "#RECIPES") != NULL);
    ASSERT(strstr(buffer, "recipe 99004") != NULL);
    ASSERT(strstr(buffer, "rom olc recipe") != NULL);
    ASSERT(strstr(buffer, "skill") != NULL);
    ASSERT(strstr(buffer, "cooking") != NULL);  // workstation type
    ASSERT(strstr(buffer, "trainer") != NULL);  // discovery type
    ASSERT(strstr(buffer, "input 700 2") != NULL);
    ASSERT(strstr(buffer, "output 800 1") != NULL);
    ASSERT(strstr(buffer, "#ENDRECIPES") != NULL);
    
    remove_recipe(99004);
    // area cleanup handled by test framework
    
    return 0;
}

static int test_recipe_area_filter()
{
    // Create two areas
    AreaData* area1 = new_area_data();
    area1->header.name = AS_STRING(OBJ_VAL(copy_string("Area One", 8)));
    
    AreaData* area2 = new_area_data();
    area2->header.name = AS_STRING(OBJ_VAL(copy_string("Area Two", 8)));
    
    // Create recipes in different areas
    Recipe* recipe1 = new_recipe();
    VNUM_FIELD(recipe1) = 99005;
    recipe1->header.name = AS_STRING(OBJ_VAL(copy_string("recipe one", 10)));
    recipe1->area = area1;
    add_recipe(recipe1);
    
    Recipe* recipe2 = new_recipe();
    VNUM_FIELD(recipe2) = 99006;
    recipe2->header.name = AS_STRING(OBJ_VAL(copy_string("recipe two", 10)));
    recipe2->area = area2;
    add_recipe(recipe2);
    
    // Build JSON for area1 only
    json_t* arr = recipe_persist_json_build((Entity*)area1);
    ASSERT(json_array_size(arr) == 1);
    
    json_t* obj = json_array_get(arr, 0);
    ASSERT(json_integer_value(json_object_get(obj, "vnum")) == 99005);
    
    json_decref(arr);
    
    // Build JSON for area2 only
    arr = recipe_persist_json_build((Entity*)area2);
    ASSERT(json_array_size(arr) == 1);
    
    obj = json_array_get(arr, 0);
    ASSERT(json_integer_value(json_object_get(obj, "vnum")) == 99006);
    
    json_decref(arr);
    
    remove_recipe(99005);
    remove_recipe(99006);
    // area cleanup handled by test framework
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #15: Workstation Helper Tests
////////////////////////////////////////////////////////////////////////////////

static int test_find_workstation_none_present()
{
    Room* room = mock_room(70101, NULL, NULL);
    
    Object* result = find_workstation(room, WORK_FORGE);
    
    ASSERT(result == NULL);
    return 0;
}

static int test_find_workstation_exact_match()
{
    Room* room = mock_room(70102, NULL, NULL);
    Object* forge = mock_workstation("a forge", 70001, WORK_FORGE, 0);
    obj_to_room(forge, room);
    
    Object* result = find_workstation(room, WORK_FORGE);
    
    ASSERT(result == forge);
    return 0;
}

static int test_find_workstation_multi_flag()
{
    Room* room = mock_room(70103, NULL, NULL);
    Object* combo = mock_workstation("master station", 70002, WORK_FORGE | WORK_SMELTER, 0);
    obj_to_room(combo, room);
    
    // Should find station when searching for FORGE
    Object* result1 = find_workstation(room, WORK_FORGE);
    ASSERT(result1 == combo);
    
    // Should find station when searching for SMELTER
    Object* result2 = find_workstation(room, WORK_SMELTER);
    ASSERT(result2 == combo);
    
    return 0;
}

static int test_find_workstation_wrong_type()
{
    Room* room = mock_room(70104, NULL, NULL);
    Object* tannery = mock_workstation("a tanning rack", 70003, WORK_TANNERY, 0);
    obj_to_room(tannery, room);
    
    // Looking for FORGE should not find TANNERY
    Object* result = find_workstation(room, WORK_FORGE);
    
    ASSERT(result == NULL);
    return 0;
}

static int test_find_workstation_by_vnum_found()
{
    Room* room = mock_room(70105, NULL, NULL);
    Object* forge = mock_workstation("special forge", 70005, WORK_FORGE, 10);
    obj_to_room(forge, room);
    
    Object* result = find_workstation_by_vnum(room, 70005);
    
    ASSERT(result == forge);
    return 0;
}

static int test_find_workstation_by_vnum_not_found()
{
    Room* room = mock_room(70106, NULL, NULL);
    
    Object* result = find_workstation_by_vnum(room, 99999);
    
    ASSERT(result == NULL);
    return 0;
}

static int test_has_required_workstation_none_needed()
{
    Room* room = mock_room(70107, NULL, NULL);
    Recipe* recipe = new_recipe();
    VNUM_FIELD(recipe) = 70010;
    recipe->station_type = WORK_NONE;
    recipe->station_vnum = VNUM_NONE;
    
    bool result = has_required_workstation(room, recipe);
    
    ASSERT(result == true);
    
    free_recipe(recipe);
    return 0;
}

static int test_has_required_workstation_by_type()
{
    Room* room = mock_room(70108, NULL, NULL);
    Object* forge = mock_workstation("a forge", 70006, WORK_FORGE, 0);
    obj_to_room(forge, room);
    
    Recipe* recipe = new_recipe();
    VNUM_FIELD(recipe) = 70011;
    recipe->station_type = WORK_FORGE;
    recipe->station_vnum = VNUM_NONE;
    
    bool result = has_required_workstation(room, recipe);
    
    ASSERT(result == true);
    
    free_recipe(recipe);
    return 0;
}

static int test_has_required_workstation_by_vnum()
{
    Room* room = mock_room(70109, NULL, NULL);
    Object* station = mock_workstation("quest forge", 70007, WORK_FORGE, 0);
    obj_to_room(station, room);
    
    Recipe* recipe = new_recipe();
    VNUM_FIELD(recipe) = 70012;
    recipe->station_type = WORK_NONE;
    recipe->station_vnum = 70007;
    
    bool result = has_required_workstation(room, recipe);
    
    ASSERT(result == true);
    
    free_recipe(recipe);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #9: Skin Command Tests
////////////////////////////////////////////////////////////////////////////////

static int test_skin_no_argument()
{
    Room* room = mock_room(80001, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_skin(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Skin what?");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_skin_not_a_corpse()
{
    Room* room = mock_room(80002, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    Object* sword = mock_sword("a sword", 80003, 5, 1, 6);
    obj_to_room(sword, room);
    
    test_socket_output_enabled = true;
    do_skin(ch, "sword");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("not a corpse");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_skin_already_skinned()
{
    Room* room = mock_room(80003, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    // Create an NPC and kill it to make a corpse
    MobPrototype* mp = mock_mob_proto(80001);
    Mobile* mob = create_mobile(mp);
    transfer_mob(mob, room);
    
    make_corpse(mob);
    
    // Find the corpse in the room
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    corpse->corpse.extraction_flags |= CORPSE_SKINNED;
    
    test_socket_output_enabled = true;
    do_skin(ch, "corpse");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("already been skinned");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_skin_no_skinnable_mats()
{
    Room* room = mock_room(80004, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    // Create mob with only meat (not skinnable)
    MobPrototype* mp = mock_mob_proto(80002);
    ObjPrototype* meat_proto = mock_obj_proto(80010);
    meat_proto->item_type = ITEM_MAT;
    meat_proto->craft_mat.mat_type = MAT_MEAT;
    meat_proto->craft_mat.amount = 1;
    meat_proto->craft_mat.quality = 50;
    
    mp->craft_mat_count = 1;
    mp->craft_mats = alloc_mem(sizeof(VNUM));
    mp->craft_mats[0] = VNUM_FIELD(meat_proto);
    
    Mobile* mob = create_mobile(mp);
    transfer_mob(mob, room);
    
    make_corpse(mob);
    
    test_socket_output_enabled = true;
    do_skin(ch, "corpse");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("nothing to skin");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_skin_success()
{
    Room* room = mock_room(80005, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    // Create mob with skinnable hide
    MobPrototype* mp = mock_mob_proto(80003);
    ObjPrototype* hide_proto = mock_obj_proto(80020);
    hide_proto->item_type = ITEM_MAT;
    hide_proto->craft_mat.mat_type = MAT_HIDE;
    hide_proto->craft_mat.amount = 1;
    hide_proto->craft_mat.quality = 50;
    
    mp->craft_mat_count = 1;
    mp->craft_mats = alloc_mem(sizeof(VNUM));
    mp->craft_mats[0] = VNUM_FIELD(hide_proto);
    
    Mobile* mob = create_mobile(mp);
    transfer_mob(mob, room);
    
    make_corpse(mob);
    
    // Find the corpse in the room
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    // Count player inventory before
    int inv_before = 0;
    FOR_EACH_MOB_OBJ(obj, ch) { inv_before++; }
    
    test_socket_output_enabled = true;
    do_skin(ch, "corpse");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("carefully skin");
    
    // Check corpse is now skinned
    ASSERT(corpse->corpse.extraction_flags & CORPSE_SKINNED);
    
    // Count player inventory after
    int inv_after = 0;
    FOR_EACH_MOB_OBJ(obj, ch) { inv_after++; }
    ASSERT(inv_after == inv_before + 1);
    
    test_output_buffer = NIL_VAL;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #10: Butcher Command Tests
////////////////////////////////////////////////////////////////////////////////

static int test_butcher_no_argument()
{
    Room* room = mock_room(80101, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_butcher(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Butcher what?");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_butcher_already_butchered()
{
    Room* room = mock_room(80102, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    MobPrototype* mp = mock_mob_proto(80011);
    Mobile* mob = create_mobile(mp);
    transfer_mob(mob, room);
    
    make_corpse(mob);
    
    // Find the corpse in the room
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    corpse->corpse.extraction_flags |= CORPSE_BUTCHERED;
    
    test_socket_output_enabled = true;
    do_butcher(ch, "corpse");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("already been butchered");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_butcher_no_butcherable_mats()
{
    Room* room = mock_room(80103, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    // Create mob with only hide (not butcherable)
    MobPrototype* mp = mock_mob_proto(80012);
    ObjPrototype* hide_proto = mock_obj_proto(80030);
    hide_proto->item_type = ITEM_MAT;
    hide_proto->craft_mat.mat_type = MAT_HIDE;
    hide_proto->craft_mat.amount = 1;
    hide_proto->craft_mat.quality = 50;
    
    mp->craft_mat_count = 1;
    mp->craft_mats = alloc_mem(sizeof(VNUM));
    mp->craft_mats[0] = VNUM_FIELD(hide_proto);
    
    Mobile* mob = create_mobile(mp);
    transfer_mob(mob, room);
    
    make_corpse(mob);
    
    test_socket_output_enabled = true;
    do_butcher(ch, "corpse");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("nothing to butcher");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_butcher_success()
{
    Room* room = mock_room(80104, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    // Create mob with butcherable meat
    MobPrototype* mp = mock_mob_proto(80013);
    ObjPrototype* meat_proto = mock_obj_proto(80040);
    meat_proto->item_type = ITEM_MAT;
    meat_proto->craft_mat.mat_type = MAT_MEAT;
    meat_proto->craft_mat.amount = 1;
    meat_proto->craft_mat.quality = 50;
    
    mp->craft_mat_count = 1;
    mp->craft_mats = alloc_mem(sizeof(VNUM));
    mp->craft_mats[0] = VNUM_FIELD(meat_proto);
    
    Mobile* mob = create_mobile(mp);
    transfer_mob(mob, room);
    
    make_corpse(mob);
    
    // Find the corpse in the room
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    int inv_before = 0;
    FOR_EACH_MOB_OBJ(obj, ch) { inv_before++; }
    
    test_socket_output_enabled = true;
    do_butcher(ch, "corpse");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("carefully butcher");
    ASSERT(corpse->corpse.extraction_flags & CORPSE_BUTCHERED);
    
    int inv_after = 0;
    FOR_EACH_MOB_OBJ(obj, ch) { inv_after++; }
    ASSERT(inv_after == inv_before + 1);
    
    test_output_buffer = NIL_VAL;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #12: Salvage Command Tests
////////////////////////////////////////////////////////////////////////////////

static int test_salvage_no_argument()
{
    Room* room = mock_room(80201, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_salvage(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Salvage what?");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_salvage_item_not_found()
{
    Room* room = mock_room(80202, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_salvage(ch, "nonexistent");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("don't have that");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_salvage_no_salvage_mats()
{
    Room* room = mock_room(80203, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    // Create an item with no salvage mats
    Object* sword = mock_sword("sword", 80050, 5, 1, 6);
    obj_to_char(sword, ch);
    
    test_socket_output_enabled = true;
    do_salvage(ch, "sword");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("nothing to salvage");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_salvage_success()
{
    Room* room = mock_room(80204, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    // Create salvage material proto
    ObjPrototype* metal_proto = mock_obj_proto(80060);
    metal_proto->item_type = ITEM_MAT;
    metal_proto->craft_mat.mat_type = MAT_INGOT;
    metal_proto->craft_mat.amount = 1;
    metal_proto->craft_mat.quality = 50;
    
    // Create an item with salvage mats
    ObjPrototype* sword_proto = mock_obj_proto(80051);
    sword_proto->header.name = AS_STRING(mock_str("sword blade"));
    sword_proto->short_descr = str_dup("a rusty sword");
    sword_proto->item_type = ITEM_WEAPON;
    sword_proto->salvage_mat_count = 1;
    sword_proto->salvage_mats = alloc_mem(sizeof(VNUM));
    sword_proto->salvage_mats[0] = VNUM_FIELD(metal_proto);
    
    Object* sword = create_object(sword_proto, 5);
    obj_to_char(sword, ch);
    
    int inv_before = 0;
    Object* obj;
    FOR_EACH_MOB_OBJ(obj, ch) { inv_before++; }
    
    test_socket_output_enabled = true;
    do_salvage(ch, "sword");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("salvage");
    
    // Inventory should have gained materials (sword destroyed, mats added)
    int inv_after = 0;
    FOR_EACH_MOB_OBJ(obj, ch) { inv_after++; }
    // Sword gone (-1), mats added (+1) = same count
    ASSERT(inv_after >= inv_before);
    
    test_output_buffer = NIL_VAL;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #11: Craft Command Tests
////////////////////////////////////////////////////////////////////////////////

static int test_craft_no_argument()
{
    Room* room = mock_room(80301, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_craft(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Craft what?");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_craft_recipe_not_found()
{
    Room* room = mock_room(80302, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_craft(ch, "nonexistent_recipe");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("don't know");
    test_output_buffer = NIL_VAL;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #13: Recipes Command Tests
////////////////////////////////////////////////////////////////////////////////

static int test_recipes_list()
{
    Room* room = mock_room(80401, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_recipes(ch, "");
    test_socket_output_enabled = false;
    
    // Should output something about recipes (even if none known)
    ASSERT_OUTPUT_CONTAINS("recipe");
    test_output_buffer = NIL_VAL;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #21: Craft OLC Helper Tests
////////////////////////////////////////////////////////////////////////////////

static int test_craft_olc_add_mat_valid()
{
    Room* room = mock_room(85001, NULL, NULL);
    Mobile* ch = mock_player("Builder");
    transfer_mob(ch, room);
    
    // Create an ITEM_MAT prototype
    ObjPrototype* mat_proto = mock_obj_proto(85010);
    mat_proto->item_type = ITEM_MAT;
    mat_proto->craft_mat.mat_type = MAT_HIDE;
    mat_proto->short_descr = str_dup("raw hide");
    
    VNUM* list = NULL;
    int16_t count = 0;
    
    test_socket_output_enabled = true;
    bool result = craft_olc_add_mat(&list, &count, 85010, ch);
    test_socket_output_enabled = false;
    
    ASSERT(result == true);
    ASSERT(count == 1);
    ASSERT(list != NULL);
    ASSERT(list[0] == 85010);
    
    ASSERT_OUTPUT_CONTAINS("Added");
    test_output_buffer = NIL_VAL;
    
    free(list);
    return 0;
}

static int test_craft_olc_add_mat_invalid_vnum()
{
    Room* room = mock_room(85002, NULL, NULL);
    Mobile* ch = mock_player("Builder");
    transfer_mob(ch, room);
    
    VNUM* list = NULL;
    int16_t count = 0;
    
    test_socket_output_enabled = true;
    bool result = craft_olc_add_mat(&list, &count, 99999, ch);  // Non-existent
    test_socket_output_enabled = false;
    
    ASSERT(result == false);
    ASSERT(count == 0);
    ASSERT(list == NULL);
    
    ASSERT_OUTPUT_CONTAINS("doesn't exist");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_craft_olc_add_mat_not_item_mat()
{
    Room* room = mock_room(85003, NULL, NULL);
    Mobile* ch = mock_player("Builder");
    transfer_mob(ch, room);
    
    // Create a non-ITEM_MAT prototype (weapon)
    Object* sword = mock_sword("sword", 85020, 5, 1, 6);
    (void)sword;
    
    VNUM* list = NULL;
    int16_t count = 0;
    
    test_socket_output_enabled = true;
    bool result = craft_olc_add_mat(&list, &count, 85020, ch);
    test_socket_output_enabled = false;
    
    ASSERT(result == false);
    ASSERT(count == 0);
    
    ASSERT_OUTPUT_CONTAINS("not a crafting material");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_craft_olc_add_mat_duplicate()
{
    Room* room = mock_room(85004, NULL, NULL);
    Mobile* ch = mock_player("Builder");
    transfer_mob(ch, room);
    
    ObjPrototype* mat_proto = mock_obj_proto(85030);
    mat_proto->item_type = ITEM_MAT;
    mat_proto->craft_mat.mat_type = MAT_LEATHER;
    mat_proto->short_descr = str_dup("leather");
    
    VNUM* list = NULL;
    int16_t count = 0;
    
    // Add once
    craft_olc_add_mat(&list, &count, 85030, ch);
    test_output_buffer = NIL_VAL;
    
    // Try to add again
    test_socket_output_enabled = true;
    bool result = craft_olc_add_mat(&list, &count, 85030, ch);
    test_socket_output_enabled = false;
    
    ASSERT(result == false);
    ASSERT(count == 1);  // Still just 1
    
    ASSERT_OUTPUT_CONTAINS("already in the list");
    test_output_buffer = NIL_VAL;
    
    free(list);
    return 0;
}

static int test_craft_olc_remove_mat_by_index()
{
    Room* room = mock_room(85005, NULL, NULL);
    Mobile* ch = mock_player("Builder");
    transfer_mob(ch, room);
    
    ObjPrototype* mat1 = mock_obj_proto(85040);
    mat1->item_type = ITEM_MAT;
    mat1->craft_mat.mat_type = MAT_HIDE;
    mat1->short_descr = str_dup("hide1");
    
    ObjPrototype* mat2 = mock_obj_proto(85041);
    mat2->item_type = ITEM_MAT;
    mat2->craft_mat.mat_type = MAT_FUR;
    mat2->short_descr = str_dup("hide2");
    
    VNUM* list = NULL;
    int16_t count = 0;
    
    craft_olc_add_mat(&list, &count, 85040, ch);
    craft_olc_add_mat(&list, &count, 85041, ch);
    test_output_buffer = NIL_VAL;
    
    ASSERT(count == 2);
    
    // Remove by 1-based index
    test_socket_output_enabled = true;
    bool result = craft_olc_remove_mat(&list, &count, "1", ch);
    test_socket_output_enabled = false;
    
    ASSERT(result == true);
    ASSERT(count == 1);
    ASSERT(list[0] == 85041);  // Second one remains
    
    ASSERT_OUTPUT_CONTAINS("Removed");
    test_output_buffer = NIL_VAL;
    
    free(list);
    return 0;
}

static int test_craft_olc_remove_mat_not_found()
{
    Room* room = mock_room(85006, NULL, NULL);
    Mobile* ch = mock_player("Builder");
    transfer_mob(ch, room);
    
    VNUM* list = NULL;
    int16_t count = 0;
    
    test_socket_output_enabled = true;
    bool result = craft_olc_remove_mat(&list, &count, "1", ch);
    test_socket_output_enabled = false;
    
    ASSERT(result == false);
    
    ASSERT_OUTPUT_CONTAINS("empty");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_craft_olc_clear_mats()
{
    Room* room = mock_room(85007, NULL, NULL);
    Mobile* ch = mock_player("Builder");
    transfer_mob(ch, room);
    
    ObjPrototype* mat = mock_obj_proto(85050);
    mat->item_type = ITEM_MAT;
    mat->craft_mat.mat_type = MAT_BONE;
    mat->short_descr = str_dup("bone");
    
    VNUM* list = NULL;
    int16_t count = 0;
    
    craft_olc_add_mat(&list, &count, 85050, ch);
    test_output_buffer = NIL_VAL;
    
    ASSERT(count == 1);
    
    craft_olc_clear_mats(&list, &count);
    
    ASSERT(count == 0);
    ASSERT(list == NULL);
    
    return 0;
}

static int test_craft_olc_show_mats()
{
    Room* room = mock_room(85008, NULL, NULL);
    Mobile* ch = mock_player("Builder");
    transfer_mob(ch, room);
    
    ObjPrototype* mat = mock_obj_proto(85060);
    mat->item_type = ITEM_MAT;
    mat->craft_mat.mat_type = MAT_MEAT;
    mat->short_descr = str_dup("raw meat");
    
    VNUM* list = NULL;
    int16_t count = 0;
    
    craft_olc_add_mat(&list, &count, 85060, ch);
    test_output_buffer = NIL_VAL;
    
    test_socket_output_enabled = true;
    craft_olc_show_mats(list, count, ch, "Craft Mats");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Craft Mats");
    ASSERT_OUTPUT_CONTAINS("raw meat");
    ASSERT_OUTPUT_CONTAINS("85060");
    test_output_buffer = NIL_VAL;
    
    free(list);
    return 0;
}

static int test_craft_olc_show_mats_empty()
{
    Room* room = mock_room(85009, NULL, NULL);
    Mobile* ch = mock_player("Builder");
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    craft_olc_show_mats(NULL, 0, ch, "Salvage Mats");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("(none)");
    test_output_buffer = NIL_VAL;
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
    
    // Issue #8: Recipe Persistence
    REGISTER("Recipe: JSON Build", test_recipe_json_build);
    REGISTER("Recipe: JSON Parse", test_recipe_json_parse);
    REGISTER("Recipe: JSON Roundtrip", test_recipe_json_roundtrip);
    REGISTER("Recipe: ROM-OLC Save", test_recipe_rom_olc_save);
    REGISTER("Recipe: Area Filter", test_recipe_area_filter);
    
    // Issue #15: Workstation Helpers
    REGISTER("Workstation: Find None", test_find_workstation_none_present);
    REGISTER("Workstation: Find Exact", test_find_workstation_exact_match);
    REGISTER("Workstation: Multi Flag", test_find_workstation_multi_flag);
    REGISTER("Workstation: Wrong Type", test_find_workstation_wrong_type);
    REGISTER("Workstation: By VNUM", test_find_workstation_by_vnum_found);
    REGISTER("Workstation: VNUM Not Found", test_find_workstation_by_vnum_not_found);
    REGISTER("Workstation: None Required", test_has_required_workstation_none_needed);
    REGISTER("Workstation: By Type", test_has_required_workstation_by_type);
    REGISTER("Workstation: By VNUM Req", test_has_required_workstation_by_vnum);
    
    // Issue #9: Skin Command
    REGISTER("Skin: No Argument", test_skin_no_argument);
    REGISTER("Skin: Not A Corpse", test_skin_not_a_corpse);
    REGISTER("Skin: Already Skinned", test_skin_already_skinned);
    REGISTER("Skin: No Skinnable Mats", test_skin_no_skinnable_mats);
    REGISTER("Skin: Success", test_skin_success);
    
    // Issue #10: Butcher Command
    REGISTER("Butcher: No Argument", test_butcher_no_argument);
    REGISTER("Butcher: Already Butchered", test_butcher_already_butchered);
    REGISTER("Butcher: No Butcherable Mats", test_butcher_no_butcherable_mats);
    REGISTER("Butcher: Success", test_butcher_success);
    
    // Issue #12: Salvage Command
    REGISTER("Salvage: No Argument", test_salvage_no_argument);
    REGISTER("Salvage: Item Not Found", test_salvage_item_not_found);
    REGISTER("Salvage: No Salvage Mats", test_salvage_no_salvage_mats);
    REGISTER("Salvage: Success", test_salvage_success);
    
    // Issue #11: Craft Command
    REGISTER("Craft: No Argument", test_craft_no_argument);
    REGISTER("Craft: Recipe Not Found", test_craft_recipe_not_found);
    
    // Issue #13: Recipes Command
    REGISTER("Recipes: List", test_recipes_list);
    
    // Issue #21: Craft OLC Helpers
    REGISTER("CraftOLC: Add Mat Valid", test_craft_olc_add_mat_valid);
    REGISTER("CraftOLC: Add Mat Invalid VNUM", test_craft_olc_add_mat_invalid_vnum);
    REGISTER("CraftOLC: Add Mat Not ITEM_MAT", test_craft_olc_add_mat_not_item_mat);
    REGISTER("CraftOLC: Add Mat Duplicate", test_craft_olc_add_mat_duplicate);
    REGISTER("CraftOLC: Remove Mat By Index", test_craft_olc_remove_mat_by_index);
    REGISTER("CraftOLC: Remove Mat Not Found", test_craft_olc_remove_mat_not_found);
    REGISTER("CraftOLC: Clear Mats", test_craft_olc_clear_mats);
    REGISTER("CraftOLC: Show Mats", test_craft_olc_show_mats);
    REGISTER("CraftOLC: Show Mats Empty", test_craft_olc_show_mats_empty);

#undef REGISTER
}
