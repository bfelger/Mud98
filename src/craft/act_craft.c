////////////////////////////////////////////////////////////////////////////////
// craft/act_craft.c
// Crafting command implementations.
////////////////////////////////////////////////////////////////////////////////

#include "act_craft.h"

#include "craft.h"
#include "craft_skill.h"
#include "recipe.h"
#include "workstation.h"

#include <data/item.h>
#include <data/skill.h>

#include <entities/entity.h>
#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/obj_prototype.h>
#include <entities/room.h>

#include <comm.h>
#include <db.h>
#include <gsn.h>
#include <handler.h>
#include <interp.h>
#include <lookup.h>
#include <skills.h>
#include <stringbuffer.h>

#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Material Type Classification
////////////////////////////////////////////////////////////////////////////////

bool is_skinnable_type(CraftMatType type)
{
    return type == MAT_HIDE || type == MAT_LEATHER 
        || type == MAT_FUR || type == MAT_SCALE;
}

bool is_butcherable_type(CraftMatType type)
{
    return type == MAT_MEAT || type == MAT_BONE;
}

////////////////////////////////////////////////////////////////////////////////
// Salvage Material Derivation from Object Material String
////////////////////////////////////////////////////////////////////////////////

// Crafting material VNUMs for auto-derivation
#define VNUM_SALVAGE_LEATHER      102   // tanned leather
#define VNUM_SALVAGE_IRON_INGOT   112   // iron ingot
#define VNUM_SALVAGE_BRONZE_INGOT 111   // bronze ingot
#define VNUM_SALVAGE_COPPER_INGOT 109   // copper ingot
#define VNUM_SALVAGE_LINEN_SCRAPS 114   // linen scraps
#define VNUM_SALVAGE_WOOL         116   // raw wool
#define VNUM_SALVAGE_MEAT         104   // raw meat

// Derive salvage_mats from an object's material string
// Returns the number of materials added to the out_mats array
// Caller must provide an array of at least max_mats VNUMs
static int derive_salvage_mats_from_material(const char* material, VNUM* out_mats, int max_mats)
{
    if (material == NULL || material[0] == '\0' || max_mats <= 0)
        return 0;
    
    int count = 0;
    
    // Leather and organic materials
    if (!str_cmp(material, "leather") && count < max_mats) {
        out_mats[count++] = VNUM_SALVAGE_LEATHER;
    }
    // Metal materials - iron and steel both yield iron ingots
    else if ((!str_cmp(material, "iron") || !str_cmp(material, "steel")) && count < max_mats) {
        out_mats[count++] = VNUM_SALVAGE_IRON_INGOT;
    }
    // Bronze
    else if (!str_cmp(material, "bronze") && count < max_mats) {
        out_mats[count++] = VNUM_SALVAGE_BRONZE_INGOT;
    }
    // Copper
    else if (!str_cmp(material, "copper") && count < max_mats) {
        out_mats[count++] = VNUM_SALVAGE_COPPER_INGOT;
    }
    // Cloth and fabric
    else if ((!str_cmp(material, "cloth") || !str_cmp(material, "linen")) && count < max_mats) {
        out_mats[count++] = VNUM_SALVAGE_LINEN_SCRAPS;
    }
    // Wool
    else if (!str_cmp(material, "wool") && count < max_mats) {
        out_mats[count++] = VNUM_SALVAGE_WOOL;
    }
    // Food and meat
    else if ((!str_cmp(material, "food") || !str_cmp(material, "meat")) && count < max_mats) {
        out_mats[count++] = VNUM_SALVAGE_MEAT;
    }
    
    return count;
}

////////////////////////////////////////////////////////////////////////////////
// Corpse Extraction Helpers
////////////////////////////////////////////////////////////////////////////////

