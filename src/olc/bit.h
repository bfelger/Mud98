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

char* flag_string(const struct flag_type* flag_table, FLAGS bits);

extern const struct flag_stat_type flag_stat_table[];

#endif // !MUD98__BIT_H
