////////////////////////////////////////////////////////////////////////////////
// persist/recipe/json/recipe_persist_json.c
// JSON format persistence for recipes.
////////////////////////////////////////////////////////////////////////////////

#include "recipe_persist_json.h"

#include <craft/craft.h>
#include <craft/recipe.h>

#include <data/skill.h>

#include <persist/json/persist_json.h>

#include <db.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Format Definition
////////////////////////////////////////////////////////////////////////////////

const RecipePersistFormat RECIPE_PERSIST_JSON = {
    .name = "json",
    .load = recipe_persist_json_load,
    .save = recipe_persist_json_save
};

////////////////////////////////////////////////////////////////////////////////
// Load
////////////////////////////////////////////////////////////////////////////////

PersistResult recipe_persist_json_load(const PersistReader* reader, const char* filename, Entity* owner)
{
    // For standalone recipe file loading (not embedded in area)
    (void)reader;
    (void)filename;
    (void)owner;
    return (PersistResult){ PERSIST_OK, NULL, 0 };
}

////////////////////////////////////////////////////////////////////////////////
// Save
////////////////////////////////////////////////////////////////////////////////

PersistResult recipe_persist_json_save(const PersistWriter* writer, const char* filename, Entity* owner)
{
    // For standalone recipe file saving (not embedded in area)
    (void)writer;
    (void)filename;
    (void)owner;
    return (PersistResult){ PERSIST_OK, NULL, 0 };
}

////////////////////////////////////////////////////////////////////////////////
// JSON Building/Parsing Helpers (for embedding in area JSON)
////////////////////////////////////////////////////////////////////////////////

static json_t* build_single_recipe(const Recipe* recipe)
{
    json_t* obj = json_object();
    
    JSON_SET_INT(obj, "vnum", VNUM_FIELD(recipe));
    
    if (NAME_STR(recipe) && NAME_STR(recipe)[0] != '\0')
        JSON_SET_STRING(obj, "name", NAME_STR(recipe));
    
    // Required skill
    if (recipe->required_skill >= 0) {
        const char* skill_name = skill_table[recipe->required_skill].name;
        if (skill_name)
            JSON_SET_STRING(obj, "skill", skill_name);
    }
    
    if (recipe->min_skill > 0)
        JSON_SET_INT(obj, "minSkill", recipe->min_skill);
    
    if (recipe->min_level > 1)
        JSON_SET_INT(obj, "minLevel", recipe->min_level);
    
    // Workstation type (flags -> array of names)
    if (recipe->station_type != WORK_NONE) {
        json_t* stations = json_array();
        for (WorkstationType flag = WORK_FORGE; flag <= WORK_JEWELER; flag <<= 1) {
            if (recipe->station_type & flag) {
                const char* name = workstation_type_name(flag);
                if (name && str_cmp(name, "none") != 0)
                    json_array_append_new(stations, json_string(name));
            }
        }
        if (json_array_size(stations) > 0)
            json_object_set_new(obj, "stationType", stations);
        else
            json_decref(stations);
    }
    
    if (recipe->station_vnum != VNUM_NONE && recipe->station_vnum != 0)
        JSON_SET_INT(obj, "stationVnum", recipe->station_vnum);
    
    // Discovery type
    if (recipe->discovery != DISC_KNOWN) {
        const char* disc_name = discovery_type_name(recipe->discovery);
        if (disc_name)
            JSON_SET_STRING(obj, "discovery", disc_name);
    }
    
    // Ingredients
    if (recipe->ingredient_count > 0) {
        json_t* inputs = json_array();
        for (int i = 0; i < recipe->ingredient_count; i++) {
            json_t* input = json_object();
            JSON_SET_INT(input, "vnum", recipe->ingredients[i].mat_vnum);
            JSON_SET_INT(input, "quantity", recipe->ingredients[i].quantity);
            json_array_append_new(inputs, input);
        }
        json_object_set_new(obj, "inputs", inputs);
    }
    
    // Output
    if (recipe->product_vnum != VNUM_NONE)
        JSON_SET_INT(obj, "outputVnum", recipe->product_vnum);
    if (recipe->product_quantity != 1)
        JSON_SET_INT(obj, "outputQuantity", recipe->product_quantity);
    
    return obj;
}

