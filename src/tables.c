/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.   *
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

#include "merc.h"

#include "tables.h"

#include "comm.h"

#include <entities/descriptor.h>

#include <data/damage.h>
#include <data/events.h>
#include <data/mobile_data.h>
#include <data/skill.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

/* for clans */
const struct clan_type clan_table[MAX_CLAN] = {
//    name          who entry       death-transfer room     independent
    { "",           "",             ROOM_VNUM_ALTAR,        true        },
    { "loner",      "[ Loner ] ",   ROOM_VNUM_ALTAR,        true        },
    { "rom",        "[  ROM  ] ",   ROOM_VNUM_ALTAR,        false       }
};

/* various flag tables */
const struct flag_type act_flag_table[] = {
    { "npc",            BIT(0),     false   },
    { "sentinel",       BIT(1),     true    },
    { "scavenger",      BIT(2),     true    },
    { "aggressive",     BIT(5),     true    },
    { "stay_area",      BIT(6),     true    },
    { "wimpy",          BIT(7),     true    },
    { "pet",            BIT(8),     true    },
    { "train",          BIT(9),     true    },
    { "practice",       BIT(10),    true    },
    { "undead",         BIT(14),    true    },
    { "cleric",         BIT(16),    true    },
    { "mage",           BIT(17),    true    },
    { "thief",          BIT(18),    true    },
    { "warrior",        BIT(19),    true    },
    { "noalign",        BIT(20),    true    },
    { "nopurge",        BIT(21),    true    },
    { "outdoors",       BIT(22),    true    },
    { "indoors",        BIT(24),    true    },
    { "healer",         BIT(26),    true    },
    { "gain",           BIT(27),    true    },
    { "update_always",  BIT(28),    true    },
    { "changer",        BIT(29),    true    },
    { NULL,             0,          false   }
};

const struct flag_type plr_flag_table[] = {
    { "npc",            BIT(0),     false   },
    { "autoassist",     BIT(2),     false   },
    { "autoexit",       BIT(3),     false   },
    { "autoloot",       BIT(4),     false   },
    { "autosac",        BIT(5),     false   },
    { "autogold",       BIT(6),     false   },
    { "autosplit",      BIT(7),     false   },
    { "tester",         BIT(8),     true    },
    { "holylight",      BIT(13),    false   },
    { "can_loot",       BIT(15),    false   },
    { "nosummon",       BIT(16),    false   },
    { "nofollow",       BIT(17),    false   },
    { "colour",         BIT(19),    false   },
    { "permit",         BIT(20),    true    },
    { "log",            BIT(22),    false   },
    { "deny",           BIT(23),    false   },
    { "freeze",         BIT(24),    false   },
    { "thief",          BIT(25),    false   },
    { "killer",         BIT(26),    false   },
    { NULL,             0,          0       }
};

const struct flag_type affect_flag_table[] = {
    { "blind",          BIT(0),     true    },
    { "invisible",      BIT(1),     true    },
    { "detect_evil",    BIT(2),     true    },  
    { "detect_invis",   BIT(3),     true    },
    { "detect_magic",   BIT(4),     true    }, 
    { "detect_hidden",  BIT(5),     true    },
    { "detect_good",    BIT(6),     true    },  
    { "sanctuary",      BIT(7),     true    },
    { "faerie_fire",    BIT(8),     true    },  
    { "infrared",       BIT(9),     true    },
    { "curse",          BIT(10),    true    },        
    { "poison",         BIT(12),    true    },
    { "protect_evil",   BIT(13),    true    }, 
    { "protect_good",   BIT(14),    true    },
    { "sneak",          BIT(15),    true    },        
    { "hide",           BIT(16),    true    },
    { "sleep",          BIT(17),    true    },        
    { "charm",          BIT(18),    true    },
    { "flying",         BIT(19),    true    },       
    { "pass_door",      BIT(20),    true    },
    { "haste",          BIT(21),    true    },        
    { "calm",           BIT(22),    true    },
    { "plague",         BIT(23),    true    },       
    { "weaken",         BIT(24),    true    },
    { "dark_vision",    BIT(25),    true    },  
    { "berserk",        BIT(26),    true    },
    { "swim",           BIT(27),    true    },        
    { "regeneration",   BIT(28),    true    },
    { "slow",           BIT(29),    true    },        
    { NULL,             0,          0       }
};

const struct flag_type off_flag_table[] = {
    { "area_attack",    BIT(0),     true    },    
    { "backstab",       BIT(1),     true    },
    { "bash",           BIT(2),     true    },           
    { "berserk",        BIT(3),     true    },
    { "disarm",         BIT(4),     true    },         
    { "dodge",          BIT(5),     true    },
    { "fade",           BIT(6),     true    },           
    { "fast",           BIT(7),     true    },
    { "kick",           BIT(8),     true    },           
    { "dirt_kick",      BIT(9),     true    },
    { "parry",          BIT(10),    true    },          
    { "rescue",         BIT(11),    true    },
    { "tail",           BIT(12),    true    },           
    { "trip",           BIT(13),    true    },
    { "crush",          BIT(14),    true    },          
    { "assist_all",     BIT(15),    true    },
    { "assist_align",   BIT(16),    true    },   
    { "assist_race",    BIT(17),    true    },
    { "assist_players", BIT(18),    true    }, 
    { "assist_guard",   BIT(19),    true    },
    { "assist_vnum",    BIT(20),    true    },    
    { NULL,             0,          0       }
};

