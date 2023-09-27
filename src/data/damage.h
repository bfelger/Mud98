////////////////////////////////////////////////////////////////////////////////
// damage.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__DAMAGE_H
#define MUD98__DATA__DAMAGE_H

#include "merc.h"

////////////////////////////////////////////////////////////////////////////////
// Resistances
////////////////////////////////////////////////////////////////////////////////

typedef enum resist_type_t {
    IS_NORMAL               = 0,
    IS_IMMUNE               = 1,
    IS_RESISTANT            = 2,
    IS_VULNERABLE           = 3,
} ResistType;

typedef enum immune_flags_t {
    IMM_NONE                = 0,
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
    RES_NONE                = 0,
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
    VULN_NONE               = 0,
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

////////////////////////////////////////////////////////////////////////////////
// Damage
////////////////////////////////////////////////////////////////////////////////

typedef enum damage_type_t {
    DAM_NONE                = 0,
    DAM_BASH                = 1,
    DAM_PIERCE              = 2,
    DAM_SLASH               = 3,
    DAM_FIRE                = 4,
    DAM_COLD                = 5,
    DAM_LIGHTNING           = 6,
    DAM_ACID                = 7,
    DAM_POISON              = 8,
    DAM_NEGATIVE            = 9,
    DAM_HOLY                = 10,
    DAM_ENERGY              = 11,
    DAM_MENTAL              = 12,
    DAM_DISEASE             = 13,
    DAM_DROWNING            = 14,
    DAM_LIGHT               = 15,
    DAM_OTHER               = 16,
    DAM_HARM                = 17,
    DAM_CHARM               = 18,
    DAM_SOUND               = 19,
} DamageType;

#define DAM_TYPE_COUNT 20

#define TYPE_UNDEFINED     -1
#define TYPE_HIT           1000

typedef struct damage_info_t {
    const DamageType type;
    const char* name;
    const ImmuneFlags imm;
    const ResistFlags res;
    const VulnFlags vuln;
} DamageInfo;

extern const DamageInfo damage_table[DAM_TYPE_COUNT];

////////////////////////////////////////////////////////////////////////////////
// Attacks
////////////////////////////////////////////////////////////////////////////////

#define ATTACK_COUNT 41

typedef struct attack_info_t {
    char* name;
    char* noun;
    DamageType damage;
} AttackInfo;

extern const AttackInfo attack_table[ATTACK_COUNT];

////////////////////////////////////////////////////////////////////////////////
// Dice
////////////////////////////////////////////////////////////////////////////////

#define DICE_NUMBER             0
#define DICE_TYPE               1
#define DICE_BONUS              2

//typedef struct dice_t {
//    int16_t num_dice;
//    int16_t die_type;
//    int16_t bonus;
//} Dice;

////////////////////////////////////////////////////////////////////////////////

#endif // !MUD98__DATA__DAMAGE_H
