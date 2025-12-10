////////////////////////////////////////////////////////////////////////////////
// persist/tutorial/rom-olc/tutorial_persist_rom_olc.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__TUTORIAL__ROM_OLC__TUTORIAL_PERSIST_ROM_OLC_H
#define MUD98__PERSIST__TUTORIAL__ROM_OLC__TUTORIAL_PERSIST_ROM_OLC_H

#include <persist/tutorial/tutorial_persist.h>

PersistResult tutorial_persist_rom_olc_load(const PersistReader* reader, const char* filename);
PersistResult tutorial_persist_rom_olc_save(const PersistWriter* writer, const char* filename);

extern const TutorialPersistFormat TUTORIAL_PERSIST_ROM_OLC;

#endif // MUD98__PERSIST__TUTORIAL__ROM_OLC__TUTORIAL_PERSIST_ROM_OLC_H
