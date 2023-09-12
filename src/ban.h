////////////////////////////////////////////////////////////////////////////////
// ban.h
////////////////////////////////////////////////////////////////////////////////

typedef struct ban_data_t BanData;

#pragma once
#ifndef MUD98__BAN_H
#define MUD98__BAN_H

#include "merc.h"

#include <stdbool.h>

#define BAN_SUFFIX    BIT(0)
#define BAN_PREFIX    BIT(1)
#define BAN_NEWBIES   BIT(2)
#define BAN_ALL       BIT(3)
#define BAN_PERMIT    BIT(4)
#define BAN_PERMANENT BIT(5)

struct ban_data_t {
    BanData* next;
    char* name;
    int ban_flags;
    LEVEL level;
    bool valid;
};

bool check_ban(char* site, int type);
void free_ban(BanData* ban);
void load_bans();
BanData* new_ban(void);

#endif // !MUD98__BAN_H
