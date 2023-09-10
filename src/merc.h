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

typedef struct player_data_t PlayerData;
typedef struct char_data_t CharData;

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
#define DECLARE_DO_FUN( fun )       DO_FUN fun
#define DECLARE_SPEC_FUN( fun )     SPEC_FUN fun
#define DECLARE_SPELL_FUN( fun )    SPELL_FUN fun
#define DECLARE_LOOKUP_FUN( fun )   LOOKUP_F fun

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

////////////////////////////////////////////////////////////////////////////////
// Custom Types
////////////////////////////////////////////////////////////////////////////////

#define LEVEL               int16_t

#define SKNUM               int16_t

// If you want to change the type for VNUM's, this is where you do it.
#define MAX_VNUM            INT32_MAX
#define VNUM                int32_t
#define PRVNUM              PRId32
#define STRTOVNUM(s)        (VNUM)strtol(s, NULL, 0)
#define VNUM_NONE           -1

////////////////////////////////////////////////////////////////////////////////

/*
 * Structure types.
 */
typedef struct affect_data AFFECT_DATA;
typedef struct area_data AREA_DATA;
typedef struct ban_data BAN_DATA;
typedef struct buf_type BUFFER;
typedef struct descriptor_data DESCRIPTOR_DATA;
typedef struct exit_data EXIT_DATA;
typedef struct extra_descr_data EXTRA_DESCR_DATA;
typedef struct help_data HELP_DATA;
typedef struct help_area_data HELP_AREA;
typedef struct kill_data KILL_DATA;
typedef struct mem_data MEM_DATA;
typedef struct mob_index_data MOB_INDEX_DATA;
typedef struct mprog_list MPROG_LIST;
typedef struct mprog_code MPROG_CODE;
typedef struct note_data NOTE_DATA;
typedef struct obj_data OBJ_DATA;
typedef struct obj_index_data OBJ_INDEX_DATA;
typedef struct gen_data GEN_DATA;
typedef struct reset_data RESET_DATA;
typedef struct room_index_data ROOM_INDEX_DATA;
typedef struct shop_data SHOP_DATA;
typedef struct time_info_data TIME_INFO_DATA;
typedef struct weather_data WEATHER_DATA;

// New structures for Mud98 -- Halivar
typedef struct color_theme_t ColorTheme;

/*
 * Function types.
 */
typedef void DO_FUN(CharData * ch, char* argument);
typedef bool SPEC_FUN(CharData * ch);
typedef void SPELL_FUN(SKNUM sn, LEVEL level, CharData* ch, void* vo, int target);
typedef int	LOOKUP_F(const char*);

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

extern SKNUM max_skill;
extern int max_group;

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

/*
 * Site ban structure.
 */

#define BAN_SUFFIX    BIT(0)
#define BAN_PREFIX    BIT(1)
#define BAN_NEWBIES   BIT(2)
#define BAN_ALL       BIT(3)
#define BAN_PERMIT    BIT(4)
#define BAN_PERMANENT BIT(5)

struct ban_data {
    BAN_DATA* next;
    char* name;
    int ban_flags;
    LEVEL level;
    bool valid;
};

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
 * Help table types.
 */
struct help_data {
    HELP_DATA* next;
    HELP_DATA* next_area;
    int16_t level;
    char* keyword;
    char* text;
};

struct help_area_data {
    HELP_AREA* next;
    HELP_DATA* first;
    HELP_DATA* last;
    AREA_DATA* area;
    char* filename;
    bool changed;
};

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
    long act; /* act bits for the race */
    long aff; /* aff bits for the race */
    long off; /* off bits for the race */
    long imm; /* imm bits for the race */
    long res; /* res bits for the race */
    long vuln; /* vuln bits for the race */
    long form; /* default form flag for the race */
    long parts; /* default parts for the race */
    int16_t race_id;
    int16_t points;			/* cost in points of the race */
    int16_t class_mult[MAX_CLASS];	/* exp multiplier for class, * 100 */
    int16_t stats[MAX_STATS];	/* starting stats */
    int16_t max_stats[MAX_STATS];	/* maximum stats */
    int16_t size;			/* aff bits for the race */
};

struct spec_type {
    char* name; /* special function name */
    SPEC_FUN* function; /* the function */
};

/*
 * Data structure for notes.
 */

#define NOTE_NOTE    0
#define NOTE_IDEA    1
#define NOTE_PENALTY 2
#define NOTE_NEWS    3
#define NOTE_CHANGES 4
struct note_data {
    NOTE_DATA* next;
    char* sender;
    char* date;
    char* to_list;
    char* subject;
    char* text;
    time_t date_stamp;
    int16_t type;
    bool valid;
};

/*
 * An affect.
 */
struct affect_data {
    AFFECT_DATA* next;
    bool valid;
    int16_t where;
    SKNUM type;
    LEVEL level;
    int16_t duration;
    int16_t location;
    int16_t modifier;
    int bitvector;
};

