////////////////////////////////////////////////////////////////////////////////
// data/loot_save.c
// ROM-OLC persistence for loot groups and tables
////////////////////////////////////////////////////////////////////////////////

#include "loot.h"

#include <merc.h>
#include <comm.h>
#include <config.h>

#include <stdio.h>
#include <string.h>

// Forward declarations
static void write_loot_group(FILE* fp, const LootGroup* group);
static void write_loot_table(FILE* fp, const LootTable* table);

////////////////////////////////////////////////////////////////////////////////
// Save loot section to file (global or area-specific)
////////////////////////////////////////////////////////////////////////////////

void save_loot_section(FILE* fp, Entity* owner)
{
    if (!global_loot_db) {
        return;
    }

    bool has_content = false;

    // Check if there's any content to write for this owner
    for (int i = 0; i < global_loot_db->group_count; i++) {
        if (global_loot_db->groups[i].owner == owner) {
            has_content = true;
            break;
        }
    }
    
    if (!has_content) {
        for (int i = 0; i < global_loot_db->table_count; i++) {
            if (global_loot_db->tables[i].owner == owner) {
                has_content = true;
                break;
            }
        }
    }

    if (!has_content) {
        return;
    }

    fprintf(fp, "#LOOT\n");

    // Write all groups owned by this owner
    for (int i = 0; i < global_loot_db->group_count; i++) {
        if (global_loot_db->groups[i].owner == owner) {
            write_loot_group(fp, &global_loot_db->groups[i]);
        }
    }

    // Write all tables owned by this owner
    for (int i = 0; i < global_loot_db->table_count; i++) {
        if (global_loot_db->tables[i].owner == owner) {
            write_loot_table(fp, &global_loot_db->tables[i]);
        }
    }

    fprintf(fp, "#ENDLOOT\n\n");
}

////////////////////////////////////////////////////////////////////////////////
// Write a single loot group
////////////////////////////////////////////////////////////////////////////////

static void write_loot_group(FILE* fp, const LootGroup* group)
{
    fprintf(fp, "group %s %d\n", group->name, group->rolls);

    for (int i = 0; i < group->entry_count; i++) {
        const LootEntry* entry = &group->entries[i];
        
        if (entry->type == LOOT_ITEM) {
            fprintf(fp, "  item %"PRVNUM" %d %d weight %d\n",
                entry->item_vnum,
                entry->min_qty,
                entry->max_qty,
                entry->weight);
        }
        else if (entry->type == LOOT_CP) {
            fprintf(fp, "  cp %d %d weight %d\n",
                entry->min_qty,
                entry->max_qty,
                entry->weight);
        }
    }

    //fprintf(fp, "end\n\n");
}

////////////////////////////////////////////////////////////////////////////////
// Write a single loot table
////////////////////////////////////////////////////////////////////////////////

static void write_loot_table(FILE* fp, const LootTable* table)
{
    if (table->parent_name && table->parent_name[0] != '\0') {
        fprintf(fp, "table %s : %s\n", table->name, table->parent_name);
    }
    else {
        fprintf(fp, "table %s\n", table->name);
    }

    for (int i = 0; i < table->op_count; i++) {
        const LootOp* op = &table->ops[i];

        switch (op->type) {
        case LOOT_OP_USE_GROUP:
            fprintf(fp, "  use_group %s %d\n", op->group_name, op->a);
            break;

        case LOOT_OP_ADD_ITEM:
            fprintf(fp, "  add_item %d %d %d %d\n", op->a, op->b, op->c, op->d);
            break;

        case LOOT_OP_ADD_CP:
            fprintf(fp, "  add_cp %d %d %d\n", op->a, op->b, op->c);
            break;

        case LOOT_OP_MUL_CP:
            fprintf(fp, "  mul_cp %d\n", op->a);
            break;

        case LOOT_OP_MUL_ALL_CHANCES:
            fprintf(fp, "  mul_all_chances %d\n", op->a);
            break;

        case LOOT_OP_REMOVE_ITEM:
            fprintf(fp, "  remove_item %d\n", op->a);
            break;

        case LOOT_OP_REMOVE_GROUP:
            fprintf(fp, "  remove_group %s\n", op->group_name);
            break;

        default:
            break;
        }
    }

    //fprintf(fp, "end\n\n");
}

////////////////////////////////////////////////////////////////////////////////
// Save global loot database
////////////////////////////////////////////////////////////////////////////////

void save_global_loot_db()
{
    if (!global_loot_db) {
        return;
    }

    char loot_file_path[MIL];
    sprintf(loot_file_path, "%s%s", cfg_get_data_dir(), cfg_get_loot_file());

    FILE* fp = fopen(loot_file_path, "w");
    if (!fp) {
        fprintf(stderr, "Could not open loot file for writing: %s\n", loot_file_path);
        return;
    }

    save_loot_section(fp, NULL);  // NULL owner = global loot
    fclose(fp);

    printf_log("Saved global loot DB: %d groups, %d tables", 
        global_loot_db->group_count, global_loot_db->table_count);
}
