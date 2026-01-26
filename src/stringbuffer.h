////////////////////////////////////////////////////////////////////////////////
// stringbuffer.h - Fast string building with tracked length and tail pointer
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__STRINGBUFFER_H
#define MUD98__STRINGBUFFER_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// StringBuffer - Efficient string concatenation buffer
//
// Design goals:
// 1. Track current length for O(1) append (no strlen() calls)
// 2. Use size buckets from alloc_mem for efficient memory pooling
// 3. Grow intelligently without debug logging/allocation side effects
// 4. Simple API: create, append, appendf, clear, free
//
// Usage:
//   StringBuffer* sb = sb_new(void);
//   sb_append(sb, "Hello ");
//   sb_appendf(sb, "world %d", 42);
//   printf("%s\n", sb_string(sb));  // or SB(sb) macro
//   sb_free(sb);

typedef struct string_buffer_t StringBuffer;

struct string_buffer_t {
    char*  data;        // Allocated string buffer
    size_t length;      // Current string length (not including null terminator)
    size_t capacity;    // Total allocated capacity (including space for null)
    StringBuffer* next; // For free list recycling
};

// Create a new string buffer with default capacity
StringBuffer* sb_new(void);

// Create a new string buffer with specific initial capacity hint
StringBuffer* sb_new_size(size_t capacity_hint);

// Append a null-terminated string
// Returns false only on catastrophic failure (overflow)
bool sb_append(StringBuffer* sb, const char* str);

// Append formatted string (printf-style)
// Returns false only on catastrophic failure (overflow)
bool sb_appendf(StringBuffer* sb, const char* fmt, ...);

// Append a single character
bool sb_append_char(StringBuffer* sb, char c);

// Append exactly n bytes from str (doesn't need to be null-terminated)
bool sb_append_n(StringBuffer* sb, const char* str, size_t n);

// Clear the buffer (reset length to 0, keep capacity)
void sb_clear(StringBuffer* sb);

// Get the current string (null-terminated)
const char* sb_string(const StringBuffer* sb);

// Get the current length (number of characters, not counting null terminator)
size_t sb_length(const StringBuffer* sb);

// Get the current capacity
size_t sb_capacity(const StringBuffer* sb);

// Free the buffer and return it to the free list
void sb_free(StringBuffer* sb);

// Note: The SB() macro has been removed due to conflict with telnet.h
// Use sb_string(sb) to access the underlying string data instead

// Statistics (for debugging/profiling)
typedef struct {
    int sb_created;     // Total StringBuffers allocated
    int sb_freed;       // Total StringBuffers freed
    int sb_recycled;    // Number reused from free list
    int growth_count;   // Number of times buffers had to grow
    size_t bytes_grown; // Total bytes allocated due to growth
} StringBufferStats;

extern StringBufferStats sb_stats;

// Reset statistics
void sb_reset_stats(void);

#endif // MUD98__STRINGBUFFER_H
