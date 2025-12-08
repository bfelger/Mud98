////////////////////////////////////////////////////////////////////////////////
// persist/command/rom-olc/command_persist_rom_olc.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__COMMAND__ROM_OLC__COMMAND_PERSIST_ROM_OLC_H
#define MUD98__PERSIST__COMMAND__ROM_OLC__COMMAND_PERSIST_ROM_OLC_H

#include <persist/command/command_persist.h>

PersistResult command_persist_rom_olc_load(const PersistReader* reader, const char* filename);
PersistResult command_persist_rom_olc_save(const PersistWriter* writer, const char* filename);

extern const CommandPersistFormat COMMAND_PERSIST_ROM_OLC;

#endif // MUD98__PERSIST__COMMAND__ROM_OLC__COMMAND_PERSIST_ROM_OLC_H
