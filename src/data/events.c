////////////////////////////////////////////////////////////////////////////////
// events.c
////////////////////////////////////////////////////////////////////////////////

#include "events.h"

const EventDefaultConfig event_default_configs[] = {
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