const struct flag_type imm_flag_table[] = {
    { "summon",         BIT(0),     true    },    
    { "charm",          BIT(1),     true    },   
    { "magic",          BIT(2),     true    },
    { "weapon",         BIT(3),     true    },     
    { "bash",           BIT(4),     true    },    
    { "pierce",         BIT(5),     true    },
    { "slash",          BIT(6),     true    },     
    { "fire",           BIT(7),     true    },    
    { "cold",           BIT(8),     true    },
    { "lightning",      BIT(9),     true    }, 
    { "acid",           BIT(10),    true    },    
    { "poison",         BIT(11),    true    },
    { "negative",       BIT(12),    true    },  
    { "holy",           BIT(13),    true    },    
    { "energy",         BIT(14),    true    },
    { "mental",         BIT(15),    true    },    
    { "disease",        BIT(16),    true    }, 
    { "drowning",       BIT(17),    true    },
    { "light",          BIT(18),    true    },     
    { "sound",          BIT(19),    true    },   
    { "wood",           BIT(23),    true    },
    { "silver",         BIT(24),    true    },    
    { "iron",           BIT(25),    true    },    
    { NULL,             0,          0       }
};

const struct flag_type form_flag_table[] = {
    { "edible",         FORM_EDIBLE,        true    },
    { "poison",         FORM_POISON,        true    },
    { "magical",        FORM_MAGICAL,       true    },
    { "instant_decay",  FORM_INSTANT_DECAY, true    },
    { "other",          FORM_OTHER,         true    },
    { "animal",         FORM_ANIMAL,        true    },
    { "sentient",       FORM_SENTIENT,      true    },
    { "undead",         FORM_UNDEAD,        true    },
    { "construct",      FORM_CONSTRUCT,     true    },
    { "mist",           FORM_MIST,          true    },
    { "intangible",     FORM_INTANGIBLE,    true    },
    { "biped",          FORM_BIPED,         true    },
    { "centaur",        FORM_CENTAUR,       true    },
    { "insect",         FORM_INSECT,        true    },
    { "spider",         FORM_SPIDER,        true    },
    { "crustacean",     FORM_CRUSTACEAN,    true    },
    { "worm",           FORM_WORM,          true    },
    { "blob",           FORM_BLOB,          true    },
    { "mammal",         FORM_MAMMAL,        true    },
    { "bird",           FORM_BIRD,          true    },
    { "reptile",        FORM_REPTILE,       true    },
    { "snake",          FORM_SNAKE,         true    },
    { "dragon",         FORM_DRAGON,        true    },
    { "amphibian",      FORM_AMPHIBIAN,     true    },
    { "fish",           FORM_FISH,          true    },
    { "cold_blood",     FORM_COLD_BLOOD,    true    },
    { NULL,             0,                  0       }
};

const struct flag_type part_flag_table[] = {
    { "head",           PART_HEAD,          true    },
    { "arms",           PART_ARMS,          true    },
    { "legs",           PART_LEGS,          true    },
    { "heart",          PART_HEART,         true    },
    { "brains",         PART_BRAINS,        true    },
    { "guts",           PART_GUTS,          true    },
    { "hands",          PART_HANDS,         true    },
    { "feet",           PART_FEET,          true    },
    { "fingers",        PART_FINGERS,       true    },
    { "ear",            PART_EAR,           true    },
    { "eye",            PART_EYE,           true    },
    { "long_tongue",    PART_LONG_TONGUE,   true    },
    { "eyestalks",      PART_EYESTALKS,     true    },
    { "tentacles",      PART_TENTACLES,     true    },
    { "fins",           PART_FINS,          true    },
    { "wings",          PART_WINGS,         true    },
    { "tail",           PART_TAIL,          true    },
    { "claws",          PART_CLAWS,         true    },
    { "fangs",          PART_FANGS,         true    },
    { "horns",          PART_HORNS,         true    },
    { "scales",         PART_SCALES,        true    },
    { "tusks",          PART_TUSKS,         true    },
    { NULL,             0,                  0       }
};

const struct flag_type comm_flag_table[] = {
    { "quiet",          COMM_QUIET,         true    },
    { "deaf",           COMM_DEAF,          true    },
    { "nowiz",          COMM_NOWIZ,         true    },
    { "noclangossip",   COMM_NOAUCTION,     true    },
    { "nogossip",       COMM_NOGOSSIP,      true    },
    { "noquestion",     COMM_NOQUESTION,    true    },
    { "nomusic",        COMM_NOMUSIC,       true    },
    { "noclan",         COMM_NOCLAN,        true    },
    { "noquote",        COMM_NOQUOTE,       true    },
    { "shoutsoff",      COMM_SHOUTSOFF,     true    },
    { "compact",        COMM_COMPACT,       true    },
    { "brief",          COMM_BRIEF,         true    },
    { "prompt",         COMM_PROMPT,        true    },
    { "combine",        COMM_COMBINE,       true    },
    { "telnet_ga",      COMM_TELNET_GA,     true    },
    { "show_affects",   COMM_SHOW_AFFECTS,  true    },
    { "nograts",        COMM_NOGRATS,       true    },
    { "noemote",        COMM_NOEMOTE,       false   },
    { "noshout",        COMM_NOSHOUT,       false   },
    { "notell",         COMM_NOTELL,        false   },
    { "nochannels",     COMM_NOCHANNELS,    false   },
    { "snoop_proof",    COMM_SNOOP_PROOF,   false   },
    { "afk",            COMM_AFK,           true    },
    { NULL,             0,                  0       }
};

