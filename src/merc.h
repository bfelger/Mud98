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
#ifndef ROM__MERC_H
#define ROM__MERC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifndef _MSC_VER
#include <stddef.h>
#endif

#define args( list )             list
#define DECLARE_DO_FUN( fun )    DO_FUN fun
#define DECLARE_SPEC_FUN( fun )  SPEC_FUN fun
#define DECLARE_SPELL_FUN( fun ) SPELL_FUN fun

/*
 * Structure types.
 */
typedef struct affect_data AFFECT_DATA;
typedef struct area_data AREA_DATA;
typedef struct ban_data BAN_DATA;
typedef struct buf_type BUFFER;
typedef struct char_data CHAR_DATA;
typedef struct colour_data COLOUR_DATA;
typedef struct descriptor_data DESCRIPTOR_DATA;
typedef struct exit_data EXIT_DATA;
typedef struct extra_descr_data EXTRA_DESCR_DATA;
typedef struct help_data HELP_DATA;
typedef struct kill_data KILL_DATA;
typedef struct mem_data MEM_DATA;
typedef struct mob_index_data MOB_INDEX_DATA;
typedef struct note_data NOTE_DATA;
typedef struct obj_data OBJ_DATA;
typedef struct obj_index_data OBJ_INDEX_DATA;
typedef struct pc_data PC_DATA;
typedef struct gen_data GEN_DATA;
typedef struct reset_data RESET_DATA;
typedef struct room_index_data ROOM_INDEX_DATA;
typedef struct shop_data SHOP_DATA;
typedef struct time_info_data TIME_INFO_DATA;
typedef struct weather_data WEATHER_DATA;

/*
 * Function types.
 */
typedef void DO_FUN args((CHAR_DATA * ch, char* argument));
typedef bool SPEC_FUN args((CHAR_DATA * ch));
typedef void SPELL_FUN args((int sn, int level, CHAR_DATA* ch, void* vo,
                             int target));

/*
 * String and memory management parameters.
 */
#define MAX_KEY_HASH       1024
#define MAX_STRING_LENGTH  4608
#define MAX_INPUT_LENGTH   256
#define PAGELEN            22

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_SOCIALS        256
#define MAX_SKILL          150
#define MAX_GROUP          30
#define MAX_IN_GROUP       15
#define MAX_ALIAS          5
#define MAX_CLASS          4
#define MAX_PC_RACE        5
#define MAX_CLAN           3
#define MAX_DAMAGE_MESSAGE 41
#define MAX_LEVEL          60
#define LEVEL_HERO         (MAX_LEVEL - 9)
#define LEVEL_IMMORTAL     (MAX_LEVEL - 8)

#define PULSE_PER_SECOND   4
#define PULSE_VIOLENCE     (3 * PULSE_PER_SECOND)
#define PULSE_MOBILE       (4 * PULSE_PER_SECOND)
#define PULSE_MUSIC        (6 * PULSE_PER_SECOND)
#define PULSE_TICK         (60 * PULSE_PER_SECOND)
#define PULSE_AREA         (120 * PULSE_PER_SECOND)

#define IMPLEMENTOR        MAX_LEVEL
#define CREATOR            (MAX_LEVEL - 1)
#define SUPREME            (MAX_LEVEL - 2)
#define DEITY              (MAX_LEVEL - 3)
#define GOD                (MAX_LEVEL - 4)
#define IMMORTAL           (MAX_LEVEL - 5)
#define DEMI               (MAX_LEVEL - 6)
#define ANGEL              (MAX_LEVEL - 7)
#define AVATAR             (MAX_LEVEL - 8)
#define HERO               LEVEL_HERO

/*
 * ColoUr stuff v2.0, by Lope.
 */
#define CLEAR              "\033[0m" /* Resets Colour	*/
#define C_RED              "\033[0;31m" /* Normal Colours	*/
#define C_GREEN            "\033[0;32m"
#define C_YELLOW           "\033[0;33m"
#define C_BLUE             "\033[0;34m"
#define C_MAGENTA          "\033[0;35m"
#define C_CYAN             "\033[0;36m"
#define C_WHITE            "\033[0;37m"
#define C_D_GREY           "\033[1;30m" /* Light Colors		*/
#define C_B_RED            "\033[1;31m"
#define C_B_GREEN          "\033[1;32m"
#define C_B_YELLOW         "\033[1;33m"
#define C_B_BLUE           "\033[1;34m"
#define C_B_MAGENTA        "\033[1;35m"
#define C_B_CYAN           "\033[1;36m"
#define C_B_WHITE          "\033[1;37m"

