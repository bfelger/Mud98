////////////////////////////////////////////////////////////////////////////////
// olc/event_edit.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__OLC__EVENT_EDIT_H
#define MUD98__OLC__EVENT_EDIT_H

#include <entities/mobile.h>

#include <stdbool.h>

bool olc_edit_event(Mobile* ch, char* argument);
void olc_display_events(Mobile* ch, Entity* entity);

#endif // !MUD98__OLC__EVENT_EDIT_H
