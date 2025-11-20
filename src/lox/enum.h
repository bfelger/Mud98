////////////////////////////////////////////////////////////////////////////////
// lox/enum.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__LOX__ENUM_H
#define MUD98__LOX__ENUM_H

#include "object.h"
#include "table.h"

typedef struct obj_enum_t {
    Obj obj;
    ObjString* name;
    Table values;
} ObjEnum;

ObjEnum* new_enum(ObjString* name);

#endif // !MUD98__LOX__ENUM_H
