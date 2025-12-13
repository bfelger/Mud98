////////////////////////////////////////////////////////////////////////////////
// tests/mem_watchpoint_tests.c
// Unit tests for memory watchpoint debugging facility
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <mem_watchpoint.h>
#include <merc.h>

#include <string.h>
#include <stdio.h>

// Test basic watchpoint addition
static int test_watchpoint_add()
{
    mem_watch_clear();
    
    int test_var = 42;
    mem_watch_add(&test_var, sizeof(test_var), "test_var");
    
    // Check should pass (no changes)
    ASSERT(mem_watch_check() == 0);
    
    mem_watch_clear();
    return 0;
}

// Test detecting simple change
static int test_watchpoint_detect_change()
{
    mem_watch_clear();
    
    int test_var = 100;
    mem_watch_add(&test_var, sizeof(test_var), "test_var");
    
    // No change yet
    ASSERT(mem_watch_check() == 0);
    
    // Change the value
    test_var = 200;
    
    // Should detect the change
    ASSERT(mem_watch_check() > 0);
    
    mem_watch_clear();
    return 0;
}

// Test multiple watchpoints
static int test_watchpoint_multiple()
{
    mem_watch_clear();
    
    int var1 = 10;
    int var2 = 20;
    int var3 = 30;
    
    ASSERT(mem_watch_add(&var1, sizeof(var1), "var1"));
    ASSERT(mem_watch_add(&var2, sizeof(var2), "var2"));
    ASSERT(mem_watch_add(&var3, sizeof(var3), "var3"));
    
    // All unchanged
    ASSERT(mem_watch_check() == 0);
    
    // Change middle variable
    var2 = 999;
    
    // Should detect change in var2
    ASSERT(mem_watch_check() > 0);
    
    mem_watch_clear();
    return 0;
}

// Test watchpoint on struct
static int test_watchpoint_struct()
{
    mem_watch_clear();
    
    typedef struct {
        int x;
        int y;
        char name[32];
    } TestStruct;
    
    TestStruct ts;
    ts.x = 100;
    ts.y = 200;
    strcpy(ts.name, "original");
    
    WATCH_STRUCT(&ts, TestStruct);
    
    // No changes
    ASSERT(mem_watch_check() == 0);
    
    // Modify a field
    ts.x = 999;
    
    // Should detect change
    ASSERT(mem_watch_check() > 0);
    
    mem_watch_clear();
    return 0;
}

// Test watchpoint on array
static int test_watchpoint_array()
{
    mem_watch_clear();
    
    int arr[10];
    for (int i = 0; i < 10; i++) {
        arr[i] = i * 10;
    }
    
    mem_watch_add(arr, sizeof(arr), "test_array");
    
    // Unchanged
    ASSERT(mem_watch_check() == 0);
    
    // Modify one element
    arr[5] = 9999;
    
    // Should detect
    ASSERT(mem_watch_check() > 0);
    
    mem_watch_clear();
    return 0;
}

// Test watchpoint on string
static int test_watchpoint_string()
{
    mem_watch_clear();
    
    char buffer[64];
    strcpy(buffer, "Hello World");
    
    mem_watch_add(buffer, sizeof(buffer), "string_buffer");
    
    // Unchanged
    ASSERT(mem_watch_check() == 0);
    
    // Modify string
    strcpy(buffer, "Modified");
    
    // Should detect
    ASSERT(mem_watch_check() > 0);
    
    mem_watch_clear();
    return 0;
}

// Test maximum watchpoints
static int test_watchpoint_max()
{
    mem_watch_clear();
    
    #define TEST_VARS 35
    int vars[TEST_VARS];
    
    // Try to add more than MAX_WATCHPOINTS (32)
    int added = 0;
    for (int i = 0; i < TEST_VARS; i++) {
        vars[i] = i;
        char label[32];
        sprintf(label, "var_%d", i);
        if (mem_watch_add(&vars[i], sizeof(int), label)) {
            added++;
        }
    }
    
    // Should have added MAX_WATCHPOINTS, then failed
    ASSERT(added == MAX_WATCHPOINTS);
    
    mem_watch_clear();
    #undef TEST_VARS
    return 0;
}

// Test clearing watchpoints
static int test_watchpoint_clear()
{
    mem_watch_clear();
    
    int var1 = 10;
    int var2 = 20;
    
    ASSERT(mem_watch_add(&var1, sizeof(var1), "var1"));
    ASSERT(mem_watch_add(&var2, sizeof(var2), "var2"));
    
    mem_watch_clear();
    
    // After clear, should be able to add more watchpoints
    // (tests that count was reset)
    for (int i = 0; i < 10; i++) {
        char label[32];
        sprintf(label, "new_var_%d", i);
        ASSERT(mem_watch_add(&var1, sizeof(var1), label));
        mem_watch_clear();
    }
    
    return 0;
}

