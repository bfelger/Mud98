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

#include "save.h"

#include "color.h"
#include "comm.h"
#include "config.h"
#include "db.h"
#include "digest.h"
#include "handler.h"
#include "lookup.h"
#include "magic.h"
#include "recycle.h"
#include "skills.h"
#include "stringutils.h"
#include "tables.h"
#include "vt.h"

#include <persist/player/player_persist.h>
#include <persist/persist_io_adapters.h>

#include <entities/descriptor.h>
#include <entities/faction.h>
#include <entities/object.h>

#include <data/mobile_data.h>
#include <data/player.h>
#include <data/race.h>
#include <data/skill.h>
#include <data/tutorial.h>

#include <lox/lox.h>

#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#ifdef _MSC_VER
#include <direct.h>
#endif

extern int _filbuf(FILE*);
extern bool test_output_enabled;

static bool ensure_dir_exists(const char* path)
{
#ifdef _MSC_VER
    if (_mkdir(path) == 0 || errno == EEXIST)
        return true;
#else
    if (mkdir(path, 0775) == 0 || errno == EEXIST)
        return true;
#endif

    perror(path);
    return false;
}

char* print_flags(FLAGS flag)
{
    int count, pos = 0;
    static char buf[52];

    for (count = 0; count < 32; count++) {
        if (IS_SET(flag, 1 << count)) {
            if (count < 26)
                buf[pos] = (char)('A' + count);
            else
                buf[pos] = (char)('a' + (count - 26));
            pos++;
        }
    }

    if (pos == 0) {
        buf[pos] = '0';
        pos++;
    }

    buf[pos] = '\0';

    return buf;
}

static void build_player_path(char* out, size_t out_len, const char* dir,
    const char* capitalized_name, PlayerPersistFormat fmt, bool is_temp)
{
    const char* ext = player_persist_format_extension(fmt);
    snprintf(out, out_len, "%s%s%s%s", dir, capitalized_name, ext ? ext : "",
        is_temp ? ".temp" : "");
}

static void maybe_decompress(const char* path)
{
#ifndef _MSC_VER
    char gz_path[MIL];
    snprintf(gz_path, sizeof gz_path, "%s.gz", path);
    if (!file_exists(gz_path))
        return;

    char cmd[2 * MIL];
    snprintf(cmd, sizeof cmd, "gzip -dfq %s", gz_path);
    if (system(cmd) != 0) {
        char buf[MIL];
        snprintf(buf, sizeof buf, "ERROR: Failed to unzip file (Error Code: %d).", errno);
        log_string(buf);
        log_string(gz_path);
    }
#endif
}

static PersistResult player_persist_save_dispatch(PlayerPersistFormat fmt,
    const PlayerPersistSaveParams* params)
{
    switch (fmt) {
    case PLAYER_PERSIST_JSON:
        return player_persist_json_save(params);
    case PLAYER_PERSIST_ROM_OLC:
    default:
        return player_persist_rom_olc_save(params);
    }
}

static PersistResult player_persist_load_dispatch(PlayerPersistFormat fmt,
    const PlayerPersistLoadParams* params)
{
    switch (fmt) {
    case PLAYER_PERSIST_JSON:
        return player_persist_json_load(params);
    case PLAYER_PERSIST_ROM_OLC:
    default:
        return player_persist_rom_olc_load(params);
    }
}

static void resave_in_default_format(Mobile* ch)
{
    bool prev = test_output_enabled;
    test_output_enabled = false;
    save_char_obj(ch);
    test_output_enabled = prev;
}

void save_char_obj(Mobile* ch)
{
    char capitalized[MAX_INPUT_LENGTH];
    char final_path[MIL];
    char temp_path[MIL];
    FILE* fp;
    const char* player_dir = cfg_get_player_dir();

    if (IS_NPC(ch) || test_output_enabled)
        return;

    if (ch->desc != NULL && ch->desc->original != NULL)
        ch = ch->desc->original;

    if (!ensure_dir_exists(player_dir))
        return;

    if (IS_IMMORTAL(ch) || ch->level >= LEVEL_IMMORTAL) {
        const char* gods_dir = cfg_get_gods_dir();
        if (!ensure_dir_exists(gods_dir))
            return;

        sprintf(capitalized, "%s", capitalize(NAME_STR(ch)));
        char god_path[MIL];
        snprintf(god_path, sizeof god_path, "%s%s", gods_dir, capitalized);
        OPEN_OR_RETURN(fp = open_write_file(god_path));
        fprintf(fp, "Lev %2d Trust %2d  %s%s\n", ch->level, get_trust(ch), NAME_STR(ch), ch->pcdata->title);
        close_file(fp);
    }

    sprintf(capitalized, "%s", capitalize(NAME_STR(ch)));
    PlayerPersistFormat fmt = player_persist_format_from_string(cfg_get_default_format());
    build_player_path(final_path, sizeof final_path, player_dir, capitalized, fmt, false);
    build_player_path(temp_path, sizeof temp_path, player_dir, capitalized, fmt, true);

    OPEN_OR_RETURN(fp = open_write_file(temp_path));
    PersistWriter writer = persist_writer_from_file(fp, temp_path);
    PlayerPersistSaveParams params = {
        .ch = ch,
        .fp = fp,
        .writer = &writer,
        .path = temp_path,
    };
    PersistResult save_res = player_persist_save_dispatch(fmt, &params);
    close_file(fp);

    if (!persist_succeeded(save_res)) {
        bugf("save_char_obj: failed to save %s (%s): %s", NAME_STR(ch),
            player_persist_format_name(fmt), save_res.message ? save_res.message : "unknown");
        remove(temp_path);
        return;
    }

#ifdef _MSC_VER
    if (!MoveFileExA(temp_path, final_path, MOVEFILE_REPLACE_EXISTING)) {
        bugf("save_char_obj : Could not rename %s to %s!", temp_path, final_path);
        perror(final_path);
    }
#else
    if (rename(temp_path, final_path) != 0) {
        bugf("save_char_obj : Could not rename %s to %s!", temp_path, final_path);
        perror(final_path);
    }
#endif
}

