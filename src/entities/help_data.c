////////////////////////////////////////////////////////////////////////////////
// entities/help_data.c
// Utilities to handle helps and area helps
////////////////////////////////////////////////////////////////////////////////

#include "help_data.h"

#include <db.h>

#include <string.h>

HelpData* help_free = NULL;
HelpArea* help_area_free = NULL;

HelpData* help_first;
HelpData* help_last;
HelpArea* help_area_list;

int help_count;
int help_perm_count;

int help_area_count;
int help_area_perm_count;

HelpArea* new_help_area()
{
    LIST_ALLOC_PERM(help_area, HelpArea);

    return help_area;
}

void free_help_area(HelpArea* help_area)
{
    LIST_FREE(help_area);
}

HelpData* new_help_data()
{
    LIST_ALLOC_PERM(help, HelpData);

    return help;
}

void free_help_data(HelpData* help)
{
    free_string(help->keyword);
    free_string(help->text);

    LIST_FREE(help);
}
