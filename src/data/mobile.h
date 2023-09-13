////////////////////////////////////////////////////////////////////////////////
// mobile.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__MOBILE_H
#define MUD98__DATA__MOBILE_H

#include "merc.h"

typedef enum mob_act_flags_t {
    ACT_IS_NPC              = BIT(0),   // Auto set for mobs
    ACT_SENTINEL            = BIT(1),   // Stays in one room
    ACT_SCAVENGER           = BIT(2),   // Picks up objects
    ACT_AGGRESSIVE          = BIT(5),   // Attacks PC's
    ACT_STAY_AREA           = BIT(6),   // Won't leave area
    ACT_WIMPY               = BIT(7),   //
    ACT_PET                 = BIT(8),   // Auto set for pets
    ACT_TRAIN               = BIT(9),   // Can train PC's
    ACT_PRACTICE            = BIT(10),  // Can practice PC's
    // Unused                 BIT(11)
    // Unused                 BIT(12)
    // Unused                 BIT(13)
    ACT_UNDEAD              = BIT(14),
    // Unused                 BIT(15)
    ACT_CLERIC              = BIT(16),
    ACT_MAGE                = BIT(17),
    ACT_THIEF               = BIT(18),
    ACT_WARRIOR             = BIT(19),
    ACT_NOALIGN             = BIT(20),
    ACT_NOPURGE             = BIT(21),
    ACT_OUTDOORS            = BIT(22),
    // Unused                 BIT(23)
    ACT_INDOORS             = BIT(24),
    // Unused                 BIT(25)
    ACT_IS_HEALER           = BIT(26),
    ACT_GAIN                = BIT(27),
    ACT_UPDATE_ALWAYS       = BIT(28),
    ACT_IS_CHANGER          = BIT(29),
} MobActFlags;

typedef enum attack_flags_t {
    ATK_AREA_ATTACK         = BIT(0),
    ATK_BACKSTAB            = BIT(1),
    ATK_BASH                = BIT(2),
    ATK_BERSERK             = BIT(3),
    ATK_DISARM              = BIT(4),
    ATK_DODGE               = BIT(5),
    ATK_FADE                = BIT(6),
    ATK_FAST                = BIT(7),
    ATK_KICK                = BIT(8),
    ATK_KICK_DIRT           = BIT(9),
    ATK_PARRY               = BIT(10),
    ATK_RESCUE              = BIT(11),
    ATK_TAIL                = BIT(12),
    ATK_TRIP                = BIT(13),
    ATK_CRUSH               = BIT(14),
    ASSIST_ALL              = BIT(15),
    ASSIST_ALIGN            = BIT(16),
    ASSIST_RACE             = BIT(17),
    ASSIST_PLAYERS          = BIT(18),
    ASSIST_GUARD            = BIT(19),
    ASSIST_VNUM             = BIT(20),
} AttackFlags;

typedef enum immune_flags_t {
    IMM_SUMMON              = BIT(0),
    IMM_CHARM               = BIT(1),
    IMM_MAGIC               = BIT(2),
    IMM_WEAPON              = BIT(3),
    IMM_BASH                = BIT(4),
    IMM_PIERCE              = BIT(5),
    IMM_SLASH               = BIT(6),
    IMM_FIRE                = BIT(7),
    IMM_COLD                = BIT(8),
    IMM_LIGHTNING           = BIT(9),
    IMM_ACID                = BIT(10),
    IMM_POISON              = BIT(11),
    IMM_NEGATIVE            = BIT(12),
    IMM_HOLY                = BIT(13),
    IMM_ENERGY              = BIT(14),
    IMM_MENTAL              = BIT(15),
    IMM_DISEASE             = BIT(16),
    IMM_DROWNING            = BIT(17),
    IMM_LIGHT               = BIT(18),
    IMM_SOUND               = BIT(19),
    // Unused                 BIT(20)
    // Unused                 BIT(21)
    // Unused                 BIT(22)
    IMM_WOOD                = BIT(23),
    IMM_SILVER              = BIT(24),
    IMM_IRON                = BIT(25),
} ImmuneFlags;

typedef enum resist_flags_t {
    RES_SUMMON              = BIT(0),
    RES_CHARM               = BIT(1),
    RES_MAGIC               = BIT(2),
    RES_WEAPON              = BIT(3),
    RES_BASH                = BIT(4),
    RES_PIERCE              = BIT(5),
    RES_SLASH               = BIT(6),
    RES_FIRE                = BIT(7),
    RES_COLD                = BIT(8),
    RES_LIGHTNING           = BIT(9),
    RES_ACID                = BIT(10),
    RES_POISON              = BIT(11),
    RES_NEGATIVE            = BIT(12),
    RES_HOLY                = BIT(13),
    RES_ENERGY              = BIT(14),
    RES_MENTAL              = BIT(15),
    RES_DISEASE             = BIT(16),
    RES_DROWNING            = BIT(17),
    RES_LIGHT               = BIT(18),
    RES_SOUND               = BIT(19),
    // Unused                 BIT(20)
    // Unused                 BIT(21)
    // Unused                 BIT(22)
    RES_WOOD                = BIT(23),
    RES_SILVER              = BIT(24),
    RES_IRON                = BIT(25),
} ResistFlags;

