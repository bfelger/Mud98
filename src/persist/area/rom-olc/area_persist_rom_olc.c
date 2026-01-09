////////////////////////////////////////////////////////////////////////////////
// persist/rom-olc/area_persist_rom_olc.c
// Stub for the legacy rom-olc text format backend.
////////////////////////////////////////////////////////////////////////////////

#include "area_persist_rom_olc.h"

#include <persist/persist_io_adapters.h>
#include <persist/rom-olc/loader_guard.h>
#include <persist/rom-olc/db_rom_olc.h>
#include <persist/recipe/rom-olc/recipe_persist_rom_olc.h>

#include <olc/olc.h>
#include <olc/olc_save.h>

#include <entities/area.h>
#include <entities/faction.h>
#include <entities/help_data.h>
#include <entities/mob_prototype.h>
#include <entities/obj_prototype.h>
#include <entities/reset.h>
#include <entities/room.h>

#include <data/quest.h>
#include <data/loot.h>

#include <lox/memory.h>

#include <db.h>
#include <mob_prog.h>

#include <stdlib.h>
#include <string.h>

const AreaPersistFormat AREA_PERSIST_ROM_OLC = {
    .name = "rom-olc",
    .load = persist_area_rom_olc_load,
    .save = persist_area_rom_olc_save,
};

static PersistResult rom_olc_not_implemented(const char* message)
{
    PersistResult result = {
        .status = PERSIST_ERR_INTERNAL,
        .message = message,
        .line = -1,
    };
    return result;
}

LoaderGuard* current_loader_guard = NULL;

// Hooks to intercept exit(1) style aborts in legacy loaders.
void loader_longjmp(const char* message, int line)
{
    static char guard_msg[256];
    if (current_loader_guard) {
        if (message) {
            strncpy(guard_msg, message, sizeof(guard_msg) - 1);
            guard_msg[sizeof(guard_msg) - 1] = '\0';
            current_loader_guard->message = guard_msg;
        }
        else {
            current_loader_guard->message = "loader error";
        }
        current_loader_guard->line = line;
        longjmp(current_loader_guard->env, 1);
    }
    exit(1);
}

