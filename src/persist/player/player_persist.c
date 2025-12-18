////////////////////////////////////////////////////////////////////////////////
// persist/player/player_persist.c
// Format helpers shared by player persistence implementations.
////////////////////////////////////////////////////////////////////////////////

#include "player_persist.h"

#include <db.h>

PlayerPersistFormat player_persist_format_from_string(const char* name)
{
    if (!name) {
#ifdef ENABLE_ROM_OLC_PERSISTENCE
        return PLAYER_PERSIST_ROM_OLC;
#else
        return PLAYER_PERSIST_JSON;
#endif
    }

#ifdef ENABLE_JSON_PERSISTENCE
    if (!str_cmp(name, "json"))
        return PLAYER_PERSIST_JSON;
#endif
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    if (!str_cmp(name, "rom") || !str_cmp(name, "rom-olc") || !str_cmp(name, "olc"))
        return PLAYER_PERSIST_ROM_OLC;
#endif

#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return PLAYER_PERSIST_ROM_OLC;
#elif defined(ENABLE_JSON_PERSISTENCE)
    return PLAYER_PERSIST_JSON;
#else
    // No formats enabled - should not happen
    return PLAYER_PERSIST_ROM_OLC;
#endif
}

const char* player_persist_format_name(PlayerPersistFormat fmt)
{
    switch (fmt) {
#ifdef ENABLE_JSON_PERSISTENCE
    case PLAYER_PERSIST_JSON: return "json";
#endif
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    case PLAYER_PERSIST_ROM_OLC:
#endif
    default:
        return "rom-olc";
    }
}

const char* player_persist_format_extension(PlayerPersistFormat fmt)
{
    switch (fmt) {
#ifdef ENABLE_JSON_PERSISTENCE
    case PLAYER_PERSIST_JSON: return ".json";
#endif
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    case PLAYER_PERSIST_ROM_OLC:
#endif
    default:
        return "";
    }
}

PlayerPersistFormat player_persist_alternate_format(PlayerPersistFormat fmt)
{
#if defined(ENABLE_JSON_PERSISTENCE) && defined(ENABLE_ROM_OLC_PERSISTENCE)
    return (fmt == PLAYER_PERSIST_JSON) ? PLAYER_PERSIST_ROM_OLC : PLAYER_PERSIST_JSON;
#elif defined(ENABLE_JSON_PERSISTENCE)
    return PLAYER_PERSIST_JSON;
#else
    return PLAYER_PERSIST_ROM_OLC;
#endif
}
