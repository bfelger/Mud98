////////////////////////////////////////////////////////////////////////////////
// tablesave.c
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef ROM__TABLESAVE_H
#define ROM__TABLESAVE_H

void save_skills();
void save_races();
void save_progs(int minvnum, int maxvnum);
void save_command_table();
void save_socials();
void save_groups();
MPROG_CODE* pedit_prog(int vnum);
void load_command_table();
void load_races_table();
void load_socials_table();
void load_skills_table();
void load_groups();

#endif // !ROM__TABLESAVE_H