PersistResult persist_area_rom_olc_load(const AreaPersistLoadParams* params)
{
    if (!params || !params->reader || !params->reader->ctx)
        return rom_olc_not_implemented("rom-olc load: missing reader");

    if (params->reader->ops != &PERSIST_FILE_STREAM_OPS)
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "rom-olc load requires FILE-backed reader", -1 };

    PersistResult result = { PERSIST_OK, NULL, -1 };
    FILE* fp = NULL;
    bool owns_file = false;
    const char* fname = params->file_name ? params->file_name : params->reader->name;

    if (params->reader->ops == &PERSIST_FILE_STREAM_OPS) {
        fp = (FILE*)params->reader->ctx;
    }
    else if (params->reader->ops == &PERSIST_BUFFER_STREAM_OPS) {
        PersistBufferReaderCtx* bufctx = (PersistBufferReaderCtx*)params->reader->ctx;
        fp = tmpfile();
        if (!fp) {
            return (PersistResult){ PERSIST_ERR_IO, "rom-olc load: could not create tmpfile", -1 };
        }
        size_t remaining = bufctx->len > bufctx->pos ? bufctx->len - bufctx->pos : 0;
        if (remaining > 0) {
            fwrite(bufctx->data + bufctx->pos, 1, remaining, fp);
            rewind(fp);
        }
        owns_file = true;
    }
    else {
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "rom-olc load requires FILE or buffer reader", -1 };
    }

    // Preserve global state expected by legacy loaders.
    FILE* prev_strArea = strArea;
    char prev_fpArea[MAX_INPUT_LENGTH];
    strcpy(prev_fpArea, fpArea);
    bool prev_fBootDb = fBootDb;

    strArea = fp;
    fBootDb = true;
    current_area_data = NULL;
    if (fname) {
        strncpy(fpArea, fname, sizeof(fpArea) - 1);
        fpArea[sizeof(fpArea) - 1] = '\0';
    }
    else {
        fpArea[0] = '\0';
    }

    LoaderGuard guard = { 0 };
    current_loader_guard = &guard;

    if (setjmp(guard.env) != 0) {
        result = (PersistResult){ PERSIST_ERR_FORMAT, guard.message ? guard.message : "loader error", guard.line };
    }
    else for (;;) {
        int letter = fread_letter(fp);
        if (letter != '#') {
            result = (PersistResult){ PERSIST_ERR_FORMAT, "Boot_db: # not found.", -1 };
            break;
        }

        char* word = fread_word(fp);

        if (word[0] == '$') {
            break;
        }
        else if (!str_cmp(word, "AREADATA"))
            load_area(fp);
        else if (!str_cmp(word, "HELPS"))
            load_helps(fp, fpArea);
        else if (!str_cmp(word, "MOBILES"))
            load_mobiles(fp);
        else if (!str_cmp(word, "MOBPROGS"))
            load_mobprogs(fp);
        else if (!str_cmp(word, "OBJECTS"))
            load_objects(fp);
        else if (!str_cmp(word, "FACTIONS"))
            load_factions(fp);
        else if (!str_cmp(word, "STORYBEATS"))
            load_story_beats(fp);
        else if (!str_cmp(word, "CHECKLIST"))
            load_checklist(fp);
        else if (!str_cmp(word, "DAYCYCLE"))
            load_area_daycycle(fp);
        else if (!str_cmp(word, "LOOT"))
            load_area_loot(fp, &current_area_data->header);
        else if (!str_cmp(word, "RECIPES"))
            load_recipes(fp);
        else if (!str_cmp(word, "ENDRECIPES"))
            ; // Skip end marker
        else if (!str_cmp(word, "RESETS"))
            load_resets(fp);
        else if (!str_cmp(word, "ROOMS"))
            load_rooms(fp);
        else if (!str_cmp(word, "SHOPS"))
            load_shops(fp);
        else if (!str_cmp(word, "SPECIALS"))
            load_specials(fp);
        else if (!str_cmp(word, "QUEST"))
            load_quest(fp);
        else {
            result = (PersistResult){ PERSIST_ERR_FORMAT, "Boot_db: bad section name.", -1 };
            break;
        }

        gc_protect_clear();
    }

    current_loader_guard = NULL;

    // Mirror boot_db behavior: create single-instance areas immediately.
    if (area_persist_succeeded(result)
        && current_area_data
        && params->create_single_instance
        && current_area_data->inst_type == AREA_INST_SINGLE) {
        create_area_instance(current_area_data, false);
    }

    // Restore globals so callers don’t inherit parser state.
    strArea = prev_strArea;
    strcpy(fpArea, prev_fpArea);
    fBootDb = prev_fBootDb;

    if (owns_file && fp)
        fclose(fp);

    return result;
}

