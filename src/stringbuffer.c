////////////////////////////////////////////////////////////////////////////////
// stringbuffer.c - Fast string building with tracked length
////////////////////////////////////////////////////////////////////////////////

#include "stringbuffer.h"

#include <db.h>
#include <merc.h>

#include <stdio.h>
#include <string.h>

// Free list for recycling StringBuffer structs
static StringBuffer* sb_free_list = NULL;

// Statistics
StringBufferStats sb_stats = {0};

void sb_reset_stats(void)
{
    memset(&sb_stats, 0, sizeof(sb_stats));
}

// Size buckets - these match alloc_mem's rgSizeList for efficiency
// We want sizes that will map cleanly to alloc_mem buckets
static const size_t INITIAL_CAPACITY = 1024;
static const size_t MAX_CAPACITY = MAX_STRING_LENGTH * 4;

// Calculate next size bucket - grows by ~2x but aligns to common sizes
static size_t next_capacity(size_t current, size_t needed)
{
    // Size buckets that align with alloc_mem
    static const size_t buckets[] = {
        16, 32, 64, 128, 256, 512, 1024, 2048, 4096,
        MAX_STRING_LENGTH,      // 4608
        8192,
        MAX_STRING_LENGTH * 2,  // 9216
        16384,
        MAX_STRING_LENGTH * 4,  // 18432
        32768,
        65536
    };
    static const int num_buckets = sizeof(buckets) / sizeof(buckets[0]);

    // Find smallest bucket that fits needed size
    for (int i = 0; i < num_buckets; i++) {
        if (buckets[i] >= needed) {
            return buckets[i];
        }
    }

    // If we get here, the requested size is too large
    return SIZE_MAX;
}

StringBuffer* sb_new(void)
{
    return sb_new_size(INITIAL_CAPACITY);
}

StringBuffer* sb_new_size(size_t capacity_hint)
{
    StringBuffer* sb;

    // Try to recycle from free list
    if (sb_free_list != NULL) {
        sb = sb_free_list;
        sb_free_list = sb->next;
        sb_stats.sb_recycled++;
    }
    else {
        // Allocate new StringBuffer struct using permanent memory
        sb = (StringBuffer*)alloc_perm(sizeof(StringBuffer));
        sb_stats.sb_created++;
    }

    // Calculate actual capacity
    size_t capacity = next_capacity(0, capacity_hint + 1); // +1 for null
    if (capacity == SIZE_MAX) {
        capacity = MAX_CAPACITY;
    }

    // Allocate data buffer using memory pool
    sb->data = (char*)alloc_mem(capacity);
    sb->data[0] = '\0';
    sb->length = 0;
    sb->capacity = capacity;
    sb->next = NULL;

    return sb;
}

// Grow the buffer to accommodate at least new_length characters + null
static bool grow_buffer(StringBuffer* sb, size_t new_length)
{
    size_t needed = new_length + 1; // +1 for null terminator
    size_t new_capacity = next_capacity(sb->capacity, needed);

    if (new_capacity == SIZE_MAX || new_capacity > MAX_CAPACITY) {
        // Can't grow any more
        return false;
    }

    // Allocate new buffer
    char* new_data = (char*)alloc_mem(new_capacity);
    
    // Copy existing content
    if (sb->length > 0) {
        memcpy(new_data, sb->data, sb->length);
    }
    new_data[sb->length] = '\0';

    // Free old buffer and switch to new one
    free_mem(sb->data, sb->capacity);
    sb->data = new_data;
    sb->capacity = new_capacity;

    // Update statistics
    sb_stats.growth_count++;
    sb_stats.bytes_grown += new_capacity;

    return true;
}

bool sb_append(StringBuffer* sb, const char* str)
{
    if (str == NULL || str[0] == '\0') {
        return true; // Nothing to append
    }

    size_t str_len = strlen(str);
    size_t new_length = sb->length + str_len;

    // Check if we need to grow
    if (new_length + 1 > sb->capacity) {
        if (!grow_buffer(sb, new_length)) {
            return false; // Growth failed
        }
        // Verify we actually have enough space after growing
        if (new_length + 1 > sb->capacity) {
            return false; // Still not enough space
        }
    }

    // Fast append using memcpy - we know the target location
    // Double-check we have space (should never happen if grow worked)
    if (new_length >= sb->capacity) {
        printf("OVERFLOW: new_length=%zu capacity=%zu str_len=%zu\n", 
               new_length, sb->capacity, str_len);
        fflush(stdout);
        bug("StringBuffer overflow prevented!");
        return false;
    }
    
    // Additional paranoid check - verify we won't write past buffer end
    if (sb->length + str_len + 1 > sb->capacity) {
        printf("OVERFLOW2: sb->length=%zu str_len=%zu capacity=%zu\n",
               sb->length, str_len, sb->capacity);
        fflush(stdout);
        bug("StringBuffer would overflow!");
        return false;
    }
    
    memcpy(sb->data + sb->length, str, str_len + 1); // +1 to copy null
    sb->length = new_length;

    return true;
}

bool sb_appendf(StringBuffer* sb, const char* fmt, ...)
{
    va_list args;
    char temp[MAX_STRING_LENGTH];

    va_start(args, fmt);
    vsnprintf(temp, sizeof(temp), fmt, args);
    va_end(args);

    return sb_append(sb, temp);
}

bool sb_append_char(StringBuffer* sb, char c)
{
    size_t new_length = sb->length + 1;

    // Check if we need to grow
    if (new_length + 1 > sb->capacity) {
        if (!grow_buffer(sb, new_length)) {
            return false;
        }
        // Verify we actually have enough space after growing
        if (new_length + 1 > sb->capacity) {
            return false;
        }
    }

    // Append character and update null terminator
    sb->data[sb->length] = c;
    sb->data[new_length] = '\0';
    sb->length = new_length;

    return true;
}

bool sb_append_n(StringBuffer* sb, const char* str, size_t n)
{
    if (str == NULL || n == 0) {
        return true;
    }

    size_t new_length = sb->length + n;

    // Check if we need to grow
    if (new_length + 1 > sb->capacity) {
        if (!grow_buffer(sb, new_length)) {
            return false;
        }
        // Verify we actually have enough space after growing
        if (new_length + 1 > sb->capacity) {
            return false;
        }
    }

    // Append n bytes and null terminate
    memcpy(sb->data + sb->length, str, n);
    sb->data[new_length] = '\0';
    sb->length = new_length;

    return true;
}

void sb_clear(StringBuffer* sb)
{
    sb->data[0] = '\0';
    sb->length = 0;
    // Keep capacity - buffer can be reused efficiently
}

const char* sb_string(const StringBuffer* sb)
{
    return sb->data;
}

size_t sb_length(const StringBuffer* sb)
{
    return sb->length;
}

size_t sb_capacity(const StringBuffer* sb)
{
    return sb->capacity;
}

void sb_free(StringBuffer* sb)
{
    if (sb == NULL) {
        return;
    }

    // Free the data buffer
    if (sb->data != NULL) {
        free_mem(sb->data, sb->capacity);
        sb->data = NULL;
    }

    // Add StringBuffer struct to free list for recycling
    sb->next = sb_free_list;
    sb_free_list = sb;
    
    sb_stats.sb_freed++;
}
