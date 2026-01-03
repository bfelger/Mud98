////////////////////////////////////////////////////////////////////////////////
// craft/act_craft.c
// Crafting command implementations.
////////////////////////////////////////////////////////////////////////////////

#include "act_craft.h"

#include "craft.h"
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
#include <handler.h>
#include <interp.h>
#include <lookup.h>
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
// Corpse Extraction Helpers
////////////////////////////////////////////////////////////////////////////////

// Common extraction logic for both skin and butcher
static ExtractionResult evaluate_extraction(Mobile* ch, Object* corpse,
    bool (*type_filter)(CraftMatType), int already_flag,
    const char* already_msg, const char* nothing_msg)
{
    ExtractionResult result = { false, NULL, NULL, 0 };
    
    (void)ch;  // ch may be used for skill checks later
    
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
    result.success = true;
    return result;
}

ExtractionResult evaluate_skin(Mobile* ch, Object* corpse)
{
    return evaluate_extraction(ch, corpse,
        is_skinnable_type,
        CORPSE_SKINNED,
        "That corpse has already been skinned.",
        "There's nothing to skin from that corpse.");
}

ExtractionResult evaluate_butcher(Mobile* ch, Object* corpse)
{
    return evaluate_extraction(ch, corpse,
        is_butcherable_type,
        CORPSE_BUTCHERED,
        "That corpse has already been butchered.",
        "There's nothing to butcher from that corpse.");
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
    
    // Set the corpse flag
    corpse->corpse.extraction_flags |= corpse_flag;
    
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

SalvageResult evaluate_salvage(Mobile* ch, Object* obj)
{
    SalvageResult result = { false, NULL, NULL, 0 };
    
    (void)ch;  // Used for skill checks in later phases
    
    if (obj == NULL) {
        result.error_msg = "You don't have that.";
        return result;
    }
    
    if (obj->salvage_mat_count == 0 || obj->salvage_mats == NULL) {
        result.error_msg = "There's nothing to salvage from that.";
        return result;
    }
    
    // For MVP, no skill check required
    // Skill checks will be added in Phase 4
    
    // Calculate yield - for now, return all materials
    VNUM* mats = malloc((size_t)obj->salvage_mat_count * sizeof(VNUM));
    if (mats == NULL) {
        result.error_msg = "Memory allocation failed.";
        return result;
    }
    
    int count = 0;
    for (int i = 0; i < obj->salvage_mat_count; i++) {
        // For MVP, 100% recovery rate
        mats[count++] = obj->salvage_mats[i];
    }
    
    result.mat_vnums = mats;
    result.mat_count = count;
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

int calculate_craft_quality(Mobile* ch, Recipe* recipe)
{
    // Phase 4: implement quality based on skill check margin
    (void)ch;
    (void)recipe;
    return 0;  // Standard quality for now
}

CraftResult evaluate_craft(Mobile* ch, const char* recipe_name)
{
    CraftResult result = { false, false, NULL, NULL, 0 };
    
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
    
    // Check skill requirement (MVP: just check if they have any skill)
    if (recipe->required_skill >= 0) {
        int skill = get_skill(ch, recipe->required_skill);
        if (skill < recipe->min_skill_pct) {
            result.error_msg = "You're not skilled enough to craft that.";
            return result;
        }
    }
    
    // Check workstation requirement using helper function
    if (!has_required_workstation(ch->in_room, recipe)) {
        result.error_msg = "You need to be near a workstation to craft that.";
        return result;
    }
    
    // Check materials
    if (!has_materials(ch, recipe)) {
        result.error_msg = "You don't have the required materials.";
        return result;
    }
    
    result.can_attempt = true;
    
    // Skill check
    if (recipe->required_skill >= 0) {
        int skill = get_skill(ch, recipe->required_skill);
        result.success = (number_percent() <= skill);
    } else {
        result.success = true;  // No skill required = auto success
    }
    
    if (result.success) {
        result.quality = calculate_craft_quality(ch, recipe);
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
    
    // Send messages
    if (result.mat_count > 0) {
        printf_to_char(ch, "You salvage %s, recovering %s.\n\r",
            saved_name, sb_string(mat_list));
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
    
    // Consume materials regardless of success
    consume_materials(ch, result.recipe);
    
    if (!result.success) {
        printf_to_char(ch, "You fail to craft %s, wasting the materials.\n\r",
            NAME_STR(result.recipe) ? NAME_STR(result.recipe) : "the item");
        act("$n fails to craft something.", ch, NULL, NULL, TO_ROOM);
        return;
    }
    
    // Create output
    ObjPrototype* proto = get_object_prototype(result.recipe->product_vnum);
    if (proto == NULL) {
        send_to_char("Error: Recipe output does not exist.\n\r", ch);
        return;
    }
    
    for (int i = 0; i < result.recipe->product_quantity; i++) {
        Object* product = create_object(proto, 0);
        // TODO: Apply quality modifiers in Phase 4
        obj_to_char(product, ch);
    }
    
    printf_to_char(ch, "You successfully craft %s.\n\r",
        proto->short_descr ? proto->short_descr : NAME_STR(result.recipe));
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
