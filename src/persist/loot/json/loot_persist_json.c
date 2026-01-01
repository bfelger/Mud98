////////////////////////////////////////////////////////////////////////////////
// persist/loot/json/loot_persist_json.c
// JSON persistence implementation for loot groups and tables
////////////////////////////////////////////////////////////////////////////////

#include "loot_persist_json.h"

#include <persist/json/persist_json.h>
#include <persist/persist_io_adapters.h>

#include <data/loot.h>

#include <comm.h>
#include <config.h>
#include <merc.h>

#include <jansson.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const LootPersistFormat LOOT_PERSIST_JSON = {
    .name = "json",
    .load = loot_persist_json_load,
    .save = loot_persist_json_save,
};

////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////

static json_t* build_loot_group(const LootGroup* group);
static json_t* build_loot_table(const LootTable* table);
static bool parse_loot_group(json_t* obj, LootDB* db, Entity* owner);
static bool parse_loot_table(json_t* obj, LootDB* db, Entity* owner);

////////////////////////////////////////////////////////////////////////////////
// Save
////////////////////////////////////////////////////////////////////////////////

PersistResult loot_persist_json_save(const PersistWriter* writer, 
    const char* filename, Entity* owner)
{
    if (!writer)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "loot JSON save: missing writer", -1 };

    if (!global_loot_db)
        return (PersistResult){ PERSIST_OK, NULL, -1 };  // Nothing to save

    json_t* root = json_object();
    JSON_SET_INT(root, "formatVersion", 1);

    // Build groups array
    json_t* groups = json_array();
    for (int i = 0; i < global_loot_db->group_count; i++) {
        if (global_loot_db->groups[i].owner == owner) {
            json_array_append_new(groups, build_loot_group(&global_loot_db->groups[i]));
        }
    }
    json_object_set_new(root, "groups", groups);

    // Build tables array
    json_t* tables = json_array();
    for (int i = 0; i < global_loot_db->table_count; i++) {
        if (global_loot_db->tables[i].owner == owner) {
            json_array_append_new(tables, build_loot_table(&global_loot_db->tables[i]));
        }
    }
    json_object_set_new(root, "tables", tables);

    char* dump = json_dumps(root, JSON_INDENT(2));
    json_decref(root);
    if (!dump)
        return (PersistResult){ PERSIST_ERR_INTERNAL, "loot JSON save: dump failed", -1 };

    size_t len = strlen(dump);
    bool ok = writer_write_all(writer, dump, len);
    free(dump);
    if (!ok)
        return (PersistResult){ PERSIST_ERR_IO, "loot JSON save: write failed", -1 };

    if (writer->ops->flush)
        writer->ops->flush(writer->ctx);

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

////////////////////////////////////////////////////////////////////////////////
// Load
////////////////////////////////////////////////////////////////////////////////

PersistResult loot_persist_json_load(const PersistReader* reader, 
    const char* filename, Entity* owner)
{
    if (!reader)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "loot JSON load: missing reader", -1 };

    ReaderBuffer buf = { 0 };
    if (!reader_fill_buffer(reader, &buf))
        return (PersistResult){ PERSIST_ERR_IO, "loot JSON load: failed to read", -1 };

    json_error_t error;
    json_t* root = json_loadb(buf.data, buf.len, 0, &error);
    free(buf.data);
    if (!root) {
        static char msg[256];
        snprintf(msg, sizeof(msg), "loot JSON parse error at line %d: %s", error.line, error.text);
        return (PersistResult){ PERSIST_ERR_FORMAT, msg, (int)error.line };
    }

    json_t* fmtv = json_object_get(root, "formatVersion");
    if (json_is_integer(fmtv) && json_integer_value(fmtv) != 1) {
        json_decref(root);
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "loot JSON: unsupported formatVersion", -1 };
    }

    // Initialize global_loot_db if needed
    if (!global_loot_db) {
        global_loot_db = calloc(1, sizeof(LootDB));
        if (!global_loot_db) {
            json_decref(root);
            return (PersistResult){ PERSIST_ERR_INTERNAL, "loot JSON load: alloc failed", -1 };
        }
    }

    // Parse groups
    json_t* groups = json_object_get(root, "groups");
    if (json_is_array(groups)) {
        size_t count = json_array_size(groups);
        for (size_t i = 0; i < count; i++) {
            json_t* g = json_array_get(groups, i);
            if (json_is_object(g)) {
                parse_loot_group(g, global_loot_db, owner);
            }
        }
    }

    // Parse tables
    json_t* tables = json_object_get(root, "tables");
    if (json_is_array(tables)) {
        size_t count = json_array_size(tables);
        for (size_t i = 0; i < count; i++) {
            json_t* t = json_array_get(tables, i);
            if (json_is_object(t)) {
                parse_loot_table(t, global_loot_db, owner);
            }
        }
    }

    json_decref(root);

    // Resolve inheritance after adding new groups/tables
    resolve_all_loot_tables(global_loot_db);

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}

