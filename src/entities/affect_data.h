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

/* where definitions */
typedef enum {
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
    LEVEL level;
    int16_t duration;
    int16_t location;
    int16_t modifier;
    bool valid;
} AffectData;

void affect_check(CharData* ch, int where, int vector);
void affect_enchant(ObjectData* obj);
AffectData* affect_find(AffectData* paf, SKNUM sn);
void affect_join(CharData* ch, AffectData* paf);
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

#endif // !MUD98__ENTITIES__AFFECT_DATA_H