PersistResult persist_area_rom_olc_save(const AreaPersistSaveParams* params)
{
    if (!params || !params->writer || !params->area)
        return rom_olc_not_implemented("rom-olc save: missing writer or area");

    FILE* fp = NULL;
    bool owns_file = false;

    if (params->writer->ops == &PERSIST_FILE_WRITER_OPS) {
        fp = (FILE*)params->writer->ctx;
    }
    else if (params->writer->ops == &PERSIST_BUFFER_WRITER_OPS) {
        fp = tmpfile();
        if (!fp) {
            return (PersistResult){ PERSIST_ERR_IO, "rom-olc save: could not create tmpfile", -1 };
        }
        owns_file = true;
    }
    else {
        return (PersistResult){ PERSIST_ERR_UNSUPPORTED, "rom-olc save requires FILE or buffer writer", -1 };
    }

    const AreaData* area = params->area;

    fprintf(fp, "#AREADATA\n");
    fprintf(fp, "Version %d\n", AREA_VERSION);
    fprintf(fp, "Name %s~\n", NAME_STR(area));
    fprintf(fp, "Builders %s~\n", fix_string(area->builders));
    fprintf(fp, "VNUMs %"PRVNUM" %"PRVNUM"\n", area->min_vnum, area->max_vnum);
    fprintf(fp, "Credits %s~\n", area->credits);
    fprintf(fp, "Security %d\n", area->security);
    if (area->sector != 0)
        fprintf(fp, "Sector %d\n", area->sector);
    fprintf(fp, "Low %d\n", area->low_range);
    fprintf(fp, "High %d\n", area->high_range);
    if (area->reset_thresh != 6)
        fprintf(fp, "Reset %d\n", area->reset_thresh);
    if (area->always_reset)
        fprintf(fp, "AlwaysReset %d\n", (int)area->always_reset);
    if (area->inst_type != AREA_INST_SINGLE)
        fprintf(fp, "InstType %d\n", area->inst_type);
    if (!IS_NULLSTR(area->loot_table))
        fprintf(fp, "LootTable %s~\n", area->loot_table);
    const GatherSpawnArray* gather_spawns = &area->gather_spawns;
    for (size_t i = 0; i < gather_spawns->count; ++i) {
        const GatherSpawn* spawn = &gather_spawns->spawns[i];
        fprintf(fp, "GatherSpawn %d %" PRVNUM " %d %d\n",
            spawn->spawn_sector, spawn->vnum, spawn->quantity, spawn->respawn_timer);
    }
    fprintf(fp, "End\n\n\n\n");

    save_area_daycycle(fp, (AreaData*)area);
    save_area_loot(fp, (AreaData*)area);
    save_story_beats(fp, (AreaData*)area);
    save_checklist(fp, (AreaData*)area);
    save_factions(fp, (AreaData*)area);
    save_mobiles(fp, (AreaData*)area);
    save_objects(fp, (AreaData*)area);
    save_rooms(fp, (AreaData*)area);
    save_specials(fp, (AreaData*)area);
    save_resets(fp, (AreaData*)area);
    save_shops(fp, (AreaData*)area);
    save_mobprogs(fp, (AreaData*)area);
    save_progs(area->min_vnum, area->max_vnum);
    save_quests(fp, (AreaData*)area);
    save_recipes(fp, (AreaData*)area);

    if (area->helps && area->helps->first)
        save_helps(fp, area->helps);

    fprintf(fp, "#$\n");

    if (params->writer->ops->flush)
        params->writer->ops->flush(params->writer->ctx);

    // If writing to buffer, copy temp file contents into the buffer writer.
    if (owns_file) {
        fflush(fp);
        fseek(fp, 0, SEEK_END);
        long len = ftell(fp);
        if (len < 0) {
            fclose(fp);
            return (PersistResult){ PERSIST_ERR_IO, "rom-olc save: ftell failed", -1 };
        }
        rewind(fp);

        char* tmp_buf = malloc((size_t)len);
        if (!tmp_buf) {
            fclose(fp);
            return (PersistResult){ PERSIST_ERR_INTERNAL, "rom-olc save: alloc failed", -1 };
        }
        size_t read = fread(tmp_buf, 1, (size_t)len, fp);
        if (read != (size_t)len) {
            free(tmp_buf);
            fclose(fp);
            return (PersistResult){ PERSIST_ERR_IO, "rom-olc save: fread failed", -1 };
        }

        size_t written = params->writer->ops->write(tmp_buf, (size_t)len, params->writer->ctx);
        free(tmp_buf);
        if (written != (size_t)len) {
            fclose(fp);
            return (PersistResult){ PERSIST_ERR_IO, "rom-olc save: buffer write failed", -1 };
        }
        fclose(fp);
    }

    return (PersistResult){ PERSIST_OK, NULL, -1 };
}
