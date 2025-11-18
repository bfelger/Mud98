////////////////////////////////////////////////////////////////////////////////
// entities/event.c
// Trigger scripted events with Lox callbacks
////////////////////////////////////////////////////////////////////////////////

#include "event.h"

#include <comm.h>
#include <db.h>
#include <lookup.h>

#include <data/events.h>

#include <olc/bit.h>

#include <lox/list.h>
#include <lox/memory.h>
#include <lox/vm.h>

int event_count = 0;
int event_perm_count = 0;
Event* event_free = NULL;

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

    gc_protect(OBJ_VAL(event));

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
    for (Node* node = entity->events.front; node != NULL; node = node->next) {
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

    // TODO: Parse subsequent Lox Value
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
            bug("get_event_by_trigger: Invalid event node on entity #%"PRVNUM".\n", 
                entity->vnum);
            continue;
        }

        Event* ev = AS_EVENT(node->value);
        if (ev && (ev->trigger & trigger) != 0)
            return ev;
    }

    return NULL;
}

Event* get_event_by_trigger_strval(Entity* entity, FLAGS trigger, const char* str)
{
    if (entity == NULL)
        return NULL;

    if (!HAS_EVENT_TRIGGER(entity, trigger))
        return NULL;

    for (Node* node = entity->events.front; node != NULL; node = node->next) {
        if (!IS_EVENT(node->value)) {
            bug("get_event_by_trigger_strval: Invalid event node on entity #%"PRVNUM".\n", 
                entity->vnum);
            continue;
        }

        Event* ev = AS_EVENT(node->value);
        if (ev == NULL || (ev->trigger & trigger) == 0)
            continue;

        if (!IS_STRING(ev->criteria)) {
            bug("get_event_by_trigger_strval: Invalid trigger criteria node on entity #%"PRVNUM","
                " event '%s'; expected string criteria.\n",
                entity->vnum, get_event_name(ev->trigger));
            continue;
        }

        ObjString* trig_phrase = AS_STRING(ev->criteria);

        if (strstr(str, trig_phrase->chars) != NULL)
            return ev;
    }

    return NULL;
}