#define COLOUR_NONE        7 /* White, hmm...	*/
#define RED                1 /* Normal Colours	*/
#define GREEN              2
#define YELLOW             3
#define BLUE               4
#define MAGENTA            5
#define CYAN               6
#define WHITE              7
#define BLACK              0

#define NORMAL             0 /* Bright/Normal colours */
#define BRIGHT             1

#define ALTER_COLOUR(type)                                                     \
    if (!str_prefix(argument, "red")) {                                        \
        ch->pcdata->type[0] = NORMAL;                                          \
        ch->pcdata->type[1] = RED;                                             \
    }                                                                          \
    else if (!str_prefix(argument, "hi-red")) {                                \
        ch->pcdata->type[0] = BRIGHT;                                          \
        ch->pcdata->type[1] = RED;                                             \
    }                                                                          \
    else if (!str_prefix(argument, "green")) {                                 \
        ch->pcdata->type[0] = NORMAL;                                          \
        ch->pcdata->type[1] = GREEN;                                           \
    }                                                                          \
    else if (!str_prefix(argument, "hi-green")) {                              \
        ch->pcdata->type[0] = BRIGHT;                                          \
        ch->pcdata->type[1] = GREEN;                                           \
    }                                                                          \
    else if (!str_prefix(argument, "yellow")) {                                \
        ch->pcdata->type[0] = NORMAL;                                          \
        ch->pcdata->type[1] = YELLOW;                                          \
    }                                                                          \
    else if (!str_prefix(argument, "hi-yellow")) {                             \
        ch->pcdata->type[0] = BRIGHT;                                          \
        ch->pcdata->type[1] = YELLOW;                                          \
    }                                                                          \
    else if (!str_prefix(argument, "blue")) {                                  \
        ch->pcdata->type[0] = NORMAL;                                          \
        ch->pcdata->type[1] = BLUE;                                            \
    }                                                                          \
    else if (!str_prefix(argument, "hi-blue")) {                               \
        ch->pcdata->type[0] = BRIGHT;                                          \
        ch->pcdata->type[1] = BLUE;                                            \
    }                                                                          \
    else if (!str_prefix(argument, "magenta")) {                               \
        ch->pcdata->type[0] = NORMAL;                                          \
        ch->pcdata->type[1] = MAGENTA;                                         \
    }                                                                          \
    else if (!str_prefix(argument, "hi-magenta")) {                            \
        ch->pcdata->type[0] = BRIGHT;                                          \
        ch->pcdata->type[1] = MAGENTA;                                         \
    }                                                                          \
    else if (!str_prefix(argument, "cyan")) {                                  \
        ch->pcdata->type[0] = NORMAL;                                          \
        ch->pcdata->type[1] = CYAN;                                            \
    }                                                                          \
    else if (!str_prefix(argument, "hi-cyan")) {                               \
        ch->pcdata->type[0] = BRIGHT;                                          \
        ch->pcdata->type[1] = CYAN;                                            \
    }                                                                          \
    else if (!str_prefix(argument, "white")) {                                 \
        ch->pcdata->type[0] = NORMAL;                                          \
        ch->pcdata->type[1] = WHITE;                                           \
    }                                                                          \
    else if (!str_prefix(argument, "hi-white")) {                              \
        ch->pcdata->type[0] = BRIGHT;                                          \
        ch->pcdata->type[1] = WHITE;                                           \
    }                                                                          \
    else if (!str_prefix(argument, "grey")) {                                  \
        ch->pcdata->type[0] = BRIGHT;                                          \
        ch->pcdata->type[1] = BLACK;                                           \
    }                                                                          \
    else if (!str_prefix(argument, "beep")) {                                  \
        ch->pcdata->type[2] = 1;                                               \
    }                                                                          \
    else if (!str_prefix(argument, "nobeep")) {                                \
        ch->pcdata->type[2] = 0;                                               \
    }                                                                          \
    else {                                                                     \
        send_to_char_bw("Unrecognised colour, unchanged.\n\r", ch);            \
        return;                                                                \
    }

#define LOAD_COLOUR(field)                                                     \
    ch->pcdata->field[1] = fread_number(fp);                                   \
    if (ch->pcdata->field[1] > 100) {                                          \
        ch->pcdata->field[1] -= 100;                                           \
        ch->pcdata->field[2] = 1;                                              \
    }                                                                          \
    else {                                                                     \
        ch->pcdata->field[2] = 0;                                              \
    }                                                                          \
    if (ch->pcdata->field[1] > 10) {                                           \
        ch->pcdata->field[1] -= 10;                                            \
        ch->pcdata->field[0] = 1;                                              \
    }                                                                          \
    else {                                                                     \
        ch->pcdata->field[0] = 0;                                              \
    }

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
    bool valid;
    int16_t ban_flags;
    int16_t level;
    char* name;
};