/* where definitions */
#define TO_AFFECTS 0
#define TO_OBJECT  1
#define TO_IMMUNE  2
#define TO_RESIST  3
#define TO_VULN    4
#define TO_WEAPON  5

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
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
#define MOB_VNUM_FIDO           3090
#define MOB_VNUM_CITYGUARD      3060
#define MOB_VNUM_VAMPIRE        3404

#define MOB_VNUM_PATROLMAN      2106
#define GROUP_VNUM_TROLLS       2100
#define GROUP_VNUM_OGRES        2101

// Used for flags and other bit fields
#define BIT(x) (1 << x)

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
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
#define AFF_BLIND               BIT(0)
#define AFF_INVISIBLE           BIT(1)
#define AFF_DETECT_EVIL         BIT(2)
#define AFF_DETECT_INVIS        BIT(3)
#define AFF_DETECT_MAGIC        BIT(4)
#define AFF_DETECT_HIDDEN       BIT(5)
#define AFF_DETECT_GOOD         BIT(6)
#define AFF_SANCTUARY           BIT(7)
#define AFF_FAERIE_FIRE         BIT(8)
#define AFF_INFRARED            BIT(9)
#define AFF_CURSE               BIT(10)
#define AFF_UNUSED_FLAG         BIT(11) /* unused */
#define AFF_POISON              BIT(12)
#define AFF_PROTECT_EVIL        BIT(13)
#define AFF_PROTECT_GOOD        BIT(14)
#define AFF_SNEAK               BIT(15)
#define AFF_HIDE                BIT(16)
#define AFF_SLEEP               BIT(17)
#define AFF_CHARM               BIT(18)
#define AFF_FLYING              BIT(19)
#define AFF_PASS_DOOR           BIT(20)
#define AFF_HASTE               BIT(21)
#define AFF_CALM                BIT(22)
#define AFF_PLAGUE              BIT(23)
#define AFF_WEAKEN              BIT(24)
#define AFF_DARK_VISION         BIT(25)
#define AFF_BERSERK             BIT(26)
#define AFF_SWIM                BIT(27)
#define AFF_REGENERATION        BIT(28)
#define AFF_SLOW                BIT(29)

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
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_SILVER_ONE     1
#define OBJ_VNUM_GOLD_ONE       2
#define OBJ_VNUM_GOLD_SOME      3
#define OBJ_VNUM_SILVER_SOME    4
#define OBJ_VNUM_COINS          5

#define OBJ_VNUM_CORPSE_NPC     10
#define OBJ_VNUM_CORPSE_PC      11
#define OBJ_VNUM_SEVERED_HEAD   12
#define OBJ_VNUM_TORN_HEART     13
#define OBJ_VNUM_SLICED_ARM     14
#define OBJ_VNUM_SLICED_LEG     15
#define OBJ_VNUM_GUTS           16
#define OBJ_VNUM_BRAINS         17

#define OBJ_VNUM_MUSHROOM       20
#define OBJ_VNUM_LIGHT_BALL     21
#define OBJ_VNUM_SPRING         22
#define OBJ_VNUM_DISC           23
#define OBJ_VNUM_PORTAL         25

#define OBJ_VNUM_ROSE           1001

#define OBJ_VNUM_PIT            3010

#define OBJ_VNUM_SCHOOL_MACE    3700
#define OBJ_VNUM_SCHOOL_DAGGER  3701
#define OBJ_VNUM_SCHOOL_SWORD   3702
#define OBJ_VNUM_SCHOOL_SPEAR   3717
#define OBJ_VNUM_SCHOOL_STAFF   3718
#define OBJ_VNUM_SCHOOL_AXE     3719
#define OBJ_VNUM_SCHOOL_FLAIL   3720
#define OBJ_VNUM_SCHOOL_WHIP    3721
#define OBJ_VNUM_SCHOOL_POLEARM 3722

#define OBJ_VNUM_SCHOOL_VEST    3703
#define OBJ_VNUM_SCHOOL_SHIELD  3704
#define OBJ_VNUM_SCHOOL_BANNER  3716
#define OBJ_VNUM_MAP            3162

#define OBJ_VNUM_WHISTLE        2116

