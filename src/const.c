/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
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

#include "comm.h"
#include "interp.h"
#include "spell_list.h"

#include "entities/object_data.h"

#include "data/damage.h"
#include "data/item.h"

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

/* wiznet table and prototype for future flag setting */
const struct wiznet_type wiznet_table[] = {
    { "on",         WIZ_ON,         IM  },
    { "prefix",     WIZ_PREFIX,     IM  },
    { "ticks",      WIZ_TICKS,      IM  },
    { "logins",     WIZ_LOGINS,     IM  },
    { "sites",      WIZ_SITES,      L4  },
    { "links",      WIZ_LINKS,      L7  },
    { "newbies",    WIZ_NEWBIE,     IM  },
    { "spam",       WIZ_SPAM,       L5  },
    { "deaths",     WIZ_DEATHS,     IM  },
    { "resets",     WIZ_RESETS,     L4  },
    { "mobdeaths",  WIZ_MOBDEATHS,  L4  },
    { "flags",      WIZ_FLAGS,      L5  },
    { "penalties",  WIZ_PENALTIES,  L5  },
    { "saccing",    WIZ_SACCING,    L5  },
    { "levels",     WIZ_LEVELS,     IM  },
    { "load",       WIZ_LOAD,       L2  },
    { "restore",    WIZ_RESTORE,    L2  },
    { "snoops",     WIZ_SNOOPS,     L2  },
    { "switches",   WIZ_SWITCHES,   L2  },
    { "secure",     WIZ_SECURE,     L1  },
    { NULL,         0,              0   }
};
