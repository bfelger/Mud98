////////////////////////////////////////////////////////////////////////////////
// affect_data.h
// Utilities to handle special conditions and status affects
////////////////////////////////////////////////////////////////////////////////

typedef struct affect_data_t AffectData;

#pragma once
#ifndef MUD98__ENTITIES__AFFECT_DATA_H
#define MUD98__ENTITIES__AFFECT_DATA_H

#include "merc.h"

#include "char_data.h"
#include "object_data.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum affect_flags_t {
    AFF_BLIND               = BIT(0),
    AFF_INVISIBLE           = BIT(1),
    AFF_DETECT_EVIL         = BIT(2),
    AFF_DETECT_INVIS        = BIT(3),
    AFF_DETECT_MAGIC        = BIT(4),
    AFF_DETECT_HIDDEN       = BIT(5),
    AFF_DETECT_GOOD         = BIT(6),
    AFF_SANCTUARY           = BIT(7),
    AFF_FAERIE_FIRE         = BIT(8),
    AFF_INFRARED            = BIT(9),
    AFF_CURSE               = BIT(10),
    AFF_UNUSED_FLAG         = BIT(11),
    AFF_POISON              = BIT(12),
    AFF_PROTECT_EVIL        = BIT(13),
    AFF_PROTECT_GOOD        = BIT(14),
    AFF_SNEAK               = BIT(15),
    AFF_HIDE                = BIT(16),
    AFF_SLEEP               = BIT(17),
    AFF_CHARM               = BIT(18),
    AFF_FLYING              = BIT(19),
    AFF_PASS_DOOR           = BIT(20),
    AFF_HASTE               = BIT(21),
    AFF_CALM                = BIT(22),
    AFF_PLAGUE              = BIT(23),
    AFF_WEAKEN              = BIT(24),
    AFF_DARK_VISION         = BIT(25),
    AFF_BERSERK             = BIT(26),
    AFF_SWIM                = BIT(27),
    AFF_REGENERATION        = BIT(28),
    AFF_SLOW                = BIT(29),
} AffectFlags;

typedef enum location_t {
    APPLY_NONE          = 0,
    APPLY_STR           = 1,
    APPLY_DEX           = 2,
    APPLY_INT           = 3,
    APPLY_WIS           = 4,
    APPLY_CON           = 5,
    APPLY_SEX           = 6,
    APPLY_CLASS         = 7,
    APPLY_LEVEL         = 8,
    APPLY_AGE           = 9,
    APPLY_HEIGHT        = 10,
    APPLY_WEIGHT        = 11,
    APPLY_MANA          = 12,
    APPLY_HIT           = 13,
    APPLY_MOVE          = 14,
    APPLY_GOLD          = 15,
    APPLY_EXP           = 16,
    APPLY_AC            = 17,
    APPLY_HITROLL       = 18,
    APPLY_DAMROLL       = 19,
    APPLY_SAVES         = 20,
    APPLY_SAVING_PARA   = 21,
    APPLY_SAVING_ROD    = 22,
    APPLY_SAVING_PETRI  = 23,
    APPLY_SAVING_BREATH = 24,
    APPLY_SAVING_SPELL  = 25,
    APPLY_SPELL_AFFECT  = 26,
} AffectLocation;

typedef enum where_t {
    TO_AFFECTS  = 0,
    TO_OBJECT   = 1,
    TO_IMMUNE   = 2,
    TO_RESIST   = 3,
    TO_VULN     = 4,
    TO_WEAPON   = 5,
} Where;

typedef struct affect_data_t {
    AffectData* next;
    int bitvector;
    SKNUM type;
    Where where;
    AffectLocation location;
    LEVEL level;
    int16_t duration;
    int16_t modifier;
    bool valid;
} AffectData;

char* affect_bit_name(int vector);
void affect_check(CharData* ch, Where where, int vector);
void affect_enchant(ObjectData* obj);
AffectData* affect_find(AffectData* paf, SKNUM sn);
void affect_join(CharData* ch, AffectData* paf);
char* affect_loc_name(AffectLocation location);
void affect_modify(CharData* ch, AffectData* paf, bool fAdd);
void affect_remove(CharData* ch, AffectData* paf);
void affect_remove_obj(ObjectData* obj, AffectData* paf);
void affect_strip(CharData* ch, SKNUM sn);
void affect_to_char(CharData* ch, AffectData* paf);
void affect_to_obj(ObjectData* obj, AffectData* paf);
void free_affect(AffectData* af);
bool is_affected(CharData* ch, SKNUM sn);
AffectData* new_affect();

extern AffectData* affect_free;
extern int top_affect;

#endif // !MUD98__ENTITIES__AFFECT_DATA_H
