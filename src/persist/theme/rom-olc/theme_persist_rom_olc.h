////////////////////////////////////////////////////////////////////////////////
// persist/theme/rom-olc/theme_persist_rom_olc.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__THEME__ROM_OLC__THEME_PERSIST_ROM_OLC_H
#define MUD98__PERSIST__THEME__ROM_OLC__THEME_PERSIST_ROM_OLC_H

#include <persist/theme/theme_persist.h>

PersistResult theme_persist_rom_olc_load(const PersistReader* reader, const char* filename);
PersistResult theme_persist_rom_olc_save(const PersistWriter* writer, const char* filename);

extern const ThemePersistFormat THEME_PERSIST_ROM_OLC;

#endif // !MUD98__PERSIST__THEME__ROM_OLC__THEME_PERSIST_ROM_OLC_H
