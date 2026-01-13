////////////////////////////////////////////////////////////////////////////////
// loot_edit.c
// OLC editor for loot groups and tables
////////////////////////////////////////////////////////////////////////////////

#include "loot_edit.h"

#include "editor_stack.h"
#include "olc.h"
#include "string_edit.h"

#include <entities/entity.h>
#include <entities/obj_prototype.h>
#include <data/loot.h>

#include <comm.h>
#include <command.h>
#include <config.h>
#include <stringutils.h>

#include <stdio.h>
#include <string.h>

// Forward declarations
static bool lootedit_show(Mobile* ch, char* argument);
static bool lootedit_edit(Mobile* ch, char* argument);
static bool lootedit_list(Mobile* ch, char* argument);
static bool lootedit_group_list(Mobile* ch, char* argument);
static bool lootedit_table_list(Mobile* ch, char* argument);
static bool lootedit_create_group(Mobile* ch, char* argument);
static bool lootedit_create_table(Mobile* ch, char* argument);
static bool lootedit_delete_group(Mobile* ch, char* argument);
static bool lootedit_delete_table(Mobile* ch, char* argument);
static bool lootedit_add_item(Mobile* ch, char* argument);
static bool lootedit_add_cp(Mobile* ch, char* argument);
static bool lootedit_remove_entry(Mobile* ch, char* argument);
static bool lootedit_add_op(Mobile* ch, char* argument);
static bool lootedit_remove_op(Mobile* ch, char* argument);
static bool lootedit_parent(Mobile* ch, char* argument);
static bool lootedit_save(Mobile* ch, char* argument);
static bool lootedit_help(Mobile* ch, char* argument);
static void enter_group_editor(Mobile* ch, LootGroup* group);
static void enter_table_editor(Mobile* ch, LootTable* table);

typedef struct {
    const char* name;
    const char* usage;
    const char* desc;
} LooteditHelpEntry;

static const LooteditHelpEntry lootedit_help_table[] = {
    { "list",        "list",                               "Show all loot groups and tables for this entity." },
    { "show",        "show",                               "Display loot configuration summary." },
    { "edit",        "edit <group|table>",                 "Edit a loot group or table." },
    { "groups",      "groups",                             "List all loot groups (detailed)." },
    { "tables",      "tables",                             "List all loot tables (detailed)." },
    { "create",      "create group|table <args>",          "Create a new group or table (and edit it)." },
    { "delete",      "delete group|table <name>",          "Delete a group or table." },
    { "add",         "add item|cp|op <args>",              "Add an item, cp, or operation." },
    { "remove",      "remove entry|op <args>",             "Remove an entry or operation." },
    { "parent",      "parent <table> [parent|none]",       "Set or clear a table's parent." },
    { "save",        "save [json|olc]",                    "Save loot data (optionally force format)." },
    { "olist",       "olist",                              "List all objects in the edited loot area." },
    { "help",        "help [command]",                     "Show help for loot editor commands." },
    { "?",           "?",                                  "Alias for 'help'." },
    { "done",        "done",                               "Exit the loot editor." },
    { NULL,          NULL,                                 NULL }
};

// Helper to get the owner entity for the current editing context
// When editing loot from within aedit/medit/oedit, the parent editor's
// pEdit contains the owner entity.
static Entity* get_loot_owner(Mobile* ch)
{
    // If we have a parent editor, get the owner from there
    if (has_parent_editor(ch->desc)) {
        // Parent is at depth-2 (stack is 0-indexed, top is depth-1)
        int depth = editor_depth(ch->desc);
        EditorFrame* parent = editor_stack_at(&ch->desc->editor_stack, depth - 2);
        if (parent) {
            return (Entity*)parent->pEdit;
        }
    }
    // In main ED_LOOT mode (global loot edit), pEdit is the owner (NULL for global)
    if (get_editor(ch->desc) == ED_LOOT) {
        return (Entity*)get_pEdit(ch->desc);
    }
    // Fallback: no owner (global)
    return NULL;
}

static void ledit_olist(Mobile* ch, char* argument)
{
    Entity* owner = get_loot_owner(ch);
    if (!owner) {
        printf_to_char(ch, COLOR_INFO "OList: You must be editing loot for an"
            " entity to list its area's objects." COLOR_EOL);
        return;
    }
    // We have an owner. Determine if it's AreaData, ObjPrototype, or MobPrototype
    AreaData* area = NULL;
    if (owner->obj.type == OBJ_AREA_DATA) {
        area = (AreaData*)owner;
    } else if (owner->obj.type == OBJ_OBJ_PROTO) {
        ObjPrototype* obj_proto = (ObjPrototype*)owner;
        area = obj_proto->area;
    } else if (owner->obj.type == OBJ_MOB_PROTO) {
        MobPrototype* mob_proto = (MobPrototype*)owner;
        area = mob_proto->area;
    }  
    if (!area) {
        printf_to_char(ch, COLOR_INFO "OList: Unable to determine area for"
            " the current loot owner entity." COLOR_EOL);
        return;
    }
    // Build arg: "obj <filter> <area_name>"
    char olist_arg[MAX_INPUT_LENGTH];
    if (IS_NULLSTR(argument)) {
        snprintf(olist_arg, sizeof(olist_arg), "obj all %s", area->credits);
    }
    else {
        snprintf(olist_arg, sizeof(olist_arg), "obj %s %s", argument, area->credits);
    }
    do_olist(ch, olist_arg);
}

////////////////////////////////////////////////////////////////////////////////
// ledit - Main loot editor interpreter
// Called from run_olc_editor() when in ED_LOOT mode or sub-editor mode
////////////////////////////////////////////////////////////////////////////////

