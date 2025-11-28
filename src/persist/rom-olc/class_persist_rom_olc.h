////////////////////////////////////////////////////////////////////////////////
// persist/rom-olc/class_persist_rom_olc.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__ROM_OLC__CLASS_PERSIST_ROM_OLC_H
#define MUD98__PERSIST__ROM_OLC__CLASS_PERSIST_ROM_OLC_H

#include <persist/persist_result.h>
#include <persist/persist_io.h>

PersistResult class_persist_rom_load(const PersistReader* reader, const char* filename);
PersistResult class_persist_rom_save(const PersistWriter* writer, const char* filename);

#endif // MUD98__PERSIST__ROM_OLC__CLASS_PERSIST_ROM_OLC_H
