////////////////////////////////////////////////////////////////////////////////
// craft/recedit.c
// Recipe OLC Editor Implementation
////////////////////////////////////////////////////////////////////////////////

#include <merc.h>

#include "recedit.h"
#include "craft.h"
#include "recipe.h"

#include <olc/olc.h>

#include <comm.h>
#include <db.h>
#include <handler.h>
#include <interp.h>
#include <lookup.h>
#include <recycle.h>
#include <tables.h>

#include <entities/object.h>

#include <data/skill.h>

#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Local Constants
////////////////////////////////////////////////////////////////////////////////

#define MIN_RECEDIT_SECURITY    5

////////////////////////////////////////////////////////////////////////////////
// Dummy Object for Field Offsets
////////////////////////////////////////////////////////////////////////////////

Recipe xRecipe;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

////////////////////////////////////////////////////////////////////////////////
// OLC Command Table
////////////////////////////////////////////////////////////////////////////////

const OlcCmdEntry recipe_olc_comm_table[] = {
    { "name",       U(&xRecipe.header.name),    ed_line_lox_string, 0               },
    { "skill",      0,                          ed_olded,       U(recedit_skill)    },
    { "level",      U(&xRecipe.min_level),      ed_number_level,0                   },
    { "station",    0,                          ed_olded,       U(recedit_station)  },
    { "discovery",  0,                          ed_olded,       U(recedit_discovery)},
    { "input",      0,                          ed_olded,       U(recedit_input)    },
    { "rminput",    0,                          ed_olded,       U(recedit_rminput)  },
    { "output",     0,                          ed_olded,       U(recedit_output)   },
    { "create",     0,                          ed_olded,       U(recedit_create)   },
    { "show",       0,                          ed_olded,       U(recedit_show)     },
    { "list",       0,                          ed_olded,       U(recedit_list)     },
    { "commands",   0,                          ed_olded,       U(show_commands)    },
    { "?",          0,                          ed_olded,       U(show_help)        },
    { "version",    0,                          ed_olded,       U(show_version)     },
    { NULL,         0,                          NULL,           0                   }
};

////////////////////////////////////////////////////////////////////////////////
// Editor Interpreter
////////////////////////////////////////////////////////////////////////////////

void recedit(Mobile* ch, char* argument)
{
    if (ch->pcdata->security < MIN_RECEDIT_SECURITY) {
        send_to_char(COLOR_INFO "You do not have enough security to edit "
            "recipes." COLOR_EOL, ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (emptystring(argument)) {
        recedit_show(ch, argument);
        return;
    }

    // Search Table and Dispatch Command
    if (!process_olc_command(ch, argument, recipe_olc_comm_table))
        interpret(ch, argument);
}

////////////////////////////////////////////////////////////////////////////////
// Entry Point
////////////////////////////////////////////////////////////////////////////////

void do_recedit(Mobile* ch, char* argument)
{
    Recipe* pRecipe;
    char arg[MAX_INPUT_LENGTH];
    VNUM vnum;

    if (IS_NPC(ch))
        return;

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: recedit <vnum>" COLOR_EOL
                     COLOR_INFO "        recedit create <vnum>" COLOR_EOL
                     COLOR_INFO "        recedit list" COLOR_EOL, ch);
        return;
    }

    if (ch->pcdata->security < MIN_RECEDIT_SECURITY) {
        send_to_char(COLOR_INFO "You do not have enough security to edit "
            "recipes." COLOR_EOL, ch);
        return;
    }

    READ_ARG(arg);

    // Handle "list" command
    if (!str_cmp(arg, "list")) {
        recedit_list(ch, argument);
        return;
    }

    // Handle "create" command
    if (!str_cmp(arg, "create")) {
        if (recedit_create(ch, argument)) {
            // Recipe created, show it
            recedit_show(ch, "");
        }
        return;
    }

    // Try to parse as VNUM to edit existing recipe
    if (!is_number(arg)) {
        send_to_char(COLOR_INFO "Invalid VNUM." COLOR_EOL, ch);
        return;
    }

    vnum = (VNUM)atoi(arg);
    pRecipe = get_recipe(vnum);

    if (!pRecipe) {
        send_to_char(COLOR_INFO "That recipe does not exist. Use 'recedit create <vnum>'." COLOR_EOL, ch);
        return;
    }

    // Check builder permissions for the area
    if (pRecipe->area && !IS_BUILDER(ch, pRecipe->area)) {
        send_to_char(COLOR_INFO "You do not have access to that recipe's area." COLOR_EOL, ch);
        return;
    }

    set_editor(ch->desc, ED_RECIPE, U(pRecipe));
    recedit_show(ch, "");
}

