////////////////////////////////////////////////////////////////////////////////
// player.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__DATA__PLAYER_H
#define MUD98__DATA__PLAYER_H

#include "merc.h"

typedef enum plr_act_flags_t {
    PLR_IS_NPC              = BIT(0), // Don't EVER set.
    // Unused                 BIT(1)
// auto flags
    PLR_AUTOASSIST          = BIT(2),
    PLR_AUTOEXIT            = BIT(3),
    PLR_AUTOLOOT            = BIT(4),
    PLR_AUTOSAC             = BIT(5),
    PLR_AUTOGOLD            = BIT(6),
    PLR_AUTOSPLIT           = BIT(7),
    PLR_TESTER              = BIT(8),
    // Unused                 BIT(9)
    // Unused                 BIT(10)
    // Unused                 BIT(11)
    // Unused                 BIT(12)
// personal flags
    PLR_HOLYLIGHT           = BIT(13),
    // Unused                 BIT(14)
    PLR_CANLOOT             = BIT(15),
    PLR_NOSUMMON            = BIT(16),
    PLR_NOFOLLOW            = BIT(17),
    // Unused                 BIT(18)           
    PLR_COLOUR              = BIT(19),
// penalty flags
    PLR_PERMIT              = BIT(20),
    // Unused                 BIT(21)  
    PLR_LOG                 = BIT(22),
    PLR_DENY                = BIT(23),
    PLR_FREEZE              = BIT(24),
    PLR_THIEF               = BIT(25),
    PLR_KILLER              = BIT(26),
} PlayerActFlags;

#endif // !MUD98__DATA__PLAYER_H
