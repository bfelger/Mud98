////////////////////////////////////////////////////////////////////////////////
// char_data.c
// Character data
////////////////////////////////////////////////////////////////////////////////

#include "char_data.h"

#include "db.h"
#include "handler.h"
#include "recycle.h"

#include "object_data.h"

#include "data/mobile.h"

CharData* char_list;
CharData* char_free;

int mobile_count = 0;

void free_char_data(CharData* ch)
{
    ObjectData* obj;
    ObjectData* obj_next = NULL;
    AffectData* paf;
    AffectData* paf_next = NULL;

    if (!IS_VALID(ch)) return;

    if (IS_NPC(ch)) mobile_count--;

    for (obj = ch->carrying; obj != NULL; obj = obj_next) {
        obj_next = obj->next_content;
        extract_obj(obj);
    }

    for (paf = ch->affected; paf != NULL; paf = paf_next) {
        paf_next = paf->next;
        affect_remove(ch, paf);
    }

    free_string(ch->name);
    free_string(ch->short_descr);
    free_string(ch->long_descr);
    free_string(ch->description);
    free_string(ch->prompt);
    free_string(ch->prefix);
    free_note(ch->pnote);
    free_player_data(ch->pcdata);

    ch->next = char_free;
    char_free = ch;

    INVALIDATE(ch);
    return;
}

CharData* new_char_data()
{
    static CharData ch_zero;
    CharData* ch;
    int i;

    if (char_free == NULL)
        ch = alloc_perm(sizeof(*ch));
    else {
        ch = char_free;
        NEXT_LINK(char_free);
    }

    *ch = ch_zero;
    VALIDATE(ch);
    ch->name = &str_empty[0];
    ch->short_descr = &str_empty[0];
    ch->long_descr = &str_empty[0];
    ch->description = &str_empty[0];
    ch->prompt = &str_empty[0];
    ch->prefix = &str_empty[0];
    ch->logon = current_time;
    ch->lines = PAGELEN;
    for (i = 0; i < 4; i++) ch->armor[i] = 100;
    ch->position = POS_STANDING;
    ch->hit = 20;
    ch->max_hit = 20;
    ch->mana = 100;
    ch->max_mana = 100;
    ch->move = 100;
    ch->max_move = 100;
    for (i = 0; i < STAT_COUNT; i++) {
        ch->perm_stat[i] = 13;
        ch->mod_stat[i] = 0;
    }

    return ch;
}
