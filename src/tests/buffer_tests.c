////////////////////////////////////////////////////////////////////////////////
// tests/buffer_tests.c
// Unit tests for Buffer allocation and growth
////////////////////////////////////////////////////////////////////////////////

#include "tests.h"
#include "test_registry.h"
#include "mock.h"

#include <db.h>
#include <recycle.h>
#include <merc.h>

#include <string.h>
#include <stdio.h>

// Test basic buffer allocation
static int test_buffer_new()
{
    Buffer* buf = new_buf();
    ASSERT(buf != NULL);
    ASSERT(buf->string != NULL);
    ASSERT(buf->size >= BASE_BUF);
    ASSERT(buf->state == BUFFER_SAFE);
    ASSERT(strlen(buf->string) == 0);
    free_buf(buf);


    return 0;
}


// Test buffer growth through multiple additions
static int test_buffer_growth()
{
    Buffer* buf = new_buf();
    size_t initial_size = buf->size;
    
    // Add strings until buffer needs to grow (initial size is 1024)
    // Each line is ~50 bytes, so 25 lines * 50 = 1250 bytes > 1024
    char chunk[100];
    for (int i = 0; i < 25; i++) {
        sprintf(chunk, "Line %d: Some test content here to make it longer\n", i);
        ASSERT(add_buf(buf, chunk));
    }
    
    // Buffer should have grown
    ASSERT(buf->size > initial_size);
    ASSERT(buf->state == BUFFER_SAFE);
    
    // Verify content is intact
    ASSERT(strstr(buf->string, "Line 0:") != NULL);
    ASSERT(strstr(buf->string, "Line 19:") != NULL);
    
    free_buf(buf);

    return 0;
}


// Test large buffer allocation
static int test_buffer_large()
{
    Buffer* buf = new_buf_size(MAX_STRING_LENGTH);
    ASSERT(buf != NULL);
    ASSERT(buf->size >= MAX_STRING_LENGTH);
    
    // Fill with large content
    for (int i = 0; i < 100; i++) {
        ASSERT(add_buf(buf, "0123456789"));
    }
    
    ASSERT(buf->state == BUFFER_SAFE);
    ASSERT(strlen(buf->string) == 1000);
    
    free_buf(buf);

    return 0;
}


// Test buffer boundary conditions
static int test_buffer_boundaries()
{
    Buffer* buf = new_buf();
    
    // Add content that exactly fills current size
    size_t space = buf->size - 1; // -1 for null terminator
    char* filler = malloc(space + 1);
    memset(filler, 'x', space);
    filler[space] = '\0';
    
    ASSERT(add_buf(buf, filler));
    ASSERT(strlen(buf->string) == space);
    
    // Next add should trigger growth
    size_t old_size = buf->size;
    ASSERT(add_buf(buf, "y"));
    ASSERT(buf->size > old_size);
    ASSERT(buf->string[space] == 'y');
    
    free(filler);
    free_buf(buf);

    return 0;
}


// Test addf_buf with formatting
static int test_buffer_printf()
{
    Buffer* buf = new_buf();
    
    ASSERT(addf_buf(buf, "Number: %d\n", 42));
    ASSERT(addf_buf(buf, "String: %s\n", "test"));
    ASSERT(addf_buf(buf, "Float: %.2f\n", 3.14));
    
    ASSERT(strstr(buf->string, "Number: 42") != NULL);
    ASSERT(strstr(buf->string, "String: test") != NULL);
    ASSERT(strstr(buf->string, "Float: 3.14") != NULL);
    
    free_buf(buf);

    return 0;
}


// Test clear_buf
static int test_buffer_clear()
{
    Buffer* buf = new_buf();
    
    add_buf(buf, "Some content");
    ASSERT(strlen(buf->string) > 0);
    
    clear_buf(buf);
    ASSERT(strlen(buf->string) == 0);
    ASSERT(buf->state == BUFFER_SAFE);
    
    // Should be reusable after clear
    add_buf(buf, "New content");
    ASSERT(strcmp(buf->string, "New content") == 0);
    
    free_buf(buf);

    return 0;
}


