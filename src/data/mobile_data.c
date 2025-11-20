////////////////////////////////////////////////////////////////////////////////
// data/mobile_data.c
////////////////////////////////////////////////////////////////////////////////

#include "mobile_data.h"

#include <lookup.h>

const PositionInfo position_table[POS_MAX] = {
    { POS_DEAD,         "dead",             "dead"  },
    { POS_MORTAL,       "mortally wounded", "mort"  },
    { POS_INCAP,        "incapacitated",    "incap" },
    { POS_STUNNED,      "stunned",          "stun"  },
    { POS_SLEEPING,     "sleeping",         "sleep" },
    { POS_RESTING,      "resting",          "rest"  },
    { POS_SITTING,      "sitting",          "sit"   },
    { POS_FIGHTING,     "fighting",         "fight" },
    { POS_STANDING,     "standing",         "stand" },
};

const SexInfo sex_table[SEX_COUNT] = {
//    Sex           Name        Subj    Subj_cap    Obj     Poss;
    { SEX_NEUTRAL,  "none",     "it",   "It",       "it",   "its",     },
    { SEX_MALE,     "male",     "he",   "He",       "him",  "his",     },
    { SEX_FEMALE,   "female",   "she",  "She",      "her",  "her",     },
    { SEX_EITHER,   "either",   "they", "They",     "them", "their",   },
    { SEX_YOU,      "you",      "you",  "You",      "You",  "your",    },
};

const MobSizeInfo mob_size_table[MOB_SIZE_COUNT] = {
    { SIZE_TINY,    "tiny"    },
    { SIZE_SMALL,   "small"   },
    { SIZE_MEDIUM,  "medium"  },
    { SIZE_LARGE,   "large"   },
    { SIZE_HUGE,    "huge"    },
    { SIZE_GIANT,   "giant"   },
};

bool position_read(void* temp, char* arg)
{
    int16_t* pos = (int16_t*)temp;
    int16_t ffg = (int16_t)position_lookup(arg);

    *pos = UMAX(0, ffg);

    if (ffg == -1)
        return false;
    else
        return true;
}

const char* position_str(void* temp)
{
    Position* flags = (Position*)temp;
    return position_table[*flags].name;
}

bool size_read(void* temp, char* arg)
{
    int16_t* size = (int16_t*)temp;
    int16_t ffg = (int16_t)size_lookup(arg);

    *size = UMAX(0, ffg);

    if (ffg == -1)
        return false;
    else
        return true;
}

const char* size_str(void* temp)
{
    MobSize* size = (MobSize*)temp;
    return mob_size_table[UMAX(0, *size)].name;
}


