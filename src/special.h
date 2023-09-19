////////////////////////////////////////////////////////////////////////////////
// special.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__SPECIAL_H
#define MUD98__SPECIAL_H

#include "interp.h"

SpecFunc* spec_lookup(const char* name);
char* spec_name(SpecFunc* function);

struct spec_type {
    char* name; /* special function name */
    SpecFunc* function; /* the function */
};

extern const struct	spec_type spec_table[];

#endif // !MUD98__SPECIAL_H