////////////////////////////////////////////////////////////////////////////////
// Build helpers
////////////////////////////////////////////////////////////////////////////////

static json_t* build_loot_group(const LootGroup* group)
{
    json_t* obj = json_object();
    JSON_SET_STRING(obj, "name", group->name);
    JSON_SET_INT(obj, "rolls", group->rolls);

    json_t* entries = json_array();
    for (int i = 0; i < group->entry_count; i++) {
        const LootEntry* entry = &group->entries[i];
        json_t* e = json_object();

        if (entry->type == LOOT_ITEM) {
            JSON_SET_STRING(e, "type", "item");
            JSON_SET_INT(e, "vnum", entry->item_vnum);
        }
        else if (entry->type == LOOT_CP) {
            JSON_SET_STRING(e, "type", "cp");
        }

        JSON_SET_INT(e, "minQty", entry->min_qty);
        JSON_SET_INT(e, "maxQty", entry->max_qty);
        JSON_SET_INT(e, "weight", entry->weight);

        json_array_append_new(entries, e);
    }
    json_object_set_new(obj, "entries", entries);

    return obj;
}

static json_t* build_loot_table(const LootTable* table)
{
    json_t* obj = json_object();
    JSON_SET_STRING(obj, "name", table->name);
    
    if (table->parent_name && table->parent_name[0] != '\0') {
        JSON_SET_STRING(obj, "parent", table->parent_name);
    }

    json_t* ops = json_array();
    for (int i = 0; i < table->op_count; i++) {
        const LootOp* op = &table->ops[i];
        json_t* o = json_object();

        switch (op->type) {
        case LOOT_OP_USE_GROUP:
            JSON_SET_STRING(o, "op", "use_group");
            JSON_SET_STRING(o, "group", op->group_name);
            JSON_SET_INT(o, "chance", op->a);
            break;

        case LOOT_OP_ADD_ITEM:
            JSON_SET_STRING(o, "op", "add_item");
            JSON_SET_INT(o, "vnum", op->a);
            JSON_SET_INT(o, "chance", op->b);
            JSON_SET_INT(o, "minQty", op->c);
            JSON_SET_INT(o, "maxQty", op->d);
            break;

        case LOOT_OP_ADD_CP:
            JSON_SET_STRING(o, "op", "add_cp");
            JSON_SET_INT(o, "chance", op->a);
            JSON_SET_INT(o, "minQty", op->c);
            JSON_SET_INT(o, "maxQty", op->d);
            break;

        case LOOT_OP_MUL_CP:
            JSON_SET_STRING(o, "op", "mul_cp");
            JSON_SET_INT(o, "multiplier", op->a);
            break;

        case LOOT_OP_MUL_ALL_CHANCES:
            JSON_SET_STRING(o, "op", "mul_all_chances");
            JSON_SET_INT(o, "multiplier", op->a);
            break;

        case LOOT_OP_REMOVE_ITEM:
            JSON_SET_STRING(o, "op", "remove_item");
            JSON_SET_INT(o, "vnum", op->a);
            break;

        case LOOT_OP_REMOVE_GROUP:
            JSON_SET_STRING(o, "op", "remove_group");
            JSON_SET_STRING(o, "group", op->group_name);
            break;

        default:
            continue;
        }

        json_array_append_new(ops, o);
    }
    json_object_set_new(obj, "ops", ops);

    return obj;
}

