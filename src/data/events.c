////////////////////////////////////////////////////////////////////////////////
// data/events.c
////////////////////////////////////////////////////////////////////////////////

#include "events.h"

#include <lox/object.h>

#include <entities/entity.h>

const EventDefaultConfig event_default_configs[TRIG_COUNT] = {
    { TRIG_ACT,         "on_act",       ENT_MOB | ENT_OBJ | ENT_ROOM },
    { TRIG_BRIBE,       "on_bribe",     ENT_MOB     },
    { TRIG_DEATH,       "on_death",     ENT_MOB     },
    { TRIG_ENTRY,       "on_entry",     ENT_MOB     },
    { TRIG_FIGHT,       "on_fight",     ENT_MOB     },
    { TRIG_GIVE,        "on_give",      ENT_MOB     },
    { TRIG_GREET,       "on_greet",     ENT_MOB | ENT_OBJ | ENT_ROOM },
    { TRIG_GRALL,       "on_greet",     ENT_MOB     },
    { TRIG_KILL,        "on_kill",      ENT_MOB     },
    { TRIG_HPCNT,       "on_hpcnt",     ENT_MOB     },
    { TRIG_RANDOM,      "on_random",    ENT_MOB     },
    { TRIG_SPEECH,      "on_speech",    ENT_MOB     },
    { TRIG_EXIT,        "on_exit",      ENT_MOB     },
    { TRIG_EXALL,       "on_exit",      ENT_MOB     },
    { TRIG_DELAY,       "on_delay",     ENT_MOB     },
    { TRIG_SURR,        "on_surr",      ENT_MOB     },
    { TRIG_LOGIN,       "on_login",     ENT_ROOM    },
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
    exit(-1);
}

