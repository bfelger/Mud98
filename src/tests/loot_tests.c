////////////////////////////////////////////////////////////////////////////////
// loot_tests.c
// Unit tests for loot database parsing, resolution, and generation
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <data/loot.h>
#include <db.h>
#include <merc.h>
#include <stringbuffer.h>

#include <string.h>
#include <stdio.h>

TestGroup loot_tests;

// ============================================================================
// Test Helper Functions
// ============================================================================

// Create a fresh empty LootDB for testing
static LootDB* test_create_db()
{
    LootDB* db = (LootDB*)malloc(sizeof(LootDB));
    memset(db, 0, sizeof(LootDB));
    return db;
}

// Parse a simple loot configuration from string
static void parse_test_loot(LootDB* db, const char* config)
{
    StringBuffer* sb = sb_new();
    sb_append(sb, config);
    parse_loot_section(db, sb, NULL);  // NULL owner for test data
    sb_free(sb);
}

// ============================================================================
// LootGroup Tests
// ============================================================================

static int test_loot_group_creation()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group BASIC_DROP 1\n"
        "  item 1001 1 1 weight 50\n"
        "  item 1002 2 3 weight 30\n";

    parse_test_loot(db, config);

    ASSERT(db->group_count == 1);
    LootGroup* group = &db->groups[0];
    ASSERT(group != NULL);
    ASSERT(!strcmp(group->name, "BASIC_DROP"));
    ASSERT(group->rolls == 1);
    ASSERT(group->entry_count == 2);

    // Check first entry
    ASSERT(group->entries[0].type == LOOT_ITEM);
    ASSERT(group->entries[0].item_vnum == 1001);
    ASSERT(group->entries[0].min_qty == 1);
    ASSERT(group->entries[0].max_qty == 1);
    ASSERT(group->entries[0].weight == 50);

    // Check second entry
    ASSERT(group->entries[1].type == LOOT_ITEM);
    ASSERT(group->entries[1].item_vnum == 1002);
    ASSERT(group->entries[1].min_qty == 2);
    ASSERT(group->entries[1].max_qty == 3);
    ASSERT(group->entries[1].weight == 30);

    loot_db_free(db);
    return 0;
}

static int test_loot_group_with_cp()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group GOLD_DROP 1\n"
        "  item 1001 1 1 weight 40\n"
        "  cp 100 200 weight 60\n";

    parse_test_loot(db, config);

    ASSERT(db->group_count == 1);
    LootGroup* group = &db->groups[0];
    ASSERT(group->entry_count == 2);

    // First entry is item
    ASSERT(group->entries[0].type == LOOT_ITEM);

    // Second entry is copper
    ASSERT(group->entries[1].type == LOOT_CP);
    ASSERT(group->entries[1].min_qty == 100);
    ASSERT(group->entries[1].max_qty == 200);
    ASSERT(group->entries[1].weight == 60);

    loot_db_free(db);
    return 0;
}

static int test_loot_group_multiple_rolls()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group MULTI_ROLL 3\n"
        "  item 1001 1 1 weight 50\n"
        "  item 1002 1 1 weight 50\n";

    parse_test_loot(db, config);

    ASSERT(db->group_count == 1);
    LootGroup* group = &db->groups[0];
    ASSERT(group->rolls == 3);
    ASSERT(group->entry_count == 2);

    loot_db_free(db);
    return 0;
}

static int test_loot_duplicate_group_merge()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group DUPLICATE 1\n"
        "  item 1001 1 1 weight 50\n"
        "group DUPLICATE 1\n"
        "  item 1002 1 1 weight 50\n";

    parse_test_loot(db, config);

    // Should only have 1 group
    ASSERT(db->group_count == 1);
    LootGroup* group = &db->groups[0];
    // But both entries should be present (second definition augments first)
    ASSERT(group->entry_count == 2);
    ASSERT(group->rolls == 1);
    ASSERT(group->entries[0].item_vnum == 1001);
    ASSERT(group->entries[1].item_vnum == 1002);

    loot_db_free(db);
    return 0;
}

