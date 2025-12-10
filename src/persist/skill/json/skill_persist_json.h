////////////////////////////////////////////////////////////////////////////////
// persist/skill/json/skill_persist_json.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__SKILL__JSON__SKILL_PERSIST_JSON_H
#define MUD98__PERSIST__SKILL__JSON__SKILL_PERSIST_JSON_H

#include <persist/skill/skill_persist.h>

PersistResult skill_persist_json_load(const PersistReader* reader, const char* filename);
PersistResult skill_persist_json_save(const PersistWriter* writer, const char* filename);

PersistResult skill_group_persist_json_load(const PersistReader* reader, const char* filename);
PersistResult skill_group_persist_json_save(const PersistWriter* writer, const char* filename);

extern const SkillPersistFormat SKILL_PERSIST_JSON;
extern const SkillGroupPersistFormat SKILL_GROUP_PERSIST_JSON;

#endif // MUD98__PERSIST__SKILL__JSON__SKILL_PERSIST_JSON_H
