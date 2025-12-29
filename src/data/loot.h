////////////////////////////////////////////////////////////////////////////////
// loot.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__LOOT_H
#define MUD98__DATA__LOOT_H

#include <merc.h>

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
void parse_loot_section(LootDB* db, StringBuffer* sb);
void load_global_loot_db();

// Internal API for testing
void resolve_all_loot_tables(LootDB* db);

extern LootDB* global_loot_db;

#endif // !MUD98__DATA__LOOT_H
