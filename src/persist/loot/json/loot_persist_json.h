////////////////////////////////////////////////////////////////////////////////
// persist/loot/json/loot_persist_json.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__LOOT__JSON__LOOT_PERSIST_JSON_H
#define MUD98__PERSIST__LOOT__JSON__LOOT_PERSIST_JSON_H

#include <persist/loot/loot_persist.h>
#include <persist/persist_result.h>
#include <persist/persist_io.h>

#include <entities/entity.h>

PersistResult loot_persist_json_load(const PersistReader* reader, const char* filename, Entity* owner);
PersistResult loot_persist_json_save(const PersistWriter* writer, const char* filename, Entity* owner);

extern const LootPersistFormat LOOT_PERSIST_JSON;

#endif // MUD98__PERSIST__LOOT__JSON__LOOT_PERSIST_JSON_H
