////////////////////////////////////////////////////////////////////////////////
// persist/rom-olc/class_persist_rom_olc.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__ROM_OLC__CLASS_PERSIST_ROM_OLC_H
#define MUD98__PERSIST__ROM_OLC__CLASS_PERSIST_ROM_OLC_H

#include <persist/class/class_persist.h>
#include <persist/persist_result.h>
#include <persist/persist_io.h>

PersistResult class_persist_rom_olc_load(const PersistReader* reader, const char* filename);
PersistResult class_persist_rom_olc_save(const PersistWriter* writer, const char* filename);

extern const ClassPersistFormat CLASS_PERSIST_ROM_OLC;

#endif // MUD98__PERSIST__ROM_OLC__CLASS_PERSIST_ROM_OLC_H