// Common extraction logic for both skin and butcher
static ExtractionResult evaluate_extraction(Mobile* ch, Object* corpse,
    bool (*type_filter)(CraftMatType), int already_flag,
    const char* already_msg, const char* nothing_msg,
    SKNUM required_skill, const char* skill_name)
{
    ExtractionResult result = { false, false, NULL, NULL, 0, -1, 0, 0 };
    
    // Check corpse validity
    if (corpse == NULL) {
        result.error_msg = "You don't see that here.";
        return result;
    }
    
    if (corpse->item_type != ITEM_CORPSE_NPC && corpse->item_type != ITEM_CORPSE_PC) {
        result.error_msg = "That's not a corpse.";
        return result;
    }
    
    // Check if already processed
    if (corpse->corpse.extraction_flags & already_flag) {
        result.error_msg = already_msg;
        return result;
    }
    
    // Check if player has the required skill
    int skill_pct = get_skill(ch, required_skill);
    if (skill_pct == 0) {
        static char no_skill_msg[128];
        snprintf(no_skill_msg, sizeof(no_skill_msg), 
            "You don't know how to %s.", skill_name);
        result.error_msg = no_skill_msg;
        return result;
    }
    
    // Count matching materials
    int match_count = 0;
    for (int i = 0; i < corpse->craft_mat_count; i++) {
        ObjPrototype* proto = get_object_prototype(corpse->craft_mats[i]);
        if (proto && proto->item_type == ITEM_MAT) {
            CraftMatType mat_type = (CraftMatType)proto->value[0];
            if (type_filter(mat_type)) {
                match_count++;
            }
        }
    }
    
    if (match_count == 0) {
        result.error_msg = nothing_msg;
        return result;
    }
    
    // Allocate and fill material array
    result.mat_vnums = malloc((size_t)match_count * sizeof(VNUM));
    if (result.mat_vnums == NULL) {
        result.error_msg = "Memory allocation failed.";
        return result;
    }
    
    int idx = 0;
    for (int i = 0; i < corpse->craft_mat_count; i++) {
        ObjPrototype* proto = get_object_prototype(corpse->craft_mats[i]);
        if (proto && proto->item_type == ITEM_MAT) {
            CraftMatType mat_type = (CraftMatType)proto->value[0];
            if (type_filter(mat_type)) {
                result.mat_vnums[idx++] = corpse->craft_mats[i];
            }
        }
    }
    
    result.mat_count = match_count;
    result.skill_used = required_skill;
    
    // Perform skill check
    int roll = number_percent();
    result.skill_roll = roll;
    result.skill_target = skill_pct;
    result.skill_passed = (roll <= skill_pct);
    
    result.success = true;
    return result;
}

ExtractionResult evaluate_skin(Mobile* ch, Object* corpse)
{
    return evaluate_extraction(ch, corpse,
        is_skinnable_type,
        CORPSE_SKINNED,
        "That corpse has already been skinned.",
        "There's nothing to skin from that corpse.",
        gsn_skinning,
        "skin");
}

ExtractionResult evaluate_butcher(Mobile* ch, Object* corpse)
{
    return evaluate_extraction(ch, corpse,
        is_butcherable_type,
        CORPSE_BUTCHERED,
        "That corpse has already been butchered.",
        "There's nothing to butcher from that corpse.",
        gsn_butchering,
        "butcher");
}

void free_extraction_result(ExtractionResult* result)
{
    if (result && result->mat_vnums) {
        free(result->mat_vnums);
        result->mat_vnums = NULL;
        result->mat_count = 0;
    }
}

void apply_extraction(Mobile* ch, Object* corpse, ExtractionResult* result,
                      int corpse_flag, const char* verb)
{
    if (!result->success || result->mat_count == 0)
        return;
    
    // Always set the corpse flag (can't retry even on failure)
    corpse->corpse.extraction_flags |= corpse_flag;
    
    // Skill improvement check
    if (result->skill_used >= 0) {
        check_improve(ch, result->skill_used, result->skill_passed, 2);
    }
    
    // Handle skill failure
    if (!result->skill_passed) {
        printf_to_char(ch, "You botch the %sing, ruining the materials.\n\r", verb);
        act("$n botches $s attempt to $t $p.", ch, corpse, verb, TO_ROOM);
        return;
    }
    
    // Create and give materials
    StringBuffer* mat_list = sb_new();
    for (int i = 0; i < result->mat_count; i++) {
        ObjPrototype* proto = get_object_prototype(result->mat_vnums[i]);
        if (proto) {
            Object* mat = create_object(proto, 0);
            obj_to_char(mat, ch);
            
            if (i > 0) {
                if (i == result->mat_count - 1)
                    sb_append(mat_list, " and ");
                else
                    sb_append(mat_list, ", ");
            }
            sb_append(mat_list, proto->short_descr ? proto->short_descr : "something");
        }
    }
    
    // Send messages
    printf_to_char(ch, "You carefully %s %s, obtaining %s.\n\r",
        verb, corpse->short_descr ? corpse->short_descr : "the corpse",
        sb_string(mat_list));
    
    act("$n $ts $p.", ch, corpse, verb, TO_ROOM);
    
    sb_free(mat_list);
}

