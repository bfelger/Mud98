////////////////////////////////////////////////////////////////////////////////
// qedit.c
////////////////////////////////////////////////////////////////////////////////

#include <merc.h>

#include "olc.h"

#include <entities/faction.h>
#include <entities/obj_prototype.h>
#include <entities/player_data.h>

#include <data/quest.h>

#include <comm.h>
#include <db.h>
#include <handler.h>
#include <magic.h>
#include <skills.h>

#define QEDIT(fun) bool fun(Mobile *ch, char *argument)

Quest xQuest;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const OlcCmdEntry quest_olc_comm_table[] =
{
    { "create",     0,                      ed_olded,           U(qedit_create)     },
    { "type",       U(&xQuest.type),        ed_flag_set_sh,     U(quest_type_table) },
    { "xp",         U(&xQuest.xp),          ed_number_pos,      0                   },
    { "level",      U(&xQuest.level),       ed_number_level,    0                   },
    { "target",     U(&xQuest.target),      ed_olded,           U(qedit_target)     },
    { "upper",      U(&xQuest.target_upper),ed_olded,           U(qedit_upper)      },
    { "amount",     U(&xQuest.amount),      ed_number_s_pos,    0                   },
    { "end",        U(&xQuest.end),         ed_number_pos,      U(qedit_end)        },
    { "faction",    0,                      ed_olded,           U(qedit_faction)    },
    { "rewardrep",      0,                  ed_olded,           U(qedit_reward_reputation) },
    { "rewardgold", U(&xQuest.reward_gold),   ed_number_s_pos,  0                   },
    { "rewardsilver", U(&xQuest.reward_silver), ed_number_s_pos,0                   },
    { "rewardcopper", U(&xQuest.reward_copper), ed_number_s_pos,0                   },
    { "rewarditem",    0,                  ed_olded,           U(qedit_reward_item) },
    { "name",       U(&xQuest.name),        ed_line_string,     0                   },
    { "entry",      U(&xQuest.entry),       ed_desc,            0                   },
    { "show",       0,                      ed_olded,           U(qedit_show)       },
    { "commands",   0,                      ed_olded,           U(show_commands)    },
    { "?",          0,                      ed_olded,           U(show_help)        },
    { "version",    0,                      ed_olded,           U(show_version)     },
    { NULL,         0,                      0,                  0                   }
};

