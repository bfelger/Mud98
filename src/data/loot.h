////////////////////////////////////////////////////////////////////////////////
// loot.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__LOOT_H
#define MUD98__DATA__LOOT_H

#include <merc.h>

#include <entities/entity.h>
#include <entities/object.h>
#include <entities/mobile.h>

#include <stringbuffer.h>

#define MAX_LOOT_DROPS 64

typedef enum {
    LOOT_CP         = 1,
    LOOT_ITEM       = 2,
} LootType;

typedef struct loot_entry_t {
    LootType type;
    VNUM item_vnum;     // For LOOT_ITEM, vnum of item. For LOOT_CP, 0.
    int min_qty;        // Item amount or CP amount minimum.
    int max_qty;        // Item amount or CP amount maximum.
    int weight;         // For group weighting.
} LootEntry;

typedef struct loot_group_t {
    Entity* owner;        // NULL=global, AreaData*, MobPrototype*, etc.
    const char* name;
    int rolls;          // Number of times to roll on this group.
    LootEntry* entries;
    int entry_count;
    int entry_capacity;
} LootGroup;

typedef enum {
    LOOT_OP_USE_GROUP           = 1,
    LOOT_OP_ADD_ITEM            = 2,
    LOOT_OP_ADD_CP              = 3,
    LOOT_OP_MUL_CP              = 4,
    LOOT_OP_MUL_ALL_CHANCES     = 5,
    LOOT_OP_REMOVE_ITEM         = 6,
    LOOT_OP_REMOVE_GROUP        = 7,
} LootOpType;

typedef struct loot_op_t {
    LootOpType type;
    const char* group_name;     // For group operations.
    int a;      // Generic params:
    int b;      //     USE_GROUP: a=chance
    int c;      //     ADD_ITEM : a=vnum b=chance c=min (max stored in d? we use b/c + extra in e) -> see below
    int d;      //     ADD_ITEM max, ADD_CP max, etc
} LootOp;

typedef struct loot_table_t {
    Entity* owner;        // NULL=global, AreaData*, MobPrototype*, etc.
    const char* name;
    const char* parent_name;
    
    LootOp* ops;
    int op_count;
    int op_capacity;

    // Resolved/linearized ops (parent first)
    LootOp* resolved_ops;
    int resolved_op_count;

    // For cycle detection during resolution
    int _visit; // 0=unvisited, 1=visiting, 2=visited
} LootTable;

typedef struct loot_db_t {
    LootGroup* groups;
    int group_count;
    int group_capacity;

    LootTable* tables;
    int table_count;
    int table_capacity;
} LootDB;

// Result of a single loot roll
typedef struct loot_drop_t {
    LootType type;
    VNUM item_vnum;     // For LOOT_ITEM, vnum of item. For LOOT_CP, 0.
    int qty;            // Item amount or CP amount.
} LootDrop;

void loot_db_free(LootDB* db);
void generate_loot(LootDB* db, const char* table_name, 
    LootDrop drops[MAX_LOOT_DROPS], size_t* drop_count);
void add_loot_to_container(LootDrop* drops, size_t drop_count, Object* container);
void add_loot_to_mobile(LootDrop* drops, size_t drop_count, Mobile* mob);
void parse_loot_section(LootDB* db, StringBuffer* sb, Entity* owner);
void load_global_loot_db();
void save_global_loot_db();
void save_global_loot_db_as(const char* filename);  // Force specific format via extension

// Per-area/mob loot support (merged into global DB with owner tagging)
void load_area_loot(FILE* fp, Entity* owner);
void save_loot_section(FILE* fp, Entity* owner);
LootGroup* loot_db_find_group(LootDB* db, const char* name);
LootTable* loot_db_find_table(LootDB* db, const char* name);

// OLC editing support
LootGroup* loot_db_create_group(LootDB* db, const char* name, int rolls, Entity* owner);
LootTable* loot_db_create_table(LootDB* db, const char* name, const char* parent, Entity* owner);
bool loot_db_delete_group(LootDB* db, const char* name, Entity* owner);
bool loot_db_delete_table(LootDB* db, const char* name, Entity* owner);
bool loot_group_add_item(LootGroup* group, VNUM vnum, int weight, int min_qty, int max_qty);
bool loot_group_add_cp(LootGroup* group, int weight, int min_qty, int max_qty);
bool loot_group_remove_entry(LootGroup* group, int index);
bool loot_table_add_op(LootTable* table, LootOp op);
bool loot_table_remove_op(LootTable* table, int index);
bool loot_table_set_parent(LootTable* table, const char* parent_name);

// Internal API for testing
void resolve_all_loot_tables(LootDB* db);

extern LootDB* global_loot_db;

#endif // !MUD98__DATA__LOOT_H