struct buf_type {
    BUFFER* next;
    bool valid;
    int16_t state; /* error state of the buffer */
    int16_t size; /* size in k */
    char* string; /* buffer's string */
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
    int16_t level;
    char* keyword;
    char* text;
};

/*
 * Shop types.
 */
#define MAX_TRADE 5

struct shop_data {
    SHOP_DATA* next; /* Next shop in list		*/
    int16_t keeper; /* Vnum of shop keeper mob	*/
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
    int16_t vnum;
    int16_t type;
    int16_t* gsn;
};

struct wiznet_type {
    char* name;
    long flag;
    int level;
};

struct attack_type {
    char* name; /* name */
    char* noun; /* message */
    int damage; /* damage class */
};

struct race_type {
    char* name; /* call name of the race */
    bool pc_race; /* can be chosen by pcs */
    long act; /* act bits for the race */
    long aff; /* aff bits for the race */
    long off; /* off bits for the race */
    long imm; /* imm bits for the race */
    long res; /* res bits for the race */
    long vuln; /* vuln bits for the race */
    long form; /* default form flag for the race */
    long parts; /* default parts for the race */
};

struct pc_race_type /* additional data for pc races */
{
    char* name; /* MUST be in race_type */
    char who_name[6];
    int16_t points; /* cost in points of the race */
    int16_t class_mult[MAX_CLASS]; /* exp multiplier for class, * 100 */
    char* skills[5]; /* bonus skills for the race */
    int16_t stats[MAX_STATS]; /* starting stats */
    int16_t max_stats[MAX_STATS]; /* maximum stats */
    int16_t size; /* aff bits for the race */
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
    bool valid;
    int16_t type;
    char* sender;
    char* date;
    char* to_list;
    char* subject;
    char* text;
    time_t date_stamp;
};

/*
 * An affect.
 */
struct affect_data {
    AFFECT_DATA* next;
    bool valid;
    int16_t where;
    int16_t type;
    int16_t level;
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

/* damage classes */
#define DAM_NONE                0
#define DAM_BASH                1
#define DAM_PIERCE              2
#define DAM_SLASH               3
#define DAM_FIRE                4
#define DAM_COLD                5
#define DAM_LIGHTNING           6
#define DAM_ACID                7
#define DAM_POISON              8
#define DAM_NEGATIVE            9
#define DAM_HOLY                10
#define DAM_ENERGY              11
#define DAM_MENTAL              12
#define DAM_DISEASE             13
#define DAM_DROWNING            14
#define DAM_LIGHT               15
#define DAM_OTHER               16
#define DAM_HARM                17
#define DAM_CHARM               18
#define DAM_SOUND               19

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
    int16_t vnum;
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
    char* material;
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

/*
 * One character (PC or NPC).
 */
struct char_data {
    CHAR_DATA* next;
    CHAR_DATA* next_in_room;
    CHAR_DATA* master;
    CHAR_DATA* leader;
    CHAR_DATA* fighting;
    CHAR_DATA* reply;
    CHAR_DATA* pet;
    MEM_DATA* memory;
    SPEC_FUN* spec_fun;
    MOB_INDEX_DATA* pIndexData;
    DESCRIPTOR_DATA* desc;
    AFFECT_DATA* affected;
    NOTE_DATA* pnote;
    OBJ_DATA* carrying;
    OBJ_DATA* on;
    ROOM_INDEX_DATA* in_room;
    ROOM_INDEX_DATA* was_in_room;
    AREA_DATA* zone;
    PC_DATA* pcdata;
    GEN_DATA* gen_data;
    bool valid;
    char* name;
    long id;
    int16_t version;
    char* short_descr;
    char* long_descr;
    char* description;
    char* prompt;
    char* prefix;
    int16_t group;
    int16_t clan;
    int16_t sex;
    int16_t ch_class;
    int16_t race;
    int16_t level;
    int16_t trust;
    int played;
    int lines; /* for the pager */
    time_t logon;
    int16_t timer;
    int16_t wait;
    int16_t daze;
    int16_t hit;
    int16_t max_hit;
    int16_t mana;
    int16_t max_mana;
    int16_t move;
    int16_t max_move;
    long gold;
    long silver;
    int exp;
    long act;
    long comm; /* RT added to pad the vector */
    long wiznet; /* wiz stuff */
    long imm_flags;
    long res_flags;
    long vuln_flags;
    int16_t invis_level;
    int16_t incog_level;
    long affected_by;
    int16_t position;
    int16_t practice;
    int16_t train;
    int16_t carry_weight;
    int16_t carry_number;
    int16_t saving_throw;
    int16_t alignment;
    int16_t hitroll;
    int16_t damroll;
    int16_t armor[4];
    int16_t wimpy;
    /* stats */
    int16_t perm_stat[MAX_STATS];
    int16_t mod_stat[MAX_STATS];
    /* parts stuff */
    long form;
    long parts;
    int16_t size;
    char* material;
    /* mobile stuff */
    long off_flags;
    int16_t damage[3];
    int16_t dam_type;
    int16_t start_pos;
    int16_t default_pos;
};

/*
 * Data which only PC's have.
 */
struct pc_data {
    PC_DATA* next;
    BUFFER* buffer;
    COLOUR_DATA* code; /* Data for coloUr configuration	*/
    bool valid;
    char* pwd;
    char* bamfin;
    char* bamfout;
    char* title;
    time_t last_note;
    time_t last_idea;
    time_t last_penalty;
    time_t last_news;
    time_t last_changes;
    int16_t perm_hit;
    int16_t perm_mana;
    int16_t perm_move;
    int16_t true_sex;
    int last_level;
    int16_t condition[4];
    int16_t learned[MAX_SKILL];
    bool group_known[MAX_GROUP];
    int16_t points;
    bool confirm_delete;
    char* alias[MAX_ALIAS];
    char* alias_sub[MAX_ALIAS];

