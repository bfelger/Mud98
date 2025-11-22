////////////////////////////////////////////////////////////////////////////////
// data/quest.c
////////////////////////////////////////////////////////////////////////////////

#include "quest.h"

#include <olc/olc.h>

#include <comm.h>
#include <db.h>
#include <handler.h>
#include <save.h>
#include <tables.h>
#include <update.h>

#include <string.h>

#include <entities/faction.h>
#include <entities/object.h>
#include <entities/obj_prototype.h>

int quest_count = 0;
int quest_perm_count = 0;
Quest* quest_free = NULL;

const struct flag_type quest_type_table[] = {
    { "visit_mob",      QUEST_VISIT_MOB,    true    },
    { "kill_mob",       QUEST_KILL_MOB,     true    },
    { "get_obj",        QUEST_GET_OBJ,      true    },
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
    { "reward_faction", FIELD_VNUM,             U(&tmp_quest.reward_faction_vnum), 0,           0   },
    { "reward_reputation", FIELD_INT16,         U(&tmp_quest.reward_reputation),  0,           0   },
    { "reward_gold",    FIELD_INT16,            U(&tmp_quest.reward_gold),  0,                  0   },
    { "reward_silver",  FIELD_INT16,            U(&tmp_quest.reward_silver),0,                  0   },
    { "reward_copper",  FIELD_INT16,            U(&tmp_quest.reward_copper),0,                  0   },
    { "reward_objs",    FIELD_VNUM_ARRAY,       U(&tmp_quest.reward_obj_vnum),   U(QUEST_MAX_REWARD_ITEMS), 0 },
    { "reward_counts",  FIELD_INT16_ARRAY,      U(&tmp_quest.reward_obj_count), U(QUEST_MAX_REWARD_ITEMS), 0 },
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
    ALLOC(QuestLog, ql);
    memset(ql, 0, sizeof(QuestLog));
    ql->target_ends = NULL;
    ql->target_mobs = NULL;
    ql->target_objs = NULL;
    ql->quests = NULL;
    return ql;
}

