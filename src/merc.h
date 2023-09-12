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

#pragma once
#ifndef MUD98__MERC_H
#define MUD98__MERC_H

#define MUD_NAME "Mud98"
#define MUD_VER "0.90"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef _MSC_VER
#include <stddef.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// Data files used by the server.
// 
// AREA_LIST contains a list of areas to boot.
// All files are read in completely at bootup.
// Most output files (bug, idea, typo, shutdown) are append-only.
// 
// The NULL_FILE is held open so that we have a stream handle in reserve, so
// players can go ahead and telnet to all the other descriptors. Then we close
// it whenever we need to open a file (e.g. a save file).
////////////////////////////////////////////////////////////////////////////////

extern char area_dir[];

#define DEFAULT_AREA_DIR "./"           // Default is legacy usage
#define PLAYER_DIR      "../player/"    // Player files
#define GOD_DIR         "../gods/"      // list of gods
#define TEMP_DIR        "../temp/"
#define DATA_DIR        "../data/"
#define PROG_DIR        DATA_DIR "progs/"
#define SOCIAL_FILE	    DATA_DIR "socials"
#define SKILL_FILE      DATA_DIR "skills"
#define COMMAND_FILE    DATA_DIR "commands"
#define RACES_FILE      DATA_DIR "races"
#define AREA_LIST       "area.lst"      // List of areas
#define BUG_FILE        "bugs.txt"      // For 'bug' and bug()
#define TYPO_FILE       "typos.txt"     // For 'typo'
#define NOTE_FILE       "notes.not"     // For 'notes'
#define IDEA_FILE       "ideas.not"
#define PENALTY_FILE    "penal.not"
#define NEWS_FILE       "news.not"
#define CHANGES_FILE    "chang.not"
#define SHUTDOWN_FILE   "shutdown.txt"  // For 'shutdown'
#define BAN_FILE        "ban.txt"
#define MUSIC_FILE      "music.txt"
#ifndef _MSC_VER
#define NULL_FILE       "/dev/null"     // To reserve one stream
#else
#define NULL_FILE       "nul"
#endif

#ifndef USE_RAW_SOCKETS
#define CERT_FILE       "../keys/rom-mud.pem"
#define PKEY_FILE       "../keys/rom-mud.key"
#endif

////////////////////////////////////////////////////////////////////////////////

#define args( list )                list

////////////////////////////////////////////////////////////////////////////////
// Custom Types
////////////////////////////////////////////////////////////////////////////////

// Used for flags and other bit fields
#define BIT(x) (1 << x)

// Expand when you run out of bits
#define FLAGS               uint32_t
#define FLAGS_MAX           BIT(31)

#define SHORT_FLAGS         uint16_t
#define SHORT_FLAGS_MAX     BIT(

#define LEVEL               int16_t

#define SKNUM               int16_t

// If you want to change the type for VNUM's, this is where you do it.
#define MAX_VNUM            INT32_MAX
#define VNUM                int32_t
#define PRVNUM              PRId32
#define STRTOVNUM(s)        (VNUM)strtol(s, NULL, 0)
#define VNUM_NONE           -1

////////////////////////////////////////////////////////////////////////////////

typedef struct char_data_t CharData;
typedef struct area_data_t AreaData;

typedef void DoFunc(CharData* ch, char* argument);
typedef bool SpecFunc(CharData* ch);
typedef void SpellFunc(SKNUM sn, LEVEL level, CharData* ch, void* vo, int target);
typedef int	LookupFunc(const char*);

#define DECLARE_DO_FUN( fun )       DoFunc fun
#define DECLARE_SPEC_FUN( fun )     SpecFunc fun
#define DECLARE_SPELL_FUN( fun )    SpellFunc fun
#define DECLARE_LOOKUP_FUN( fun )   LookupFunc fun

// OLC2
#define SPELL(spell)		DECLARE_SPELL_FUN(spell);
#define SPELL_FUN_DEC(spell)	FRetVal spell(SKNUM sn, LEVEL level, Entity* caster, Entity* ent, int target)
#define COMMAND(cmd)		DECLARE_DO_FUN(cmd);
#define DO_FUN_DEC(x)		void x(CharData* ch, char* argument)
#define NEW_DO_FUN_DEC(x)	FRetVal x(Entity* ent, char* argument)
#define DECLARE_SPELL_CB(x)	FRetVal x(Entity* ent)

