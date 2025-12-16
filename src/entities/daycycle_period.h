////////////////////////////////////////////////////////////////////////////////
// entities/daycycle_period.h
// Shared helpers for time-of-day period definitions used by rooms and areas.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ENTITIES__DAYCYCLE_PERIOD_H
#define MUD98__ENTITIES__DAYCYCLE_PERIOD_H

#include <merc.h>

typedef struct daycycle_period_t {
    struct daycycle_period_t* next;
    char* name;
    char* description;
    char* enter_message;
    char* exit_message;
    int8_t start_hour;
    int8_t end_hour;
} DayCyclePeriod;

DayCyclePeriod* daycycle_period_new(const char* name, int start_hour, int end_hour);
void free_daycycle_period(DayCyclePeriod* period);
void daycycle_period_append(DayCyclePeriod** head, DayCyclePeriod* period);
DayCyclePeriod* daycycle_period_clone_list(const DayCyclePeriod* head);
DayCyclePeriod* daycycle_period_find(DayCyclePeriod* head, const char* name);
bool daycycle_period_remove(DayCyclePeriod** head, const char* name);
void daycycle_period_clear(DayCyclePeriod** head);
bool daycycle_period_contains_hour(const DayCyclePeriod* period, int hour);
int daycycle_normalize_hour(int hour);

#endif // !MUD98__ENTITIES__DAYCYCLE_PERIOD_H
