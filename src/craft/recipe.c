////////////////////////////////////////////////////////////////////////////////
// craft/recipe.c
// Recipe management implementation using OrderedTable.
////////////////////////////////////////////////////////////////////////////////

#include "recipe.h"

#include "db.h"
#include "entities/entity.h"
#include "tables.h"

#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Global Recipe Table
////////////////////////////////////////////////////////////////////////////////

static OrderedTable recipes;

void init_global_recipes(void)
{
    ordered_table_init(&recipes);
}

void free_global_recipes(void)
{
    ordered_table_free(&recipes);
}

////////////////////////////////////////////////////////////////////////////////
// Type Checking Macros
////////////////////////////////////////////////////////////////////////////////

#define IS_RECIPE(value) \
    (IS_OBJ(value) && AS_OBJ(value)->type == OBJ_RECIPE)

#define AS_RECIPE(value) \
    ((Recipe*)AS_OBJ(value))

////////////////////////////////////////////////////////////////////////////////
// Lifecycle
////////////////////////////////////////////////////////////////////////////////

Recipe* new_recipe(void)
{
    Recipe* recipe = calloc(1, sizeof(Recipe));
    if (recipe == NULL) {
        perror("new_recipe: calloc failed");
        exit(1);
    }

    init_header(&recipe->header, OBJ_RECIPE);

    recipe->required_skill = -1;
    recipe->min_skill_pct = 0;
    recipe->min_level = 1;
    recipe->station_type = WORK_NONE;
    recipe->station_vnum = VNUM_NONE;
    recipe->discovery = DISC_KNOWN;
    recipe->input_count = 0;
    recipe->output_vnum = VNUM_NONE;
    recipe->output_quantity = 1;

    return recipe;
}

void free_recipe(Recipe* recipe)
{
    if (recipe == NULL)
        return;

    // Note: header.name is an ObjString* managed by the Lox VM GC
    // We don't free it here

    free(recipe);
}

////////////////////////////////////////////////////////////////////////////////
// Lookup
////////////////////////////////////////////////////////////////////////////////

Recipe* get_recipe(VNUM vnum)
{
    if (vnum == VNUM_NONE)
        return NULL;

    Value value;
    if (ordered_table_get_vnum(&recipes, vnum, &value) && IS_RECIPE(value))
        return AS_RECIPE(value);

    return NULL;
}

Recipe* get_recipe_by_name(const char* name)
{
    if (name == NULL || name[0] == '\0')
        return NULL;

    // Exact match first
    RecipeIter iter = make_recipe_iter();
    Recipe* recipe;
    while ((recipe = recipe_iter_next(&iter)) != NULL) {
        if (!str_cmp(NAME_STR(recipe), name))
            return recipe;
    }

    // Prefix match
    iter = make_recipe_iter();
    while ((recipe = recipe_iter_next(&iter)) != NULL) {
        if (!str_prefix(name, NAME_STR(recipe)))
            return recipe;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Registration
////////////////////////////////////////////////////////////////////////////////

bool add_recipe(Recipe* recipe)
{
    if (recipe == NULL)
        return false;

    VNUM vnum = VNUM_FIELD(recipe);
    if (vnum == VNUM_NONE)
        return false;

    return ordered_table_set_vnum(&recipes, vnum, OBJ_VAL(recipe));
}

bool remove_recipe(VNUM vnum)
{
    return ordered_table_delete_vnum(&recipes, vnum);
}

////////////////////////////////////////////////////////////////////////////////
// Iteration
////////////////////////////////////////////////////////////////////////////////

int recipe_count(void)
{
    return ordered_table_count(&recipes);
}

RecipeIter make_recipe_iter(void)
{
    RecipeIter iter = { ordered_table_iter(&recipes) };
    return iter;
}

Recipe* recipe_iter_next(RecipeIter* iter)
{
    if (iter == NULL)
        return NULL;

    Value value;
    while (ordered_table_iter_next(&iter->iter, NULL, &value)) {
        if (IS_RECIPE(value))
            return AS_RECIPE(value);
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Input Management
////////////////////////////////////////////////////////////////////////////////

bool recipe_add_input(Recipe* recipe, VNUM mat_vnum, int16_t quantity)
{
    if (recipe == NULL)
        return false;

    if (recipe->input_count >= MAX_RECIPE_INPUTS)
        return false;

    recipe->inputs[recipe->input_count].mat_vnum = mat_vnum;
    recipe->inputs[recipe->input_count].quantity = quantity;
    recipe->input_count++;

    return true;
}

bool recipe_remove_input(Recipe* recipe, int index)
{
    if (recipe == NULL)
        return false;

    if (index < 0 || index >= recipe->input_count)
        return false;

    // Shift remaining inputs down
    for (int i = index; i < recipe->input_count - 1; i++) {
        recipe->inputs[i] = recipe->inputs[i + 1];
    }

    recipe->input_count--;
    return true;
}

void recipe_clear_inputs(Recipe* recipe)
{
    if (recipe == NULL)
        return;

    recipe->input_count = 0;
    memset(recipe->inputs, 0, sizeof(recipe->inputs));
}
