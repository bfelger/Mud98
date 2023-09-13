////////////////////////////////////////////////////////////////////////////////
// mobile.c
////////////////////////////////////////////////////////////////////////////////

#include "mobile.h"

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