////////////////////////////////////////////////////////////////////////////////
// recedit_show - Display recipe details
////////////////////////////////////////////////////////////////////////////////

RECEDIT(recedit_show)
{
    Recipe* pRecipe;
    char buf[MAX_STRING_LENGTH];

    EDIT_RECIPE(ch, pRecipe);

    olc_print_num(ch, "Vnum", VNUM_FIELD(pRecipe));
    olc_print_str(ch, "Name", NAME_STR(pRecipe));
    
    // Area
    if (pRecipe->area) {
        snprintf(buf, sizeof(buf), "[%d] %s", 
            VNUM_FIELD(pRecipe->area), NAME_STR(pRecipe->area));
        olc_print_str(ch, "Area", buf);
    } else {
        olc_print_str(ch, "Area", "(none)");
    }

    // Skill requirement
    if (pRecipe->required_skill >= 0 && pRecipe->required_skill < skill_count) {
        snprintf(buf, sizeof(buf), "%s (min %d%%)", 
            skill_table[pRecipe->required_skill].name,
            pRecipe->min_skill);
        olc_print_str(ch, "Skill", buf);
    } else {
        olc_print_str(ch, "Skill", "(none)");
    }

    // Level
    olc_print_num(ch, "Min Level", pRecipe->min_level);

    // Station
    if (pRecipe->station_type != WORK_NONE) {
        char station_buf[MAX_INPUT_LENGTH];
        workstation_type_flags_string(pRecipe->station_type, station_buf, sizeof(station_buf));
        if (pRecipe->station_vnum != VNUM_NONE && pRecipe->station_vnum != 0) {
            snprintf(buf, sizeof(buf), "%s (vnum %d)", station_buf, pRecipe->station_vnum);
        } else {
            snprintf(buf, sizeof(buf), "%s", station_buf);
        }
        olc_print_str(ch, "Station", buf);
    } else {
        olc_print_str(ch, "Station", "(none required)");
    }

    // Discovery
    olc_print_str(ch, "Discovery", discovery_type_name(pRecipe->discovery));

    // Ingredients
    send_to_char("\n\r" COLOR_TITLE "Inputs:" COLOR_EOL, ch);
    if (pRecipe->ingredient_count == 0) {
        send_to_char("  (none)\n\r", ch);
    } else {
        for (int i = 0; i < pRecipe->ingredient_count; i++) {
            ObjPrototype* proto = get_object_prototype(pRecipe->ingredients[i].mat_vnum);
            printf_to_char(ch, 
                COLOR_DECOR_1 "  [" COLOR_ALT_TEXT_1 "%d" COLOR_DECOR_1 "]" COLOR_CLEAR
                " %dx " COLOR_ALT_TEXT_2 "%s" COLOR_CLEAR " (%d)" COLOR_EOL,
                i + 1,
                pRecipe->ingredients[i].quantity,
                proto ? proto->short_descr : "(unknown)",
                pRecipe->ingredients[i].mat_vnum);
        }
    }

    // Output
    send_to_char("\n\r" COLOR_TITLE "Output:" COLOR_EOL, ch);
    if (pRecipe->product_vnum != VNUM_NONE && pRecipe->product_vnum != 0) {
        ObjPrototype* proto = get_object_prototype(pRecipe->product_vnum);
        printf_to_char(ch, 
            "  %dx " COLOR_ALT_TEXT_2 "%s" COLOR_CLEAR " (%d)" COLOR_EOL,
            pRecipe->product_quantity,
            proto ? proto->short_descr : "(unknown)",
            pRecipe->product_vnum);
    } else {
        send_to_char("  (not set)\n\r", ch);
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// recedit_list - List all recipes (optionally in current area)
////////////////////////////////////////////////////////////////////////////////

RECEDIT(recedit_list)
{
    AreaData* area = NULL;
    int count = 0;

    // If editing an area, default to that area
    if (get_editor(ch->desc) == ED_AREA) {
        EDIT_AREA(ch, area);
    }

    // Check for "all" argument
    if (!IS_NULLSTR(argument) && !str_prefix(argument, "all")) {
        area = NULL;
    }

    send_to_char(COLOR_TITLE "VNUM   Name                           Skill                Station" COLOR_EOL, ch);
    send_to_char(COLOR_DECOR_2 "====== ============================== ==================== ============" COLOR_EOL, ch);

    RecipeIter iter = make_recipe_iter();
    Recipe* recipe;
    while ((recipe = recipe_iter_next(&iter)) != NULL) {
        // Filter by area if specified
        if (area && recipe->area != area) {
            continue;
        }

        const char* skill_name = "(none)";
        if (recipe->required_skill >= 0 && recipe->required_skill < skill_count) {
            skill_name = skill_table[recipe->required_skill].name;
        }

        char station_buf[64];
        if (recipe->station_type != WORK_NONE) {
            workstation_type_flags_string(recipe->station_type, station_buf, sizeof(station_buf));
        } else {
            strcpy(station_buf, "(none)");
        }

        printf_to_char(ch, 
            COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR
            " %-30.30s %-20.20s %s" COLOR_EOL,
            VNUM_FIELD(recipe),
            NAME_STR(recipe),
            skill_name,
            station_buf);
        count++;
    }

    printf_to_char(ch, "\n\r" COLOR_INFO "%d recipe(s) found." COLOR_EOL, count);
    return false;
}

////////////////////////////////////////////////////////////////////////////////
// recedit_create - Create a new recipe
////////////////////////////////////////////////////////////////////////////////

RECEDIT(recedit_create)
{
    Recipe* pRecipe;
    AreaData* area;
    VNUM vnum;

    if (IS_NULLSTR(argument) || !is_number(argument)) {
        send_to_char(COLOR_INFO "Syntax: create <vnum>" COLOR_EOL, ch);
        return false;
    }

    vnum = (VNUM)atoi(argument);

    // Check if already exists
    if (get_recipe(vnum)) {
        send_to_char(COLOR_INFO "That recipe VNUM already exists." COLOR_EOL, ch);
        return false;
    }

    // Get area for this VNUM
    area = get_vnum_area(vnum);
    if (!area) {
        send_to_char(COLOR_INFO "That VNUM is not assigned to any area." COLOR_EOL, ch);
        return false;
    }

    if (!IS_BUILDER(ch, area)) {
        send_to_char(COLOR_INFO "You do not have access to that area." COLOR_EOL, ch);
        return false;
    }

    // Create the recipe
    pRecipe = new_recipe();
    pRecipe->header.vnum = vnum;
    pRecipe->area = area;
    
    // Initialize with reasonable defaults
    pRecipe->required_skill = -1;
    pRecipe->min_skill = 0;
    pRecipe->min_level = 1;
    pRecipe->station_type = WORK_NONE;
    pRecipe->station_vnum = VNUM_NONE;
    pRecipe->discovery = DISC_KNOWN;
    pRecipe->product_vnum = VNUM_NONE;
    pRecipe->product_quantity = 1;

    // Add to global recipes
    if (!add_recipe(pRecipe)) {
        send_to_char(COLOR_INFO "Failed to add recipe." COLOR_EOL, ch);
        free_recipe(pRecipe);
        return false;
    }

    // Enter editor
    set_editor(ch->desc, ED_RECIPE, U(pRecipe));
    printf_to_char(ch, COLOR_INFO "Recipe %d created." COLOR_EOL, vnum);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// recedit_skill - Set required skill and optional minimum percentage
////////////////////////////////////////////////////////////////////////////////

RECEDIT(recedit_skill)
{
    Recipe* pRecipe;
    char skill_arg[MAX_INPUT_LENGTH];
    char pct_arg[MAX_INPUT_LENGTH];
    SKNUM sn;
    int min_pct = 0;

    EDIT_RECIPE(ch, pRecipe);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: skill <skill name> [min%]" COLOR_EOL
                     COLOR_INFO "        skill none" COLOR_EOL, ch);
        return false;
    }

    READ_ARG(skill_arg);
    one_argument(argument, pct_arg);

    // Handle "none"
    if (!str_cmp(skill_arg, "none")) {
        pRecipe->required_skill = -1;
        pRecipe->min_skill = 0;
        send_to_char(COLOR_INFO "Skill requirement removed." COLOR_EOL, ch);
        return true;
    }

    // Look up skill
    sn = skill_lookup(skill_arg);
    if (sn == -1) {
        send_to_char(COLOR_INFO "That skill does not exist." COLOR_EOL, ch);
        return false;
    }

    // Parse optional percentage
    if (!IS_NULLSTR(pct_arg)) {
        min_pct = atoi(pct_arg);
        if (min_pct < 0) min_pct = 0;
        if (min_pct > 100) min_pct = 100;
    }

    pRecipe->required_skill = sn;
    pRecipe->min_skill = (int16_t)min_pct;

    printf_to_char(ch, COLOR_INFO "Skill set to: " COLOR_ALT_TEXT_1 "%s" 
                   COLOR_INFO " (min %d%%)" COLOR_EOL,
                   skill_table[sn].name, min_pct);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// recedit_station - Set workstation requirement
////////////////////////////////////////////////////////////////////////////////

RECEDIT(recedit_station)
{
    Recipe* pRecipe;
    char type_arg[MAX_INPUT_LENGTH];
    char vnum_arg[MAX_INPUT_LENGTH];

    EDIT_RECIPE(ch, pRecipe);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: station <type> [vnum]" COLOR_EOL
                     COLOR_INFO "        station none" COLOR_EOL
                     "\n\rAvailable types:\n\r", ch);
        WorkstationType types[] = { 
            WORK_FORGE, WORK_SMELTER, WORK_TANNERY, WORK_LOOM, WORK_ALCHEMY, 
            WORK_COOKING, WORK_ENCHANT, WORK_WOODWORK, WORK_JEWELER 
        };
        for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
            printf_to_char(ch, "  " COLOR_ALT_TEXT_1 "%s" COLOR_EOL, 
                workstation_type_name(types[i]));
        }
        return false;
    }

    READ_ARG(type_arg);
    one_argument(argument, vnum_arg);

    // Handle "none"
    if (!str_cmp(type_arg, "none")) {
        pRecipe->station_type = WORK_NONE;
        pRecipe->station_vnum = VNUM_NONE;
        send_to_char(COLOR_INFO "Station requirement removed." COLOR_EOL, ch);
        return true;
    }

    // Look up station type
    WorkstationType type = workstation_type_lookup(type_arg);
    if (type == WORK_NONE) {
        send_to_char(COLOR_INFO "Unknown workstation type." COLOR_EOL, ch);
        return false;
    }

    pRecipe->station_type = type;

    // Optional specific VNUM
    if (!IS_NULLSTR(vnum_arg)) {
        if (!is_number(vnum_arg)) {
            send_to_char(COLOR_INFO "VNUM must be a number." COLOR_EOL, ch);
            return false;
        }
        pRecipe->station_vnum = (VNUM)atoi(vnum_arg);
    } else {
        pRecipe->station_vnum = VNUM_NONE;
    }

    if (pRecipe->station_vnum != VNUM_NONE) {
        printf_to_char(ch, COLOR_INFO "Station set to: " COLOR_ALT_TEXT_1 "%s" 
                       COLOR_INFO " (VNUM %d)" COLOR_EOL,
                       workstation_type_name(type), pRecipe->station_vnum);
    } else {
        printf_to_char(ch, COLOR_INFO "Station set to: " COLOR_ALT_TEXT_1 "%s" COLOR_EOL,
                       workstation_type_name(type));
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// recedit_discovery - Set discovery type
////////////////////////////////////////////////////////////////////////////////

RECEDIT(recedit_discovery)
{
    Recipe* pRecipe;

    EDIT_RECIPE(ch, pRecipe);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: discovery <type>" COLOR_EOL
                     "\n\rAvailable types:\n\r", ch);
        for (int i = 0; i < DISCOVERY_TYPE_COUNT; i++) {
            printf_to_char(ch, "  " COLOR_ALT_TEXT_1 "%s" COLOR_EOL, 
                discovery_type_name((DiscoveryType)i));
        }
        return false;
    }

    DiscoveryType type = discovery_type_lookup(argument);
    if (type == DISCOVERY_TYPE_COUNT) {
        send_to_char(COLOR_INFO "Unknown discovery type." COLOR_EOL, ch);
        return false;
    }

    pRecipe->discovery = type;
    printf_to_char(ch, COLOR_INFO "Discovery type set to: " COLOR_ALT_TEXT_1 "%s" COLOR_EOL,
                   discovery_type_name(type));
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// recedit_input - Add an input ingredient
////////////////////////////////////////////////////////////////////////////////