// ============================================================================
// LootTable and Operation Tests
// ============================================================================

static int test_loot_table_creation()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group ITEMS 1\n"
        "  item 1001 1 1 weight 50\n"
        "table TEST_TABLE\n"
        "  use_group ITEMS 50\n"
        "  add_item 2001 30 1 1\n";

    parse_test_loot(db, config);

    ASSERT(db->table_count == 1);
    LootTable* table = &db->tables[0];
    ASSERT(!strcmp(table->name, "TEST_TABLE"));
    ASSERT(table->op_count == 2);

    // Check use_group op
    ASSERT(table->ops[0].type == LOOT_OP_USE_GROUP);
    ASSERT(!strcmp(table->ops[0].group_name, "ITEMS"));
    ASSERT(table->ops[0].a == 50);

    // Check add_item op
    ASSERT(table->ops[1].type == LOOT_OP_ADD_ITEM);
    ASSERT(table->ops[1].a == 2001);  // vnum
    ASSERT(table->ops[1].b == 30);    // chance
    ASSERT(table->ops[1].c == 1);     // min
    ASSERT(table->ops[1].d == 1);     // max

    loot_db_free(db);
    return 0;
}

static int test_loot_table_with_parent()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group ITEMS 1\n"
        "  item 1001 1 1 weight 50\n"
        "table BASE\n"
        "  use_group ITEMS 50\n"
        "table DERIVED parent BASE\n"
        "  add_item 2001 30 1 1\n";

    parse_test_loot(db, config);

    ASSERT(db->table_count == 2);
    LootTable* base = &db->tables[0];
    LootTable* derived = &db->tables[1];

    ASSERT(!strcmp(base->name, "BASE"));
    ASSERT(!strcmp(derived->name, "DERIVED"));
    ASSERT(!strcmp(derived->parent_name, "BASE"));

    loot_db_free(db);
    return 0;
}

static int test_loot_op_add_cp()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table TEST\n"
        "  add_cp 50 100 500\n";

    parse_test_loot(db, config);

    ASSERT(db->table_count == 1);
    LootTable* table = &db->tables[0];
    ASSERT(table->op_count == 1);

    LootOp* op = &table->ops[0];
    ASSERT(op->type == LOOT_OP_ADD_CP);
    ASSERT(op->a == 50);   // chance
    ASSERT(op->c == 100);  // min
    ASSERT(op->d == 500);  // max

    loot_db_free(db);
    return 0;
}

static int test_loot_op_mul_cp()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table TEST\n"
        "  mul_cp 150\n";

    parse_test_loot(db, config);

    ASSERT(db->table_count == 1);
    LootTable* table = &db->tables[0];
    ASSERT(table->op_count == 1);

    LootOp* op = &table->ops[0];
    ASSERT(op->type == LOOT_OP_MUL_CP);
    ASSERT(op->a == 150);

    loot_db_free(db);
    return 0;
}

static int test_loot_op_mul_all_chances()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table TEST\n"
        "  mul_all_chances 50\n";

    parse_test_loot(db, config);

    ASSERT(db->table_count == 1);
    LootTable* table = &db->tables[0];
    ASSERT(table->op_count == 1);

    LootOp* op = &table->ops[0];
    ASSERT(op->type == LOOT_OP_MUL_ALL_CHANCES);
    ASSERT(op->a == 50);

    loot_db_free(db);
    return 0;
}

static int test_loot_op_remove_item()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table TEST\n"
        "  remove_item 1001\n";

    parse_test_loot(db, config);

    ASSERT(db->table_count == 1);
    LootTable* table = &db->tables[0];
    ASSERT(table->op_count == 1);

    LootOp* op = &table->ops[0];
    ASSERT(op->type == LOOT_OP_REMOVE_ITEM);
    ASSERT(op->a == 1001);

    loot_db_free(db);
    return 0;
}

