////////////////////////////////////////////////////////////////////////////////
// data/events.c
////////////////////////////////////////////////////////////////////////////////

#include "events.h"

#include <lox/object.h>

#include <entities/entity.h>

const EventDefaultConfig event_default_configs[TRIG_COUNT] = {                      // Wired?
    { TRIG_ACT,         "act",      "on_act",       ENT_MOB | ENT_OBJ | ENT_ROOM }, //
    { TRIG_BRIBE,       "bribe",    "on_bribe",     ENT_MOB     },                  //
    { TRIG_DEATH,       "death",    "on_death",     ENT_MOB     },                  //
    { TRIG_ENTRY,       "entry",    "on_entry",     ENT_MOB     },                  //
    { TRIG_FIGHT,       "fight",    "on_fight",     ENT_MOB     },                  //
    { TRIG_GIVE,        "give",     "on_give",      ENT_MOB     },                  //
    { TRIG_GREET,       "greet",    "on_greet",     ENT_MOB | ENT_OBJ | ENT_ROOM }, // Mob,Room
    { TRIG_GRALL,       "greet",    "on_greet",     ENT_MOB     },                  // Mob,Room
    { TRIG_KILL,        "kill",     "on_kill",      ENT_MOB     },                  //
    { TRIG_HPCNT,       "hpcnt",    "on_hpcnt",     ENT_MOB     },                  //
    { TRIG_RANDOM,      "random",   "on_random",    ENT_MOB     },                  //
    { TRIG_SPEECH,      "speech",   "on_speech",    ENT_MOB     },                  //
    { TRIG_EXIT,        "exit",     "on_exit",      ENT_MOB     },                  //
    { TRIG_EXALL,       "exit",     "on_exit",      ENT_MOB     },                  //
    { TRIG_DELAY,       "delay",    "on_delay",     ENT_MOB     },                  //
    { TRIG_SURR,        "surr",     "on_surr",      ENT_MOB     },                  //
    { TRIG_LOGIN,       "login",    "on_login",     ENT_ROOM    },                  // Yes
};

EventEnts get_entity_type(Entity* entity)
{
    switch (entity->obj.type) {
    case OBJ_MOB_PROTO:     return ENT_MOB;
    case OBJ_OBJ_PROTO:     return ENT_OBJ;
    case OBJ_AREA_DATA:     return ENT_AREA;
    case OBJ_ROOM_DATA:     return ENT_ROOM;
    default:                return 0;
    }
}

const char* get_event_default_callback(EventTrigger trigger)
{
    for (int i = 0; i < TRIG_COUNT; i++) {
        if (event_default_configs[i].trigger == trigger)
            return event_default_configs[i].default_callback;
    }

    fprintf(stderr, "get_event_default_callback: Unknown trigger type '%d'.\n",
        trigger);
    return "";
}

const char* get_event_name(EventTrigger trigger)
{
    for (int i = 0; i < TRIG_COUNT; i++) {
        if (event_default_configs[i].trigger == trigger)
            return event_default_configs[i].name;
    }

    fprintf(stderr, "get_event_name: Unknown trigger type '%d'.\n",
        trigger);
    return "";
}