RECEDIT(recedit_input)
{
    Recipe* pRecipe;
    char vnum_arg[MAX_INPUT_LENGTH];
    char qty_arg[MAX_INPUT_LENGTH];
    VNUM vnum;
    int qty = 1;

    EDIT_RECIPE(ch, pRecipe);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: input <vnum> [quantity]" COLOR_EOL, ch);
        return false;
    }

    READ_ARG(vnum_arg);
    one_argument(argument, qty_arg);

    if (!is_number(vnum_arg)) {
        send_to_char(COLOR_INFO "VNUM must be a number." COLOR_EOL, ch);
        return false;
    }

    vnum = (VNUM)atoi(vnum_arg);

    // Validate VNUM exists and is ITEM_MAT
    ObjPrototype* proto = get_object_prototype(vnum);
    if (!proto) {
        send_to_char(COLOR_INFO "That object VNUM does not exist." COLOR_EOL, ch);
        return false;
    }

    if (proto->item_type != ITEM_MAT) {
        send_to_char(COLOR_INFO "That object is not a crafting material (ITEM_MAT)." COLOR_EOL, ch);
        return false;
    }

    // Parse quantity
    if (!IS_NULLSTR(qty_arg)) {
        qty = atoi(qty_arg);
        if (qty < 1) qty = 1;
        if (qty > 100) qty = 100;
    }

    // Add ingredient
    if (!recipe_add_ingredient(pRecipe, vnum, (int16_t)qty)) {
        send_to_char(COLOR_INFO "Failed to add ingredient (recipe full or duplicate?)." COLOR_EOL, ch);
        return false;
    }

    printf_to_char(ch, COLOR_INFO "Added %dx " COLOR_ALT_TEXT_1 "%s" 
                   COLOR_INFO " (%d) to inputs." COLOR_EOL,
                   qty, proto->short_descr, vnum);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// recedit_rminput - Remove an input ingredient by index