// Test partial memory corruption detection
static int test_watchpoint_partial_corruption()
{
    mem_watch_clear();
    
    char buffer[100];
    memset(buffer, 'A', 100);
    
    mem_watch_add(buffer, 100, "buffer");
    
    // Corrupt just the middle
    buffer[50] = 'X';
    
    // Should detect even single byte change
    ASSERT(mem_watch_check() > 0);
    
    mem_watch_clear();
    return 0;
}

// Test watchpoint survives benign access
static int test_watchpoint_read_access()
{
    mem_watch_clear();
    
    int value = 12345;
    mem_watch_add(&value, sizeof(value), "value");
    
    // Read the value multiple times (shouldn't affect watchpoint)
    volatile int read1 = value;
    volatile int read2 = value;
    (void)read1;
    (void)read2;
    
    // Should still pass
    ASSERT(mem_watch_check() == 0);
    
    mem_watch_clear();
    return 0;
}

// Test detecting off-by-one writes
static int test_watchpoint_boundary_write()
{
    mem_watch_clear();
    
    // Create buffer with guard bytes
    char buffer[10];
    char guard_before = 'G';
    char guard_after = 'G';
    
    memset(buffer, 'A', 10);
    
    // Watch the guards
    mem_watch_add(&guard_before, 1, "guard_before");
    mem_watch_add(&guard_after, 1, "guard_after");
    
    // Normal writes to buffer shouldn't trigger
    buffer[0] = 'X';
    buffer[9] = 'Y';
    ASSERT(mem_watch_check() == 0);
    
    // But corrupting a guard should trigger
    guard_after = 'X';
    ASSERT(mem_watch_check() > 0);
    
    mem_watch_clear();
    return 0;
}

// Test WATCH_VAR macro
static int test_watchpoint_macro_var()
{
    mem_watch_clear();
    
    int test_value = 999;
    WATCH_VAR(test_value);
    
    ASSERT(mem_watch_check() == 0);
    
    test_value = 111;
    ASSERT(mem_watch_check() > 0);
    
    mem_watch_clear();
    return 0;
}

// Simulate the mob prototype corruption scenario
static int test_watchpoint_mob_scenario()
{
    mem_watch_clear();
    
    // Simulate a "mob prototype" structure
    typedef struct {
        int vnum;
        char name[32];
        int level;
        int type;  // The field that was getting corrupted
    } MockMobProto;
    
    MockMobProto mob1 = {6301, "spider", 10, 1};
    MockMobProto mob2 = {6306, "guard", 15, 1};
    
    // Watch both mobs
    mem_watch_add(&mob1, sizeof(mob1), "mob_6301");
    mem_watch_add(&mob2, sizeof(mob2), "mob_6306");
    
    // Should be clean initially
    ASSERT(mem_watch_check() == 0);
    
    // Simulate corruption from buffer overflow
    mob1.type = 0x7c20203a;  // Garbage value like we saw
    mob2.type = 0x30323033;
    
    // Should detect corruption
    ASSERT(mem_watch_check() > 0);
    
    mem_watch_clear();
    return 0;
}

static TestGroup mem_watchpoint_tests_group;

void register_mem_watchpoint_tests()
{
#define REGISTER(name, fn) register_test(&mem_watchpoint_tests_group, (name), (fn))

    init_test_group(&mem_watchpoint_tests_group, "MEMORY WATCHPOINT TESTS");
    register_test_group(&mem_watchpoint_tests_group);
    
    REGISTER("add watchpoint", test_watchpoint_add);
    REGISTER("detect simple change", test_watchpoint_detect_change);
    REGISTER("multiple watchpoints", test_watchpoint_multiple);
    REGISTER("watchpoint on struct", test_watchpoint_struct);
    REGISTER("watchpoint on array", test_watchpoint_array);
    REGISTER("watchpoint on string", test_watchpoint_string);
    REGISTER("maximum watchpoints", test_watchpoint_max);
    REGISTER("clear watchpoints", test_watchpoint_clear);
    REGISTER("partial corruption detection", test_watchpoint_partial_corruption);
    REGISTER("read access doesn't trigger", test_watchpoint_read_access);
    REGISTER("boundary write detection", test_watchpoint_boundary_write);
    REGISTER("WATCH_VAR macro", test_watchpoint_macro_var);
    REGISTER("mob prototype corruption scenario", test_watchpoint_mob_scenario);

#undef REGISTER
}
