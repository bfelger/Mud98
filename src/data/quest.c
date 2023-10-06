////////////////////////////////////////////////////////////////////////////////
// quest.c
////////////////////////////////////////////////////////////////////////////////

#include "quest.h"

#include "olc/olc.h"

#include "comm.h"
#include "db.h"
#include "tables.h"
#include "update.h"

Quest* quest_free = NULL;
int top_quest = 0;

const struct flag_type quest_type_table[] = {
    { "visit_mob",      QUEST_VISIT_MOB,    true    },
    { NULL,             0,                  0       }
};

Quest tmp_quest;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const SaveTableEntry quest_save_table[] = {
    { "name",	        FIELD_STRING,           U(&tmp_quest.name),	        0,		            0   },
    { "vnum",	        FIELD_VNUM,             U(&tmp_quest.vnum),	        0,		            0   },
    { "entry",	        FIELD_STRING,           U(&tmp_quest.entry),	    0,		            0   },
    { "type",		    FIELD_INT16_FLAGSTRING,	U(&tmp_quest.type),	        U(quest_type_table),0   },
    { "xp",             FIELD_INT,              U(&tmp_quest.xp),           0,                  0   },
    { "level",          FIELD_INT16,            U(&tmp_quest.level),        0,                  0   },
    { "end",            FIELD_VNUM,             U(&tmp_quest.end),          0,                  0   },
    { "target",         FIELD_VNUM,             U(&tmp_quest.target),       0,                  0   },
    { NULL,		        0,				        0,			                0,		            0   }
};

Quest* new_quest()
{
    static Quest qZero;
    Quest* quest;

    if (quest_free == NULL) {
        quest = alloc_perm(sizeof(Quest));
        top_quest++;
    }
    else {
        quest = quest_free;
        NEXT_LINK(quest_free);
    }

    *quest = qZero;

    quest->name = &str_empty[0];
    quest->entry = &str_empty[0];
    quest->level = 0;
    quest->type = 0;
    quest->vnum = 0;
    quest->target = 0;
    quest->end = 0;
    quest->xp = 0;

    return quest;
}
QuestLog* new_quest_log()
{
    QuestLog* ql = alloc_mem(sizeof(QuestLog));
    ql->target_mobs = NULL;
    ql->target_objs = NULL;
    return ql;
}

void free_quest(Quest* quest)
{
    free_string(quest->name);
    free_string(quest->entry);

    quest->next = quest_free;
    quest_free = quest;
}

void free_quest_log(QuestLog* quest_log)
{
    QuestTarget* t;

    while (quest_log->target_mobs) {
        t = quest_log->target_mobs;
        NEXT_LINK(quest_log->target_mobs);
        free_mem(t, sizeof(QuestTarget));
    }

    while (quest_log->target_objs) {
        t = quest_log->target_objs;
        NEXT_LINK(quest_log->target_objs);
        free_mem(t, sizeof(QuestTarget));
    }

    free_mem(quest_log, sizeof(QuestLog));
}

Quest* get_quest(VNUM vnum)
{
    AreaData* area = get_vnum_area(vnum);
    Quest* quest = NULL;
    
    if (!area)
        return NULL;

    ORDERED_GET(Quest, quest, area->quests, vnum, vnum);

    return quest;
}

QuestTarget* get_quest_targ_mob(CharData* ch, VNUM target_vnum)
{
    QuestTarget* qt = NULL;

    if (IS_NPC(ch) || !ch->pcdata)
        return 0;

    ORDERED_GET(QuestTarget, qt, ch->pcdata->quest_log->target_mobs, target_vnum, target_vnum);

    if (qt == NULL)
        return 0;

    return qt;
}

QuestTarget* get_quest_targ_obj(CharData* ch, VNUM target_vnum)
{
    QuestTarget* qt = NULL;

    if (IS_NPC(ch) || !ch->pcdata)
        return 0;

    ORDERED_GET(QuestTarget, qt, ch->pcdata->quest_log->target_objs, target_vnum, target_vnum);

    if (qt == NULL)
        return 0;

    return qt;
}

QuestStatus* get_quest_status(CharData* ch, VNUM vnum)
{
    QuestStatus* qs = NULL;

    if (IS_NPC(ch) || !ch->pcdata)
        return NULL;

    ORDERED_GET(QuestStatus, qs, ch->pcdata->quest_log->quests, vnum, vnum);

    return qs;
}

static void remove_quest_target(CharData* ch, Quest* quest)
{
    QuestTarget* qt = NULL;
    switch (quest->type) {
    case QUEST_VISIT_MOB:
        UNORDERED_REMOVE(QuestTarget, qt, ch->pcdata->quest_log->target_mobs, quest_vnum, quest->vnum);
        if (qt)
            free_mem(qt, sizeof(QuestTarget));
        break;
    }
}

void finish_quest(CharData* ch, Quest* quest, QuestStatus* status)
{
    if (!ch || !ch->pcdata || !quest || !status)
        return;

    status->state = QSTAT_COMPLETE;
    printf_to_char(ch, "{_You have completed the quest, \"{*%s{_\".{x\n\r", quest->name);
        
    remove_quest_target(ch, quest);

    int xp;
    if (ch->level < quest->level || ch->level > quest->level + 10)
        xp = 0;
    else
        xp = quest->xp - (quest->xp * (ch->level - quest->level)) / 10;

    if (xp > 0) {
        printf_to_char(ch, "{_You have been awarded {*%d{_ xp.{x\n\r", xp);
        gain_exp(ch, xp);
    }
}

