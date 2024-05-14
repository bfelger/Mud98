////////////////////////////////////////////////////////////////////////////////
// quest.c
////////////////////////////////////////////////////////////////////////////////


#include "olc/olc.h"

#include "comm.h"
#include "db.h"
#include "save.h"
#include "tables.h"
#include "update.h"
#include "quest.h"

int quest_count = 0;
int quest_perm_count = 0;
Quest* quest_free = NULL;

const struct flag_type quest_type_table[] = {
    { "visit_mob",      QUEST_VISIT_MOB,    true    },
    { "kill_mob",       QUEST_KILL_MOB,     true    },
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
    { "xp",             FIELD_INT16,            U(&tmp_quest.xp),           0,                  0   },
    { "level",          FIELD_INT16,            U(&tmp_quest.level),        0,                  0   },
    { "end",            FIELD_VNUM,             U(&tmp_quest.end),          0,                  0   },
    { "target",         FIELD_VNUM,             U(&tmp_quest.target),       0,                  0   },
    { "upper",          FIELD_VNUM,             U(&tmp_quest.target_upper), 0,                  0   },
    { "count",          FIELD_INT16,            U(&tmp_quest.amount),       0,                  0   },
    { NULL,		        0,				        0,			                0,		            0   }
};

Quest* new_quest()
{
    LIST_ALLOC_PERM(quest, Quest);

    quest->name = &str_empty[0];
    quest->entry = &str_empty[0];

    return quest;
}

void free_quest(Quest* quest)
{
    free_string(quest->name);
    free_string(quest->entry);

    LIST_FREE(quest);
}

