////////////////////////////////////////////////////////////////////////////////
// persist/skill/rom-olc/skill_persist_rom_olc.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__SKILL__ROM_OLC__SKILL_PERSIST_ROM_OLC_H
#define MUD98__PERSIST__SKILL__ROM_OLC__SKILL_PERSIST_ROM_OLC_H

#include <persist/skill/skill_persist.h>

PersistResult skill_persist_rom_olc_load(const PersistReader* reader, const char* filename);
PersistResult skill_persist_rom_olc_save(const PersistWriter* writer, const char* filename);

PersistResult skill_group_persist_rom_olc_load(const PersistReader* reader, const char* filename);
PersistResult skill_group_persist_rom_olc_save(const PersistWriter* writer, const char* filename);

extern const SkillPersistFormat SKILL_PERSIST_ROM_OLC;
extern const SkillGroupPersistFormat SKILL_GROUP_PERSIST_ROM_OLC;

#endif // MUD98__PERSIST__SKILL__ROM_OLC__SKILL_PERSIST_ROM_OLC_H
