////////////////////////////////////////////////////////////////////////////////
// extra_desc.c
// Utilities to handle extra, named descriptions
////////////////////////////////////////////////////////////////////////////////

#include "extra_desc.h"

#include "db.h"
#include "handler.h"

#include <stdlib.h>

ExtraDesc* extra_desc_free;

void free_extra_desc(ExtraDesc* ed)
{
    if (!IS_VALID(ed)) return;

    free_string(ed->keyword);
    free_string(ed->description);
    INVALIDATE(ed);

    ed->next = extra_desc_free;
    extra_desc_free = ed;
}

char* get_extra_desc(const char* name, ExtraDesc* ed)
{
    for (; ed != NULL; ed = ed->next) {
        if (is_name((char*)name, ed->keyword))
            return ed->description;
    }
    return NULL;
}

ExtraDesc* new_extra_desc()
{
    ExtraDesc* ed;

    if (extra_desc_free == NULL)
        ed = alloc_perm(sizeof(*ed));
    else {
        ed = extra_desc_free;
        extra_desc_free = extra_desc_free->next;
    }

    ed->keyword = &str_empty[0];
    ed->description = &str_empty[0];
    VALIDATE(ed);
    return ed;
}