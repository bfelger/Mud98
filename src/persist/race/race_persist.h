////////////////////////////////////////////////////////////////////////////////
// persist/race/race_persist.h
// Persistence selector for race data (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__RACE__RACE_PERSIST_H
#define MUD98__PERSIST__RACE__RACE_PERSIST_H

#include <persist/persist_io.h>
#include <persist/persist_result.h>

typedef struct race_persist_format_t {
    const char* name;
    PersistResult (*load)(const PersistReader* reader, const char* filename);
    PersistResult (*save)(const PersistWriter* writer, const char* filename);
} RacePersistFormat;

PersistResult race_persist_load(const char* filename);
PersistResult race_persist_save(const char* filename);

#endif // MUD98__PERSIST__RACE__RACE_PERSIST_H