const struct flag_type mprog_flag_table[TRIG_COUNT + 1] = {
    { "act",            TRIG_ACT,           true    },
    { "attacked",       TRIG_ATTACKED,      true    },
    { "bribe",          TRIG_BRIBE,         true    },
    { "death",          TRIG_DEATH,         true    },
    { "entry",          TRIG_ENTRY,         true    },
    { "fight",          TRIG_FIGHT,         true    },
    { "give",           TRIG_GIVE,          true    },
    { "greet",          TRIG_GREET,         true    },
    { "grall",          TRIG_GRALL,         true    },
    { "hpcnt",          TRIG_HPCNT,         true    },
    { "random",         TRIG_RANDOM,        true    },
    { "speech",         TRIG_SPEECH,        true    },
    { "exit",           TRIG_EXIT,          true    },
    { "exall",          TRIG_EXALL,         true    },
    { "delay",          TRIG_DELAY,         true    },
    { "surr",           TRIG_SURR,          true    },
    { "login",          TRIG_LOGIN,         true    },
    { NULL,             0,                  true    }
};

const struct flag_type area_flag_table[] = {
    { "none",           AREA_NONE,          false   },
    { "changed",        AREA_CHANGED,       true    },
    { "added",          AREA_ADDED,         true    },
    { "loading",        AREA_LOADING,       false   },
    { NULL,             0,                  0       }
};

const struct flag_type sex_flag_table[] = {
    { "male",           SEX_MALE,           true    },
    { "female",         SEX_FEMALE,         true    },
    { "neutral",        SEX_NEUTRAL,        true    },
    { "random",         3,                  true    },
    { "none",           SEX_NEUTRAL,        true    },
    { NULL,             0,                  0       }
};

const struct flag_type exit_flag_table[] = {
    { "door",           EX_ISDOOR,          true    },
    { "closed",         EX_CLOSED,          true    },
    { "locked",         EX_LOCKED,          true    },
    { "pickproof",      EX_PICKPROOF,       true    },
    { "nopass",         EX_NOPASS,          true    },
    { "easy",           EX_EASY,            true    },
    { "hard",           EX_HARD,            true    },
    { "infuriating",    EX_INFURIATING,     true    },
    { "noclose",        EX_NOCLOSE,         true    },
    { "nolock",         EX_NOLOCK,          true    },
    { NULL,             0,                  0       }
};

const struct flag_type door_resets[] = {
    { "open and unlocked",  0,              true    },
    { "closed and unlocked",1,              true    },
    { "closed and locked",  2,              true    },
    { NULL,                 0,              0       }
};

const struct flag_type room_flag_table[] = {
    { "dark",           ROOM_DARK,          true    },
    { "no_mob",         ROOM_NO_MOB,        true    },
    { "indoors",        ROOM_INDOORS,       true    },
    { "private",        ROOM_PRIVATE,       true    },
    { "safe",           ROOM_SAFE,          true    },
    { "solitary",       ROOM_SOLITARY,      true    },
    { "pet_shop",       ROOM_PET_SHOP,      true    },
    { "no_recall",      ROOM_NO_RECALL,     true    },
    { "imp_only",       ROOM_IMP_ONLY,      true    },
    { "gods_only",      ROOM_GODS_ONLY,     true    },
    { "heroes_only",    ROOM_HEROES_ONLY,   true    },
    { "newbies_only",   ROOM_NEWBIES_ONLY,  true    },
    { "law",            ROOM_LAW,           true    },
    { "nowhere",        ROOM_NOWHERE,       true    },
    { "recall",         ROOM_RECALL,        true    },
    { NULL,             0,                  0       }
};

const struct flag_type sector_flag_table[] = {
    { "inside",         SECT_INSIDE,        true    },
    { "city",           SECT_CITY,          true    },
    { "field",          SECT_FIELD,         true    },
    { "forest",         SECT_FOREST,        true    },
    { "hills",          SECT_HILLS,         true    },
    { "mountain",       SECT_MOUNTAIN,      true    },
    { "swim",           SECT_WATER_SWIM,    true    },
    { "noswim",         SECT_WATER_NOSWIM,  true    },
    { "unused",         SECT_UNUSED,        true    },
    { "air",            SECT_AIR,           true    },
    { "desert",         SECT_DESERT,        true    },
    { NULL,             0,                  0       }
};

