////////////////////////////////////////////////////////////////////////////////
// player_data.h
// Player character data
////////////////////////////////////////////////////////////////////////////////

typedef struct player_data_t PlayerData;

#pragma once
#ifndef MUD98__ENTITIES__PLAYER_DATA_H
#define MUD98__ENTITIES__PLAYER_DATA_H

#include "merc.h"

#include "color.h"

#include "char_data.h"

typedef struct player_data_t {
    CharData* ch;
    ColorConfig theme_config;
    ColorTheme* current_theme;              // VT102 color assignments
    ColorTheme* color_themes[MAX_THEMES];   // Personal themes
    PlayerData* next;
    BUFFER* buffer;
    char* bamfin;
    char* bamfout;
    char* title;
    char* alias[MAX_ALIAS];
    char* alias_sub[MAX_ALIAS];
    SKNUM* learned;
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
