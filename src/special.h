////////////////////////////////////////////////////////////////////////////////
// special.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__SPECIAL_H
#define MUD98__SPECIAL_H

#include "interp.h"

SPEC_FUN* spec_lookup(const char* name);
char* spec_name(SPEC_FUN* function);

extern const struct	spec_type spec_table[];

#endif // !MUD98__SPECIAL_H
