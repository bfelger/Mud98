////////////////////////////////////////////////////////////////////////////////
// persist/json/class_persist_json.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__JSON__CLASS_PERSIST_JSON_H
#define MUD98__PERSIST__JSON__CLASS_PERSIST_JSON_H

#include <persist/class/class_persist.h>
#include <persist/persist_result.h>
#include <persist/persist_io.h>

PersistResult class_persist_json_load(const PersistReader* reader, const char* filename);
PersistResult class_persist_json_save(const PersistWriter* writer, const char* filename);

extern const ClassPersistFormat CLASS_PERSIST_JSON;

#endif // MUD98__PERSIST__JSON__CLASS_PERSIST_JSON_H