    /*
     * Colour data stuff for config.
     */
    int text[3];            /* {t */
    int auction[3];         /* {a */
    int auction_text[3];    /* {A */
    int gossip[3];          /* {d */
    int gossip_text[3];     /* {9 */
    int music[3];           /* {e */
    int music_text[3];      /* {E */
    int question[3];        /* {q */
    int question_text[3];   /* {Q */
    int answer[3];          /* {f */
    int answer_text[3];     /* {F */
    int quote[3];           /* {h */
    int quote_text[3];      /* {H */
    int immtalk_text[3];    /* {i */
    int immtalk_type[3];    /* {I */
    int info[3];            /* {j */
    int say[3];             /* {6 */
    int say_text[3];        /* {7 */
    int tell[3];            /* {k */
    int tell_text[3];       /* {K */
    int reply[3];           /* {l */
    int reply_text[3];      /* {L */
    int gtell_text[3];      /* {n */
    int gtell_type[3];      /* {N */
    int wiznet[3];          /* {B */
    int room_title[3];      /* {s */
    int room_text[3];       /* {S */
    int room_exits[3];      /* {o */
    int room_things[3];     /* {O */
    int prompt[3];          /* {p */
    int fight_death[3];     /* {1 */
    int fight_yhit[3];      /* {2 */
    int fight_ohit[3];      /* {3 */
    int fight_thit[3];      /* {4 */
    int fight_skill[3];     /* {5 */
};

/* Data for generating characters -- only used during generation */
struct gen_data {
    GEN_DATA* next;
    bool valid;
    bool skill_chosen[MAX_SKILL];
    bool group_chosen[MAX_GROUP];
    int points_chosen;
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
    EXTRA_DESCR_DATA* next; /* Next in list                     */
    bool valid;
    char* keyword; /* Keyword in look/examine          */
    char* description; /* What to see                      */
};

/*
 * Prototype for an object.
 */
struct obj_index_data {
    OBJ_INDEX_DATA* next;
    EXTRA_DESCR_DATA* extra_descr;
    AFFECT_DATA* affected;
    bool new_format;
    char* name;
    char* short_descr;
    char* description;
    int16_t vnum;
    int16_t reset_num;
    char* material;
    int16_t item_type;
    int extra_flags;
    int wear_flags;
    int16_t level;
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
    CHAR_DATA* carried_by;
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
    int16_t level;
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
        int16_t vnum;
    } u1;
    int16_t exit_info;
    int16_t key;
    char* keyword;
    char* description;
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
    int16_t arg1;
    int16_t arg2;
    int16_t arg3;
    int16_t arg4;
};

/*
 * Area definition.
 */
struct area_data {
    AREA_DATA* next;
    RESET_DATA* reset_first;
    RESET_DATA* reset_last;
    char* file_name;
    char* name;
    char* credits;
    int16_t age;
    int16_t nplayer;
    int16_t low_range;
    int16_t high_range;
    int16_t min_vnum;
    int16_t max_vnum;
    bool empty;
};

/*
 * Room type.
 */
