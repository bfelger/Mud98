////////////////////////////////////////////////////////////////////////////////
// data/events.c
////////////////////////////////////////////////////////////////////////////////

#include "events.h"

#include <lox/object.h>

#include <entities/entity.h>

#include <db.h>

const EventTypeInfo event_type_info_table[TRIG_COUNT] = {
    { TRIG_ACT,         "act",      "on_act",       ENT_ALL,    CRIT_STR,       0       },
    { TRIG_ATTACKED,    "attacked", "on_attacked",  ENT_MOB,    CRIT_INT,       "100"   },
    { TRIG_BRIBE,       "bribe",    "on_bribe",     ENT_MOB,    CRIT_INT,       "0"     },
    { TRIG_DEATH,       "death",    "on_death",     ENT_MOB,    CRIT_NONE,      0       },
    { TRIG_ENTRY,       "entry",    "on_entry",     ENT_MOB,    CRIT_INT,       "100"   },
    { TRIG_FIGHT,       "fight",    "on_fight",     ENT_MOB,    CRIT_INT,       "100"   },
    { TRIG_GIVE,        "give",     "on_give",      ENT_MOB,    CRIT_INT_OR_STR,0       },
    { TRIG_GREET,       "greet",    "on_greet",     ENT_ALL,    CRIT_NONE,      0       },
    { TRIG_GRALL,       "grall",    "on_greet",     ENT_MOB,    CRIT_NONE,      0       },
    { TRIG_HPCNT,       "hpcnt",    "on_hpcnt",     ENT_MOB,    CRIT_INT,       "100"   },
    { TRIG_RANDOM,      "random",   "on_random",    ENT_MOB,    CRIT_INT,       "100"   },
    { TRIG_SPEECH,      "speech",   "on_speech",    ENT_MOB,    CRIT_STR,       0       },
    { TRIG_EXIT,        "exit",     "on_exit",      ENT_MOB,    CRIT_INT,       0       },
    { TRIG_EXALL,       "exall",    "on_exit",      ENT_MOB,    CRIT_INT,       0       },
    { TRIG_DELAY,       "delay",    "on_delay",     ENT_MOB,    CRIT_INT,       "1"     },
    { TRIG_SURR,        "surr",     "on_surr",      ENT_MOB,    CRIT_INT,       "100"   },
    { TRIG_LOGIN,       "login",    "on_login",     ENT_ROOM,   CRIT_NONE,      0       },
    { TRIG_GIVEN,       "given",    "on_given",     ENT_OBJ,    CRIT_NONE,      0       },
    { TRIG_TAKEN,       "taken",    "on_taken",     ENT_OBJ,    CRIT_NONE,      0       },
    { TRIG_DROPPED,     "dropped",  "on_dropped",   ENT_OBJ,    CRIT_NONE,      0       },
    { TRIG_PRDSTART,    "prdstart", "on_prdstart",  ENT_PLACE,  CRIT_STR,       0       },
    { TRIG_PRDSTOP,     "prdstop",  "on_prdstop",   ENT_PLACE,  CRIT_STR,       0       },
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

EventTrigger get_event_type_info_by_name(const char* name) 
{
    if (IS_NULLSTR(name))
        return NO_FLAG;

    for (int i = 0; i < TRIG_COUNT; i++) {
        if (str_prefix(event_type_info_table[i].name, name) == 0)
            return event_type_info_table[i].trigger;
    }

    return NO_FLAG;
}

const char* get_event_default_callback(EventTrigger trigger)
{
    for (int i = 0; i < TRIG_COUNT; i++) {
        if (event_type_info_table[i].trigger == trigger)
            return event_type_info_table[i].default_callback;
    }

    fprintf(stderr, "get_event_default_callback: Unknown trigger type '%d'.\n",
        trigger);
    return "";
}

const char* get_event_name(EventTrigger trigger)
{
    for (int i = 0; i < TRIG_COUNT; i++) {
        if (event_type_info_table[i].trigger == trigger)
            return event_type_info_table[i].name;
    }

    fprintf(stderr, "get_event_name: Unknown trigger type '%d'.\n",
        trigger);
    return "";
}

const EventTypeInfo* get_event_type_info(EventTrigger trigger)
{
    for (int i = 0; i < TRIG_COUNT; ++i) {
        if (event_type_info_table[i].trigger == trigger)
            return &event_type_info_table[i];
    }

    fprintf(stderr, "get_event_type_info: Unknown trigger type '%d'.\n",
        trigger);
        
    return NULL;
}

