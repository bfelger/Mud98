////////////////////////////////////////////////////////////////////////////////
// craft/recipe.h
// Recipe structure and management.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__CRAFT__RECIPE_H
#define MUD98__CRAFT__RECIPE_H

#include "craft.h"
#include "entities/entity.h"
#include "lox/ordered_table.h"

////////////////////////////////////////////////////////////////////////////////
// Recipe Ingredient
////////////////////////////////////////////////////////////////////////////////

// A single ingredient requirement for a recipe
typedef struct ingredient_t {
    VNUM mat_vnum;      // VNUM of required ITEM_MAT object
    int16_t quantity;   // Number required
} Ingredient;

////////////////////////////////////////////////////////////////////////////////
// Recipe Structure
////////////////////////////////////////////////////////////////////////////////

// Maximum number of ingredients per recipe
#define MAX_RECIPE_INGREDIENTS 8

// Forward declaration
typedef struct area_data_t AreaData;

typedef struct recipe_t {
    Entity header;              // Entity header (vnum, name, etc.)
    AreaData* area;             // Owning area for persistence

    SKNUM required_skill;       // Skill needed (gsn_leatherworking, etc.)
    int16_t min_skill;          // Minimum skill percentage (0-100)
    LEVEL min_level;            // Minimum character level

    WorkstationType station_type;   // Required workstation type flags
    VNUM station_vnum;              // Specific station VNUM (0 = any of type)

    DiscoveryType discovery;    // How recipe is learned

    Ingredient ingredients[MAX_RECIPE_INGREDIENTS]; // Required ingredients
    int16_t ingredient_count;                       // Number of ingredients

    VNUM product_vnum;          // Product object prototype VNUM
    int16_t product_quantity;   // Number produced
} Recipe;

////////////////////////////////////////////////////////////////////////////////
// Recipe Iterator
////////////////////////////////////////////////////////////////////////////////

typedef struct {
    OrderedTableIter iter;
} RecipeIter;

////////////////////////////////////////////////////////////////////////////////
// Recipe Management
////////////////////////////////////////////////////////////////////////////////

// Initialize/shutdown
void init_global_recipes(void);
void free_global_recipes(void);

// Lifecycle
Recipe* new_recipe(void);
void free_recipe(Recipe* recipe);

// Lookup
Recipe* get_recipe(VNUM vnum);
Recipe* get_recipe_by_name(const char* name);

// Registration
bool add_recipe(Recipe* recipe);
bool remove_recipe(VNUM vnum);

// Iteration
int recipe_count(void);
RecipeIter make_recipe_iter(void);
Recipe* recipe_iter_next(RecipeIter* iter);

// GC rooting
void mark_global_recipes(void);

// Ingredient management
bool recipe_add_ingredient(Recipe* recipe, VNUM mat_vnum, int16_t quantity);
bool recipe_remove_ingredient(Recipe* recipe, int index);
void recipe_clear_ingredients(Recipe* recipe);

#endif // MUD98__CRAFT__RECIPE_H