/* ea */
#define MSL MAX_STRING_LENGTH
#define MIL MAX_INPUT_LENGTH

/*
 * Structure types.
 */
typedef struct buf_type BUFFER;
typedef struct descriptor_data DESCRIPTOR_DATA;
typedef struct kill_data KILL_DATA;
typedef struct mprog_list MPROG_LIST;
typedef struct mprog_code MPROG_CODE;
typedef struct gen_data GEN_DATA;
typedef struct shop_data SHOP_DATA;
typedef struct time_info_data TIME_INFO_DATA;
typedef struct weather_data WEATHER_DATA;

/*
 * String and memory management parameters.
 */
#define MAX_KEY_HASH        1024
#define MAX_STRING_LENGTH   4608
#define MAX_INPUT_LENGTH    256
#define PAGELEN             22

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_SOCIALS         256

#define MAX_IN_GROUP        15
#define MAX_ALIAS           5
#define MAX_THEMES          5
#define MAX_CLASS           4
#define MAX_PC_RACE         5
#define MAX_CLAN            3
#define MAX_DAMAGE_MESSAGE  41
#define MAX_LEVEL           (LEVEL)60
#define LEVEL_HERO          (MAX_LEVEL - 9)
#define LEVEL_IMMORTAL      (MAX_LEVEL - 8)

#define PULSE_PER_SECOND    4
#define PULSE_VIOLENCE      (3 * PULSE_PER_SECOND)
#define PULSE_MOBILE        (4 * PULSE_PER_SECOND)
#define PULSE_MUSIC         (6 * PULSE_PER_SECOND)
#define PULSE_TICK          (60 * PULSE_PER_SECOND)
#define PULSE_AREA          (120 * PULSE_PER_SECOND)

#define IMPLEMENTOR         MAX_LEVEL
#define CREATOR             (MAX_LEVEL - 1)
#define SUPREME             (MAX_LEVEL - 2)
#define DEITY               (MAX_LEVEL - 3)
#define GOD                 (MAX_LEVEL - 4)
#define IMMORTAL            (MAX_LEVEL - 5)
#define DEMI                (MAX_LEVEL - 6)
#define ANGEL               (MAX_LEVEL - 7)
#define AVATAR              (MAX_LEVEL - 8)
#define HERO                LEVEL_HERO

struct buf_type {
    BUFFER* next;
    char* string; /* buffer's string */
    size_t size;  /* size in k */
    int state; /* error state of the buffer */
    bool valid;
};

struct skhash {
    struct skhash* next;
    SKNUM sn;
};

/*
 * Time and weather stuff.
 */
#define SUN_DARK      0
#define SUN_RISE      1
#define SUN_LIGHT     2
#define SUN_SET       3

#define SKY_CLOUDLESS 0
#define SKY_CLOUDY    1
#define SKY_RAINING   2
#define SKY_LIGHTNING 3

struct time_info_data {
    int hour;
    int day;
    int month;
    int year;
};

struct weather_data {
    int mmhg;
    int change;
    int sky;
    int sunlight;
};

/*
 * Connected state for a channel.
 */
#define CON_PLAYING              0
#define CON_GET_NAME             1
#define CON_GET_OLD_PASSWORD     2
#define CON_CONFIRM_NEW_NAME     3
#define CON_GET_NEW_PASSWORD     4
#define CON_CONFIRM_NEW_PASSWORD 5
#define CON_GET_NEW_RACE         6
#define CON_GET_NEW_SEX          7
#define CON_GET_NEW_CLASS        8
#define CON_GET_ALIGNMENT        9
#define CON_DEFAULT_CHOICE       10
#define CON_GEN_GROUPS           11
#define CON_PICK_WEAPON          12
#define CON_READ_IMOTD           13
#define CON_READ_MOTD            14
#define CON_BREAK_CONNECT        15

/* damage classes */
typedef enum {
    DAM_NONE        = 0,
    DAM_BASH        = 1,
    DAM_PIERCE      = 2,
    DAM_SLASH       = 3,
    DAM_FIRE        = 4,
    DAM_COLD        = 5,
    DAM_LIGHTNING   = 6,
    DAM_ACID        = 7,
    DAM_POISON      = 8,
    DAM_NEGATIVE    = 9,
    DAM_HOLY        = 10,
    DAM_ENERGY      = 11,
    DAM_MENTAL      = 12,
    DAM_DISEASE     = 13,
    DAM_DROWNING    = 14,
    DAM_LIGHT       = 15,
    DAM_OTHER       = 16,
    DAM_HARM        = 17,
    DAM_CHARM       = 18,
    DAM_SOUND       = 19,
    
    DAM_NOT_FOUND   = -1,
} DamageType;

