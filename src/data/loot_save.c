////////////////////////////////////////////////////////////////////////////////
// data/loot_save.c
// High-level save functions for loot data (delegates to persistence layer)
////////////////////////////////////////////////////////////////////////////////

#include "loot.h"

#include <persist/loot/loot_persist.h>
#include <persist/persist_io_adapters.h>

#include <merc.h>
#include <comm.h>
#include <config.h>

#include <stdio.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Save loot section to file (global or area-specific)
// This is a legacy wrapper for area persistence (always uses ROM-OLC format)
////////////////////////////////////////////////////////////////////////////////

void save_loot_section(FILE* fp, Entity* owner)
{
    PersistWriter writer = persist_writer_from_file(fp, NULL);
    PersistResult res = loot_persist_save_section(&writer, owner);
    if (!persist_succeeded(res)) {
        bugf("save_loot_section: %s", res.message ? res.message : "unknown error");
    }
}

////////////////////////////////////////////////////////////////////////////////
// Save global loot database (uses configured loot file)
////////////////////////////////////////////////////////////////////////////////

void save_global_loot_db()
{
    save_global_loot_db_as(cfg_get_loot_file());
}

////////////////////////////////////////////////////////////////////////////////
// Save global loot database to a specific file (format determined by extension)
////////////////////////////////////////////////////////////////////////////////

void save_global_loot_db_as(const char* filename)
{
    if (!global_loot_db) {
        return;
    }

    PersistResult res = loot_persist_save(filename);
    if (!persist_succeeded(res)) {
        bugf("save_global_loot_db_as: %s", res.message ? res.message : "unknown error");
        return;
    }

    printf_log("Saved global loot DB to %s: %d groups, %d tables", 
        filename,
        global_loot_db->group_count, global_loot_db->table_count);
}