static int test_loot_op_remove_group()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table TEST\n"
        "  remove_group EXCLUDE_GROUP\n";

    parse_test_loot(db, config);

    ASSERT(db->table_count == 1);
    LootTable* table = &db->tables[0];
    ASSERT(table->op_count == 1);

    LootOp* op = &table->ops[0];
    ASSERT(op->type == LOOT_OP_REMOVE_GROUP);
    ASSERT(!strcmp(op->group_name, "EXCLUDE_GROUP"));

    loot_db_free(db);
    return 0;
}

// ============================================================================
// Table Inheritance and Resolution Tests
// ============================================================================

static int test_loot_inheritance_single_level()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group ITEMS 1\n"
        "  item 1001 1 1 weight 50\n"
        "table BASE\n"
        "  use_group ITEMS 50\n"
        "table CHILD parent BASE\n"
        "  add_item 2001 30 1 1\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    ASSERT(db->table_count == 2);
    LootTable* child = &db->tables[1];

    // Child should have 2 resolved ops: parent's use_group + own add_item
    ASSERT(child->resolved_op_count == 2);
    ASSERT(child->resolved_ops[0].type == LOOT_OP_USE_GROUP);
    ASSERT(child->resolved_ops[1].type == LOOT_OP_ADD_ITEM);

    loot_db_free(db);
    return 0;
}

static int test_loot_inheritance_multi_level()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group ITEMS 1\n"
        "  item 1001 1 1 weight 50\n"
        "table BASE\n"
        "  use_group ITEMS 50\n"
        "table MIDDLE parent BASE\n"
        "  add_cp 50 100 200\n"
        "table LEAF parent MIDDLE\n"
        "  add_item 2001 30 1 1\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    ASSERT(db->table_count == 3);
    LootTable* leaf = &db->tables[2];

    // Leaf should have 3 resolved ops: BASE's use_group + MIDDLE's add_cp + own add_item
    ASSERT(leaf->resolved_op_count == 3);
    ASSERT(leaf->resolved_ops[0].type == LOOT_OP_USE_GROUP);
    ASSERT(leaf->resolved_ops[1].type == LOOT_OP_ADD_CP);
    ASSERT(leaf->resolved_ops[2].type == LOOT_OP_ADD_ITEM);

    loot_db_free(db);
    return 0;
}

static int test_loot_table_without_parent()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table STANDALONE\n"
        "  add_item 2001 50 1 1\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    ASSERT(db->table_count == 1);
    LootTable* table = &db->tables[0];

    // Table without parent should have just own op
    ASSERT(table->resolved_op_count == 1);
    ASSERT(table->resolved_ops[0].type == LOOT_OP_ADD_ITEM);

    loot_db_free(db);
    return 0;
}

static int test_loot_cycle_detection()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table A parent B\n"
        "table B parent A\n";

    parse_test_loot(db, config);

    // Should not crash or infinite loop, just detect cycle
    resolve_all_loot_tables(db);

    // Both tables should be marked as visited (even with cycle)
    ASSERT(db->tables[0]._visit != 0);
    ASSERT(db->tables[1]._visit != 0);

    loot_db_free(db);
    return 0;
}

// ============================================================================
// Loot Generation Tests
// ============================================================================

static int test_generate_loot_simple_group()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group ITEMS 1\n"
        "  item 1001 1 1 weight 100\n"
        "table TEST\n"
        "  use_group ITEMS 100\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    LootDrop drops[MAX_LOOT_DROPS];
    size_t drop_count = 0;

    generate_loot(db, "TEST", drops, &drop_count);

    // Should have generated exactly one drop (100% chance, 1 roll)
    ASSERT(drop_count == 1);
    ASSERT(drops[0].type == LOOT_ITEM);
    ASSERT(drops[0].item_vnum == 1001);
    ASSERT(drops[0].qty == 1);

    loot_db_free(db);
    return 0;
}