void free_quest_log(PlayerData* pc)
{
    QuestLog* quest_log = pc->quest_log;
    QuestTarget* t;
    QuestStatus* s;

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

    while (quest_log->quests) {
        s = quest_log->quests;
        NEXT_LINK(quest_log->quests);
        free_mem(s, sizeof(QuestStatus));
    }

    free_mem(quest_log, sizeof(QuestLog));
    pc->quest_log = NULL;
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

    QuestTarget* temp_qt;
    FOR_EACH(temp_qt, ch->pcdata->quest_log->target_objs)
    {
        if (temp_qt->target_vnum == target_vnum
            || (temp_qt->type == QUEST_GET_OBJ
                && temp_qt->target_upper > temp_qt->target_vnum
                && target_vnum >= temp_qt->target_vnum
                && target_vnum <= temp_qt->target_upper)) {
            qt = temp_qt;
            break;
        }
        else if (temp_qt->target_vnum > target_vnum)
            break;
    }

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
    case QUEST_KILL_MOB:
        UNORDERED_REMOVE_ALL(QuestTarget, qt, ch->pcdata->quest_log->target_mobs, quest_vnum, 
            quest->vnum);
        if (qt)
            free_mem(qt, sizeof(QuestTarget));
        break;
    case QUEST_GET_OBJ:
        UNORDERED_REMOVE_ALL(QuestTarget, qt, ch->pcdata->quest_log->target_objs, quest_vnum, 
            quest->vnum);
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
    printf_to_char(ch, "\n\r" COLOR_INFO "You have completed the quest, \"" COLOR_ALT_TEXT_1 "%s" COLOR_INFO "\"." COLOR_EOL, quest->name);
        
    remove_quest_target(ch, quest);

    int xp;
    if (ch->level < quest->level || ch->level > quest->level + 10)
        xp = 0;
    else
        xp = quest->xp - (quest->xp * (ch->level - quest->level)) / 10;

    if (xp > 0) {
        printf_to_char(ch, COLOR_INFO "You have been awarded " COLOR_ALT_TEXT_1 "%d" COLOR_INFO " xp." COLOR_EOL, xp);
        gain_exp(ch, xp);
    }

    if (quest->reward_gold > 0 || quest->reward_silver > 0 || quest->reward_copper > 0) {
        ch->gold += quest->reward_gold;
        ch->silver += quest->reward_silver;
        ch->copper += quest->reward_copper;

        char reward_buf[MAX_INPUT_LENGTH] = { 0 };
        bool first = true;
        if (quest->reward_gold > 0) {
            sprintf(reward_buf + strlen(reward_buf), "%s%d gold", first ? "" : ", ",
                quest->reward_gold);
            first = false;
        }
        if (quest->reward_silver > 0) {
            sprintf(reward_buf + strlen(reward_buf), "%s%d silver", first ? "" : ", ",
                quest->reward_silver);
            first = false;
        }
        if (quest->reward_copper > 0) {
            sprintf(reward_buf + strlen(reward_buf), "%s%d copper", first ? "" : ", ",
                quest->reward_copper);
        }

        printf_to_char(ch, COLOR_INFO "You receive " COLOR_ALT_TEXT_1 "%s" COLOR_INFO "." COLOR_EOL,
            reward_buf[0] ? reward_buf : "your coin reward");
    }

    if (quest->reward_faction_vnum != 0 && quest->reward_reputation != 0) {
        Faction* faction = get_faction(quest->reward_faction_vnum);
        if (faction != NULL)
            faction_adjust(ch, faction, quest->reward_reputation);
        else
            bugf("finish_quest: missing faction %" PRVNUM " for quest %" PRVNUM,
                quest->reward_faction_vnum, quest->vnum);
    }

    for (int i = 0; i < QUEST_MAX_REWARD_ITEMS; ++i) {
        VNUM vnum = quest->reward_obj_vnum[i];
        int count = quest->reward_obj_count[i];
        if (vnum <= 0 || count <= 0)
            continue;

        ObjPrototype* proto = get_object_prototype(vnum);
        if (proto == NULL) {
            bugf("finish_quest: missing reward object %" PRVNUM " for quest %" PRVNUM,
                vnum, quest->vnum);
            continue;
        }

        for (int j = 0; j < count; ++j) {
            Object* reward_obj = create_object(proto, proto->level);
            obj_to_char(reward_obj, ch);
        }

        printf_to_char(ch,
            COLOR_INFO "You receive " COLOR_ALT_TEXT_1 "%d" COLOR_INFO " x "
            COLOR_ALT_TEXT_2 "%s" COLOR_INFO "." COLOR_EOL,
            count, proto->short_descr);
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
    case QUEST_GET_OBJ:
        ORDERED_INSERT(QuestTarget, qt, qlog->target_objs, target_vnum);
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

    printf_to_char(ch, "\n\r" COLOR_INFO "You have started the quest, " COLOR_ALT_TEXT_1 "\"%s" COLOR_INFO "\"." COLOR_EOL, quest->name);
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
        FOR_EACH_ROOM_MOB(vch, ch->in_room) {
            if (vch->prototype && VNUM_FIELD(vch->prototype) == q->target)
                return true;
        }
        break;
    }
    case QUEST_KILL_MOB:
    case QUEST_GET_OBJ:
        return ( qs->progress >= qs->amount);
    
    // End switch
    }

    return false;
}

