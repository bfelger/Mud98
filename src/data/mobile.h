////////////////////////////////////////////////////////////////////////////////
// mobile.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__MOBILE_H
#define MUD98__DATA__MOBILE_H

#include "merc.h"

typedef enum armor_type_t {
    AC_PIERCE       = 0,
    AC_BASH         = 1,
    AC_SLASH        = 2,
    AC_EXOTIC       = 3,
} ArmorType;

#define AC_COUNT 4

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
    //POS_UNKNOWN             = -1,
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
    const Position pos;
    const char* name;
    const char* short_name;
} PositionInfo;

bool position_read(void* temp, char* arg);
const char* position_str(void* temp);

extern const PositionInfo position_table[POS_MAX];

////////////////////////////////////////////////////////////////////////////////
// Sex
////////////////////////////////////////////////////////////////////////////////

typedef enum sex_t {
    SEX_NEUTRAL             = 0,
    SEX_MALE                = 1,
    SEX_FEMALE              = 2,
    SEX_EITHER              = 3,
} Sex;

#define SEX_MIN SEX_NEUTRAL
#define SEX_MAX SEX_EITHER
#define SEX_COUNT 4

typedef struct sex_info_t {
    const Sex sex;
    const char* name;
    const char* subj;
    const char* subj_cap;
    const char* obj;
    const char* poss;
} SexInfo;

extern const SexInfo sex_table[SEX_COUNT];

////////////////////////////////////////////////////////////////////////////////
// Size
////////////////////////////////////////////////////////////////////////////////

typedef enum mob_size_t {
    SIZE_TINY               = 0,
    SIZE_SMALL              = 1,
    SIZE_MEDIUM             = 2,
    SIZE_LARGE              = 3,
    SIZE_HUGE               = 4,
    SIZE_GIANT              = 5,
} MobSize;

#define MOB_SIZE_MIN SIZE_TINY
#define MOB_SIZE_MAX SIZE_GIANT
#define MOB_SIZE_COUNT 6

typedef struct mob_size_info_t {
    const MobSize size;
    const char* name;
} MobSizeInfo;

bool size_read(void* temp, char* arg);
const char* size_str(void* temp);

extern const MobSizeInfo mob_size_table[MOB_SIZE_COUNT];

////////////////////////////////////////////////////////////////////////////////

#endif // !MUD98__DATA__MOBILE_H