////////////////////////////////////////////////////////////////////////////////
// Salvage Helpers
////////////////////////////////////////////////////////////////////////////////

SKNUM get_salvage_skill(Object* obj)
{
    if (obj == NULL)
        return -1;
    
    // Weapons use blacksmithing
    if (obj->item_type == ITEM_WEAPON)
        return gsn_blacksmithing;
    
    // Armor depends on material
    if (obj->item_type == ITEM_ARMOR) {
        // Check material string for hints
        if (obj->material != NULL && obj->material[0] != '\0') {
            if (strstr(obj->material, "leather") != NULL 
                || strstr(obj->material, "hide") != NULL)
                return gsn_leatherworking;
            if (strstr(obj->material, "cloth") != NULL 
                || strstr(obj->material, "silk") != NULL
                || strstr(obj->material, "wool") != NULL)
                return gsn_tailoring;
            if (strstr(obj->material, "metal") != NULL
                || strstr(obj->material, "iron") != NULL
                || strstr(obj->material, "steel") != NULL
                || strstr(obj->material, "mithril") != NULL)
                return gsn_blacksmithing;
        }
        // Default armor to blacksmithing
        return gsn_blacksmithing;
    }
    
    // Jewelry uses jewelcraft
    if (obj->item_type == ITEM_JEWELRY)
        return gsn_jewelcraft;
    
    // No skill required for other item types
    return -1;
}

SalvageResult evaluate_salvage(Mobile* ch, Object* obj)
{
    SalvageResult result = { false, false, false, NULL, NULL, 0, -1, 0, 0 };
    
    if (obj == NULL) {
        result.error_msg = "You don't have that.";
        return result;
    }
    
    // Use explicit salvage_mats if present, otherwise derive from material string
    VNUM* salvage_mats = obj->salvage_mats;
    int salvage_mat_count = obj->salvage_mat_count;
    VNUM derived_mats[4];  // Max 4 derived materials
    int derived_count = 0;
    
    if (salvage_mat_count == 0 || salvage_mats == NULL) {
        // Try to derive from material string
        derived_count = derive_salvage_mats_from_material(obj->material, derived_mats, 4);
        if (derived_count > 0) {
            salvage_mats = derived_mats;
            salvage_mat_count = derived_count;
        } else {
            result.error_msg = "There's nothing to salvage from that.";
            return result;
        }
    }
    
    // Determine required skill and perform check
    SKNUM required_skill = get_salvage_skill(obj);
    result.skill_used = required_skill;
    
    int skill_pct = 0;
    bool has_skill = true;
    
    if (required_skill >= 0) {
        skill_pct = get_skill(ch, required_skill);
        result.skill_pct = skill_pct;
        result.has_skill = (skill_pct > 0);
        has_skill = result.has_skill;
    } else {
        // No skill required - always succeeds
        result.has_skill = true;
        result.skill_passed = true;
        skill_pct = 100;  // Treat as auto-success
    }
    
    // Perform skill roll if skill is required
    int roll = number_percent();
    result.skill_roll = roll;
    
    if (required_skill >= 0 && has_skill) {
        result.skill_passed = (roll <= skill_pct);
    }
    
    // Calculate yield based on skill result
    // No skill: 25% of materials
    // Has skill but failed: 50% of materials
    // Has skill and passed: 75-100% based on margin
    int yield_pct;
    if (required_skill < 0) {
        yield_pct = 100;  // No skill required, full yield
    } else if (!has_skill) {
        yield_pct = 25;
    } else if (!result.skill_passed) {
        yield_pct = 50;
    } else {
        // Success: 75-100% based on margin
        int margin = skill_pct - roll;
        yield_pct = 75 + (margin / 4);  // +1% per 4 margin
        if (yield_pct > 100) yield_pct = 100;
    }
    
    // Calculate how many materials to give
    int base_count = salvage_mat_count;
    int yield_count = (base_count * yield_pct + 50) / 100;  // Round
    if (yield_count < 1 && base_count > 0) yield_count = 1;  // At least 1
    
    VNUM* mats = malloc((size_t)yield_count * sizeof(VNUM));
    if (mats == NULL) {
        result.error_msg = "Memory allocation failed.";
        return result;
    }
    
    // Copy first yield_count materials
    for (int i = 0; i < yield_count; i++) {
        mats[i] = salvage_mats[i];
    }
    
    result.mat_vnums = mats;
    result.mat_count = yield_count;
    result.success = true;
    return result;
}

