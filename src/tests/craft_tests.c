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
#include <craft/craft_skill.h>

#include <persist/recipe/json/recipe_persist_json.h>
#include <persist/recipe/rom-olc/recipe_persist_rom_olc.h>

#include <data/class.h>
#include <data/item.h>
#include <data/skill.h>

#include <entities/area.h>
#include <entities/mobile.h>
#include <entities/mob_prototype.h>
#include <entities/object.h>
#include <entities/obj_prototype.h>
#include <entities/room.h>

#include <lox/array.h>

#include <fight.h>
#include <comm.h>
#include <handler.h>
#include <skills.h>

#include <jansson.h>

#include <stdlib.h>
#include <string.h>

TestGroup craft_tests;

static void equip_crafting_tool(Mobile* ch, WeaponType weapon_type, VNUM vnum,
    const char* name)
{
    Object* tool = mock_sword(name, vnum, 1, 1, 4);
    tool->weapon.weapon_type = weapon_type;
    tool->prototype->weapon.weapon_type = weapon_type;
    obj_to_char(tool, ch);
    equip_char(ch, tool, WEAR_WIELD);
}

static void equip_skinning_knife(Mobile* ch, VNUM vnum)
{
    equip_crafting_tool(ch, WEAPON_SKINNING_KNIFE, vnum, "skinning knife");
}

static void equip_butcher_knife(Mobile* ch, VNUM vnum)
{
    equip_crafting_tool(ch, WEAPON_BUTCHER_KNIFE, vnum, "butcher knife");
}

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
    // Use unique names that won't collide with real recipes
    Recipe* r1 = mock_recipe("xyzzy alpha", 80010);
    r1->product_vnum = 70100;
    
    Recipe* r2 = mock_recipe("xyzzy beta", 80011);
    r2->product_vnum = 70101;
    
    // Exact match
    Recipe* found = get_recipe_by_name("xyzzy alpha");
    ASSERT(found == r1);
    
    // Exact match on second recipe
    found = get_recipe_by_name("xyzzy beta");
    ASSERT(found == r2);
    
    // Case-insensitive
    found = get_recipe_by_name("XYZZY ALPHA");
    ASSERT(found == r1);
    
    // Prefix match - "xyzzy" should find one of our recipes
    found = get_recipe_by_name("xyzzy");
    ASSERT(found == r1 || found == r2);
    
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
    recipe->min_skill = 75;
    
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
    recipe->header.name = AS_STRING(mock_str("test leather armor"));
    recipe->required_skill = gsn_second_attack;  // Use any available skill
    recipe->min_skill = 50;
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
            ASSERT(json_integer_value(json_object_get(obj, "minSkill")) == 50);
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
    json_object_set_new(obj, "minSkill", json_integer(75));
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
    ASSERT(recipe->min_skill == 75);
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
    recipe->header.name = AS_STRING(mock_str("roundtrip recipe"));
    recipe->required_skill = gsn_second_attack;
    recipe->min_skill = 60;
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
    ASSERT(loaded->min_skill == 60);
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
    area->header.name = AS_STRING(mock_str("Test Area"));
    area->min_vnum = 99000;
    area->max_vnum = 99999;
    
    Recipe* recipe = new_recipe();
    VNUM_FIELD(recipe) = 99004;
    recipe->header.name = AS_STRING(mock_str("rom olc recipe"));
    recipe->area = area;
    recipe->required_skill = gsn_second_attack;
    recipe->min_skill = 40;
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
    area1->header.name = AS_STRING(mock_str("Area One"));
    
    AreaData* area2 = new_area_data();
    area2->header.name = AS_STRING(mock_str("Area Two"));
    
    // Create recipes in different areas
    Recipe* recipe1 = new_recipe();
    VNUM_FIELD(recipe1) = 99005;
    recipe1->header.name = AS_STRING(mock_str("recipe one"));
    recipe1->area = area1;
    add_recipe(recipe1);
    
    Recipe* recipe2 = new_recipe();
    VNUM_FIELD(recipe2) = 99006;
    recipe2->header.name = AS_STRING(mock_str("recipe two"));
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
    
    bool result = has_required_workstation(room, recipe, NULL);
    
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
    
    bool result = has_required_workstation(room, recipe, NULL);
    
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
    
    bool result = has_required_workstation(room, recipe, NULL);
    
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

