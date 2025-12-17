////////////////////////////////////////////////////////////////////////////////
// globals.c
////////////////////////////////////////////////////////////////////////////////

// Game Globals
//
// Shared global variables used throughout the game core.
// These are defined here and linked into all executables.

#include "merc.h"
#include <stdbool.h>
#include <time.h>

// Global game state
bool merc_down = false;             // Shutdown
bool wizlock = false;               // Game is wizlocked
bool newlock = false;               // Game is newlocked
char str_boot_time[MAX_INPUT_LENGTH];
time_t boot_time;                   // time of boot
time_t current_time;                // time of this pulse
bool events_enabled = true;         // act() switch

int get_uptime()
{
    return (int)(current_time - boot_time);
}
