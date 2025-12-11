////////////////////////////////////////////////////////////////////////////////
// persist/lox/lox_persist.h
// Persistence selector for public Lox script catalog.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__LOX__LOX_PERSIST_H
#define MUD98__PERSIST__LOX__LOX_PERSIST_H

#include <persist/persist_io.h>
#include <persist/persist_result.h>

typedef struct lox_persist_format_t {
    const char* name;
    PersistResult (*load)(const PersistReader* reader, const char* filename);
    PersistResult (*save)(const PersistWriter* writer, const char* filename);
} LoxPersistFormat;

PersistResult lox_persist_load(const char* filename);
PersistResult lox_persist_save(const char* filename);

#endif // MUD98__PERSIST__LOX__LOX_PERSIST_H
