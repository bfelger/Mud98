////////////////////////////////////////////////////////////////////////////////
// persist/loot/loot_persist.h
// Persistence selector for loot data (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__LOOT__LOOT_PERSIST_H
#define MUD98__PERSIST__LOOT__LOOT_PERSIST_H

#include <persist/persist_io.h>
#include <persist/persist_result.h>

#include <entities/entity.h>

typedef struct loot_persist_format_t {
    const char* name;
    PersistResult (*load)(const PersistReader* reader, const char* filename, Entity* owner);
    PersistResult (*save)(const PersistWriter* writer, const char* filename, Entity* owner);
} LootPersistFormat;

// Load/save global loot DB with format selection based on extension
PersistResult loot_persist_load(const char* filename);
PersistResult loot_persist_save(const char* filename);

// Load/save loot section for a specific owner (area-specific loot)
PersistResult loot_persist_load_section(const PersistReader* reader, Entity* owner);
PersistResult loot_persist_save_section(const PersistWriter* writer, Entity* owner);

// Format selection
const LootPersistFormat* loot_persist_select_format(const char* filename);

#endif // MUD98__PERSIST__LOOT__LOOT_PERSIST_H
