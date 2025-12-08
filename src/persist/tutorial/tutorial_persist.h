////////////////////////////////////////////////////////////////////////////////
// persist/tutorial/tutorial_persist.h
// Persistence selector for tutorials (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__TUTORIAL__TUTORIAL_PERSIST_H
#define MUD98__PERSIST__TUTORIAL__TUTORIAL_PERSIST_H

#include <persist/persist_io.h>
#include <persist/persist_result.h>

typedef struct tutorial_persist_format_t {
    const char* name;
    PersistResult (*load)(const PersistReader* reader, const char* filename);
    PersistResult (*save)(const PersistWriter* writer, const char* filename);
} TutorialPersistFormat;

PersistResult tutorial_persist_load(const char* filename);
PersistResult tutorial_persist_save(const char* filename);

#endif // MUD98__PERSIST__TUTORIAL__TUTORIAL_PERSIST_H