/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT              1
#define ITEM_SCROLL             2
#define ITEM_WAND               3
#define ITEM_STAFF              4
#define ITEM_WEAPON             5
#define ITEM_TREASURE           8
#define ITEM_ARMOR              9
#define ITEM_POTION             10
#define ITEM_CLOTHING           11
#define ITEM_FURNITURE          12
#define ITEM_TRASH              13
#define ITEM_CONTAINER          15
#define ITEM_DRINK_CON          17
#define ITEM_KEY                18
#define ITEM_FOOD               19
#define ITEM_MONEY              20
#define ITEM_BOAT               22
#define ITEM_CORPSE_NPC         23
#define ITEM_CORPSE_PC          24
#define ITEM_FOUNTAIN           25
#define ITEM_PILL               26
#define ITEM_PROTECT            27
#define ITEM_MAP                28
#define ITEM_PORTAL             29
#define ITEM_WARP_STONE         30
#define ITEM_ROOM_KEY           31
#define ITEM_GEM                32
#define ITEM_JEWELRY            33
#define ITEM_JUKEBOX            34

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
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
#define APPLY_NONE              0
#define APPLY_STR               1
#define APPLY_DEX               2
#define APPLY_INT               3
#define APPLY_WIS               4
#define APPLY_CON               5
#define APPLY_SEX               6
#define APPLY_CLASS             7
#define APPLY_LEVEL             8
#define APPLY_AGE               9
#define APPLY_HEIGHT            10
#define APPLY_WEIGHT            11
#define APPLY_MANA              12
#define APPLY_HIT               13
#define APPLY_MOVE              14
#define APPLY_GOLD              15
#define APPLY_EXP               16
#define APPLY_AC                17
#define APPLY_HITROLL           18
#define APPLY_DAMROLL           19
#define APPLY_SAVES             20
#define APPLY_SAVING_PARA       20
#define APPLY_SAVING_ROD        21
#define APPLY_SAVING_PETRI      22
#define APPLY_SAVING_BREATH     23
#define APPLY_SAVING_SPELL      24
#define APPLY_SPELL_AFFECT      25

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
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_LIMBO         2
#define ROOM_VNUM_CHAT          1200
#define ROOM_VNUM_TEMPLE        3001
#define ROOM_VNUM_ALTAR         3054
#define ROOM_VNUM_SCHOOL        3700
#define ROOM_VNUM_BALANCE       4500
#define ROOM_VNUM_CIRCLE        4400
#define ROOM_VNUM_DEMISE        4201
#define ROOM_VNUM_HONOR         4300

/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK               BIT(0)
#define ROOM_NO_MOB             BIT(2)
#define ROOM_INDOORS            BIT(3)

#define ROOM_PRIVATE            BIT(9)
#define ROOM_SAFE               BIT(10)
#define ROOM_SOLITARY           BIT(11)
#define ROOM_PET_SHOP           BIT(12)
#define ROOM_NO_RECALL          BIT(13)
#define ROOM_IMP_ONLY           BIT(14)
#define ROOM_GODS_ONLY          BIT(15)
#define ROOM_HEROES_ONLY        BIT(16)
#define ROOM_NEWBIES_ONLY       BIT(17)
#define ROOM_LAW                BIT(18)
#define ROOM_NOWHERE            BIT(19)

/*
 * Directions.
 * Used in #ROOMS.
 */
#define DIR_NORTH               0
#define DIR_EAST                1
#define DIR_SOUTH               2
#define DIR_WEST                3
#define DIR_UP                  4
#define DIR_DOWN                5

/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR               BIT(0)
#define EX_CLOSED               BIT(1)
#define EX_LOCKED               BIT(2)
#define EX_PICKPROOF            BIT(5)
#define EX_NOPASS               BIT(6)
#define EX_EASY                 BIT(7)
#define EX_HARD                 BIT(8)
#define EX_INFURIATING          BIT(9)
#define EX_NOCLOSE              BIT(10)
#define EX_NOLOCK               BIT(11)

/*
 * Sector types.
 * Used in #ROOMS.
 */
#define SECT_INSIDE             0
#define SECT_CITY               1
#define SECT_FIELD              2
#define SECT_FOREST             3
#define SECT_HILLS              4
#define SECT_MOUNTAIN           5
#define SECT_WATER_SWIM         6
#define SECT_WATER_NOSWIM       7
#define SECT_UNUSED             8
#define SECT_AIR                9
#define SECT_DESERT             10
#define SECT_MAX                11

/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
#define WEAR_NONE               -1
#define WEAR_LIGHT              0
#define WEAR_FINGER_L           1
#define WEAR_FINGER_R           2
#define WEAR_NECK_1             3
#define WEAR_NECK_2             4
#define WEAR_BODY               5
#define WEAR_HEAD               6
#define WEAR_LEGS               7
#define WEAR_FEET               8
#define WEAR_HANDS              9
#define WEAR_ARMS               10
#define WEAR_SHIELD             11
#define WEAR_ABOUT              12
#define WEAR_WAIST              13
#define WEAR_WRIST_L            14
#define WEAR_WRIST_R            15
#define WEAR_WIELD              16
#define WEAR_HOLD               17
#define WEAR_FLOAT              18
#define MAX_WEAR                19

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

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct mob_index_data {
    MOB_INDEX_DATA* next;
    SPEC_FUN* spec_fun;
    SHOP_DATA* pShop;
    MPROG_LIST* mprogs;
    AREA_DATA* area;        // OLC
    VNUM vnum;
    int16_t group;
    bool new_format;
    int16_t count;
    int16_t killed;
    char* player_name;
    char* short_descr;
    char* long_descr;
    char* description;
    long act;
    long affected_by;
    int16_t alignment;
    int16_t level;
    int16_t hitroll;
    int16_t hit[3];
    int16_t mana[3];
    int16_t damage[3];
    int16_t ac[4];
    int16_t dam_type;
    long off_flags;
    long imm_flags;
    long res_flags;
    long vuln_flags;
    int16_t start_pos;
    int16_t default_pos;
    int16_t sex;
    int16_t race;
    long wealth;
    long form;
    long parts;
    int16_t size;
    int16_t reset_num;
    char* material;
    long mprog_flags;
};

