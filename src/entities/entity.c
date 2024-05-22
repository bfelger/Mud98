////////////////////////////////////////////////////////////////////////////////
// entity.c
////////////////////////////////////////////////////////////////////////////////

#include "entities/entity.h"

#include "db.h"

void init_header(EntityHeader* header, ObjType type)
{
    header->obj.type = type;
    init_table(&header->fields);

    header->name = lox_empty_string;
    SET_LOX_FIELD(header, header->name, name);

    SET_NATIVE_FIELD(header, header->vnum, vnum, I32);
}