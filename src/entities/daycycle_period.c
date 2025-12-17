////////////////////////////////////////////////////////////////////////////////
// entities/daycycle_period.c
////////////////////////////////////////////////////////////////////////////////

#include "daycycle_period.h"

#include <db.h>

int daycycle_period_count;
int daycycle_period_perm_count;
DayCyclePeriod* daycycle_period_free;

int daycycle_normalize_hour(int hour)
{
    int normalized = hour % 24;
    if (normalized < 0)
        normalized += 24;
    return normalized;
}

DayCyclePeriod* daycycle_period_new(const char* name, int start_hour, int end_hour)
{
    LIST_ALLOC_PERM(daycycle_period, DayCyclePeriod);

    daycycle_period->name = str_dup(name ? name : "");
    daycycle_period->description = &str_empty[0];
    daycycle_period->enter_message = &str_empty[0];
    daycycle_period->exit_message = &str_empty[0];
    daycycle_period->start_hour = (int8_t)daycycle_normalize_hour(start_hour);
    daycycle_period->end_hour = (int8_t)daycycle_normalize_hour(end_hour);
    daycycle_period->next = NULL;

    return daycycle_period;
}

void free_daycycle_period(DayCyclePeriod* daycycle_period)
{
    if (daycycle_period == NULL)
        return;

    free_string(daycycle_period->name);
    free_string(daycycle_period->description);
    free_string(daycycle_period->enter_message);
    free_string(daycycle_period->exit_message);

    LIST_FREE(daycycle_period);
}

void daycycle_period_append(DayCyclePeriod** head, DayCyclePeriod* period)
{
    if (!head || !period)
        return;

    period->next = NULL;

    if (*head == NULL) {
        *head = period;
        return;
    }

    DayCyclePeriod* tail = *head;
    while (tail->next != NULL)
        tail = tail->next;
    tail->next = period;
}

DayCyclePeriod* daycycle_period_clone_list(const DayCyclePeriod* head)
{
    DayCyclePeriod* new_head = NULL;
    DayCyclePeriod* tail = NULL;

    for (const DayCyclePeriod* period = head; period != NULL; period = period->next) {
        DayCyclePeriod* node = daycycle_period_new(period->name, period->start_hour, period->end_hour);
        free_string(node->description);
        node->description = str_dup(period->description ? period->description : "");
        free_string(node->enter_message);
        node->enter_message = str_dup(period->enter_message ? period->enter_message : "");
        free_string(node->exit_message);
        node->exit_message = str_dup(period->exit_message ? period->exit_message : "");

        if (new_head == NULL)
            new_head = node;
        else
            tail->next = node;
        tail = node;
    }

    return new_head;
}

DayCyclePeriod* daycycle_period_find(DayCyclePeriod* head, const char* name)
{
    if (!head || !name || name[0] == '\0')
        return NULL;

    for (DayCyclePeriod* period = head; period != NULL; period = period->next) {
        if (!str_cmp(period->name, name))
            return period;
    }

    return NULL;
}

bool daycycle_period_remove(DayCyclePeriod** head, const char* name)
{
    if (!head || !*head || !name || name[0] == '\0')
        return false;

    DayCyclePeriod* prev = NULL;
    DayCyclePeriod* curr = *head;

    while (curr != NULL) {
        if (!str_cmp(curr->name, name)) {
            if (prev == NULL)
                *head = curr->next;
            else
                prev->next = curr->next;
            free_daycycle_period(curr);
            return true;
        }
        prev = curr;
        curr = curr->next;
    }

    return false;
}

void daycycle_period_clear(DayCyclePeriod** head)
{
    if (!head)
        return;

    DayCyclePeriod* period = *head;
    while (period != NULL) {
        DayCyclePeriod* next = period->next;
        free_daycycle_period(period);
        period = next;
    }

    *head = NULL;
}

bool daycycle_period_contains_hour(const DayCyclePeriod* period, int hour)
{
    if (!period)
        return false;

    int normalized = daycycle_normalize_hour(hour);
    int8_t start = period->start_hour;
    int8_t end = period->end_hour;

    if (start <= end)
        return normalized >= start && normalized <= end;
    return normalized >= start || normalized <= end;
}