void free_salvage_result(SalvageResult* result)
{
    if (result && result->mat_vnums) {
        free(result->mat_vnums);
        result->mat_vnums = NULL;
        result->mat_count = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Crafting Helpers
////////////////////////////////////////////////////////////////////////////////

bool knows_recipe(Mobile* ch, Recipe* recipe)
{
    if (recipe == NULL)
        return false;
    
    (void)ch;  // Used in later phases for learned recipe tracking
    
    switch (recipe->discovery) {
        case DISC_KNOWN:
            return true;  // Always known
        case DISC_TRAINER:
        case DISC_SCROLL:
        case DISC_DISCOVERY:
        case DISC_QUEST:
            // TODO: Check player's learned recipes list (Phase 4)
            // For MVP, only DISC_KNOWN is supported
            return false;
        default:
            return false;
    }
}

Recipe* find_known_recipe(Mobile* ch, const char* name)
{
    if (IS_NULLSTR(name))
        return NULL;
    
    RecipeIter iter = make_recipe_iter();
    Recipe* recipe;
    while ((recipe = recipe_iter_next(&iter)) != NULL) {
        if (knows_recipe(ch, recipe) && !str_prefix(name, NAME_STR(recipe))) {
            return recipe;
        }
    }
    return NULL;
}

bool has_materials(Mobile* ch, Recipe* recipe)
{
    if (recipe == NULL)
        return false;
    
    for (int i = 0; i < recipe->ingredient_count; i++) {
        VNUM mat_vnum = recipe->ingredients[i].mat_vnum;
        int needed = recipe->ingredients[i].quantity;
        int have = 0;
        
        Object* obj;
        FOR_EACH_MOB_OBJ(obj, ch) {
            if (obj->prototype && VNUM_FIELD(obj->prototype) == mat_vnum) {
                // Check stack amount for ITEM_MAT
                if (obj->item_type == ITEM_MAT) {
                    have += obj->craft_mat.amount > 0 ? obj->craft_mat.amount : 1;
                } else {
                    have++;
                }
            }
        }
        
        if (have < needed)
            return false;
    }
    return true;
}

void consume_materials(Mobile* ch, Recipe* recipe)
{
    if (recipe == NULL)
        return;
    
    for (int i = 0; i < recipe->ingredient_count; i++) {
        VNUM mat_vnum = recipe->ingredients[i].mat_vnum;
        int to_consume = recipe->ingredients[i].quantity;
        
        // We need to collect objects to remove first, then remove them
        // The FOR_EACH_MOB_OBJ macro handles safe iteration during removal
        Object* obj;
        FOR_EACH_MOB_OBJ(obj, ch) {
            if (to_consume <= 0)
                break;
            
            if (obj->prototype && VNUM_FIELD(obj->prototype) == mat_vnum) {
                if (obj->item_type == ITEM_MAT) {
                    int stack = obj->craft_mat.amount > 0 ? obj->craft_mat.amount : 1;
                    if (stack <= to_consume) {
                        to_consume -= stack;
                        extract_obj(obj);
                    } else {
                        obj->craft_mat.amount -= to_consume;
                        to_consume = 0;
                    }
                } else {
                    to_consume--;
                    extract_obj(obj);
                }
            }
        }
    }
}

// Consume partial materials (50%, rounded up) on crafting failure
void consume_materials_partial(Mobile* ch, Recipe* recipe)
{
    if (recipe == NULL)
        return;
    
    // Consume half of the ingredient types (rounded up)
    int ingredients_to_consume = (recipe->ingredient_count + 1) / 2;
    
    for (int i = 0; i < ingredients_to_consume; i++) {
        VNUM mat_vnum = recipe->ingredients[i].mat_vnum;
        int to_consume = recipe->ingredients[i].quantity;
        
        Object* obj;
        FOR_EACH_MOB_OBJ(obj, ch) {
            if (to_consume <= 0)
                break;
            
            if (obj->prototype && VNUM_FIELD(obj->prototype) == mat_vnum) {
                if (obj->item_type == ITEM_MAT) {
                    int stack = obj->craft_mat.amount > 0 ? obj->craft_mat.amount : 1;
                    if (stack <= to_consume) {
                        to_consume -= stack;
                        extract_obj(obj);
                    } else {
                        obj->craft_mat.amount -= to_consume;
                        to_consume = 0;
                    }
                } else {
                    to_consume--;
                    extract_obj(obj);
                }
            }
        }
    }
}

int calculate_craft_quality(Mobile* ch, Recipe* recipe)
{
    // Deprecated - now use CraftCheckResult.quality directly
    (void)ch;
    (void)recipe;
    return 0;
}

CraftResult evaluate_craft(Mobile* ch, const char* recipe_name)
{
    CraftResult result = { .can_attempt = false, .success = false, 
                           .error_msg = NULL, .recipe = NULL, .check = {0} };
    
    if (IS_NULLSTR(recipe_name)) {
        result.error_msg = "Craft what?";
        return result;
    }
    
    Recipe* recipe = find_known_recipe(ch, recipe_name);
    if (recipe == NULL) {
        result.error_msg = "You don't know that recipe.";
        return result;
    }
    result.recipe = recipe;
    
    // Check level requirement
    if (ch->level < recipe->min_level) {
        result.error_msg = "You need to be a higher level to craft that.";
        return result;
    }
    
    // Check skill requirement - player must have the skill
    int skill = 0;
    if (recipe->required_skill >= 0) {
        skill = get_skill(ch, recipe->required_skill);
        if (skill == 0) {
            result.error_msg = "You don't know how to do that type of crafting.";
            return result;
        }
        if (skill < recipe->min_skill) {
            result.error_msg = "You're not skilled enough to craft that.";
            return result;
        }
    }
    
    // Check workstation requirement
    Object* station = NULL;
    if (!has_required_workstation(ch->in_room, recipe, &station)) {
        result.error_msg = "You need to be near a workstation to craft that.";
        return result;
    }
    if (station != NULL && recipe->required_skill >= 0
        && station->workstation.min_skill > 0
        && skill < station->workstation.min_skill) {
        result.error_msg = "You're not skilled enough to use that workstation.";
        return result;
    }
    
    // Check materials
    if (!has_materials(ch, recipe)) {
        result.error_msg = "You don't have the required materials.";
        return result;
    }
    
    result.can_attempt = true;
    
    // Perform skill check using the new system
    if (recipe->required_skill >= 0) {
        result.check = craft_skill_check(ch, recipe);
        result.success = result.check.success;
    } else {
        // No skill required = auto success with normal quality
        result.success = true;
        result.check.success = true;
        result.check.quality = 30;  // Normal quality range
        result.check.skill_used = -1;
    }
    
    return result;
}

////////////////////////////////////////////////////////////////////////////////
// Command Implementations
////////////////////////////////////////////////////////////////////////////////

void do_skin(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);
    
    if (arg[0] == '\0') {
        send_to_char("Skin what?\n\r", ch);
        return;
    }
    
    Object* corpse = get_obj_here(ch, arg);
    ExtractionResult result = evaluate_skin(ch, corpse);
    
    if (!result.success) {
        send_to_char(result.error_msg, ch);
        send_to_char("\n\r", ch);
        free_extraction_result(&result);
        return;
    }
    
    apply_extraction(ch, corpse, &result, CORPSE_SKINNED, "skin");
    free_extraction_result(&result);
}

void do_butcher(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);
    
    if (arg[0] == '\0') {
        send_to_char("Butcher what?\n\r", ch);
        return;
    }
    
    Object* corpse = get_obj_here(ch, arg);
    ExtractionResult result = evaluate_butcher(ch, corpse);
    
    if (!result.success) {
        send_to_char(result.error_msg, ch);
        send_to_char("\n\r", ch);
        free_extraction_result(&result);
        return;
    }
    
    apply_extraction(ch, corpse, &result, CORPSE_BUTCHERED, "butcher");
    free_extraction_result(&result);
}