const struct flag_type type_flag_table[] = {
    { "none",           ITEM_NONE,          false   },
    { "light",          ITEM_LIGHT,         true    },
    { "scroll",         ITEM_SCROLL,        true    },
    { "wand",           ITEM_WAND,          true    },
    { "staff",          ITEM_STAFF,         true    },
    { "weapon",         ITEM_WEAPON,        true    },
    { "reserved",       ITEM_NONE,          false   },
    { "reserved",       ITEM_NONE,          false   },
    { "treasure",       ITEM_TREASURE,      true    },
    { "armor",          ITEM_ARMOR,         true    },
    { "potion",         ITEM_POTION,        true    },
    { "clothing",       ITEM_CLOTHING,      true    },
    { "furniture",      ITEM_FURNITURE,     true    },
    { "trash",          ITEM_TRASH,         true    },
    { "reserved",       ITEM_NONE,          false   },
    { "container",      ITEM_CONTAINER,     true    },
    { "reserved",       ITEM_NONE,          false   },
    { "drinkcontainer", ITEM_DRINK_CON,     true    },
    { "key",            ITEM_KEY,           true    },
    { "food",           ITEM_FOOD,          true    },
    { "money",          ITEM_MONEY,         true    },
    { "reserved",       ITEM_NONE,          false   },
    { "boat",           ITEM_BOAT,          true    },
    { "npccorpse",      ITEM_CORPSE_NPC,    true    },
    { "pc_corpse",      ITEM_CORPSE_PC,     false   },
    { "fountain",       ITEM_FOUNTAIN,      true    },
    { "pill",           ITEM_PILL,          true    },
    { "protect",        ITEM_PROTECT,       true    },
    { "map",            ITEM_MAP,           true    },
    { "portal",         ITEM_PORTAL,        true    },
    { "warpstone",      ITEM_WARP_STONE,    true    },
    { "roomkey",        ITEM_ROOM_KEY,      true    },
    { "gem",            ITEM_GEM,           true    },
    { "jewelry",        ITEM_JEWELRY,       true    },
    { "jukebox",        ITEM_JUKEBOX,       true    },
    { NULL,             0,                  0       }
};

const struct flag_type extra_flag_table[] = {
    { "glow",           ITEM_GLOW,          true    },
    { "hum",            ITEM_HUM,           true    },
    { "dark",           ITEM_DARK,          true    },
    { "lock",           ITEM_LOCK,          true    },
    { "evil",           ITEM_EVIL,          true    },
    { "invis",          ITEM_INVIS,         true    },
    { "magic",          ITEM_MAGIC,         true    },
    { "nodrop",         ITEM_NODROP,        true    },
    { "bless",          ITEM_BLESS,         true    },
    { "antigood",       ITEM_ANTI_GOOD,     true    },
    { "antievil",       ITEM_ANTI_EVIL,     true    },
    { "antineutral",    ITEM_ANTI_NEUTRAL,  true    },
    { "noremove",       ITEM_NOREMOVE,      true    },
    { "inventory",      ITEM_INVENTORY,     true    },
    { "nopurge",        ITEM_NOPURGE,       true    },
    { "rotdeath",       ITEM_ROT_DEATH,     true    },
    { "visdeath",       ITEM_VIS_DEATH,     true    },
    { "nonmetal",       ITEM_NONMETAL,      true    },
    { "meltdrop",       ITEM_MELT_DROP,     true    },
    { "hadtimer",       ITEM_HAD_TIMER,     true    },
    { "sellextract",    ITEM_SELL_EXTRACT,  true    },
    { "burnproof",      ITEM_BURN_PROOF,    true    },
    { "nouncurse",      ITEM_NOUNCURSE,     true    },
    { NULL,             0,                  0       }
};

const struct flag_type wear_flag_table[] = {
    { "take",           ITEM_TAKE,          true    },
    { "finger",         ITEM_WEAR_FINGER,   true    },
    { "neck",           ITEM_WEAR_NECK,     true    },
    { "body",           ITEM_WEAR_BODY,     true    },
    { "head",           ITEM_WEAR_HEAD,     true    },
    { "legs",           ITEM_WEAR_LEGS,     true    },
    { "feet",           ITEM_WEAR_FEET,     true    },
    { "hands",          ITEM_WEAR_HANDS,    true    },
    { "arms",           ITEM_WEAR_ARMS,     true    },
    { "shield",         ITEM_WEAR_SHIELD,   true    },
    { "about",          ITEM_WEAR_ABOUT,    true    },
    { "waist",          ITEM_WEAR_WAIST,    true    },
    { "wrist",          ITEM_WEAR_WRIST,    true    },
    { "wield",          ITEM_WIELD,         true    },
    { "hold",           ITEM_HOLD,          true    },
    { "nosac",          ITEM_NO_SAC,        true    },
    { "wearfloat",      ITEM_WEAR_FLOAT,    true    },
//  { "twohands",       ITEM_TWO_HANDS,     true    },
    { NULL,             0,            0             }
};

/*
 * Used when adding an affect to tell where it goes.
 * See addaffect and delaffect in act_olc.c
 */
const struct flag_type apply_flag_table[] = {
    { "none",           APPLY_NONE,         true    },
    { "strength",       APPLY_STR,          true    },
    { "dexterity",      APPLY_DEX,          true    },
    { "intelligence",   APPLY_INT,          true    },
    { "wisdom",         APPLY_WIS,          true    },
    { "constitution",   APPLY_CON,          true    },
    { "sex",            APPLY_SEX,          true    },
    { "class",          APPLY_CLASS,        true    },
    { "level",          APPLY_LEVEL,        true    },
    { "age",            APPLY_AGE,          true    },
    { "height",         APPLY_HEIGHT,       true    },
    { "weight",         APPLY_WEIGHT,       true    },
    { "mana",           APPLY_MANA,         true    },
    { "hp",             APPLY_HIT,          true    },
    { "move",           APPLY_MOVE,         true    },
    { "gold",           APPLY_GOLD,         true    },
    { "experience",     APPLY_EXP,          true    },
    { "ac",             APPLY_AC,           true    },
    { "hitroll",        APPLY_HITROLL,      true    },
    { "damroll",        APPLY_DAMROLL,      true    },
    { "saves",          APPLY_SAVES,        true    },
    { "savingpara",     APPLY_SAVING_PARA,  true    },
    { "savingrod",      APPLY_SAVING_ROD,   true    },
    { "savingpetri",    APPLY_SAVING_PETRI, true    },
    { "savingbreath",   APPLY_SAVING_BREATH,true    },
    { "savingspell",    APPLY_SAVING_SPELL, true    },
    { "spellaffect",    APPLY_SPELL_AFFECT, false   },
    { NULL,             0,                  0       }
};

