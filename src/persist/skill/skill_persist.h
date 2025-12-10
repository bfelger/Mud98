////////////////////////////////////////////////////////////////////////////////
// persist/skill/skill_persist.h
// Persistence selectors for skills and skill groups (ROM-OLC text and JSON).
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__SKILL__SKILL_PERSIST_H
#define MUD98__PERSIST__SKILL__SKILL_PERSIST_H

#include <persist/persist_io.h>
#include <persist/persist_result.h>

typedef struct skill_persist_format_t {
    const char* name;
    PersistResult (*load)(const PersistReader* reader, const char* filename);
    PersistResult (*save)(const PersistWriter* writer, const char* filename);
} SkillPersistFormat;

typedef struct skill_group_persist_format_t {
    const char* name;
    PersistResult (*load)(const PersistReader* reader, const char* filename);
    PersistResult (*save)(const PersistWriter* writer, const char* filename);
} SkillGroupPersistFormat;

PersistResult skill_persist_load(const char* filename);
PersistResult skill_persist_save(const char* filename);

PersistResult skill_group_persist_load(const char* filename);
PersistResult skill_group_persist_save(const char* filename);

#endif // MUD98__PERSIST__SKILL__SKILL_PERSIST_H
