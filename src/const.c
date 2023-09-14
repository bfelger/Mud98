/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@hypercube.org)                                *
 *      Gabrielle Taylor (gtaylor@hypercube.org)                           *
 *      Brian Moore (zump@rom.org)                                         *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

#include "comm.h"
#include "interp.h"
#include "spell_list.h"

#include "entities/object_data.h"

#include "data/damage.h"
#include "data/item.h"

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

/* wiznet table and prototype for future flag setting */
const struct wiznet_type wiznet_table[] = {
    { "on",         WIZ_ON,         IM  },
    { "prefix",     WIZ_PREFIX,     IM  },
    { "ticks",      WIZ_TICKS,      IM  },
    { "logins",     WIZ_LOGINS,     IM  },
    { "sites",      WIZ_SITES,      L4  },
    { "links",      WIZ_LINKS,      L7  },
    { "newbies",    WIZ_NEWBIE,     IM  },
    { "spam",       WIZ_SPAM,       L5  },
    { "deaths",     WIZ_DEATHS,     IM  },
    { "resets",     WIZ_RESETS,     L4  },
    { "mobdeaths",  WIZ_MOBDEATHS,  L4  },
    { "flags",      WIZ_FLAGS,      L5  },
    { "penalties",  WIZ_PENALTIES,  L5  },
    { "saccing",    WIZ_SACCING,    L5  },
    { "levels",     WIZ_LEVELS,     IM  },
    { "load",       WIZ_LOAD,       L2  },
    { "restore",    WIZ_RESTORE,    L2  },
    { "snoops",     WIZ_SNOOPS,     L2  },
    { "switches",   WIZ_SWITCHES,   L2  },
    { "secure",     WIZ_SECURE,     L1  },
    { NULL,         0,              0   }
};

/*
 * Class table.
 */
const struct class_type class_table[MAX_CLASS] = {
    {
        "mage",
        "Mag",
        STAT_INT,
        OBJ_VNUM_SCHOOL_DAGGER,
        {3018, 9618},
        75,
        20,
        6,
        6,
        8,
        true,
        "mage basics",
        "mage default"
    },
    {
        "cleric",
        "Cle",
        STAT_WIS,
        OBJ_VNUM_SCHOOL_MACE,
        {3003, 9619},
        75,
        20,
        2,
        7,
        10,
        true,
        "cleric basics",
        "cleric default"
    },
    {
        "thief",
        "Thi",
        STAT_DEX,
        OBJ_VNUM_SCHOOL_DAGGER,
        {3028, 9639},
        75,
        20,
        -4,
        8,
        13,
        false,
        "thief basics",
        "thief default"
    },
    {
        "warrior",
        "War",
        STAT_STR,
        OBJ_VNUM_SCHOOL_SWORD,
        {3022, 9633},
        75,
        20,
        -10,
        11,
        15,
        false,
        "warrior basics",
        "warrior default"
    },
};

/*
 * Titles.
 */
