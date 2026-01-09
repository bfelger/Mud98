////////////////////////////////////////////////////////////////////////////////
// craft/gather.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__CRAFT__GATHER_H
#define MUD98__CRAFT__GATHER_H

#include <merc.h>

#include <data/direction.h>

typedef struct area_t Area;
typedef struct area_data_t AreaData;
typedef struct mobile_t Mobile;

////////////////////////////////////////////////////////////////////////////////
// Gather types
////////////////////////////////////////////////////////////////////////////////

typedef enum gather_type_t {
    GATHER_NONE         = 0,
    GATHER_ORE          = 1,    // Gathered by mining
    GATHER_HERB         = 2,    // Gathered by herbalism
} GatherType;

#define GATHER_TYPE_COUNT 3

typedef struct gather_info_t {
    const GatherType type;
    const char* name;
} GatherInfo;

const char* gather_type_name(GatherType type);
GatherType gather_lookup(const char* name);

extern const GatherInfo gather_type_table[GATHER_TYPE_COUNT];

////////////////////////////////////////////////////////////////////////////////
// Gather spawn data
////////////////////////////////////////////////////////////////////////////////

typedef struct gather_spawn_t {
    Sector spawn_sector;        // Sector type where this spawn can appear
    VNUM vnum;                  // VNUM of ITEM_GATHER object to spawn
    int quantity;               // Quantity to spawn across area
    int respawn_timer;          // In minutes
} GatherSpawn;

typedef struct gather_spawn_array_t {
    GatherSpawn* spawns;
    size_t count;
    size_t capacity;
} GatherSpawnArray;

void add_gather_spawn(GatherSpawnArray* array, Sector sector, VNUM vnum, 
    int quantity, int respawn_timer);
void free_gather_spawn_array(GatherSpawnArray* array);
void copy_spawn_array(GatherSpawnArray* dest, const GatherSpawnArray* src);
void reset_area_gather_spawns(Area* area);
void force_reset_area_gather_spawns(Area* area);
void reset_gather_spawn(Area* area, GatherSpawn* spawn);

void olc_show_gather_spawns(Mobile* ch, AreaData* area_data);

#endif // !MUD98__CRAFT__GATHER_H