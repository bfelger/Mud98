/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@hypercube.org)                                *
 *      Gabrielle Taylor (gtaylor@hypercube.org)                           *
 *      Brian Moore (zump@rom.org)                                         *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

#include "merc.h"

#include "db.h"
#include "handler.h"
#include "tables.h"

#include "data/mobile_data.h"
#include "data/race.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

FLAGS flag_lookup(const char* name, const struct flag_type* flag_table)
{
    int flag;

    for (flag = 0; flag_table[flag].name != NULL; flag++) {
        if (LOWER(name[0]) == LOWER(flag_table[flag].name[0])
            && !str_prefix(name, flag_table[flag].name))
            return flag_table[flag].bit;
    }

    return NO_FLAG;
}

int flag_index(FLAGS flag, const struct flag_type* flag_table)
{
    for (int index = 0; flag_table[index].name != NULL; index++) {
        if (flag == flag_table[index].bit)
            return index;
    }

    return NO_FLAG;
}

int clan_lookup(const char* name)
{
    int clan;

    for (clan = 0; clan < MAX_CLAN; clan++) {
        if (LOWER(name[0]) == LOWER(clan_table[clan].name[0])
            && !str_prefix(name, clan_table[clan].name))
            return clan;
    }

    return 0;
}

Position position_lookup(const char* name)
{
    for (int i = 0; i < POS_MAX; i++) {
        if (!str_prefix(name, position_table[i].name))
            return position_table[i].pos;
    }

    return POS_DEAD;
}

Sex sex_lookup(const char* name)
{
    for (int i = 0; i < SEX_COUNT; i++) {
        if (LOWER(name[0]) == LOWER(sex_table[i].name[0])
            && !str_prefix(name, sex_table[i].name))
            return sex_table[i].sex;
    }

    return SEX_NEUTRAL;
}

MobSize size_lookup(const char* name)
{
    for (int size = 0; size < MOB_SIZE_COUNT; size++) {
        if (LOWER(name[0]) == LOWER(mob_size_table[size].name[0])
            && !str_prefix(name, mob_size_table[size].name))
            return (MobSize)size;
    }

    return SIZE_MEDIUM;
}

/* returns race number */
int16_t race_lookup(const char* name)
{
    for (int16_t race = 0; race_table[race].name != NULL; race++) {
        if (LOWER(name[0]) == LOWER(race_table[race].name[0])
            && !str_prefix(name, race_table[race].name))
            return race;
    }

    return 0;
}

Stat stat_lookup(const char* name)
{
    for (int i = 0; i < STAT_COUNT; i++) {
        if (!str_prefix(name, stat_table[i].name))
            return stat_table[i].bit;
    }

    return -1;
}

ItemType item_lookup(const char* name)
{
    for (int type = 0; type < ITEM_TYPE_COUNT; type++) {
        if (LOWER(name[0]) == LOWER(item_type_table[type].name[0])
            && !str_prefix(name, item_type_table[type].name))
            return item_type_table[type].type;
    }

    return -1;
}

int liquid_lookup(const char* name)
{
    for (int liq = 0; liq < LIQ_COUNT; liq++) {
        if (LOWER(name[0]) == LOWER(liquid_table[liq].name[0])
            && !str_prefix(name, liquid_table[liq].name))
            return liq;
    }

    return -1;
}

HelpData* help_lookup(char* keyword)
{
    HelpData* pHelp;
    char temp[MIL];
    char argall[MIL] = "";

    argall[0] = '\0';

    while (keyword[0] != '\0') {
        keyword = one_argument(keyword, temp);
        if (argall[0] != '\0')
            strcat(argall, " ");
        strcat(argall, temp);
    }

    FOR_EACH(pHelp, help_first)
        if (is_name(argall, pHelp->keyword))
            return pHelp;

    return NULL;
}

HelpArea* had_lookup(char* arg)
{
    HelpArea* temp;

    FOR_EACH(temp, help_area_list)
        if (!str_cmp(arg, temp->filename))
            return temp;

    return NULL;
}