static int test_generate_loot_multi_roll()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group ITEMS 3\n"
        "  item 1001 1 1 weight 50\n"
        "  item 1002 1 1 weight 50\n"
        "table TEST\n"
        "  use_group ITEMS 100\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    LootDrop drops[MAX_LOOT_DROPS];
    size_t drop_count = 0;

    generate_loot(db, "TEST", drops, &drop_count);

    // Should generate 3 rolls, items may merge
    ASSERT(drop_count >= 1);
    ASSERT(drop_count <= 2);

    int total_qty = 0;
    for (size_t i = 0; i < drop_count; i++)
        total_qty += drops[i].qty;

    ASSERT(total_qty == 3);  // 3 rolls total

    loot_db_free(db);
    return 0;
}

static int test_generate_loot_with_cp()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group MONEY 1\n"
        "  cp 100 100 weight 100\n"
        "table TEST\n"
        "  use_group MONEY 100\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    LootDrop drops[MAX_LOOT_DROPS];
    size_t drop_count = 0;

    generate_loot(db, "TEST", drops, &drop_count);

    ASSERT(drop_count == 1);
    ASSERT(drops[0].type == LOOT_CP);
    ASSERT(drops[0].qty == 100);

    loot_db_free(db);
    return 0;
}

static int test_generate_loot_direct_add_item()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table TEST\n"
        "  add_item 1001 100 2 3\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    LootDrop drops[MAX_LOOT_DROPS];
    size_t drop_count = 0;

    generate_loot(db, "TEST", drops, &drop_count);

    ASSERT(drop_count == 1);
    ASSERT(drops[0].type == LOOT_ITEM);
    ASSERT(drops[0].item_vnum == 1001);
    // Qty should be between 2-3
    ASSERT(drops[0].qty >= 2);
    ASSERT(drops[0].qty <= 3);

    loot_db_free(db);
    return 0;
}

static int test_generate_loot_direct_add_cp()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table TEST\n"
        "  add_cp 100 50 100\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    LootDrop drops[MAX_LOOT_DROPS];
    size_t drop_count = 0;

    generate_loot(db, "TEST", drops, &drop_count);

    ASSERT(drop_count == 1);
    ASSERT(drops[0].type == LOOT_CP);
    // Qty should be between 50-100
    ASSERT(drops[0].qty >= 50);
    ASSERT(drops[0].qty <= 100);

    loot_db_free(db);
    return 0;
}

static int test_generate_loot_cp_multiplier()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table TEST\n"
        "  mul_cp 200\n"
        "  add_cp 100 100 100\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    LootDrop drops[MAX_LOOT_DROPS];
    size_t drop_count = 0;

    generate_loot(db, "TEST", drops, &drop_count);

    ASSERT(drop_count == 1);
    ASSERT(drops[0].type == LOOT_CP);
    // Should be approximately 200 (100 * 200/100)
    ASSERT(drops[0].qty >= 150);  // Allow some variance
    ASSERT(drops[0].qty <= 250);

    loot_db_free(db);
    return 0;
}

static int test_generate_loot_chance_multiplier()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table TEST\n"
        "  mul_all_chances 50\n"
        "  add_item 1001 100 1 1\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    // With 50% chance multiplier, effective chance is 50%
    // Run multiple times to verify distribution
    int generated_count = 0;
    for (int i = 0; i < 100; i++) {
        LootDrop drops[MAX_LOOT_DROPS];
        size_t drop_count = 0;
        generate_loot(db, "TEST", drops, &drop_count);
        if (drop_count > 0)
            generated_count++;
    }

    // Should generate approximately 50% of the time
    // Allow 20-80 range for 100 trials
    ASSERT(generated_count >= 20);
    ASSERT(generated_count <= 80);

    loot_db_free(db);
    return 0;
}