void ledit(Mobile* ch, char* argument)
{
    char command[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    char* rest;

    if (IS_NULLSTR(argument)) {
        lootedit_show(ch, "");
        return;
    }

    // Parse the first command word
    strcpy(buf, argument);
    char* word = buf;
    rest = buf;
    while (*rest && !isspace((unsigned char)*rest)) {
        rest++;
    }
    if (*rest) {
        *rest++ = '\0';
    }
    while (isspace((unsigned char)*rest)) {
        rest++;
    }

    strcpy(command, word);

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    if (!str_cmp(command, "show")) {
        lootedit_show(ch, rest);
        return;
    }

    if (!str_cmp(command, "edit")) {
        lootedit_edit(ch, rest);
        return;
    }

    if (!str_cmp(command, "list")) {
        lootedit_list(ch, rest);
        return;
    }

    if (!str_cmp(command, "groups")) {
        lootedit_group_list(ch, rest);
        return;
    }

    if (!str_cmp(command, "tables")) {
        lootedit_table_list(ch, rest);
        return;
    }

    if (!str_cmp(command, "olist")) {
        ledit_olist(ch, rest);
        return;
    }

    if (!str_cmp(command, "create")) {
        char subcommand[MSL];
        rest = one_argument(rest, subcommand);
        if (!str_cmp(subcommand, "group")) {
            lootedit_create_group(ch, rest);
        } else if (!str_cmp(subcommand, "table")) {
            lootedit_create_table(ch, rest);
        } else {
            printf_to_char(ch, COLOR_INFO "Usage: create group|table <args>" COLOR_EOL);
        }
        return;
    }

    if (!str_cmp(command, "delete")) {
        char subcommand[MSL];
        rest = one_argument(rest, subcommand);
        if (!str_cmp(subcommand, "group")) {
            lootedit_delete_group(ch, rest);
        } else if (!str_cmp(subcommand, "table")) {
            lootedit_delete_table(ch, rest);
        } else {
            printf_to_char(ch, COLOR_INFO "Usage: delete group|table <name>" COLOR_EOL);
        }
        return;
    }

    if (!str_cmp(command, "add")) {
        char subcommand[MSL];
        rest = one_argument(rest, subcommand);
        if (!str_cmp(subcommand, "item")) {
            lootedit_add_item(ch, rest);
        } else if (!str_cmp(subcommand, "cp")) {
            lootedit_add_cp(ch, rest);
        } else if (!str_cmp(subcommand, "op")) {
            lootedit_add_op(ch, rest);
        } else {
            printf_to_char(ch, COLOR_INFO "Usage: add item|cp|op <args>" COLOR_EOL);
        }
        return;
    }

    if (!str_cmp(command, "remove")) {
        char subcommand[MSL];
        rest = one_argument(rest, subcommand);
        if (!str_cmp(subcommand, "entry")) {
            lootedit_remove_entry(ch, rest);
        } else if (!str_cmp(subcommand, "op")) {
            lootedit_remove_op(ch, rest);
        } else {
            printf_to_char(ch, COLOR_INFO "Usage: remove entry|op <args>" COLOR_EOL);
        }
        return;
    }

    if (!str_cmp(command, "parent")) {
        lootedit_parent(ch, rest);
        return;
    }

    if (!str_cmp(command, "save")) {
        lootedit_save(ch, rest);
        return;
    }

    if (!str_cmp(command, "help") || !str_cmp(command, "?")) {
        lootedit_help(ch, rest);
        return;
    }

    printf_to_char(ch, COLOR_INFO "Unknown loot editor command." COLOR_EOL);
    printf_to_char(ch, COLOR_INFO "Use 'help' for available commands." COLOR_EOL);
}

////////////////////////////////////////////////////////////////////////////////
// olc_edit_loot - Sub-editor entry point
// Called from parent OLC editors (medit, aedit, oedit) to enter loot sub-editor
////////////////////////////////////////////////////////////////////////////////

bool olc_edit_loot(Mobile* ch, char* argument)
{
    Entity* entity;

    EDIT_ENTITY(ch, entity);

    if (!IS_NULLSTR(argument)) {
        printf_to_char(ch, COLOR_INFO "Syntax: loot" COLOR_EOL);
        printf_to_char(ch, COLOR_INFO "Enters the loot sub-editor for this entity." COLOR_EOL);
        return false;
    }

    // Enter sub-editor mode with entity as context
    push_sub_editor(ch->desc, ED_LOOT, (uintptr_t)entity);

    printf_to_char(ch, COLOR_INFO "Entering loot editor for entity %"PRVNUM"." COLOR_EOL, entity->vnum);
    printf_to_char(ch, COLOR_INFO "Type 'help' for loot editor commands, 'done'"
        " to exit.\n\r" COLOR_EOL);
    lootedit_show(ch, "");

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// do_ledit - Entry point for "edit loot" command (global loot editing)
////////////////////////////////////////////////////////////////////////////////

void do_ledit(Mobile* ch, char* argument)
{
    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    if (ch->pcdata->security < MIN_LEDIT_SECURITY) {
        send_to_char(COLOR_INFO "You do not have enough security to edit global"
            " loot database." COLOR_EOL, ch);
        return;
    }


    if (!global_loot_db) {
        send_to_char(COLOR_INFO "No global loot database loaded." COLOR_EOL, ch);
        return;
    }

    // Enter main ED_LOOT mode with NULL owner (global loot)
    set_editor(ch->desc, ED_LOOT, (uintptr_t)NULL);

    printf_to_char(ch, COLOR_INFO "Entering global loot editor." COLOR_EOL);
    printf_to_char(ch, COLOR_INFO "Type 'help' for loot editor commands, 'done'"
        " to exit.\n\r" COLOR_EOL);
    lootedit_show(ch, "");
}

////////////////////////////////////////////////////////////////////////////////
// Loot Editor Commands
////////////////////////////////////////////////////////////////////////////////

static bool lootedit_show(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    printf_to_char(ch, COLOR_DECOR_2 "=== " COLOR_TITLE "LOOT EDITOR "
        COLOR_DECOR_2 "===" COLOR_EOL);
    if (entity) {
        if (entity->obj.type == OBJ_MOB_PROTO) {
            olc_print_num_str(ch, "Mobile", entity->vnum, entity->name->chars);
        } else if (entity->obj.type == OBJ_AREA_DATA) {
            olc_print_num_str(ch, "Area", entity->vnum, entity->name->chars);
        } else if (entity->obj.type == OBJ_OBJ_PROTO) {
            olc_print_num_str(ch, "Object", entity->vnum, entity->name->chars);
        } else {
            olc_print_num_str(ch, "Entity", entity->vnum, entity->name->chars);
        }
    } else {
        olc_print_str_box(ch, "Editing", "Global Loot", NULL);
    }

    // Count owned groups and tables
    int group_count = 0;
    int table_count = 0;

    for (int i = 0; i < global_loot_db->group_count; i++) {
        if (global_loot_db->groups[i].owner == entity) {
            group_count++;
        }
    }

    for (int i = 0; i < global_loot_db->table_count; i++) {
        if (global_loot_db->tables[i].owner == entity) {
            table_count++;
        }
    }

    olc_print_num(ch, "Loot Groups", group_count);
    olc_print_num(ch, "Loot Tables", table_count);

    return true;
}

static bool lootedit_list(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    printf_to_char(ch, COLOR_DECOR_2 "=== " COLOR_TITLE "LOOT EDITOR "
        COLOR_DECOR_2 "===" COLOR_EOL);
    if (entity) {
        if (entity->obj.type == OBJ_MOB_PROTO) {
            olc_print_num_str(ch, "Mobile", entity->vnum, entity->name->chars);
        } else if (entity->obj.type == OBJ_AREA_DATA) {
            olc_print_num_str(ch, "Area", entity->vnum, entity->name->chars);
        } else if (entity->obj.type == OBJ_OBJ_PROTO) {
            olc_print_num_str(ch, "Object", entity->vnum, entity->name->chars);
        } else {
            olc_print_num_str(ch, "Entity", entity->vnum, entity->name->chars);
        }
    } else {
        olc_print_str_box(ch, "Editing", "Global Loot", NULL);
    }

    // List owned groups
    bool has_groups = false;
    for (int i = 0; i < global_loot_db->group_count; i++) {
        if (global_loot_db->groups[i].owner == entity) {
            if (!has_groups) {
                printf_to_char(ch, "Groups:\n\r");
                printf_to_char(ch, COLOR_TITLE "    Name                 Rolls  Entries\n\r");
                printf_to_char(ch, COLOR_DECOR_2 "    ==================== ====== =======" COLOR_EOL);
                has_groups = true;
            }
            LootGroup* g = &global_loot_db->groups[i];
            printf_to_char(ch, "    %-20s " COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 
                "%-2d" COLOR_DECOR_1 " ] [ " COLOR_ALT_TEXT_1 "%-3d" COLOR_DECOR_1 " ]" COLOR_EOL,
                g->name, g->rolls, g->entry_count);
        }
    }

    // List owned tables
    bool has_tables = false;
    for (int i = 0; i < global_loot_db->table_count; i++) {
        if (global_loot_db->tables[i].owner == entity) {
            if (!has_tables) {
                printf_to_char(ch, "Tables:\n\r");
                printf_to_char(ch, COLOR_TITLE "    Name                 Ops    Parent\n\r");
                printf_to_char(ch, COLOR_DECOR_2 "    ==================== ====== ====================" COLOR_EOL);
                has_tables = true;
            }
            LootTable* t = &global_loot_db->tables[i];
            const char* parent = (t->parent_name && t->parent_name[0]) ? t->parent_name : "";
            printf_to_char(ch, "    %-20s " COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 
                "%-2d" COLOR_DECOR_1 " ] "COLOR_ALT_TEXT_2 "%-20s" COLOR_EOL,
                t->name, t->op_count, parent);
        }
    }

    if (!has_groups && !has_tables) {
        printf_to_char(ch, COLOR_INFO "No loot groups or tables defined for this entity." COLOR_EOL);
    }

    return true;
}

static void show_loot_group_entries(Mobile* ch, LootGroup* g)
{
    if (g->entry_count == 0) {
        printf_to_char(ch, COLOR_ALT_TEXT_1 "    (no entries)\n\r" 
            COLOR_EOL);
        return;
    }
    else {
        printf_to_char(ch, COLOR_TITLE   "     Idx  Type  Min-Max  Wt     VNUM" COLOR_EOL);
        printf_to_char(ch, COLOR_DECOR_2 "    ===== ==== ========= === ===========" COLOR_EOL);
    }

    for (int j = 0; j < g->entry_count; j++) {
        LootEntry* e = &g->entries[j];
        if (e->type == LOOT_ITEM) {
            ObjPrototype* obj_proto = get_object_prototype(e->item_vnum);
            char* obj_name = obj_proto ? obj_proto->short_descr : "<unknown>";
            printf_to_char(ch, "    " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%3d" COLOR_DECOR_1 "]" COLOR_CLEAR " Item " COLOR_ALT_TEXT_1 "%4d" COLOR_DECOR_1 "-" COLOR_ALT_TEXT_1 "%-4d %3d " COLOR_DECOR_1 "[ " COLOR_ALT_TEXT_1 "%7"PRVNUM COLOR_DECOR_1 " ] " COLOR_ALT_TEXT_2 "%-30.30s" COLOR_EOL,
                j + 1, e->min_qty, e->max_qty, e->weight, e->item_vnum, obj_name);
        } else if (e->type == LOOT_CP) {
            printf_to_char(ch, "    " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%3d" COLOR_DECOR_1 "]" COLOR_CLEAR "  CP  " COLOR_ALT_TEXT_1 "%4d" COLOR_DECOR_1 "-" COLOR_ALT_TEXT_1 "%-4d %3d" COLOR_EOL,
                j + 1, e->min_qty, e->max_qty, e->weight);
        }
    }
    printf_to_char(ch, "\n\r");
}

