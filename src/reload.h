////////////////////////////////////////////////////////////////////////////////
// reload.h - Hot-reloading of game data from disk or prototypes
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef ROM24_RELOAD_H
#define ROM24_RELOAD_H

#include "merc.h"

// Forward declarations
typedef struct room_t Room;

// Commands
DECLARE_DO_FUN(do_reload);

// Reload specific subsystems
void reload_helps(Mobile* ch);
void reload_room(Mobile* ch, Room* room);

#endif // ROM24_RELOAD_H