/*
 * Attribute bonus structures.
 */
struct str_app_type {
    int16_t tohit;
    int16_t todam;
    int16_t carry;
    int16_t wield;
};

struct int_app_type {
    int16_t learn;
};

struct wis_app_type {
    int16_t practice;
};

struct dex_app_type {
    int16_t defensive;
};

struct con_app_type {
    int16_t hitp;
    int16_t shock;
};

/*
 * TO types for act.
 */
#define TO_ROOM    0
#define TO_NOTVICT 1
#define TO_VICT    2
#define TO_CHAR    3
#define TO_ALL     4

/*
 * Shop types.
 */
#define MAX_TRADE 5

struct shop_data {
    SHOP_DATA* next; /* Next shop in list		*/
    VNUM keeper; /* Vnum of shop keeper mob	*/
    int16_t buy_type[MAX_TRADE]; /* Item types shop will buy	*/
    int16_t profit_buy; /* Cost multiplier for buying	*/
    int16_t profit_sell; /* Cost multiplier for selling	*/
    int16_t open_hour; /* First opening hour		*/
    int16_t close_hour; /* First closing hour		*/
};

/*
 * Per-class stuff.
 */
#define MAX_GUILD 2
#define MAX_STATS 5
#define STAT_STR  0
#define STAT_INT  1
#define STAT_WIS  2
#define STAT_DEX  3
#define STAT_CON  4

struct class_type {
    char* name; /* the full name of the class */
    char who_name[4]; /* Three-letter name for 'who'	*/
    int16_t attr_prime; /* Prime attribute		*/
    int16_t weapon; /* First weapon			*/
    int16_t guild[MAX_GUILD]; /* Vnum of guild rooms		*/
    int16_t skill_adept; /* Maximum skill level		*/
    int16_t thac0_00; /* Thac0 for level  0		*/
    int16_t thac0_32; /* Thac0 for level 32		*/
    int16_t hp_min; /* Min hp gained on leveling	*/
    int16_t hp_max; /* Max hp gained on leveling	*/
    bool fMana; /* Class gains mana on level	*/
    char* base_group; /* base skills gained		*/
    char* default_group; /* default skills gained	*/
};

struct item_type {
    int type;
    char* name;
};

struct weapon_type {
    char* name;
    SKNUM* gsn;
    VNUM vnum;
    int type;
};

struct wiznet_type {
    char* name;
    long flag;
    LEVEL level;
};

struct attack_type {
    char* name; /* name */
    char* noun; /* message */
    DamageType damage; /* damage class */
};

struct race_type {
    char* name; /* call name of the race */
    char* who_name;
    char* skills[5];		/* bonus skills for the race */
    bool pc_race; /* can be chosen by pcs */
    FLAGS act; /* act bits for the race */
    FLAGS aff; /* aff bits for the race */
    FLAGS off; /* off bits for the race */
    FLAGS imm; /* imm bits for the race */
    FLAGS res; /* res bits for the race */
    FLAGS vuln; /* vuln bits for the race */
    FLAGS form; /* default form flag for the race */
    FLAGS parts; /* default parts for the race */
    int16_t race_id;
    int16_t points;			/* cost in points of the race */
    int16_t class_mult[MAX_CLASS];	/* exp multiplier for class, * 100 */
    int16_t stats[MAX_STATS];	/* starting stats */
    int16_t max_stats[MAX_STATS];	/* maximum stats */
    int16_t size;			/* aff bits for the race */
};

/*
 * A kill structure (indexed by level).
 */
