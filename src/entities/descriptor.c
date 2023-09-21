////////////////////////////////////////////////////////////////////////////////
// descriptor.c
////////////////////////////////////////////////////////////////////////////////

#include "descriptor.h"

#include "db.h"

Descriptor* descriptor_free = NULL;
Descriptor* descriptor_list = NULL;

void free_descriptor(Descriptor* d)
{
    if (!IS_VALID(d))
        return;

    free_string(d->host);
    free_mem(d->outbuf, d->outsize);
    INVALIDATE(d);
    d->next = descriptor_free;
    descriptor_free = d;
}

Descriptor* new_descriptor()
{
    Descriptor* d;

    if (descriptor_free == NULL)
        d = alloc_perm(sizeof(*d));
    else {
        d = descriptor_free;
        descriptor_free = descriptor_free->next;
    }

    memset(d, 0, sizeof(Descriptor));
    VALIDATE(d);
    return d;
}
