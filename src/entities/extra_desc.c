////////////////////////////////////////////////////////////////////////////////
// entities/extra_desc.c
// Utilities to handle extra, named descriptions
////////////////////////////////////////////////////////////////////////////////

#include "extra_desc.h"

#include <db.h>
#include <handler.h>

#include <stdlib.h>

ExtraDesc* extra_desc_free;

int extra_desc_count;
int extra_desc_perm_count;

ExtraDesc* new_extra_desc()
{
    LIST_ALLOC_PERM(extra_desc, ExtraDesc);

    extra_desc->keyword = &str_empty[0];
    extra_desc->description = &str_empty[0];
    VALIDATE(extra_desc);

    return extra_desc;
}

void free_extra_desc(ExtraDesc* extra_desc)
{
    if (!IS_VALID(extra_desc))
        return;

    free_string(extra_desc->keyword);
    free_string(extra_desc->description);
    INVALIDATE(extra_desc);

    LIST_FREE(extra_desc);
}

char* get_extra_desc(const char* name, ExtraDesc* ed)
{
    for (; ed != NULL; NEXT_LINK(ed)) {
        if (is_name((char*)name, ed->keyword))
            return ed->description;
    }
    return NULL;
}