////////////////////////////////////////////////////////////////////////////////

RECEDIT(recedit_rminput)
{
    Recipe* pRecipe;
    int index;

    EDIT_RECIPE(ch, pRecipe);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: rminput <index>" COLOR_EOL
                     COLOR_INFO "Use 'show' to see ingredient indices." COLOR_EOL, ch);
        return false;
    }

    if (!is_number(argument)) {
        send_to_char(COLOR_INFO "Index must be a number." COLOR_EOL, ch);
        return false;
    }

    index = atoi(argument) - 1;  // Convert to 0-based

    if (index < 0 || index >= pRecipe->ingredient_count) {
        send_to_char(COLOR_INFO "Invalid ingredient index." COLOR_EOL, ch);
        return false;
    }

    VNUM removed_vnum = pRecipe->ingredients[index].mat_vnum;
    ObjPrototype* proto = get_object_prototype(removed_vnum);

    if (!recipe_remove_ingredient(pRecipe, index)) {
        send_to_char(COLOR_INFO "Failed to remove ingredient." COLOR_EOL, ch);
        return false;
    }

    printf_to_char(ch, COLOR_INFO "Removed " COLOR_ALT_TEXT_1 "%s" 
                   COLOR_INFO " (%d) from inputs." COLOR_EOL,
                   proto ? proto->short_descr : "(unknown)", removed_vnum);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// recedit_output - Set output product