struct room_index_data {
    ROOM_INDEX_DATA* next;
    CHAR_DATA* people;
    OBJ_DATA* contents;
    EXTRA_DESCR_DATA* extra_descr;
    AREA_DATA* area;
    EXIT_DATA* exit[6];
    EXIT_DATA* old_exit[6];
    char* name;
    char* description;
    char* owner;
    int16_t vnum;
    int room_flags;
    int16_t light;
    int16_t sector_type;
    int16_t heal_rate;
    int16_t mana_rate;
    int16_t clan;
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
    int16_t skill_level[MAX_CLASS]; /* Level needed by class	*/
    int16_t rating[MAX_CLASS]; /* How hard it is to learn	*/
    SPELL_FUN* spell_fun; /* Spell pointer (for spells)	*/
    int16_t target; /* Legal targets		*/
    int16_t minimum_position; /* Position for caster / user	*/
    int16_t* pgsn; /* Pointer to associated gsn	*/
    int16_t slot; /* Slot for #OBJECT loading	*/
    int16_t min_mana; /* Minimum mana used		*/
    int16_t beats; /* Waiting time after use	*/
    char* noun_damage; /* Damage message		*/
    char* msg_off; /* Wear off message		*/
    char* msg_obj; /* Wear off message for obects	*/
};

struct group_type {
    char* name;
    int16_t rating[MAX_CLASS];
    char* spells[MAX_IN_GROUP];
};

/*
 * These are skill_lookup return values for common skills and spells.
 */
extern int16_t gsn_backstab;
extern int16_t gsn_dodge;
extern int16_t gsn_envenom;
extern int16_t gsn_hide;
extern int16_t gsn_peek;
extern int16_t gsn_pick_lock;
extern int16_t gsn_sneak;
extern int16_t gsn_steal;

extern int16_t gsn_disarm;
extern int16_t gsn_enhanced_damage;
extern int16_t gsn_kick;
extern int16_t gsn_parry;
extern int16_t gsn_rescue;
extern int16_t gsn_second_attack;
extern int16_t gsn_third_attack;

extern int16_t gsn_blindness;
extern int16_t gsn_charm_person;
extern int16_t gsn_curse;
extern int16_t gsn_invis;
extern int16_t gsn_mass_invis;
extern int16_t gsn_plague;
extern int16_t gsn_poison;
extern int16_t gsn_sleep;
extern int16_t gsn_fly;
extern int16_t gsn_sanctuary;

/* new gsns */
extern int16_t gsn_axe;
extern int16_t gsn_dagger;
extern int16_t gsn_flail;
extern int16_t gsn_mace;
extern int16_t gsn_polearm;
extern int16_t gsn_shield_block;
extern int16_t gsn_spear;
extern int16_t gsn_sword;
extern int16_t gsn_whip;

extern int16_t gsn_bash;
extern int16_t gsn_berserk;
extern int16_t gsn_dirt;
extern int16_t gsn_hand_to_hand;
extern int16_t gsn_trip;

extern int16_t gsn_fast_healing;
extern int16_t gsn_haggle;
extern int16_t gsn_lore;
extern int16_t gsn_meditation;

extern int16_t gsn_scrolls;
extern int16_t gsn_staves;
extern int16_t gsn_wands;
extern int16_t gsn_recall;

/*
 * Utility macros.
 */
#define IS_VALID(data)        ((data) != NULL && (data)->valid)
#define VALIDATE(data)        ((data)->valid = true)
#define INVALIDATE(data)      ((data)->valid = false)
#define UMIN(a, b)            ((a) < (b) ? (a) : (b))
#define UMAX(a, b)            ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)       ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)              ((c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : (c))
#define UPPER(c)              ((c) >= 'a' && (c) <= 'z' ? (c) + 'A' - 'a' : (c))
#define IS_SET(flag, bit)     ((flag) & (bit))
#define SET_BIT(var, bit)     ((var) |= (bit))
#define REMOVE_BIT(var, bit)  ((var) &= ~(bit))

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
    char name[20];
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
extern const struct race_type race_table[];
extern const struct pc_race_type pc_race_table[];
extern const struct spec_type spec_table[];
extern const struct liq_type liq_table[];
extern const struct skill_type skill_table[MAX_SKILL];
extern const struct group_type group_table[MAX_GROUP];
extern struct social_type social_table[MAX_SOCIALS];
extern char* const title_table[MAX_CLASS][MAX_LEVEL + 1][2];

/*
 * Global variables.
 */
extern HELP_DATA* help_first;
extern SHOP_DATA* shop_first;

extern CHAR_DATA* char_list;
extern DESCRIPTOR_DATA* descriptor_list;
extern OBJ_DATA* object_list;

extern char bug_buf[];
extern time_t current_time;
extern bool fLogAll;
extern FILE* fpReserve;
extern KILL_DATA kill_table[];
extern char log_buf[];
extern TIME_INFO_DATA time_info;
extern WEATHER_DATA weather_info;

