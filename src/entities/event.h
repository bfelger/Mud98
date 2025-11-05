////////////////////////////////////////////////////////////////////////////////
// entities/event.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ENTITIES__EVENT_H
#define MUD98__ENTITIES__EVENT_H

#include <merc.h>

#include "entity.h"

#include <lox/object.h>

#include <recycle.h>

struct event_t {
    Obj obj;
    Event* next;
    FLAGS trigger;
    Value criteria;
    ObjString* method_name;
};

Event* new_event();
void free_event(Event* event);
void add_event(Entity* entity, Event* event);
void add_event_timer(ObjClosure* closure, int ticks);
void event_timer_tick();
void remove_event(Entity* entity, Event* event);
void load_event(FILE* fp, Entity* owner);
Event* get_event_by_trigger(Entity* entity, FLAGS trigger);
ObjClosure* get_event_closure(Entity* entity, Event* event);
void olc_display_event_info(Mobile* ch, Entity* entity);

#define HAS_EVENT_TRIGGER(entity, trigger)                                     \
    (((Entity*)(entity))->event_triggers & trigger)

extern int event_count;
extern int event_perm_count;
extern Event* event_free;

#endif // !MUD98__ENTITIES__EVENT_H
