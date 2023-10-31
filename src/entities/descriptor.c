////////////////////////////////////////////////////////////////////////////////
// descriptor.c
////////////////////////////////////////////////////////////////////////////////

#include "descriptor.h"

#include "db.h"

Descriptor* descriptor_free = NULL;
Descriptor* descriptor_list = NULL;

int descriptor_count;
int descriptor_perm_count;

Descriptor* new_descriptor()
{
    LIST_ALLOC_PERM(descriptor, Descriptor);
    
    VALIDATE(descriptor);

    return descriptor;
}

void free_descriptor(Descriptor* descriptor)
{
    if (descriptor == NULL)
        return;

    if (descriptor->client) {
#ifndef NO_OPENSSL
        if (descriptor->client->type == SOCK_TLS)
            free_mem(descriptor->client, sizeof(TlsClient));
        else
#endif
            free_mem(descriptor->client, sizeof(SockClient));
    }
    free_string(descriptor->host);
    free_mem(descriptor->outbuf, descriptor->outsize);
    INVALIDATE(descriptor);

    LIST_FREE(descriptor);
}


