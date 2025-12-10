////////////////////////////////////////////////////////////////////////////////
// persist/social/rom-olc/social_persist_rom_olc.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__SOCIAL__ROM_OLC__SOCIAL_PERSIST_ROM_OLC_H
#define MUD98__PERSIST__SOCIAL__ROM_OLC__SOCIAL_PERSIST_ROM_OLC_H

#include <persist/social/social_persist.h>

PersistResult social_persist_rom_olc_load(const PersistReader* reader, const char* filename);
PersistResult social_persist_rom_olc_save(const PersistWriter* writer, const char* filename);

extern const SocialPersistFormat SOCIAL_PERSIST_ROM_OLC;

#endif // MUD98__PERSIST__SOCIAL__ROM_OLC__SOCIAL_PERSIST_ROM_OLC_H
