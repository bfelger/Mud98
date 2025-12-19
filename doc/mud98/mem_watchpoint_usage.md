# Debugging Memory Corruption with Watchpoints

## Quick Start

Add to the suspect code location (e.g., in cmdedit before "list commands"):

```c
#include <mem_watchpoint.h>

// Watch specific mob prototypes that are getting corrupted
WATCH_STRUCT(get_mob_proto(6301), MobPrototype);
WATCH_STRUCT(get_mob_proto(6306), MobPrototype);
WATCH_STRUCT(get_mob_proto(6308), MobPrototype);

// ... do the operation that causes corruption ...

// Check immediately after suspicious operation
mem_watch_check();

// Check periodically
mem_watch_check();  // After buffer allocation
mem_watch_check();  // After building string
mem_watch_check();  // After page_to_char
mem_watch_check();  // After free

mem_watch_clear();  // Clean up
```

## Example: Debugging cmdedit corruption

In `src/olc/cmdedit.c`, in `cmdedit_list()`:

```c
CMDEDIT(cmdedit_list)
{
    // ... argument parsing ...
    
    // WATCH THE PROTOTYPES BEFORE OPERATION
    extern MobPrototype* get_mob_proto(VNUM vnum);  // or whatever the function is
    mem_watch_add(get_mob_proto(6301), sizeof(MobPrototype), "mob_proto_6301");
    mem_watch_add(get_mob_proto(6306), sizeof(MobPrototype), "mob_proto_6306");
    fprintf(stderr, "WATCHPOINTS SET before cmdedit list\n");
    
    StringBuffer* sb = sb_new();
    mem_watch_check();  // Check #1
    
    if (!str_prefix(arg, "commands"))
        list_commands(sb, minlev, maxlev, filter);
    mem_watch_check();  // Check #2
    
    page_to_char(SB(sb), ch);
    mem_watch_check();  // Check #3
    
    sb_free(sb);
    mem_watch_check();  // Check #4
    
    mem_watch_clear();
    return false;
}
```

## What to look for

The watchpoint system will print:
- Which watchpoint was corrupted
- The memory address
- Which bytes changed
- Old vs new values

Example output:
```
**********************************************
CORRUPTION DETECTED: mob_proto_6301->name at 0x12345678
**********************************************
  Offset +8: was 0x41, now 0xCC (was 'A', now garbage)
  Offset +9: was 0x72, now 0xDD (was 'r')
**********************************************
```

This tells you EXACTLY when the corruption happens (between which check calls).

## Common patterns

### Pattern 1: Immediate corruption
If check #1 shows corruption, the problem happens during `sb_new()` or memory allocation.

### Pattern 2: During string building  
If check #2 shows corruption, it happens in `list_commands()`.

### Pattern 3: During display
If check #3 shows corruption, it happens in `page_to_char()`.

### Pattern 4: During cleanup
If check #4 shows corruption, it happens in `sb_free()`.

### Pattern 5: Delayed corruption
If NO checks show corruption but it appears later, you need to check more frequently
or the corruption is from a completely different code path.

## Advanced usage

Watch a buffer's memory to see if something writes past the end:

```c
StringBuffer* sb = sb_new();
mem_watch_add(sb->data, sb->capacity, "string_buffer_data");

// Watch the bytes AFTER the buffer
mem_watch_add(sb->data + sb->capacity, 256, "guard_bytes_after_buffer");
```

Watch multiple related structures:

```c
for (int i = 6301; i <= 6319; i++) {
    char label[64];
    sprintf(label, "mob_proto_%d", i);
    WATCH_STRUCT(get_mob_proto(i), MobPrototype);
}
```

## Limitations

- Max 32 watchpoints (increase MAX_WATCHPOINTS if needed)
- Uses malloc for snapshots (not from the game's allocator)
- Has performance overhead (memcmp on every check)
- Only detects changes, doesn't tell you WHO changed it
- Need to strategically place checks

## Tips

1. Start broad, narrow down based on which check fails
2. Once you know which operation, add more checks inside that function
3. Binary search: add check in middle of suspect code, see which half has the bug
4. Check BEFORE and AFTER every buffer operation
5. If corruption is delayed, add periodic checks in update loop