/* memory settings */
#define MEM_CUSTOMER BIT(0)
#define MEM_SELLER   BIT(1)
#define MEM_HOSTILE  BIT(2)
#define MEM_AFRAID   BIT(3)

/* memory for mobs */
struct mem_data {
    MEM_DATA* next;
    bool valid;
    int id;
    int reaction;
    time_t when;
};



typedef struct color_config_t {
    char* current_theme_name;   // For lazy-loading and discardability
    bool hide_256;          // Whether to show these higher-bit themes. Some
    bool hide_24bit;        // clients (like Windows CMD) can't handle them.
    bool xterm;             // Use xterm semi-colons for 24-bit colors.
    bool hide_rgb_help;     // Hide verbose 24-bit help at the end of THEME LIST.
} ColorConfig;

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
 * Extra description data for a room or object.
 */
struct extra_descr_data {
    EXTRA_DESCR_DATA* next; // Next in list
    bool valid;
    char* keyword;          // Keyword in look/examine
    char* description;      // What to see
};

/*
 * Prototype for an object.
 */
struct obj_index_data {
    OBJ_INDEX_DATA* next;
    EXTRA_DESCR_DATA* extra_descr;
    AFFECT_DATA* affected;
    AREA_DATA* area;        // OLC
    bool new_format;
    char* name;
    char* short_descr;
    char* description;
    VNUM vnum;
    int16_t reset_num;
    char* material;
    int16_t item_type;
    int extra_flags;
    int wear_flags;
    LEVEL level;
    int16_t condition;
    int16_t count;
    int16_t weight;
    int cost;
    int value[5];
};

/*
 * One object.
 */
struct obj_data {
    OBJ_DATA* next;
    OBJ_DATA* next_content;
    OBJ_DATA* contains;
    OBJ_DATA* in_obj;
    OBJ_DATA* on;
    CharData* carried_by;
    EXTRA_DESCR_DATA* extra_descr;
    AFFECT_DATA* affected;
    OBJ_INDEX_DATA* pIndexData;
    ROOM_INDEX_DATA* in_room;
    bool valid;
    bool enchanted;
    char* owner;
    char* name;
    char* short_descr;
    char* description;
    int16_t item_type;
    int extra_flags;
    int wear_flags;
    int16_t wear_loc;
    int16_t weight;
    int cost;
    LEVEL level;
    int16_t condition;
    char* material;
    int16_t timer;
    int value[5];
};

/*
 * Exit data.
 */
struct exit_data {
    union {
        ROOM_INDEX_DATA* to_room;
        VNUM vnum;
    } u1;
    int16_t exit_info;
    int16_t key;
    char* keyword;
    char* description;
    EXIT_DATA* next;    // OLC
    int16_t rs_flags;   // OLC
    int orig_door;      // OLC
};

/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct reset_data {
    RESET_DATA* next;
    char command;
    VNUM arg1;          
    int16_t arg2;       
    VNUM arg3;          
    int16_t arg4;       
};

/*
 * Area definition.
 */
struct area_data {
    AREA_DATA* next;
    HELP_AREA* helps;
    char* file_name;
    char* name;
    char* credits;
    int16_t age;
    int nplayer;
    LEVEL low_range;
    LEVEL high_range;
    VNUM min_vnum;
    VNUM max_vnum;
    bool empty;
    char* builders;     // OLC
    VNUM vnum;          // OLC
    int area_flags;     // OLC
    int security;       // OLC Value 1-9
};

/*
 * Room type.
 */
