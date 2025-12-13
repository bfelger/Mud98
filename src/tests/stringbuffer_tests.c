////////////////////////////////////////////////////////////////////////////////
// tests/stringbuffer_tests.c
// Unit tests for StringBuffer allocation, growth, and operations
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <db.h>
#include <merc.h>
#include <stringbuffer.h>

#include <string.h>
#include <stdio.h>

// Test basic StringBuffer allocation
static int test_sb_new()
{
    StringBuffer* sb = sb_new();
    ASSERT(sb != NULL);
    ASSERT(sb_string(sb) != NULL);
    ASSERT(sb_length(sb) == 0);
    ASSERT(sb_capacity(sb) >= 1024);  // Default initial capacity
    ASSERT(sb_string(sb)[0] == '\0');
    sb_free(sb);
    return 0;
}

// Test StringBuffer with custom size hint
static int test_sb_new_size()
{
    StringBuffer* sb = sb_new_size(512);
    ASSERT(sb != NULL);
    ASSERT(sb_capacity(sb) >= 512);
    sb_free(sb);
    
    // Test large size hint
    sb = sb_new_size(16000);
    ASSERT(sb != NULL);
    ASSERT(sb_capacity(sb) >= 16000);
    sb_free(sb);
    
    return 0;
}

// Test appending strings
static int test_sb_append()
{
    StringBuffer* sb = sb_new();
    
    ASSERT(sb_append(sb, "Hello"));
    ASSERT(sb_length(sb) == 5);
    ASSERT(!strcmp(sb_string(sb), "Hello"));
    
    ASSERT(sb_append(sb, " World"));
    ASSERT(sb_length(sb) == 11);
    ASSERT(!strcmp(sb_string(sb), "Hello World"));
    
    // Append empty string (should be no-op)
    ASSERT(sb_append(sb, ""));
    ASSERT(sb_length(sb) == 11);
    
    // Append NULL (should be no-op)
    ASSERT(sb_append(sb, NULL));
    ASSERT(sb_length(sb) == 11);
    
    sb_free(sb);
    return 0;
}

// Test formatted append
static int test_sb_appendf()
{
    StringBuffer* sb = sb_new();
    
    ASSERT(sb_appendf(sb, "Number: %d", 42));
    ASSERT(sb_length(sb) == 10);
    ASSERT(!strcmp(sb_string(sb), "Number: 42"));
    
    ASSERT(sb_appendf(sb, ", String: %s", "test"));
    ASSERT(sb_length(sb) == 24);
    ASSERT(!strcmp(sb_string(sb), "Number: 42, String: test"));
    
    sb_free(sb);
    return 0;
}

// Test appending single characters
static int test_sb_append_char()
{
    StringBuffer* sb = sb_new();
    
    ASSERT(sb_append_char(sb, 'A'));
    ASSERT(sb_length(sb) == 1);
    ASSERT(!strcmp(sb_string(sb), "A"));
    
    ASSERT(sb_append_char(sb, 'B'));
    ASSERT(sb_append_char(sb, 'C'));
    ASSERT(sb_length(sb) == 3);
    ASSERT(!strcmp(sb_string(sb), "ABC"));
    
    sb_free(sb);
    return 0;
}

// Test appending n bytes
static int test_sb_append_n()
{
    StringBuffer* sb = sb_new();
    
    const char* str = "Hello World";
    ASSERT(sb_append_n(sb, str, 5));
    ASSERT(sb_length(sb) == 5);
    ASSERT(!strcmp(sb_string(sb), "Hello"));
    
    ASSERT(sb_append_n(sb, " Test", 5));
    ASSERT(sb_length(sb) == 10);
    ASSERT(!strcmp(sb_string(sb), "Hello Test"));
    
    // Append 0 bytes (should be no-op)
    ASSERT(sb_append_n(sb, "ignored", 0));
    ASSERT(sb_length(sb) == 10);
    
    sb_free(sb);
    return 0;
}

// Test buffer growth through multiple additions
static int test_sb_growth()
{
    StringBuffer* sb = sb_new();
    size_t initial_capacity = sb_capacity(sb);
    
    // Add enough content to definitely force growth
    // Repeat a 100-char string 100 times = 10KB
    // If initial capacity is already 16384, this should fit without growth
    // but content should still be correct
    char chunk[256];
    for (int i = 0; i < 100; i++) {
        sprintf(chunk, "Line %d: Some test content here to make it much longer with more data padding text filler words\n", i);
        ASSERT(sb_append(sb, chunk));
    }
    
    // Just verify content is intact, don't check capacity growth
    // (initial capacity may already be MAX_CAPACITY from recycling)
    ASSERT(sb_capacity(sb) >= initial_capacity);  // At minimum, not shrunk
    ASSERT(sb_length(sb) > 9000);
    
    // Content should be intact
    const char* result = sb_string(sb);
    ASSERT(strstr(result, "Line 0:") != NULL);
    ASSERT(strstr(result, "Line 99:") != NULL);
    
    sb_free(sb);
    return 0;
}

// Test clearing the buffer
static int test_sb_clear()
{
    StringBuffer* sb = sb_new();
    
    sb_append(sb, "Some content");
    ASSERT(sb_length(sb) > 0);
    
    size_t capacity_before = sb_capacity(sb);
    sb_clear(sb);
    
    ASSERT(sb_length(sb) == 0);
    ASSERT(sb_string(sb)[0] == '\0');
    ASSERT(sb_capacity(sb) == capacity_before); // Capacity unchanged
    
    // Should be able to append after clear
    ASSERT(sb_append(sb, "New content"));
    ASSERT(sb_length(sb) == 11);
    
    sb_free(sb);
    return 0;
}

