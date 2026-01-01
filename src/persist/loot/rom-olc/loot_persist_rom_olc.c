////////////////////////////////////////////////////////////////////////////////
// persist/loot/rom-olc/loot_persist_rom_olc.c
// ROM-OLC persistence implementation for loot groups and tables
////////////////////////////////////////////////////////////////////////////////

#include "loot_persist_rom_olc.h"

#include <persist/persist_io_adapters.h>

#include <data/loot.h>

#include <comm.h>
#include <config.h>
#include <merc.h>
#include <stringbuffer.h>

#include <stdio.h>
#include <string.h>

const LootPersistFormat LOOT_PERSIST_ROM_OLC = {
    .name = "rom-olc",
    .load = loot_persist_rom_olc_load,
    .save = loot_persist_rom_olc_save,
};

////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////

static void write_loot_group(FILE* fp, const LootGroup* group);
static void write_loot_table(FILE* fp, const LootTable* table);
static char* scarf_loot_section(FILE* fp, StringBuffer* sb);

////////////////////////////////////////////////////////////////////////////////
// Load
////////////////////////////////////////////////////////////////////////////////

PersistResult loot_persist_rom_olc_load(const PersistReader* reader, 
    const char* filename, Entity* owner)
{
    if (!reader)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "loot ROM-OLC load: missing reader", -1 };

    // Get FILE* from reader (ROM-OLC format requires FILE*)
    FILE* fp = NULL;
    if (reader->ops == &PERSIST_FILE_STREAM_OPS) {
        fp = (FILE*)reader->ctx;
    }
    else {
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "loot ROM-OLC load: requires FILE reader", -1 };
    }

    // Initialize global_loot_db if needed
    if (!global_loot_db) {
        global_loot_db = calloc(1, sizeof(LootDB));
        if (!global_loot_db)
            return (PersistResult){ PERSIST_ERR_INTERNAL, "loot ROM-OLC load: alloc failed", -1 };
    }

    // Read loot section until #ENDLOOT
    StringBuffer* sb = sb_new();
    scarf_loot_section(fp, sb);
    parse_loot_section(global_loot_db, sb, owner);
    sb_free(sb);

    // Resolve inheritance after adding new groups/tables
    resolve_all_loot_tables(global_loot_db);

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

////////////////////////////////////////////////////////////////////////////////
// Save
////////////////////////////////////////////////////////////////////////////////

PersistResult loot_persist_rom_olc_save(const PersistWriter* writer, 
    const char* filename, Entity* owner)
{
    if (!writer)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "loot ROM-OLC save: missing writer", -1 };

    if (!global_loot_db)
        return (PersistResult){ PERSIST_OK, NULL, -1 };  // Nothing to save

    // Get FILE* from writer (ROM-OLC format requires FILE*)
    FILE* fp = NULL;
    if (writer->ops == &PERSIST_FILE_WRITER_OPS) {
        fp = (FILE*)writer->ctx;
    }
    else {
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "loot ROM-OLC save: requires FILE writer", -1 };
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

    if (!has_content)
        return (PersistResult){ PERSIST_OK, NULL, -1 };

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

    if (writer->ops->flush)
        writer->ops->flush(writer->ctx);

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

////////////////////////////////////////////////////////////////////////////////
// Helpers
////////////////////////////////////////////////////////////////////////////////

static char* scarf_loot_section(FILE* fp, StringBuffer* sb) 
{
    // Read all until #ENDLOOT or EOF
    char line[MIL];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "#ENDLOOT", 8) == 0) {
            break;
        }
        sb_append(sb, line);
    }
    return sb->data;
}

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
}

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
            fprintf(fp, "  add_cp %d %d %d\n", op->a, op->c, op->d);
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
}