static int test_generate_loot_remove_item()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group ITEMS 1\n"
        "  item 1001 1 1 weight 50\n"
        "  item 1002 1 1 weight 50\n"
        "table TEST\n"
        "  remove_item 1002\n"
        "  use_group ITEMS 100\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    // Run multiple times since it's random which item is picked
    bool generated_1001 = false;
    for (int trial = 0; trial < 50; trial++) {
        LootDrop drops[MAX_LOOT_DROPS];
        size_t drop_count = 0;

        generate_loot(db, "TEST", drops, &drop_count);

        // Item 1002 should never be generated (removed before use_group)
        for (size_t i = 0; i < drop_count; i++) {
            if (drops[i].type == LOOT_ITEM) {
                ASSERT(drops[i].item_vnum != 1002);
                if (drops[i].item_vnum == 1001)
                    generated_1001 = true;
            }
        }
    }

    // Should have seen item 1001 at least once
    ASSERT(generated_1001);

    loot_db_free(db);
    return 0;
}

static int test_generate_loot_remove_group()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group ITEMS1 1\n"
        "  item 1001 1 1 weight 100\n"
        "group ITEMS2 1\n"
        "  item 1002 1 1 weight 100\n"
        "table TEST\n"
        "  remove_group ITEMS2\n"
        "  use_group ITEMS1 100\n"
        "  use_group ITEMS2 100\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    // Run multiple times to ensure removal works
    bool generated_1001 = false;
    for (int trial = 0; trial < 50; trial++) {
        LootDrop drops[MAX_LOOT_DROPS];
        size_t drop_count = 0;

        generate_loot(db, "TEST", drops, &drop_count);

        // Should only have item 1001, not 1002 (ITEMS2 was removed before use)
        for (size_t i = 0; i < drop_count; i++) {
            ASSERT(drops[i].item_vnum != 1002);
            if (drops[i].item_vnum == 1001)
                generated_1001 = true;
        }
    }

    // Should have seen item 1001 at least once
    ASSERT(generated_1001);

    loot_db_free(db);
    return 0;
}

static int test_generate_loot_drop_merging()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table TEST\n"
        "  add_item 1001 100 1 1\n"
        "  add_item 1001 100 1 1\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    LootDrop drops[MAX_LOOT_DROPS];
    size_t drop_count = 0;

    generate_loot(db, "TEST", drops, &drop_count);

    // Should merge two drops of same item into one
    ASSERT(drop_count == 1);
    ASSERT(drops[0].item_vnum == 1001);
    ASSERT(drops[0].qty == 2);

    loot_db_free(db);
    return 0;
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

static int test_generate_loot_empty_group()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group EMPTY 1\n"
        "table TEST\n"
        "  use_group EMPTY 100\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    LootDrop drops[MAX_LOOT_DROPS];
    size_t drop_count = 0;

    generate_loot(db, "TEST", drops, &drop_count);

    // Empty group should produce no drops
    ASSERT(drop_count == 0);

    loot_db_free(db);
    return 0;
}

static int test_generate_loot_invalid_table()
{
    LootDB* db = test_create_db();

    LootDrop drops[MAX_LOOT_DROPS];
    size_t drop_count = 0;

    // Trying to generate with non-existent table should not crash
    generate_loot(db, "NONEXISTENT", drops, &drop_count);

    ASSERT(drop_count == 0);

    loot_db_free(db);
    return 0;
}

static int test_loot_group_with_zero_weight()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group ITEMS 1\n"
        "  item 1001 1 1 weight 0\n"
        "  item 1002 1 1 weight 50\n"
        "table TEST\n"
        "  use_group ITEMS 100\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    LootDrop drops[MAX_LOOT_DROPS];
    size_t drop_count = 0;

    generate_loot(db, "TEST", drops, &drop_count);

    // Item with 0 weight should be skipped
    for (size_t i = 0; i < drop_count; i++) {
        ASSERT(drops[i].item_vnum != 1001);
    }

    loot_db_free(db);
    return 0;
}

