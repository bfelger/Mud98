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

#define DECLARE_CONFIG(type, val)                                              \
    void cfg_set_ ## val(const type new_val);                                  \
    const type cfg_get_ ## val();

#define DECLARE_OPEN_CFG_FILE(val, rw)                                         \
    FILE* open_ ## rw ## _ ## val();

#define DECLARE_FILE_EXISTS(val)                                               \
    bool val ## _exists();

#define DECLARE_FILE_CONFIG(val)                                               \
    DECLARE_CONFIG(char*, val)                                                 \
    DECLARE_OPEN_CFG_FILE(val, read)                                           \
    DECLARE_OPEN_CFG_FILE(val, write)                                          \
    DECLARE_FILE_EXISTS(val)

#define DECLARE_LOG_CONFIG(val)                                                \
    DECLARE_CONFIG(char*, val)                                                 \
    DECLARE_OPEN_CFG_FILE(val, append)                                         \
    DECLARE_FILE_EXISTS(val)

DECLARE_CONFIG(char*, base_dir)
DECLARE_CONFIG(char*, area_dir)
DECLARE_CONFIG(char*, player_dir)
DECLARE_CONFIG(char*, gods_dir)
DECLARE_CONFIG(char*, temp_dir)
DECLARE_CONFIG(char*, data_dir)
DECLARE_CONFIG(char*, progs_dir)
DECLARE_FILE_CONFIG(socials_file)
DECLARE_FILE_CONFIG(groups_file)
DECLARE_FILE_CONFIG(skills_file)
DECLARE_FILE_CONFIG(commands_file)
DECLARE_FILE_CONFIG(races_file)
DECLARE_FILE_CONFIG(classes_file)
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

DECLARE_CONFIG(bool, telnet_enabled)
DECLARE_CONFIG(int, telnet_port)

#ifndef USE_RAW_SOCKETS
DECLARE_CONFIG(bool, tls_enabled)
DECLARE_CONFIG(int, tls_port)
DECLARE_CONFIG(char*, keys_dir)
DECLARE_FILE_CONFIG(cert_file)
DECLARE_FILE_CONFIG(pkey_file)
#endif

#endif // !MUD98__CONFIG_H
