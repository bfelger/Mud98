////////////////////////////////////////////////////////////////////////////////
// tablesave.c
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__TABLESAVE_H
#define MUD98__TABLESAVE_H

void save_skills();
void save_races();
void save_progs(VNUM minvnum, VNUM maxvnum);
void save_command_table();
void save_socials();
void save_groups();
MobProgCode* pedit_prog(VNUM vnum);
void load_command_table();
void load_races_table();
void load_socials_table();
void load_skills_table();
void load_groups();

#endif // !MUD98__TABLESAVE_H
