////////////////////////////////////////////////////////////////////////////////
// data/events.h
// Trigger scripted events with Lox callbacks
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__EVENTS_H
#define MUD98__DATA__EVENTS_H

#include <merc.h>

typedef struct entity_t Entity;

typedef enum event_trigger_t {
    TRIG_ACT        = BIT(0),           // A
    TRIG_ATTACKED   = BIT(1),           // B
    TRIG_BRIBE      = BIT(2),           // C
    TRIG_DEATH      = BIT(3),           // D
    TRIG_ENTRY      = BIT(4),           // E
    TRIG_FIGHT      = BIT(5),           // F
    TRIG_GIVE       = BIT(6),           // G
    TRIG_GREET      = BIT(7),           // H
    TRIG_GRALL      = BIT(8),           // I
    TRIG_HPCNT      = BIT(9),           // J
    TRIG_RANDOM     = BIT(10),          // K
    TRIG_SPEECH     = BIT(12),          // L
    TRIG_EXIT       = BIT(13),          // M
    TRIG_EXALL      = BIT(14),          // N
    TRIG_DELAY      = BIT(15),          // O
    TRIG_SURR       = BIT(16),          // P
    TRIG_LOGIN      = BIT(17),          // Q
    TRIG_GIVEN      = BIT(18),          // R
    TRIG_TAKEN      = BIT(19),          // S
    TRIG_DROPPED    = BIT(20),          // T
} EventTrigger;

#define TRIG_COUNT 21

typedef enum {
    ENT_AREA    = BIT(0),
    ENT_ROOM    = BIT(1),
    ENT_MOB     = BIT(2),
    ENT_OBJ     = BIT(3),
    ENT_QUEST   = BIT(4),
} EventEnts;

#define ENT_ALL (ENT_ROOM | ENT_MOB | ENT_OBJ)

typedef enum {
    CRIT_NONE = 0,
    CRIT_INT,
    CRIT_STR,
    CRIT_INT_OR_STR,
} EventCriteriaType;

typedef struct {
    EventTrigger trigger;               // EventTrigger
    const char* name;                   // Name to use for display/save/load
    const char* default_callback;       // Lox method to call (by default)
    FLAGS valid_ents;                   // Valid recipients
    EventCriteriaType criteria_type;    // What type of criteria this trigger accepts
    const char* default_criteria;       // Default criteria (string form) or NULL
} EventTypeInfo;

EventEnts get_entity_type(Entity* entity);
const char* get_event_default_callback(EventTrigger trigger);
const char* get_event_name(EventTrigger trigger);
const EventTypeInfo* get_event_type_info(EventTrigger trigger);

extern const EventTypeInfo event_type_info_table[];

#define LAST_TRIG TRIG_LOGIN

#endif // !MUD98__DATA__EVENTS_H
