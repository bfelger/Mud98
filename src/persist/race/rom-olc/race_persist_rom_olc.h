////////////////////////////////////////////////////////////////////////////////
// persist/rom-olc/race_persist_rom_olc.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__ROM_OLC__RACE_PERSIST_ROM_OLC_H
#define MUD98__PERSIST__ROM_OLC__RACE_PERSIST_ROM_OLC_H

#include <persist/persist_result.h>
#include <persist/persist_io.h>
#include <persist/race/race_persist.h>

PersistResult race_persist_rom_load(const PersistReader* reader, const char* filename);
PersistResult race_persist_rom_save(const PersistWriter* writer, const char* filename);

extern const RacePersistFormat RACE_PERSIST_ROM_OLC;

#endif // MUD98__PERSIST__ROM_OLC__RACE_PERSIST_ROM_OLC_H
