////////////////////////////////////////////////////////////////////////////////
// persist/recipe/rom-olc/recipe_persist_rom_olc.c
// ROM-OLC text format persistence for recipes.
////////////////////////////////////////////////////////////////////////////////

#include "recipe_persist_rom_olc.h"

#include <craft/craft.h>
#include <craft/recipe.h>

#include <data/skill.h>

#include <db.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Format Definition
////////////////////////////////////////////////////////////////////////////////

const RecipePersistFormat RECIPE_PERSIST_ROM_OLC = {
    .name = "rom-olc",
    .load = recipe_persist_rom_olc_load,
    .save = recipe_persist_rom_olc_save
};

////////////////////////////////////////////////////////////////////////////////
// Load
////////////////////////////////////////////////////////////////////////////////

// Load #RECIPES section from area file
// Format:
//   recipe <vnum> '<name>'
//   skill <skillname> <min_pct>
//   level <min_level>
//   station <type_flags> <vnum>
//   discovery <type>
//   input <vnum> <quantity>
//   ...
//   output <vnum> <quantity>
//   end

void load_recipes(FILE* fp)
{
    if (current_area_data == NULL) {
        bug("load_recipes: no #AREA seen yet.", 0);
        return;
    }

    for (;;) {
        char* word = fread_word(fp);
        
        if (!str_cmp(word, "end") || !str_cmp(word, "#ENDRECIPES") || word[0] == '#') {
            // Check if we need to push back the section marker
            if (word[0] == '#' && str_cmp(word, "#ENDRECIPES") != 0) {
                // Push back the # for the next section
                fseek(fp, -((long)strlen(word) + 1), SEEK_CUR);
            }
            break;
        }
        
        if (!str_cmp(word, "recipe")) {
            Recipe* recipe = new_recipe();
            
            VNUM_FIELD(recipe) = fread_number(fp);
            const char* name = fread_string(fp);
            if (name && name[0] != '\0')
                recipe->header.name = AS_STRING(OBJ_VAL(copy_string(name, (int)strlen(name))));
            
            // Set owner area
            recipe->area = current_area_data;
            
            // Read recipe properties until 'end'
            for (;;) {
                char* prop = fread_word(fp);
                
                if (!str_cmp(prop, "end")) {
                    break;
                }
                else if (!str_cmp(prop, "skill")) {
                    char* skill_name = fread_word(fp);
                    recipe->required_skill = skill_lookup(skill_name);
                    recipe->min_skill = (int16_t)fread_number(fp);
                }
                else if (!str_cmp(prop, "level")) {
                    recipe->min_level = (LEVEL)fread_number(fp);
                }
                else if (!str_cmp(prop, "station")) {
                    char* flags_str = fread_word(fp);
                    // Parse flags - could be comma-separated or space-separated
                    recipe->station_type = WORK_NONE;
                    char* tok = strtok(flags_str, ",");
                    while (tok != NULL) {
                        // Trim whitespace
                        while (*tok == ' ') tok++;
                        WorkstationType flag = workstation_type_lookup(tok);
                        recipe->station_type |= flag;
                        tok = strtok(NULL, ",");
                    }
                    recipe->station_vnum = (VNUM)fread_number(fp);
                }
                else if (!str_cmp(prop, "discovery")) {
                    char* disc_name = fread_word(fp);
                    recipe->discovery = discovery_type_lookup(disc_name);
                }
                else if (!str_cmp(prop, "input")) {
                    VNUM mat_vnum = (VNUM)fread_number(fp);
                    int16_t quantity = (int16_t)fread_number(fp);
                    recipe_add_ingredient(recipe, mat_vnum, quantity);
                }
                else if (!str_cmp(prop, "output")) {
                    recipe->product_vnum = (VNUM)fread_number(fp);
                    recipe->product_quantity = (int16_t)fread_number(fp);
                }
                else {
                    bug("load_recipes: unknown property '%s'", prop);
                }
            }
            
            add_recipe(recipe);
        }
    }
}

PersistResult recipe_persist_rom_olc_load(const PersistReader* reader, const char* filename, Entity* owner)
{
    (void)reader;
    (void)filename;
    (void)owner;
    // Loading is done via load_recipes(FILE* fp) called from area loader
    return (PersistResult){ PERSIST_OK, NULL, 0 };
}

////////////////////////////////////////////////////////////////////////////////
// Save
////////////////////////////////////////////////////////////////////////////////

void save_recipes(FILE* fp, const AreaData* area)
{
    bool has_recipes = false;
    
    // Check if this area has any recipes
    RecipeIter iter = make_recipe_iter();
    Recipe* recipe;
    while ((recipe = recipe_iter_next(&iter)) != NULL) {
        if (recipe->area == area) {
            has_recipes = true;
            break;
        }
    }
    
    if (!has_recipes)
        return;
    
    fprintf(fp, "#RECIPES\n");
    
    iter = make_recipe_iter();
    while ((recipe = recipe_iter_next(&iter)) != NULL) {
        if (recipe->area != area)
            continue;
        
        fprintf(fp, "recipe %" PRVNUM " '%s'\n", 
                VNUM_FIELD(recipe),
                NAME_STR(recipe) ? NAME_STR(recipe) : "");
        
        // Skill requirement
        if (recipe->required_skill >= 0) {
            const char* skill_name = skill_table[recipe->required_skill].name;
            if (skill_name)
                fprintf(fp, "skill %s %d\n", skill_name, recipe->min_skill);
        }
        
        // Level requirement
        if (recipe->min_level > 1)
            fprintf(fp, "level %d\n", recipe->min_level);
        
        // Workstation
        if (recipe->station_type != WORK_NONE) {
            char flags_buf[256] = "";
            workstation_type_flags_string(recipe->station_type, flags_buf, sizeof(flags_buf));
            fprintf(fp, "station %s %" PRVNUM "\n", flags_buf, recipe->station_vnum);
        }
        
        // Discovery
        if (recipe->discovery != DISC_KNOWN) {
            const char* disc_name = discovery_type_name(recipe->discovery);
            if (disc_name)
                fprintf(fp, "discovery %s\n", disc_name);
        }
        
        // Inputs
        for (int i = 0; i < recipe->ingredient_count; i++) {
            fprintf(fp, "input %" PRVNUM " %d\n",
                    recipe->ingredients[i].mat_vnum,
                    recipe->ingredients[i].quantity);
        }
        
        // Output
        fprintf(fp, "output %" PRVNUM " %d\n",
                recipe->product_vnum,
                recipe->product_quantity);
        
        fprintf(fp, "end\n\n");
    }
    
    fprintf(fp, "#ENDRECIPES\n\n");
}

PersistResult recipe_persist_rom_olc_save(const PersistWriter* writer, const char* filename, Entity* owner)
{
    (void)writer;
    (void)filename;
    (void)owner;
    // Saving is done via save_recipes(FILE* fp, area) called from area saver
    return (PersistResult){ PERSIST_OK, NULL, 0 };
}