static bool player_try_load_format(Descriptor* d, Mobile* ch, const char* name,
    const char* capitalized_name, PlayerPersistFormat fmt, PlayerPersistFormat* loaded_fmt)
{
    char path[MIL];
    build_player_path(path, sizeof path, cfg_get_player_dir(), capitalized_name, fmt, false);
    maybe_decompress(path);
    if (!file_exists(path))
        return false;

    FILE* fp;
    OPEN_OR_RETURN_FALSE(fp = open_read_file(path));
    PersistReader reader = persist_reader_from_file(fp, path);
    PlayerPersistLoadParams params = {
        .ch = ch,
        .descriptor = d,
        .player_name = name,
        .fp = fp,
        .reader = &reader,
        .path = path,
    };
    PersistResult load_res = player_persist_load_dispatch(fmt, &params);
    close_file(fp);
    if (!persist_succeeded(load_res)) {
        bugf("load_char_obj: failed loading %s (%s): %s", name,
            player_persist_format_name(fmt), load_res.message ? load_res.message : "unknown");
        return false;
    }

    if (loaded_fmt)
        *loaded_fmt = fmt;
    return true;
}

static void migrate_player_file(Mobile* ch, const char* capitalized_name,
    PlayerPersistFormat from_fmt, PlayerPersistFormat to_fmt)
{
    if (from_fmt == to_fmt)
        return;

    resave_in_default_format(ch);

    char old_path[MIL];
    build_player_path(old_path, sizeof old_path, cfg_get_player_dir(), capitalized_name, from_fmt, false);
    remove(old_path);
}

bool load_char_obj(Descriptor* d, char* name)
{
    char capitalized[MAX_INPUT_LENGTH] = { 0 };
    Mobile* ch;
    int stat;

    ch = new_mobile();
    ch->pcdata = new_player_data();
    ch->pcdata->ch = ch;
    d->character = ch;
    ch->desc = d;
    SET_NAME(ch, lox_string(name));
    ch->id = get_pc_id();
    ch->race = race_lookup("human");
    ch->act_flags = PLR_NOSUMMON;
    ch->comm_flags = COMM_COMBINE | COMM_PROMPT;
    ch->prompt = str_dup("<%hhp %mm %vmv> ");
    ch->pcdata->confirm_delete = false;
    ch->pcdata->pwd_digest = NULL;
    ch->pcdata->pwd_digest_len = 0;
    ch->pcdata->bamfin = str_dup("");
    ch->pcdata->bamfout = str_dup("");
    ch->pcdata->title = str_dup("");
    for (stat = 0; stat < STAT_COUNT; stat++)
        ch->perm_stat[stat] = 13;
    ch->pcdata->condition[COND_THIRST] = 48;
    ch->pcdata->condition[COND_FULL] = 48;
    ch->pcdata->condition[COND_HUNGER] = 48;
    ch->pcdata->security = 0;

    SET_BIT(ch->act_flags, PLR_COLOUR);
    set_default_colors(ch);
    for (int i = 0; i < MAX_THEMES; ++i)
        ch->pcdata->color_themes[i] = NULL;

    sprintf(capitalized, "%s", capitalize(name));

    PlayerPersistFormat preferred = player_persist_format_from_string(cfg_get_default_format());
    PlayerPersistFormat loaded_fmt = preferred;
    bool loaded = player_try_load_format(d, ch, name, capitalized, preferred, &loaded_fmt);
    if (!loaded) {
        PlayerPersistFormat fallback = player_persist_alternate_format(preferred);
        loaded = player_try_load_format(d, ch, name, capitalized, fallback, &loaded_fmt);
        if (!loaded)
            return false;
    }

    if (loaded_fmt != preferred)
        migrate_player_file(ch, capitalized, loaded_fmt, preferred);

    int i;

    if (ch->race == 0)
        ch->race = race_lookup("human");

    ch->size = race_table[ch->race].size;
    ch->dam_type = DAM_BASH;

    for (i = 0; i < RACE_NUM_SKILLS; i++) {
        if (race_table[ch->race].skills[i] == NULL)
            break;
        group_add(ch, race_table[ch->race].skills[i], false);
    }
    ch->affect_flags = ch->affect_flags | race_table[ch->race].aff;
    ch->imm_flags = ch->imm_flags | race_table[ch->race].imm;
    ch->res_flags = ch->res_flags | race_table[ch->race].res;
    ch->vuln_flags = ch->vuln_flags | race_table[ch->race].vuln;
    ch->form = race_table[ch->race].form;
    ch->parts = race_table[ch->race].parts;

    
    char* theme_name;
    if ((theme_name = ch->pcdata->theme_config.current_theme_name)) {
        ColorTheme* theme = lookup_color_theme(ch, theme_name);
        if (!theme) {
            free_string(theme_name);
            theme = (ColorTheme*)get_default_system_color_theme();
            if (theme)
                ch->pcdata->theme_config.current_theme_name = str_dup(theme->name);
            else
                ch->pcdata->theme_config.current_theme_name = NULL;
        }
        if (theme) {
            if (ch->pcdata->current_theme)
                free_color_theme(ch->pcdata->current_theme);
            ch->pcdata->current_theme = dup_color_theme(theme);
        }
        set_default_colors(ch);
    }

    return true;
}
