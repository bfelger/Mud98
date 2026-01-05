////////////////////////////////////////////////////////////////////////////////
// data/item.c
////////////////////////////////////////////////////////////////////////////////

#include "item.h"

#include <db.h>
#include <gsn.h>

static const char* armor_type_names[ARMOR_TYPE_COUNT] = {
    "old_style",
    "cloth",
    "light",
    "medium",
    "heavy",
};

const char* armor_type_name(ArmorTier type)
{
    if (type < 0 || type >= ARMOR_TYPE_COUNT)
        return armor_type_names[ARMOR_OLD_STYLE];
    return armor_type_names[type];
}

int armor_type_lookup(const char* name)
{
    if (IS_NULLSTR(name))
        return -1;

    for (int i = 0; i < ARMOR_TYPE_COUNT; i++) {
        if (!str_prefix(name, armor_type_names[i]))
            return i;
    }

    return -1;
}

ArmorTier armor_type_from_value(int value)
{
    if (value < ARMOR_OLD_STYLE || value >= ARMOR_TYPE_COUNT)
        return ARMOR_OLD_STYLE;

    return (ArmorTier)value;
}

bool armor_type_read(void* temp, char* arg)
{
    int* armor_type = (int*)temp;
    int value = armor_type_lookup(arg);

    if (value < 0) {
        *armor_type = ARMOR_OLD_STYLE;
        return false;
    }

    *armor_type = (int16_t)value;
    return true;
}

const char* armor_type_str(void* temp)
{
    int* armor_type = (int*)temp;
    return armor_type_name(armor_type_from_value(*armor_type));
}

const ItemInfo item_type_table[ITEM_TYPE_COUNT] = {
    { ITEM_NONE,            "none"          },
    { ITEM_LIGHT,           "light"         },
    { ITEM_SCROLL,          "scroll"        },
    { ITEM_WAND,            "wand"          },
    { ITEM_STAFF,           "staff"         },
    { ITEM_WEAPON,          "weapon"        },
    { ITEM_NONE,            ""              },
    { ITEM_NONE,            ""              },
    { ITEM_TREASURE,        "treasure"      },
    { ITEM_ARMOR,           "armor"         },
    { ITEM_POTION,          "potion"        },
    { ITEM_CLOTHING,        "clothing"      },
    { ITEM_FURNITURE,       "furniture"     },
    { ITEM_TRASH,           "trash"         },
    { ITEM_NONE,            ""              },
    { ITEM_CONTAINER,       "container"     },
    { ITEM_NONE,            ""              },
    { ITEM_DRINK_CON,       "drink"         },
    { ITEM_KEY,             "key"           },
    { ITEM_FOOD,            "food"          },
    { ITEM_MONEY,           "money"         },
    { ITEM_NONE,            ""              },
    { ITEM_BOAT,            "boat"          },
    { ITEM_CORPSE_NPC,      "npc_corpse"    },
    { ITEM_CORPSE_PC,       "pc_corpse"     },
    { ITEM_FOUNTAIN,        "fountain"      },
    { ITEM_PILL,            "pill"          },
    { ITEM_PROTECT,         "protect"       },
    { ITEM_MAP,             "map"           },
    { ITEM_PORTAL,          "portal"        },
    { ITEM_WARP_STONE,      "warp_stone"    },
    { ITEM_ROOM_KEY,        "room_key"      },
    { ITEM_GEM,             "gem"           },
    { ITEM_JEWELRY,         "jewelry"       },
    { ITEM_JUKEBOX,         "jukebox"       },
    { ITEM_MAT,             "material"      },
    { ITEM_WORKSTATION,     "workstation"   },
};

const char* item_name(ItemType type)
{
    for (int i = 0; i < ITEM_TYPE_COUNT; i++) {
        if (type == item_type_table[i].type)
            return item_type_table[i].name;
    }

    return "";
}

const LiquidInfo liquid_table[LIQ_COUNT] = {
    // name			        color	           proof full thirst food ssize
    { "water",              "clear",            0,    1,    10,   0,   16   },
    { "beer",               "amber",            12,   1,    8,    1,   12   },
    { "red wine",           "burgundy",         30,   1,    8,    1,   5    },
    { "ale",                "brown",            15,   1,    8,    1,   12   },
    { "dark ale",           "dark",             16,   1,    8,    1,   12   },
    { "whisky",             "golden",           120,  1,    5,    0,   2    },
    { "lemonade",           "pink",             0,    1,    9,    2,   12   },
    { "firebreather",       "boiling",          190,  0,    4,    0,   2    },
    { "local specialty",    "clear",            151,  1,    3,    0,   2    },
    { "slime mold juice",   "green",            0,    2,    -8,   1,   2    },
    { "milk",               "white",            0,    2,    9,    3,   12   },
    { "tea",                "tan",              0,    1,    8,    0,   6    },
    { "coffee",             "black",            0,    1,    8,    0,   6    },
    { "blood",              "red",              0,    2,    -1,   2,   6    },
    { "salt water",         "clear",            0,    1,    -2,   0,   1    },
    { "coke",               "brown",            0,    2,    9,    2,   12   },
    { "root beer",          "brown",            0,    2,    9,    2,   12   },
    { "elvish wine",        "green",            35,   2,    8,    1,   5    },
    { "white wine",         "golden",           28,   1,    8,    1,   5    },
    { "champagne",          "golden",           32,   1,    8,    1,   5    },
    { "mead",               "honey-colored",    34,   2,    8,    2,   12   },
    { "rose wine",          "pink",             26,   1,    8,    1,   5    },
    { "benedictine wine",   "burgundy",         40,   1,    8,    1,   5    },
    { "vodka",              "clear",            130,  1,    5,    0,   2    },
    { "cranberry juice",    "red",              0,    1,    9,    2,   12   },
    { "orange juice",       "orange",           0,    2,    9,    3,   12   },
    { "absinthe",           "green",            200,  1,    4,    0,   2    },
    { "brandy",             "golden",           80,   1,    5,    0,   4    },
    { "aquavit",            "clear",            140,  1,    5,    0,   2    },
    { "schnapps",           "clear",            90,   1,    5,    0,   2    },
    { "icewine",            "purple",           50,   2,    6,    1,   5    },
    { "amontillado",        "burgundy",         35,   2,    8,    1,   5    },
    { "sherry",             "red",              38,   2,    7,    1,   5    },
    { "framboise",          "red",              50,   1,    7,    1,   5    },
    { "rum",                "amber",            151,  1,    4,    0,   2    },
    { "cordial",            "clear",            100,  1,    5,    0,   2    },
};

