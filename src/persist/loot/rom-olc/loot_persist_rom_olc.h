////////////////////////////////////////////////////////////////////////////////
// persist/loot/rom-olc/loot_persist_rom_olc.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__LOOT__ROM_OLC__LOOT_PERSIST_ROM_OLC_H
#define MUD98__PERSIST__LOOT__ROM_OLC__LOOT_PERSIST_ROM_OLC_H

#include <persist/loot/loot_persist.h>
#include <persist/persist_result.h>
#include <persist/persist_io.h>

#include <entities/entity.h>

PersistResult loot_persist_rom_olc_load(const PersistReader* reader, const char* filename, Entity* owner);
PersistResult loot_persist_rom_olc_save(const PersistWriter* writer, const char* filename, Entity* owner);

extern const LootPersistFormat LOOT_PERSIST_ROM_OLC;

#endif // MUD98__PERSIST__LOOT__ROM_OLC__LOOT_PERSIST_ROM_OLC_H