static int test_loot_zero_chance_add_item()
{
    LootDB* db = test_create_db();

    const char* config = 
        "table TEST\n"
        "  add_item 1001 0 1 1\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    // Run 10 times to verify 0% chance always fails
    for (int i = 0; i < 10; i++) {
        LootDrop drops[MAX_LOOT_DROPS];
        size_t drop_count = 0;

        generate_loot(db, "TEST", drops, &drop_count);

        // 0% chance should never generate
        ASSERT(drop_count == 0);
    }

    loot_db_free(db);
    return 0;
}

static int test_loot_complex_scenario()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group COMMON 2\n"
        "  item 1001 1 1 weight 60\n"
        "  item 1002 1 2 weight 40\n"
        "group RARE 1\n"
        "  item 2001 1 1 weight 100\n"
        "group GOLD 1\n"
        "  cp 50 150 weight 100\n"
        "table GLOBAL_BASE\n"
        "  use_group GOLD 100\n"
        "  mul_cp 150\n"
        "table CREATURE parent GLOBAL_BASE\n"
        "  use_group COMMON 100\n"
        "  add_cp 20 25 75\n"
        "table BOSS parent CREATURE\n"
        "  use_group RARE 50\n"
        "  mul_all_chances 150\n";

    parse_test_loot(db, config);
    resolve_all_loot_tables(db);

    ASSERT(db->group_count == 3);
    ASSERT(db->table_count == 3);

    // Verify BOSS table inherited all operations
    LootTable* boss = &db->tables[2];
    ASSERT(boss->resolved_op_count >= 5);

    LootDrop drops[MAX_LOOT_DROPS];
    size_t drop_count = 0;

    generate_loot(db, "BOSS", drops, &drop_count);

    // Should generate something
    ASSERT(drop_count >= 1);

    loot_db_free(db);
    return 0;
}

// ============================================================================
// Persistence Tests
// ============================================================================

static int test_loot_save_load_roundtrip()
{
    LootDB* db = test_create_db();

    const char* config = 
        "group PERSIST_TEST 2\n"
        "  item 5001 1 1 weight 40\n"
        "  cp 60 10 weight 20\n"
        "table PERSIST_TABLE\n"
        "  use_group PERSIST_TEST 100\n"
        "  add_cp 50 5 10\n"
        "#ENDLOOT\n";

    parse_test_loot(db, config);

    // Save to a temp file
    FILE* fp = tmpfile();
    ASSERT(fp != NULL);

    // Manually write the section (simulating save_loot_section)
    fprintf(fp, "#LOOT\n");
    for (int i = 0; i < db->group_count; i++) {
        LootGroup* g = &db->groups[i];
        fprintf(fp, "group %s %d\n", g->name, g->rolls);
        for (int j = 0; j < g->entry_count; j++) {
            LootEntry* e = &g->entries[j];
            if (e->type == LOOT_ITEM) {
                fprintf(fp, "  item %"PRVNUM" %d %d weight %d\n", 
                    e->item_vnum, e->min_qty, e->max_qty, e->weight);
            }
            else if (e->type == LOOT_CP) {
                fprintf(fp, "  cp %d %d weight %d\n", 
                    e->min_qty, e->max_qty, e->weight);
            }
        }
    }
    for (int i = 0; i < db->table_count; i++) {
        LootTable* t = &db->tables[i];
        if (t->parent_name) {
            fprintf(fp, "table %s : %s\n", t->name, t->parent_name);
        }
        else {
            fprintf(fp, "table %s\n", t->name);
        }
        for (int j = 0; j < t->op_count; j++) {
            LootOp* op = &t->ops[j];
            switch (op->type) {
            case LOOT_OP_USE_GROUP:
                fprintf(fp, "  use_group %s %d\n", op->group_name, op->a);
                break;
            case LOOT_OP_ADD_CP:
                fprintf(fp, "  add_cp %d %d %d\n", op->a, op->b, op->c);
                break;
            default:
                break;
            }
        }
    }
    fprintf(fp, "#ENDLOOT\n");

    // Rewind and read it back
    rewind(fp);
    LootDB* db2 = test_create_db();
    
    StringBuffer* sb = sb_new();
    char line[1024];
    bool in_loot = false;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "#LOOT", 5) == 0) {
            in_loot = true;
            continue;
        }
        if (strncmp(line, "#ENDLOOT", 8) == 0) {
            break;
        }
        if (in_loot) {
            sb_append(sb, line);
        }
    }
    
    parse_loot_section(db2, sb, NULL);
    sb_free(sb);
    fclose(fp);

    // Verify the loaded data matches
    ASSERT(db2->group_count == 1);
    ASSERT(!strcmp(db2->groups[0].name, "PERSIST_TEST"));
    ASSERT(db2->groups[0].rolls == 2);
    ASSERT(db2->groups[0].entry_count == 2);
    
    ASSERT(db2->table_count == 1);
    ASSERT(!strcmp(db2->tables[0].name, "PERSIST_TABLE"));
    ASSERT(db2->tables[0].op_count == 2);

    loot_db_free(db);
    loot_db_free(db2);
    return 0;
}