void grant_quest(CharData* ch, Quest* quest)
{
    static QuestStatus x_qs = { 0 };
    static QuestTarget x_qt = { 0 };

    if (!quest)
        return;

    QuestLog* qlog = ch->pcdata->quest_log;

    ALLOC(QuestStatus, qs);
    *qs = x_qs;
    qs->vnum = quest->vnum;
    qs->state = QSTAT_ACCEPTED;

    ORDERED_INSERT(QuestStatus, qs, qlog->quests, vnum);

    if (quest->target > 0) {
        ALLOC(QuestTarget, qt);
        *qt = x_qt;
        qt->quest_vnum = quest->vnum;
        qt->type = quest->type;
        qt->target_vnum = quest->target;
        qt->end_vnum = quest->end;

        switch (quest->type) {
        case QUEST_VISIT_MOB:
            ORDERED_INSERT(QuestTarget, qt, qlog->target_mobs, target_vnum);
            break;
        }
    }

    printf_to_char(ch, "{_You have started the quest, \"{*%s{_\".{x\n\r", quest->name);
}

void load_quest(FILE* fp)
{
    if (area_last == NULL) {
        bug("load_quest: no #AREA seen yet.", 0);
        exit(1);
    }

    Quest* quest = new_quest();
    if (quest == NULL) {
        perror("load_quest");
        exit(1);
    }

    load_struct(fp, U(&tmp_quest), quest_save_table, U(quest));
    quest->area = area_last;

    ORDERED_INSERT(Quest, quest, area_last->quests, vnum);

    return;
}

void save_quests(FILE* fp, AreaData* pArea)
{
    Quest* q;

    FOR_EACH(q, pArea->quests)
    {
        fprintf(fp, "#QUEST\n");
        save_struct(fp, U(&tmp_quest), quest_save_table, U(q));
        fprintf(fp, "#END\n\n");
    }
}

bool can_quest(CharData* ch, VNUM vnum)
{
    if (!ch->pcdata)
        return false;

    Quest* q = get_quest(vnum);
    if (!q)
        return false;

    if (ch->level < q->level)
        return false;

    return (get_quest_status(ch, vnum) == NULL);
}

bool has_quest(CharData* ch, VNUM vnum)
{
    if (!ch->pcdata)
        return false;

    QuestStatus* qs = get_quest_status(ch, vnum);

    if (!qs)
        return false;

    if (qs->state == QSTAT_COMPLETE)
        return false;

    return true;
}

bool can_finish_quest(CharData* ch, VNUM vnum)
{
    QuestStatus* qs = get_quest_status(ch, vnum);

    if (!qs || qs->state != QSTAT_ACCEPTED)
        return false;

    Quest* q = get_quest(vnum);

    if (!q)
        return false;

    switch (q->type) {
    case QUEST_VISIT_MOB:
    {
        CharData* vch;
        FOR_EACH_IN_ROOM(vch, ch->in_room->people) {
            if (vch->prototype && vch->prototype->vnum == q->target)
                return true;
        }
        break;
    }
    
    // End switch
    }

    return false;
}

void do_quest(CharData* ch, char* argument)
{
    if (!ch->pcdata)
        return;

    QuestStatus* qs = NULL;

    INIT_BUF(local, MSL);
    INIT_BUF(world, MSL);
    INIT_BUF(out, MSL);

    AreaData* area = ch->in_room ? ch->in_room->area : NULL;

    int i = 0;

    if (area != NULL) {
        FOR_EACH(qs, ch->pcdata->quest_log->quests) {
            if (qs->state != QSTAT_COMPLETE && qs->vnum >= area->min_vnum && qs->vnum <= area->max_vnum) {
                Quest* q = get_quest(qs->vnum);
                ++i;
                addf_buf(local, "%d. {T%s {|[{*Level %d{|]\n\r{_%s{x\n\r\n\r",
                    i, q->name, q->level, q->entry);
            }
        }
    }

    FOR_EACH(qs, ch->pcdata->quest_log->quests) {
        if (qs->state != QSTAT_COMPLETE && (!area || qs->vnum < area->min_vnum || qs->vnum > area->max_vnum)) {
            Quest* q = get_quest(qs->vnum);
            ++i;
            addf_buf(world, "%d. {T%s {|[{*Level %d{|] {_(%s){x\n\r",
                i, q->name, q->level, q->area->name);
        }
    }

    if (area != NULL && BUF(local)[0]) {
        addf_buf(out, "{*Active Quests in %s:\n\r\n\r", area->name);
        add_buf(out, BUF(local));
    }

    if (BUF(world)[0]) {
        add_buf(out, "{*Active World Quests:\n\r\n\r");
        add_buf(out, BUF(world));
    }

    if (!BUF(local)[0] && !BUF(world)) {
        send_to_char("You have no active quests.\n\r", ch);
    }
    
    page_to_char(BUF(out), ch);

    free_buf(local);
    free_buf(world);
    free_buf(out);
}
