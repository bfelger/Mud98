////////////////////////////////////////////////////////////////////////////////
// events.h
// Trigger scripted events with Lox callbacks
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__EVENTS_H
#define MUD98__DATA__EVENTS_H

#include <merc.h>

typedef enum {
    TRIG_ACT    = BIT(0),           // A
    TRIG_BRIBE  = BIT(1),           // B
    TRIG_DEATH  = BIT(2),           // C
    TRIG_ENTRY  = BIT(3),           // D
    TRIG_FIGHT  = BIT(4),           // E
    TRIG_GIVE   = BIT(5),           // F
    TRIG_GREET  = BIT(6),           // G
    TRIG_GRALL  = BIT(7),           // H
    TRIG_KILL   = BIT(8),           // I
    TRIG_HPCNT  = BIT(9),           // J
    TRIG_RANDOM = BIT(10),          // K
    TRIG_SPEECH = BIT(12),          // L
    TRIG_EXIT   = BIT(13),          // M
    TRIG_EXALL  = BIT(14),          // N
    TRIG_DELAY  = BIT(15),          // O
    TRIG_SURR   = BIT(16),          // P
    TRIG_LOGIN  = BIT(17),          // Q
} EventTrigger;

#define LAST_TRIG TRIG_LOGIN

#endif // !MUD98__DATA__EVENTS_H
