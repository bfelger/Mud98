////////////////////////////////////////////////////////////////////////////////
// events.c
// Trigger scripted events with Lox callbacks
////////////////////////////////////////////////////////////////////////////////

#include "events.h"

#include <db.h>

#include <lox/list.h>

int event_count;
int event_perm_count;
Event* event_free;

typedef struct event_timer_t EventTimer;

// Used for delayed events
struct event_timer_t {
    EventTimer* next;
    ObjClosure* closure;
    int ticks;
};

int event_timer_count;
int event_timer_perm_count;
EventTimer* event_timer_free;
EventTimer* event_timers = NULL;

EventTimer* new_event_timer(ObjClosure* closure, int ticks)
{
    LIST_ALLOC_PERM(event_timer, EventTimer);

    event_timer->closure = closure;
    event_timer->ticks = ticks;

    return event_timer;
}

void free_event_timer(EventTimer* event_timer)
{
    LIST_FREE(event_timer);
}

void add_event_timer(ObjClosure* closure, int ticks)
{
    EventTimer* timer = new_event_timer(closure, ticks);
    timer->next = event_timers;
    event_timers = timer;
}

void event_timer_tick()
{
    EventTimer* timer = NULL;
    EventTimer* timer_next = NULL;

    for (timer = event_timers; timer != NULL; timer = timer_next) {
        timer_next = timer->next;
        if (--timer->ticks <= 0) {
            // Time to run the event
            ObjClosure* closure = timer->closure;

            // Remove from list
            if (timer == event_timers)
                event_timers = timer->next;
            else {
                EventTimer* t = event_timers;
                while (t != NULL && t->next != timer)
                    t = t->next;
                if (t != NULL)
                    t->next = timer->next;
            }

            free_event_timer(timer);

            if (closure != NULL) {
                // Run the closure
                invoke_closure(closure, 0);
            }
        }
    }
}

Event* new_event()
{
    LIST_ALLOC_PERM(event, Event);

    event->trigger = 0;
    event->criteria = NIL_VAL;
    event->name = lox_empty_string;
    event->func_name = lox_empty_string;
    event->closure = NULL;

    return event;
}

void free_event(Event* event)
{
    LIST_FREE(event);
}

void add_event(Entity* entity, Event* event)
{
    list_push(&entity->events, OBJ_VAL(event));
    entity->event_triggers |= event->trigger;
}

void remove_event(Entity* entity, Event* event)
{
    list_remove_value(&entity->events, OBJ_VAL(event));

    // Recalculate event triggers
    entity->event_triggers = 0;;
    for (Node* node = entity->events.front; node != NULL; node = node->next){
        Event* ev = (Event*)node->value;
        entity->event_triggers |= ev->trigger;
    }
}
