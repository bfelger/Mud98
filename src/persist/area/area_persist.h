////////////////////////////////////////////////////////////////////////////////
// persist/area/area_persist.h
// Area-specific persistence API built on generic persist primitives.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__AREA_PERSIST_H
#define MUD98__PERSIST__AREA_PERSIST_H

#include <persist/persist_io.h>
#include <persist/persist_result.h>

#include <stdbool.h>

typedef struct area_data_t AreaData;

typedef struct area_persist_load_params_t {
    const PersistReader* reader;
    const char* file_name; // Used for error text and to seed AreaData->file_name.
    bool create_single_instance; // Mirror boot_db behavior for AREA_INST_SINGLE.
} AreaPersistLoadParams;

typedef struct area_persist_save_params_t {
    const PersistWriter* writer;
    const AreaData* area;
    const char* file_name; // Destination label/path for errors.
} AreaPersistSaveParams;

typedef struct area_persist_format_t {
    const char* name; // e.g., "rom-olc", "json".
    PersistResult (*load)(const AreaPersistLoadParams* params);
    PersistResult (*save)(const AreaPersistSaveParams* params);
} AreaPersistFormat;

static inline bool area_persist_succeeded(PersistResult result)
{
    return persist_succeeded(result);
}

// Format selection helper: returns a format based on filename/extension.
// Always returns a non-NULL format (defaults to rom-olc).
const AreaPersistFormat* area_persist_select_format(const char* file_name);

#endif // !MUD98__PERSIST__AREA_PERSIST_H
