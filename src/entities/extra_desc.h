////////////////////////////////////////////////////////////////////////////////
// entities/extra_desc.h
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

ExtraDesc* new_extra_desc();
void free_extra_desc(ExtraDesc* ed);
char* get_extra_desc(const char* name, ExtraDesc* ed);

extern int extra_desc_count;
extern int extra_desc_perm_count;

#define ADD_EXTRA_DESC(ent, ed)                                                \
    if (!ent->extra_desc) {                                                    \
        ent->extra_desc = ed;                                                  \
    }                                                                          \
    else {                                                                     \
        ExtraDesc* ed_i = ent->extra_desc;                                     \
        while (ed_i->next != NULL)                                             \
            NEXT_LINK(ed_i);                                                   \
        ed_i->next = ed;                                                       \
    }                                                                          \
    ed->next = NULL;

#endif // !MUD98__ENTITIES__EXTRA_DESC_H
