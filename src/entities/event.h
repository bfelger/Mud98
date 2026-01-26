////////////////////////////////////////////////////////////////////////////////
// entities/event.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ENTITIES__EVENT_H
#define MUD98__ENTITIES__EVENT_H

#include <merc.h>

#include "entity.h"

typedef struct object_t Object;
typedef struct obj_closure_t ObjClosure;

#include <lox/object.h>

#include <recycle.h>

typedef struct event_t {
    Obj obj;
    struct event_t* next;
    FLAGS trigger;
    Value criteria;
    ObjString* method_name;
} Event;

// Used for delayed events
typedef struct event_timer_t {
    struct event_timer_t* next;
    ObjClosure* closure;
    int ticks;
} EventTimer;

Event* new_event(void);
void free_event(Event* event);
void add_event(Entity* entity, Event* event);
void add_event_timer(ObjClosure* closure, int ticks);
void event_timer_tick(void);
void remove_event(Entity* entity, Event* event);
void load_event(FILE* fp, Entity* owner);
void save_events(FILE* fp, Entity* entity);
Event* get_event_by_trigger(Entity* entity, FLAGS trigger);
Event* get_event_by_trigger_strval(Entity* entity, FLAGS trigger, const char* str);
Event* get_event_by_trigger_intval(Entity* entity, FLAGS trigger, int val);
ObjClosure* get_event_closure(Entity* entity, Event* event);

#define HAS_EVENT_TRIGGER(entity, trigger)                                     \
    (((Entity*)(entity))->event_triggers & trigger)

extern int event_count;
extern int event_perm_count;
extern Event* event_free;

extern EventTimer* event_timers;

// EVENT TRIGGER ROUTINES //////////////////////////////////////////////////////

/* TRIG_ACT     */  void raise_act_event(Entity* receiver, EventTrigger trig_type, Entity* actor, char* msg);
/* TRIG_ATTACKED*/  void raise_attacked_event(Mobile* victim, Mobile* attacker, int pct_chance);
/* TRIG_BRIBE   */  void raise_bribe_event(Mobile* mob, Mobile* ch, int amount);
/* TRIG_DEATH   */  void raise_death_event(Mobile* victim, Mobile* killer);
/* TRIG_ENTRY   */  void raise_entry_event(Mobile* mob, int pct_chance);
/* TRIG_FIGHT   */  void raise_fight_event(Mobile* attacker, Mobile* victim, int pct_chance);
/* TRIG_GIVE    */  void raise_give_event(Mobile* receiver, Mobile* giver, Object* obj);
/* TRIG_GREET   */  void raise_greet_event(Mobile* ch);
/* TRIG_GRALL   */  // raise_greet_event
/* TRIG_HPCNT   */  void raise_hpcnt_event(Mobile* victim, Mobile* attacker);
/* TRIG_RANDOM  */  bool raise_random_event(Mobile* mob, int pct_chance);
/* TRIG_SPEECH  */  // raise_act_event
/* TRIG_EXIT    */  bool raise_exit_event(Mobile* ch, Direction dir);
/* TRIG_EXALL   */  // raise_exit_event
/* TRIG_DELAY   */  // not implemented in Mud98 events; MobProgs only
/* TRIG_SURR    */  bool raise_surrender_event(Mobile* ch, Mobile* mob, int pct_chance);
/* TRIG_LOGIN   */  void raise_login_event(Mobile* ch);
/* TRIG_GIVEN   */  void raise_object_given_event(Object* obj, Mobile* giver, Mobile* taker);
/* TRIG_TAKEN   */  void raise_object_taken_event(Object* obj, Mobile* taker);
/* TRIG_DROPPED */  void raise_object_dropped_event(Object* obj, Mobile* dropper);
/* TRIG_PRDSTART*/  void raise_prdstart_event(Entity* entity, const char* period_name);
/* TRIG_PRDSTOP */  void raise_prdstop_event(Entity* entity, const char* period_name);

#endif // !MUD98__ENTITIES__EVENT_H
