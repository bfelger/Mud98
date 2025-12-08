////////////////////////////////////////////////////////////////////////////////
// persist/tutorial/json/tutorial_persist_json.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__TUTORIAL__JSON__TUTORIAL_PERSIST_JSON_H
#define MUD98__PERSIST__TUTORIAL__JSON__TUTORIAL_PERSIST_JSON_H

#include <persist/tutorial/tutorial_persist.h>

PersistResult tutorial_persist_json_load(const PersistReader* reader, const char* filename);
PersistResult tutorial_persist_json_save(const PersistWriter* writer, const char* filename);

extern const TutorialPersistFormat TUTORIAL_PERSIST_JSON;

#endif // MUD98__PERSIST__TUTORIAL__JSON__TUTORIAL_PERSIST_JSON_H
