////////////////////////////////////////////////////////////////////////////////
// olc/period_edit.h
// Shared period editing functionality for areas and rooms
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__OLC__PERIOD_EDIT_H
#define MUD98__OLC__PERIOD_EDIT_H

#include <merc.h>

typedef struct entity_t Entity;
typedef struct daycycle_period_t DayCyclePeriod;
typedef struct area_data_t AreaData;
typedef struct room_data_t RoomData;

// Main entry points
bool olc_edit_period(Mobile* ch, char* argument, Entity* entity /*, const PeriodOps* ops*/);
void olc_show_periods(Mobile* ch, Entity* entity/*, const PeriodOps* ops*/);

#endif // !MUD98__OLC__PERIOD_EDIT_H
