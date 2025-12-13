////////////////////////////////////////////////////////////////////////////////
// mem_watchpoint.h - Memory corruption debugging utility
//
// Use this to track down wild pointer writes by monitoring specific memory
// addresses for unexpected modifications.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__MEM_WATCHPOINT_H
#define MUD98__MEM_WATCHPOINT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_WATCHPOINTS 32

// Add a watchpoint on a memory address
// Call this before the suspected corruption point
// Returns true on success, false if too many watchpoints
bool mem_watch_add(void* addr, size_t size, const char* label);

// Check if any watched memory has been modified
// Call this periodically or after suspicious operations
// Returns number of corrupted watchpoints
int mem_watch_check(void);

// Clear all watchpoints
void mem_watch_clear(void);

// Print watchpoint status
void mem_watch_dump(void);

// Helper macro to watch a specific variable
#define WATCH_VAR(var) mem_watch_add(&(var), sizeof(var), #var)

// Helper macro to watch a struct
#define WATCH_STRUCT(ptr, type) mem_watch_add((ptr), sizeof(type), #ptr)

#endif // MUD98__MEM_WATCHPOINT_H
