////////////////////////////////////////////////////////////////////////////////
// events.c
// Trigger scripted events with Lox callbacks
////////////////////////////////////////////////////////////////////////////////

#include "events.h"

#include <comm.h>
#include <db.h>
#include <lookup.h>

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

    event->obj.type = OBJ_EVENT;
    event->trigger = 0;
    event->criteria = NIL_VAL;
    event->method_name = lox_empty_string;
    event->next = NULL;

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
    entity->event_triggers = 0;
    for (Node* node = entity->events.front; node != NULL; node = node->next){
        Event* ev = (Event*)node->value;
        entity->event_triggers |= ev->trigger;
    }
}

void load_event(FILE* fp, Entity* owner)
{
    Event* event;
    char* keyword;

    if ((event = new_event()) == NULL) {
        perror("load_event: could not allocate new Event!");
        exit(-1);
    }

    // Read trigger keyword
    keyword = fread_word(fp);
    if ((event->trigger = flag_lookup(keyword, mprog_flag_table)) == NO_FLAG) {
        bugf("load_event: invalid trigger '%s'.", keyword);
        event->trigger = 0;
    }

    // Read the method name to call on event
    event->method_name = fread_lox_string(fp);

    event->criteria = NIL_VAL;

    add_event(owner, event);
}

// Get the event with the specified trigger flag from an entity
Event* get_event_by_trigger(Entity* entity, FLAGS trigger)
{
    if (entity == NULL)
        return NULL;

    if (!HAS_EVENT_TRIGGER(entity, trigger))
        return NULL;

    for (Node* node = entity->events.front; node != NULL; node = node->next) {
        if (!IS_EVENT(node->value)) {
            bug("ERROR: Invalid event node on entity #%"PRVNUM".\n", entity->vnum);
            continue;
        }

        Event* ev = AS_EVENT(node->value);
        if (ev && (ev->trigger & trigger) != 0)
            return ev;
    }

    return NULL;
}

// Get the event closure by checking the entity's class for method name
ObjClosure* get_event_closure(Entity* entity, Event* event)
{
    if (entity == NULL || event == NULL || event->method_name == NULL)
        return NULL;

    if (entity->klass == NULL) {
        bugf("get_event_closure: entity '%s' has no class.",
            entity->name ? entity->name->chars : "<unnamed>");
        return NULL;
    }

    Value method_val;
    if (!table_get(&entity->klass->methods, event->method_name, &method_val)) {
        bugf("get_event_closure: entity '%s' class '%s' has no method '%s' for event.",
            entity->name ? entity->name->chars : "<unnamed>",
            entity->klass->name ? entity->klass->name->chars : "<unnamed>",
            event->method_name->chars ? event->method_name->chars : "<unnamed>");
        return NULL;
    }

    if (!IS_CLOSURE(method_val)) {
        bugf("get_event_closure: entity '%s' class '%s' method '%s' is not a closure.",
            entity->name ? entity->name->chars : "<unnamed>",
            entity->klass->name ? entity->klass->name->chars : "<unnamed>",
            event->method_name->chars ? event->method_name->chars : "<unnamed>");
        return NULL;
    }

    return AS_CLOSURE(method_val);
}