////////////////////////////////////////////////////////////////////////////////

RECEDIT(recedit_output)
{
    Recipe* pRecipe;
    char vnum_arg[MAX_INPUT_LENGTH];
    char qty_arg[MAX_INPUT_LENGTH];
    VNUM vnum;
    int qty = 1;

    EDIT_RECIPE(ch, pRecipe);

    if (IS_NULLSTR(argument)) {
        send_to_char(COLOR_INFO "Syntax: output <vnum> [quantity]" COLOR_EOL, ch);
        return false;
    }

    READ_ARG(vnum_arg);
    one_argument(argument, qty_arg);

    if (!is_number(vnum_arg)) {
        send_to_char(COLOR_INFO "VNUM must be a number." COLOR_EOL, ch);
        return false;
    }

    vnum = (VNUM)atoi(vnum_arg);

    // Validate VNUM exists
    ObjPrototype* proto = get_object_prototype(vnum);
    if (!proto) {
        send_to_char(COLOR_INFO "That object VNUM does not exist." COLOR_EOL, ch);
        return false;
    }

    // Parse quantity
    if (!IS_NULLSTR(qty_arg)) {
        qty = atoi(qty_arg);
        if (qty < 1) qty = 1;
        if (qty > 100) qty = 100;
    }

    pRecipe->product_vnum = vnum;
    pRecipe->product_quantity = (int16_t)qty;

    printf_to_char(ch, COLOR_INFO "Output set to %dx " COLOR_ALT_TEXT_1 "%s" 
                   COLOR_INFO " (%d)." COLOR_EOL,
                   qty, proto->short_descr, vnum);
    return true;
}
