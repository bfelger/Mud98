////////////////////////////////////////////////////////////////////////////////
// entities/player_data.c
// Player character data
////////////////////////////////////////////////////////////////////////////////

#include "player_data.h"

#include <data/skill.h>

#include <entities/faction.h>

#include <color.h>
#include <config.h>
#include <db.h>
#include <digest.h>
#include <recycle.h>
#include <skills.h>

PlayerData* player_data_list = NULL;
PlayerData* player_data_free = NULL;
CharGenData* gen_data_free = NULL;

int player_data_count;
int player_data_perm_count;

int gen_data_count;
int gen_data_perm_count;

PlayerData* new_player_data()
{
    LIST_ALLOC_PERM(player_data, PlayerData);

    player_data->buffer = new_buf();
    player_data->learned = new_learned();
    player_data->group_known = new_boolarray(skill_group_count);
    player_data->quest_log = new_quest_log();
    player_data->reputations.entries = NULL;
    player_data->reputations.count = 0;
    player_data->reputations.capacity = 0;
    player_data->pwd_digest_hex = str_dup("");

    player_data->recall = cfg_get_default_recall();

    VALIDATE(player_data);

    return player_data;
}

void free_player_data(PlayerData* player_data)
{
    int alias;

    if (!IS_VALID(player_data))
        return;

    free_learned(player_data->learned);
    free_boolarray(player_data->group_known);
    free_digest(player_data->pwd_digest);
    free_string(player_data->pwd_digest_hex);
    free_color_theme(player_data->current_theme);
    for (int i = 0; i < MAX_THEMES; ++i)
        if (player_data->color_themes[i] != NULL)
            free_color_theme(player_data->color_themes[i]);
    free_string(player_data->bamfin);
    free_string(player_data->bamfout);
    free_string(player_data->title);
    free_buf(player_data->buffer);
    free_quest_log(player_data);

    for (alias = 0; alias < MAX_ALIAS; alias++) {
        free_string(player_data->alias[alias]);
        free_string(player_data->alias_sub[alias]);
    }

    if (player_data->reputations.entries != NULL && player_data->reputations.capacity > 0) {
        free_mem(
            player_data->reputations.entries,
            player_data->reputations.capacity * sizeof(FactionReputation));
        player_data->reputations.entries = NULL;
    }
    player_data->reputations.count = 0;
    player_data->reputations.capacity = 0;

    INVALIDATE(player_data);

    LIST_FREE(player_data);
}

CharGenData* new_gen_data()
{
    LIST_ALLOC_PERM(gen_data, CharGenData);

    gen_data->skill_chosen = new_boolarray(skill_count);
    gen_data->group_chosen = new_boolarray(skill_group_count);

    VALIDATE(gen_data);

    return gen_data;
}

void free_gen_data(CharGenData* gen_data)
{
    if (!IS_VALID(gen_data))
        return;

    INVALIDATE(gen_data);

    free_boolarray(gen_data->skill_chosen);
    free_boolarray(gen_data->group_chosen);

    LIST_FREE(gen_data);
}
