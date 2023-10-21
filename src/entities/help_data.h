////////////////////////////////////////////////////////////////////////////////
// help_data.h
// Utilities to handle helps and area helps
////////////////////////////////////////////////////////////////////////////////

typedef struct help_data_t HelpData;
typedef struct help_area_t HelpArea;

#pragma once
#ifndef MUD98__ENTITIES__HELP_DATA_H
#define MUD98__ENTITIES__HELP_DATA_H

#include "area_data.h"
#include "merc.h"

typedef struct help_data_t {
    HelpData* next;
    HelpData* next_area;
    LEVEL level;
    char* keyword;
    char* text;
} HelpData;

typedef struct help_area_t {
    HelpArea* next;
    HelpData* first;
    HelpData* last;
    AreaData* area;
    char* filename;
    bool changed;
} HelpArea;

void free_help(HelpData* help);
HelpArea* new_help_area();
HelpData* new_help_data();

extern HelpArea* help_area_list;
extern HelpData* help_first;
extern HelpData* help_last;
extern int help_count;

#endif // !MUD98__ENTITIES__HELP_DATA_H
