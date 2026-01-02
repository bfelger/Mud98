////////////////////////////////////////////////////////////////////////////////
// loot_edit.h
// OLC editor for loot groups and tables
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__OLC__LOOT_EDIT_H
#define MUD98__OLC__LOOT_EDIT_H

#include <entities/entity.h>
#include <entities/mobile.h>

#include <stdbool.h>

// Sub-editor entry point for use within other OLC commands (enters sub-editor mode)
bool olc_edit_loot(Mobile* ch, char* argument);

// Main loot editor interpreter (handles commands when in loot editor mode)
void ledit(Mobile* ch, char* argument);

// Loot group sub-editor interpreter
void loot_group_edit(Mobile* ch, char* argument);

// Loot table sub-editor interpreter
void loot_table_edit(Mobile* ch, char* argument);

// Entry point for "edit loot" command (global loot editing)
void do_ledit(Mobile* ch, char* argument);

#endif // !MUD98__OLC__LOOT_EDIT_H
