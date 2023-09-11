////////////////////////////////////////////////////////////////////////////////
// player_data.c
// Player character data
////////////////////////////////////////////////////////////////////////////////

#include "player_data.h"

#include "color.h"
#include "db.h"
#include "digest.h"
#include "recycle.h"
#include "skills.h"

PlayerData* player_list = NULL;
PlayerData* player_free = NULL;

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
    pcdata->group_known = new_boolarray(max_group);

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