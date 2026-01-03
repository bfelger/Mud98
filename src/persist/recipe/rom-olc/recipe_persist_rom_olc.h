////////////////////////////////////////////////////////////////////////////////
// persist/recipe/rom-olc/recipe_persist_rom_olc.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__RECIPE__ROM_OLC__RECIPE_PERSIST_ROM_OLC_H
#define MUD98__PERSIST__RECIPE__ROM_OLC__RECIPE_PERSIST_ROM_OLC_H

#include <persist/recipe/recipe_persist.h>
#include <persist/persist_result.h>
#include <persist/persist_io.h>

#include <entities/entity.h>

PersistResult recipe_persist_rom_olc_load(const PersistReader* reader, const char* filename, Entity* owner);
PersistResult recipe_persist_rom_olc_save(const PersistWriter* writer, const char* filename, Entity* owner);

extern const RecipePersistFormat RECIPE_PERSIST_ROM_OLC;

#endif // MUD98__PERSIST__RECIPE__ROM_OLC__RECIPE_PERSIST_ROM_OLC_H