void do_salvage(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);
    
    if (arg[0] == '\0') {
        send_to_char("Salvage what?\n\r", ch);
        return;
    }
    
    Object* obj = get_obj_carry(ch, arg);
    SalvageResult result = evaluate_salvage(ch, obj);
    
    if (!result.success) {
        send_to_char(result.error_msg, ch);
        send_to_char("\n\r", ch);
        free_salvage_result(&result);
        return;
    }
    
    // Call check_improve if a skill was used
    if (result.skill_used >= 0 && result.has_skill) {
        check_improve(ch, result.skill_used, result.skill_passed, 2);
    }
    
    // Build material list string
    StringBuffer* mat_list = sb_new();
    for (int i = 0; i < result.mat_count; i++) {
        ObjPrototype* proto = get_object_prototype(result.mat_vnums[i]);
        if (proto) {
            Object* mat = create_object(proto, 0);
            obj_to_char(mat, ch);
            
            if (i > 0) {
                if (i == result.mat_count - 1)
                    sb_append(mat_list, " and ");
                else
                    sb_append(mat_list, ", ");
            }
            sb_append(mat_list, proto->short_descr ? proto->short_descr : "something");
        }
    }
    
    // Destroy the original object
    const char* obj_name = obj->short_descr ? obj->short_descr : "something";
    char saved_name[256];
    snprintf(saved_name, sizeof(saved_name), "%s", obj_name);
    extract_obj(obj);
    
    // Send messages based on skill outcome
    if (result.mat_count > 0) {
        if (result.skill_used >= 0 && !result.has_skill) {
            printf_to_char(ch, "You clumsily salvage %s, recovering only %s.\n\r",
                saved_name, sb_string(mat_list));
        } else if (result.skill_used >= 0 && !result.skill_passed) {
            printf_to_char(ch, "You salvage %s with some difficulty, recovering %s.\n\r",
                saved_name, sb_string(mat_list));
        } else {
            printf_to_char(ch, "You salvage %s, recovering %s.\n\r",
                saved_name, sb_string(mat_list));
        }
    } else {
        printf_to_char(ch, "You salvage %s but recover nothing useful.\n\r",
            saved_name);
    }
    
    act("$n salvages something.", ch, NULL, NULL, TO_ROOM);
    
    sb_free(mat_list);
    free_salvage_result(&result);
}

