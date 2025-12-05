////////////////////////////////////////////////////////////////////////////////
// persist/theme/theme_persist.h
// Persistence selector for color theme data (ROM-OLC text).
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__THEME__THEME_PERSIST_H
#define MUD98__PERSIST__THEME__THEME_PERSIST_H

#include <persist/persist_io.h>
#include <persist/persist_result.h>

typedef struct theme_persist_format_t {
    const char* name;
    PersistResult (*load)(const PersistReader* reader, const char* filename);
    PersistResult (*save)(const PersistWriter* writer, const char* filename);
} ThemePersistFormat;

PersistResult theme_persist_load(const char* filename);
PersistResult theme_persist_save(const char* filename);
PersistResult theme_persist_save_with_format(const char* filename, const char* format_hint);

#endif // !MUD98__PERSIST__THEME__THEME_PERSIST_H
