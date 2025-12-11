////////////////////////////////////////////////////////////////////////////////
// persist/lox/json/lox_persist_json.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__LOX__JSON__LOX_PERSIST_JSON_H
#define MUD98__PERSIST__LOX__JSON__LOX_PERSIST_JSON_H

#include <persist/lox/lox_persist.h>

PersistResult lox_persist_json_load(const PersistReader* reader, const char* filename);
PersistResult lox_persist_json_save(const PersistWriter* writer, const char* filename);

extern const LoxPersistFormat LOX_PERSIST_JSON;

#endif // MUD98__PERSIST__LOX__JSON__LOX_PERSIST_JSON_H