void do_craft(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);
    
    CraftResult result = evaluate_craft(ch, arg);
    
    if (!result.can_attempt) {
        send_to_char(result.error_msg, ch);
        send_to_char("\n\r", ch);
        return;
    }
    
    // Skill improvement check (for both success and failure)
    if (result.check.skill_used >= 0) {
        int multiplier = craft_improve_multiplier(result.recipe, ch);
        check_improve(ch, result.check.skill_used, result.success, multiplier);
    }
    
    if (!result.success) {
        // On failure, consume only half the materials
        consume_materials_partial(ch, result.recipe);
        printf_to_char(ch, "You fail to craft %s, ruining some materials.\n\r",
            NAME_STR(result.recipe) ? NAME_STR(result.recipe) : "the item");
        act("$n fails to craft something.", ch, NULL, NULL, TO_ROOM);
        return;
    }
    
    // On success, consume all materials
    consume_materials(ch, result.recipe);
    
    // Create output
    ObjPrototype* proto = get_object_prototype(result.recipe->product_vnum);
    if (proto == NULL) {
        send_to_char("Error: Recipe output does not exist.\n\r", ch);
        return;
    }
    
    QualityTier tier = quality_tier_from_margin(result.check.quality);
    
    for (int i = 0; i < result.recipe->product_quantity; i++) {
        Object* product = create_object(proto, 0);
        
        // Apply quality modifiers
        switch (tier) {
        case QUALITY_POOR:
            // -10% to cost
            product->cost = product->cost * 9 / 10;
            break;
        case QUALITY_FINE:
            // +10% to cost
            product->cost = product->cost * 11 / 10;
            break;
        case QUALITY_EXCEPTIONAL:
            // +25% to cost
            product->cost = product->cost * 5 / 4;
            break;
        case QUALITY_MASTERWORK:
            // +50% to cost
            product->cost = product->cost * 3 / 2;
            break;
        default:  // QUALITY_NORMAL
            break;
        }
        
        obj_to_char(product, ch);
    }
    
    // Quality-specific messages
    const char* item_name = proto->short_descr ? proto->short_descr 
                                               : NAME_STR(result.recipe);
    switch (tier) {
    case QUALITY_POOR:
        printf_to_char(ch, "You craft a poor quality %s.\n\r", item_name);
        break;
    case QUALITY_FINE:
        printf_to_char(ch, "You craft a fine %s!\n\r", item_name);
        break;
    case QUALITY_EXCEPTIONAL:
        printf_to_char(ch, "You craft an exceptional %s!\n\r", item_name);
        break;
    case QUALITY_MASTERWORK:
        printf_to_char(ch, "You craft a masterwork %s!!\n\r", item_name);
        break;
    default:  // QUALITY_NORMAL
        printf_to_char(ch, "You successfully craft %s.\n\r", item_name);
        break;
    }
    act("$n crafts something.", ch, NULL, NULL, TO_ROOM);
}