/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 *   United States to foreign countries.
 * Turn on NOCRYPT to keep passwords in plain text.
 */
#if defined(NOCRYPT)
#define crypt(s1, s2) (s1)
#endif

/*
 * Data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 *
 * The NULL_FILE is held open so that we have a stream handle in reserve,
 *   so players can go ahead and telnet to all the other descriptors.
 * Then we close it whenever we need to open a file (e.g. a save file).
 */

extern char area_dir[];

#define DEFAULT_AREA_DIR "./"
#define PLAYER_DIR      "../player/"    // Player files
#define GOD_DIR         "../gods/"      // list of gods
#define TEMP_FILE       "../player/romtmp"
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
    #define NULL_FILE   "/dev/null"     // To reserve one stream
#else
    #define NULL_FILE   "nul"
#endif

/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
#define CD            CHAR_DATA
#define MID           MOB_INDEX_DATA
#define OD            OBJ_DATA
#define OID           OBJ_INDEX_DATA
#define RID           ROOM_INDEX_DATA
#define SF            SPEC_FUN
#define AD            AFFECT_DATA

/* act_comm.c */
void check_sex args((CHAR_DATA * ch));
void add_follower args((CHAR_DATA * ch, CHAR_DATA* master));
void stop_follower args((CHAR_DATA * ch));
void nuke_pets args((CHAR_DATA * ch));
void die_follower args((CHAR_DATA * ch));
bool is_same_group args((CHAR_DATA * ach, CHAR_DATA* bch));

/* act_enter.c */
RID* get_random_room args((CHAR_DATA * ch));

/* act_info.c */
void set_title args((CHAR_DATA * ch, char* title));

/* act_move.c */
void move_char args((CHAR_DATA * ch, int door, bool follow));

/* act_obj.c */
bool can_loot args((CHAR_DATA * ch, OBJ_DATA* obj));
void get_obj args((CHAR_DATA * ch, OBJ_DATA* obj, OBJ_DATA* container));

/* act_wiz.c */
void wiznet args((char* string, CHAR_DATA* ch, OBJ_DATA* obj, long flag,
                  long flag_skip, int min_level));
/* alias.c */
void substitute_alias args((DESCRIPTOR_DATA * d, char* input));

/* ban.c */
bool check_ban args((char* site, int type));

/* comm.c */
void show_string args((struct descriptor_data * d, char* input));
void close_socket args((DESCRIPTOR_DATA * dclose));
void write_to_buffer args((DESCRIPTOR_DATA * d, const char* txt, size_t length));
void send_to_char args((const char* txt, CHAR_DATA* ch));
void page_to_char args((const char* txt, CHAR_DATA* ch));
void act args((const char* format, CHAR_DATA* ch, const void* arg1,
               const void* arg2, int type));
void act_new args((const char* format, CHAR_DATA* ch, const void* arg1,
                   const void* arg2, int type, int min_pos));
/*
 * Colour stuff by Lope
 */
size_t colour(char type, CHAR_DATA* ch, char* string);
void colourconv args((char* buffer, const char* txt, CHAR_DATA* ch));
void send_to_char_bw args((const char* txt, CHAR_DATA* ch));
void page_to_char_bw args((const char* txt, CHAR_DATA* ch));

/* db.c */
char* print_flags args((int flag));
void boot_db args((void));
void area_update args((void));
CD* create_mobile args((MOB_INDEX_DATA * pMobIndex));
void clone_mobile args((CHAR_DATA * parent, CHAR_DATA* clone));
OD* create_object args((OBJ_INDEX_DATA * pObjIndex, int level));
void clone_object args((OBJ_DATA * parent, OBJ_DATA* clone));
void clear_char args((CHAR_DATA * ch));
char* get_extra_descr args((const char* name, EXTRA_DESCR_DATA* ed));
MID* get_mob_index args((int vnum));
OID* get_obj_index args((int vnum));
RID* get_room_index args((int vnum));
char fread_letter args((FILE * fp));
int fread_number args((FILE * fp));
long fread_flag args((FILE * fp));
char* fread_string args((FILE * fp));
char* fread_string_eol args((FILE * fp));
void fread_to_eol args((FILE * fp));
char* fread_word args((FILE * fp));
long flag_convert args((char letter));
void* alloc_mem(size_t sMem);
void* alloc_perm(size_t sMem);
void free_mem(void* pMem, size_t sMem);
char* str_dup args((const char* str));
void free_string args((char* pstr));
int number_fuzzy args((int number));
int number_range args((int from, int to));
int number_percent args((void));
int number_door args((void));
int number_bits args((int width));
long number_mm args((void));
int dice args((int number, int size));
int interpolate args((int level, int value_00, int value_32));
void smash_tilde args((char* str));
bool str_cmp args((const char* astr, const char* bstr));
bool str_prefix args((const char* astr, const char* bstr));
bool str_infix args((const char* astr, const char* bstr));
bool str_suffix args((const char* astr, const char* bstr));
char* capitalize args((const char* str));
void append_file args((CHAR_DATA * ch, char* file, char* str));
void bug(const char* fmt, ...);
void log_string args((const char* str));
void tail_chain args((void));