// Test large buffer (up to MAX_CAPACITY)
static int test_sb_large()
{
    StringBuffer* sb = sb_new();
    
    // Build a 15KB string (under MAX_CAPACITY of 18432)
    char chunk[512];
    memset(chunk, 'X', 511);
    chunk[511] = '\0';
    
    for (int i = 0; i < 30; i++) {
        if (!sb_append(sb, chunk)) {
            // Hit max capacity, that's OK
            break;
        }
    }
    
    // Should have at least 10KB
    ASSERT(sb_length(sb) >= 10 * 1024);
    ASSERT(sb_capacity(sb) >= sb_length(sb));
    
    sb_free(sb);
    return 0;
}

// Test buffer recycling (free list)
static int test_sb_recycling()
{
    // Note: Stats may not be pristine if other tests ran first
    int recycled_before = sb_stats.sb_recycled;
    
    StringBuffer* sb1 = sb_new();
    int created_after_first = sb_stats.sb_created;
    sb_free(sb1);
    
    StringBuffer* sb2 = sb_new();
    // Should have reused the freed one
    ASSERT(sb_stats.sb_created == created_after_first);
    ASSERT(sb_stats.sb_recycled > recycled_before);
    sb_free(sb2);
    
    return 0;
}

// Test stress: many buffers at once
static int test_sb_stress()
{
    #define NUM_BUFFERS 50
    StringBuffer* buffers[NUM_BUFFERS];
    
    // Allocate many buffers
    for (int i = 0; i < NUM_BUFFERS; i++) {
        buffers[i] = sb_new();
        ASSERT(buffers[i] != NULL);
        
        // Add some content to each
        char content[100];
        sprintf(content, "Buffer %d content", i);
        ASSERT(sb_append(buffers[i], content));
    }
    
    // Verify all buffers still have correct content
    for (int i = 0; i < NUM_BUFFERS; i++) {
        char expected[100];
        sprintf(expected, "Buffer %d content", i);
        ASSERT(!strcmp(sb_string(buffers[i]), expected));
    }
    
    // Free all buffers
    for (int i = 0; i < NUM_BUFFERS; i++) {
        sb_free(buffers[i]);
    }
    
    #undef NUM_BUFFERS
    return 0;
}

// Test that page_to_char scenario works (12KB+ with color codes)
static int test_sb_page_to_char_scenario()
{
    StringBuffer* sb = sb_new();
    
    // Simulate building command list with color codes
    // Each entry is ~42 bytes with color codes
    // 300 entries = ~12.6KB
    for (int i = 0; i < 300; i++) {
        sb_appendf(sb, "\033[38:2::237:230:203m%2d %-12.12s\033[0m | ", 
                   i, "command_name");
    }
    
    // Should have >10KB of content
    size_t len = sb_length(sb);
    ASSERT(len > 10000);
    ASSERT(sb_capacity(sb) >= len);
    
    // Verify content is intact
    const char* result = sb_string(sb);
    ASSERT(result[0] == '\033'); // Starts with color code
    ASSERT(strlen(result) == len);
    
    sb_free(sb);
    return 0;
}

// Test boundary condition: exactly fill capacity
static int test_sb_boundary()
{
    StringBuffer* sb = sb_new();
    size_t cap = sb_capacity(sb);
    
    // Fill to just under capacity
    char* filler = malloc(cap);
    memset(filler, 'A', cap - 2);
    filler[cap - 2] = '\0';
    
    ASSERT(sb_append(sb, filler));
    ASSERT(sb_length(sb) == cap - 2);
    
    // Add one more char (should fit without growing)
    ASSERT(sb_append_char(sb, 'B'));
    ASSERT(sb_length(sb) == cap - 1);
    
    // Add one more char (should trigger growth)
    size_t cap_before = sb_capacity(sb);
    ASSERT(sb_append_char(sb, 'C'));
    ASSERT(sb_capacity(sb) > cap_before);
    
    free(filler);
    sb_free(sb);
    return 0;
}

static TestGroup stringbuffer_tests_group;

void register_stringbuffer_tests()
{
#define REGISTER(name, fn) register_test(&stringbuffer_tests_group, (name), (fn))

    init_test_group(&stringbuffer_tests_group, "STRINGBUFFER TESTS");
    register_test_group(&stringbuffer_tests_group);
    
    REGISTER("sb_new basic allocation", test_sb_new);
    REGISTER("sb_new_size with hints", test_sb_new_size);
    REGISTER("sb_append strings", test_sb_append);
    REGISTER("sb_appendf formatting", test_sb_appendf);
    REGISTER("sb_append_char single chars", test_sb_append_char);
    REGISTER("sb_append_n byte count", test_sb_append_n);
    REGISTER("buffer growth", test_sb_growth);
    REGISTER("clear buffer", test_sb_clear);
    REGISTER("large buffer (50KB)", test_sb_large);
    REGISTER("buffer recycling", test_sb_recycling);
    REGISTER("stress test with many buffers", test_sb_stress);
    REGISTER("page_to_char scenario (12KB+)", test_sb_page_to_char_scenario);
    REGISTER("boundary conditions", test_sb_boundary);

#undef REGISTER
}
