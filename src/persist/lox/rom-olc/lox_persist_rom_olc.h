////////////////////////////////////////////////////////////////////////////////
// persist/lox/rom-olc/lox_persist_rom_olc.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__LOX__ROM_OLC__LOX_PERSIST_ROM_OLC_H
#define MUD98__PERSIST__LOX__ROM_OLC__LOX_PERSIST_ROM_OLC_H

#include <persist/lox/lox_persist.h>

PersistResult lox_persist_rom_olc_load(const PersistReader* reader, const char* filename);
PersistResult lox_persist_rom_olc_save(const PersistWriter* writer, const char* filename);

extern const LoxPersistFormat LOX_PERSIST_ROM_OLC;

#endif // MUD98__PERSIST__LOX__ROM_OLC__LOX_PERSIST_ROM_OLC_H