struct kill_data {
    int16_t number;
    int16_t killed;
};

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
#define ACT_IS_NPC              BIT(0) /* Auto set for mobs	*/
#define ACT_SENTINEL            BIT(1) /* Stays in one room	*/
#define ACT_SCAVENGER           BIT(2) /* Picks up objects	*/
#define ACT_AGGRESSIVE          BIT(5) /* Attacks PC's		*/
#define ACT_STAY_AREA           BIT(6) /* Won't leave area	*/
#define ACT_WIMPY               BIT(7)
#define ACT_PET                 BIT(8) /* Auto set for pets	*/
#define ACT_TRAIN               BIT(9) /* Can train PC's	*/
#define ACT_PRACTICE            BIT(10) /* Can practice PC's	*/
#define ACT_UNDEAD              BIT(14)
#define ACT_CLERIC              BIT(16)
#define ACT_MAGE                BIT(17)
#define ACT_THIEF               BIT(18)
#define ACT_WARRIOR             BIT(19)
#define ACT_NOALIGN             BIT(20)
#define ACT_NOPURGE             BIT(21)
#define ACT_OUTDOORS            BIT(22)
#define ACT_INDOORS             BIT(24)
#define ACT_IS_HEALER           BIT(26)
#define ACT_GAIN                BIT(27)
#define ACT_UPDATE_ALWAYS       BIT(28)
#define ACT_IS_CHANGER          BIT(29)

/* OFF bits for mobiles */
#define OFF_AREA_ATTACK         BIT(0)
#define OFF_BACKSTAB            BIT(1)
#define OFF_BASH                BIT(2)
#define OFF_BERSERK             BIT(3)
#define OFF_DISARM              BIT(4)
#define OFF_DODGE               BIT(5)
#define OFF_FADE                BIT(6)
#define OFF_FAST                BIT(7)
#define OFF_KICK                BIT(8)
#define OFF_KICK_DIRT           BIT(9)
#define OFF_PARRY               BIT(10)
#define OFF_RESCUE              BIT(11)
#define OFF_TAIL                BIT(12)
#define OFF_TRIP                BIT(13)
#define OFF_CRUSH               BIT(14)
#define ASSIST_ALL              BIT(15)
#define ASSIST_ALIGN            BIT(16)
#define ASSIST_RACE             BIT(17)
#define ASSIST_PLAYERS          BIT(18)
#define ASSIST_GUARD            BIT(19)
#define ASSIST_VNUM             BIT(20)

/* return values for check_imm */
#define IS_NORMAL               0
#define IS_IMMUNE               1
#define IS_RESISTANT            2
#define IS_VULNERABLE           3

/* IMM bits for mobs */
#define IMM_SUMMON              BIT(0)
#define IMM_CHARM               BIT(1)
#define IMM_MAGIC               BIT(2)
#define IMM_WEAPON              BIT(3)
#define IMM_BASH                BIT(4)
#define IMM_PIERCE              BIT(5)
#define IMM_SLASH               BIT(6)
#define IMM_FIRE                BIT(7)
#define IMM_COLD                BIT(8)
#define IMM_LIGHTNING           BIT(9)
#define IMM_ACID                BIT(10)
#define IMM_POISON              BIT(11)
#define IMM_NEGATIVE            BIT(12)
#define IMM_HOLY                BIT(13)
#define IMM_ENERGY              BIT(14)
#define IMM_MENTAL              BIT(15)
#define IMM_DISEASE             BIT(16)
#define IMM_DROWNING            BIT(17)
#define IMM_LIGHT               BIT(18)
#define IMM_SOUND               BIT(19)
#define IMM_WOOD                BIT(23)
#define IMM_SILVER              BIT(24)
#define IMM_IRON                BIT(25)

/* RES bits for mobs */
#define RES_SUMMON              BIT(0)
#define RES_CHARM               BIT(1)
#define RES_MAGIC               BIT(2)
#define RES_WEAPON              BIT(3)
#define RES_BASH                BIT(4)
#define RES_PIERCE              BIT(5)
#define RES_SLASH               BIT(6)
#define RES_FIRE                BIT(7)
#define RES_COLD                BIT(8)
#define RES_LIGHTNING           BIT(9)
#define RES_ACID                BIT(10)
#define RES_POISON              BIT(11)
#define RES_NEGATIVE            BIT(12)
#define RES_HOLY                BIT(13)
#define RES_ENERGY              BIT(14)
#define RES_MENTAL              BIT(15)
#define RES_DISEASE             BIT(16)
#define RES_DROWNING            BIT(17)
#define RES_LIGHT               BIT(18)
#define RES_SOUND               BIT(19)
#define RES_WOOD                BIT(23)
#define RES_SILVER              BIT(24)
#define RES_IRON                BIT(25)