static void show_loot_group(Mobile* ch, LootGroup* g)
{
    if (g->rolls == 1)
        printf_to_char(ch, "%s " COLOR_ALT_TEXT_1 "(1 roll) " COLOR_CLEAR ":\n\r", g->name);
    else
        printf_to_char(ch, "%s " COLOR_ALT_TEXT_1 "(%d rolls) " COLOR_CLEAR ":\n\r", g->name, g->rolls);
    show_loot_group_entries(ch, g);
}

static bool lootedit_group_list(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    printf_to_char(ch, COLOR_DECOR_2 "=== "  COLOR_TITLE "LOOT GROUPS " 
        COLOR_DECOR_2 "===" COLOR_EOL);

    bool found = false;
    for (int i = 0; i < global_loot_db->group_count; i++) {
        if (global_loot_db->groups[i].owner == entity) {
            found = true;
            LootGroup* g = &global_loot_db->groups[i];
            show_loot_group(ch, g);
        }
    }

    if (!found) {
        printf_to_char(ch, COLOR_INFO "No loot groups defined for this entity." COLOR_EOL);
    }

    return true;
}

static void lootedit_show_table_entries(Mobile* ch, LootTable* t)
{
    if (t->op_count == 0) {
        printf_to_char(ch, COLOR_ALT_TEXT_1 "    (no operations)\n\r" 
            COLOR_EOL);
        return;
    }

    ObjPrototype* obj_proto = NULL;
    for (int j = 0; j < t->op_count; j++) {
        LootOp* op = &t->ops[j];
        switch (op->type) {
        case LOOT_OP_USE_GROUP:
            printf_to_char(ch, PRETTY_IDX " use_group " COLOR_ALT_TEXT_1 "%s" COLOR_CLEAR " %d%%\n\r",
                j + 1, op->group_name, op->a);
            break;
        case LOOT_OP_ADD_ITEM:
            obj_proto = get_object_prototype(op->a);
            printf_to_char(ch, PRETTY_IDX " add_item " COLOR_ALT_TEXT_1 "%d" COLOR_CLEAR " %d%% %d-%d "  COLOR_ALT_TEXT_2 "(%.30s)" COLOR_EOL,
                j + 1, op->a, op->b, op->c, op->d, obj_proto ? obj_proto->header.name->chars : "unknown");
            break;
        case LOOT_OP_ADD_CP:
            printf_to_char(ch, PRETTY_IDX " add_cp %d%% %d-%d\n\r",
                j + 1, op->a, op->c, op->d);
            break;
        case LOOT_OP_MUL_CP:
            printf_to_char(ch, PRETTY_IDX " mul_cp %d%%\n\r",
                j + 1, op->a);
            break;
        case LOOT_OP_MUL_ALL_CHANCES:
            printf_to_char(ch, PRETTY_IDX " mul_all_chances %d%%\n\r",
                j + 1, op->a);
            break;
        case LOOT_OP_REMOVE_ITEM:
            obj_proto = get_object_prototype(op->a);
            printf_to_char(ch, PRETTY_IDX " remove_item " COLOR_ALT_TEXT_1 "%d " COLOR_ALT_TEXT_2 "(%-30.30s)" COLOR_EOL,
                j + 1, op->a, obj_proto ? obj_proto->header.name->chars : "unknown");
            break;
        case LOOT_OP_REMOVE_GROUP:
            printf_to_char(ch, PRETTY_IDX " remove_group " COLOR_ALT_TEXT_1 "%s" COLOR_EOL,
                j + 1, op->group_name);
            break;
        default:
            break;
        }
    }
    printf_to_char(ch, "\n\r");
}

