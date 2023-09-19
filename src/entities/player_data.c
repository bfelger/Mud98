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

    static PlayerData pcdata_zero;
    PlayerData* pcdata;

    if (player_free == NULL)
        pcdata = alloc_perm(sizeof(*pcdata));
    else {
        pcdata = player_free;
        player_free = player_free->next;
    }

    *pcdata = pcdata_zero;

    for (alias = 0; alias < MAX_ALIAS; alias++) {
        pcdata->alias[alias] = NULL;
        pcdata->alias_sub[alias] = NULL;
    }

    pcdata->buffer = new_buf();
    pcdata->learned = new_learned();
    pcdata->group_known = new_boolarray(skill_group_count);

    VALIDATE(pcdata);
    return pcdata;
}

void free_player_data(PlayerData* pcdata)
{
    int alias;

    if (!IS_VALID(pcdata))
        return;

    if (pcdata == player_list) {
        player_list = pcdata->next;
    }
    else {
        PlayerData* prev;

        for (prev = player_list; prev != NULL; prev = prev->next) {
            if (prev->next == pcdata) {
                prev->next = pcdata->next;
                break;
            }
        }

        if (prev == NULL) {
            bug("Free_pcdata: pcdata not found.", 0);
            return;
        }
    }

    free_learned(pcdata->learned);
    free_boolarray(pcdata->group_known);
    free_digest(pcdata->pwd_digest);
    free_color_theme(pcdata->current_theme);
    for (int i = 0; i < MAX_THEMES; ++i)
        if (pcdata->color_themes[i] != NULL)
            free_color_theme(pcdata->color_themes[i]);
    free_string(pcdata->bamfin);
    free_string(pcdata->bamfout);
    free_string(pcdata->title);
    free_buf(pcdata->buffer);

    for (alias = 0; alias < MAX_ALIAS; alias++) {
        free_string(pcdata->alias[alias]);
        free_string(pcdata->alias_sub[alias]);
    }
    INVALIDATE(pcdata);
    pcdata->next = player_free;
    player_free = pcdata;

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
