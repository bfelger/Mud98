////////////////////////////////////////////////////////////////////////////////
// area_data.c
// Utilities to handle area
////////////////////////////////////////////////////////////////////////////////

#include "area_data.h"

#include "merc.h"

#include "db.h"

int area_count;
AreaData* area_first;
AreaData* area_last;
AreaData* area_free;

AreaData* new_area()
{
    AreaData* pArea;
    char buf[MAX_INPUT_LENGTH];

    if (!area_free) {
        pArea = alloc_perm(sizeof(*pArea));
        area_count++;
    }
    else {
        pArea = area_free;
        area_free = area_free->next;
    }

    pArea->next = NULL;
    pArea->name = str_dup("New area");
    pArea->area_flags = AREA_ADDED;
    pArea->security = 1;
    pArea->builders = str_dup("None");
    pArea->credits = str_dup("None");
    pArea->min_vnum = 0;
    pArea->max_vnum = 0;
    pArea->age = 0;
    pArea->nplayer = 0;
    pArea->empty = true;
    pArea->vnum = area_count - 1;
    sprintf(buf, "area%"PRVNUM".are", pArea->vnum);
    pArea->file_name = str_dup(buf);

    return pArea;
}

void free_area(AreaData* pArea)
{
    free_string(pArea->name);
    free_string(pArea->file_name);
    free_string(pArea->builders);
    free_string(pArea->credits);

    pArea->next = area_free->next;
    area_free = pArea;
    return;
}