// What is seen.
const struct flag_type wear_loc_strings[] = {
    { "in the inventory",   WEAR_UNHELD,      true    },
    { "as a light",         WEAR_LIGHT,     true    },
    { "on the left finger", WEAR_FINGER_L,  true    },
    { "on the right finger",WEAR_FINGER_R,  true    },
    { "around the neck (1)",WEAR_NECK_1,    true    },
    { "around the neck (2)",WEAR_NECK_2,    true    },
    { "on the body",        WEAR_BODY,      true    },
    { "over the head",      WEAR_HEAD,      true    },
    { "on the legs",        WEAR_LEGS,      true    },
    { "on the feet",        WEAR_FEET,      true    },
    { "on the hands",       WEAR_HANDS,     true    },
    { "on the arms",        WEAR_ARMS,      true    },
    { "as a shield",        WEAR_SHIELD,    true    },
    { "about the shoulders",WEAR_ABOUT,     true    },
    { "around the waist",   WEAR_WAIST,     true    },
    { "on the left wrist",  WEAR_WRIST_L,   true    },
    { "on the right wrist", WEAR_WRIST_R,   true    },
    { "wielded",            WEAR_WIELD,     true    },
    { "held in the hands",  WEAR_HOLD,      true    },
    { "floating nearby",    WEAR_FLOAT,     true    },
    {NULL,                  0,              0       }
};

const struct flag_type wear_loc_flag_table[] = {
    { "none",           WEAR_UNHELD,          true    },
    { "light",          WEAR_LIGHT,         true    },
    { "lfinger",        WEAR_FINGER_L,      true    },
    { "rfinger",        WEAR_FINGER_R,      true    },
    { "neck1",          WEAR_NECK_1,        true    },
    { "neck2",          WEAR_NECK_2,        true    },
    { "body",           WEAR_BODY,          true    },
    { "head",           WEAR_HEAD,          true    },
    { "legs",           WEAR_LEGS,          true    },
    { "feet",           WEAR_FEET,          true    },
    { "hands",          WEAR_HANDS,         true    },
    { "arms",           WEAR_ARMS,          true    },
    { "shield",         WEAR_SHIELD,        true    },
    { "about",          WEAR_ABOUT,         true    },
    { "waist",          WEAR_WAIST,         true    },
    { "lwrist",         WEAR_WRIST_L,       true    },
    { "rwrist",         WEAR_WRIST_R,       true    },
    { "wielded",        WEAR_WIELD,         true    },
    { "hold",           WEAR_HOLD,          true    },
    { "floating",       WEAR_FLOAT,         true    },
    { NULL,             0,                  0       }
};

const struct flag_type container_flag_table[] = {
    { "closeable",      1,                  true    },
    { "pickproof",      2,                  true    },
    { "closed",         4,                  true    },
    { "locked",         8,                  true    },
    { "puton",          16,                 true    },
    { NULL,             0,                  0       }
};

/*****************************************************************************
                      ROM - specific tables:
 ****************************************************************************/

const struct flag_type ac_type[] = {
    { "pierce",         AC_PIERCE,          true    },
    { "bash",           AC_BASH,            true    },
    { "slash",          AC_SLASH,           true    },
    { "exotic",         AC_EXOTIC,          true    },
    { NULL,             0,                  0       }
};

const struct flag_type size_flag_table[MOB_SIZE_COUNT+1] = {
    { "tiny",           SIZE_TINY,          true    },
    { "small",          SIZE_SMALL,         true    },
    { "medium",         SIZE_MEDIUM,        true    },
    { "large",          SIZE_LARGE,         true    },
    { "huge",           SIZE_HUGE,          true    },
    { "giant",          SIZE_GIANT,         true    },
    { NULL,             0,                  0       }
};

const struct flag_type weapon_class[] = {
    { "exotic",         WEAPON_EXOTIC,      true    },
    { "sword",          WEAPON_SWORD,       true    },
    { "dagger",         WEAPON_DAGGER,      true    },
    { "spear",          WEAPON_SPEAR,       true    },
    { "mace",           WEAPON_MACE,        true    },
    { "axe",            WEAPON_AXE,         true    },
    { "flail",          WEAPON_FLAIL,       true    },
    { "whip",           WEAPON_WHIP,        true    },
    { "polearm",        WEAPON_POLEARM,     true    },
    { NULL,             0,                  0       }
};

