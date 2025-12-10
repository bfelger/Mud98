////////////////////////////////////////////////////////////////////////////////
// persist/social/json/social_persist_json.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__SOCIAL__JSON__SOCIAL_PERSIST_JSON_H
#define MUD98__PERSIST__SOCIAL__JSON__SOCIAL_PERSIST_JSON_H

#include <persist/social/social_persist.h>

PersistResult social_persist_json_load(const PersistReader* reader, const char* filename);
PersistResult social_persist_json_save(const PersistWriter* writer, const char* filename);

extern const SocialPersistFormat SOCIAL_PERSIST_JSON;

#endif // MUD98__PERSIST__SOCIAL__JSON__SOCIAL_PERSIST_JSON_H
