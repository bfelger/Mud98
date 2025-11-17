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
Event* get_event_by_trigger_strval(Entity* entity, FLAGS trigger, const char* str);
ObjClosure* get_event_closure(Entity* entity, Event* event);

#define HAS_EVENT_TRIGGER(entity, trigger)                                     \
    (((Entity*)(entity))->event_triggers & trigger)

extern int event_count;
extern int event_perm_count;
extern Event* event_free;

// EVENT TRIGGER ROUTINES //////////////////////////////////////////////////////

/* TRIG_ACT     */  void raise_act_event(Entity* receiver, EventTrigger trig_type, Entity* actor, char* msg);
/* TRIG_BRIBE   */
/* TRIG_DEATH   */
/* TRIG_ENTRY   */
/* TRIG_FIGHT   */
/* TRIG_GIVE    */
/* TRIG_GREET   */  void raise_greet_event(Mobile* ch);
/* TRIG_GRALL   */  // ...
/* TRIG_KILL    */
/* TRIG_HPCNT   */
/* TRIG_RANDOM  */
/* TRIG_SPEECH  */
/* TRIG_EXIT    */
/* TRIG_EXALL   */
/* TRIG_DELAY   */
/* TRIG_SURR    */
/* TRIG_LOGIN   */  void raise_login_event(Mobile* ch);

#endif // !MUD98__ENTITIES__EVENT_H
