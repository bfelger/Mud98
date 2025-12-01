////////////////////////////////////////////////////////////////////////////////
// persist/area/area_persist.c
// Shared helpers for area persistence formats.
////////////////////////////////////////////////////////////////////////////////

#include "area_persist.h"

#include "rom-olc/area_persist_rom_olc.h"

#ifdef HAVE_JSON_AREAS
#include "json/area_persist_json.h"
#endif

#include <string.h>

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
#ifdef HAVE_JSON_AREAS
    if (ext && strcasecmp(ext, "json") == 0)
        return &AREA_PERSIST_JSON;
#endif
    (void)ext;
    return &AREA_PERSIST_ROM_OLC;
}