/* effect.c */
void acid_effect args((void* vo, int level, int dam, int target));
void cold_effect args((void* vo, int level, int dam, int target));
void fire_effect args((void* vo, int level, int dam, int target));
void poison_effect args((void* vo, int level, int dam, int target));
void shock_effect args((void* vo, int level, int dam, int target));

/* fight.c */
bool is_safe args((CHAR_DATA * ch, CHAR_DATA* victim));
bool is_safe_spell args((CHAR_DATA * ch, CHAR_DATA* victim, bool area));
void violence_update args((void));
void multi_hit args((CHAR_DATA * ch, CHAR_DATA* victim, int dt));
bool damage(CHAR_DATA * ch, CHAR_DATA* victim, int dam, int dt, int ch_class, 
    bool show);
bool damage_old(CHAR_DATA * ch, CHAR_DATA* victim, int dam, int dt, 
    int ch_class, bool show);
void update_pos args((CHAR_DATA * victim));
void stop_fighting args((CHAR_DATA * ch, bool fBoth));
void check_killer args((CHAR_DATA * ch, CHAR_DATA* victim));

/* handler.c */
AD* affect_find args((AFFECT_DATA * paf, int sn));
void affect_check args((CHAR_DATA * ch, int where, int vector));
int count_users args((OBJ_DATA * obj));
void deduct_cost args((CHAR_DATA * ch, int cost));
void affect_enchant args((OBJ_DATA * obj));
int check_immune args((CHAR_DATA * ch, int dam_type));
int liq_lookup args((const char* name));
int material_lookup args((const char* name));
int weapon_lookup args((const char* name));
int weapon_type args((const char* name));
char* weapon_name args((int weapon_Type));
int item_lookup args((const char* name));
char* item_name args((int item_type));
int attack_lookup args((const char* name));
int race_lookup args((const char* name));
long wiznet_lookup args((const char* name));
int class_lookup args((const char* name));
bool is_clan args((CHAR_DATA * ch));
bool is_same_clan args((CHAR_DATA * ch, CHAR_DATA* victim));
bool is_old_mob args((CHAR_DATA * ch));
int get_skill args((CHAR_DATA * ch, int sn));
int get_weapon_sn args((CHAR_DATA * ch));
int get_weapon_skill args((CHAR_DATA * ch, int sn));
int get_age args((CHAR_DATA * ch));
void reset_char args((CHAR_DATA * ch));
int get_trust args((CHAR_DATA * ch));
int get_curr_stat args((CHAR_DATA * ch, int stat));
int get_max_train args((CHAR_DATA * ch, int stat));
int can_carry_n args((CHAR_DATA * ch));
int can_carry_w args((CHAR_DATA * ch));
bool is_name args((char* str, char* namelist));
bool is_exact_name args((char* str, char* namelist));
void affect_to_char args((CHAR_DATA * ch, AFFECT_DATA* paf));
void affect_to_obj args((OBJ_DATA * obj, AFFECT_DATA* paf));
void affect_remove args((CHAR_DATA * ch, AFFECT_DATA* paf));
void affect_remove_obj args((OBJ_DATA * obj, AFFECT_DATA* paf));
void affect_strip args((CHAR_DATA * ch, int sn));
bool is_affected args((CHAR_DATA * ch, int sn));
void affect_join args((CHAR_DATA * ch, AFFECT_DATA* paf));
void char_from_room args((CHAR_DATA * ch));
void char_to_room args((CHAR_DATA * ch, ROOM_INDEX_DATA* pRoomIndex));
void obj_to_char args((OBJ_DATA * obj, CHAR_DATA* ch));
void obj_from_char args((OBJ_DATA * obj));
int apply_ac args((OBJ_DATA * obj, int iWear, int type));
OD* get_eq_char args((CHAR_DATA * ch, int iWear));
void equip_char args((CHAR_DATA * ch, OBJ_DATA* obj, int iWear));
void unequip_char args((CHAR_DATA * ch, OBJ_DATA* obj));
int count_obj_list args((OBJ_INDEX_DATA * obj, OBJ_DATA* list));
void obj_from_room args((OBJ_DATA * obj));
void obj_to_room args((OBJ_DATA * obj, ROOM_INDEX_DATA* pRoomIndex));
void obj_to_obj args((OBJ_DATA * obj, OBJ_DATA* obj_to));
void obj_from_obj args((OBJ_DATA * obj));
void extract_obj args((OBJ_DATA * obj));
void extract_char args((CHAR_DATA * ch, bool fPull));
CD* get_char_room args((CHAR_DATA * ch, char* argument));
CD* get_char_world args((CHAR_DATA * ch, char* argument));
OD* get_obj_type args((OBJ_INDEX_DATA * pObjIndexData));
OD* get_obj_list args((CHAR_DATA * ch, char* argument, OBJ_DATA* list));
OD* get_obj_carry args((CHAR_DATA * ch, char* argument, CHAR_DATA* viewer));
OD* get_obj_wear args((CHAR_DATA * ch, char* argument));
OD* get_obj_here args((CHAR_DATA * ch, char* argument));
OD* get_obj_world args((CHAR_DATA * ch, char* argument));
OD* create_money args((int gold, int silver));
int get_obj_number args((OBJ_DATA * obj));
int get_obj_weight args((OBJ_DATA * obj));
int get_true_weight args((OBJ_DATA * obj));
bool room_is_dark args((ROOM_INDEX_DATA * pRoomIndex));
bool is_room_owner args((CHAR_DATA * ch, ROOM_INDEX_DATA* room));
bool room_is_private args((ROOM_INDEX_DATA * pRoomIndex));
bool can_see args((CHAR_DATA * ch, CHAR_DATA* victim));
bool can_see_obj args((CHAR_DATA * ch, OBJ_DATA* obj));
bool can_see_room args((CHAR_DATA * ch, ROOM_INDEX_DATA* pRoomIndex));
bool can_drop_obj args((CHAR_DATA * ch, OBJ_DATA* obj));
char* affect_loc_name args((int location));
char* affect_bit_name args((int vector));
char* extra_bit_name args((int extra_flags));
char* wear_bit_name args((int wear_flags));
char* act_bit_name args((int act_flags));
char* off_bit_name args((int off_flags));
char* imm_bit_name args((int imm_flags));
char* form_bit_name args((int form_flags));
char* part_bit_name args((int part_flags));
char* weapon_bit_name args((int weapon_flags));
char* comm_bit_name args((int comm_flags));
char* cont_bit_name args((int cont_flags));
/*
 * Colour Config
 */
