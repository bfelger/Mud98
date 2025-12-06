////////////////////////////////////////////////////////////////////////////////
// persist/json/area_persist_json.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__JSON__AREA_PERSIST_JSON_H
#define MUD98__PERSIST__JSON__AREA_PERSIST_JSON_H

#include <persist/area/area_persist.h>

PersistResult json_save(const AreaPersistSaveParams* params);
PersistResult json_load(const AreaPersistLoadParams* params);

extern const AreaPersistFormat AREA_PERSIST_JSON;

#endif // !MUD98__PERSIST__JSON__AREA_PERSIST_JSON_H
