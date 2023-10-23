////////////////////////////////////////////////////////////////////////////////
// area.c
// Utilities to handle area
////////////////////////////////////////////////////////////////////////////////

#include "area.h"

#include "merc.h"

#include "db.h"

int area_count;
Area* area_first;
Area* area_last;
Area* area_free;

Area* new_area()
{
    static Area zero = { 0 };
    Area* area;
    char buf[MAX_INPUT_LENGTH];

    if (!area_free) {
        area = alloc_perm(sizeof(*area));
        area_count++;
    }
    else {
        area = area_free;
        NEXT_LINK(area_free);
    }

    *area = zero;

    area->name = str_dup("New area");
    area->area_flags = AREA_ADDED;
    area->security = 1;
    area->builders = str_dup("None");
    area->credits = str_dup("None");
    area->reset_thresh = 6;
    area->empty = true;
    area->vnum = area_count - 1;
    sprintf(buf, "area%"PRVNUM".are", area->vnum);
    area->file_name = str_dup(buf);

    return area;
}

void free_area(Area* area)
{
    free_string(area->name);
    free_string(area->file_name);
    free_string(area->builders);
    free_string(area->credits);

    area->next = area_free->next;
    area_free = area;
    return;
}