const struct flag_type weapon_type2[] = {
    { "flaming",        WEAPON_FLAMING,     true    },
    { "frost",          WEAPON_FROST,       true    },
    { "vampiric",       WEAPON_VAMPIRIC,    true    },
    { "sharp",          WEAPON_SHARP,       true    },
    { "vorpal",         WEAPON_VORPAL,      true    },
    { "twohands",       WEAPON_TWO_HANDS,   true    },
    { "shocking",       WEAPON_SHOCKING,    true    },
    { "poison",         WEAPON_POISON,      true    },
    { NULL,             0,                  0       }
};

const struct flag_type res_flag_table[] = {
    { "summon",         RES_SUMMON,         true    },
    { "charm",          RES_CHARM,          true    },
    { "magic",          RES_MAGIC,          true    },
    { "weapon",         RES_WEAPON,         true    },
    { "bash",           RES_BASH,           true    },
    { "pierce",         RES_PIERCE,         true    },
    { "slash",          RES_SLASH,          true    },
    { "fire",           RES_FIRE,           true    },
    { "cold",           RES_COLD,           true    },
    { "lightning",      RES_LIGHTNING,      true    },
    { "acid",           RES_ACID,           true    },
    { "poison",         RES_POISON,         true    },
    { "negative",       RES_NEGATIVE,       true    },
    { "holy",           RES_HOLY,           true    },
    { "energy",         RES_ENERGY,         true    },
    { "mental",         RES_MENTAL,         true    },
    { "disease",        RES_DISEASE,        true    },
    { "drowning",       RES_DROWNING,       true    },
    { "light",          RES_LIGHT,          true    },
    { "sound",          RES_SOUND,          true    },
    { "wood",           RES_WOOD,           true    },
    { "silver",         RES_SILVER,         true    },
    { "iron",           RES_IRON,           true    },
    { NULL,             0,                  0       }
};

const struct flag_type vuln_flag_table[] = {
    { "none",           VULN_NONE,          true    },
    { "summon",         VULN_SUMMON,        true    },
    { "charm",          VULN_CHARM,         true    },
    { "magic",          VULN_MAGIC,         true    },
    { "weapon",         VULN_WEAPON,        true    },
    { "bash",           VULN_BASH,          true    },
    { "pierce",         VULN_PIERCE,        true    },
    { "slash",          VULN_SLASH,         true    },
    { "fire",           VULN_FIRE,          true    },
    { "cold",           VULN_COLD,          true    },
    { "lightning",      VULN_LIGHTNING,     true    },
    { "acid",           VULN_ACID,          true    },
    { "poison",         VULN_POISON,        true    },
    { "negative",       VULN_NEGATIVE,      true    },
    { "holy",           VULN_HOLY,          true    },
    { "energy",         VULN_ENERGY,        true    },
    { "mental",         VULN_MENTAL,        true    },
    { "disease",        VULN_DISEASE,       true    },
    { "drowning",       VULN_DROWNING,      true    },
    { "light",          VULN_LIGHT,         true    },
    { "sound",          VULN_SOUND,         true    },
    { "wood",           VULN_WOOD,          true    },
    { "silver",         VULN_SILVER,        true    },
    { "iron",           VULN_IRON,          true    },
    { NULL,             0,                  0       }
};

const struct flag_type position_flag_table[] = {
    { "dead",           POS_DEAD,           false   },
    { "mortal",         POS_MORTAL,         false   },
    { "incap",          POS_INCAP,          false   },
    { "stunned",        POS_STUNNED,        false   },
    { "sleeping",       POS_SLEEPING,       true    },
    { "resting",        POS_RESTING,        true    },
    { "sitting",        POS_SITTING,        true    },
    { "fighting",       POS_FIGHTING,       false   },
    { "standing",       POS_STANDING,       true    },
    { NULL,             0,                  0       }
};

const struct flag_type portal_flag_table [] = {
    { "normal_exit",    PORTAL_NORMAL_EXIT,   true    },
    { "no_curse",       PORTAL_NOCURSE,       true    },
    { "go_with",        PORTAL_GOWITH,        true    },
    { "buggy",          PORTAL_BUGGY,         true    },
    { "random",         PORTAL_RANDOM,        true    },
    { NULL,             0,                  0       }
};

const struct flag_type furniture_flag_table[] = {
    { "stand_at",       STAND_AT,           true    },
    { "stand_on",       STAND_ON,           true    },
    { "stand_in",       STAND_IN,           true    },
    { "sit_at",         SIT_AT,             true    },
    { "sit_on",         SIT_ON,             true    },
    { "sit_in",         SIT_IN,             true    },
    { "rest_at",        REST_AT,            true    },
    { "rest_on",        REST_ON,            true    },
    { "rest_in",        REST_IN,            true    },
    { "sleep_at",       SLEEP_AT,           true    },
    { "sleep_on",       SLEEP_ON,           true    },
    { "sleep_in",       SLEEP_IN,           true    },
    { "put_at",         PUT_AT,             true    },
    { "put_on",         PUT_ON,             true    },
    { "put_in",         PUT_IN,             true    },
    { "put_inside",     PUT_INSIDE,         true    },
    { NULL,             0,                  0       }
};

const struct flag_type apply_types[] = {
    { "affects",        TO_AFFECTS,         true    },
    { "object",         TO_OBJECT,          true    },
    { "immune",         TO_IMMUNE,          true    },
    { "resist",         TO_RESIST,          true    },
    { "vuln",           TO_VULN,            true    },
    { "weapon",         TO_WEAPON,          true    },
    { NULL,             0,                  true    }
};