static bool lootedit_table_list(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);

    if (entity == NULL && !global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    printf_to_char(ch, COLOR_DECOR_2 "=== " COLOR_TITLE "LOOT TABLES " 
        COLOR_DECOR_2 "===" COLOR_EOL);

    bool found = false;
    for (int i = 0; i < global_loot_db->table_count; i++) {
        if (global_loot_db->tables[i].owner == entity) {
            LootTable* t = &global_loot_db->tables[i];
            if (t->parent_name && t->parent_name[0])
                printf_to_char(ch, "%s " COLOR_ALT_TEXT_1 "(parent: %s)" COLOR_CLEAR ":\n\r", t->name, t->parent_name);
            else
                printf_to_char(ch, "%s:\n\r", t->name);
            lootedit_show_table_entries(ch, t);
            found = true;
        }
    }

    if (!found) {
        printf_to_char(ch, COLOR_INFO "No loot tables defined for this entity." COLOR_EOL);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// lootedit_save - Save loot data
////////////////////////////////////////////////////////////////////////////////

static bool lootedit_save(Mobile* ch, char* argument)
{
    Entity* owner = get_loot_owner(ch);
    
    // For global loot (owner == NULL), we can save to a specific format
    if (owner == NULL) {
        const char* filename = cfg_get_loot_file();
        char new_filename[MSL];
        
        if (!IS_NULLSTR(argument)) {
            // User specified format
            if (!str_cmp(argument, "json")) {
                // Replace extension with .json
                strncpy(new_filename, filename, sizeof(new_filename) - 1);
                new_filename[sizeof(new_filename) - 1] = '\0';
                char* ext = strrchr(new_filename, '.');
                if (ext) {
                    strcpy(ext, ".json");
                } else {
                    strcat(new_filename, ".json");
                }
                filename = new_filename;
            } else if (!str_cmp(argument, "olc")) {
                // Replace extension with .olc
                strncpy(new_filename, filename, sizeof(new_filename) - 1);
                new_filename[sizeof(new_filename) - 1] = '\0';
                char* ext = strrchr(new_filename, '.');
                if (ext) {
                    strcpy(ext, ".olc");
                } else {
                    strcat(new_filename, ".olc");
                }
                filename = new_filename;
            } else {
                printf_to_char(ch, COLOR_INFO "Usage: save [json|olc]" COLOR_EOL);
                return false;
            }
        }
        
        save_global_loot_db_as(filename);
        printf_to_char(ch, COLOR_INFO "Global loot saved to %s." COLOR_EOL, filename);
        return true;
    }
    
    // For area/mob loot, the data is saved with the area
    printf_to_char(ch, COLOR_INFO "Loot for this entity will be saved with its area." COLOR_EOL);
    printf_to_char(ch, COLOR_INFO "Use 'asave area' to save the area file." COLOR_EOL);
    return true;
}

static bool lootedit_help(Mobile* ch, char* argument)
{
    if (IS_NULLSTR(argument)) {
        // Show all commands
        printf_to_char(ch, COLOR_DECOR_2 "=== " COLOR_TITLE "LOOT EDITOR COMMANDS" COLOR_DECOR_2 " ===" COLOR_EOL);
        for (int i = 0; lootedit_help_table[i].name; i++) {
            printf_to_char(ch, COLOR_ALT_TEXT_1 "%-10s " COLOR_INFO "%s\n\r",
                lootedit_help_table[i].name, lootedit_help_table[i].desc);
        }
        printf_to_char(ch, COLOR_DECOR_2 "============================" COLOR_EOL);
    } else {
        // Show specific command
        for (int i = 0; lootedit_help_table[i].name; i++) {
            if (!str_cmp(argument, lootedit_help_table[i].name)) {
                printf_to_char(ch, "%s\n\r", lootedit_help_table[i].name);
                printf_to_char(ch, "Usage: %s\n\r", lootedit_help_table[i].usage);
                printf_to_char(ch, "%s\n\r", lootedit_help_table[i].desc);
                return true;
            }
        }
        printf_to_char(ch, COLOR_INFO "No such command." COLOR_EOL);
        return false;
    }

    return true;
}

static bool lootedit_create_group(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);
    char name[MSL];
    int rolls;

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    argument = one_argument(argument, name);
    if (IS_NULLSTR(name)) {
        printf_to_char(ch, COLOR_INFO "Usage: create group <name> <rolls>" COLOR_EOL);
        return false;
    }

    if (!is_number(argument)) {
        printf_to_char(ch, COLOR_INFO "Rolls must be a number." COLOR_EOL);
        return false;
    }

    rolls = atoi(argument);
    if (rolls < 0) {
        printf_to_char(ch, COLOR_INFO "Rolls must be non-negative." COLOR_EOL);
        return false;
    }

    LootGroup* group = loot_db_create_group(global_loot_db, name, rolls, entity);
    if (!group) {
        printf_to_char(ch, COLOR_INFO "Failed to create group '%s' (already exists?)." COLOR_EOL, name);
        return false;
    }

    // Mark area as changed if entity is area-bound
    if (entity != NULL && entity->obj.type == OBJ_AREA_DATA) {
        AreaData* area = (AreaData*)entity;
        SET_BIT(area->area_flags, AREA_CHANGED);
    }

    printf_to_char(ch, COLOR_INFO "Created loot group '%s' with %d rolls." COLOR_EOL, name, rolls);
    
    // Enter sub-editor for the new group
    enter_group_editor(ch, group);
    return true;
}

static bool lootedit_create_table(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);
    char name[MSL];
    char parent[MSL];

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    argument = one_argument(argument, name);
    argument = one_argument(argument, parent);

    if (IS_NULLSTR(name)) {
        printf_to_char(ch, COLOR_INFO "Usage: create table <name> [parent]" COLOR_EOL);
        return false;
    }

    LootTable* table = loot_db_create_table(global_loot_db, name, 
        IS_NULLSTR(parent) ? NULL : parent, entity);
    if (!table) {
        printf_to_char(ch, COLOR_INFO "Failed to create table '%s' (already exists?)." COLOR_EOL, name);
        return false;
    }

    // Mark area as changed if entity is area-bound
    if (entity != NULL && entity->obj.type == OBJ_AREA_DATA) {
        AreaData* area = (AreaData*)entity;
        SET_BIT(area->area_flags, AREA_CHANGED);
    }

    if (IS_NULLSTR(parent)) {
        printf_to_char(ch, COLOR_INFO "Created loot table '%s'." COLOR_EOL, name);
    } else {
        printf_to_char(ch, COLOR_INFO "Created loot table '%s' with parent '%s'." COLOR_EOL, name, parent);
    }

    // Enter sub-editor for the new table
    enter_table_editor(ch, table);
    return true;
}

static bool lootedit_delete_group(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);
    char name[MSL];

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    one_argument(argument, name);
    if (IS_NULLSTR(name)) {
        printf_to_char(ch, COLOR_INFO "Usage: delete group <name>" COLOR_EOL);
        return false;
    }

    if (!loot_db_delete_group(global_loot_db, name, entity)) {
        printf_to_char(ch, COLOR_INFO "Failed to delete group '%s' (not found?)." COLOR_EOL, name);
        return false;
    }

    // Mark area as changed if entity is area-bound
    if (entity != NULL && entity->obj.type == OBJ_AREA_DATA) {
        AreaData* area = (AreaData*)entity;
        SET_BIT(area->area_flags, AREA_CHANGED);
    }

    printf_to_char(ch, COLOR_INFO "Deleted loot group '%s'." COLOR_EOL, name);
    return true;
}

static bool lootedit_delete_table(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);
    char name[MSL];

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    one_argument(argument, name);
    if (IS_NULLSTR(name)) {
        printf_to_char(ch, COLOR_INFO "Usage: delete table <name>" COLOR_EOL);
        return false;
    }

    if (!loot_db_delete_table(global_loot_db, name, entity)) {
        printf_to_char(ch, COLOR_INFO "Failed to delete table '%s' (not found?)." COLOR_EOL, name);
        return false;
    }

    // Mark area as changed if entity is area-bound
    if (entity != NULL && entity->obj.type == OBJ_AREA_DATA) {
        AreaData* area = (AreaData*)entity;
        SET_BIT(area->area_flags, AREA_CHANGED);
    }

    printf_to_char(ch, COLOR_INFO "Deleted loot table '%s'." COLOR_EOL, name);
    return true;
}

static bool lootedit_add_item(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);
    char group_name[MSL];
    char vnum_str[MSL];
    char weight_str[MSL];
    char min_str[MSL];
    char max_str[MSL];

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    argument = one_argument(argument, group_name);
    argument = one_argument(argument, vnum_str);
    argument = one_argument(argument, weight_str);
    argument = one_argument(argument, min_str);
    argument = one_argument(argument, max_str);

    if (IS_NULLSTR(group_name) || IS_NULLSTR(vnum_str) || 
        IS_NULLSTR(weight_str) || IS_NULLSTR(min_str) || IS_NULLSTR(max_str)) {
        printf_to_char(ch, COLOR_INFO "Usage: add item <group> <vnum> <weight> <min> <max>" COLOR_EOL);
        return false;
    }

    LootGroup* group = loot_db_find_group(global_loot_db, group_name);
    if (!group || group->owner != entity) {
        printf_to_char(ch, COLOR_INFO "Group '%s' not found for this entity." COLOR_EOL, group_name);
        return false;
    }

    VNUM vnum = atol(vnum_str);
    int weight = atoi(weight_str);
    int min_qty = atoi(min_str);
    int max_qty = atoi(max_str);

    if (!loot_group_add_item(group, vnum, weight, min_qty, max_qty)) {
        printf_to_char(ch, COLOR_INFO "Failed to add item (invalid parameters?)." COLOR_EOL);
        return false;
    }

    // Mark area as changed if entity is area-bound
    if (entity != NULL && entity->obj.type == OBJ_AREA_DATA) {
        AreaData* area = (AreaData*)entity;
        SET_BIT(area->area_flags, AREA_CHANGED);
    }

    printf_to_char(ch, COLOR_INFO "Added item vnum %"PRVNUM" to group '%s'." COLOR_EOL, vnum, group_name);
    return true;
}

