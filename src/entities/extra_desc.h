////////////////////////////////////////////////////////////////////////////////
// extra_desc.h
// Utilities to handle extra, named descriptions
////////////////////////////////////////////////////////////////////////////////

typedef struct extra_desc_t ExtraDesc;

#pragma once
#ifndef MUD98__ENTITIES__EXTRA_DESC_H
#define MUD98__ENTITIES__EXTRA_DESC_H

#include <stdbool.h>

typedef struct extra_desc_t {
    ExtraDesc* next;        // Next in list
    char* keyword;          // Keyword in look/examine
    char* description;      // What to see
    bool valid;
} ExtraDesc;

void free_extra_desc(ExtraDesc* ed);
char* get_extra_desc(const char* name, ExtraDesc* ed);
ExtraDesc* new_extra_desc();

#endif // !MUD98__ENTITIES__EXTRA_DESC_H