QuestLog* new_quest_log()
{
    static QuestLog ql_zero = { 0 };
    ALLOC(QuestLog, ql);
    *ql = ql_zero;
    ql->target_mobs = NULL;
    ql->target_objs = NULL;
    return ql;
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

    while (quest_log->target_ends) {
        t = quest_log->target_ends;
        NEXT_LINK(quest_log->target_ends);
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

QuestTarget* get_quest_targ_mob(Mobile* ch, VNUM target_vnum)
{
    QuestTarget* qt = NULL;

    if (IS_NPC(ch) || !ch->pcdata)
        return 0;

    {
        QuestTarget* temp_qt; 
        FOR_EACH(temp_qt, ch->pcdata->quest_log->target_mobs) {
            if (temp_qt->target_vnum == target_vnum 
                || (temp_qt->type == QUEST_KILL_MOB 
                    && temp_qt->target_upper > temp_qt->target_vnum
                    && target_vnum >= temp_qt->target_vnum
                    && target_vnum <= temp_qt->target_upper)) {
                qt = temp_qt; 
                break;
            }
            else if (temp_qt->target_vnum > target_vnum) 
                break;
        }
    };

    if (qt == NULL)
        return 0;

    return qt;
}

QuestTarget* get_quest_targ_obj(Mobile* ch, VNUM target_vnum)
{
    QuestTarget* qt = NULL;

    if (IS_NPC(ch) || !ch->pcdata)
        return 0;

    ORDERED_GET(QuestTarget, qt, ch->pcdata->quest_log->target_objs, target_vnum, target_vnum);

    if (qt == NULL)
        return 0;

    return qt;
}

QuestTarget* get_quest_targ_end(Mobile* ch, VNUM end_vnum)
{
    QuestTarget* qt = NULL;

    if (IS_NPC(ch) || !ch->pcdata)
        return 0;

    ORDERED_GET(QuestTarget, qt, ch->pcdata->quest_log->target_ends, target_vnum, end_vnum);

    if (qt == NULL)
        return 0;

    return qt;
}

QuestStatus* get_quest_status(Mobile* ch, VNUM quest_vnum)
{
    QuestStatus* qs = NULL;

    if (IS_NPC(ch) || !ch->pcdata)
        return NULL;

    ORDERED_GET(QuestStatus, qs, ch->pcdata->quest_log->quests, vnum, quest_vnum);

    return qs;
}

static void remove_quest_target(Mobile* ch, Quest* quest)
{
    QuestTarget* qt = NULL;
    switch (quest->type) {
    case QUEST_VISIT_MOB:
        UNORDERED_REMOVE(QuestTarget, qt, ch->pcdata->quest_log->target_mobs, quest_vnum, quest->vnum);
        if (qt)
            free_mem(qt, sizeof(QuestTarget));
        break;
    case QUEST_KILL_MOB:
        UNORDERED_REMOVE(QuestTarget, qt, ch->pcdata->quest_log->target_mobs, quest_vnum, quest->vnum);
        if (qt)
            free_mem(qt, sizeof(QuestTarget));
        break;
    }

    UNORDERED_REMOVE(QuestTarget, qt, ch->pcdata->quest_log->target_ends, quest_vnum, quest->vnum);
    if (qt)
        free_mem(qt, sizeof(QuestTarget));
}

void finish_quest(Mobile* ch, Quest* quest, QuestStatus* status)
{
    if (!ch || !ch->pcdata || !quest || !status)
        return;

    status->state = QSTAT_COMPLETE;
    printf_to_char(ch, "\n\r{jYou have completed the quest, \"{*%s{j\".{x\n\r", quest->name);
        
    remove_quest_target(ch, quest);

    int xp;
    if (ch->level < quest->level || ch->level > quest->level + 10)
        xp = 0;
    else
        xp = quest->xp - (quest->xp * (ch->level - quest->level)) / 10;

    if (xp > 0) {
        printf_to_char(ch, "{jYou have been awarded {*%d{j xp.{x\n\r", xp);
        gain_exp(ch, xp);
    }
    send_to_char("\n\r", ch);

    save_char_obj(ch);
}
static QuestTarget x_qt = { 0 };

static void add_target(QuestLog* qlog, Quest* quest, VNUM target_vnum)
{
    ALLOC(QuestTarget, qt);
    *qt = x_qt;
    qt->quest_vnum = quest->vnum;
    qt->type = quest->type;
    qt->target_vnum = target_vnum;

    switch (quest->type) {
    case QUEST_VISIT_MOB:
    case QUEST_KILL_MOB:
        ORDERED_INSERT(QuestTarget, qt, qlog->target_mobs, target_vnum);
        break;
    }
}

void add_quest_to_log(QuestLog* quest_log, Quest* quest, QuestState quest_state, int progress)
{
    static QuestStatus x_qs = { 0 };

    ALLOC(QuestStatus, qs);
    *qs = x_qs;
    qs->vnum = quest->vnum;
    qs->quest = quest;
    qs->amount = quest->amount;
    qs->progress = progress;
    qs->state = quest_state;
    ORDERED_INSERT(QuestStatus, qs, quest_log->quests, vnum);

    if (quest_state != QSTAT_COMPLETE) {
        ALLOC(QuestTarget, qe);
        *qe = x_qt;
        qe->quest_vnum = quest->vnum;
        qe->type = quest->type;
        qe->target_vnum = quest->end;
        ORDERED_INSERT(QuestTarget, qe, quest_log->target_ends, target_vnum);

        if (quest->target > 0) {
            if (quest->target_upper > quest->target) {
                for (VNUM i = quest->target; i <= quest->target_upper; ++i)
                    add_target(quest_log, quest, i);
            }
        }
    }
}

void grant_quest(Mobile* ch, Quest* quest)
{
    if (!quest)
        return;

    add_quest_to_log(ch->pcdata->quest_log, quest, QSTAT_ACCEPTED, 0);

    save_char_obj(ch);

    printf_to_char(ch, "\n\r{jYou have started the quest, \"{*%s{j\".{x\n\r", quest->name);
}

void load_quest(FILE* fp)
{
    if (global_areas.count == 0) {
        bug("load_quest: no #AREA seen yet.", 0);
        exit(1);
    }

    Quest* quest = new_quest();
    if (quest == NULL) {
        perror("load_quest");
        exit(1);
    }

    load_struct(fp, U(&tmp_quest), quest_save_table, U(quest));
    quest->area_data = LAST_AREA_DATA;

    ORDERED_INSERT(Quest, quest, LAST_AREA_DATA->quests, vnum);

    return;
}

void save_quests(FILE* fp, AreaData* area_data)
{
    Quest* q;

    FOR_EACH(q, area_data->quests) {
        fprintf(fp, "#QUEST\n");
        save_struct(fp, U(&tmp_quest), quest_save_table, U(q));
        fprintf(fp, "#END\n\n");
    }
}

bool can_quest(Mobile* ch, VNUM vnum)
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

bool has_quest(Mobile* ch, VNUM vnum)
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

bool can_finish_quest(Mobile* ch, VNUM vnum)
{
    if (!ch->pcdata)
        return false;

    QuestStatus* qs = get_quest_status(ch, vnum);

    if (!qs || qs->state != QSTAT_ACCEPTED)
        return false;

    Quest* q = get_quest(vnum);

    if (!q)
        return false;

    switch (q->type) {
    case QUEST_VISIT_MOB: {
        Mobile* vch;
        FOR_EACH_IN_ROOM(vch, ch->in_room->people) {
            if (vch->prototype && vch->prototype->vnum == q->target)
                return true;
        }
        break;
    }
    case QUEST_KILL_MOB:
        return ( qs->progress >= qs->amount);
    
    // End switch
    }

    return false;
}

void do_quest(Mobile* ch, char* argument)
{
    if (!ch->pcdata)
        return;

    QuestStatus* qs = NULL;

    INIT_BUF(local, MSL);
    INIT_BUF(world, MSL);
    INIT_BUF(out, MSL);

    AreaData* area = ch->in_room ? ch->in_room->area->data : NULL;

    int i = 0;

    if (area != NULL) {
        FOR_EACH(qs, ch->pcdata->quest_log->quests) {
            if (qs->state != QSTAT_COMPLETE && qs->vnum >= area->min_vnum && qs->vnum <= area->max_vnum) {
                Quest* q = get_quest(qs->vnum);
                ++i;
                addf_buf(local, "%d. {T%s {|[{*Level %d{|] ",
                    i, q->name, q->level);
                if (qs->amount > 0) {
                    addf_buf(local, "{j(%d/%d){x\n\r", qs->progress, qs->amount);
                }
                addf_buf(local, "{_%s{x\n\r\n\r", q->entry);
            }
        }
    }

    FOR_EACH(qs, ch->pcdata->quest_log->quests) {
        if (qs->state != QSTAT_COMPLETE && (!area || qs->vnum < area->min_vnum || qs->vnum > area->max_vnum)) {
            Quest* q = get_quest(qs->vnum);
            ++i;
            addf_buf(world, "%d. {T%s {|[{*Level %d{|] {_(%s){x",
                i, q->name, q->level, NAME_STR(q->area_data));                
            if (qs->amount > 0) {
                addf_buf(local, " {j(%d/%d){x\n\r", qs->progress, qs->amount);
            }
            add_buf(local, "\n\r");
        }
    }

    if (area != NULL && BUF(local)[0]) {
        addf_buf(out, "{*Active Quests in %s:\n\r\n\r", NAME_STR(area));
        add_buf(out, BUF(local));
    }

    if (BUF(world)[0]) {
        add_buf(out, "{*Active World Quests:\n\r\n\r");
        add_buf(out, BUF(world));
    }

    if (!BUF(local)[0] && !BUF(world)[0]) {
        send_to_char("You have no active quests.\n\r", ch);
    }
    
    page_to_char(BUF(out), ch);

    free_buf(local);
    free_buf(world);
    free_buf(out);
}