char* const title_table[MAX_CLASS][MAX_LEVEL + 1][2]
    = {{{"Man", "Woman"},

        {"Apprentice of Magic", "Apprentice of Magic"},
        {"Spell Student", "Spell Student"},
        {"Scholar of Magic", "Scholar of Magic"},
        {"Delver in Spells", "Delveress in Spells"},
        {"Medium of Magic", "Medium of Magic"},

        {"Scribe of Magic", "Scribess of Magic"},
        {"Seer", "Seeress"},
        {"Sage", "Sage"},
        {"Illusionist", "Illusionist"},
        {"Abjurer", "Abjuress"},

        {"Invoker", "Invoker"},
        {"Enchanter", "Enchantress"},
        {"Conjurer", "Conjuress"},
        {"Magician", "Witch"},
        {"Creator", "Creator"},

        {"Savant", "Savant"},
        {"Magus", "Craftess"},
        {"Wizard", "Wizard"},
        {"Warlock", "War Witch"},
        {"Sorcerer", "Sorceress"},

        {"Elder Sorcerer", "Elder Sorceress"},
        {"Grand Sorcerer", "Grand Sorceress"},
        {"Great Sorcerer", "Great Sorceress"},
        {"Golem Maker", "Golem Maker"},
        {"Greater Golem Maker", "Greater Golem Maker"},

        {"Maker of Stones", "Maker of Stones"},
        {"Maker of Potions", "Maker of Potions"},
        {"Maker of Scrolls", "Maker of Scrolls"},
        {"Maker of Wands", "Maker of Wands"},
        {"Maker of Staves", "Maker of Staves"},

        {"Demon Summoner", "Demon Summoner"},
        {"Greater Demon Summoner", "Greater Demon Summoner"},
        {"Dragon Charmer", "Dragon Charmer"},
        {"Greater Dragon Charmer", "Greater Dragon Charmer"},
        {"Master of all Magic", "Master of all Magic"},

        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},

        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},

        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},
        {"Master Mage", "Master Mage"},

        {"Mage Hero", "Mage Heroine"},
        {"Avatar of Magic", "Avatar of Magic"},
        {"Angel of Magic", "Angel of Magic"},
        {"Demigod of Magic", "Demigoddess of Magic"},
        {"Immortal of Magic", "Immortal of Magic"},
        {"God of Magic", "Goddess of Magic"},
        {"Deity of Magic", "Deity of Magic"},
        {"Supremity of Magic", "Supremity of Magic"},
        {"Creator", "Creator"},
        {"Implementor", "Implementress"}},

       {{"Man", "Woman"},

        {"Believer", "Believer"},
        {"Attendant", "Attendant"},
        {"Acolyte", "Acolyte"},
        {"Novice", "Novice"},
        {"Missionary", "Missionary"},

        {"Adept", "Adept"},
        {"Deacon", "Deaconess"},
        {"Vicar", "Vicaress"},
        {"Priest", "Priestess"},
        {"Minister", "Lady Minister"},

        {"Canon", "Canon"},
        {"Levite", "Levitess"},
        {"Curate", "Curess"},
        {"Monk", "Nun"},
        {"Healer", "Healess"},

        {"Chaplain", "Chaplain"},
        {"Expositor", "Expositress"},
        {"Bishop", "Bishop"},
        {"Arch Bishop", "Arch Lady of the Church"},
        {"Patriarch", "Matriarch"},

        {"Elder Patriarch", "Elder Matriarch"},
        {"Grand Patriarch", "Grand Matriarch"},
        {"Great Patriarch", "Great Matriarch"},
        {"Demon Killer", "Demon Killer"},
        {"Greater Demon Killer", "Greater Demon Killer"},

        {"Cardinal of the Sea", "Cardinal of the Sea"},
        {"Cardinal of the Earth", "Cardinal of the Earth"},
        {"Cardinal of the Air", "Cardinal of the Air"},
        {"Cardinal of the Ether", "Cardinal of the Ether"},
        {"Cardinal of the Heavens", "Cardinal of the Heavens"},

        {"Avatar of an Immortal", "Avatar of an Immortal"},
        {"Avatar of a Deity", "Avatar of a Deity"},
        {"Avatar of a Supremity", "Avatar of a Supremity"},
        {"Avatar of an Implementor", "Avatar of an Implementor"},
        {"Master of all Divinity", "Mistress of all Divinity"},

        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},

        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},

        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},
        {"Master Cleric", "Master Cleric"},

        {"Holy Hero", "Holy Heroine"},
        {"Holy Avatar", "Holy Avatar"},
        {"Angel", "Angel"},
        {"Demigod",  "Demigoddess"},
        {"Immortal", "Immortal"},
        {"God", "Goddess"},
        {"Deity", "Deity"},
        {"Supreme Master", "Supreme Mistress"},
        {"Creator", "Creator"},
        {"Implementor", "Implementress"}},

       {{"Man", "Woman"},

        {"Pilferer", "Pilferess"},
        {"Footpad", "Footpad"},
        {"Filcher", "Filcheress"},
        {"Pick-Pocket", "Pick-Pocket"},
        {"Sneak", "Sneak"},

        {"Pincher", "Pincheress"},
        {"Cut-Purse", "Cut-Purse"},
        {"Snatcher", "Snatcheress"},
        {"Sharper", "Sharpress"},
        {"Rogue", "Rogue"},

        {"Robber", "Robber"},
        {"Magsman", "Magswoman"},
        {"Highwayman", "Highwaywoman"},
        {"Burglar", "Burglaress"},
        {"Thief", "Thief"},

        {"Knifer", "Knifer"},
        {"Quick-Blade", "Quick-Blade"},
        {"Killer", "Murderess"},
        {"Brigand", "Brigand"},
        {"Cut-Throat", "Cut-Throat"},

        {"Spy", "Spy"},
        {"Grand Spy", "Grand Spy"},
        {"Master Spy", "Master Spy"},
        {"Assassin", "Assassin"},
        {"Greater Assassin", "Greater Assassin"},

        {"Master of Vision", "Mistress of Vision"},
        {"Master of Hearing", "Mistress of Hearing"},
        {"Master of Smell", "Mistress of Smell"},
        {"Master of Taste", "Mistress of Taste"},
        {"Master of Touch", "Mistress of Touch"},

        {"Crime Lord", "Crime Mistress"},
        {"Infamous Crime Lord", "Infamous Crime Mistress"},
        {"Greater Crime Lord", "Greater Crime Mistress"},
        {"Master Crime Lord", "Master Crime Mistress"},
        {"Godfather", "Godmother"},

        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},

        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},

        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},
        {"Master Thief", "Master Thief"},

        {"Assassin Hero", "Assassin Heroine"},
        {"Avatar of Death", "Avatar of Death"},
        {"Angel of Death", "Angel of Death"},
        {"Demigod of Assassins", "Demigoddess of Assassins"},
        {"Immortal Assasin", "Immortal Assassin"},
        {"God of Assassins", "God of Assassins"},
        {"Deity of Assassins", "Deity of Assassins"},
        {"Supreme Master", "Supreme Mistress"},
        {"Creator", "Creator"},
        {"Implementor", "Implementress"}},

       {{"Man", "Woman"},

        {"Swordpupil", "Swordpupil"},
        {"Recruit", "Recruit"},
        {"Sentry", "Sentress"},
        {"Fighter", "Fighter"},
        {"Soldier", "Soldier"},

        {"Warrior", "Warrior"},
        {"Veteran", "Veteran"},
        {"Swordsman", "Swordswoman"},
        {"Fencer", "Fenceress"},
        {"Combatant", "Combatess"},

        {"Hero", "Heroine"},
        {"Myrmidon", "Myrmidon"},
        {"Swashbuckler", "Swashbuckleress"},
        {"Mercenary", "Mercenaress"},
        {"Swordmaster", "Swordmistress"},

        {"Lieutenant", "Lieutenant"},
        {"Champion", "Lady Champion"},
        {"Dragoon", "Lady Dragoon"},
        {"Cavalier", "Lady Cavalier"},
        {"Knight", "Lady Knight"},

        {"Grand Knight", "Grand Knight"},
        {"Master Knight", "Master Knight"},
        {"Paladin", "Paladin"},
        {"Grand Paladin", "Grand Paladin"},
        {"Demon Slayer", "Demon Slayer"},

        {"Greater Demon Slayer", "Greater Demon Slayer"},
        {"Dragon Slayer", "Dragon Slayer"},
        {"Greater Dragon Slayer", "Greater Dragon Slayer"},
        {"Underlord", "Underlord"},
        {"Overlord", "Overlord"},

        {"Baron of Thunder", "Baroness of Thunder"},
        {"Baron of Storms", "Baroness of Storms"},
        {"Baron of Tornadoes", "Baroness of Tornadoes"},
        {"Baron of Hurricanes", "Baroness of Hurricanes"},
        {"Baron of Meteors", "Baroness of Meteors"},

        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},

        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},

        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},
        {"Master Warrior", "Master Warrior"},

        {"Knight Hero", "Knight Heroine"},
        {"Avatar of War", "Avatar of War"},
        {"Angel of War", "Angel of War"},
        {"Demigod of War", "Demigoddess of War"},
        {"Immortal Warlord", "Immortal Warlord"},
        {"God of War", "God of War"},
        {"Deity of War", "Deity of War"},
        {"Supreme Master of War", "Supreme Mistress of War"},
        {"Creator", "Creator"},
        {"Implementor", "Implementress"}}};