static bool lootedit_add_cp(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);
    char group_name[MSL];
    char weight_str[MSL];
    char min_str[MSL];
    char max_str[MSL];

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    argument = one_argument(argument, group_name);
    argument = one_argument(argument, weight_str);
    argument = one_argument(argument, min_str);
    argument = one_argument(argument, max_str);

    if (IS_NULLSTR(group_name) || IS_NULLSTR(weight_str) || 
        IS_NULLSTR(min_str) || IS_NULLSTR(max_str)) {
        printf_to_char(ch, COLOR_INFO "Usage: add cp <group> <weight> <min> <max>" COLOR_EOL);
        return false;
    }

    LootGroup* group = loot_db_find_group(global_loot_db, group_name);
    if (!group || group->owner != entity) {
        printf_to_char(ch, COLOR_INFO "Group '%s' not found for this entity." COLOR_EOL, group_name);
        return false;
    }

    int weight = atoi(weight_str);
    int min_qty = atoi(min_str);
    int max_qty = atoi(max_str);

    if (!loot_group_add_cp(group, weight, min_qty, max_qty)) {
        printf_to_char(ch, COLOR_INFO "Failed to add cp (invalid parameters?)." COLOR_EOL);
        return false;
    }

    // Mark area as changed if entity is area-bound
    if (entity != NULL && entity->obj.type == OBJ_AREA_DATA) {
        AreaData* area = (AreaData*)entity;
        SET_BIT(area->area_flags, AREA_CHANGED);
    }

    printf_to_char(ch, COLOR_INFO "Added cp entry to group '%s'." COLOR_EOL, group_name);
    return true;
}

static bool lootedit_remove_entry(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);
    char group_name[MSL];
    char index_str[MSL];

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    argument = one_argument(argument, group_name);
    argument = one_argument(argument, index_str);

    if (IS_NULLSTR(group_name) || IS_NULLSTR(index_str)) {
        printf_to_char(ch, COLOR_INFO "Usage: remove entry <group> <index>" COLOR_EOL);
        return false;
    }

    LootGroup* group = loot_db_find_group(global_loot_db, group_name);
    if (!group || group->owner != entity) {
        printf_to_char(ch, COLOR_INFO "Group '%s' not found for this entity." COLOR_EOL, group_name);
        return false;
    }

    int index = atoi(index_str) - 1; // Convert to 0-based
    if (!loot_group_remove_entry(group, index)) {
        printf_to_char(ch, COLOR_INFO "Failed to remove entry (invalid index?)." COLOR_EOL);
        return false;
    }

    // Mark area as changed if entity is area-bound
    if (entity != NULL && entity->obj.type == OBJ_AREA_DATA) {
        AreaData* area = (AreaData*)entity;
        SET_BIT(area->area_flags, AREA_CHANGED);
    }

    printf_to_char(ch, COLOR_INFO "Removed entry %d from group '%s'." COLOR_EOL, index, group_name);
    return true;
}

