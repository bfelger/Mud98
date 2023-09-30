////////////////////////////////////////////////////////////////////////////////
// player_data.c
// Player character data
////////////////////////////////////////////////////////////////////////////////

#include "player_data.h"

#include "data/skill.h"

#include "color.h"
#include "db.h"
#include "digest.h"
#include "recycle.h"
#include "skills.h"

PlayerData* player_list = NULL;
PlayerData* player_free = NULL;
CharGenData* gen_data_free = NULL;

PlayerData* new_player_data()
{
    int alias;

    static PlayerData player_zero = { 0 };
    PlayerData* player;

    if (player_free == NULL)
        player = alloc_perm(sizeof(*player));
    else {
        player = player_free;
        player_free = player_free->next;
    }

    *player = player_zero;

    for (alias = 0; alias < MAX_ALIAS; alias++) {
        player->alias[alias] = NULL;
        player->alias_sub[alias] = NULL;
    }

    player->buffer = new_buf();
    player->learned = new_learned();
    player->group_known = new_boolarray(skill_group_count);

    VALIDATE(player);
    return player;
}

void free_player_data(PlayerData* player)
{
    int alias;

    if (!IS_VALID(player))
        return;

    if (player == player_list) {
        player_list = player->next;
    }
    else {
        PlayerData* prev;

        for (prev = player_list; prev != NULL; prev = prev->next) {
            if (prev->next == player) {
                prev->next = player->next;
                break;
            }
        }

        if (prev == NULL) {
            bug("free_player_data: player_data not found.", 0);
            return;
        }
    }

    free_learned(player->learned);
    free_boolarray(player->group_known);
    free_digest(player->pwd_digest);
    free_color_theme(player->current_theme);
    for (int i = 0; i < MAX_THEMES; ++i)
        if (player->color_themes[i] != NULL)
            free_color_theme(player->color_themes[i]);
    free_string(player->bamfin);
    free_string(player->bamfout);
    free_string(player->title);
    free_buf(player->buffer);

    for (alias = 0; alias < MAX_ALIAS; alias++) {
        free_string(player->alias[alias]);
        free_string(player->alias_sub[alias]);
    }
    INVALIDATE(player);
    player->next = player_free;
    player_free = player;

    return;
}

CharGenData* new_gen_data()
{
    static CharGenData gen_zero;
    CharGenData* gen;

    if (gen_data_free == NULL)
        gen = alloc_perm(sizeof(*gen));
    else {
        gen = gen_data_free;
        gen_data_free = gen_data_free->next;
    }
    *gen = gen_zero;

    gen->skill_chosen = new_boolarray(skill_count);
    gen->group_chosen = new_boolarray(skill_group_count);

    VALIDATE(gen);
    return gen;
}

void free_gen_data(CharGenData* gen)
{
    if (!IS_VALID(gen)) return;

    INVALIDATE(gen);

    free_boolarray(gen->skill_chosen);
    free_boolarray(gen->group_chosen);

    gen->next = gen_data_free;
    gen_data_free = gen;
}
