////////////////////////////////////////////////////////////////////////////////
// persist/rom-olc/area_persist_rom_olc.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__PERSIST__ROM_OLC__AREA_PERSIST_ROM_OLC_H
#define MUD98__PERSIST__ROM_OLC__AREA_PERSIST_ROM_OLC_H

#include <persist/area/area_persist.h>
#include <persist/persist_io.h>

extern const AreaPersistFormat AREA_PERSIST_ROM_OLC;

PersistResult persist_area_rom_olc_save(const AreaPersistSaveParams* params);
PersistResult persist_area_rom_olc_load(const AreaPersistLoadParams* params);

#endif // !MUD98__PERSIST__ROM_OLC__AREA_PERSIST_ROM_OLC_H