const struct bit_type bitvector_type[] = {
    { affect_flag_table,    "affect"    },
    { apply_flag_table,     "apply"     },
    { imm_flag_table,       "imm"       },
    { res_flag_table,       "res"       },
    { vuln_flag_table,      "vuln"      },
    { weapon_type2,         "weapon"    }
};

const struct flag_type target_table[] = {
    { "tar_ignore",         SKILL_TARGET_IGNORE,            true},
    { "tar_char_offensive", SKILL_TARGET_CHAR_OFFENSIVE,    true},
    { "tar_char_defensive", SKILL_TARGET_CHAR_DEFENSIVE,    true},
    { "tar_char_self",      SKILL_TARGET_CHAR_SELF,         true},
    { "tar_obj_inv",        SKILL_TARGET_OBJ_INV,           true},
    { "tar_obj_char_def",   SKILL_TARGET_OBJ_CHAR_DEF,      true},
    { "tar_obj_char_off",   SKILL_TARGET_OBJ_CHAR_OFF,      true},
    { NULL,                 0,                              true}
};

const struct recval_type recval_table[] = {
//    hit,                       AC,   dam   type  bonus
    { 2, /*d*/ 6, /*+*/ 10,      9,    1,    4,    0    },
    { 2, /*d*/ 7, /*+*/ 21,      8,    1,    5,    0    },
    { 2, /*d*/ 6, /*+*/ 35,      7,    1,    6,    0    },
    { 2, /*d*/ 7, /*+*/ 46,      6,    1,    5,    1    },
    { 2, /*d*/ 6, /*+*/ 60,      5,    1,    6,    1    },
    { 2, /*d*/ 7, /*+*/ 71,      4,    1,    7,    1    },
    { 2, /*d*/ 6, /*+*/ 85,      4,    1,    8,    1    },
    { 2, /*d*/ 7, /*+*/ 96,      3,    1,    7,    2    },
    { 2, /*d*/ 6, /*+*/ 110,     2,    1,    8,    2    },
    { 2, /*d*/ 7, /*+*/ 121,     1,    2,    4,    2    }, /* 10 */
    { 2, /*d*/ 8, /*+*/ 134,     1,    1,    10,   2    },
    { 2, /*d*/ 10,/*+*/ 150,     0,    1,    10,   3    },
    { 2, /*d*/ 10,/*+*/ 170,     -1,   2,    5,    3    },
    { 2, /*d*/ 10,/*+*/ 190,     -1,   1,    12,   3    },
    { 3, /*d*/ 9, /*+*/ 208,     -2,   2,    6,    3    },
    { 3, /*d*/ 9, /*+*/ 233,     -2,   2,    6,    4    },
    { 3, /*d*/ 9, /*+*/ 258,     -3,   3,    4,    4    },
    { 3, /*d*/ 9, /*+*/ 283,     -3,   2,    7,    4    },
    { 3, /*d*/ 9, /*+*/ 308,     -4,   2,    7,    5    },
    { 3, /*d*/ 9, /*+*/ 333,     -4,   2,    8,    5    }, /* 20 */
    { 4, /*d*/ 10,/*+*/ 360,     -5,   4,    4,    5    },
    { 5, /*d*/ 10,/*+*/ 400,     -5,   4,    4,    6    },
    { 5, /*d*/ 10,/*+*/ 450,     -6,   3,    6,    6    },
    { 5, /*d*/ 10,/*+*/ 500,     -6,   2,    10,   6    },
    { 5, /*d*/ 10,/*+*/ 550,     -7,   2,    10,   7    },
    { 5, /*d*/ 10,/*+*/ 600,     -7,   3,    7,    7    },
    { 5, /*d*/ 10,/*+*/ 650,     -8,   5,    4,    7    },
    { 6, /*d*/ 12,/*+*/ 703,     -8,   2,    12,   8    },
    { 6, /*d*/ 12,/*+*/ 778,     -9,   2,    12,   8    },
    { 6, /*d*/ 12,/*+*/ 853,     -9,   4,    6,    8    }, /* 30 */
    { 6, /*d*/ 12,/*+*/ 928,     -10,  6,    4,    9    },
    { 10,/*d*/ 10,/*+*/ 1000,    -10,  4,    7,    9    },
    { 10,/*d*/ 10,/*+*/ 1100,    -11,  7,    4,    10   },
    { 10,/*d*/ 10,/*+*/ 1200,    -11,  5,    6,    10   },
    { 10,/*d*/ 10,/*+*/ 1300,    -11,  6,    5,    11   },
    { 10,/*d*/ 10,/*+*/ 1400,    -12,  4,    8,    11   },
    { 10,/*d*/ 10,/*+*/ 1500,    -12,  8,    4,    12   },
    { 10,/*d*/ 10,/*+*/ 1600,    -13,  16,   2,    12   },
    { 15,/*d*/ 10,/*+*/ 1700,    -13,  17,   2,    13   },
    { 15,/*d*/ 10,/*+*/ 1850,    -13,  6,    6,    13   }, /* 40 */
    { 25,/*d*/ 10,/*+*/ 2000,    -14,  12,   3,    14   },
    { 25,/*d*/ 10,/*+*/ 2250,    -14,  5,    8,    14   },
    { 25,/*d*/ 10,/*+*/ 2500,    -15,  10,   4,    15   },
    { 25,/*d*/ 10,/*+*/ 2750,    -15,  5,    9,    15   },
    { 25,/*d*/ 10,/*+*/ 3000,    -15,  9,    5,    16   },
    { 25,/*d*/ 10,/*+*/ 3250,    -16,  7,    7,    16   },
    { 25,/*d*/ 10,/*+*/ 3500,    -17,  13,   4,    17   },
    { 25,/*d*/ 10,/*+*/ 3750,    -18,  5,    11,   18   },
    { 50,/*d*/ 10,/*+*/ 4000,    -19,  11,   5,    18   },
    { 50,/*d*/ 10,/*+*/ 4500,    -20,  10,   6,    19   }, /* 50 */
    { 50,/*d*/ 10,/*+*/ 5000,    -21,  5,    13,   20   },
    { 50,/*d*/ 10,/*+*/ 5500,    -22,  15,   5,    20   },
    { 50,/*d*/ 10,/*+*/ 6000,    -23,  15,   6,    21   },
    { 50,/*d*/ 10,/*+*/ 6500,    -24,  15,   7,    22   },
    { 50,/*d*/ 10,/*+*/ 7000,    -25,  15,   8,    23   },
    { 50,/*d*/ 10,/*+*/ 7500,    -26,  15,   9,    24   },
    { 50,/*d*/ 10,/*+*/ 8000,    -27,  15,   10,   24   },
    { 50,/*d*/ 10,/*+*/ 8500,    -28,  20,   10,   25   },
    { 50,/*d*/ 10,/*+*/ 9000,    -29,  30,   10,   26   },
    { 50,/*d*/ 10,/*+*/ 9500,    -30,  50,   10,   28   }  /* 60 */
};