void qedit(Mobile* ch, char* argument)
{
    if (ch->pcdata->security < MIN_QEDIT_SECURITY) {
        send_to_char(COLOR_INFO "You do not have enough security to edit quests." COLOR_EOL, ch);
        edit_done(ch);
        return;
    }

    if (emptystring(argument)) {
        qedit_show(ch, argument);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (!process_olc_command(ch, argument, quest_olc_comm_table))
        interpret(ch, argument);

    return;
}

void do_qedit(Mobile* ch, char* argument)
{
    Quest* quest;
    AreaData* area_data;
    char command[MAX_STRING_LENGTH];
    VNUM  vnum;

    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    READ_ARG(command);

    if (is_number(command)) {
        vnum = (VNUM)atoi(command);

        if ((area_data = get_vnum_area(vnum)) == NULL) {
            send_to_char(COLOR_INFO "That VNUM is not assigned to an area." COLOR_EOL, ch);
            return;
        }

        if (!IS_BUILDER(ch, area_data)) {
            send_to_char(COLOR_INFO "You do not have enough security to edit quests in that area." COLOR_EOL, ch);
            return;
        }

        if (!(quest = get_quest(vnum))) {
            send_to_char(COLOR_INFO "That VNUM does not exist." COLOR_EOL, ch);
            return;
        }

        set_editor(ch->desc, ED_QUEST, U(quest));
        qedit_show(ch, "");
        return;
    }

    if (!str_cmp(command, "create")) {
        if (argument[0] == '\0') {
            send_to_char(COLOR_INFO "Syntax: " COLOR_ALT_TEXT_1 "QEDIT CREATE [VNUM]" COLOR_EOL, ch);
            return;
        }

        if (qedit_create(ch, argument))
            qedit_show(ch, "");
        return;
    }

    send_to_char(
        COLOR_INFO "Syntax: " COLOR_ALT_TEXT_1 "QEDIT [VNUM]\n\r"
        "        QEDIT CREATE [VNUM]" COLOR_EOL, ch);
    return;
}

QEDIT(qedit_create)
{
    Quest* quest;
    AreaData* area_data;

    VNUM vnum = (VNUM)atoi(argument);

    if (argument[0] == '\0' || vnum == 0) {
        send_to_char(COLOR_INFO "Syntax: " COLOR_ALT_TEXT_1 "QEDIT CREATE [VNUM]" COLOR_EOL, ch);
        return false;
    }

    if (get_quest(vnum)) {
        send_to_char(COLOR_INFO "That quest already exists." COLOR_EOL, ch);
        return false;
    }

    if ((area_data = get_vnum_area(vnum)) == NULL) {
        send_to_char(COLOR_INFO "That vnum has not been assigned to an area." COLOR_EOL, ch);
        return false;
    }

    if (!IS_BUILDER(ch, area_data)) {
        send_to_char(COLOR_INFO "You do not have access to this area." COLOR_EOL, ch);
        return false;
    }

    if (ch->pcdata->security < MIN_QEDIT_SECURITY) {
        send_to_char(COLOR_INFO "You do not have enough security to create quests." COLOR_EOL, ch);
        return false;
    }

    quest = new_quest();
    quest->vnum = vnum;
    quest->area_data = area_data;
    quest->level = area_data->low_range;

    ORDERED_INSERT(Quest, quest, area_data->quests, vnum);

    set_editor(ch->desc, ED_QUEST, U(quest));

    send_to_char(COLOR_INFO "New quest created." COLOR_EOL, ch);

    return true;
}

static const char* get_targ_name(Quest* quest, VNUM vnum)
{
    MobPrototype* mob;
    ObjPrototype* obj;
    char* target_name = NULL;

    if (vnum > 0) {
        switch (quest->type) {
        case QUEST_VISIT_MOB:
        case QUEST_KILL_MOB:
            if (!(mob = get_mob_prototype(vnum)))
                target_name = "(invalid)";  // Quest type may have changed
            else
                target_name = mob->short_descr;
            break;
        case QUEST_GET_OBJ:
            if (!(obj = get_object_prototype(vnum)))
                target_name = "(invalid)";  // Quest type may have changed
            else
                target_name = obj->short_descr;
            break;
        }
    }
    else {
        target_name = "(none)";
    }

    return target_name;
}

QEDIT(qedit_show)
{
    Quest* quest;
    MobPrototype* end_mob;

    EDIT_QUEST(ch, quest);

    char* end_name = NULL;
    if (quest->end > 0) {
        if (!(end_mob = get_mob_prototype(quest->end)))
            end_name = "(invalid)";  // Quest type may have changed
        else
            end_name = end_mob->short_descr;
    }
    else {
        end_name = "(none)";
    }

    olc_print_num_str(ch, "Quest", quest->vnum, quest->name && quest->name[0] ? quest->name : "(none)");
    olc_print_str(ch, "Area", NAME_STR(quest->area_data));
    olc_print_flags(ch, "Type", quest_type_table, quest->type);
    olc_print_num(ch, "Level", quest->level);
    olc_print_num_str(ch, "End", quest->end, end_name);
    olc_print_num_str(ch, "Target", quest->target, get_targ_name(quest, quest->target));
    
    if (quest->type == QUEST_KILL_MOB) {
        if (quest->target_upper <= quest->target)
            olc_print_num_str(ch, "Upper", 0, COLOR_INFO "(set " COLOR_ALT_TEXT_1 "UPPER" COLOR_INFO " to use a range of VNUMs)" COLOR_CLEAR );
        else {
            MobPrototype* targ_mob;
            for (VNUM i = quest->target+1; i < quest->target_upper; ++i)
                if ((targ_mob = get_mob_prototype(i)) != NULL)
                    olc_print_num_str(ch, "", VNUM_FIELD(targ_mob), targ_mob->short_descr);
            if ((targ_mob = get_mob_prototype(quest->target_upper)) != NULL)
                olc_print_num_str(ch, "Upper", quest->target_upper, targ_mob->short_descr);
            else
                olc_print_num(ch, "Upper", quest->target_upper);
        }
        olc_print_num(ch, "Amount", quest->amount);
    }

    olc_print_num(ch, "XP", quest->xp);

    if (quest->reward_gold > 0 || quest->reward_silver > 0 || quest->reward_copper > 0) {
        char buf[MAX_INPUT_LENGTH] = { 0 };
        bool first = true;
        if (quest->reward_gold > 0) {
            sprintf(buf + strlen(buf), "%s%d gold", first ? "" : ", ", quest->reward_gold);
            first = false;
        }
        if (quest->reward_silver > 0) {
            sprintf(buf + strlen(buf), "%s%d silver", first ? "" : ", ", quest->reward_silver);
            first = false;
        }
        if (quest->reward_copper > 0)
            sprintf(buf + strlen(buf), "%s%d copper", first ? "" : ", ", quest->reward_copper);
        olc_print_str(ch, "Currency", buf);
    }
    else {
        olc_print_str(ch, "Currency", "(none)");
    }

    if (quest->reward_faction_vnum != 0) {
        Faction* faction = get_faction(quest->reward_faction_vnum);
        const char* faction_name = faction ? NAME_STR(faction) : "(invalid)";
        olc_print_num_str(ch, "Faction", quest->reward_faction_vnum, faction_name);
    }
    else {
        olc_print_str_box(ch, "Faction", "(none)", NULL);
        olc_print_num(ch, "Reputation", quest->reward_reputation);
    }


    bool has_item_reward = false;
    for (int i = 0; i < QUEST_MAX_REWARD_ITEMS; ++i) {
        if (quest->reward_obj_vnum[i] <= 0 || quest->reward_obj_count[i] <= 0)
            continue;

        char label[32];
        sprintf(label, "Reward Item %d", i + 1);

        ObjPrototype* obj = get_object_prototype(quest->reward_obj_vnum[i]);
        const char* obj_name = obj ? obj->short_descr : "(invalid)";
        char line[MAX_INPUT_LENGTH];
        sprintf(line, "%d x %s", quest->reward_obj_count[i], obj_name);

        olc_print_num_str(ch, label, quest->reward_obj_vnum[i], line);
        has_item_reward = true;
    }
    if (!has_item_reward)
        olc_print_str_box(ch, "Reward Items", "(none)", NULL);

    olc_print_text(ch, "Entry", quest->entry);

    return false;
}

QEDIT(qedit_target)
{
    Quest* quest;
    char arg[MAX_INPUT_LENGTH];
    MobPrototype* mob;
    ObjPrototype* obj;

    EDIT_QUEST(ch, quest);

    READ_ARG(arg);

    if (arg[0] == '\0' || !is_number(arg)) {
        send_to_char(COLOR_INFO "Syntax:  " COLOR_ALT_TEXT_1 "TARGET <VNUM>" COLOR_EOL, ch);
        return false;
    }

    // No area validation. Some quests can lead to other areas.
    VNUM vnum = STRTOVNUM(arg);

    switch (quest->type) {
    case QUEST_VISIT_MOB:
    case QUEST_KILL_MOB:
        if ((mob = get_mob_prototype(vnum)) == NULL) {
            send_to_char(COLOR_INFO "No mobile has that VNUM." COLOR_EOL, ch);
            return false;
        }
        break;
    case QUEST_GET_OBJ:
        if ((obj = get_object_prototype(vnum)) == NULL) {
            send_to_char(COLOR_INFO "No object has that VNUM." COLOR_EOL, ch);
            return false;
        }
        break;
    }

    quest->target = vnum;

    return true;
}

QEDIT(qedit_upper)
{
    Quest* quest;
    char arg[MAX_INPUT_LENGTH];

    EDIT_QUEST(ch, quest);

    READ_ARG(arg);

    if (arg[0] == '\0' || !is_number(arg)) {
        send_to_char(COLOR_INFO "Syntax:  " COLOR_ALT_TEXT_1 "UPPER <VNUM>" COLOR_EOL, ch);
        return false;
    }

    VNUM vnum = STRTOVNUM(arg);

    if (vnum < quest->target && vnum > 0) {
        printf_to_char(ch, COLOR_INFO "Upper-bound on VNUM range must by greater than lower-bound %d.\n\r", quest->target);
        return false;
    }
    
    quest->target_upper = vnum;

    return true;
}


QEDIT(qedit_end)
{
    Quest* quest;
    char arg[MAX_INPUT_LENGTH];
    MobPrototype* mob;

    EDIT_QUEST(ch, quest);

    READ_ARG(arg);

    if (arg[0] == '\0' || !is_number(arg)) {
        send_to_char(COLOR_INFO "Syntax:  " COLOR_ALT_TEXT_1 "END <VNUM>" COLOR_EOL, ch);
        return false;
    }

    VNUM vnum = STRTOVNUM(arg);

    if ((mob = get_mob_prototype(vnum)) == NULL) {
        send_to_char(COLOR_INFO "No mobile has that VNUM." COLOR_EOL, ch);
        return false;
    }

    quest->end = VNUM_FIELD(mob);

    return true;
}

QEDIT(qedit_faction)
{
    Quest* quest;
    char arg[MAX_INPUT_LENGTH];

    EDIT_QUEST(ch, quest);

    READ_ARG(arg);
    if (arg[0] == '\0' || !is_number(arg)) {
        printf_to_char(ch, COLOR_INFO "Syntax: " COLOR_ALT_TEXT_1 "FACTION <vnum|0>" COLOR_EOL);
        return false;
    }

    VNUM vnum = STRTOVNUM(arg);
    if (vnum != 0 && get_faction(vnum) == NULL) {
        send_to_char(COLOR_INFO "No faction has that VNUM." COLOR_EOL, ch);
        return false;
    }

    quest->reward_faction_vnum = vnum;

    return true;
}

QEDIT(qedit_reward_reputation)
{
    Quest* quest;
    char arg[MAX_INPUT_LENGTH];

    EDIT_QUEST(ch, quest);
    READ_ARG(arg);

    if (arg[0] == '\0' || !is_number(arg)) {
        printf_to_char(ch, COLOR_INFO "Syntax: " COLOR_ALT_TEXT_1 "REWARDREP <value>" COLOR_EOL);
        return false;
    }

    int value = atoi(arg);
    if (value < -10000 || value > 10000) {
        send_to_char(COLOR_INFO "Reputation reward must be between -10000 and 10000." COLOR_EOL, ch);
        return false;
    }

    quest->reward_reputation = (int16_t)value;
    return true;
}

QEDIT(qedit_reward_item)
{
    Quest* quest;
    char arg_slot[MAX_INPUT_LENGTH];
    char arg_vnum[MAX_INPUT_LENGTH];
    char arg_count[MAX_INPUT_LENGTH];

    EDIT_QUEST(ch, quest);

    READ_ARG(arg_slot);
    READ_ARG(arg_vnum);
    READ_ARG(arg_count);

    if (arg_slot[0] == '\0' || arg_vnum[0] == '\0'
        || !is_number(arg_slot) || !is_number(arg_vnum)) {
        printf_to_char(ch,
            COLOR_INFO "Syntax: " COLOR_ALT_TEXT_1 "REWARDITEM <slot 1-%d> <vnum|0> [count]" COLOR_EOL,
            QUEST_MAX_REWARD_ITEMS);
        return false;
    }

    int slot = atoi(arg_slot);
    if (slot < 1 || slot > QUEST_MAX_REWARD_ITEMS) {
        printf_to_char(ch, COLOR_INFO "Slot must be between 1 and %d." COLOR_EOL,
            QUEST_MAX_REWARD_ITEMS);
        return false;
    }

    VNUM vnum = STRTOVNUM(arg_vnum);
    int count = 1;
    if (arg_count[0] != '\0') {
        if (!is_number(arg_count)) {
            send_to_char(COLOR_INFO "Count must be numeric." COLOR_EOL, ch);
            return false;
        }
        count = atoi(arg_count);
    }

    if (vnum == 0) {
        quest->reward_obj_vnum[slot - 1] = 0;
        quest->reward_obj_count[slot - 1] = 0;
        return true;
    }

    if (count <= 0 || count > 1000) {
        send_to_char(COLOR_INFO "Count must be between 1 and 1000." COLOR_EOL, ch);
        return false;
    }

    if (get_object_prototype(vnum) == NULL) {
        send_to_char(COLOR_INFO "No object has that VNUM." COLOR_EOL, ch);
        return false;
    }

    quest->reward_obj_vnum[slot - 1] = vnum;
    quest->reward_obj_count[slot - 1] = (int16_t)count;
    return true;
}