static int test_skin_requires_tool()
{
    Room* room = mock_room(80009, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    ch->level = 10;
    mock_skill(ch, gsn_skinning, 100);

    MobPrototype* mp = mock_mob_proto(80007);
    ObjPrototype* hide_proto = mock_obj_proto(80024);
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
    do_skin(ch, "corpse");
    test_socket_output_enabled = false;

    ASSERT_OUTPUT_CONTAINS("need a skinning knife");

    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    ASSERT(!(corpse->corpse.extraction_flags & CORPSE_SKINNED));

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_skin_already_skinned()
{
    Room* room = mock_room(80003, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    equip_skinning_knife(ch, 80901);
    
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
    ch->level = 10;  // High enough for skinning skill (requires level 5)
    mock_skill(ch, gsn_skinning, 100);  // Give skill but no skinnable mats
    equip_skinning_knife(ch, 80902);
    
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
    ch->level = 10;  // High enough for skinning skill (requires level 5)
    mock_skill(ch, gsn_skinning, 100);  // 100% skill guarantees success
    equip_skinning_knife(ch, 80903);
    
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

// Issue #27: Skill integration tests for skin command

static int test_skin_no_skill()
{
    // Player lacks skinning skill entirely
    Room* room = mock_room(80006, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    ch->level = 10;
    // Don't set skinning skill - player hasn't learned it
    equip_skinning_knife(ch, 80904);
    
    // Create mob with skinnable hide
    MobPrototype* mp = mock_mob_proto(80004);
    ObjPrototype* hide_proto = mock_obj_proto(80021);
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
    do_skin(ch, "corpse");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("don't know how to skin");
    
    // Corpse should NOT be marked as skinned (didn't attempt)
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    ASSERT(!(corpse->corpse.extraction_flags & CORPSE_SKINNED));
    
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_skin_failure()
{
    // Player has skill but fails the check
    Room* room = mock_room(80007, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    ch->level = 10;
    mock_skill(ch, gsn_skinning, 1);  // Only 1% skill - will likely fail
    equip_skinning_knife(ch, 80905);
    
    // Create mob with skinnable hide
    MobPrototype* mp = mock_mob_proto(80005);
    ObjPrototype* hide_proto = mock_obj_proto(80022);
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
    
    // Try multiple times to get a failure (1% skill should fail most of the time)
    // We'll run the test in a way that with 1% skill, it almost always fails
    test_socket_output_enabled = true;
    do_skin(ch, "corpse");
    test_socket_output_enabled = false;
    
    // Whether success or failure, corpse should be marked as skinned
    ASSERT(corpse->corpse.extraction_flags & CORPSE_SKINNED);
    
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_skin_skill_improve()
{
    // Test that skill improvement is called (regardless of success/failure)
    Room* room = mock_room(80008, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    ch->level = 10;
    mock_skill(ch, gsn_skinning, 50);  // 50% skill
    equip_skinning_knife(ch, 80906);
    
    // Create mob with skinnable hide
    MobPrototype* mp = mock_mob_proto(80006);
    ObjPrototype* hide_proto = mock_obj_proto(80023);
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
    
    // Just verify the command completes without error
    // check_improve is called internally - we verify by ensuring the skill
    // system is engaged (corpse gets marked)
    do_skin(ch, "corpse");
    
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    ASSERT(corpse->corpse.extraction_flags & CORPSE_SKINNED);
    
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

static int test_butcher_requires_tool()
{
    Room* room = mock_room(80106, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    ch->level = 10;
    mock_skill(ch, gsn_butchering, 100);

    MobPrototype* mp = mock_mob_proto(80016);
    ObjPrototype* meat_proto = mock_obj_proto(80046);
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
    do_butcher(ch, "corpse");
    test_socket_output_enabled = false;

    ASSERT_OUTPUT_CONTAINS("need a butcher knife");

    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    ASSERT(!(corpse->corpse.extraction_flags & CORPSE_BUTCHERED));

    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_butcher_already_butchered()
{
    Room* room = mock_room(80102, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    equip_butcher_knife(ch, 80911);
    
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
    ch->level = 10;  // High enough for butchering skill (requires level 5)
    mock_skill(ch, gsn_butchering, 100);  // Give skill but no butcherable mats
    equip_butcher_knife(ch, 80912);
    
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
    ch->level = 10;  // High enough for butchering skill (requires level 5)
    mock_skill(ch, gsn_butchering, 100);  // 100% skill guarantees success
    equip_butcher_knife(ch, 80913);
    
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

// Issue #29: Salvage Skill Integration Tests

static int test_salvage_weapon_uses_blacksmithing()
{
    // Weapons should use blacksmithing skill
    ObjPrototype* sword_proto = mock_obj_proto(80052);
    sword_proto->item_type = ITEM_WEAPON;
    Object* sword = create_object(sword_proto, 5);
    
    SKNUM skill = get_salvage_skill(sword);
    ASSERT(skill == gsn_blacksmithing);
    
    return 0;
}

static int test_salvage_armor_uses_material_skill()
{
    // Metal armor uses blacksmithing
    ObjPrototype* plate_proto = mock_obj_proto(80053);
    plate_proto->item_type = ITEM_ARMOR;
    plate_proto->material = str_dup("steel");
    Object* plate = create_object(plate_proto, 5);
    ASSERT(get_salvage_skill(plate) == gsn_blacksmithing);
    
    // Leather armor uses leatherworking
    ObjPrototype* leather_proto = mock_obj_proto(80054);
    leather_proto->item_type = ITEM_ARMOR;
    leather_proto->material = str_dup("leather");
    Object* leather = create_object(leather_proto, 5);
    ASSERT(get_salvage_skill(leather) == gsn_leatherworking);
    
    // Cloth armor uses tailoring
    ObjPrototype* cloth_proto = mock_obj_proto(80055);
    cloth_proto->item_type = ITEM_ARMOR;
    cloth_proto->material = str_dup("cloth");
    Object* cloth = create_object(cloth_proto, 5);
    ASSERT(get_salvage_skill(cloth) == gsn_tailoring);
    
    return 0;
}

static int test_salvage_no_skill_reduced_yield()
{
    // Salvaging without the required skill gives reduced yield
    Room* room = mock_room(80205, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    ch->level = 10;
    // Don't give blacksmithing skill
    
    // Create multiple salvage materials
    ObjPrototype* metal1_proto = mock_obj_proto(80061);
    metal1_proto->item_type = ITEM_MAT;
    metal1_proto->craft_mat.mat_type = MAT_INGOT;
    
    ObjPrototype* metal2_proto = mock_obj_proto(80062);
    metal2_proto->item_type = ITEM_MAT;
    metal2_proto->craft_mat.mat_type = MAT_INGOT;
    
    ObjPrototype* metal3_proto = mock_obj_proto(80063);
    metal3_proto->item_type = ITEM_MAT;
    metal3_proto->craft_mat.mat_type = MAT_INGOT;
    
    ObjPrototype* metal4_proto = mock_obj_proto(80064);
    metal4_proto->item_type = ITEM_MAT;
    metal4_proto->craft_mat.mat_type = MAT_INGOT;
    
    // Create weapon with 4 salvage mats
    ObjPrototype* sword_proto = mock_obj_proto(80056);
    sword_proto->header.name = AS_STRING(mock_str("salvage sword"));
    sword_proto->short_descr = str_dup("a salvage sword");
    sword_proto->item_type = ITEM_WEAPON;
    sword_proto->salvage_mat_count = 4;
    sword_proto->salvage_mats = alloc_mem(4 * sizeof(VNUM));
    sword_proto->salvage_mats[0] = VNUM_FIELD(metal1_proto);
    sword_proto->salvage_mats[1] = VNUM_FIELD(metal2_proto);
    sword_proto->salvage_mats[2] = VNUM_FIELD(metal3_proto);
    sword_proto->salvage_mats[3] = VNUM_FIELD(metal4_proto);
    
    Object* sword = create_object(sword_proto, 5);
    
    // Test evaluate_salvage without skill
    SalvageResult result = evaluate_salvage(ch, sword);
    ASSERT(result.success == true);
    ASSERT(result.skill_used == gsn_blacksmithing);
    ASSERT(result.has_skill == false);
    // 25% of 4 = 1 material (at least 1)
    ASSERT(result.mat_count == 1);
    
    free_salvage_result(&result);
    return 0;
}

static int test_salvage_with_skill_full_yield()
{
    // Salvaging with skill gives better yield
    Room* room = mock_room(80206, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    ch->level = 20;
    mock_skill(ch, gsn_blacksmithing, 100);  // 100% skill
    
    // Create multiple salvage materials
    ObjPrototype* metal1_proto = mock_obj_proto(80065);
    metal1_proto->item_type = ITEM_MAT;
    metal1_proto->craft_mat.mat_type = MAT_INGOT;
    
    ObjPrototype* metal2_proto = mock_obj_proto(80066);
    metal2_proto->item_type = ITEM_MAT;
    metal2_proto->craft_mat.mat_type = MAT_INGOT;
    
    ObjPrototype* metal3_proto = mock_obj_proto(80067);
    metal3_proto->item_type = ITEM_MAT;
    metal3_proto->craft_mat.mat_type = MAT_INGOT;
    
    ObjPrototype* metal4_proto = mock_obj_proto(80068);
    metal4_proto->item_type = ITEM_MAT;
    metal4_proto->craft_mat.mat_type = MAT_INGOT;
    
    // Create weapon with 4 salvage mats
    ObjPrototype* sword_proto = mock_obj_proto(80057);
    sword_proto->header.name = AS_STRING(mock_str("quality sword"));
    sword_proto->short_descr = str_dup("a quality sword");
    sword_proto->item_type = ITEM_WEAPON;
    sword_proto->salvage_mat_count = 4;
    sword_proto->salvage_mats = alloc_mem(4 * sizeof(VNUM));
    sword_proto->salvage_mats[0] = VNUM_FIELD(metal1_proto);
    sword_proto->salvage_mats[1] = VNUM_FIELD(metal2_proto);
    sword_proto->salvage_mats[2] = VNUM_FIELD(metal3_proto);
    sword_proto->salvage_mats[3] = VNUM_FIELD(metal4_proto);
    
    Object* sword = create_object(sword_proto, 5);
    
    // Test evaluate_salvage with high skill
    SalvageResult result = evaluate_salvage(ch, sword);
    ASSERT(result.success == true);
    ASSERT(result.skill_used == gsn_blacksmithing);
    ASSERT(result.has_skill == true);
    // With 100% skill, should get 75-100% yield (3-4 of 4 materials)
    ASSERT(result.mat_count >= 3);
    
    free_salvage_result(&result);
    return 0;
}

static int test_salvage_no_skill_required()
{
    // Items without a required skill always give full yield
    Room* room = mock_room(80207, NULL, NULL);
    Mobile* ch = mock_player("Tester");
    transfer_mob(ch, room);
    
    // Create salvage material
    ObjPrototype* wood_proto = mock_obj_proto(80069);
    wood_proto->item_type = ITEM_MAT;
    wood_proto->craft_mat.mat_type = MAT_WOOD;
    
    // Create a food item (no skill required)
    ObjPrototype* food_proto = mock_obj_proto(80058);
    food_proto->header.name = AS_STRING(mock_str("food item"));
    food_proto->short_descr = str_dup("some food");
    food_proto->item_type = ITEM_FOOD;
    food_proto->salvage_mat_count = 2;
    food_proto->salvage_mats = alloc_mem(2 * sizeof(VNUM));
    food_proto->salvage_mats[0] = VNUM_FIELD(wood_proto);
    food_proto->salvage_mats[1] = VNUM_FIELD(wood_proto);
    
    Object* food = create_object(food_proto, 1);
    
    // No skill required for food
    SKNUM skill = get_salvage_skill(food);
    ASSERT(skill == -1);
    
    // Should get full yield
    SalvageResult result = evaluate_salvage(ch, food);
    ASSERT(result.success == true);
    ASSERT(result.skill_used == -1);
    ASSERT(result.mat_count == 2);  // Full yield
    
    free_salvage_result(&result);
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
    int count = 0;
    
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
    int count = 0;
    
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
    int count = 0;
    
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
    int count = 0;
    
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
    int count = 0;
    
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
    int count = 0;
    
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
    int count = 0;
    
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
    int count = 0;
    
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
// Issue #16: RecEdit Tests
////////////////////////////////////////////////////////////////////////////////

#include <craft/recedit.h>
#include <olc/olc.h>
#include <olc/editor_stack.h>

static int test_recedit_entry_no_arg()
{
    Room* room = mock_room(86001, NULL, NULL);
    Mobile* ch = mock_player("RecBuilder");
    ch->pcdata->security = 9;  // Need security >= 5
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_recedit(ch, "");
    test_socket_output_enabled = false;
    
    // Should show syntax help
    ASSERT_OUTPUT_CONTAINS("recedit");
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_recedit_entry_valid()
{
    Room* room = mock_room(86002, NULL, NULL);
    Mobile* ch = mock_player("RecBuilder2");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    // Create a recipe first
    (void)mock_recipe("test entry recipe", 86100);
    
    test_socket_output_enabled = true;
    do_recedit(ch, "86100");
    test_socket_output_enabled = false;
    
    // Should be in recedit now
    EditorFrame* frame = editor_stack_top(&ch->desc->editor_stack);
    ASSERT(frame != NULL);
    ASSERT(frame->editor == ED_RECIPE);
    
    // Output should show recipe info
    ASSERT_OUTPUT_CONTAINS("86100");
    test_output_buffer = NIL_VAL;
    
    edit_done(ch);
    return 0;
}

static int test_recedit_create()
{
    Room* room = mock_room(86003, NULL, NULL);
    Mobile* ch = mock_player("RecBuilder3");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    // Create an area first for the recipe with proper VNUM range
    // The area data needs to be registered globally for get_vnum_area() to find it
    AreaData* ad = mock_area_data();
    ad->min_vnum = 86200;
    ad->max_vnum = 86299;
    // Register in global areas so get_vnum_area() can find it
    write_value_array(&global_areas, OBJ_VAL(ad));
    
    test_socket_output_enabled = true;
    do_recedit(ch, "create 86200");
    test_socket_output_enabled = false;
    
    // Should be in recedit
    EditorFrame* frame = editor_stack_top(&ch->desc->editor_stack);
    ASSERT(frame != NULL);
    ASSERT(frame->editor == ED_RECIPE);
    ASSERT_OUTPUT_CONTAINS("created");
    
    // Check the recipe was created
    Recipe* recipe = get_recipe(86200);
    ASSERT(recipe != NULL);
    
    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

static int test_recedit_show()
{
    Room* room = mock_room(86004, NULL, NULL);
    Mobile* ch = mock_player("RecBuilder4");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    // Create a recipe
    Recipe* recipe = mock_recipe("test show recipe", 86300);
    recipe->min_level = 10;
    recipe->discovery = DISC_TRAINER;
    
    // Enter the editor
    set_editor(ch->desc, ED_RECIPE, (uintptr_t)recipe);
    
    test_socket_output_enabled = true;
    recedit(ch, "show");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("86300");
    ASSERT_OUTPUT_CONTAINS("test show recipe");
    ASSERT_OUTPUT_CONTAINS("10");  // Level
    ASSERT_OUTPUT_CONTAINS("trainer");  // Discovery
    
    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

static int test_recedit_list()
{
    Room* room = mock_room(86005, NULL, NULL);
    Mobile* ch = mock_player("RecBuilder5");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    // Create recipes
    mock_recipe("list recipe 1", 86400);
    mock_recipe("list recipe 2", 86401);
    
    // Enter editor with first recipe
    Recipe* recipe = get_recipe(86400);
    set_editor(ch->desc, ED_RECIPE, (uintptr_t)recipe);
    
    test_socket_output_enabled = true;
    recedit(ch, "list");
    test_socket_output_enabled = false;
    
    // Should list recipes
    ASSERT_OUTPUT_CONTAINS("86400");
    ASSERT_OUTPUT_CONTAINS("86401");
    ASSERT_OUTPUT_CONTAINS("list recipe");
    
    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

static int test_recedit_skill()
{
    Room* room = mock_room(86006, NULL, NULL);
    Mobile* ch = mock_player("RecBuilder6");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    Recipe* recipe = mock_recipe("test skill recipe", 86500);
    set_editor(ch->desc, ED_RECIPE, (uintptr_t)recipe);
    
    test_socket_output_enabled = true;
    recedit(ch, "skill 'sword' 50");
    test_socket_output_enabled = false;
    
    // Skill should be set
    ASSERT(recipe->required_skill >= 0);
    ASSERT(recipe->min_skill == 50);
    
    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

static int test_recedit_discovery()
{
    Room* room = mock_room(86007, NULL, NULL);
    Mobile* ch = mock_player("RecBuilder7");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    Recipe* recipe = mock_recipe("test discovery recipe", 86600);
    set_editor(ch->desc, ED_RECIPE, (uintptr_t)recipe);
    
    test_socket_output_enabled = true;
    recedit(ch, "discovery trainer");
    test_socket_output_enabled = false;
    
    ASSERT(recipe->discovery == DISC_TRAINER);
    ASSERT_OUTPUT_CONTAINS("trainer");
    
    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #22: AEdit Recipe Tests
////////////////////////////////////////////////////////////////////////////////

// aedit() function is the editor interpreter
void aedit(Mobile* ch, char* argument);

static int test_aedit_recipe_list_empty()
{
    Room* room = mock_room(87001, NULL, NULL);
    Mobile* ch = mock_player("AEditBuilder1");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    // Create area for testing
    AreaData* ad = mock_area_data();
    ad->min_vnum = 87000;
    ad->max_vnum = 87099;
    write_value_array(&global_areas, OBJ_VAL(ad));
    
    // Enter aedit for this area
    set_editor(ch->desc, ED_AREA, (uintptr_t)ad);
    
    test_socket_output_enabled = true;
    aedit(ch, "recipe list");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("(none)");
    ASSERT_OUTPUT_CONTAINS("0 recipe");
    
    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

static int test_aedit_recipe_create()
{
    Room* room = mock_room(87002, NULL, NULL);
    Mobile* ch = mock_player("AEditBuilder2");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    // Create area for testing
    AreaData* ad = mock_area_data();
    ad->min_vnum = 87100;
    ad->max_vnum = 87199;
    write_value_array(&global_areas, OBJ_VAL(ad));
    
    // Enter aedit for this area
    set_editor(ch->desc, ED_AREA, (uintptr_t)ad);
    
    test_socket_output_enabled = true;
    aedit(ch, "recipe create 87150");
    test_socket_output_enabled = false;
    
    // Should have entered recedit
    EditorFrame* frame = editor_stack_top(&ch->desc->editor_stack);
    ASSERT(frame != NULL);
    ASSERT(frame->editor == ED_RECIPE);
    
    // Recipe should exist
    Recipe* recipe = get_recipe(87150);
    ASSERT(recipe != NULL);
    ASSERT(recipe->area == ad);
    
    ASSERT_OUTPUT_CONTAINS("created");
    
    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

static int test_aedit_recipe_create_out_of_range()
{
    Room* room = mock_room(87003, NULL, NULL);
    Mobile* ch = mock_player("AEditBuilder3");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    // Create area for testing
    AreaData* ad = mock_area_data();
    ad->min_vnum = 87200;
    ad->max_vnum = 87299;
    write_value_array(&global_areas, OBJ_VAL(ad));
    
    // Enter aedit for this area
    set_editor(ch->desc, ED_AREA, (uintptr_t)ad);
    
    test_socket_output_enabled = true;
    aedit(ch, "recipe create 99999");  // Out of range
    test_socket_output_enabled = false;
    
    // Should NOT have entered recedit - still in aedit
    EditorFrame* frame = editor_stack_top(&ch->desc->editor_stack);
    ASSERT(frame != NULL);
    ASSERT(frame->editor == ED_AREA);
    
    ASSERT_OUTPUT_CONTAINS("must be in area range");
    
    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

static int test_aedit_recipe_edit()
{
    Room* room = mock_room(87004, NULL, NULL);
    Mobile* ch = mock_player("AEditBuilder4");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    // Create area for testing
    AreaData* ad = mock_area_data();
    ad->min_vnum = 87300;
    ad->max_vnum = 87399;
    write_value_array(&global_areas, OBJ_VAL(ad));
    
    // Create a recipe in this area
    Recipe* recipe = mock_recipe("test aedit edit recipe", 87350);
    recipe->area = ad;
    
    // Enter aedit for this area
    set_editor(ch->desc, ED_AREA, (uintptr_t)ad);
    
    test_socket_output_enabled = true;
    aedit(ch, "recipe edit 87350");
    test_socket_output_enabled = false;
    
    // Should have entered recedit
    EditorFrame* frame = editor_stack_top(&ch->desc->editor_stack);
    ASSERT(frame != NULL);
    ASSERT(frame->editor == ED_RECIPE);
    
    ASSERT_OUTPUT_CONTAINS("Editing");
    
    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

static int test_aedit_recipe_delete()
{
    Room* room = mock_room(87005, NULL, NULL);
    Mobile* ch = mock_player("AEditBuilder5");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    // Create area for testing
    AreaData* ad = mock_area_data();
    ad->min_vnum = 87400;
    ad->max_vnum = 87499;
    write_value_array(&global_areas, OBJ_VAL(ad));
    
    // Create a recipe in this area
    Recipe* recipe = mock_recipe("test aedit delete recipe", 87450);
    recipe->area = ad;
    
    // Verify recipe exists
    ASSERT(get_recipe(87450) != NULL);
    
    // Enter aedit for this area
    set_editor(ch->desc, ED_AREA, (uintptr_t)ad);
    
    test_socket_output_enabled = true;
    aedit(ch, "recipe delete 87450");
    test_socket_output_enabled = false;
    
    // Recipe should be deleted
    ASSERT(get_recipe(87450) == NULL);
    
    ASSERT_OUTPUT_CONTAINS("deleted");
    
    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// do_olist - Standalone Object/Material List Command
////////////////////////////////////////////////////////////////////////////////

extern void do_olist(Mobile* ch, char* argument);

static int test_olist_syntax_help()
{
    Room* room = mock_room(88001, NULL, NULL);
    Mobile* ch = mock_player("OlistTester1");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_olist(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Syntax:");
    ASSERT_OUTPUT_CONTAINS("olist obj");
    ASSERT_OUTPUT_CONTAINS("olist mat");
    
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_olist_obj_local()
{
    // Create area with objects
    AreaData* ad = mock_area_data();
    ad->min_vnum = 88100;
    ad->max_vnum = 88199;
    SET_NAME(ad, lox_string("Test OList Area"));
    write_value_array(&global_areas, OBJ_VAL(ad));
    
    // Create some objects in the area
    ObjPrototype* sword = mock_obj_proto(88101);
    RESTRING(sword->short_descr, "a test sword");
    sword->item_type = ITEM_WEAPON;
    sword->area = ad;
    
    ObjPrototype* armor = mock_obj_proto(88102);
    RESTRING(armor->short_descr, "test armor");
    armor->item_type = ITEM_ARMOR;
    armor->area = ad;
    
    // Create room in this area using mock_room with proper args
    Area* area = mock_area(ad);
    Room* room = mock_room(88150, NULL, area);
    
    Mobile* ch = mock_player("OlistTester2");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_olist(ch, "obj all");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Objects in Test OList Area");
    ASSERT_OUTPUT_CONTAINS("88101");
    ASSERT_OUTPUT_CONTAINS("88102");
    
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_olist_mat_local()
{
    // Create area with materials
    AreaData* ad = mock_area_data();
    ad->min_vnum = 88200;
    ad->max_vnum = 88299;
    SET_NAME(ad, lox_string("Test Mat Area"));
    write_value_array(&global_areas, OBJ_VAL(ad));
    
    // Create a material object
    ObjPrototype* hide = mock_obj_proto(88201);
    RESTRING(hide->short_descr, "some wolf hide");
    hide->item_type = ITEM_MAT;
    hide->craft_mat.mat_type = MAT_HIDE;
    hide->area = ad;
    
    ObjPrototype* bone = mock_obj_proto(88202);
    RESTRING(bone->short_descr, "a wolf bone");
    bone->item_type = ITEM_MAT;
    bone->craft_mat.mat_type = MAT_BONE;
    bone->area = ad;
    
    // Create room in this area
    Area* area = mock_area(ad);
    Room* room = mock_room(88250, NULL, area);
    
    Mobile* ch = mock_player("OlistTester3");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    test_socket_output_enabled = true;
    do_olist(ch, "mat");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("materials in Test Mat Area");
    ASSERT_OUTPUT_CONTAINS("88201");
    ASSERT_OUTPUT_CONTAINS("hide");
    ASSERT_OUTPUT_CONTAINS("88202");
    ASSERT_OUTPUT_CONTAINS("bone");
    
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_olist_mat_filter()
{
    // Create area with materials
    AreaData* ad = mock_area_data();
    ad->min_vnum = 88300;
    ad->max_vnum = 88399;
    SET_NAME(ad, lox_string("Test Filter Area"));
    write_value_array(&global_areas, OBJ_VAL(ad));
    
    // Create material objects of different types
    ObjPrototype* hide = mock_obj_proto(88301);
    RESTRING(hide->short_descr, "test hide");
    hide->item_type = ITEM_MAT;
    hide->craft_mat.mat_type = MAT_HIDE;
    hide->area = ad;
    
    ObjPrototype* ingot = mock_obj_proto(88302);
    RESTRING(ingot->short_descr, "test ingot");
    ingot->item_type = ITEM_MAT;
    ingot->craft_mat.mat_type = MAT_INGOT;
    ingot->area = ad;
    
    // Create room in this area
    Area* area = mock_area(ad);
    Room* room = mock_room(88350, NULL, area);
    
    Mobile* ch = mock_player("OlistTester4");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    // Filter by hide - should see hide but not ingot
    test_socket_output_enabled = true;
    do_olist(ch, "mat hide");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("hide materials");
    ASSERT_OUTPUT_CONTAINS("88301");
    // Ingot should NOT appear (verify output doesn't contain it)
    ASSERT(strstr(AS_STRING(test_output_buffer)->chars, "88302") == NULL);
    
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_olist_olc_aware()
{
    // Create area for OLC
    AreaData* ad = mock_area_data();
    ad->min_vnum = 88400;
    ad->max_vnum = 88499;
    SET_NAME(ad, lox_string("OLC Test Area"));
    write_value_array(&global_areas, OBJ_VAL(ad));
    
    // Create an object in this area
    ObjPrototype* obj = mock_obj_proto(88401);
    RESTRING(obj->short_descr, "olc aware test object");
    obj->item_type = ITEM_TRASH;
    obj->area = ad;
    
    // Create player in different area
    Room* room = mock_room(88500, NULL, NULL);
    Mobile* ch = mock_player("OlistTester5");
    ch->pcdata->security = 9;
    transfer_mob(ch, room);
    
    // Enter aedit for the OLC area
    set_editor(ch->desc, ED_AREA, (uintptr_t)ad);
    
    // olist should use the aedit area, not the room area
    test_socket_output_enabled = true;
    do_olist(ch, "obj all");
    test_socket_output_enabled = false;
    
    ASSERT_OUTPUT_CONTAINS("Objects in OLC Test Area");
    ASSERT_OUTPUT_CONTAINS("88401");
    
    test_output_buffer = NIL_VAL;
    edit_done(ch);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #23: Crafting Skills
////////////////////////////////////////////////////////////////////////////////

static int test_craft_skills_loaded()
{
    // All crafting skill GSNs should be valid (>= 0)
    ASSERT(gsn_skinning >= 0);
    ASSERT(gsn_butchering >= 0);
    ASSERT(gsn_leatherworking >= 0);
    ASSERT(gsn_blacksmithing >= 0);
    ASSERT(gsn_tailoring >= 0);
    ASSERT(gsn_alchemy >= 0);
    ASSERT(gsn_cooking >= 0);
    ASSERT(gsn_jewelcraft >= 0);
    ASSERT(gsn_woodworking >= 0);
    ASSERT(gsn_enchanting >= 0);
    
    return 0;
}

static int test_craft_skill_lookup()
{
    // Verify skill_lookup finds each crafting skill
    ASSERT(skill_lookup("skinning") == gsn_skinning);
    ASSERT(skill_lookup("butchering") == gsn_butchering);
    ASSERT(skill_lookup("leatherworking") == gsn_leatherworking);
    ASSERT(skill_lookup("blacksmithing") == gsn_blacksmithing);
    ASSERT(skill_lookup("tailoring") == gsn_tailoring);
    ASSERT(skill_lookup("alchemy") == gsn_alchemy);
    ASSERT(skill_lookup("cooking") == gsn_cooking);
    ASSERT(skill_lookup("jewelcraft") == gsn_jewelcraft);
    ASSERT(skill_lookup("woodworking") == gsn_woodworking);
    ASSERT(skill_lookup("enchanting") == gsn_enchanting);
    
    return 0;
}

static int test_craft_skill_names()
{
    // Verify skill names are correct
    ASSERT_STR_EQ("skinning", skill_table[gsn_skinning].name);
    ASSERT_STR_EQ("butchering", skill_table[gsn_butchering].name);
    ASSERT_STR_EQ("leatherworking", skill_table[gsn_leatherworking].name);
    ASSERT_STR_EQ("blacksmithing", skill_table[gsn_blacksmithing].name);
    ASSERT_STR_EQ("tailoring", skill_table[gsn_tailoring].name);
    ASSERT_STR_EQ("alchemy", skill_table[gsn_alchemy].name);
    ASSERT_STR_EQ("cooking", skill_table[gsn_cooking].name);
    ASSERT_STR_EQ("jewelcraft", skill_table[gsn_jewelcraft].name);
    ASSERT_STR_EQ("woodworking", skill_table[gsn_woodworking].name);
    ASSERT_STR_EQ("enchanting", skill_table[gsn_enchanting].name);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #24: Crafting Skill Group
////////////////////////////////////////////////////////////////////////////////

static int test_craft_group_exists()
{
    // The crafting skill group should exist
    SKNUM gn = group_lookup("crafting");
    ASSERT(gn >= 0);
    ASSERT_STR_EQ("crafting", skill_group_table[gn].name);
    
    return 0;
}

static int test_craft_group_contains_skills()
{
    // The crafting group should contain all crafting skills
    SKNUM gn = group_lookup("crafting");
    ASSERT(gn >= 0);
    
    // Check all skills are in the group
    const char* expected_skills[] = {
        "skinning", "butchering", "leatherworking", "blacksmithing",
        "tailoring", "alchemy", "cooking", "jewelcraft", 
        "woodworking", "enchanting", NULL
    };
    
    for (int i = 0; expected_skills[i] != NULL; i++) {
        bool found = false;
        for (int j = 0; skill_group_table[gn].skills[j] != NULL; j++) {
            if (str_cmp(skill_group_table[gn].skills[j], expected_skills[i]) == 0) {
                found = true;
                break;
            }
        }
        ASSERT(found);  // Skill should be in crafting group
    }
    
    return 0;
}

static int test_craft_group_rating()
{
    // The crafting group should be available to all classes
    SKNUM gn = group_lookup("crafting");
    ASSERT(gn >= 0);
    
    // All classes should be able to buy the crafting group (rating > 0)
    for (int i = 0; i < class_count; i++) {
        int16_t rating = GET_ELEM(&skill_group_table[gn].rating, i);
        ASSERT(rating > 0);  // All classes can buy crafting
    }
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #25: Crafting Skill Checks
////////////////////////////////////////////////////////////////////////////////

static int test_evaluate_craft_check_success()
{
    // Roll below target = success
    CraftCheckResult result = evaluate_craft_check(
        50,     // 50% skill
        10,     // level 10 player
        5,      // level 5 recipe
        30      // roll 30
    );
    
    // target = 50 + (10-5)*2 = 60, roll 30 <= 60 = success
    ASSERT(result.success == true);
    ASSERT(result.target == 60);
    ASSERT(result.roll == 30);
    ASSERT(result.quality == 30);  // 60 - 30 = 30
    
    return 0;
}

static int test_evaluate_craft_check_failure()
{
    // Roll above target = failure
    CraftCheckResult result = evaluate_craft_check(
        30,     // 30% skill
        5,      // level 5 player
        5,      // level 5 recipe
        50      // roll 50
    );
    
    // target = 30 + 0 = 30, roll 50 > 30 = failure
    ASSERT(result.success == false);
    ASSERT(result.target == 30);
    ASSERT(result.roll == 50);
    ASSERT(result.quality == -20);  // 30 - 50 = -20
    
    return 0;
}

static int test_evaluate_craft_check_quality_margin()
{
    // Quality is target - roll
    CraftCheckResult result = evaluate_craft_check(
        80,     // 80% skill
        20,     // level 20 player
        10,     // level 10 recipe
        10      // roll 10
    );
    
    // target = 80 + 20 = 100, clamped to 95
    // quality = 95 - 10 = 85
    ASSERT(result.success == true);
    ASSERT(result.target == 95);  // Clamped
    ASSERT(result.quality == 85);
    
    return 0;
}

static int test_evaluate_craft_check_level_bonus()
{
    // Higher level gives +2% per level above recipe
    CraftCheckResult low = evaluate_craft_check(50, 5, 5, 50);   // No bonus
    CraftCheckResult high = evaluate_craft_check(50, 15, 5, 50); // +20 bonus
    
    ASSERT(low.target == 50);
    ASSERT(high.target == 70);  // 50 + (15-5)*2 = 70
    
    return 0;
}

static int test_evaluate_craft_check_target_clamp()
{
    // Target clamped to 5-95 range
    CraftCheckResult low = evaluate_craft_check(1, 1, 10, 50);   // Very low skill
    CraftCheckResult high = evaluate_craft_check(99, 50, 1, 50); // Very high skill
    
    ASSERT(low.target == 5);   // Clamped to minimum
    ASSERT(high.target == 95); // Clamped to maximum
    
    return 0;
}

static int test_quality_tier_poor()
{
    ASSERT(quality_tier_from_margin(0) == QUALITY_POOR);
    ASSERT(quality_tier_from_margin(10) == QUALITY_POOR);
    ASSERT(quality_tier_from_margin(19) == QUALITY_POOR);
    
    return 0;
}

static int test_quality_tier_normal()
{
    ASSERT(quality_tier_from_margin(20) == QUALITY_NORMAL);
    ASSERT(quality_tier_from_margin(30) == QUALITY_NORMAL);
    ASSERT(quality_tier_from_margin(39) == QUALITY_NORMAL);
    
    return 0;
}

static int test_quality_tier_fine()
{
    ASSERT(quality_tier_from_margin(40) == QUALITY_FINE);
    ASSERT(quality_tier_from_margin(50) == QUALITY_FINE);
    ASSERT(quality_tier_from_margin(59) == QUALITY_FINE);
    
    return 0;
}

static int test_quality_tier_exceptional()
{
    ASSERT(quality_tier_from_margin(60) == QUALITY_EXCEPTIONAL);
    ASSERT(quality_tier_from_margin(70) == QUALITY_EXCEPTIONAL);
    ASSERT(quality_tier_from_margin(79) == QUALITY_EXCEPTIONAL);
    
    return 0;
}

static int test_quality_tier_masterwork()
{
    ASSERT(quality_tier_from_margin(80) == QUALITY_MASTERWORK);
    ASSERT(quality_tier_from_margin(90) == QUALITY_MASTERWORK);
    ASSERT(quality_tier_from_margin(100) == QUALITY_MASTERWORK);
    
    return 0;
}

static int test_quality_tier_names()
{
    ASSERT_STR_EQ("poor", quality_tier_name(QUALITY_POOR));
    ASSERT_STR_EQ("normal", quality_tier_name(QUALITY_NORMAL));
    ASSERT_STR_EQ("fine", quality_tier_name(QUALITY_FINE));
    ASSERT_STR_EQ("exceptional", quality_tier_name(QUALITY_EXCEPTIONAL));
    ASSERT_STR_EQ("masterwork", quality_tier_name(QUALITY_MASTERWORK));
    
    return 0;
}

static int test_craft_effective_skill()
{
    Room* room = mock_room(99001, NULL, NULL);
    Mobile* ch = mock_player("SkillTester");
    transfer_mob(ch, room);
    ch->level = 20;  // Must be high enough to use crafting skills
    
    // Set up a skill
    mock_skill(ch, gsn_leatherworking, 75);
    
    int skill = craft_effective_skill(ch, gsn_leatherworking);
    ASSERT(skill == 75);
    
    // Non-existent skill returns 0 (even if above level requirement)
    int zero = craft_effective_skill(ch, gsn_blacksmithing);
    ASSERT(zero == 0);
    
    return 0;
}

static int test_craft_skill_check_integration()
{
    Room* room = mock_room(99002, NULL, NULL);
    Mobile* ch = mock_player("CraftChecker");
    transfer_mob(ch, room);
    ch->level = 20;
    
    // Set up the skill
    mock_skill(ch, gsn_leatherworking, 60);
    
    // Create a recipe
    Recipe* recipe = new_recipe();
    recipe->header.vnum = 99999;
    recipe->required_skill = gsn_leatherworking;
    recipe->min_level = 10;
    
    // Run the check
    CraftCheckResult result = craft_skill_check(ch, recipe);
    
    // Verify result structure is populated
    ASSERT(result.skill_used == gsn_leatherworking);
    // target = 60 + (20-10)*2 = 80, clamped to 80
    ASSERT(result.target == 80);
    // Roll is random, so we just verify it's in range
    ASSERT(result.roll >= 1 && result.roll <= 100);
    
    free_recipe(recipe);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #30: Skill Improvement Difficulty
////////////////////////////////////////////////////////////////////////////////

static int test_craft_improve_easy()
{
    // Easy recipes (10+ levels below player) = multiplier 4
    Mobile* ch = mock_player("EasyCrafter");
    ch->level = 30;
    
    Recipe* recipe = new_recipe();
    recipe->min_level = 10;  // 20 levels below player
    
    int mult = craft_improve_multiplier(recipe, ch);
    ASSERT(mult == CRAFT_IMPROVE_EASY);  // 4
    
    free_recipe(recipe);
    return 0;
}

static int test_craft_improve_medium()
{
    // Medium recipes (within 5 levels) = multiplier 3
    Mobile* ch = mock_player("MediumCrafter");
    ch->level = 20;
    
    Recipe* recipe = new_recipe();
    recipe->min_level = 18;  // 2 levels below player (within -10 to +5)
    
    int mult = craft_improve_multiplier(recipe, ch);
    ASSERT(mult == CRAFT_IMPROVE_MEDIUM);  // 3
    
    free_recipe(recipe);
    return 0;
}

static int test_craft_improve_hard()
{
    // Hard recipes (5+ levels above player) = multiplier 2
    Mobile* ch = mock_player("HardCrafter");
    ch->level = 10;
    
    Recipe* recipe = new_recipe();
    recipe->min_level = 20;  // 10 levels above player
    
    int mult = craft_improve_multiplier(recipe, ch);
    ASSERT(mult == CRAFT_IMPROVE_HARD);  // 2
    
    free_recipe(recipe);
    return 0;
}

static int test_craft_improve_boundary()
{
    // Test boundary conditions
    Mobile* ch = mock_player("BoundaryCrafter");
    
    Recipe* recipe = new_recipe();
    
    // Exactly -10: should be medium
    ch->level = 20;
    recipe->min_level = 10;  // diff = -10
    ASSERT(craft_improve_multiplier(recipe, ch) == CRAFT_IMPROVE_MEDIUM);
    
    // Exactly -11: should be easy
    ch->level = 21;
    recipe->min_level = 10;  // diff = -11
    ASSERT(craft_improve_multiplier(recipe, ch) == CRAFT_IMPROVE_EASY);
    
    // Exactly +4: should be medium
    ch->level = 10;
    recipe->min_level = 14;  // diff = +4
    ASSERT(craft_improve_multiplier(recipe, ch) == CRAFT_IMPROVE_MEDIUM);
    
    // Exactly +5: should be hard
    ch->level = 10;
    recipe->min_level = 15;  // diff = +5
    ASSERT(craft_improve_multiplier(recipe, ch) == CRAFT_IMPROVE_HARD);
    
    free_recipe(recipe);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #26: Craft Command Skill Integration
////////////////////////////////////////////////////////////////////////////////

static int test_craft_no_skill()
{
    // Player lacks the required skill entirely
    Room* room = mock_room(99101, NULL, NULL);
    Mobile* ch = mock_player("NoSkillCrafter");
    transfer_mob(ch, room);
    ch->level = 20;
    // Don't set any crafting skill - player hasn't learned it
    
    // Create a recipe that requires leatherworking
    Recipe* recipe = mock_recipe("leather test", 99101);
    recipe->required_skill = gsn_leatherworking;
    recipe->min_skill = 1;
    recipe->discovery = DISC_KNOWN;  // Known by default
    
    CraftResult result = evaluate_craft(ch, "leather test");
    
    ASSERT(result.can_attempt == false);
    ASSERT(result.error_msg != NULL);
    // Should mention not knowing how to do the crafting type
    
    free_recipe(recipe);
    return 0;
}

static int test_craft_skill_too_low()
{
    // Player has skill but below minimum required percentage
    Room* room = mock_room(99102, NULL, NULL);
    Mobile* ch = mock_player("LowSkillCrafter");
    transfer_mob(ch, room);
    ch->level = 20;
    mock_skill(ch, gsn_leatherworking, 25);  // Only 25% skill
    
    // Create a recipe requiring 50% skill
    Recipe* recipe = mock_recipe("advanced leather", 99102);
    recipe->required_skill = gsn_leatherworking;
    recipe->min_skill = 50;  // Requires 50%
    recipe->discovery = DISC_KNOWN;
    
    CraftResult result = evaluate_craft(ch, "advanced leather");
    
    ASSERT(result.can_attempt == false);
    ASSERT(result.error_msg != NULL);
    // Should mention not skilled enough
    
    free_recipe(recipe);
    return 0;
}

static int test_craft_evaluate_uses_skill_check()
{
    // Verify evaluate_craft uses the new skill check system
    Room* room = mock_room(99103, NULL, NULL);
    Mobile* ch = mock_player("SkillCheckCrafter");
    transfer_mob(ch, room);
    ch->level = 20;
    mock_skill(ch, gsn_leatherworking, 75);
    
    // Create a simple recipe
    Recipe* recipe = mock_recipe("skill check test", 99103);
    recipe->required_skill = gsn_leatherworking;
    recipe->min_skill = 1;
    recipe->min_level = 1;
    recipe->discovery = DISC_KNOWN;
    
    CraftResult result = evaluate_craft(ch, "skill check test");
    
    ASSERT(result.can_attempt == true);
    // The check result should be populated
    ASSERT(result.check.skill_used == gsn_leatherworking);
    ASSERT(result.check.target > 0);
    ASSERT(result.check.roll >= 1 && result.check.roll <= 100);
    
    free_recipe(recipe);
    return 0;
}

static int test_craft_quality_tier_affects_output()
{
    // Verify quality tiers work correctly
    
    // Test each tier boundary
    ASSERT(quality_tier_from_margin(5) == QUALITY_POOR);
    ASSERT(quality_tier_from_margin(25) == QUALITY_NORMAL);
    ASSERT(quality_tier_from_margin(45) == QUALITY_FINE);
    ASSERT(quality_tier_from_margin(65) == QUALITY_EXCEPTIONAL);
    ASSERT(quality_tier_from_margin(85) == QUALITY_MASTERWORK);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #31: Crafting Skill Display
////////////////////////////////////////////////////////////////////////////////

static int test_skills_shows_crafting()
{
    // Verify crafting skills appear in skills command when learned
    Room* room = mock_room(99131, NULL, NULL);
    Mobile* ch = mock_player("SkillDisplay");
    transfer_mob(ch, room);
    ch->level = 20;
    
    // Give player a crafting skill
    mock_skill(ch, gsn_skinning, 75);
    
    test_socket_output_enabled = true;
    do_skills(ch, "");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    ASSERT_OUTPUT_CONTAINS("skinning");
    ASSERT_OUTPUT_CONTAINS("75%");
    
    test_output_buffer = NIL_VAL;
    return 0;
}

static int test_groups_crafting_display()
{
    // Verify 'groups crafting' shows crafting skills
    Room* room = mock_room(99132, NULL, NULL);
    Mobile* ch = mock_player("GroupDisplay");
    transfer_mob(ch, room);
    ch->level = 20;
    
    test_socket_output_enabled = true;
    do_groups(ch, "crafting");
    test_socket_output_enabled = false;
    
    ASSERT(!IS_NIL(test_output_buffer));
    ASSERT_OUTPUT_CONTAINS("skinning");
    ASSERT_OUTPUT_CONTAINS("butchering");
    ASSERT_OUTPUT_CONTAINS("leatherworking");
    
    test_output_buffer = NIL_VAL;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #32: Starter Material Objects
////////////////////////////////////////////////////////////////////////////////

static int test_starter_materials_exist()
{
    // Verify all starter material VNUMs exist
    VNUM material_vnums[] = {
        101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
        111, 112, 113, 114, 115, 116, 117, 118, 119, 120
    };
    
    for (size_t i = 0; i < sizeof(material_vnums) / sizeof(VNUM); i++) {
        ObjPrototype* proto = get_object_prototype(material_vnums[i]);
        ASSERT(proto != NULL);
    }
    
    return 0;
}

static int test_starter_materials_item_type()
{
    // Verify all starter materials are ITEM_MAT type
    VNUM material_vnums[] = {
        101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
        111, 112, 113, 114, 115, 116, 117, 118, 119, 120
    };
    
    for (size_t i = 0; i < sizeof(material_vnums) / sizeof(VNUM); i++) {
        ObjPrototype* proto = get_object_prototype(material_vnums[i]);
        ASSERT_OR_GOTO(proto != NULL, test_end);
        ASSERT(proto->item_type == ITEM_MAT);
    }
    
test_end:
    return 0;
}

static int test_starter_materials_mat_type()
{
    // Verify materials have correct CraftMatType in value[0]
    struct { VNUM vnum; CraftMatType expected; } checks[] = {
        { 101, MAT_HIDE },      // raw hide
        { 102, MAT_LEATHER },   // leather
        { 103, MAT_LEATHER },   // leather strips
        { 104, MAT_MEAT },      // raw meat
        { 105, MAT_MEAT },      // cooked meat
        { 106, MAT_ORE },       // copper ore
        { 108, MAT_ORE },       // iron ore
        { 109, MAT_INGOT },     // copper ingot
        { 112, MAT_INGOT },     // iron ingot
        { 113, MAT_HERB },      // raw flax
        { 114, MAT_CLOTH },     // linen scraps
        { 116, MAT_HIDE },      // raw wool (MAT_HIDE for skinning)
        { 117, MAT_CLOTH },     // wool fabric
        { 120, MAT_HERB },      // herbs
    };
    
    for (size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); i++) {
        ObjPrototype* proto = get_object_prototype(checks[i].vnum);
        ASSERT_OR_GOTO(proto != NULL, test_end);
        ASSERT(proto->value[0] == (int)checks[i].expected);
    }

test_end:    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #33: Starter Workstation Objects
////////////////////////////////////////////////////////////////////////////////

static int test_starter_workstations_exist()
{
    // Verify all starter workstation VNUMs exist
    VNUM workstation_vnums[] = {
        201, 202, 203, 204, 205, 206, 207, 208, 209, 210
    };
    
    for (size_t i = 0; i < sizeof(workstation_vnums) / sizeof(VNUM); i++) {
        ObjPrototype* proto = get_object_prototype(workstation_vnums[i]);
        ASSERT(proto != NULL);
    }
    
    return 0;
}

static int test_starter_workstations_item_type()
{
    // Verify all starter workstations are ITEM_WORKSTATION type
    VNUM workstation_vnums[] = {
        201, 202, 203, 204, 205, 206, 207, 208, 209, 210
    };
    
    for (size_t i = 0; i < sizeof(workstation_vnums) / sizeof(VNUM); i++) {
        ObjPrototype* proto = get_object_prototype(workstation_vnums[i]);
        ASSERT_OR_GOTO(proto != NULL, test_end);
        ASSERT(proto->item_type == ITEM_WORKSTATION);
    }
    
test_end:
    return 0;
}

static int test_starter_workstations_station_type()
{
    // Verify workstations have correct WorkstationType in value[0]
    struct { VNUM vnum; WorkstationType expected; } checks[] = {
        { 201, WORK_TANNERY },   // tanning rack
        { 202, WORK_FORGE },     // blacksmith forge
        { 203, WORK_COOKING },   // cooking fire
        { 204, WORK_ALCHEMY },   // alchemy bench
        { 205, WORK_ENCHANT },   // enchanting circle
        { 206, WORK_WOODWORK },  // woodworking bench
        { 207, WORK_JEWELER },   // jeweler's table
        { 208, WORK_LOOM },      // sewing table
        { 209, WORK_SMELTER },   // smelter
        { 210, WORK_FORGE | WORK_SMELTER },  // master forge (combo)
    };
    
    for (size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); i++) {
        ObjPrototype* proto = get_object_prototype(checks[i].vnum);
        ASSERT_OR_GOTO(proto != NULL, test_end);
        ASSERT(proto->value[0] == (int)checks[i].expected);
    }

test_end:    
    return 0;
}

static int test_starter_workstations_master_forge_bonus()
{
    // Verify master forge (210) has a crafting bonus
    ObjPrototype* proto = get_object_prototype(210);
    ASSERT(proto != NULL);
    ASSERT(proto->value[1] > 0);  // bonus field should be non-zero
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #34: Starter Recipe Objects
////////////////////////////////////////////////////////////////////////////////

static int test_starter_recipes_exist()
{
    // Verify all starter recipe VNUMs exist
    VNUM recipe_vnums[] = {
        301, 302, 303, 304,  // Leatherworking chain
        311, 312,            // Cooking chain
        321, 322, 323        // Blacksmithing chain
    };
    
    for (size_t i = 0; i < sizeof(recipe_vnums) / sizeof(VNUM); i++) {
        Recipe* recipe = get_recipe(recipe_vnums[i]);
        ASSERT(recipe != NULL);
    }
    
    return 0;
}

static int test_starter_recipes_valid_inputs()
{
    // Verify recipe inputs reference valid material objects
    Recipe* tan_hide = get_recipe(301);
    ASSERT(tan_hide != NULL);
    ASSERT(tan_hide->ingredient_count > 0);
    
    // First ingredient should be raw hide (101)
    ASSERT(tan_hide->ingredients[0].mat_vnum == 101);
    ObjPrototype* mat = get_object_prototype(101);
    ASSERT(mat != NULL);
    
    return 0;
}

static int test_starter_recipes_valid_outputs()
{
    // Verify recipe outputs reference valid objects
    Recipe* tan_hide = get_recipe(301);
    ASSERT(tan_hide != NULL);
    ASSERT(tan_hide->product_vnum == 102);  // leather
    ObjPrototype* output = get_object_prototype(102);
    ASSERT(output != NULL);
    
    Recipe* iron_dagger = get_recipe(322);
    ASSERT(iron_dagger != NULL);
    ASSERT(iron_dagger->product_vnum == 511);  // iron dagger
    output = get_object_prototype(511);
    ASSERT(output != NULL);
    
    return 0;
}

static int test_starter_recipes_chain()
{
    // Verify output of one recipe is input of next
    // tan hide (301) outputs leather (102)
    // cut strips (302) inputs leather (102)
    Recipe* tan_hide = get_recipe(301);
    Recipe* cut_strips = get_recipe(302);
    
    ASSERT(tan_hide != NULL);
    ASSERT(cut_strips != NULL);
    
    // tan_hide outputs leather (102)
    ASSERT(tan_hide->product_vnum == 102);
    
    // cut_strips takes leather (102) as input
    bool found_leather = false;
    for (int i = 0; i < cut_strips->ingredient_count; i++) {
        if (cut_strips->ingredients[i].mat_vnum == 102) {
            found_leather = true;
            break;
        }
    }
    ASSERT(found_leather);
    
    return 0;
}

static int test_starter_recipes_discovery()
{
    // Verify all starter recipes are DISC_KNOWN (default)
    VNUM recipe_vnums[] = {
        301, 302, 303, 304, 311, 312, 321, 322, 323
    };
    
    for (size_t i = 0; i < sizeof(recipe_vnums) / sizeof(VNUM); i++) {
        Recipe* recipe = get_recipe(recipe_vnums[i]);
        ASSERT_OR_GOTO(recipe != NULL, test_end);
        ASSERT(recipe->discovery == DISC_KNOWN);
    }
    
test_end:
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #36: Auto-derive craft_mats from form flags
////////////////////////////////////////////////////////////////////////////////

// Test that mammal corpses auto-derive hide and meat
static int test_corpse_derive_mammal()
{
    Room* room = mock_room(95001, NULL, NULL);
    ASSERT(room != NULL);
    
    // Create a mob prototype WITHOUT explicit craft_mats
    MobPrototype* mp = mock_mob_proto(95001);
    ASSERT(mp != NULL);
    mp->craft_mats = NULL;
    mp->craft_mat_count = 0;
    
    // Create mob from prototype with mammal form
    Mobile* mob = create_mobile(mp);
    ASSERT(mob != NULL);
    mob->form = FORM_MAMMAL | FORM_EDIBLE;  // e.g., a deer
    
    transfer_mob(mob, room);
    make_corpse(mob);
    
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
    
    // Mammals with FORM_EDIBLE should yield hide + meat
    ASSERT(corpse->craft_mat_count == 2);
    ASSERT(corpse->craft_mats != NULL);
    ASSERT(corpse->craft_mats[0] == 101);  // raw hide
    ASSERT(corpse->craft_mats[1] == 104);  // raw meat
    
    return 0;
}

// Test that bird corpses auto-derive meat only (if edible)
static int test_corpse_derive_bird()
{
    Room* room = mock_room(95002, NULL, NULL);
    ASSERT(room != NULL);
    
    MobPrototype* mp = mock_mob_proto(95002);
    ASSERT(mp != NULL);
    mp->craft_mats = NULL;
    mp->craft_mat_count = 0;
    
    Mobile* mob = create_mobile(mp);
    ASSERT(mob != NULL);
    mob->form = FORM_BIRD | FORM_EDIBLE;  // e.g., a chicken
    
    transfer_mob(mob, room);
    make_corpse(mob);
    
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    // Birds yield meat only (no hide)
    ASSERT(corpse->craft_mat_count == 1);
    ASSERT(corpse->craft_mats != NULL);
    ASSERT(corpse->craft_mats[0] == 104);  // raw meat
    
    return 0;
}

// Test that reptile corpses auto-derive meat (if edible)
static int test_corpse_derive_reptile()
{
    Room* room = mock_room(95003, NULL, NULL);
    ASSERT(room != NULL);
    
    MobPrototype* mp = mock_mob_proto(95003);
    ASSERT(mp != NULL);
    mp->craft_mats = NULL;
    mp->craft_mat_count = 0;
    
    Mobile* mob = create_mobile(mp);
    ASSERT(mob != NULL);
    mob->form = FORM_REPTILE | FORM_EDIBLE;  // e.g., a lizard
    
    transfer_mob(mob, room);
    make_corpse(mob);
    
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    ASSERT(corpse->craft_mat_count == 1);
    ASSERT(corpse->craft_mats != NULL);
    ASSERT(corpse->craft_mats[0] == 104);  // raw meat
    
    return 0;
}

// Test that explicit craft_mats override auto-derivation
static int test_corpse_derive_override()
{
    Room* room = mock_room(95004, NULL, NULL);
    ASSERT(room != NULL);
    
    // Create a mob prototype WITH explicit craft_mats
    MobPrototype* mp = mock_mob_proto(95004);
    ASSERT(mp != NULL);
    mp->craft_mat_count = 1;
    mp->craft_mats = malloc(sizeof(VNUM) * 1);
    mp->craft_mats[0] = 108;  // iron ore (for a special creature)
    
    // Create mob from prototype with mammal form
    Mobile* mob = create_mobile(mp);
    ASSERT(mob != NULL);
    mob->form = FORM_MAMMAL | FORM_EDIBLE;  // would normally yield hide + meat
    
    transfer_mob(mob, room);
    make_corpse(mob);
    
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    // Explicit craft_mats should override, not auto-derive
    ASSERT(corpse->craft_mat_count == 1);
    ASSERT(corpse->craft_mats != NULL);
    ASSERT(corpse->craft_mats[0] == 108);  // iron ore, not hide/meat
    
    return 0;
}

// Test that non-edible creatures don't yield meat
static int test_corpse_derive_non_edible()
{
    Room* room = mock_room(95005, NULL, NULL);
    ASSERT(room != NULL);
    
    MobPrototype* mp = mock_mob_proto(95005);
    ASSERT(mp != NULL);
    mp->craft_mats = NULL;
    mp->craft_mat_count = 0;
    
    Mobile* mob = create_mobile(mp);
    ASSERT(mob != NULL);
    mob->form = FORM_BIRD;  // Bird without FORM_EDIBLE
    
    transfer_mob(mob, room);
    make_corpse(mob);
    
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    // Non-edible bird should yield nothing
    ASSERT(corpse->craft_mat_count == 0);
    
    return 0;
}

// Test that mobs with no relevant form flags yield nothing
static int test_corpse_derive_no_form()
{
    Room* room = mock_room(95006, NULL, NULL);
    ASSERT(room != NULL);
    
    MobPrototype* mp = mock_mob_proto(95006);
    ASSERT(mp != NULL);
    mp->craft_mats = NULL;
    mp->craft_mat_count = 0;
    
    Mobile* mob = create_mobile(mp);
    ASSERT(mob != NULL);
    mob->form = 0;  // No form flags at all
    
    transfer_mob(mob, room);
    make_corpse(mob);
    
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    // No form flags = no materials
    ASSERT(corpse->craft_mat_count == 0);
    
    return 0;
}

// Test that FORM_EDIBLE alone yields meat
static int test_corpse_derive_edible_only()
{
    Room* room = mock_room(95007, NULL, NULL);
    ASSERT(room != NULL);
    
    MobPrototype* mp = mock_mob_proto(95007);
    ASSERT(mp != NULL);
    mp->craft_mats = NULL;
    mp->craft_mat_count = 0;
    
    Mobile* mob = create_mobile(mp);
    ASSERT(mob != NULL);
    mob->form = FORM_EDIBLE;  // Edible but not a specific animal type
    
    transfer_mob(mob, room);
    make_corpse(mob);
    
    Object* corpse = NULL;
    Object* obj = NULL;
    FOR_EACH_ROOM_OBJ(obj, room) {
        if (obj->item_type == ITEM_CORPSE_NPC) {
            corpse = obj;
            break;
        }
    }
    ASSERT(corpse != NULL);
    
    // Just FORM_EDIBLE should yield meat
    ASSERT(corpse->craft_mat_count == 1);
    ASSERT(corpse->craft_mats != NULL);
    ASSERT(corpse->craft_mats[0] == 104);  // raw meat
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Issue #37: Auto-derive salvage_mats from object material string
////////////////////////////////////////////////////////////////////////////////

// Test that leather objects can be salvaged for leather
static int test_salvage_derive_leather()
{
    Room* room = mock_room(96001, NULL, NULL);
    Mobile* ch = mock_player("Salvager");
    transfer_mob(ch, room);
    
    // Give the player the leatherworking skill
    ch->pcdata->learned[gsn_leatherworking] = 100;
    
    // Create an object with leather material but NO explicit salvage_mats
    ObjPrototype* op = mock_obj_proto(96001);
    ASSERT(op != NULL);
    op->item_type = ITEM_ARMOR;
    op->salvage_mats = NULL;
    op->salvage_mat_count = 0;
    free_string(op->material);
    op->material = str_dup("leather");
    
    Object* obj = create_object(op, 0);
    obj_to_char(obj, ch);
    
    // Evaluate salvage
    SalvageResult result = evaluate_salvage(ch, obj);
    
    ASSERT(result.success == true);
    ASSERT(result.mat_count == 1);
    ASSERT(result.mat_vnums != NULL);
    ASSERT(result.mat_vnums[0] == 102);  // tanned leather
    
    free_salvage_result(&result);
    return 0;
}

// Test that iron/steel objects can be salvaged for iron ingots
static int test_salvage_derive_iron()
{
    Room* room = mock_room(96002, NULL, NULL);
    Mobile* ch = mock_player("Smith");
    transfer_mob(ch, room);
    
    ch->pcdata->learned[gsn_blacksmithing] = 100;
    
    ObjPrototype* op = mock_obj_proto(96002);
    ASSERT(op != NULL);
    op->item_type = ITEM_WEAPON;
    op->salvage_mats = NULL;
    op->salvage_mat_count = 0;
    free_string(op->material);
    op->material = str_dup("iron");
    
    Object* obj = create_object(op, 0);
    obj_to_char(obj, ch);
    
    SalvageResult result = evaluate_salvage(ch, obj);
    
    ASSERT(result.success == true);
    ASSERT(result.mat_count == 1);
    ASSERT(result.mat_vnums[0] == 112);  // iron ingot
    
    free_salvage_result(&result);
    return 0;
}

// Test that steel also yields iron ingots
static int test_salvage_derive_steel()
{
    Room* room = mock_room(96003, NULL, NULL);
    Mobile* ch = mock_player("Steely");
    transfer_mob(ch, room);
    
    ch->pcdata->learned[gsn_blacksmithing] = 100;
    
    ObjPrototype* op = mock_obj_proto(96003);
    ASSERT(op != NULL);
    op->item_type = ITEM_WEAPON;
    op->salvage_mats = NULL;
    op->salvage_mat_count = 0;
    free_string(op->material);
    op->material = str_dup("steel");
    
    Object* obj = create_object(op, 0);
    obj_to_char(obj, ch);
    
    SalvageResult result = evaluate_salvage(ch, obj);
    
    ASSERT(result.success == true);
    ASSERT(result.mat_count == 1);
    ASSERT(result.mat_vnums[0] == 112);  // iron ingot
    
    free_salvage_result(&result);
    return 0;
}

// Test that cloth objects yield linen scraps
static int test_salvage_derive_cloth()
{
    Room* room = mock_room(96004, NULL, NULL);
    Mobile* ch = mock_player("Tailor");
    transfer_mob(ch, room);
    
    ch->pcdata->learned[gsn_tailoring] = 100;
    
    ObjPrototype* op = mock_obj_proto(96004);
    ASSERT(op != NULL);
    op->item_type = ITEM_ARMOR;
    op->salvage_mats = NULL;
    op->salvage_mat_count = 0;
    free_string(op->material);
    op->material = str_dup("cloth");
    
    Object* obj = create_object(op, 0);
    obj_to_char(obj, ch);
    
    SalvageResult result = evaluate_salvage(ch, obj);
    
    ASSERT(result.success == true);
    ASSERT(result.mat_count == 1);
    ASSERT(result.mat_vnums[0] == 114);  // linen scraps
    
    free_salvage_result(&result);
    return 0;
}

// Test that explicit salvage_mats override derivation
static int test_salvage_derive_override()
{
    Room* room = mock_room(96005, NULL, NULL);
    Mobile* ch = mock_player("Override");
    transfer_mob(ch, room);
    
    ch->pcdata->learned[gsn_blacksmithing] = 100;
    
    // Create object with leather material but explicit salvage_mats of iron
    ObjPrototype* op = mock_obj_proto(96005);
    ASSERT(op != NULL);
    op->item_type = ITEM_WEAPON;
    free_string(op->material);
    op->material = str_dup("leather");  // Material says leather
    
    // But explicit salvage_mats says iron ingot
    op->salvage_mat_count = 1;
    op->salvage_mats = malloc(sizeof(VNUM) * 1);
    op->salvage_mats[0] = 112;  // iron ingot
    
    Object* obj = create_object(op, 0);
    obj_to_char(obj, ch);
    
    SalvageResult result = evaluate_salvage(ch, obj);
    
    // Should use explicit salvage_mats, not derived from material
    ASSERT(result.success == true);
    ASSERT(result.mat_count == 1);
    ASSERT(result.mat_vnums[0] == 112);  // iron ingot, not leather
    
    free_salvage_result(&result);
    return 0;
}

// Test that unknown materials yield nothing
static int test_salvage_derive_unknown_material()
{
    Room* room = mock_room(96006, NULL, NULL);
    Mobile* ch = mock_player("Unknown");
    transfer_mob(ch, room);
    
    ObjPrototype* op = mock_obj_proto(96006);
    ASSERT(op != NULL);
    op->item_type = ITEM_TRASH;
    op->salvage_mats = NULL;
    op->salvage_mat_count = 0;
    free_string(op->material);
    op->material = str_dup("adamantite");  // Not in our derivation table
    
    Object* obj = create_object(op, 0);
    obj_to_char(obj, ch);
    
    SalvageResult result = evaluate_salvage(ch, obj);
    
    // Should fail - no explicit mats and unknown material
    ASSERT(result.success == false);
    ASSERT(result.error_msg != NULL);
    
    free_salvage_result(&result);
    return 0;
}

// Test that bronze objects yield bronze ingots
static int test_salvage_derive_bronze()
{
    Room* room = mock_room(96007, NULL, NULL);
    Mobile* ch = mock_player("Bronzer");
    transfer_mob(ch, room);
    
    ch->pcdata->learned[gsn_blacksmithing] = 100;
    
    ObjPrototype* op = mock_obj_proto(96007);
    ASSERT(op != NULL);
    op->item_type = ITEM_ARMOR;
    op->salvage_mats = NULL;
    op->salvage_mat_count = 0;
    free_string(op->material);
    op->material = str_dup("bronze");
    
    Object* obj = create_object(op, 0);
    obj_to_char(obj, ch);
    
    SalvageResult result = evaluate_salvage(ch, obj);
    
    ASSERT(result.success == true);
    ASSERT(result.mat_count == 1);
    ASSERT(result.mat_vnums[0] == 111);  // bronze ingot
    
    free_salvage_result(&result);
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
    REGISTER("Skin: Requires Tool", test_skin_requires_tool);
    REGISTER("Skin: Already Skinned", test_skin_already_skinned);
    REGISTER("Skin: No Skinnable Mats", test_skin_no_skinnable_mats);
    REGISTER("Skin: Success", test_skin_success);
    
    // Issue #27: Skin Command Skill Integration
    REGISTER("Skin: No Skill", test_skin_no_skill);
    REGISTER("Skin: Failure", test_skin_failure);
    REGISTER("Skin: Skill Improve", test_skin_skill_improve);
    
    // Issue #10: Butcher Command
    REGISTER("Butcher: No Argument", test_butcher_no_argument);
    REGISTER("Butcher: Requires Tool", test_butcher_requires_tool);
    REGISTER("Butcher: Already Butchered", test_butcher_already_butchered);
    REGISTER("Butcher: No Butcherable Mats", test_butcher_no_butcherable_mats);
    REGISTER("Butcher: Success", test_butcher_success);
    
    // Issue #12: Salvage Command
    REGISTER("Salvage: No Argument", test_salvage_no_argument);
    REGISTER("Salvage: Item Not Found", test_salvage_item_not_found);
    REGISTER("Salvage: No Salvage Mats", test_salvage_no_salvage_mats);
    REGISTER("Salvage: Success", test_salvage_success);
    
    // Issue #29: Salvage Skill Integration
    REGISTER("Salvage: Weapon Uses Blacksmithing", test_salvage_weapon_uses_blacksmithing);
    REGISTER("Salvage: Armor Uses Material Skill", test_salvage_armor_uses_material_skill);
    REGISTER("Salvage: No Skill Reduced Yield", test_salvage_no_skill_reduced_yield);
    REGISTER("Salvage: With Skill Full Yield", test_salvage_with_skill_full_yield);
    REGISTER("Salvage: No Skill Required", test_salvage_no_skill_required);
    
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
    
    // Issue #16: RecEdit
    REGISTER("RecEdit: Entry No Arg", test_recedit_entry_no_arg);
    REGISTER("RecEdit: Entry Valid", test_recedit_entry_valid);
    REGISTER("RecEdit: Create", test_recedit_create);
    REGISTER("RecEdit: Show", test_recedit_show);
    REGISTER("RecEdit: List", test_recedit_list);
    REGISTER("RecEdit: Skill", test_recedit_skill);
    REGISTER("RecEdit: Discovery", test_recedit_discovery);
    
    // Issue #22: AEdit Recipe
    REGISTER("AEdit: Recipe List Empty", test_aedit_recipe_list_empty);
    REGISTER("AEdit: Recipe Create", test_aedit_recipe_create);
    REGISTER("AEdit: Recipe Create Out Of Range", test_aedit_recipe_create_out_of_range);
    REGISTER("AEdit: Recipe Edit", test_aedit_recipe_edit);
    REGISTER("AEdit: Recipe Delete", test_aedit_recipe_delete);
    
    // do_olist: Standalone Object/Material List
    REGISTER("OList: Syntax Help", test_olist_syntax_help);
    REGISTER("OList: Objects Local", test_olist_obj_local);
    REGISTER("OList: Materials Local", test_olist_mat_local);
    REGISTER("OList: Material Filter", test_olist_mat_filter);
    REGISTER("OList: OLC Aware", test_olist_olc_aware);
    
    // Issue #23: Crafting Skills
    REGISTER("CraftSkill: All Loaded", test_craft_skills_loaded);
    REGISTER("CraftSkill: Lookup", test_craft_skill_lookup);
    REGISTER("CraftSkill: Names", test_craft_skill_names);
    
    // Issue #24: Crafting Skill Group
    REGISTER("CraftGroup: Exists", test_craft_group_exists);
    REGISTER("CraftGroup: Contains Skills", test_craft_group_contains_skills);
    REGISTER("CraftGroup: Rating", test_craft_group_rating);
    
    // Issue #25: Crafting Skill Checks
    REGISTER("SkillCheck: Success", test_evaluate_craft_check_success);
    REGISTER("SkillCheck: Failure", test_evaluate_craft_check_failure);
    REGISTER("SkillCheck: Quality Margin", test_evaluate_craft_check_quality_margin);
    REGISTER("SkillCheck: Level Bonus", test_evaluate_craft_check_level_bonus);
    REGISTER("SkillCheck: Target Clamp", test_evaluate_craft_check_target_clamp);
    REGISTER("QualityTier: Poor", test_quality_tier_poor);
    REGISTER("QualityTier: Normal", test_quality_tier_normal);
    REGISTER("QualityTier: Fine", test_quality_tier_fine);
    REGISTER("QualityTier: Exceptional", test_quality_tier_exceptional);
    REGISTER("QualityTier: Masterwork", test_quality_tier_masterwork);
    REGISTER("QualityTier: Names", test_quality_tier_names);
    REGISTER("CraftSkill: Effective Skill", test_craft_effective_skill);
    REGISTER("CraftSkill: Integration", test_craft_skill_check_integration);
    
    // Issue #30: Skill Improvement Difficulty
    REGISTER("CraftImprove: Easy", test_craft_improve_easy);
    REGISTER("CraftImprove: Medium", test_craft_improve_medium);
    REGISTER("CraftImprove: Hard", test_craft_improve_hard);
    REGISTER("CraftImprove: Boundary", test_craft_improve_boundary);
    
    // Issue #26: Craft Command Skill Integration
    REGISTER("CraftCmd: No Skill", test_craft_no_skill);
    REGISTER("CraftCmd: Skill Too Low", test_craft_skill_too_low);
    REGISTER("CraftCmd: Uses Skill Check", test_craft_evaluate_uses_skill_check);
    REGISTER("CraftCmd: Quality Tiers", test_craft_quality_tier_affects_output);
    
    // Issue #31: Crafting Skill Display
    REGISTER("SkillDisplay: Crafting Skills", test_skills_shows_crafting);
    REGISTER("SkillDisplay: Groups Crafting", test_groups_crafting_display);
    
    // Issue #32: Starter Material Objects
    REGISTER("StarterMats: Exist", test_starter_materials_exist);
    REGISTER("StarterMats: Item Type", test_starter_materials_item_type);
    REGISTER("StarterMats: Mat Type", test_starter_materials_mat_type);
    
    // Issue #33: Starter Workstation Objects
    REGISTER("StarterWork: Exist", test_starter_workstations_exist);
    REGISTER("StarterWork: Item Type", test_starter_workstations_item_type);
    REGISTER("StarterWork: Station Type", test_starter_workstations_station_type);
    REGISTER("StarterWork: Master Bonus", test_starter_workstations_master_forge_bonus);
    
    // Issue #34: Starter Recipe Objects
    REGISTER("StarterRecipe: Exist", test_starter_recipes_exist);
    REGISTER("StarterRecipe: Valid Inputs", test_starter_recipes_valid_inputs);
    REGISTER("StarterRecipe: Valid Outputs", test_starter_recipes_valid_outputs);
    REGISTER("StarterRecipe: Chain", test_starter_recipes_chain);
    REGISTER("StarterRecipe: Discovery", test_starter_recipes_discovery);
    
    // Issue #36: Auto-derive craft_mats from form flags
    REGISTER("CorpseDerive: Mammal", test_corpse_derive_mammal);
    REGISTER("CorpseDerive: Bird", test_corpse_derive_bird);
    REGISTER("CorpseDerive: Reptile", test_corpse_derive_reptile);
    REGISTER("CorpseDerive: Override", test_corpse_derive_override);
    REGISTER("CorpseDerive: Non Edible", test_corpse_derive_non_edible);
    REGISTER("CorpseDerive: No Form", test_corpse_derive_no_form);
    REGISTER("CorpseDerive: Edible Only", test_corpse_derive_edible_only);
    
    // Issue #37: Auto-derive salvage_mats from material string
    REGISTER("SalvageDerive: Leather", test_salvage_derive_leather);
    REGISTER("SalvageDerive: Iron", test_salvage_derive_iron);
    REGISTER("SalvageDerive: Steel", test_salvage_derive_steel);
    REGISTER("SalvageDerive: Cloth", test_salvage_derive_cloth);
    REGISTER("SalvageDerive: Override", test_salvage_derive_override);
    REGISTER("SalvageDerive: Unknown", test_salvage_derive_unknown_material);
    REGISTER("SalvageDerive: Bronze", test_salvage_derive_bronze);

#undef REGISTER
}
