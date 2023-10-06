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

#define ADD_EXTRA_DESC(t, ed)                                                  \
    if (!t->extra_desc) {                                             \
        t->extra_desc = ed;                                           \
    }                                                                          \
    else {                                                                     \
        ExtraDesc* i = t->extra_desc;                                 \
        while (i->next != NULL)                                                \
            NEXT_LINK(i);                                                       \
        i->next = ed;                                                          \
    }                                                                          \
    ed->next = NULL;

#endif // !MUD98__ENTITIES__EXTRA_DESC_H
