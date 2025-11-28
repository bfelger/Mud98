////////////////////////////////////////////////////////////////////////////////
// persist/class/class_persist.h
// Persistence selector for class data (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__CLASS__CLASS_PERSIST_H
#define MUD98__PERSIST__CLASS__CLASS_PERSIST_H

#include <persist/persist_io.h>
#include <persist/persist_result.h>

typedef struct class_persist_format_t {
    const char* name;
    PersistResult (*load)(const PersistReader* reader, const char* filename);
    PersistResult (*save)(const PersistWriter* writer, const char* filename);
} ClassPersistFormat;

PersistResult class_persist_load(const char* filename);
PersistResult class_persist_save(const char* filename);

extern const ClassPersistFormat CLASS_PERSIST_ROM;
extern const ClassPersistFormat CLASS_PERSIST_JSON;

#endif // MUD98__PERSIST__CLASS__CLASS_PERSIST_H
