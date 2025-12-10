////////////////////////////////////////////////////////////////////////////////
// persist/social/social_persist.h
// Persistence selector for socials (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__SOCIAL__SOCIAL_PERSIST_H
#define MUD98__PERSIST__SOCIAL__SOCIAL_PERSIST_H

#include <persist/persist_io.h>
#include <persist/persist_result.h>

typedef struct social_persist_format_t {
    const char* name;
    PersistResult (*load)(const PersistReader* reader, const char* filename);
    PersistResult (*save)(const PersistWriter* writer, const char* filename);
} SocialPersistFormat;

PersistResult social_persist_load(const char* filename);
PersistResult social_persist_save(const char* filename);

#endif // MUD98__PERSIST__SOCIAL__SOCIAL_PERSIST_H
