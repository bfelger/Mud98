///////////////////////////////////////////////////////////////////////////////
// olc_save.h
// Exports for legacy rom-olc save routines.
///////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__OLC__OLC_SAVE_H
#define MUD98__OLC__OLC_SAVE_H

#include <merc.h>

#include <entities/area.h>
#include <entities/help_data.h>

#include <entities/quest.h>

#include <tablesave.h>

// Core save entry points.
void save_area_list();
void save_area(AreaData* area);
void save_other_helps(Mobile* ch);

// Section writers used by persistence bridge.
void save_factions(FILE* fp, AreaData* area);
void save_mobiles(FILE* fp, AreaData* area);
void save_objects(FILE* fp, AreaData* area);
void save_rooms(FILE* fp, AreaData* area);
void save_specials(FILE* fp, AreaData* area);
void save_resets(FILE* fp, AreaData* area);
void save_shops(FILE* fp, AreaData* area);
void save_mobprogs(FILE* fp, AreaData* area);
void save_progs(VNUM minvnum, VNUM maxvnum);
void save_quests(FILE* fp, AreaData* area);
void save_helps(FILE* fp, HelpArea* ha);
void save_story_beats(FILE* fp, AreaData* area);
void save_checklist(FILE* fp, AreaData* area);
void save_area_daycycle(FILE* fp, AreaData* area);
void save_area_loot(FILE* fp, AreaData* area);

// Helpers shared by persistence bridge.
char* fix_string(const char* str);
char* fix_lox_script(const char* str);
bool area_changed();

#endif // !MUD98__OLC__OLC_SAVE_H
