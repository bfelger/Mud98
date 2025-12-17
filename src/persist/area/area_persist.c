////////////////////////////////////////////////////////////////////////////////
// persist/area/area_persist.c
// Shared helpers for area persistence formats.
////////////////////////////////////////////////////////////////////////////////

#include "area_persist.h"

#ifdef ENABLE_ROM_OLC_PERSISTENCE
#include "rom-olc/area_persist_rom_olc.h"
#endif

#ifdef ENABLE_JSON_PERSISTENCE
#include "json/area_persist_json.h"
#endif

#include <string.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

static const char* file_ext(const char* path)
{
    if (!path)
        return "";
    const char* dot = strrchr(path, '.');
    return dot ? dot + 1 : "";
}

const AreaPersistFormat* area_persist_select_format(const char* file_name)
{
    const char* ext = file_ext(file_name);
#ifdef ENABLE_JSON_PERSISTENCE
    if (ext && strcasecmp(ext, "json") == 0)
        return &AREA_PERSIST_JSON;
#endif
    (void)ext;
#ifdef ENABLE_ROM_OLC_PERSISTENCE
    return &AREA_PERSIST_ROM_OLC;
#else
    return NULL;
#endif
}