/*
 * Attribute bonus tables.
 */
const struct str_app_type str_app[26] = {
    {-5, -4, 0, 0}, /* 0  */
    {-5, -4, 3, 1}, /* 1  */
    {-3, -2, 3, 2},  {-3, -1, 10, 3}, /* 3  */
    {-2, -1, 25, 4}, {-2, -1, 55, 5}, /* 5  */
    {-1, 0, 80, 6},  {-1, 0, 90, 7},  {0, 0, 100, 8},
    {0, 0, 100, 9},  {0, 0, 115, 10}, /* 10  */
    {0, 0, 115, 11}, {0, 0, 130, 12}, {0, 0, 130, 13}, /* 13  */
    {0, 1, 140, 14}, {1, 1, 150, 15}, /* 15  */
    {1, 2, 165, 16}, {2, 3, 180, 22}, {2, 3, 200, 25}, /* 18  */
    {3, 4, 225, 30}, {3, 5, 250, 35}, /* 20  */
    {4, 6, 300, 40}, {4, 6, 350, 45}, {5, 7, 400, 50},
    {5, 8, 450, 55}, {6, 9, 500, 60} /* 25   */
};

const struct int_app_type int_app[26] = {
    {3}, /*  0 */
    {5}, /*  1 */
    {7},  {8}, /*  3 */
    {9},  {10}, /*  5 */
    {11}, {12}, {13}, {15}, {17}, /* 10 */
    {19}, {22}, {25}, {28}, {31}, /* 15 */
    {34}, {37}, {40}, /* 18 */
    {44}, {49}, /* 20 */
    {55}, {60}, {70}, {80}, {85} /* 25 */
};