Event* get_event_by_trigger_intval(Entity* entity, FLAGS trigger, int val)
{
    if (entity == NULL)
        return NULL;

    if (!HAS_EVENT_TRIGGER(entity, trigger))
        return NULL;

    for (Node* node = entity->events.front; node != NULL; node = node->next) {
        if (!IS_EVENT(node->value)) {
            bug("get_event_by_trigger_intval: Invalid event node on entity #%"PRVNUM".\n", 
                entity->vnum);
            continue;
        }
        Event* ev = AS_EVENT(node->value);
        if (ev == NULL || (ev->trigger & trigger) == 0)
            continue;

        if (!IS_INT(ev->criteria)) {
            bug("get_event_by_trigger_intval: Invalid trigger criteria node on entity #%"PRVNUM","
                " event '%s'; expected number criteria.\n",
                entity->vnum, get_event_name(ev->trigger));
            continue;
        }

        int trig_val = AS_INT(ev->criteria);

        if (val >= trig_val)
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

// EVENT TRIGGERS //////////////////////////////////////////////////////////////

// TRIG_ACT
void raise_act_event(Entity* receiver, EventTrigger trig_type, Entity* actor, char* msg)
{
    if (!HAS_EVENT_TRIGGER(receiver, TRIG_ACT))
        return;

    Event* event = get_event_by_trigger_strval(receiver, trig_type, msg);

    if (event == NULL)
        return;

    ObjClosure* closure = get_event_closure(receiver, event);
    
    if (closure == NULL)
        return;

    Value msg_val = OBJ_VAL(lox_string(msg));

    invoke_method_closure(OBJ_VAL(receiver), closure, 2, OBJ_VAL(actor), msg_val);
}

// TRIG_BRIBE
void raise_bribe_event(Mobile* mob, Mobile* ch, int amount)
{
    if (!HAS_EVENT_TRIGGER(mob, TRIG_BRIBE))
        return;

    Event* event = get_event_by_trigger_intval((Entity*)mob, TRIG_BRIBE, amount);
    ObjClosure* closure = get_event_closure((Entity*)mob, event);

    if (closure == NULL)
        return;

    invoke_method_closure(OBJ_VAL(mob), closure, 2, OBJ_VAL(ch), INT_VAL(amount));
}

// TRIG_DEATH
void raise_death_event(Mobile* victim, Mobile* killer)
{
    Event* event;
    ObjClosure* closure;

    if (HAS_EVENT_TRIGGER(victim, TRIG_DEATH)
        && (event = get_event_by_trigger((Entity*)victim, TRIG_DEATH)) != NULL
        && (closure = get_event_closure((Entity*)victim, event)) != NULL)
        invoke_method_closure(OBJ_VAL(victim), closure, 1, OBJ_VAL(killer));
}

// TRIG_ENTRY
void raise_entry_event(Mobile* mob, int pct_chance)
{
    if (!HAS_EVENT_TRIGGER(mob, TRIG_ENTRY))
        return;

    Event* event = get_event_by_trigger((Entity*)mob, TRIG_ENTRY);

    if (!IS_INT(event->criteria))
        return;

    if (pct_chance > AS_INT(event->criteria))
        return;

    ObjClosure* closure = get_event_closure((Entity*)mob, event);

    if (closure == NULL)
        return;

    invoke_method_closure(OBJ_VAL(mob), closure, 0);
}

// TRIG_FIGHT
void raise_fight_event(Mobile* attacker, Mobile* victim, int pct_chance)
{
    if (!HAS_EVENT_TRIGGER(attacker, TRIG_FIGHT))
        return;

    Event* event = get_event_by_trigger((Entity*)attacker, TRIG_FIGHT);

    if (!IS_INT(event->criteria))
        return;

    if (pct_chance > AS_INT(event->criteria))
        return;

    ObjClosure* closure = get_event_closure((Entity*)attacker, event);

    if (closure == NULL)
        return;

    invoke_method_closure(OBJ_VAL(attacker), closure, 1, OBJ_VAL(victim));
}

// TRIG_GIVE

// TRIG_GREET
// TRIG_GRALL
void raise_greet_event(Mobile* ch)
{
    Mobile* mob;
    Event* event;
    ObjClosure* closure;
    Room* room = ch->in_room;

    // First check the room for event triggers
    if (HAS_EVENT_TRIGGER(room, TRIG_GREET)
        && (event = get_event_by_trigger((Entity*)room, TRIG_GREET)) != NULL
        && (closure = get_event_closure((Entity*)room, event)) != NULL)
        invoke_method_closure(OBJ_VAL(room), closure, 1, OBJ_VAL(ch));

    // Now check every mob in the room
    FOR_EACH_ROOM_MOB(mob, ch->in_room) {
        event = NULL;

        if (!IS_NPC(mob) || (!HAS_EVENT_TRIGGER(mob, TRIG_GREET) && !HAS_EVENT_TRIGGER(mob, TRIG_GRALL)))
            continue;

        if (HAS_EVENT_TRIGGER(mob, TRIG_GREET))
            event = get_event_by_trigger((Entity*)mob, TRIG_GREET);
        else if (HAS_EVENT_TRIGGER(mob, TRIG_GRALL))
            event = get_event_by_trigger((Entity*)mob, TRIG_GRALL);
       
        if (event == NULL 
            || (closure = get_event_closure((Entity*)mob, event)) == NULL)
            continue;
                
        invoke_method_closure(OBJ_VAL(mob), closure, 1, OBJ_VAL(ch));
    }
}

// TRIG_KILL
// TRIG_HPCNT
// TRIG_RANDOM
// TRIG_SPEECH
// TRIG_EXIT
// TRIG_EXALL
// TRIG_DELAY
// TRIG_SURR

// TRIG_LOGIN
void raise_login_event(Mobile* ch)
{
    Event* event = get_event_by_trigger((Entity*)ch->in_room, TRIG_LOGIN);

    if (event == NULL)
        return;

    // Get the closure for this event from the room
    ObjClosure* closure = get_event_closure((Entity*)ch->in_room, event);

    if (closure == NULL)
        return;

    // Invoke the closure with the room and character as parameters
    invoke_method_closure(OBJ_VAL(ch->in_room), closure, 1, OBJ_VAL(ch));
}