// Ignores DAM_NONE, has null record at end.
// Added DAM_TYPE_COUNT so it fails to compile if you add a new damage type and don't
// add it here.
const struct flag_type dam_classes[DAM_TYPE_COUNT] = {
    { "dam_bash",       DAM_BASH,           true    },
    { "dam_pierce",     DAM_PIERCE,         true    },
    { "dam_slash",      DAM_SLASH,          true    },
    { "dam_fire",       DAM_FIRE,           true    },
    { "dam_cold",       DAM_COLD,           true    },
    { "dam_lightning",  DAM_LIGHTNING,      true    },
    { "dam_acid",       DAM_ACID,           true    },
    { "dam_poison",     DAM_POISON,         true    },
    { "dam_negative",   DAM_NEGATIVE,       true    },
    { "dam_holy",       DAM_HOLY,           true    },
    { "dam_energy",     DAM_ENERGY,         true    },
    { "dam_mental",     DAM_MENTAL,         true    },
    { "dam_disease",    DAM_DISEASE,        true    },
    { "dam_drowning",   DAM_DROWNING,       true    },
    { "dam_light",      DAM_LIGHT,          true    },
    { "dam_other",      DAM_OTHER,          true    },
    { "dam_harm",       DAM_HARM,           true    },
    { "dam_charm",      DAM_CHARM,          true    },
    { "dam_sound",      DAM_SOUND,          true    },
    { NULL,             0,                  true    }
};

const struct flag_type log_flag_table[] = {
    { "log_normal",     LOG_NORMAL,         true    },
    { "log_always",     LOG_ALWAYS,         true    },
    { "log_never",      LOG_NEVER,          true    },
    { NULL,             0,                  true    }
};

const struct flag_type show_flag_table[] = {
    { "undef",          TYP_UNDEF,          true    },
    { "communication",  TYP_CMM,            true    },
    { "combat",         TYP_CBT,            true    },
    { "specials",       TYP_SPC,            true    },
    { "group",          TYP_GRP,            true    },
    { "objects",        TYP_OBJ,            true    },
    { "information",    TYP_INF,            true    },
    { "otros",          TYP_OTH,            true    },
    { "movimiento",     TYP_MVT,            true    },
    { "configuration",  TYP_CNF,            true    },
    { "languages",      TYP_LNG,            true    },
    { "player",         TYP_PLR,            true    },
    { "olc",            TYP_OLC,            true    },
    { NULL,             0,                  true    }
};

const struct flag_type stat_table[STAT_COUNT+1] = {
    { "str",            STAT_STR,           true    },
    { "int",            STAT_INT,           true    },
    { "wis",            STAT_WIS,           true    },
    { "dex",            STAT_DEX,           true    },
    { "con",            STAT_CON,           true    },
    { NULL,             0,                  true    }
};

const struct flag_type inst_type_table[] = {
    { "single",         AREA_INST_SINGLE,   true    },
    { "multi",          AREA_INST_MULTI,    true    },
    { NULL,             0,                  0       }
};

void show_flags_to_char(Mobile* ch, const struct flag_type* flags)
{
    char line[25];
    int col = 0;
    char buf[MAX_STRING_LENGTH] = { 0 };
    for (int i = 0; flags[i].name != NULL; ++i) {
        if (!flags[i].settable)
            continue;
        sprintf(line, COLOR_ALT_TEXT_1 "%15s" COLOR_CLEAR , flags[i].name);
        strcat(buf, line);
        if (++col == 4) {
            col = 0;
            strcat(buf, "\r\n");
        }
    }
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
}