const struct wis_app_type wis_app[26] = {
    {0}, /*  0 */
    {0}, /*  1 */
    {0}, {0}, /*  3 */
    {0}, {1}, /*  5 */
    {1}, {1}, {1}, {1}, {1}, /* 10 */
    {1}, {1}, {1}, {1}, {2}, /* 15 */
    {2}, {2}, {3}, /* 18 */
    {3}, {3}, /* 20 */
    {3}, {4}, {4}, {4}, {5} /* 25 */
};

const struct dex_app_type dex_app[26] = {
    {60}, /* 0 */
    {50}, /* 1 */
    {50},  {40},  {30},  {20}, /* 5 */
    {10},  {0},   {0},   {0},    {0}, /* 10 */
    {0},   {0},   {0},   {0},    {-10}, /* 15 */
    {-15}, {-20}, {-30}, {-40},  {-50}, /* 20 */
    {-60}, {-75}, {-90}, {-105}, {-120} /* 25 */
};

const struct con_app_type con_app[26] = {
    {-4, 20}, /*  0 */
    {-3, 25}, /*  1 */
    {-2, 30}, {-2, 35}, /*  3 */
    {-1, 40}, {-1, 45}, /*  5 */
    {-1, 50}, {0, 55},  {0, 60}, {0, 65}, {0, 70}, /* 10 */
    {0, 75},  {0, 80},  {0, 85}, {0, 88}, {1, 90}, /* 15 */
    {2, 95},  {2, 97},  {3, 99}, /* 18 */
    {3, 99},  {4, 99}, /* 20 */
    {4, 99},  {5, 99},  {6, 99}, {7, 99}, {8, 99} /* 25 */
};