struct room_index_data {
    ROOM_INDEX_DATA* next;
    CharData* people;
    OBJ_DATA* contents;
    EXTRA_DESCR_DATA* extra_descr;
    AREA_DATA* area;
    EXIT_DATA* exit[6];
    RESET_DATA* reset_first;    // OLC
    RESET_DATA* reset_last;     // OLC
    char* name;
    char* description;
    char* owner;
    VNUM vnum;
    int room_flags;
    int16_t light;
    int16_t sector_type;
    int16_t heal_rate;
    int16_t mana_rate;
    int16_t clan;
    int16_t reset_num;
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
 * Skills include spells as a particular case.
 */
struct skill_type {
    char* name; /* Name of skill		*/
    LEVEL skill_level[MAX_CLASS]; /* Level needed by class	*/
    int rating[MAX_CLASS]; /* How hard it is to learn	*/
    SPELL_FUN* spell_fun; /* Spell pointer (for spells)	*/
    int target; /* Legal targets		*/
    int minimum_position; /* Position for caster / user	*/
    SKNUM* pgsn; /* Pointer to associated gsn	*/
    int slot; /* Slot for #OBJECT loading	*/
    int min_mana; /* Minimum mana used		*/
    int beats; /* Waiting time after use	*/
    char* noun_damage; /* Damage message		*/
    char* msg_off; /* Wear off message		*/
    char* msg_obj; /* Wear off message for obects	*/
};

struct group_type {
    char* name;
    int rating[MAX_CLASS];
    char* spells[MAX_IN_GROUP];
};

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
 * Character macros.
 */
#define IS_NPC(ch)            (IS_SET((ch)->act, ACT_IS_NPC))
#define IS_IMMORTAL(ch)       (get_trust(ch) >= LEVEL_IMMORTAL)
#define IS_HERO(ch)           (get_trust(ch) >= LEVEL_HERO)
#define IS_TRUSTED(ch, level) (get_trust((ch)) >= (level))
#define IS_AFFECTED(ch, sn)   (IS_SET((ch)->affected_by, (sn)))

#define GET_AGE(ch)                                                            \
    ((int)(17 + ((ch)->played + current_time - (ch)->logon) / 72000))

#define IS_GOOD(ch)    (ch->alignment >= 350)
#define IS_EVIL(ch)    (ch->alignment <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))

#define IS_AWAKE(ch)   (ch->position > POS_SLEEPING)
#define GET_AC(ch, type)                                                       \
    ((ch)->armor[type]                                                         \
     + (IS_AWAKE(ch) ? dex_app[get_curr_stat(ch, STAT_DEX)].defensive : 0))
#define GET_HITROLL(ch)                                                        \
    ((ch)->hitroll + str_app[get_curr_stat(ch, STAT_STR)].tohit)
#define GET_DAMROLL(ch)                                                        \
    ((ch)->damroll + str_app[get_curr_stat(ch, STAT_STR)].todam)

#define IS_OUTSIDE(ch)         (!IS_SET((ch)->in_room->room_flags, ROOM_INDOORS))

#define WAIT_STATE(ch, npulse) ((ch)->wait = UMAX((ch)->wait, (npulse)))
#define DAZE_STATE(ch, npulse) ((ch)->daze = UMAX((ch)->daze, (npulse)))
#define get_carry_weight(ch)                                                   \
    ((ch)->carry_weight + (ch)->silver / 10 + (ch)->gold * 2 / 5)

#define act(format, ch, arg1, arg2, type)                                      \
    act_new((format), (ch), (arg1), (arg2), (type), POS_RESTING)

#define HAS_TRIGGER(ch, trig) (IS_SET((ch)->pIndexData->mprog_flags, (trig)))
#define IS_SWITCHED(ch) (ch->desc && ch->desc->original)
#define IS_BUILDER(ch, Area) (!IS_NPC(ch) && !IS_SWITCHED(ch) && \
                (ch->pcdata->security >= Area->security \
                || strstr(Area->builders, ch->name) \
                || strstr(Area->builders, "All")))

/*
 * Object macros.
 */
#define CAN_WEAR(obj, part)       (IS_SET((obj)->wear_flags, (part)))
#define IS_OBJ_STAT(obj, stat)    (IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj, stat) (IS_SET((obj)->value[4], (stat)))
#define WEIGHT_MULT(obj)                                                       \
    ((obj)->item_type == ITEM_CONTAINER ? (obj)->value[4] : 100)

/*
 * Description macros.
 */
#define PERS(ch, looker)                                                       \
    (can_see(looker, (ch)) ? (IS_NPC(ch) ? (ch)->short_descr : (ch)->name)     \
                           : "someone")

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
extern       struct skill_type* skill_table;
extern       struct group_type* group_table;
extern       struct social_type* social_table;
extern const struct spec_type spec_table[];
extern const struct liq_type liq_table[];
extern char* const title_table[MAX_CLASS][MAX_LEVEL + 1][2];

/*
 * Global variables.
 */
extern HELP_DATA* help_first;
extern SHOP_DATA* shop_first;

extern DESCRIPTOR_DATA* descriptor_list;
extern OBJ_DATA* object_list;

extern MPROG_CODE* mprog_list;

extern char bug_buf[];
extern time_t current_time;
extern bool fLogAll;
extern FILE* fpReserve;
extern KILL_DATA kill_table[];
extern char log_buf[];
extern TIME_INFO_DATA time_info;
extern WEATHER_DATA weather_info;
extern bool MOBtrigger;

/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
#define CD          CharData
#define MID         MOB_INDEX_DATA
#define OD          OBJ_DATA
#define OID         OBJ_INDEX_DATA
#define RID         ROOM_INDEX_DATA
#define SF          SPEC_FUN
#define AD          AFFECT_DATA
#define MPC         MPROG_CODE

