////////////////////////////////////////////////////////////////////////////////
// persist/command/json/command_persist_json.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__COMMAND__JSON__COMMAND_PERSIST_JSON_H
#define MUD98__PERSIST__COMMAND__JSON__COMMAND_PERSIST_JSON_H

#include <persist/command/command_persist.h>

PersistResult command_persist_json_load(const PersistReader* reader, const char* filename);
PersistResult command_persist_json_save(const PersistWriter* writer, const char* filename);

extern const CommandPersistFormat COMMAND_PERSIST_JSON;

#endif // MUD98__PERSIST__COMMAND__JSON__COMMAND_PERSIST_JSON_H