/* VULN bits for mobs */
#define VULN_SUMMON             BIT(0)
#define VULN_CHARM              BIT(1)
#define VULN_MAGIC              BIT(2)
#define VULN_WEAPON             BIT(3)
#define VULN_BASH               BIT(4)
#define VULN_PIERCE             BIT(5)
#define VULN_SLASH              BIT(6)
#define VULN_FIRE               BIT(7)
#define VULN_COLD               BIT(8)
#define VULN_LIGHTNING          BIT(9)
#define VULN_ACID               BIT(10)
#define VULN_POISON             BIT(11)
#define VULN_NEGATIVE           BIT(12)
#define VULN_HOLY               BIT(13)
#define VULN_ENERGY             BIT(14)
#define VULN_MENTAL             BIT(15)
#define VULN_DISEASE            BIT(16)
#define VULN_DROWNING           BIT(17)
#define VULN_LIGHT              BIT(18)
#define VULN_SOUND              BIT(19)
#define VULN_WOOD               BIT(23)
#define VULN_SILVER             BIT(24)
#define VULN_IRON               BIT(25)

/* body form */
#define FORM_EDIBLE             BIT(0)
#define FORM_POISON             BIT(1)
#define FORM_MAGICAL            BIT(2)
#define FORM_INSTANT_DECAY      BIT(3)
#define FORM_OTHER              BIT(4) /* defined by material bit */

/* actual form */
#define FORM_ANIMAL             BIT(6)
#define FORM_SENTIENT           BIT(7)
#define FORM_UNDEAD             BIT(8)
#define FORM_CONSTRUCT          BIT(9)
#define FORM_MIST               BIT(10)
#define FORM_INTANGIBLE         BIT(11)

#define FORM_BIPED              BIT(12)
#define FORM_CENTAUR            BIT(13)
#define FORM_INSECT             BIT(14)
#define FORM_SPIDER             BIT(15)
#define FORM_CRUSTACEAN         BIT(16)
#define FORM_WORM               BIT(17)
#define FORM_BLOB               BIT(18)

#define FORM_MAMMAL             BIT(21)
#define FORM_BIRD               BIT(22)
#define FORM_REPTILE            BIT(23)
#define FORM_SNAKE              BIT(24)
#define FORM_DRAGON             BIT(25)
#define FORM_AMPHIBIAN          BIT(26)
#define FORM_FISH               BIT(27)
#define FORM_COLD_BLOOD         BIT(28)

/* body parts */
#define PART_HEAD               BIT(0)
#define PART_ARMS               BIT(1)
#define PART_LEGS               BIT(2)
#define PART_HEART              BIT(3)
#define PART_BRAINS             BIT(4)
#define PART_GUTS               BIT(5)
#define PART_HANDS              BIT(6)
#define PART_FEET               BIT(7)
#define PART_FINGERS            BIT(8)
#define PART_EAR                BIT(9)
#define PART_EYE                BIT(10)
#define PART_LONG_TONGUE        BIT(11)
#define PART_EYESTALKS          BIT(12)
#define PART_TENTACLES          BIT(13)
#define PART_FINS               BIT(14)
#define PART_WINGS              BIT(15)
#define PART_TAIL               BIT(16)
/* for combat */
#define PART_CLAWS              BIT(20)
#define PART_FANGS              BIT(21)
#define PART_HORNS              BIT(22)
#define PART_SCALES             BIT(23)
#define PART_TUSKS              BIT(24)

/*
 * Sex.
 * Used in #MOBILES.
 */
#define SEX_NEUTRAL             0
#define SEX_MALE                1
#define SEX_FEMALE              2

/* AC types */
#define AC_PIERCE               0
#define AC_BASH                 1
#define AC_SLASH                2
#define AC_EXOTIC               3

/* dice */
#define DICE_NUMBER             0
#define DICE_TYPE               1
#define DICE_BONUS              2

/* size */
#define SIZE_TINY               0
#define SIZE_SMALL              1
#define SIZE_MEDIUM             2
#define SIZE_LARGE              3
#define SIZE_HUGE               4
#define SIZE_GIANT              5

