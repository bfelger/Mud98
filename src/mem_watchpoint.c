////////////////////////////////////////////////////////////////////////////////
// mem_watchpoint.c - Memory corruption debugging utility implementation
////////////////////////////////////////////////////////////////////////////////

#include "mem_watchpoint.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    void* addr;
    size_t size;
    char label[128];
    uint8_t* snapshot;
    bool active;
} Watchpoint;

static Watchpoint watchpoints[MAX_WATCHPOINTS];
static int watchpoint_count = 0;

extern bool test_output_enabled;

bool mem_watch_add(void* addr, size_t size, const char* label)
{
    if (watchpoint_count >= MAX_WATCHPOINTS) {
        if (!test_output_enabled)
            fprintf(stderr, "mem_watch_add: Too many watchpoints!\n");
        return false;
    }

    Watchpoint* wp = &watchpoints[watchpoint_count++];
    wp->addr = addr;
    wp->size = size;
    strncpy(wp->label, label ? label : "(unlabeled)", sizeof(wp->label) - 1);
    wp->label[sizeof(wp->label) - 1] = '\0';
    
    // Take snapshot of current memory
    wp->snapshot = (uint8_t*)malloc(size);
    memcpy(wp->snapshot, addr, size);
    wp->active = true;

    if (!test_output_enabled) {
        fprintf(stderr, "WATCH: Added watchpoint on %s at %p (%zu bytes)\n",
                wp->label, addr, size);
    }
    return true;
}

int mem_watch_check(void)
{
    int corrupted_count = 0;

    for (int i = 0; i < watchpoint_count; i++) {
        Watchpoint* wp = &watchpoints[i];
        if (!wp->active) continue;

        // Compare current memory with snapshot
        if (memcmp(wp->addr, wp->snapshot, wp->size) != 0) {
            if (!test_output_enabled) {
                fprintf(stderr, "\n**********************************************\n");
                fprintf(stderr, "CORRUPTION DETECTED: %s at %p\n", wp->label, wp->addr);
                fprintf(stderr, "**********************************************\n");

            
                // Show which bytes changed
                uint8_t* current = (uint8_t*)wp->addr;
                for (size_t j = 0; j < wp->size; j++) {
                    if (current[j] != wp->snapshot[j]) {
                        fprintf(stderr, "  Offset +%zu: was 0x%02x, now 0x%02x",
                                j, wp->snapshot[j], current[j]);
                        if (wp->snapshot[j] >= 32 && wp->snapshot[j] < 127)
                            fprintf(stderr, " (was '%c')", wp->snapshot[j]);
                        if (current[j] >= 32 && current[j] < 127)
                            fprintf(stderr, " (now '%c')", current[j]);
                        fprintf(stderr, "\n");
                        
                        // Only show first 20 changes
                        if (corrupted_count > 20) {
                            fprintf(stderr, "  ... (more changes not shown)\n");
                            break;
                        }
                    }
                }
                fprintf(stderr, "**********************************************\n\n");
            }
            corrupted_count++;
            
            // Update snapshot so we don't report same corruption repeatedly
            memcpy(wp->snapshot, wp->addr, wp->size);
        }
    }

    return corrupted_count;
}

void mem_watch_clear(void)
{
    for (int i = 0; i < watchpoint_count; i++) {
        if (watchpoints[i].snapshot) {
            free(watchpoints[i].snapshot);
            watchpoints[i].snapshot = NULL;
        }
        watchpoints[i].active = false;
    }
    watchpoint_count = 0;
    if (!test_output_enabled)
        fprintf(stderr, "WATCH: Cleared all watchpoints\n");
}

void mem_watch_dump(void)
{
    if (test_output_enabled)
        return;

    fprintf(stderr, "\nWatchpoint Status (%d active):\n", watchpoint_count);
    for (int i = 0; i < watchpoint_count; i++) {
        Watchpoint* wp = &watchpoints[i];
        if (!wp->active) continue;
        
        bool modified = memcmp(wp->addr, wp->snapshot, wp->size) != 0;
        fprintf(stderr, "  [%d] %s at %p (%zu bytes) - %s\n",
                i, wp->label, wp->addr, wp->size,
                modified ? "MODIFIED" : "intact");
    }
    fprintf(stderr, "\n");
}
