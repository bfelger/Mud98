////////////////////////////////////////////////////////////////////////////////
// persist/player/player_persist.c
// Format helpers shared by player persistence implementations.
////////////////////////////////////////////////////////////////////////////////

#include "player_persist.h"

#include <db.h>

PlayerPersistFormat player_persist_format_from_string(const char* name)
{
    if (!name)
        return PLAYER_PERSIST_ROM_OLC;

    if (!str_cmp(name, "json"))
        return PLAYER_PERSIST_JSON;
    if (!str_cmp(name, "rom") || !str_cmp(name, "rom-olc") || !str_cmp(name, "olc"))
        return PLAYER_PERSIST_ROM_OLC;

    return PLAYER_PERSIST_ROM_OLC;
}

const char* player_persist_format_name(PlayerPersistFormat fmt)
{
    switch (fmt) {
    case PLAYER_PERSIST_JSON: return "json";
    case PLAYER_PERSIST_ROM_OLC:
    default:
        return "rom-olc";
    }
}

const char* player_persist_format_extension(PlayerPersistFormat fmt)
{
    switch (fmt) {
    case PLAYER_PERSIST_JSON: return ".json";
    case PLAYER_PERSIST_ROM_OLC:
    default:
        return "";
    }
}

PlayerPersistFormat player_persist_alternate_format(PlayerPersistFormat fmt)
{
    return (fmt == PLAYER_PERSIST_JSON) ? PLAYER_PERSIST_ROM_OLC : PLAYER_PERSIST_JSON;
}
