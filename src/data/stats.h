////////////////////////////////////////////////////////////////////////////////
// stats.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__STATS_H
#define MUD98__DATA__STATS_H

#include <stdint.h>

typedef enum stat_t {
    STAT_STR    = 0,
    STAT_INT    = 1,
    STAT_WIS    = 2,
    STAT_DEX    = 3,
    STAT_CON    = 4,
} Stat;

#define STAT_COUNT 5

#define STAT_MAX 25
#define STAT_MAX_LVL 26

typedef struct str_mod_t {
    const int16_t tohit;
    const int16_t todam;
    const int16_t carry;
    const int16_t wield;
} StrMod;

extern const StrMod str_mod[STAT_MAX_LVL];

typedef struct int_mod_t {
    const int16_t learn;
} IntMod;

extern const IntMod int_mod[STAT_MAX_LVL];

typedef struct wis_mod_t {
    const int16_t practice;
} WisMod;

extern const WisMod wis_mod[STAT_MAX_LVL];

typedef struct dex_mod_t {
    const int16_t defensive;
} DexMod;

extern const DexMod dex_mod[STAT_MAX_LVL];

typedef struct con_mod_t {
    const int16_t hitp;
    const int16_t shock;
} ConMod;

extern const ConMod con_mod[STAT_MAX_LVL];

#endif // !MUD98__DATA__STATS_H
