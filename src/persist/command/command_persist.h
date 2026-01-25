////////////////////////////////////////////////////////////////////////////////
// persist/command/command_persist.h
// Persistence selector for the command table (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__COMMAND__COMMAND_PERSIST_H
#define MUD98__PERSIST__COMMAND__COMMAND_PERSIST_H

#include <persist/persist_io.h>
#include <persist/persist_result.h>

typedef struct command_persist_format_t {
    const char* name;
    PersistResult (*load)(const PersistReader* reader, const char* filename);
    PersistResult (*save)(const PersistWriter* writer, const char* filename);
} CommandPersistFormat;

void load_command_table(void);
PersistResult command_persist_load(const char* filename);
PersistResult command_persist_save(const char* filename);

#endif // MUD98__PERSIST__COMMAND__COMMAND_PERSIST_H
