////////////////////////////////////////////////////////////////////////////////
// persist/theme/json/theme_persist_json.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__THEME__JSON__THEME_PERSIST_JSON_H
#define MUD98__PERSIST__THEME__JSON__THEME_PERSIST_JSON_H

#include <persist/theme/theme_persist.h>

PersistResult theme_persist_json_load(const PersistReader* reader, const char* filename);
PersistResult theme_persist_json_save(const PersistWriter* writer, const char* filename);

extern const ThemePersistFormat THEME_PERSIST_JSON;

#endif // !MUD98__PERSIST__THEME__JSON__THEME_PERSIST_JSON_H