/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GLOW               BIT(0)
#define ITEM_HUM                BIT(1)
#define ITEM_DARK               BIT(2)
#define ITEM_LOCK               BIT(3)
#define ITEM_EVIL               BIT(4)
#define ITEM_INVIS              BIT(5)
#define ITEM_MAGIC              BIT(6)
#define ITEM_NODROP             BIT(7)
#define ITEM_BLESS              BIT(8)
#define ITEM_ANTI_GOOD          BIT(9)
#define ITEM_ANTI_EVIL          BIT(10)
#define ITEM_ANTI_NEUTRAL       BIT(11)
#define ITEM_NOREMOVE           BIT(12)
#define ITEM_INVENTORY          BIT(13)
#define ITEM_NOPURGE            BIT(14)
#define ITEM_ROT_DEATH          BIT(15)
#define ITEM_VIS_DEATH          BIT(16)
#define ITEM_NONMETAL           BIT(18)
#define ITEM_NOLOCATE           BIT(19)
#define ITEM_MELT_DROP          BIT(20)
#define ITEM_HAD_TIMER          BIT(21)
#define ITEM_SELL_EXTRACT       BIT(22)
#define ITEM_BURN_PROOF         BIT(24)
#define ITEM_NOUNCURSE          BIT(25)

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE               BIT(0)
#define ITEM_WEAR_FINGER        BIT(1)
#define ITEM_WEAR_NECK          BIT(2)
#define ITEM_WEAR_BODY          BIT(3)
#define ITEM_WEAR_HEAD          BIT(4)
#define ITEM_WEAR_LEGS          BIT(5)
#define ITEM_WEAR_FEET          BIT(6)
#define ITEM_WEAR_HANDS         BIT(7)
#define ITEM_WEAR_ARMS          BIT(8)
#define ITEM_WEAR_SHIELD        BIT(9)
#define ITEM_WEAR_ABOUT         BIT(10)
#define ITEM_WEAR_WAIST         BIT(11)
#define ITEM_WEAR_WRIST         BIT(12)
#define ITEM_WIELD              BIT(13)
#define ITEM_HOLD               BIT(14)
#define ITEM_NO_SAC             BIT(15)
#define ITEM_WEAR_FLOAT         BIT(16)

/* weapon class */
#define WEAPON_EXOTIC           0
#define WEAPON_SWORD            1
#define WEAPON_DAGGER           2
#define WEAPON_SPEAR            3
#define WEAPON_MACE             4
#define WEAPON_AXE              5
#define WEAPON_FLAIL            6
#define WEAPON_WHIP             7
#define WEAPON_POLEARM          8

/* weapon types */
#define WEAPON_FLAMING          BIT(0)
#define WEAPON_FROST            BIT(1)
#define WEAPON_VAMPIRIC         BIT(2)
#define WEAPON_SHARP            BIT(3)
#define WEAPON_VORPAL           BIT(4)
#define WEAPON_TWO_HANDS        BIT(5)
#define WEAPON_SHOCKING         BIT(6)
#define WEAPON_POISON           BIT(7)

/* gate flags */
#define GATE_NORMAL_EXIT        BIT(0)
#define GATE_NOCURSE            BIT(1)
#define GATE_GOWITH             BIT(2)
#define GATE_BUGGY              BIT(3)
#define GATE_RANDOM             BIT(4)

/* furniture flags */
#define STAND_AT                BIT(0)
#define STAND_ON                BIT(1)
#define STAND_IN                BIT(2)
#define SIT_AT                  BIT(3)
#define SIT_ON                  BIT(4)
#define SIT_IN                  BIT(5)
#define REST_AT                 BIT(6)
#define REST_ON                 BIT(7)
#define REST_IN                 BIT(8)
#define SLEEP_AT                BIT(9)
#define SLEEP_ON                BIT(10)
#define SLEEP_IN                BIT(11)
#define PUT_AT                  BIT(12)
#define PUT_ON                  BIT(13)
#define PUT_IN                  BIT(14)
#define PUT_INSIDE              BIT(15)

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE          1
#define CONT_PICKPROOF          2
#define CONT_CLOSED             4
#define CONT_LOCKED             8
#define CONT_PUT_ON             16

/*
 * Command logging types.
 */
#define LOG_NORMAL	0
#define LOG_ALWAYS	1
#define LOG_NEVER	2

/*
 * Command types.
 */
#define TYP_NUL 0
#define TYP_UNDEF 1
#define TYP_CMM	10
#define TYP_CBT	2
#define TYP_SPC 3
#define TYP_GRP 4
#define TYP_OBJ 5
#define TYP_INF 6
#define TYP_OTH 7
#define TYP_MVT 8
#define TYP_CNF 9
#define TYP_LNG 11
#define TYP_PLR 12
#define TYP_OLC 13

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Conditions.
 */
#define COND_DRUNK              0
#define COND_FULL               1
#define COND_THIRST             2
#define COND_HUNGER             3

/*
 * Positions.
 */