const WeaponInfo weapon_table[WEAPON_TYPE_COUNT] = {
    { WEAPON_EXOTIC,    "exotic",       &gsn_exotic,    OBJ_VNUM_SCHOOL_SWORD   },
    { WEAPON_SWORD,     "sword",        &gsn_sword,     OBJ_VNUM_SCHOOL_SWORD   },
    { WEAPON_DAGGER,    "dagger",       &gsn_dagger,    OBJ_VNUM_SCHOOL_DAGGER  },
    { WEAPON_SPEAR,     "staff",        &gsn_spear,     OBJ_VNUM_SCHOOL_STAFF   },
    { WEAPON_MACE,      "mace",         &gsn_mace,      OBJ_VNUM_SCHOOL_MACE    },
    { WEAPON_AXE,       "axe",          &gsn_axe,       OBJ_VNUM_SCHOOL_AXE     },
    { WEAPON_FLAIL,     "flail",        &gsn_flail,     OBJ_VNUM_SCHOOL_FLAIL   },
    { WEAPON_WHIP,      "whip",         &gsn_whip,      OBJ_VNUM_SCHOOL_WHIP    },
    { WEAPON_POLEARM,   "polearm",      &gsn_polearm,   OBJ_VNUM_SCHOOL_POLEARM },
};

struct wear_type {
    WearLocation wear_loc;
    WearFlags wear_bit;
};

const struct wear_type wear_table[] = {
    { WEAR_UNHELD,      ITEM_TAKE           },
    { WEAR_LIGHT,       ITEM_TAKE           },
    { WEAR_FINGER_L,    ITEM_WEAR_FINGER    },
    { WEAR_FINGER_R,    ITEM_WEAR_FINGER    },
    { WEAR_NECK_1,      ITEM_WEAR_NECK      },
    { WEAR_NECK_2,      ITEM_WEAR_NECK      },
    { WEAR_BODY,        ITEM_WEAR_BODY      },
    { WEAR_HEAD,        ITEM_WEAR_HEAD      },
    { WEAR_LEGS,        ITEM_WEAR_LEGS      },
    { WEAR_FEET,        ITEM_WEAR_FEET      },
    { WEAR_HANDS,       ITEM_WEAR_HANDS     },
    { WEAR_ARMS,        ITEM_WEAR_ARMS      },
    { WEAR_SHIELD,      ITEM_WEAR_SHIELD    },
    { WEAR_ABOUT,       ITEM_WEAR_ABOUT     },
    { WEAR_WAIST,       ITEM_WEAR_WAIST     },
    { WEAR_WRIST_L,     ITEM_WEAR_WRIST     },
    { WEAR_WRIST_R,     ITEM_WEAR_WRIST     },
    { WEAR_WIELD,       ITEM_WIELD          },
    { WEAR_HOLD,        ITEM_HOLD           },
    { WEAR_WIELD_OH,    ITEM_WIELD          },
    { NO_FLAG,          NO_FLAG             }
};

char* const where_name[WEAR_LOC_COUNT] = {
    "<used as light>     ",
    "<worn on finger>    ",
    "<worn on finger>    ",
    "<worn around neck>  ",
    "<worn around neck>  ",
    "<worn on torso>     ",
    "<worn on head>      ",
    "<worn on legs>      ",
    "<worn on feet>      ",
    "<worn on hands>     ",
    "<worn on arms>      ",
    "<worn as shield>    ",
    "<worn about body>   ",
    "<worn about waist>  ",
    "<worn around wrist> ",
    "<worn around wrist> ",
    "<wielded>           ",
    "<held>              ",
    "<floating nearby>   ",
    "<wielded off-hand>  ",
};

/*****************************************************************************
 Name:          wear_bit
 Purpose:       Converts a wear_loc into a bit.
 Called by:     redit_oreset (redit.c).
 ****************************************************************************/
WearFlags wear_bit(WearLocation loc)
{
    int flag;

    for (flag = 0; wear_table[flag].wear_loc != NO_FLAG; flag++) {
        if (loc == wear_table[flag].wear_loc)
            return wear_table[flag].wear_bit;
    }

    return 0;
}

/*****************************************************************************
 Name:          wear_loc
 Purpose:       Returns the location of the bit that matches the count.
                1 = first match, 2 = second match etc.
 Called by:     oedit_reset (oedit.c).
 ****************************************************************************/
WearLocation wear_loc(WearFlags bits, int count)
{
    int flag;

    for (flag = 0; wear_table[flag].wear_bit != NO_FLAG; flag++) {
        if (IS_SET(bits, wear_table[flag].wear_bit) && --count < 1)
            return wear_table[flag].wear_loc;
    }

    return NO_FLAG;
}