/* act_comm.c */
void add_follower(CharData * ch, CharData* master);
void stop_follower(CharData * ch);
void nuke_pets(CharData * ch);
void die_follower(CharData * ch);
bool is_same_group(CharData * ach, CharData* bch);

/* act_enter.c */
RID* get_random_room(CharData * ch);

/* act_info.c */
void set_title(CharData * ch, char* title);

/* act_move.c */
void move_char(CharData * ch, int door, bool follow);

/* act_obj.c */
bool can_loot(CharData * ch, OBJ_DATA* obj);
void wear_obj(CharData* ch, OBJ_DATA* obj, bool fReplace);
void get_obj(CharData * ch, OBJ_DATA* obj, OBJ_DATA* container);

/* act_wiz.c */
void wiznet(char* string, CharData* ch, OBJ_DATA* obj, long flag,
                  long flag_skip, int min_level);
/* alias.c */
void substitute_alias(DESCRIPTOR_DATA * d, char* input);

/* ban.c */
bool check_ban(char* site, int type);

/* comm.c */
void show_string(struct descriptor_data * d, char* input);
void close_socket(DESCRIPTOR_DATA * dclose);
void write_to_buffer(DESCRIPTOR_DATA * d, const char* txt, size_t length);
void send_to_char(const char* txt, CharData* ch);
void page_to_char(const char* txt, CharData* ch);
void act_new(const char* format, CharData* ch, const void* arg1,
                   const void* arg2, int type, int min_pos);
void printf_to_char(CharData*, char*, ...);
void bugf(char*, ...);
void flog(char*, ...);

/*
 * Colour stuff by Lope
 */
size_t colour(char type, CharData* ch, char* string);
void colourconv(char* buffer, const char* txt, CharData* ch);
void send_to_char_bw(const char* txt, CharData* ch);
void page_to_char_bw(const char* txt, CharData* ch);

/* db.c */
void reset_area(AREA_DATA* pArea);       // OLC
void reset_room(ROOM_INDEX_DATA* pRoom);	// OLC
char* print_flags(int flag);
void boot_db(void);
void area_update(void);
CD* create_mobile(MOB_INDEX_DATA * pMobIndex);
void clone_mobile(CharData * parent, CharData* clone);
OD* create_object(OBJ_INDEX_DATA * pObjIndex, LEVEL level);
void clone_object(OBJ_DATA * parent, OBJ_DATA* clone);
void clear_char(CharData * ch);
char* get_extra_descr(const char* name, EXTRA_DESCR_DATA* ed);
MID* get_mob_index(VNUM vnum);
OID* get_obj_index(VNUM vnum);
RID* get_room_index(VNUM vnum);
MPC* get_mprog_index(VNUM vnum);
char fread_letter(FILE * fp);
int fread_number(FILE * fp);
long fread_flag(FILE * fp);
char* fread_string(FILE * fp);
char* fread_string_eol(FILE * fp);
void fread_to_eol(FILE * fp);
VNUM fread_vnum(FILE* fp);
char* fread_word(FILE * fp);
long flag_convert(char letter);
void* alloc_mem(size_t sMem);
void* alloc_perm(size_t sMem);
void free_mem(void* pMem, size_t sMem);
char* str_dup(const char* str);
void free_string(char* pstr);
int number_fuzzy(int number);
int number_range(int from, int to);
int number_percent(void);
int number_door(void);
int number_bits(int width);
long number_mm(void);
int dice(int number, int size);
int interpolate(int level, int value_00, int value_32);
void smash_tilde(char* str);
bool str_cmp(const char* astr, const char* bstr);
bool str_prefix(const char* astr, const char* bstr);
bool str_infix(const char* astr, const char* bstr);
bool str_suffix(const char* astr, const char* bstr);
char* capitalize(const char* str);
void append_file(CharData * ch, char* file, char* str);
void bug(const char* fmt, ...);
void log_string(const char* str);

/* effect.c */
void acid_effect(void* vo, LEVEL level, int dam, int target);
void cold_effect(void* vo, LEVEL level, int dam, int target);
void fire_effect(void* vo, LEVEL level, int dam, int target);
void poison_effect(void* vo, LEVEL level, int dam, int target);
void shock_effect(void* vo, LEVEL level, int dam, int target);

/* fight.c */
bool is_safe(CharData * ch, CharData* victim);
bool is_safe_spell(CharData * ch, CharData* victim, bool area);
void violence_update(void);
void multi_hit(CharData * ch, CharData* victim, SKNUM dt);
bool damage(CharData * ch, CharData* victim, int dam, SKNUM dt, 
    DamageType dam_type, bool show);
void update_pos(CharData * victim);
void stop_fighting(CharData * ch, bool fBoth);
void check_killer(CharData * ch, CharData* victim);

