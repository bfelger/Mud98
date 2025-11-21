////////////////////////////////////////////////////////////////////////////////
// entities/faction.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__ENTITIES__FACTION_H
#define MUD98__ENTITIES__FACTION_H

#include <merc.h>

#include "entity.h"

#include <lox/array.h>
#include <lox/table.h>

typedef struct player_data_t PlayerData;
typedef struct area_data_t AreaData;

typedef struct faction_t {
    Entity header;
    struct faction_t* next;
    AreaData* area;
    ValueArray allies;
    ValueArray enemies;
    int default_standing;
} Faction;

typedef struct faction_reputation_t {
    VNUM vnum;
    int value;
} FactionReputation;

typedef enum faction_standing_t {
    FACTION_STANDING_HATED = 0,
    FACTION_STANDING_HOSTILE,
    FACTION_STANDING_UNFRIENDLY,
    FACTION_STANDING_NEUTRAL,
    FACTION_STANDING_FRIENDLY,
    FACTION_STANDING_HONORED,
    FACTION_STANDING_REVERED,
    FACTION_STANDING_EXALTED,
} FactionStanding;

#define FACTION_STANDING_COUNT  (FACTION_STANDING_EXALTED + 1)

extern Table faction_table;

Faction* faction_create(VNUM vnum);
Faction* get_faction(VNUM vnum);
Faction* get_faction_by_name(const char* name);
Faction* get_mob_faction(Mobile* mob);
VNUM get_mob_faction_vnum(Mobile* mob);

void load_factions(FILE* fp);

FactionStanding faction_standing_from_value(int value);
const char* faction_standing_name(FactionStanding standing);
const char* faction_standing_name_from_value(int value);
int faction_clamp_value(int value);

bool faction_is_friendly_value(int value);
bool faction_is_hostile_value(int value);

int faction_get_value(Mobile* ch, Faction* faction, bool touch);
void faction_touch(Mobile* ch, Faction* faction);
void faction_adjust(Mobile* ch, Faction* faction, int delta);
void faction_set(PlayerData* pcdata, VNUM vnum, int value);

bool faction_block_player_attack(Mobile* ch, Mobile* victim);
bool faction_block_shopkeeper(Mobile* keeper, Mobile* ch);
void faction_handle_kill(Mobile* killer, Mobile* victim);

void faction_show_reputations(Mobile* ch);
bool faction_add_ally(Faction* faction, VNUM ally_vnum);
bool faction_remove_ally(Faction* faction, VNUM ally_vnum);
bool faction_add_enemy(Faction* faction, VNUM enemy_vnum);
bool faction_remove_enemy(Faction* faction, VNUM enemy_vnum);
void faction_delete(Faction* faction);

#endif // !MUD98__ENTITIES__FACTION_H
