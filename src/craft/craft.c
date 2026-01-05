////////////////////////////////////////////////////////////////////////////////
// craft/craft.c
// Crafting system enum tables and lookup functions.
////////////////////////////////////////////////////////////////////////////////

#include "craft.h"

#include "db.h"
#include "tables.h"

#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Material Type Table
////////////////////////////////////////////////////////////////////////////////

static const struct {
    CraftMatType type;
    const char* name;
} craft_mat_type_table[] = {
    { MAT_NONE,      "none"      },
    { MAT_HIDE,      "hide"      },
    { MAT_LEATHER,   "leather"   },
    { MAT_FUR,       "fur"       },
    { MAT_SCALE,     "scale"     },
    { MAT_BONE,      "bone"      },
    { MAT_MEAT,      "meat"      },
    { MAT_ORE,       "ore"       },
    { MAT_INGOT,     "ingot"     },
    { MAT_GEM,       "gem"       },
    { MAT_CLOTH,     "cloth"     },
    { MAT_WOOD,      "wood"      },
    { MAT_HERB,      "herb"      },
    { MAT_ESSENCE,   "essence"   },
    { MAT_COMPONENT, "component" },
    { MAT_NONE,      NULL        }  // Sentinel
};

const char* craft_mat_type_name(CraftMatType type)
{
    for (int i = 0; craft_mat_type_table[i].name != NULL; i++) {
        if (craft_mat_type_table[i].type == type)
            return craft_mat_type_table[i].name;
    }
    return "unknown";
}

CraftMatType craft_mat_type_lookup(const char* name)
{
    if (name == NULL || name[0] == '\0')
        return MAT_NONE;

    for (int i = 0; craft_mat_type_table[i].name != NULL; i++) {
        if (!str_prefix(name, craft_mat_type_table[i].name))
            return craft_mat_type_table[i].type;
    }
    return MAT_NONE;
}

////////////////////////////////////////////////////////////////////////////////
// Workstation Type Table
////////////////////////////////////////////////////////////////////////////////

static const struct {
    WorkstationType type;
    const char* name;
} workstation_type_table[] = {
    { WORK_FORGE,    "forge"    },
    { WORK_SMELTER,  "smelter"  },
    { WORK_TANNERY,  "tannery"  },
    { WORK_LOOM,     "loom"     },
    { WORK_ALCHEMY,  "alchemy"  },
    { WORK_COOKING,  "cooking"  },
    { WORK_ENCHANT,  "enchant"  },
    { WORK_WOODWORK, "woodwork" },
    { WORK_JEWELER,  "jeweler"  },
    { WORK_NONE,     NULL       }  // Sentinel
};

const char* workstation_type_name(WorkstationType type)
{
    // For single flags, return the name
    for (int i = 0; workstation_type_table[i].name != NULL; i++) {
        if (workstation_type_table[i].type == type)
            return workstation_type_table[i].name;
    }
    return "none";
}

WorkstationType workstation_type_lookup(const char* name)
{
    if (name == NULL || name[0] == '\0')
        return WORK_NONE;

    for (int i = 0; workstation_type_table[i].name != NULL; i++) {
        if (!str_prefix(name, workstation_type_table[i].name))
            return workstation_type_table[i].type;
    }
    return WORK_NONE;
}

void workstation_type_flags_string(WorkstationType flags, char* buf, size_t buflen)
{
    if (buf == NULL || buflen == 0)
        return;

    buf[0] = '\0';
    size_t pos = 0;

    for (int i = 0; workstation_type_table[i].name != NULL; i++) {
        if (flags & workstation_type_table[i].type) {
            if (pos > 0 && pos < buflen - 1) {
                buf[pos++] = ' ';
            }
            size_t len = strlen(workstation_type_table[i].name);
            if (pos + len < buflen) {
                strcpy(buf + pos, workstation_type_table[i].name);
                pos += len;
            }
        }
    }

    if (pos == 0) {
        strncpy(buf, "none", buflen - 1);
        buf[buflen - 1] = '\0';
    }
}

////////////////////////////////////////////////////////////////////////////////
// Discovery Type Table
////////////////////////////////////////////////////////////////////////////////

static const struct {
    DiscoveryType type;
    const char* name;
} discovery_type_table[] = {
    { DISC_KNOWN,     "known"     },
    { DISC_TRAINER,   "trainer"   },
    { DISC_SCROLL,    "scroll"    },
    { DISC_DISCOVERY, "discovery" },
    { DISC_QUEST,     "quest"     },
    { DISC_KNOWN,     NULL        }  // Sentinel
};

const char* discovery_type_name(DiscoveryType type)
{
    for (int i = 0; discovery_type_table[i].name != NULL; i++) {
        if (discovery_type_table[i].type == type)
            return discovery_type_table[i].name;
    }
    return "unknown";
}

DiscoveryType discovery_type_lookup(const char* name)
{
    if (name == NULL || name[0] == '\0')
        return DISC_KNOWN;

    for (int i = 0; discovery_type_table[i].name != NULL; i++) {
        if (!str_prefix(name, discovery_type_table[i].name))
            return discovery_type_table[i].type;
    }
    return DISC_KNOWN;
}