/* handler.c */
AD* affect_find(AFFECT_DATA * paf, SKNUM sn);
void affect_check(CharData * ch, int where, int vector);
int count_users(OBJ_DATA * obj);
void deduct_cost(CharData * ch, int cost);
void affect_enchant(OBJ_DATA * obj);
int check_immune(CharData * ch, DamageType dam_type);
int material_lookup(const char* name);
int weapon_lookup(const char* name);
int weapon_type(const char* name);
char* weapon_name(int weapon_Type);
char* item_name(int item_type);
int attack_lookup(const char* name);
int wiznet_lookup(const char* name);
int16_t class_lookup(const char* name);
bool is_clan(CharData * ch);
bool is_same_clan(CharData * ch, CharData* victim);
bool is_old_mob(CharData * ch);
int get_skill(CharData * ch, SKNUM sn);
SKNUM get_weapon_sn(CharData * ch);
int get_weapon_skill(CharData* ch, SKNUM sn);
int get_age(CharData * ch);
void reset_char(CharData * ch);
LEVEL get_trust(CharData* ch);
int get_curr_stat(CharData * ch, int stat);
int get_max_train(CharData * ch, int stat);
int can_carry_n(CharData * ch);
int can_carry_w(CharData * ch);
bool is_name(char* str, char* namelist);
bool is_exact_name(char* str, char* namelist);
void affect_to_char(CharData * ch, AFFECT_DATA* paf);
void affect_to_obj(OBJ_DATA * obj, AFFECT_DATA* paf);
void affect_remove(CharData * ch, AFFECT_DATA* paf);
void affect_remove_obj(OBJ_DATA * obj, AFFECT_DATA* paf);
void affect_strip(CharData* ch, SKNUM sn);
bool is_affected(CharData* ch, SKNUM sn);
void affect_join(CharData * ch, AFFECT_DATA* paf);
void char_from_room(CharData * ch);
void char_to_room(CharData * ch, ROOM_INDEX_DATA* pRoomIndex);
void obj_to_char(OBJ_DATA * obj, CharData* ch);
void obj_from_char(OBJ_DATA * obj);
int apply_ac(OBJ_DATA * obj, int iWear, int type);
OD* get_eq_char(CharData * ch, int iWear);
void equip_char(CharData* ch, OBJ_DATA* obj, int16_t iWear);
void unequip_char(CharData * ch, OBJ_DATA* obj);
int count_obj_list(OBJ_INDEX_DATA * obj, OBJ_DATA* list);
void obj_from_room(OBJ_DATA * obj);
void obj_to_room(OBJ_DATA * obj, ROOM_INDEX_DATA* pRoomIndex);
void obj_to_obj(OBJ_DATA * obj, OBJ_DATA* obj_to);
void obj_from_obj(OBJ_DATA * obj);
void extract_obj(OBJ_DATA * obj);
void extract_char(CharData * ch, bool fPull);
CD* get_char_room(CharData * ch, char* argument);
CD* get_char_world(CharData * ch, char* argument);
OD* get_obj_type(OBJ_INDEX_DATA * pObjIndexData);
OD* get_obj_list(CharData * ch, char* argument, OBJ_DATA* list);
OD* get_obj_carry(CharData * ch, char* argument, CharData* viewer);
OD* get_obj_wear(CharData * ch, char* argument);
OD* get_obj_here(CharData * ch, char* argument);
OD* get_obj_world(CharData * ch, char* argument);
OD* create_money(int16_t gold, int16_t silver);
int get_obj_number(OBJ_DATA * obj);
int get_obj_weight(OBJ_DATA * obj);
int get_true_weight(OBJ_DATA * obj);
bool room_is_dark(ROOM_INDEX_DATA * pRoomIndex);
bool is_room_owner(CharData * ch, ROOM_INDEX_DATA* room);
bool room_is_private(ROOM_INDEX_DATA * pRoomIndex);
bool can_see(CharData * ch, CharData* victim);
bool can_see_obj(CharData * ch, OBJ_DATA* obj);
bool can_see_room(CharData * ch, ROOM_INDEX_DATA* pRoomIndex);
bool can_drop_obj(CharData * ch, OBJ_DATA* obj);
char* affect_loc_name(int location);
char* affect_bit_name(int vector);
char* extra_bit_name(int extra_flags);
char* wear_bit_name(int wear_flags);
char* act_bit_name(int act_flags);
char* off_bit_name(int off_flags);
char* imm_bit_name(int imm_flags);
char* form_bit_name(int form_flags);
char* part_bit_name(int part_flags);
char* weapon_bit_name(int weapon_flags);
char* comm_bit_name(int comm_flags);
char* cont_bit_name(int cont_flags);
bool emptystring(const char*);
char* itos(int);
int get_vnum_mob_name_area(char*, AREA_DATA*);
int get_vnum_obj_name_area(char*, AREA_DATA*);
int get_points(int race, int args);

/*
 * Colour Config
 */
void set_default_theme(CharData * ch);
void all_colour(CharData * ch, char* argument);