////////////////////////////////////////////////////////////////////////////////
// Parse helpers
////////////////////////////////////////////////////////////////////////////////

static bool parse_loot_group(json_t* obj, LootDB* db, Entity* owner)
{
    const char* name = JSON_STRING(obj, "name");
    if (!name || !*name)
        return false;

    int rolls = (int)json_int_or_default(obj, "rolls", 1);

    LootGroup* group = loot_db_create_group(db, name, rolls, owner);
    if (!group)
        return false;

    json_t* entries = json_object_get(obj, "entries");
    if (json_is_array(entries)) {
        size_t count = json_array_size(entries);
        for (size_t i = 0; i < count; i++) {
            json_t* e = json_array_get(entries, i);
            if (!json_is_object(e))
                continue;

            const char* type = JSON_STRING(e, "type");
            int min_qty = (int)json_int_or_default(e, "minQty", 1);
            int max_qty = (int)json_int_or_default(e, "maxQty", 1);
            int weight = (int)json_int_or_default(e, "weight", 100);

            if (type && !str_cmp(type, "item")) {
                VNUM vnum = (VNUM)json_int_or_default(e, "vnum", 0);
                loot_group_add_item(group, vnum, weight, min_qty, max_qty);
            }
            else if (type && !str_cmp(type, "cp")) {
                loot_group_add_cp(group, weight, min_qty, max_qty);
            }
        }
    }

    return true;
}

static bool parse_loot_table(json_t* obj, LootDB* db, Entity* owner)
{
    const char* name = JSON_STRING(obj, "name");
    if (!name || !*name)
        return false;

    const char* parent = JSON_STRING(obj, "parent");

    LootTable* table = loot_db_create_table(db, name, parent, owner);
    if (!table)
        return false;

    json_t* ops = json_object_get(obj, "ops");
    if (json_is_array(ops)) {
        size_t count = json_array_size(ops);
        for (size_t i = 0; i < count; i++) {
            json_t* o = json_array_get(ops, i);
            if (!json_is_object(o))
                continue;

            const char* optype = JSON_STRING(o, "op");
            if (!optype)
                continue;

            LootOp lop = { 0 };

            if (!str_cmp(optype, "use_group")) {
                lop.type = LOOT_OP_USE_GROUP;
                const char* grp = JSON_STRING(o, "group");
                lop.group_name = grp ? str_dup(grp) : NULL;
                lop.a = (int)json_int_or_default(o, "chance", 100);
            }
            else if (!str_cmp(optype, "add_item")) {
                lop.type = LOOT_OP_ADD_ITEM;
                lop.a = (int)json_int_or_default(o, "vnum", 0);
                lop.b = (int)json_int_or_default(o, "chance", 100);
                lop.c = (int)json_int_or_default(o, "minQty", 1);
                lop.d = (int)json_int_or_default(o, "maxQty", 1);
            }
            else if (!str_cmp(optype, "add_cp")) {
                lop.type = LOOT_OP_ADD_CP;
                lop.a = (int)json_int_or_default(o, "chance", 100);
                lop.c = (int)json_int_or_default(o, "minQty", 1);
                lop.d = (int)json_int_or_default(o, "maxQty", 1);
            }
            else if (!str_cmp(optype, "mul_cp")) {
                lop.type = LOOT_OP_MUL_CP;
                lop.a = (int)json_int_or_default(o, "multiplier", 100);
            }
            else if (!str_cmp(optype, "mul_all_chances")) {
                lop.type = LOOT_OP_MUL_ALL_CHANCES;
                lop.a = (int)json_int_or_default(o, "multiplier", 100);
            }
            else if (!str_cmp(optype, "remove_item")) {
                lop.type = LOOT_OP_REMOVE_ITEM;
                lop.a = (int)json_int_or_default(o, "vnum", 0);
            }
            else if (!str_cmp(optype, "remove_group")) {
                lop.type = LOOT_OP_REMOVE_GROUP;
                const char* grp = JSON_STRING(o, "group");
                lop.group_name = grp ? str_dup(grp) : NULL;
            }
            else {
                continue;
            }

            loot_table_add_op(table, lop);
        }
    }

    return true;
}