typedef enum vuln_flags_t {
    VULN_SUMMON             = BIT(0),
    VULN_CHARM              = BIT(1),
    VULN_MAGIC              = BIT(2),
    VULN_WEAPON             = BIT(3),
    VULN_BASH               = BIT(4),
    VULN_PIERCE             = BIT(5),
    VULN_SLASH              = BIT(6),
    VULN_FIRE               = BIT(7),
    VULN_COLD               = BIT(8),
    VULN_LIGHTNING          = BIT(9),
    VULN_ACID               = BIT(10),
    VULN_POISON             = BIT(11),
    VULN_NEGATIVE           = BIT(12),
    VULN_HOLY               = BIT(13),
    VULN_ENERGY             = BIT(14),
    VULN_MENTAL             = BIT(15),
    VULN_DISEASE            = BIT(16),
    VULN_DROWNING           = BIT(17),
    VULN_LIGHT              = BIT(18),
    VULN_SOUND              = BIT(19),
    // Unused                 BIT(20)
    // Unused                 BIT(21)
    // Unused                 BIT(22)
    VULN_WOOD               = BIT(23),
    VULN_SILVER             = BIT(24),
    VULN_IRON               = BIT(25),
} VulnFlags;

typedef enum resist_type_t {
    IS_NORMAL               = 0,
    IS_IMMUNE               = 1,
    IS_RESISTANT            = 2,
    IS_VULNERABLE           = 3,
} ResistType;

// body form
typedef enum form_flags_t {
    FORM_EDIBLE             = BIT(0),
    FORM_POISON             = BIT(1),
    FORM_MAGICAL            = BIT(2),
    FORM_INSTANT_DECAY      = BIT(3),
    FORM_OTHER              = BIT(4), // defined by material bit
// actual form
    // Unused                 BIT(5)
    FORM_ANIMAL             = BIT(6),
    FORM_SENTIENT           = BIT(7),
    FORM_UNDEAD             = BIT(8),
    FORM_CONSTRUCT          = BIT(9),
    FORM_MIST               = BIT(10),
    FORM_INTANGIBLE         = BIT(11),
    FORM_BIPED              = BIT(12),
    FORM_CENTAUR            = BIT(13),
    FORM_INSECT             = BIT(14),
    FORM_SPIDER             = BIT(15),
    FORM_CRUSTACEAN         = BIT(16),
    FORM_WORM               = BIT(17),
    FORM_BLOB               = BIT(18),
    // Unused                 BIT(19)
    // Unused                 BIT(20)
    FORM_MAMMAL             = BIT(21),
    FORM_BIRD               = BIT(22),
    FORM_REPTILE            = BIT(23),
    FORM_SNAKE              = BIT(24),
    FORM_DRAGON             = BIT(25),
    FORM_AMPHIBIAN          = BIT(26),
    FORM_FISH               = BIT(27),
    FORM_COLD_BLOOD         = BIT(28),
} FormFlags;

// body parts
typedef enum part_flags_t {
    PART_HEAD               = BIT(0),
    PART_ARMS               = BIT(1),
    PART_LEGS               = BIT(2),
    PART_HEART              = BIT(3),
    PART_BRAINS             = BIT(4),
    PART_GUTS               = BIT(5),
    PART_HANDS              = BIT(6),
    PART_FEET               = BIT(7),
    PART_FINGERS            = BIT(8),
    PART_EAR                = BIT(9),
    PART_EYE                = BIT(10),
    PART_LONG_TONGUE        = BIT(11),
    PART_EYESTALKS          = BIT(12),
    PART_TENTACLES          = BIT(13),
    PART_FINS               = BIT(14),
    PART_WINGS              = BIT(15),
    PART_TAIL               = BIT(16),
// for combat
    PART_CLAWS              = BIT(20),
    PART_FANGS              = BIT(21),
    PART_HORNS              = BIT(22),
    PART_SCALES             = BIT(23),
    PART_TUSKS              = BIT(24),
} PartFlags;

////////////////////////////////////////////////////////////////////////////////
// Position
////////////////////////////////////////////////////////////////////////////////

typedef enum position_t {
    POS_UNKNOWN             = -1,
    POS_DEAD                = 0,
    POS_MORTAL              = 1,
    POS_INCAP               = 2,
    POS_STUNNED             = 3,
    POS_SLEEPING            = 4,
    POS_RESTING             = 5,
    POS_SITTING             = 6,
    POS_FIGHTING            = 7,
    POS_STANDING            = 8,
} Position;

#define POS_MAX 9

typedef struct position_info_t {
    Position pos;
    char* name;
    char* short_name;
} PositionInfo;

extern const PositionInfo position_table[POS_MAX];

////////////////////////////////////////////////////////////////////////////////

#endif // !MUD98__DATA__MOBILE_H