/* interp.c */
void interpret(CharData * ch, char* argument);
bool is_number(char* arg);
int number_argument(char* argument, char* arg);
int mult_argument(char* argument, char* arg);
char* one_argument(char* argument, char* arg_first);

/* magic.c */
SKNUM find_spell(CharData * ch, const char* name);
int mana_cost(CharData* ch, int min_mana, LEVEL level);
SKNUM skill_lookup(const char* name);
SKNUM skill_slot_lookup(int slot);
bool saves_spell(LEVEL level, CharData* victim, DamageType dam_type);
void obj_cast_spell(SKNUM sn, LEVEL level, CharData* ch, CharData* victim,
                          OBJ_DATA* obj);


/* mob_prog.c */
void program_flow(VNUM vnum, char* source, CharData* mob, 
                        CharData* ch, const void* arg1, const void* arg2);
void mp_act_trigger(char* argument, CharData* mob, CharData* ch,
                          const void* arg1, const void* arg2, int type);
bool mp_percent_trigger(CharData* mob, CharData* ch, const void* arg1, 
                              const void* arg2, int type);
void mp_bribe_trigger(CharData* mob, CharData* ch, int amount);
bool mp_exit_trigger(CharData* ch, int dir);
void mp_give_trigger(CharData* mob, CharData* ch, OBJ_DATA* obj);
void mp_greet_trigger(CharData* ch);
void mp_hprct_trigger(CharData* mob, CharData* ch);

/* mob_cmds.c */
void mob_interpret(CharData* ch, char* argument);
char* mprog_type_to_name(int);

/* save.c */
void save_char_obj(CharData * ch);
bool load_char_obj(DESCRIPTOR_DATA * d, char* name);
int	race_exp_per_level(int race, int ch_class, int points);

/* skills.c */
bool parse_gen_groups(CharData * ch, char* argument);
void list_group_costs(CharData * ch);
void list_group_known(CharData * ch);
int exp_per_level(CharData * ch, int points);
void check_improve(CharData * ch, int sn, bool success, int multiplier);
int group_lookup(const char* name);
void gn_add(CharData * ch, int gn);
void gn_remove(CharData * ch, int gn);
void group_add(CharData * ch, const char* name, bool deduct);
void group_remove(CharData * ch, const char* name);

/* special.c */
SF* spec_lookup(const char* name);
char* spec_name(SPEC_FUN * function);

/* string_edit.c */
void string_edit(CharData* ch, char** pString);
void string_append(CharData* ch, char** pString);
char* string_replace(char* orig, char* old, char* new_str);
void string_add(CharData* ch, char* argument);
char* format_string(char* oldstring /*, bool fSpace */);
char* first_arg(char* argument, char* arg_first, bool fCase);
char* string_unpad(char* argument);
char* string_proper(char* argument);

/* olc.c */
bool run_olc_editor(DESCRIPTOR_DATA* d, char* incomm);
char* olc_ed_name(CharData* ch);
char* olc_ed_vnum(CharData* ch);

/* lookup.c */
int16_t race_lookup(const char* name);
int item_lookup(const char* name);
int liq_lookup(const char* name);

/* teleport.c */
RID* room_by_name(char* target, LEVEL level, bool error);

/* update.c */
void advance_level(CharData * ch, bool hide);
void gain_exp(CharData * ch, int gain);
void gain_condition(CharData * ch, int iCond, int value);
void update_handler(void);

#undef CD
#undef MID
#undef OD
#undef OID
#undef RID
#undef SF
#undef AD


/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

/*
 * Object defined in limbo.are
 * Used in save.c to load objects that don't exist.
 */
#define OBJ_VNUM_DUMMY	30

/*
 * Area flags.
 */
#define AREA_NONE       0
#define AREA_CHANGED    1	// Area has been modified.
#define AREA_ADDED      2	// Area has been added to.
#define AREA_LOADING    4	// Used for counting in db.c

#define MAX_DIR	        6
#define NO_FLAG         -99 // Must not be used in flags or stats.

/*
 * Global Constants
 */
extern char* const dir_name[];
extern const int16_t rev_dir[];
extern const struct	spec_type spec_table[];

/*
 * Global variables
 */
extern AREA_DATA* area_first;
extern AREA_DATA* area_last;
extern SHOP_DATA* shop_last;

extern int top_affect;
extern int top_area;
extern int top_ed;
extern int top_exit;
extern int top_help;
extern int top_mob_index;
extern int top_obj_index;
extern int top_reset;
extern int top_room;
extern int top_shop;

extern VNUM top_vnum_mob;
extern VNUM top_vnum_obj;
extern VNUM top_vnum_room;

extern char str_empty[1];

extern MOB_INDEX_DATA* mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA* obj_index_hash[MAX_KEY_HASH];
extern ROOM_INDEX_DATA* room_index_hash[MAX_KEY_HASH];

#endif // !MUD98__MERC_H