static bool lootedit_add_op(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);
    char table_name[MSL];
    char op_type[MSL];

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    argument = one_argument(argument, table_name);
    argument = one_argument(argument, op_type);

    if (IS_NULLSTR(table_name) || IS_NULLSTR(op_type)) {
        printf_to_char(ch, COLOR_INFO "Usage: add op <table> <type> [params...]" COLOR_EOL);
        printf_to_char(ch, COLOR_INFO "Types: use_group, add_item, add_cp, mul_cp, mul_all, remove_item, remove_group" COLOR_EOL);
        return false;
    }

    LootTable* table = loot_db_find_table(global_loot_db, table_name);
    if (!table || table->owner != entity) {
        printf_to_char(ch, COLOR_INFO "found for this entity." COLOR_EOL, table_name);
        return false;
    }

    LootOp op;
    op.type = 0; // Initialize to invalid
    
    char* end;

    if (!str_cmp(op_type, "use_group")) {
        char group_name[MSL];
        one_argument(argument, group_name);
        if (IS_NULLSTR(group_name)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op <table> use_group <group_name>" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_USE_GROUP;
        op.group_name = str_dup(group_name);
    }
    else if (!str_cmp(op_type, "add_item")) {
        char vnum_str[MSL], min_str[MSL], max_str[MSL];
        char chance_str[MSL];
        argument = one_argument(argument, vnum_str);
        argument = one_argument(argument, min_str);
        argument = one_argument(argument, max_str);
        one_argument(argument, chance_str);
        if (IS_NULLSTR(vnum_str) || IS_NULLSTR(min_str) || IS_NULLSTR(max_str) || IS_NULLSTR(chance_str)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op <table> add_item <vnum> <min> <max> <chance%%>" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_ADD_ITEM;
        op.a = atol(vnum_str);
        op.b = atoi(chance_str);
        op.c = atoi(min_str);
        op.d = atoi(max_str);
    }
    else if (!str_cmp(op_type, "add_cp")) {
        char min_str[MSL], max_str[MSL], chance_str[MSL];
        argument = one_argument(argument, min_str);
        argument = one_argument(argument, max_str);
        one_argument(argument, chance_str);
        if (IS_NULLSTR(min_str) || IS_NULLSTR(max_str) || IS_NULLSTR(chance_str)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op <table> add_cp <min> <max> <chance%%>" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_ADD_CP;
        op.a = atoi(chance_str);
        op.c = atoi(min_str);
        op.d = atoi(max_str);
    }
    else if (!str_cmp(op_type, "mul_cp")) {
        char factor_str[MSL];
        one_argument(argument, factor_str);
        if (IS_NULLSTR(factor_str)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op <table> mul_cp <factor>" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_MUL_CP;
        op.a = strtol(factor_str, &end, 10);
    }
    else if (!str_cmp(op_type, "mul_all")) {
        char factor_str[MSL];
        one_argument(argument, factor_str);
        if (IS_NULLSTR(factor_str)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op <table> mul_all <factor>" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_MUL_ALL_CHANCES;
        op.a = strtol(factor_str, &end, 10);
    }
    else if (!str_cmp(op_type, "remove_item")) {
        char vnum_str[MSL];
        one_argument(argument, vnum_str);
        if (IS_NULLSTR(vnum_str)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op <table> remove_item <vnum>" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_REMOVE_ITEM;
        op.a = strtol(vnum_str, &end, 10);
    }
    else if (!str_cmp(op_type, "remove_group")) {
        char group_name[MSL];
        one_argument(argument, group_name);
        if (IS_NULLSTR(group_name)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op <table> remove_group <group_name>" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_REMOVE_GROUP;
        op.group_name = str_dup(group_name);
    }
    else {
        printf_to_char(ch, COLOR_INFO "Unknown operation type '%s'." COLOR_EOL, op_type);
        return false;
    }

    if (!loot_table_add_op(table, op)) {
        printf_to_char(ch, COLOR_INFO "Failed to add operation." COLOR_EOL);
        return false;
    }

    // Resolve inheritance
    resolve_all_loot_tables(global_loot_db);

    // Mark area as changed if entity is area-bound
    if (entity != NULL && entity->obj.type == OBJ_AREA_DATA) {
        AreaData* area = (AreaData*)entity;
        SET_BIT(area->area_flags, AREA_CHANGED);
    }

    printf_to_char(ch, COLOR_INFO "Added %s operation to table '%s'." COLOR_EOL, op_type, table_name);
    return true;
}

static bool lootedit_remove_op(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);
    char table_name[MSL];
    char index_str[MSL];

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    argument = one_argument(argument, table_name);
    argument = one_argument(argument, index_str);

    if (IS_NULLSTR(table_name) || IS_NULLSTR(index_str)) {
        printf_to_char(ch, COLOR_INFO "Usage: remove op <table> <index>" COLOR_EOL);
        return false;
    }

    LootTable* table = loot_db_find_table(global_loot_db, table_name);
    if (!table || table->owner != entity) {
        printf_to_char(ch, COLOR_INFO "Table '%s' not found for this entity." COLOR_EOL, table_name);
        return false;
    }

    int index = atoi(index_str) - 1; // Convert to 0-based
    if (!loot_table_remove_op(table, index)) {
        printf_to_char(ch, COLOR_INFO "Failed to remove operation (invalid index?)." COLOR_EOL);
        return false;
    }

    // Resolve inheritance
    resolve_all_loot_tables(global_loot_db);

    // Mark area as changed if entity is area-bound
    if (entity != NULL && entity->obj.type == OBJ_AREA_DATA) {
        AreaData* area = (AreaData*)entity;
        SET_BIT(area->area_flags, AREA_CHANGED);
    }

    printf_to_char(ch, COLOR_INFO "Removed operation %d from table '%s'." COLOR_EOL, index, table_name);
    return true;
}

static bool lootedit_parent(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);
    char table_name[MSL];
    char parent_name[MSL];

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    argument = one_argument(argument, table_name);
    argument = one_argument(argument, parent_name);

    if (IS_NULLSTR(table_name)) {
        printf_to_char(ch, COLOR_INFO "Usage: parent <table> [parent_name|none]" COLOR_EOL);
        return false;
    }

    LootTable* table = loot_db_find_table(global_loot_db, table_name);
    if (!table || table->owner != entity) {
        printf_to_char(ch, COLOR_INFO "Table '%s' not found for this entity." COLOR_EOL, table_name);
        return false;
    }

    const char* parent = NULL;
    if (!IS_NULLSTR(parent_name) && str_cmp(parent_name, "none")) {
        parent = parent_name;
    }

    if (!loot_table_set_parent(table, parent)) {
        printf_to_char(ch, COLOR_INFO "Failed to set parent." COLOR_EOL);
        return false;
    }

    // Resolve inheritance
    resolve_all_loot_tables(global_loot_db);

    // Mark area as changed if entity is area-bound
    if (entity != NULL && entity->obj.type == OBJ_AREA_DATA) {
        AreaData* area = (AreaData*)entity;
        SET_BIT(area->area_flags, AREA_CHANGED);
    }

    if (parent) {
        printf_to_char(ch, COLOR_INFO "Set parent of table '%s' to '%s'." COLOR_EOL, table_name, parent);
    } else {
        printf_to_char(ch, COLOR_INFO "Cleared parent of table '%s'." COLOR_EOL, table_name);
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Sub-editor helpers
////////////////////////////////////////////////////////////////////////////////

//static const char* op_type_name(LootOpType type)
//{
//    switch (type) {
//        case LOOT_OP_USE_GROUP: return "use_group";
//        case LOOT_OP_ADD_ITEM: return "add_item";
//        case LOOT_OP_ADD_CP: return "add_cp";
//        case LOOT_OP_MUL_CP: return "mul_cp";
//        case LOOT_OP_MUL_ALL_CHANCES: return "mul_all";
//        case LOOT_OP_REMOVE_ITEM: return "remove_item";
//        case LOOT_OP_REMOVE_GROUP: return "remove_group";
//        default: return "unknown";
//    }
//}

static void mark_area_changed(Mobile* ch)
{
    Entity* entity = get_loot_owner(ch);
    if (entity != NULL && entity->obj.type == OBJ_AREA_DATA) {
        AreaData* area = (AreaData*)entity;
        SET_BIT(area->area_flags, AREA_CHANGED);
    }
}

static void show_loot_group_long(Mobile* ch, LootGroup* group)
{
    olc_print_str_box(ch, "Loot Group", group->name, NULL);
    if (group->owner) {
        const char* owner_type = (group->owner->obj.type == OBJ_AREA_DATA) ? "Area" : 
            (group->owner->obj.type == OBJ_MOB_PROTO) ? "Mobile" : "Entity";
        olc_print_num_str(ch, owner_type, group->owner->vnum, group->owner->name->chars);
    } else {
        olc_print_str_box(ch, "Owner", "None (global)", NULL);
    }
    olc_print_num(ch, "Rolls", group->rolls);
    
    show_loot_group_entries(ch, group);
}

static void show_loot_table(Mobile* ch, LootTable* table)
{
    olc_print_str_box(ch, "Loot Table", table->name, NULL);
    if (table->owner) {
        const char* owner_type = (table->owner->obj.type == OBJ_AREA_DATA) ? "Area" : 
            (table->owner->obj.type == OBJ_MOB_PROTO) ? "Mobile" : "Entity";
        olc_print_num_str(ch, owner_type, table->owner->vnum, table->owner->name->chars);
    } else {
        olc_print_str_box(ch, "Owner", "None (global)", NULL);
    }
    olc_print_str_box(ch, "Parent Table", 
        IS_NULLSTR(table->parent_name) ? "None" : table->parent_name, NULL);
    
    lootedit_show_table_entries(ch, table);
}

static void enter_group_editor(Mobile* ch, LootGroup* group)
{
    push_editor(ch->desc, ED_LOOT_GROUP, (uintptr_t)group);
    printf_to_char(ch, COLOR_INFO "Entering loot group editor. Type 'done' to exit, '?' for help." COLOR_EOL);
    show_loot_group_long(ch, group);
}

static void enter_table_editor(Mobile* ch, LootTable* table)
{
    push_editor(ch->desc, ED_LOOT_TABLE, (uintptr_t)table);
    printf_to_char(ch, COLOR_INFO "Entering loot table editor. Type 'done' to exit, '?' for help." COLOR_EOL);
    show_loot_table(ch, table);
}

static bool lootedit_edit(Mobile* ch, char* argument)
{
    Entity* entity = get_loot_owner(ch);
    char name[MSL];

    if (!global_loot_db) {
        printf_to_char(ch, COLOR_INFO "No global loot database loaded." COLOR_EOL);
        return false;
    }

    one_argument(argument, name);
    if (IS_NULLSTR(name)) {
        printf_to_char(ch, COLOR_INFO "Usage: edit <group_name|table_name>" COLOR_EOL);
        return false;
    }

    // Try to find as a group first
    LootGroup* group = loot_db_find_group(global_loot_db, name);
    if (group && group->owner == entity) {
        enter_group_editor(ch, group);
        return true;
    }

    // Try to find as a table
    LootTable* table = loot_db_find_table(global_loot_db, name);
    if (table && table->owner == entity) {
        enter_table_editor(ch, table);
        return true;
    }

    printf_to_char(ch, COLOR_INFO "No group or table named '%s' found for this entity." COLOR_EOL, name);
    return false;
}

////////////////////////////////////////////////////////////////////////////////
// Loot Group Sub-editor
////////////////////////////////////////////////////////////////////////////////

// Forward declarations for group sub-editor
static bool groupedit_add(Mobile* ch, LootGroup* group, char* argument);
static bool groupedit_remove(Mobile* ch, LootGroup* group, char* argument);
static bool groupedit_rolls(Mobile* ch, LootGroup* group, char* argument);

void loot_group_edit(Mobile* ch, char* argument)
{
    LootGroup* group;
    char command[MSL];

    EDIT_LOOT_GROUP(ch, group);

    argument = one_argument(argument, command);

    if (!str_cmp(command, "?")) {
        send_to_char(COLOR_INFO "Loot Group Editor Commands:\n\r", ch);
        send_to_char("  show                                 - Show group details\n\r", ch);
        send_to_char("  add item <vnum> <weight> <min> <max> - Add an item entry\n\r", ch);
        send_to_char("  add cp <weight> <min> <max>          - Add a choice point\n\r", ch);
        send_to_char("  remove <index>                       - Remove entry by index\n\r", ch);
        send_to_char("  rolls <number>                       - Set number of rolls\n\r", ch);
        send_to_char("  olist                               - List available items\n\r", ch);
        send_to_char("  done                                 - Exit group editor\n\r", ch);
        return;
    }

    if (!str_cmp(command, "show")) {
        show_loot_group_long(ch, group);
        return;
    }

    if (!str_cmp(command, "add")) {
        if (groupedit_add(ch, group, argument)) {
            mark_area_changed(ch);
        }
        return;
    }

    if (!str_cmp(command, "remove")) {
        if (groupedit_remove(ch, group, argument)) {
            mark_area_changed(ch);
        }
        return;
    }

    if (!str_cmp(command, "rolls")) {
        if (groupedit_rolls(ch, group, argument)) {
            mark_area_changed(ch);
        }
        return;
    }

    if (!str_cmp(command, "olist")) {
        ledit_olist(ch, argument);
        return;
    }

    if (!str_cmp(command, "done")) {
        pop_editor(ch->desc);
        printf_to_char(ch, COLOR_INFO "Exiting loot group editor." COLOR_EOL);
        return;
    }

    printf_to_char(ch, COLOR_INFO "Unknown command '%s'. Type '?' for help." COLOR_EOL, command);
}

static bool groupedit_add(Mobile* ch, LootGroup* group, char* argument)
{
    char type[MSL];
    char arg1[MSL];
    char arg2[MSL];
    char arg3[MSL];
    char arg4[MSL];

    argument = one_argument(argument, type);
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    argument = one_argument(argument, arg3);
    argument = one_argument(argument, arg4);

    if (!str_cmp(type, "item")) {
        if (IS_NULLSTR(arg1) || IS_NULLSTR(arg2) || IS_NULLSTR(arg3) || IS_NULLSTR(arg4)) {
            printf_to_char(ch, COLOR_INFO "Usage: add item <vnum> <weight> <min_qty> <max_qty>" COLOR_EOL);
            return false;
        }

        VNUM vnum = (VNUM)atol(arg1);
        ObjPrototype* obj = get_object_prototype(vnum);
        if (!obj) {
            printf_to_char(ch, COLOR_INFO "Object vnum %"PRVNUM" not found." COLOR_EOL, vnum);
            return false;
        }

        int weight = atoi(arg2);
        int min_qty = atoi(arg3);
        int max_qty = atoi(arg4);

        if (weight < 1) weight = 1;
        if (min_qty < 1) min_qty = 1;
        if (max_qty < min_qty) max_qty = min_qty;

        if (!loot_group_add_item(group, vnum, weight, min_qty, max_qty)) {
            printf_to_char(ch, COLOR_INFO "Failed to add item." COLOR_EOL);
            return false;
        }

        printf_to_char(ch, COLOR_INFO "Added item [%"PRVNUM"] %s (w=%d q=%d-%d)." COLOR_EOL,
            vnum, obj->short_descr, weight, min_qty, max_qty);
        return true;
    }

    if (!str_cmp(type, "cp")) {
        if (IS_NULLSTR(arg1) || IS_NULLSTR(arg2) || IS_NULLSTR(arg3)) {
            printf_to_char(ch, COLOR_INFO "Usage: add cp <weight> <min> <max>" COLOR_EOL);
            return false;
        }

        int weight = atoi(arg1);
        int min_qty = atoi(arg2);
        int max_qty = atoi(arg3);

        if (weight < 1) weight = 1;
        if (min_qty < 0) min_qty = 0;
        if (max_qty < min_qty) max_qty = min_qty;

        if (!loot_group_add_cp(group, weight, min_qty, max_qty)) {
            printf_to_char(ch, COLOR_INFO "Failed to add choice point." COLOR_EOL);
            return false;
        }

        printf_to_char(ch, COLOR_INFO "Added cp (w=%d q=%d-%d)." COLOR_EOL, weight, min_qty, max_qty);
        return true;
    }

    printf_to_char(ch, COLOR_INFO "Usage: add <item|cp> ..." COLOR_EOL);
    return false;
}

static bool groupedit_remove(Mobile* ch, LootGroup* group, char* argument)
{
    char arg[MSL];
    one_argument(argument, arg);

    if (IS_NULLSTR(arg)) {
        printf_to_char(ch, COLOR_INFO "Usage: remove <index>" COLOR_EOL);
        return false;
    }

    int index = atoi(arg) - 1;  // Convert to 0-based
    if (index < 0 || index >= group->entry_count) {
        printf_to_char(ch, COLOR_INFO "Invalid index. Use 1-%d." COLOR_EOL, group->entry_count);
        return false;
    }

    if (!loot_group_remove_entry(group, index)) {
        printf_to_char(ch, COLOR_INFO "Failed to remove entry." COLOR_EOL);
        return false;
    }

    printf_to_char(ch, COLOR_INFO "Removed entry %d." COLOR_EOL, index + 1);
    return true;
}

static bool groupedit_rolls(Mobile* ch, LootGroup* group, char* argument)
{
    char arg[MSL];
    one_argument(argument, arg);

    if (IS_NULLSTR(arg)) {
        printf_to_char(ch, COLOR_INFO "Usage: rolls <number>" COLOR_EOL);
        return false;
    }

    int rolls = atoi(arg);
    if (rolls < 1) {
        printf_to_char(ch, COLOR_INFO "Rolls must be at least 1." COLOR_EOL);
        return false;
    }

    group->rolls = rolls;
    printf_to_char(ch, COLOR_INFO "Set rolls to %d." COLOR_EOL, rolls);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Loot Table Sub-editor
////////////////////////////////////////////////////////////////////////////////

// Forward declarations for table sub-editor
static bool tableedit_add(Mobile* ch, LootTable* table, char* argument);
static bool tableedit_remove(Mobile* ch, LootTable* table, char* argument);
static bool tableedit_parent(Mobile* ch, LootTable* table, char* argument);

void loot_table_edit(Mobile* ch, char* argument)
{
    LootTable* table;
    char command[MSL];

    EDIT_LOOT_TABLE(ch, table);

    argument = one_argument(argument, command);

    if (!str_cmp(command, "?")) {
        send_to_char(COLOR_INFO "Loot Table Editor Commands:\n\r", ch);
        send_to_char("  show                                          - Show table details\n\r", ch);
        send_to_char("  add op use_group <group> [chance]             - Use a loot group\n\r", ch);
        send_to_char("  add op add_item <vnum> [chance] [min] [max]   - Add item directly\n\r", ch);
        send_to_char("  add op add_cp [chance] [min] [max]            - Add cp directly\n\r", ch);
        send_to_char("  add op mul_cp <factor>                        - Multiply cp\n\r", ch);
        send_to_char("  add op mul_all <factor>                       - Multiply all chances\n\r", ch);
        send_to_char("  add op remove_item <vnum>                     - Remove an item\n\r", ch);
        send_to_char("  add op remove_group <group>                   - Remove a group\n\r", ch);
        send_to_char("  remove <index>                                - Remove operation by index\n\r", ch);
        send_to_char("  parent [name|none]                            - Set or clear parent\n\r", ch);
        send_to_char("  done                                          - Exit table editor\n\r", ch);
        return;
    }

    if (!str_cmp(command, "show")) {
        show_loot_table(ch, table);
        return;
    }

    if (!str_cmp(command, "add")) {
        if (tableedit_add(ch, table, argument)) {
            mark_area_changed(ch);
        }
        return;
    }

    if (!str_cmp(command, "remove")) {
        if (tableedit_remove(ch, table, argument)) {
            mark_area_changed(ch);
        }
        return;
    }

    if (!str_cmp(command, "parent")) {
        if (tableedit_parent(ch, table, argument)) {
            mark_area_changed(ch);
        }
        return;
    }

    if (!str_cmp(command, "done")) {
        pop_editor(ch->desc);
        printf_to_char(ch, COLOR_INFO "Exiting loot table editor." COLOR_EOL);
        return;
    }

    printf_to_char(ch, COLOR_INFO "Unknown command '%s'. Type '?' for help." COLOR_EOL, command);
}

static bool tableedit_add(Mobile* ch, LootTable* table, char* argument)
{
    char type_arg[MSL];
    char op_type_str[MSL];
    LootOp op = {0};
    char* end;

    argument = one_argument(argument, type_arg);
    argument = one_argument(argument, op_type_str);

    if (str_cmp(type_arg, "op")) {
        printf_to_char(ch, COLOR_INFO "Usage: add op <type> ..." COLOR_EOL);
        return false;
    }

    if (IS_NULLSTR(op_type_str)) {
        printf_to_char(ch, COLOR_INFO "Usage: add op <type> ...\n\r" COLOR_EOL);
        printf_to_char(ch, COLOR_INFO "Types: use_group, add_item, add_cp, mul_cp, mul_all, remove_item, remove_group" COLOR_EOL);
        return false;
    }

    // Parse operation based on type
    if (!str_cmp(op_type_str, "use_group")) {
        char group_name[MSL];
        char chance_str[MSL];
        argument = one_argument(argument, group_name);
        one_argument(argument, chance_str);
        if (IS_NULLSTR(group_name)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op use_group <group_name> [chance]" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_USE_GROUP;
        op.group_name = str_dup(group_name);
        op.a = IS_NULLSTR(chance_str) ? 100 : atoi(chance_str);
    }
    else if (!str_cmp(op_type_str, "add_item")) {
        char vnum_str[MSL];
        char chance_str[MSL];
        char min_str[MSL];
        char max_str[MSL];
        argument = one_argument(argument, vnum_str);
        argument = one_argument(argument, chance_str);
        argument = one_argument(argument, min_str);
        one_argument(argument, max_str);
        if (IS_NULLSTR(vnum_str)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op add_item <vnum> [chance] [min] [max]" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_ADD_ITEM;
        op.a = strtol(vnum_str, &end, 10);
        op.b = IS_NULLSTR(chance_str) ? 100 : atoi(chance_str);
        op.c = IS_NULLSTR(min_str) ? 1 : atoi(min_str);
        op.d = IS_NULLSTR(max_str) ? op.c : atoi(max_str);
    }
    else if (!str_cmp(op_type_str, "add_cp")) {
        char chance_str[MSL];
        char min_str[MSL];
        char max_str[MSL];
        argument = one_argument(argument, chance_str);
        argument = one_argument(argument, min_str);
        one_argument(argument, max_str);
        op.type = LOOT_OP_ADD_CP;
        op.a = IS_NULLSTR(chance_str) ? 100 : atoi(chance_str);
        op.c = IS_NULLSTR(min_str) ? 1 : atoi(min_str);
        op.d = IS_NULLSTR(max_str) ? op.c : atoi(max_str);
    }
    else if (!str_cmp(op_type_str, "mul_cp")) {
        char factor_str[MSL];
        one_argument(argument, factor_str);
        if (IS_NULLSTR(factor_str)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op mul_cp <factor>" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_MUL_CP;
        op.a = strtol(factor_str, &end, 10);
    }
    else if (!str_cmp(op_type_str, "mul_all")) {
        char factor_str[MSL];
        one_argument(argument, factor_str);
        if (IS_NULLSTR(factor_str)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op mul_all <factor>" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_MUL_ALL_CHANCES;
        op.a = strtol(factor_str, &end, 10);
    }
    else if (!str_cmp(op_type_str, "remove_item")) {
        char vnum_str[MSL];
        one_argument(argument, vnum_str);
        if (IS_NULLSTR(vnum_str)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op remove_item <vnum>" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_REMOVE_ITEM;
        op.a = strtol(vnum_str, &end, 10);
    }
    else if (!str_cmp(op_type_str, "remove_group")) {
        char group_name[MSL];
        one_argument(argument, group_name);
        if (IS_NULLSTR(group_name)) {
            printf_to_char(ch, COLOR_INFO "Usage: add op remove_group <group_name>" COLOR_EOL);
            return false;
        }
        op.type = LOOT_OP_REMOVE_GROUP;
        op.group_name = str_dup(group_name);
    }
    else {
        printf_to_char(ch, COLOR_INFO "Unknown operation type '%s'." COLOR_EOL, op_type_str);
        return false;
    }

    if (!loot_table_add_op(table, op)) {
        printf_to_char(ch, COLOR_INFO "Failed to add operation." COLOR_EOL);
        return false;
    }

    // Re-resolve inheritance
    resolve_all_loot_tables(global_loot_db);

    printf_to_char(ch, COLOR_INFO "Added %s operation." COLOR_EOL, op_type_str);
    return true;
}

static bool tableedit_remove(Mobile* ch, LootTable* table, char* argument)
{
    char arg[MSL];
    one_argument(argument, arg);

    if (IS_NULLSTR(arg)) {
        printf_to_char(ch, COLOR_INFO "Usage: remove <index>" COLOR_EOL);
        return false;
    }

    int index = atoi(arg) - 1;  // Convert to 0-based
    if (index < 0 || index >= table->op_count) {
        printf_to_char(ch, COLOR_INFO "Invalid index. Use 1-%d." COLOR_EOL, table->op_count);
        return false;
    }

    if (!loot_table_remove_op(table, index)) {
        printf_to_char(ch, COLOR_INFO "Failed to remove operation." COLOR_EOL);
        return false;
    }

    // Re-resolve inheritance
    resolve_all_loot_tables(global_loot_db);

    printf_to_char(ch, COLOR_INFO "Removed operation %d." COLOR_EOL, index + 1);
    return true;
}

static bool tableedit_parent(Mobile* ch, LootTable* table, char* argument)
{
    char parent_name[MSL];
    one_argument(argument, parent_name);

    const char* parent = NULL;
    if (!IS_NULLSTR(parent_name) && str_cmp(parent_name, "none")) {
        parent = parent_name;
    }

    if (!loot_table_set_parent(table, parent)) {
        printf_to_char(ch, COLOR_INFO "Failed to set parent." COLOR_EOL);
        return false;
    }

    // Re-resolve inheritance
    resolve_all_loot_tables(global_loot_db);

    if (parent) {
        printf_to_char(ch, COLOR_INFO "Set parent to '%s'." COLOR_EOL, parent);
    } else {
        printf_to_char(ch, COLOR_INFO "Cleared parent." COLOR_EOL);
    }
    return true;
}