void do_quest(Mobile* ch, char* argument)
{
    static const char* help =
        COLOR_INFO
        "USAGE: " COLOR_ALT_TEXT_1 "QUEST " COLOR_INFO "              - Show all active quests.\n\r"
        "       " COLOR_ALT_TEXT_1 "QUEST AREA" COLOR_INFO "          - Show only quests in this area.\n\r"
        "       " COLOR_ALT_TEXT_1 "QUEST COMPLETED" COLOR_INFO "     - Show completed quests.\n\r"
        "       " COLOR_ALT_TEXT_1 "QUEST HELP" COLOR_INFO "          - Show this help." COLOR_EOL;
    static const char* tester_help =
        COLOR_INFO
        "The following commands are also available to you as a tester:\n\r"
        "       " COLOR_ALT_TEXT_1 "QUEST SHOW <vnum>" COLOR_INFO "         - Show quest info for specific quest.\n\r"
        "       " COLOR_ALT_TEXT_1 "QUEST GRANT <vnum>" COLOR_INFO "        - Grant yourself a specific quest.\n\r"
        "       " COLOR_ALT_TEXT_1 "QUEST PROGRESS <vnum> <#>" COLOR_INFO " - Set progress for a specific quest.\n\r"
        "       " COLOR_ALT_TEXT_1 "QUEST FINISH <vnum>" COLOR_INFO "       - Finish a specific quest.\n\r"
        "       " COLOR_ALT_TEXT_1 "QUEST CLEAR <vnum>" COLOR_INFO "        - Clear a specific quest from your log.\n\r"
        "       " COLOR_ALT_TEXT_1 "QUEST CLEAR ALL" COLOR_INFO "           - Clear your entire quest log." COLOR_EOL; 

    char arg1[MAX_INPUT_LENGTH] = { 0 };
    char arg2[MAX_INPUT_LENGTH] = { 0 };

    bool area_only = false;
    bool complete_only = false;

    if (!ch->pcdata)
        return;

    bool is_tester = IS_TESTER(ch);

    if (argument[0] != '\0') {
        READ_ARG(arg1);

        if (!str_prefix(arg1, "help")) {
            send_to_char(help, ch);
            if (IS_TESTER(ch))
                send_to_char(tester_help, ch);
            return;
        }
        else if (!str_prefix(arg1, "area")) {
            area_only = true;
        }
        else if (!str_prefix(arg1, "completed")) {
            complete_only = true;
        }
        else if (is_tester && !str_prefix(arg1, "show")) {
            READ_ARG(arg2);
            if (!is_number(arg2)) {
                send_to_char("You must provide a quest vnum.\n\r", ch);
                return;
            }
            VNUM vnum = atoi(arg2);
            Quest* q = get_quest(vnum);
            if (!q) {
                send_to_char("No such quest exists.\n\r", ch);
                return;
            }
            printf_to_char(ch, COLOR_DECOR_1 "[" COLOR_ALT_TEXT_2 "#%d" COLOR_DECOR_1 "] " COLOR_TITLE "%s " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "Level %d" COLOR_DECOR_1 "] " COLOR_ALT_TEXT_2 "(%s)" COLOR_EOL,
                     q->vnum, q->name, q->level, NAME_STR(q->area_data));
            printf_to_char(ch, COLOR_ALT_TEXT_2 "%s" COLOR_EOL, q->entry);
        }
        else if (is_tester && !str_prefix(arg1, "grant")) {
            READ_ARG(arg2);
            if (!is_number(arg2)) {
                send_to_char("You must provide a quest vnum.\n\r", ch);
                return;
            }
            VNUM vnum = atoi(arg2);
            Quest* q = get_quest(vnum);
            if (!q) {
                send_to_char("No such quest exists.\n\r", ch);
                return;
            }
            grant_quest(ch, q);
        }
        else if (is_tester && !str_prefix(arg1, "progress")) {
            READ_ARG(arg2);
            char arg3[MAX_INPUT_LENGTH] = { 0 };
            READ_ARG(arg3);
            if (!is_number(arg2) || !is_number(arg3)) {
                send_to_char("You must provide a quest vnum and progress amount.\n\r", ch);
                return;
            }
            VNUM vnum = atoi(arg2);
            int progress = atoi(arg3);
            Quest* q = get_quest(vnum);
            QuestStatus* qs = get_quest_status(ch, vnum);
            if (!q || !qs) {
                send_to_char("No such quest exists in your log.\n\r", ch);
                return;
            }
            qs->progress = progress;
            printf_to_char(ch, "Set progress of quest #%d to %d.\n\r", vnum, progress);
            save_char_obj(ch);
        }
        else if (is_tester && !str_prefix(arg1, "finish")) {
            READ_ARG(arg2);
            if (!is_number(arg2)) {
                send_to_char("You must provide a quest vnum.\n\r", ch);
                return;
            }
            VNUM vnum = atoi(arg2);
            Quest* q = get_quest(vnum);
            QuestStatus* qs = get_quest_status(ch, vnum);
            if (!q || !qs) {
                send_to_char("No such quest exists in your log.\n\r", ch);
                return;
            }
            finish_quest(ch, q, qs);
        }
        else if (is_tester && !str_prefix(arg1, "clear")) {
            READ_ARG(arg2);
            if (str_cmp(arg2, "all") == 0) {
                free_quest_log(ch->pcdata);
                ch->pcdata->quest_log = new_quest_log();
                send_to_char("Your quest log has been cleared.\n\r", ch);
                save_char_obj(ch);
                return;
            }
            if (!is_number(arg2)) {
                send_to_char("You must provide a quest vnum or ALL.\n\r", ch);
                return;
            }
            VNUM vnum = atoi(arg2);
            QuestStatus* qs = get_quest_status(ch, vnum);
            if (!qs) {
                send_to_char("No such quest exists in your log.\n\r", ch);
                return;
            }
            remove_quest_target(ch, qs->quest);
            UNORDERED_REMOVE(QuestStatus, qs, ch->pcdata->quest_log->quests, vnum, vnum);
            free_mem(qs, sizeof(QuestStatus));
            send_to_char("Quest removed from your log.\n\r", ch);
            save_char_obj(ch);
        }
        else {
            printf_to_char(ch, "Unknown quest argument: %s\n\r", arg1);
            send_to_char(help, ch);
            if (IS_TESTER(ch))
                send_to_char(tester_help, ch);
            return;
        }
    }

    QuestStatus* qs = NULL;

    INIT_BUF(out, MSL);

    AreaData* area = ch->in_room ? ch->in_room->area->data : NULL;

    int i = 0;

    if (complete_only) {
        INIT_BUF(complete, MSL);

        FOR_EACH(qs, ch->pcdata->quest_log->quests)
        {
            if (qs->state == QSTAT_COMPLETE) {
                Quest* q = get_quest(qs->vnum);
                ++i;
                if (is_tester)
                    addf_buf(complete, 
                        "%d. " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_2 "#%d" COLOR_DECOR_1 "]  " COLOR_TITLE "%s " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "Level %d" COLOR_DECOR_1 "] " COLOR_ALT_TEXT_2 "(%s)" COLOR_EOL,
                        i, q->vnum, q->name, q->level, NAME_STR(q->area_data));
                else
                    addf_buf(complete, "%d. " COLOR_TITLE "%s " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "Level %d" COLOR_DECOR_1 "] " COLOR_ALT_TEXT_2 "(%s)" COLOR_EOL,
                        i, q->name, q->level, NAME_STR(q->area_data));
            }
        }

        if (BUF(complete)[0]) {
            add_buf(out, COLOR_ALT_TEXT_1 "Completed Quests:" COLOR_EOL);
            add_buf(out, BUF(complete));
        }

        free_buf(complete);
    }

    if (area != NULL) {
        INIT_BUF(local, MSL);

        FOR_EACH(qs, ch->pcdata->quest_log->quests) {
            if (qs->state != QSTAT_COMPLETE && qs->vnum >= area->min_vnum && qs->vnum <= area->max_vnum) {
                Quest* q = get_quest(qs->vnum);
                ++i;
                if (is_tester)
                    addf_buf(local, "%d. " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_2 "#%d" COLOR_DECOR_1 "] " COLOR_TITLE "%s " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "Level %d" COLOR_DECOR_1 "] ",
                        i, q->vnum, q->name, q->level);
                else
                    addf_buf(local, "%d. " COLOR_TITLE "%s " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "Level %d" COLOR_DECOR_1 "] ",
                        i, q->name, q->level);


                if (qs->amount > 0) {
                    addf_buf(local, COLOR_INFO "(%d/%d)" COLOR_CLEAR , qs->progress, qs->amount);
                }
                addf_buf(local, "\n\r" COLOR_ALT_TEXT_2 "%s" COLOR_CLEAR "\n\r\n\r", q->entry);
            }
        }

        if (area != NULL && BUF(local)[0]) {
            addf_buf(out, COLOR_ALT_TEXT_1 "Active Quests in %s:" COLOR_EOL, NAME_STR(area));
            add_buf(out, BUF(local));
        }

        free_buf(local);
    }

    if (!area_only) {
        INIT_BUF(world, MSL);

        FOR_EACH(qs, ch->pcdata->quest_log->quests)
        {
            if (qs->state != QSTAT_COMPLETE && (!area || qs->vnum < area->min_vnum || qs->vnum > area->max_vnum)) {
                Quest* q = get_quest(qs->vnum);
                ++i;
                if (is_tester)
                    addf_buf(world, "%d. " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_2 "#%d" COLOR_DECOR_1 "] " COLOR_TITLE "%s " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "Level %d" COLOR_DECOR_1 "] " COLOR_ALT_TEXT_2 "(%s)" COLOR_CLEAR ,
                        i, q->vnum, q->name, q->level, NAME_STR(q->area_data));
                else
                    addf_buf(world, "%d. " COLOR_TITLE "%s " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "Level %d" COLOR_DECOR_1 "] " COLOR_ALT_TEXT_2 "(%s)" COLOR_CLEAR ,
                        i, q->name, q->level, NAME_STR(q->area_data));
                if (qs->amount > 0) {
                    addf_buf(world, " " COLOR_INFO "(%d/%d)" COLOR_EOL, qs->progress, qs->amount);
                }
                add_buf(world, "\n\r");
            }
        }

        if (BUF(world)[0]) {
            add_buf(out, COLOR_ALT_TEXT_1 "Active World Quests:" COLOR_EOL);
            add_buf(out, BUF(world));
        }

        free_buf(world);
    }

    if (!BUF(out)[0]) {
        if (complete_only)
            send_to_char("You have no completed quests.\n\r", ch);
        else
            send_to_char("You have no active quests.\n\r", ch);
    }
    
    page_to_char(BUF(out), ch);

    free_buf(out);
}