void do_recipes(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);
    
    StringBuffer* sb = sb_new();
    int count = 0;
    
    sb_append(sb, "Known Recipes:\n\r\n\r");
    
    RecipeIter iter = make_recipe_iter();
    Recipe* recipe;
    while ((recipe = recipe_iter_next(&iter)) != NULL) {
        if (!knows_recipe(ch, recipe))
            continue;
        
        // Filter by profession if argument given
        if (arg[0] != '\0') {
            // TODO: Filter by skill name in Phase 4
        }
        
        count++;
        
        // Recipe name and level
        sb_appendf(sb, "  %s (Lv %d)",
            NAME_STR(recipe) ? NAME_STR(recipe) : "unknown",
            recipe->min_level);
        
        // Inputs
        sb_append(sb, " - ");
        for (int i = 0; i < recipe->ingredient_count; i++) {
            if (i > 0) sb_append(sb, ", ");
            ObjPrototype* proto = get_object_prototype(recipe->ingredients[i].mat_vnum);
            sb_appendf(sb, "%dx %s",
                recipe->ingredients[i].quantity,
                proto && proto->short_descr ? proto->short_descr : "unknown");
        }
        
        // Output
        sb_append(sb, " -> ");
        ObjPrototype* out_proto = get_object_prototype(recipe->product_vnum);
        if (recipe->product_quantity > 1) {
            sb_appendf(sb, "%dx %s",
                recipe->product_quantity,
                out_proto && out_proto->short_descr ? out_proto->short_descr : "unknown");
        } else {
            sb_append(sb, out_proto && out_proto->short_descr ? out_proto->short_descr : "unknown");
        }
        
        // Workstation requirement
        if (recipe->station_type != WORK_NONE) {
            char buf[256];
            workstation_type_flags_string(recipe->station_type, buf, sizeof(buf));
            sb_appendf(sb, " [%s]", buf);
        }
        
        sb_append(sb, "\n\r");
    }
    
    if (count == 0) {
        sb_free(sb);
        send_to_char("You don't know any recipes.\n\r", ch);
        return;
    }
    
    page_to_char(sb_string(sb), ch);
    sb_free(sb);
}
