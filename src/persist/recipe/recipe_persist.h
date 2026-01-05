////////////////////////////////////////////////////////////////////////////////
// persist/recipe/recipe_persist.h
// Persistence selector for recipe data (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__RECIPE__RECIPE_PERSIST_H
#define MUD98__PERSIST__RECIPE__RECIPE_PERSIST_H

#include <persist/persist_io.h>
#include <persist/persist_result.h>

#include <entities/entity.h>

typedef struct recipe_persist_format_t {
    const char* name;
    PersistResult (*load)(const PersistReader* reader, const char* filename, Entity* owner);
    PersistResult (*save)(const PersistWriter* writer, const char* filename, Entity* owner);
} RecipePersistFormat;

// Load/save global recipe DB with format selection based on extension
PersistResult recipe_persist_load(const char* filename);
PersistResult recipe_persist_save(const char* filename);

// Load/save recipe section for a specific owner (area-specific recipes)
PersistResult recipe_persist_load_section(const PersistReader* reader, Entity* owner);
PersistResult recipe_persist_save_section(const PersistWriter* writer, Entity* owner);

// Format selection
const RecipePersistFormat* recipe_persist_select_format(const char* filename);

#endif // MUD98__PERSIST__RECIPE__RECIPE_PERSIST_H
