# StringBuffer - Fast String Building for Mud98

## Overview

`StringBuffer` is a high-performance string concatenation buffer designed to replace `Buffer` with better ergonomics and performance characteristics.

## Key Improvements Over Buffer

1. **O(1) Append** - Tracks `length` and `capacity` separately, eliminating repeated `strlen()` calls
2. **Tail Pointer** - Current length acts as tail pointer for instant append location
3. **No Side Effects** - Growth doesn't call `bug()` or trigger diagnostic logging that can cause GC issues
4. **Clean API** - Simple, predictable interface
5. **Statistics** - Built-in metrics for profiling and debugging

## Design Philosophy

### Memory Management
- Uses `alloc_mem()` / `free_mem()` for data buffers (memory pool integration)
- Uses `alloc_perm()` for StringBuffer structs (never freed, recycled via free list)
- Size buckets align with `rgSizeList` in `alloc_mem()` for efficiency
- Grows intelligently: 1K → 2K → 4K → 4608 → 8K → 9216 → 16K → 18432 → ...

### Performance
- **Tracked Length**: No `strlen()` calls during append operations
- **Batch Growth**: Grows to next size bucket, not just-barely-enough
- **Memory Pooling**: Reuses freed buffers from `alloc_mem` pools
- **Struct Recycling**: StringBuffer structs go to free list, not deallocated

### Safety
- Capacity limits respect `MAX_STRING_LENGTH * 4` (72K max)
- Returns `false` on overflow (doesn't abort or log)
- Always null-terminated
- Can't trigger GC during growth (unlike old Buffer)

## API

### Creation
```c
StringBuffer* sb = sb_new();              // Default 1K capacity
StringBuffer* sb = sb_new_size(8192);     // Hint initial capacity
```

### Append Operations
```c
sb_append(sb, "text");                    // Append C string
sb_appendf(sb, "num=%d", 42);            // Printf-style formatting
sb_append_char(sb, 'x');                  // Single character
sb_append_n(sb, data, len);               // Exact byte count
```

### Access
```c
const char* str = sb_string(sb);          // Get string pointer
const char* str = SB(sb);                 // Macro shorthand
size_t len = sb_length(sb);               // Current length
size_t cap = sb_capacity(sb);             // Allocated capacity
```

### Management
```c
sb_clear(sb);                             // Reset to empty (keep capacity)
sb_free(sb);                              // Return to free list
```

### Statistics
```c
extern StringBufferStats sb_stats;        // Global stats
sb_reset_stats();                         // Clear counters

typedef struct {
    int sb_created;     // Total allocated
    int sb_freed;       // Total freed
    int sb_recycled;    // Reused from free list
    int growth_count;   // Growth operations
    size_t bytes_grown; // Total capacity growth
} StringBufferStats;
```

## Usage Patterns

### Simple Output Building
```c
StringBuffer* out = sb_new();
sb_append(out, "Header\n");
sb_appendf(out, "Value: %d\n", some_value);
page_to_char(SB(out), ch);
sb_free(out);
```

### Large List Generation
```c
StringBuffer* list = sb_new_size(16384);  // Hint large size
for (int i = 0; i < 273; i++) {
    sb_appendf(list, "%-18s %-10s %3d\n", 
               cmd_table[i].name, 
               cmd_table[i].category,
               cmd_table[i].level);
}
send_to_char(SB(list), ch);
sb_free(list);
```

### Clear and Reuse
```c
StringBuffer* temp = sb_new();
for (int i = 0; i < iterations; i++) {
    sb_clear(temp);  // Fast reset
    build_output(temp);
    send_to_char(SB(temp), ch);
}
sb_free(temp);
```

## Migration from Buffer

| Old (Buffer) | New (StringBuffer) |
|-------------|-------------------|
| `Buffer* buf = new_buf()` | `StringBuffer* sb = sb_new()` |
| `add_buf(buf, str)` | `sb_append(sb, str)` |
| `addf_buf(buf, fmt, ...)` | `sb_appendf(sb, fmt, ...)` |
| `BUF(buf)` | `SB(sb)` |
| `clear_buf(buf)` | `sb_clear(sb)` |
| `free_buf(buf)` | `sb_free(sb)` |

## Performance Characteristics

### Time Complexity
- **Append**: O(1) amortized (O(n) when growth needed)
- **Append n chars**: O(n)
- **Clear**: O(1)
- **Get string**: O(1)

### Space Efficiency
- Grows by ~2x with size bucket alignment
- No wasted allocations below bucket sizes
- Memory pool reuse minimizes fragmentation

### Comparison to Naive strcat() Loop
```c
// Naive: O(n²) - strlen() on every iteration
for (int i = 0; i < 1000; i++) {
    strcat(buffer, "x");  // strlen(buffer) each time
}

// StringBuffer: O(n) - length tracked
StringBuffer* sb = sb_new();
for (int i = 0; i < 1000; i++) {
    sb_append(sb, "x");  // No strlen needed
}
```

## Implementation Notes

### Why Not std::string?
This is C, not C++. `StringBuffer` provides similar ergonomics with C idioms.

### Why Track Length AND Capacity?
- `length`: Current string length (excluding null terminator)
- `capacity`: Total allocated bytes (including null terminator space)
- Allows O(1) append without strlen() or pointer arithmetic

### Why Size Buckets?
Aligns with `alloc_mem()` size list to avoid wasted space and enable efficient pooling.

### Why Not Just Use Buffer?
Old `Buffer` had issues:
1. Called `strlen()` on every `add_buf()` (O(n) per append)
2. Used `bug()` during growth (allocates memory, can trigger GC)
3. No length tracking
4. Harder to debug (state flags instead of length)

## Testing

See `stringbuffer_example.c` for usage examples.

Run the example:
```bash
gcc -o sb_example src/stringbuffer.c src/stringbuffer_example.c \
    -Isrc -DMAX_STRING_LENGTH=4608
./sb_example
```

## Future Enhancements

Possible additions if needed:
- `sb_shrink()` - Reduce capacity to fit length
- `sb_reserve()` - Pre-allocate specific capacity
- `sb_insert()` - Insert at arbitrary position (expensive, rarely needed)
- `sb_substring()` - Extract substring
- `sb_to_owned()` - Transfer ownership of data buffer

## Thread Safety

`StringBuffer` is **not thread-safe**. Each thread should use its own instance. The free list is not protected by locks.