// Test buffer recycling
static int test_buffer_recycling()
{
    Buffer* buf1 = new_buf();
    Buffer* old_ptr = buf1;
    free_buf(buf1);
    
    // Next allocation should reuse the freed buffer
    Buffer* buf2 = new_buf();
    ASSERT(buf2 == old_ptr); // Should be same pointer from free list
    
    free_buf(buf2);
    return 0;
}


// Stress test: allocate many buffers
static int test_buffer_stress()
{
    #define NUM_BUFFERS 100
    Buffer* buffers[NUM_BUFFERS];
    
    // Allocate many buffers
    for (int i = 0; i < NUM_BUFFERS; i++) {
        buffers[i] = new_buf();
        ASSERT(buffers[i] != NULL);
        
        // Add unique content to each
        char content[100];
        sprintf(content, "Buffer %d content", i);
        add_buf(buffers[i], content);
    }
    
    // Verify all buffers retained their content
    for (int i = 0; i < NUM_BUFFERS; i++) {
        char expected[100];
        sprintf(expected, "Buffer %d content", i);
        ASSERT(strcmp(buffers[i]->string, expected) == 0);
    }
    
    // Free all
    for (int i = 0; i < NUM_BUFFERS; i++) {
        free_buf(buffers[i]);
    }
    return 0;
}


// Test size bucket selection
static int test_buffer_size_buckets()
{
    // Test that get_size returns appropriate buckets
    Buffer* buf = new_buf_size(100);
    ASSERT(buf->size == 128); // Should round up to next bucket
    free_buf(buf);

    
    buf = new_buf_size(1000);
    ASSERT(buf->size == 1024);
    free_buf(buf);

    
    buf = new_buf_size(5000);
    ASSERT(buf->size == 8192);
    free_buf(buf);

    return 0;
}


// Replicate the cmdedit scenario
static int test_buffer_cmdedit_scenario()
{
    Buffer* buf = new_buf();
    
    // Simulate listing 273 commands with formatting
    // Format: 18 + 1 + 10 + 1 + 3 + 2 + "description here" (16 chars) + newline = 52 bytes
    // 273 * 52 = 14196 bytes
    for (int i = 0; i < 273; i++) {
        addf_buf(buf, "%-18s %-10s %3d  %-15s\n",
            "command_name", "category", i, "description here");
    }
    
    // Should end up at 14196 bytes (52 bytes per line * 273 lines)
    size_t final_len = strlen(buf->string);
    ASSERT(final_len > 14000);
    ASSERT(final_len < 14500);
    ASSERT(buf->state == BUFFER_SAFE);
    
    //printf("Cmdedit scenario: %zu bytes in buffer of size %zu\n", 
    //    final_len, buf->size);
    
    free_buf(buf);

    return 0;
}

static TestGroup buffer_tests_group;

void register_buffer_tests()
{
#define REGISTER(name, fn) register_test(&buffer_tests_group, (name), (fn))

    init_test_group(&buffer_tests_group, "BUFFER TESTS");
    register_test_group(&buffer_tests_group);
    
    REGISTER("new_buf basic allocation", test_buffer_new);
    REGISTER("buffer growth", test_buffer_growth);
    REGISTER("large buffer", test_buffer_large);
    REGISTER("boundary conditions", test_buffer_boundaries);
    REGISTER("addf_buf formatting", test_buffer_printf);
    REGISTER("clear_buf", test_buffer_clear);
    REGISTER("buffer recycling", test_buffer_recycling);
    REGISTER("stress test with many buffers", test_buffer_stress);
    REGISTER("size bucket selection", test_buffer_size_buckets);
    REGISTER("cmdedit list scenario", test_buffer_cmdedit_scenario);

#undef REGISTER
}