// ============================================================================
// Test Registration
// ============================================================================

void register_loot_tests()
{
#define REGISTER(name, func) register_test(&loot_tests, name, func)

    init_test_group(&loot_tests, "LOOT TESTS");
    register_test_group(&loot_tests);

    // Group tests
    REGISTER("Group: Basic Creation", test_loot_group_creation);
    REGISTER("Group: With CP Entries", test_loot_group_with_cp);
    REGISTER("Group: Multiple Rolls", test_loot_group_multiple_rolls);
    REGISTER("Group: Duplicate Merge", test_loot_duplicate_group_merge);

    // Table and Operation tests
    REGISTER("Table: Creation", test_loot_table_creation);
    REGISTER("Table: With Parent", test_loot_table_with_parent);
    REGISTER("Op: ADD_CP", test_loot_op_add_cp);
    REGISTER("Op: MUL_CP", test_loot_op_mul_cp);
    REGISTER("Op: MUL_ALL_CHANCES", test_loot_op_mul_all_chances);
    REGISTER("Op: REMOVE_ITEM", test_loot_op_remove_item);
    REGISTER("Op: REMOVE_GROUP", test_loot_op_remove_group);

    // Inheritance and Resolution tests
    REGISTER("Inheritance: Single Level", test_loot_inheritance_single_level);
    REGISTER("Inheritance: Multi Level", test_loot_inheritance_multi_level);
    REGISTER("Inheritance: No Parent", test_loot_table_without_parent);
    REGISTER("Inheritance: Cycle Detection", test_loot_cycle_detection);

    // Loot Generation tests
    REGISTER("Generate: Simple Group", test_generate_loot_simple_group);
    REGISTER("Generate: Multi Roll", test_generate_loot_multi_roll);
    REGISTER("Generate: With CP", test_generate_loot_with_cp);
    REGISTER("Generate: Direct ADD_ITEM", test_generate_loot_direct_add_item);
    REGISTER("Generate: Direct ADD_CP", test_generate_loot_direct_add_cp);
    REGISTER("Generate: CP Multiplier", test_generate_loot_cp_multiplier);
    REGISTER("Generate: Chance Multiplier", test_generate_loot_chance_multiplier);
    REGISTER("Generate: Remove Item", test_generate_loot_remove_item);
    REGISTER("Generate: Remove Group", test_generate_loot_remove_group);
    REGISTER("Generate: Drop Merging", test_generate_loot_drop_merging);

    // Edge cases
    REGISTER("Edge: Empty Group", test_generate_loot_empty_group);
    REGISTER("Edge: Invalid Table", test_generate_loot_invalid_table);
    REGISTER("Edge: Zero Weight Item", test_loot_group_with_zero_weight);
    REGISTER("Edge: Zero Chance", test_loot_zero_chance_add_item);
    REGISTER("Edge: Complex Scenario", test_loot_complex_scenario);

    // Persistence
    REGISTER("Persist: Save/Load Roundtrip", test_loot_save_load_roundtrip);

#undef REGISTER
}
