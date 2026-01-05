////////////////////////////////////////////////////////////////////////////////
// persist/recipe/json/recipe_persist_json.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__RECIPE__JSON__RECIPE_PERSIST_JSON_H
#define MUD98__PERSIST__RECIPE__JSON__RECIPE_PERSIST_JSON_H

#include <persist/recipe/recipe_persist.h>
#include <persist/persist_result.h>
#include <persist/persist_io.h>

#include <entities/entity.h>

#include <jansson.h>

PersistResult recipe_persist_json_load(const PersistReader* reader, const char* filename, Entity* owner);
PersistResult recipe_persist_json_save(const PersistWriter* writer, const char* filename, Entity* owner);

// For embedding recipes in area JSON files
json_t* recipe_persist_json_build(const Entity* owner);
void recipe_persist_json_parse(json_t* recipe_arr, Entity* owner);

extern const RecipePersistFormat RECIPE_PERSIST_JSON;

#endif // MUD98__PERSIST__RECIPE__JSON__RECIPE_PERSIST_JSON_H
