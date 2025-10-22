////////////////////////////////////////////////////////////////////////////////
// player_data.h
// Player character data
////////////////////////////////////////////////////////////////////////////////

typedef struct color_config_t ColorConfig;
typedef struct char_gen_data_t CharGenData;
typedef struct player_data_t PlayerData;

#pragma once
#ifndef MUD98__ENTITIES__PLAYER_DATA_H
#define MUD98__ENTITIES__PLAYER_DATA_H

#include <merc.h>

#include "mobile.h"

#include <color.h>
#include <recycle.h>

#include <data/quest.h>
#include <data/player.h>

typedef struct color_config_t {
    char* current_theme_name;   // For lazy-loading and discardability
    bool hide_256;          // Whether to show these higher-bit themes. Some
    bool hide_24bit;        // clients (like Windows CMD) can't handle them.
    bool xterm;             // Use xterm semi-colons for 24-bit colors.
    bool hide_rgb_help;     // Hide verbose 24-bit help at the end of THEME LIST.
} ColorConfig;

typedef struct char_gen_data_t {
    CharGenData* next;
    bool* skill_chosen;
    bool* group_chosen;
    int points_chosen;
    bool valid;
} CharGenData;

typedef enum condition_type_t {
    COND_DRUNK  = 0,
    COND_FULL   = 1,
    COND_THIRST = 2,
    COND_HUNGER = 3,
} ConditionType;

#define COND_MAX 4

typedef struct player_data_t {
    char* alias[MAX_ALIAS];
    char* alias_sub[MAX_ALIAS];
    ColorTheme* color_themes[MAX_THEMES];   // Personal themes
    ColorTheme* current_theme;              // Channel color assignments
    Mobile* ch;
    ColorConfig theme_config;
    PlayerData* next;
    Buffer* buffer;
    QuestLog* quest_log;
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
    VNUM recall;
    int security;                           // OLC Builder Security
    LEVEL last_level;
    SKNUM* learned;
    int16_t perm_hit;
    int16_t perm_mana;
    int16_t perm_move;
    Sex true_sex;
    int16_t condition[COND_MAX];
    int16_t points;
    bool confirm_delete;
    bool valid;
} PlayerData;

PlayerData* new_player_data();
void free_player_data(PlayerData* pcdata);

extern PlayerData* player_data_list;
extern PlayerData* player_data_free;

extern void free_gen_data(CharGenData* gen);
extern CharGenData* new_gen_data();

extern int player_data_count;
extern int player_data_perm_count;

extern int gen_data_count;
extern int gen_data_perm_count;

#endif // !MUD98__ENTITIES__PLAYER_DATA_H