#define POS_DEAD                0
#define POS_MORTAL              1
#define POS_INCAP               2
#define POS_STUNNED             3
#define POS_SLEEPING            4
#define POS_RESTING             5
#define POS_SITTING             6
#define POS_FIGHTING            7
#define POS_STANDING            8

/*
 * ACT bits for players.
 */
#define PLR_IS_NPC              BIT(0) /* Don't EVER set.	*/

/* RT auto flags */
#define PLR_AUTOASSIST          BIT(2)
#define PLR_AUTOEXIT            BIT(3)
#define PLR_AUTOLOOT            BIT(4)
#define PLR_AUTOSAC             BIT(5)
#define PLR_AUTOGOLD            BIT(6)
#define PLR_AUTOSPLIT           BIT(7)

/* RT personal flags */
#define PLR_HOLYLIGHT           BIT(13)
#define PLR_CANLOOT             BIT(15)
#define PLR_NOSUMMON            BIT(16)
#define PLR_NOFOLLOW            BIT(17)
#define PLR_COLOUR              BIT(19)
/* 1 bit reserved, S */

/* penalty flags */
#define PLR_PERMIT              BIT(20)
#define PLR_LOG                 BIT(22)
#define PLR_DENY                BIT(23)
#define PLR_FREEZE              BIT(24)
#define PLR_THIEF               BIT(25)
#define PLR_KILLER              BIT(26)

/* RT comm flags -- may be used on both mobs and chars */
#define COMM_QUIET              BIT(0)
#define COMM_DEAF               BIT(1)
#define COMM_NOWIZ              BIT(2)
#define COMM_NOAUCTION          BIT(3)
#define COMM_NOGOSSIP           BIT(4)
#define COMM_NOQUESTION         BIT(5)
#define COMM_NOMUSIC            BIT(6)
#define COMM_NOCLAN             BIT(7)
#define COMM_NOQUOTE            BIT(8)
#define COMM_SHOUTSOFF          BIT(9)
#define COMM_OLCX               BIT(10)

/* display flags */
#define COMM_COMPACT            BIT(11)
#define COMM_BRIEF              BIT(12)
#define COMM_PROMPT             BIT(13)
#define COMM_COMBINE            BIT(14)
#define COMM_TELNET_GA          BIT(15)
#define COMM_SHOW_AFFECTS       BIT(16)
#define COMM_NOGRATS            BIT(17)

/* penalties */
#define COMM_NOEMOTE            BIT(19)
#define COMM_NOSHOUT            BIT(20)
#define COMM_NOTELL             BIT(21)
#define COMM_NOCHANNELS         BIT(22)
#define COMM_SNOOP_PROOF        BIT(24)
#define COMM_AFK                BIT(25)

/* WIZnet flags */
#define WIZ_ON                  BIT(0)
#define WIZ_TICKS               BIT(1)
#define WIZ_LOGINS              BIT(2)
#define WIZ_SITES               BIT(3)
#define WIZ_LINKS               BIT(4)
#define WIZ_DEATHS              BIT(5)
#define WIZ_RESETS              BIT(6)
#define WIZ_MOBDEATHS           BIT(7)
#define WIZ_FLAGS               BIT(8)
#define WIZ_PENALTIES           BIT(9)
#define WIZ_SACCING             BIT(10)
#define WIZ_LEVELS              BIT(11)
#define WIZ_SECURE              BIT(12)
#define WIZ_SWITCHES            BIT(13)
#define WIZ_SNOOPS              BIT(14)
#define WIZ_RESTORE             BIT(15)
#define WIZ_LOAD                BIT(16)
#define WIZ_NEWBIE              BIT(17)
#define WIZ_PREFIX              BIT(18)
#define WIZ_SPAM                BIT(19)

/* Data for generating characters -- only used during generation */
struct gen_data {
    GEN_DATA* next;
    bool* skill_chosen;
    bool* group_chosen;
    int points_chosen;
    bool valid;
};

/*
 * Liquids.
 */
#define LIQ_WATER 0

struct liq_type {
    char* liq_name;
    char* liq_color;
    int16_t liq_affect[5];
};

/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED     -1
#define TYPE_HIT           1000

/*
 *  Target types.
 */
#define TAR_IGNORE         0
#define TAR_CHAR_OFFENSIVE 1
#define TAR_CHAR_DEFENSIVE 2
#define TAR_CHAR_SELF      3
#define TAR_OBJ_INV        4
#define TAR_OBJ_CHAR_DEF   5
#define TAR_OBJ_CHAR_OFF   6