json_t* recipe_persist_json_build(const Entity* owner)
{
    json_t* arr = json_array();
    
    // Iterate all recipes, filter by owner area if provided
    RecipeIter iter = make_recipe_iter();
    Recipe* recipe;
    while ((recipe = recipe_iter_next(&iter)) != NULL) {
        // If owner is specified, only include recipes belonging to that area
        if (owner != NULL && recipe->area != (AreaData*)owner)
            continue;
        
        json_t* obj = build_single_recipe(recipe);
        json_array_append_new(arr, obj);
    }
    
    return arr;
}

static void parse_single_recipe(json_t* obj, Entity* owner)
{
    if (!json_is_object(obj))
        return;
    
    Recipe* recipe = new_recipe();
    
    VNUM_FIELD(recipe) = (VNUM)json_int_or_default(obj, "vnum", VNUM_NONE);
    
    const char* name = JSON_STRING(obj, "name");
    if (name && name[0] != '\0')
        recipe->header.name = AS_STRING(OBJ_VAL(copy_string(name, (int)strlen(name))));
    
    // Required skill
    const char* skill_name = JSON_STRING(obj, "skill");
    if (skill_name) {
        recipe->required_skill = skill_lookup(skill_name);
    }
    
    int min_skill = (int)json_int_or_default(obj, "minSkill", -1);
    if (min_skill < 0)
        min_skill = (int)json_int_or_default(obj, "minSkillPct", 0);
    recipe->min_skill = (int16_t)min_skill;
    recipe->min_level = (LEVEL)json_int_or_default(obj, "minLevel", 1);
    
    // Workstation type (array of names -> flags)
    json_t* stations = json_object_get(obj, "stationType");
    if (json_is_array(stations)) {
        recipe->station_type = WORK_NONE;
        size_t count = json_array_size(stations);
        for (size_t i = 0; i < count; i++) {
            json_t* s = json_array_get(stations, i);
            if (json_is_string(s)) {
                WorkstationType flag = workstation_type_lookup(json_string_value(s));
                recipe->station_type |= flag;
            }
        }
    }
    
    recipe->station_vnum = (VNUM)json_int_or_default(obj, "stationVnum", VNUM_NONE);
    
    // Discovery type
    const char* disc = JSON_STRING(obj, "discovery");
    if (disc) {
        recipe->discovery = discovery_type_lookup(disc);
    }
    
    // Ingredients
    json_t* inputs = json_object_get(obj, "inputs");
    if (json_is_array(inputs)) {
        size_t count = json_array_size(inputs);
        for (size_t i = 0; i < count && recipe->ingredient_count < MAX_RECIPE_INGREDIENTS; i++) {
            json_t* input = json_array_get(inputs, i);
            if (json_is_object(input)) {
                VNUM mat_vnum = (VNUM)json_int_or_default(input, "vnum", VNUM_NONE);
                int16_t quantity = (int16_t)json_int_or_default(input, "quantity", 1);
                recipe_add_ingredient(recipe, mat_vnum, quantity);
            }
        }
    }
    
    // Output
    recipe->product_vnum = (VNUM)json_int_or_default(obj, "outputVnum", VNUM_NONE);
    recipe->product_quantity = (int16_t)json_int_or_default(obj, "outputQuantity", 1);
    
    // Set owner area
    recipe->area = (AreaData*)owner;
    
    // Add to global recipe table
    add_recipe(recipe);
}

void recipe_persist_json_parse(json_t* recipe_arr, Entity* owner)
{
    if (!json_is_array(recipe_arr))
        return;
    
    size_t count = json_array_size(recipe_arr);
    for (size_t i = 0; i < count; i++) {
        json_t* obj = json_array_get(recipe_arr, i);
        parse_single_recipe(obj, owner);
    }
}
