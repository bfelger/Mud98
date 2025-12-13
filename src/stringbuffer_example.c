////////////////////////////////////////////////////////////////////////////////
// stringbuffer_example.c - Example usage of StringBuffer
//
// This shows the intended usage patterns for StringBuffer.
// You can adapt these patterns for cmdedit.
////////////////////////////////////////////////////////////////////////////////

#include "stringbuffer.h"
#include <stdio.h>

void example_basic_usage(void)
{
    printf("\n=== Basic Usage ===\n");
    
    StringBuffer* sb = sb_new();
    
    sb_append(sb, "Hello, ");
    sb_append(sb, "World!");
    
    printf("Result: %s\n", SB(sb));
    printf("Length: %zu, Capacity: %zu\n", sb_length(sb), sb_capacity(sb));
    
    sb_free(sb);
}

void example_formatted_output(void)
{
    printf("\n=== Formatted Output ===\n");
    
    StringBuffer* sb = sb_new();
    
    sb_appendf(sb, "%-20s %5s %10s\n", "Name", "Level", "Experience");
    sb_appendf(sb, "%-20s %5d %10d\n", "Player One", 50, 1234567);
    sb_appendf(sb, "%-20s %5d %10d\n", "Another Player", 42, 987654);
    
    printf("%s", SB(sb));
    
    sb_free(sb);
}

void example_building_large_list(void)
{
    printf("\n=== Building Large List ===\n");
    
    StringBuffer* sb = sb_new();
    
    sb_append(sb, "Command List:\n");
    sb_append(sb, "-------------\n");
    
    // Simulate adding 100 commands
    for (int i = 0; i < 100; i++) {
        sb_appendf(sb, "%-20s Level %2d - Description of command %d\n",
                   "command_name", i % 51, i);
    }
    
    printf("Built list with %zu characters in buffer of capacity %zu\n",
           sb_length(sb), sb_capacity(sb));
    printf("Growth count: %d, Total bytes grown: %zu\n",
           sb_stats.growth_count, sb_stats.bytes_grown);
    
    // Show first 200 chars
    char preview[201];
    strncpy(preview, SB(sb), 200);
    preview[200] = '\0';
    printf("\nFirst 200 chars:\n%s...\n", preview);
    
    sb_free(sb);
}

void example_clear_and_reuse(void)
{
    printf("\n=== Clear and Reuse ===\n");
    
    StringBuffer* sb = sb_new();
    
    sb_append(sb, "First message");
    printf("Message 1: %s (len=%zu)\n", SB(sb), sb_length(sb));
    
    sb_clear(sb);
    sb_append(sb, "Second message - buffer was reused!");
    printf("Message 2: %s (len=%zu, cap=%zu)\n", 
           SB(sb), sb_length(sb), sb_capacity(sb));
    
    sb_free(sb);
}

void example_performance_vs_strcat(void)
{
    printf("\n=== Performance Pattern ===\n");
    
    sb_reset_stats();
    
    StringBuffer* sb = sb_new();
    
    // Fast appends - O(1) because we track length
    for (int i = 0; i < 1000; i++) {
        sb_append(sb, "x");
    }
    
    printf("Appended 1000 chars\n");
    printf("  Length: %zu\n", sb_length(sb));
    printf("  Growth count: %d\n", sb_stats.growth_count);
    printf("  Capacity: %zu\n", sb_capacity(sb));
    
    // Key advantage: no strlen() calls in loops!
    // Compare to: strlen(buffer) + strlen(str) on every append
    
    sb_free(sb);
}

void example_cmdedit_pattern(void)
{
    printf("\n=== CMDEdit Pattern ===\n");
    
    StringBuffer* output = sb_new();
    
    // Header
    sb_append(output, "Command Editor\n");
    sb_append(output, "==============\n\n");
    
    // Simulate command list
    const char* commands[] = {"north", "south", "look", "quit", "help"};
    const char* categories[] = {"Movement", "Movement", "Info", "System", "Info"};
    
    for (int i = 0; i < 5; i++) {
        sb_appendf(output, "%-15s %-10s Lvl %2d\n",
                   commands[i], categories[i], i * 10);
    }
    
    // Footer
    sb_append(output, "\nTotal commands: 5\n");
    
    // In real code, you'd do: page_to_char(SB(output), ch);
    printf("%s", SB(output));
    
    sb_free(output);
}

int main(void)
{
    printf("StringBuffer Examples\n");
    printf("=====================\n");
    
    example_basic_usage();
    example_formatted_output();
    example_clear_and_reuse();
    example_building_large_list();
    example_performance_vs_strcat();
    example_cmdedit_pattern();
    
    printf("\n=== Final Statistics ===\n");
    printf("Created: %d, Freed: %d, Recycled: %d\n",
           sb_stats.sb_created, sb_stats.sb_freed, sb_stats.sb_recycled);
    
    return 0;
}
