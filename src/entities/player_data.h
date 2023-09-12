////////////////////////////////////////////////////////////////////////////////
// player_data.h
// Player character data
////////////////////////////////////////////////////////////////////////////////

typedef struct color_config_t ColorConfig;
typedef struct player_data_t PlayerData;

#pragma once
#ifndef MUD98__ENTITIES__PLAYER_DATA_H
#define MUD98__ENTITIES__PLAYER_DATA_H

#include "merc.h"

#include "color.h"

#include "char_data.h"

typedef struct color_config_t {
    char* current_theme_name;   // For lazy-loading and discardability
    bool hide_256;          // Whether to show these higher-bit themes. Some
    bool hide_24bit;        // clients (like Windows CMD) can't handle them.
    bool xterm;             // Use xterm semi-colons for 24-bit colors.
    bool hide_rgb_help;     // Hide verbose 24-bit help at the end of THEME LIST.
} ColorConfig;

typedef struct player_data_t {
    char* alias[MAX_ALIAS];
    char* alias_sub[MAX_ALIAS];
    ColorTheme* color_themes[MAX_THEMES];   // Personal themes
    ColorTheme* current_theme;              // Channel color assignments
    CharData* ch;
    ColorConfig theme_config;
    PlayerData* next;
    BUFFER* buffer;
    char* bamfin;
    char* bamfout;
    char* title;
    bool* group_known;
    unsigned char* pwd_digest;
    time_t last_note;
    time_t last_idea;
    time_t last_penalty;
    time_t last_news;
    time_t last_changes;
    unsigned int pwd_digest_len;
    int security;                           // OLC Builder Security
    LEVEL last_level;
    SKNUM* learned;
    int16_t perm_hit;
    int16_t perm_mana;
    int16_t perm_move;
    int16_t true_sex;
    int16_t condition[4];
    int16_t points;
    bool confirm_delete;
    bool valid;
} PlayerData;

PlayerData* new_player_data();
void free_player_data(PlayerData* pcdata);

extern PlayerData* player_list;
extern PlayerData* player_free;

#endif // !MUD98__ENTITIES__PLAYER_DATA_H
