////////////////////////////////////////////////////////////////////////////////
// config.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__CONFIG_H
#define MUD98__CONFIG_H

#include "file.h"

#include <stdbool.h>
#include <stdio.h>

void load_config();

#define DECLARE_CONFIG(val, type)                                              \
    void cfg_set_ ## val(type new_val);                                        \
    type cfg_get_ ## val();

#define DECLARE_OPEN_CFG_FILE(val, rw)                                         \
    FILE* open_ ## rw ## _ ## val();

#define DECLARE_FILE_EXISTS(val)                                               \
    bool val ## _exists();

#define DECLARE_FILE_CONFIG(val)                                               \
    DECLARE_CONFIG(val, const char*)                                           \
    DECLARE_OPEN_CFG_FILE(val, read)                                           \
    DECLARE_OPEN_CFG_FILE(val, write)                                          \
    DECLARE_FILE_EXISTS(val)

#define DECLARE_LOG_CONFIG(val)                                                \
    DECLARE_CONFIG(val, const char*)                                           \
    DECLARE_OPEN_CFG_FILE(val, append)                                         \
    DECLARE_FILE_EXISTS(val)

#define DECLARE_STR_CONFIG(val)                                                \
    DECLARE_CONFIG(val, const char*)

DECLARE_STR_CONFIG(mud_name)

// Connection configs
DECLARE_STR_CONFIG(hostname)
DECLARE_CONFIG(telnet_enabled, bool)
DECLARE_CONFIG(telnet_port, int)
DECLARE_CONFIG(tls_enabled, bool)
DECLARE_CONFIG(tls_port, int)
DECLARE_STR_CONFIG(keys_dir)
DECLARE_FILE_CONFIG(cert_file)
DECLARE_FILE_CONFIG(pkey_file)
DECLARE_CONFIG(debug_telopt, bool)

// GMCP configs
DECLARE_CONFIG(gmcp_enabled, bool)

// MCCP configs
DECLARE_CONFIG(mccp2_enabled, bool)
DECLARE_CONFIG(mccp3_enabled, bool)

// MSDP configs
DECLARE_CONFIG(msdp_enabled, bool)

// MSSP configs
DECLARE_CONFIG(mssp_enabled, bool)
DECLARE_STR_CONFIG(codebase)
DECLARE_STR_CONFIG(contact)
DECLARE_CONFIG(created, int)
DECLARE_STR_CONFIG(discord)
DECLARE_STR_CONFIG(icon)
DECLARE_STR_CONFIG(language)
DECLARE_STR_CONFIG(location)
DECLARE_CONFIG(min_age, int)
DECLARE_STR_CONFIG(website)
DECLARE_STR_CONFIG(family)
DECLARE_STR_CONFIG(genre)
DECLARE_STR_CONFIG(gameplay)
DECLARE_STR_CONFIG(status)
DECLARE_STR_CONFIG(subgenre)

// Path configs
DECLARE_STR_CONFIG(default_format)
DECLARE_STR_CONFIG(base_dir)
DECLARE_STR_CONFIG(area_dir)
DECLARE_STR_CONFIG(player_dir)
DECLARE_STR_CONFIG(gods_dir)
DECLARE_STR_CONFIG(temp_dir)
DECLARE_STR_CONFIG(data_dir)
DECLARE_STR_CONFIG(progs_dir)
DECLARE_STR_CONFIG(scripts_dir)
DECLARE_FILE_CONFIG(lox_file)
DECLARE_FILE_CONFIG(socials_file)
DECLARE_FILE_CONFIG(groups_file)
DECLARE_FILE_CONFIG(skills_file)
DECLARE_FILE_CONFIG(themes_file)
DECLARE_FILE_CONFIG(commands_file)
DECLARE_FILE_CONFIG(races_file)
DECLARE_FILE_CONFIG(classes_file)
DECLARE_FILE_CONFIG(tutorials_file)
DECLARE_FILE_CONFIG(area_list)
DECLARE_FILE_CONFIG(music_file)
DECLARE_LOG_CONFIG(bug_file)
DECLARE_LOG_CONFIG(typo_file)
DECLARE_LOG_CONFIG(note_file)
DECLARE_LOG_CONFIG(idea_file)
DECLARE_LOG_CONFIG(penalty_file)
DECLARE_LOG_CONFIG(news_file)
DECLARE_LOG_CONFIG(changes_file)
DECLARE_LOG_CONFIG(shutdown_file)
DECLARE_FILE_CONFIG(ban_file)
DECLARE_FILE_CONFIG(mem_dump_file)
DECLARE_FILE_CONFIG(mob_dump_file)
DECLARE_FILE_CONFIG(obj_dump_file)

// Game configs
DECLARE_CONFIG(chargen_custom, bool)
DECLARE_CONFIG(default_recall, int)
DECLARE_CONFIG(default_start_loc, int)
DECLARE_CONFIG(start_loc_by_race, bool)
DECLARE_CONFIG(start_loc_by_class, bool)
DECLARE_CONFIG(train_anywhere, bool)
DECLARE_CONFIG(practice_anywhere, bool)

#endif // !MUD98__CONFIG_H
