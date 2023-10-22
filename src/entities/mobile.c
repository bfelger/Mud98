////////////////////////////////////////////////////////////////////////////////
// mobile.c
// Mobiles
////////////////////////////////////////////////////////////////////////////////

#include "mobile.h"

#include "db.h"
#include "handler.h"
#include "recycle.h"

#include "object.h"

#include "data/mobile_data.h"

Mobile* mob_list;
Mobile* mob_free;

int mob_count = 0;

void free_mobile(Mobile* ch)
{
    Object* obj;
    Affect* paf;

    if (!IS_VALID(ch)) return;

    if (IS_NPC(ch))
        mob_count--;

    FOR_EACH_CONTENT(obj, ch->carrying) {
        extract_obj(obj);
    }

    FOR_EACH(paf, ch->affected) {
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

    ch->next = mob_free;
    mob_free = ch;

    INVALIDATE(ch);
    return;
}

Mobile* new_mobile()
{
    static Mobile ch_zero;
    Mobile* ch;
    int i;

    if (mob_free == NULL)
        ch = alloc_perm(sizeof(*ch));
    else {
        ch = mob_free;
        NEXT_LINK(mob_free);
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
