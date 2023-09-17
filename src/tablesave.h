////////////////////////////////////////////////////////////////////////////////
// tablesave.c
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__TABLESAVE_H
#define MUD98__TABLESAVE_H

#include "mob_prog.h"

typedef enum save_type_t {
    FIELD_STRING = 0,
    FIELD_FUNCTION_INT_TO_STR = 1,
    FIELD_INT16 = 2,
    FIELD_FLAGSTRING = 3,
    FIELD_INT = 4,
    FIELD_FLAGVECTOR = 5,
    FIELD_BOOL = 6,
    FIELD_INT16_ARRAY = 7,
    FIELD_STRING_ARRAY = 8,
    FIELD_FUNCTION_INT16_TO_STR = 9,
    FIELD_INT16_FLAGSTRING = 10,
    FIELD_BOOL_ARRAY = 11,
    FIELD_INUTIL = 12,
    FIELD_VNUM = 13,
    FIELD_VNUM_ARRAY = 14,
} SaveType;

typedef struct save_table_entry_t {
    char* field_name;
    SaveType field_type;
    uintptr_t field_ptr;
    const uintptr_t argument;
    const uintptr_t argument2;
} SaveTableEntry;

void load_table(FILE* fp, uintptr_t base_type, const SaveTableEntry* table,
    const uintptr_t pointer);
void load_struct(FILE* fp, uintptr_t base_type, const SaveTableEntry* table, 
    const uintptr_t pointer);
void save_struct(FILE* fp, uintptr_t base_type, const SaveTableEntry* table,
    const uintptr_t pointer);

void save_progs(VNUM minvnum, VNUM maxvnum);
void save_command_table();
MobProgCode* pedit_prog(VNUM vnum);
void load_command_table();

#endif // !MUD98__TABLESAVE_H
