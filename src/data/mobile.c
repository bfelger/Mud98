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

const SexInfo sex_table[SEX_MAX] = {
    { SEX_NEUTRAL,  "none",     "it",   "It",   "it",   "its",     },
    { SEX_MALE,     "male",     "he",   "He",   "him",  "his",     },
    { SEX_FEMALE,   "female",   "she",  "She",  "her",  "her",     },
    { SEX_EITHER,   "either",   "they", "They", "them", "their",   },
};

const MobSizeInfo mob_size_table[MOB_SIZE_MAX] = {
    { SIZE_TINY,    "tiny"    },
    { SIZE_SMALL,   "small"   },
    { SIZE_MEDIUM,  "medium"  },
    { SIZE_LARGE,   "large"   },
    { SIZE_HUGE,    "huge"    },
    { SIZE_GIANT,   "giant"   },
};
