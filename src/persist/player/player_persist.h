////////////////////////////////////////////////////////////////////////////////
// persist/player/player_persist.h
// Shared helpers for player persistence formats.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__PLAYER__PLAYER_PERSIST_H
#define MUD98__PERSIST__PLAYER__PLAYER_PERSIST_H

#include <persist/persist_io.h>
#include <persist/persist_result.h>

#include <entities/descriptor.h>
#include <entities/mobile.h>

#include <stdio.h>

typedef enum player_persist_format_t {
    PLAYER_PERSIST_ROM_OLC = 0,
    PLAYER_PERSIST_JSON = 1,
} PlayerPersistFormat;

typedef struct player_persist_load_params_t {
    Mobile* ch;
    Descriptor* descriptor;
    const char* player_name;
    FILE* fp;
    const PersistReader* reader;
    const char* path;
} PlayerPersistLoadParams;

typedef struct player_persist_save_params_t {
    Mobile* ch;
    FILE* fp;
    const PersistWriter* writer;
    const char* path;
} PlayerPersistSaveParams;

PlayerPersistFormat player_persist_format_from_string(const char* name);
const char* player_persist_format_name(PlayerPersistFormat fmt);
const char* player_persist_format_extension(PlayerPersistFormat fmt);
PlayerPersistFormat player_persist_alternate_format(PlayerPersistFormat fmt);

PersistResult player_persist_rom_olc_load(const PlayerPersistLoadParams* params);
PersistResult player_persist_rom_olc_save(const PlayerPersistSaveParams* params);
PersistResult player_persist_json_load(const PlayerPersistLoadParams* params);
PersistResult player_persist_json_save(const PlayerPersistSaveParams* params);

#endif // !MUD98__PERSIST__PLAYER__PLAYER_PERSIST_H