void default_colour args((CHAR_DATA * ch));
void all_colour args((CHAR_DATA * ch, char* argument));

/* interp.c */
void interpret args((CHAR_DATA * ch, char* argument));
bool is_number args((char* arg));
int number_argument args((char* argument, char* arg));
int mult_argument args((char* argument, char* arg));
char* one_argument args((char* argument, char* arg_first));

/* magic.c */
int find_spell args((CHAR_DATA * ch, const char* name));
int mana_cost(CHAR_DATA* ch, int min_mana, int level);
int skill_lookup args((const char* name));
int slot_lookup args((int slot));
bool saves_spell args((int level, CHAR_DATA* victim, int dam_type));
void obj_cast_spell args((int sn, int level, CHAR_DATA* ch, CHAR_DATA* victim,
                          OBJ_DATA* obj));
/* save.c */
void save_char_obj args((CHAR_DATA * ch));
bool load_char_obj args((DESCRIPTOR_DATA * d, char* name));

/* skills.c */
bool parse_gen_groups args((CHAR_DATA * ch, char* argument));
void list_group_costs args((CHAR_DATA * ch));
void list_group_known args((CHAR_DATA * ch));
int exp_per_level args((CHAR_DATA * ch, int points));
void check_improve args((CHAR_DATA * ch, int sn, bool success, int multiplier));
int group_lookup args((const char* name));
void gn_add args((CHAR_DATA * ch, int gn));
void gn_remove args((CHAR_DATA * ch, int gn));
void group_add args((CHAR_DATA * ch, const char* name, bool deduct));
void group_remove args((CHAR_DATA * ch, const char* name));

/* special.c */
SF* spec_lookup args((const char* name));
char* spec_name args((SPEC_FUN * function));

/* teleport.c */
RID* room_by_name args((char* target, int level, bool error));

/* update.c */
void advance_level args((CHAR_DATA * ch, bool hide));
void gain_exp args((CHAR_DATA * ch, int gain));
void gain_condition args((CHAR_DATA * ch, int iCond, int value));
void update_handler args((void));

#undef CD
#undef MID
#undef OD
#undef OID
#undef RID
#undef SF
#undef AD

#endif // !ROM__MERC_H
