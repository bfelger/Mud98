////////////////////////////////////////////////////////////////////////////////
// help_data.c
// Utilities to handle helps and area helps
////////////////////////////////////////////////////////////////////////////////

#include "help_data.h"

#include "db.h"

#include <string.h>

HelpData* help_free = NULL;
HelpArea* help_area_free = NULL;

HelpData* help_first;
HelpData* help_last;
HelpArea* help_area_list;

HelpArea* new_help_area()
{
    HelpArea* had;

    if (help_area_free) {
        had = help_area_free;
        help_area_free = help_area_free->next;
    }
    else
        had = alloc_perm(sizeof(HelpArea));

    if (had == NULL) {
        perror("Could not allocate HelpArea!");
        exit(-1);
    }

    memset(had, 0, sizeof(HelpArea));

    return had;
}

HelpData* new_help_data()
{
    HelpData* help;

    if (help_free) {
        help = help_free;
        help_free = help_free->next;
    }
    else
        help = alloc_perm(sizeof(HelpData));

    if (help == NULL) {
        perror("Could not allocate HelpData!");
        exit(-1);
    }

    memset(help, 0, sizeof(HelpData));

    return help;
}

void free_help(HelpData* help)
{
    free_string(help->keyword);
    free_string(help->text);
    help->next = help_free;
    help_free = help;
}