#define TARGET_CHAR        0
#define TARGET_OBJ         1
#define TARGET_ROOM        2
#define TARGET_NONE        3

/*
 * MOBprog definitions
 */
#define TRIG_ACT	BIT(0)
#define TRIG_BRIBE	BIT(1)
#define TRIG_DEATH	BIT(2)
#define TRIG_ENTRY	BIT(3)
#define TRIG_FIGHT	BIT(4)
#define TRIG_GIVE	BIT(5)
#define TRIG_GREET	BIT(6)
#define TRIG_GRALL	BIT(7)
#define TRIG_KILL	BIT(8)
#define TRIG_HPCNT	BIT(9)
#define TRIG_RANDOM	BIT(10)
#define TRIG_SPEECH	BIT(12)
#define TRIG_EXIT	BIT(13)
#define TRIG_EXALL	BIT(14)
#define TRIG_DELAY	BIT(15)
#define TRIG_SURR	BIT(16)

struct mprog_list {
    int trig_type;
    char* trig_phrase;
    VNUM vnum;
    char* code;
    MPROG_LIST* next;
    bool valid;
};

struct mprog_code {
    VNUM vnum;
    bool changed;
    char* code;
    MPROG_CODE* next;
};
 
// gsn
#define GSN(x) extern SKNUM x;
#include "gsn.h"
#undef GSN

/*
 * Utility macros.
 */
#define IS_VALID(data)       ((data) != NULL && (data)->valid)
#define VALIDATE(data)       ((data)->valid = true)
#define INVALIDATE(data)     ((data)->valid = false)
#define UMIN(a, b)           ((a) < (b) ? (a) : (b))
#define UMAX(a, b)           ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)      ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)             ((c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : (c))
#define UPPER(c)             ((c) >= 'a' && (c) <= 'z' ? (c) + 'A' - 'a' : (c))
#define IS_SET(flag, bit)    ((flag) & (bit))
#define SET_BIT(var, bit)    ((var) |= (bit))
#define REMOVE_BIT(var, bit) ((var) &= ~(bit))
#define TOGGLE_BIT(var, bit) ((var) ^= (bit))
#define IS_NULLSTR(str)		 ((str) == NULL || (str)[0] == '\0')
#define CHECKNULLSTR(str)    ((str) == NULL ? "" : (str))
#define CH(d)		         ((d)->original ? (d)->original : (d)->character)
#define BETWEEN(min,num,max) (((min) < (num)) && ((num) < (max)))
#define BETWEEN_I(x,y,z)     (((x) <= (y)) && ((y) <= (z)))
#define CHECK_POS(a, b, c)   \
    { \
        (a) = (b); \
        if ((a) < 0) \
            bug( "CHECK_POS : " c " == %d < 0", a ); \
    }
#define ARRAY_COPY(array1, array2, len) \
    { \
        int _xxx_; \
        for (_xxx_ = 0; _xxx_ < len; _xxx_++) \
            array1[_xxx_] = array2[_xxx_]; \
    }

/*
 * Structure for a social in the socials table.
 */
struct social_type {
    char* name;
    char* char_no_arg;
    char* others_no_arg;
    char* char_found;
    char* others_found;
    char* vict_found;
    char* char_not_found;
    char* char_auto;
    char* others_auto;
};

/*
 * Global constants.
 */
extern const struct str_app_type str_app[26];
extern const struct int_app_type int_app[26];
extern const struct wis_app_type wis_app[26];
extern const struct dex_app_type dex_app[26];
extern const struct con_app_type con_app[26];

extern const struct class_type class_table[MAX_CLASS];
extern const struct weapon_type weapon_table[];
extern const struct item_type item_table[];
extern const struct wiznet_type wiznet_table[];
extern const struct attack_type attack_table[];
extern       struct race_type* race_table;
extern       struct social_type* social_table;
extern const struct liq_type liq_table[];
extern char* const title_table[MAX_CLASS][MAX_LEVEL + 1][2];

/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

#define NO_FLAG         -99 // Must not be used in flags or stats.

////////////////////////////////////////////////////////////////////////////////
// Global Vars
////////////////////////////////////////////////////////////////////////////////

// main.c
extern time_t current_time;
extern FILE* fpReserve;
extern bool MOBtrigger;

// db.c
extern char str_empty[1];
extern	bool fBootDb;

#endif // !MUD98__MERC_H