// LOX METHODS /////////////////////////////////////////////////////////////////

Value can_quest_lox(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 1 || !IS_INT(args[0])) {
        runtime_error("can_quest() takes an integer argument.");
        return FALSE_VAL;
    }

    if (!IS_MOBILE(receiver))
        return FALSE_VAL;

    int32_t vnum = AS_INT(args[0]);

    bool ret = can_quest(AS_MOBILE(receiver), vnum);

    return BOOL_VAL(ret);
}

Value has_quest_lox(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 1 || !IS_INT(args[0])) {
        runtime_error("has_quest() takes an integer argument.");
        return FALSE_VAL;
    }

    if (!IS_MOBILE(receiver))
        return FALSE_VAL;

    int32_t vnum = AS_INT(args[0]);

    bool ret = has_quest(AS_MOBILE(receiver), vnum);

    return BOOL_VAL(ret);
}

Value grant_quest_lox(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 1 || !IS_INT(args[0])) {
        runtime_error("grant_quest() takes an integer argument.");
        return FALSE_VAL;
    }

    if (!IS_MOBILE(receiver))
        return FALSE_VAL;

    int32_t vnum = AS_INT(args[0]);

    Quest* q = get_quest(vnum);
    if (q == NULL) {
        runtime_error("grant_quest(): No quest for VNUM %d.", vnum);
        return FALSE_VAL;
    }

    grant_quest(AS_MOBILE(receiver), q);

    return TRUE_VAL;
}

Value can_finish_quest_lox(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 1 || !IS_INT(args[0])) {
        runtime_error("can_finish_quest() takes an integer argument.");
        return FALSE_VAL;
    }

    if (!IS_MOBILE(receiver))
        return FALSE_VAL;

    int32_t vnum = AS_INT(args[0]);

    bool ret = can_finish_quest(AS_MOBILE(receiver), vnum);

    return BOOL_VAL(ret);
}

Value finish_quest_lox(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 1 || !IS_INT(args[0])) {
        runtime_error("finish_quest() takes an integer argument.");
        return FALSE_VAL;
    }

    if (!IS_MOBILE(receiver))
        return FALSE_VAL;

    int32_t vnum = AS_INT(args[0]);
    Mobile* ch = AS_MOBILE(receiver);

    Quest* q = get_quest(vnum);
    if (q == NULL) {
        runtime_error("finish_quest(): No quest for VNUM %d.", vnum);
        return FALSE_VAL;
    }

    QuestStatus* qs = get_quest_status(ch, vnum);
    if (qs == NULL) {
        runtime_error("finish_quest(): Couldn't get quest status for VNUM %d.", vnum);
        return FALSE_VAL;
    }

    finish_quest(ch, q, qs);

    return TRUE_VAL;
}
