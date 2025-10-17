////////////////////////////////////////////////////////////////////////////////
// damage.c
////////////////////////////////////////////////////////////////////////////////

#include "damage.h"

#include "lox/lox.h"
#include "lox/vm.h"

#include "db.h"

const DamageInfo damage_table[DAM_TYPE_COUNT] = {
    { DAM_NONE,         "none",       IMM_NONE,       RES_NONE,       VULN_NONE,      },
    { DAM_BASH,         "bash",       IMM_BASH,       RES_BASH,       VULN_BASH,      },
    { DAM_PIERCE,       "pierce",     IMM_PIERCE,     RES_PIERCE,     VULN_PIERCE,    },
    { DAM_SLASH,        "slash",      IMM_SLASH,      RES_SLASH,      VULN_SLASH,     },
    { DAM_FIRE,         "fire",       IMM_FIRE,       RES_FIRE,       VULN_FIRE,      },
    { DAM_COLD,         "cold",       IMM_COLD,       RES_COLD,       VULN_COLD,      },
    { DAM_LIGHTNING,    "lightning",  IMM_LIGHTNING,  RES_LIGHTNING,  VULN_LIGHTNING, },
    { DAM_ACID,         "acid",       IMM_ACID,       RES_ACID,       VULN_ACID,      },
    { DAM_POISON,       "poison",     IMM_POISON,     RES_POISON,     VULN_POISON,    },
    { DAM_NEGATIVE,     "negative",   IMM_NEGATIVE,   RES_NEGATIVE,   VULN_NEGATIVE,  },
    { DAM_HOLY,         "holy",       IMM_HOLY,       RES_HOLY,       VULN_HOLY,      },
    { DAM_ENERGY,       "energy",     IMM_ENERGY,     RES_ENERGY,     VULN_ENERGY,    },
    { DAM_MENTAL,       "mental",     IMM_MENTAL,     RES_MENTAL,     VULN_MENTAL,    },
    { DAM_DISEASE,      "disease",    IMM_DISEASE,    RES_DISEASE,    VULN_DISEASE,   },
    { DAM_DROWNING,     "drowning",   IMM_DROWNING,   RES_DROWNING,   VULN_DROWNING,  },
    { DAM_LIGHT,        "light",      IMM_LIGHT,      RES_LIGHT,      VULN_LIGHT,     },
    { DAM_OTHER,        "other",      IMM_NONE,       RES_NONE,       VULN_NONE,      },
    { DAM_HARM,         "harm",       IMM_NONE,       RES_NONE,       VULN_NONE,      },
    { DAM_CHARM,        "charm",      IMM_CHARM,      RES_CHARM,      VULN_CHARM,     },
    { DAM_SOUND,        "sound",      IMM_SOUND,      RES_SOUND,      VULN_SOUND,     },
};

const AttackInfo attack_table[ATTACK_COUNT] = {
    { "none",           "hit",              DAM_NONE        },
    { "slice",          "slice",            DAM_SLASH       },
    { "stab",           "stab",             DAM_PIERCE      },
    { "slash",          "slash",            DAM_SLASH       },
    { "whip",           "whip",             DAM_SLASH       },
    { "claw",           "claw",             DAM_SLASH       },
    { "blast",          "blast",            DAM_BASH        },
    { "pound",          "pound",            DAM_BASH        },
    { "crush",          "crush",            DAM_BASH        },
    { "grep",           "grep",             DAM_SLASH       },
    { "bite",           "bite",             DAM_PIERCE      },
    { "pierce",         "pierce",           DAM_PIERCE      },
    { "suction",        "suction",          DAM_BASH        },
    { "beating",        "beating",          DAM_BASH        },
    { "digestion",      "digestion",        DAM_ACID        },
    { "charge",         "charge",           DAM_BASH        },
    { "slap",           "slap",             DAM_BASH        },
    { "punch",          "punch",            DAM_BASH        },
    { "wrath",          "wrath",            DAM_ENERGY      },
    { "magic",          "magic",            DAM_ENERGY      },
    { "divine",         "divine power",     DAM_HOLY        },
    { "cleave",         "cleave",           DAM_SLASH       },
    { "scratch",        "scratch",          DAM_PIERCE      },
    { "peck",           "peck",             DAM_PIERCE      },
    { "peckb",          "peck",             DAM_BASH        },
    { "chop",           "chop",             DAM_SLASH       },
    { "sting",          "sting",            DAM_PIERCE      },
    { "smash",          "smash",            DAM_BASH        },
    { "shbite",         "shocking bite",    DAM_LIGHTNING   },
    { "flbite",         "flaming bite",     DAM_FIRE        },
    { "frbite",         "freezing bite",    DAM_COLD        },
    { "acbite",         "acidic bite",      DAM_ACID        },
    { "chomp",          "chomp",            DAM_PIERCE      },
    { "drain",          "life drain",       DAM_NEGATIVE    },
    { "thrust",         "thrust",           DAM_PIERCE      },
    { "slime",          "slime",            DAM_ACID        },
    { "shock",          "shock",            DAM_LIGHTNING   },
    { "thwack",         "thwack",           DAM_BASH        },
    { "flame",          "flame",            DAM_FIRE        },
    { "chill",          "chill",            DAM_COLD        },
};

////////////////////////////////////////////////////////////////////////////////
// Lox implementation
////////////////////////////////////////////////////////////////////////////////

void init_damage_consts()
{
    static char* damtype_start =
        "class damage_type_t { "
        "   init() { ";
    
    static char* damtype_end =
        "   }"
        "}"
        "var DamageType = damage_type_t();";
    
    INIT_BUF(src, MSL);
    
    add_buf(src, damtype_start);
    
    for (int i = 0; i < DAM_TYPE_COUNT; ++i) {
        addf_buf(src, "       this.%s = %d;", pascal_case(damage_table[i].name),
            damage_table[i].type);
    }
    
    add_buf(src, damtype_end);
    
    InterpretResult result = interpret_code(src->string);
    
    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
    
    free_buf(src);
}