////////////////////////////////////////////////////////////////////////////////
// bit.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__BIT_H
#define MUD98__BIT_H

#include "tables.h"

struct flag_stat_type {
    const struct flag_type* structure;
    bool stat;
};

extern const struct flag_stat_type flag_stat_table[];

#endif // !MUD98__BIT_H
