////////////////////////////////////////////////////////////////////////////////
// qedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "magic.h"
#include "olc.h"
#include "skills.h"

#include "entities/player_data.h"

#include "data/quest.h"

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
        send_to_char("{jQEDIT: You do not have enough security to edit quests.{x\n\r", ch);
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
    Area* area;
    char command[MAX_STRING_LENGTH];
    VNUM  vnum;

    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    READ_ARG(command);

    if (is_number(command)) {
        vnum = (VNUM)atoi(command);

        if ((area = get_vnum_area(vnum)) == NULL) {
            send_to_char("{jQEDIT: That VNUM is not assigned to an area.{x\n\r", ch);
            return;
        }

        if (!IS_BUILDER(ch, area)) {
            send_to_char("{jYou do not have enough security to edit quests in that area.{x\n\r", ch);
            return;
        }

        if (!(quest = get_quest(vnum))) {
            send_to_char("{jQEDIT: That VNUM does not exist.{x\n\r", ch);
            return;
        }

        set_editor(ch->desc, ED_QUEST, U(quest));
        qedit_show(ch, "");
        return;
    }

    if (!str_cmp(command, "create")) {
        if (argument[0] == '\0') {
            send_to_char("{jSyntax: {*QEDIT CREATE [VNUM]{x\n\r", ch);
            return;
        }

        if (qedit_create(ch, argument))
            qedit_show(ch, "");
        return;
    }

    send_to_char(
        "{jSyntax: {*QEDIT [VNUM]\n\r"
        "        QEDIT CREATE [VNUM]{x\n\r", ch);
    return;
}

QEDIT(qedit_create)
{
    Quest* quest;
    Area* area;

    VNUM vnum = (VNUM)atoi(argument);

    if (argument[0] == '\0' || vnum == 0) {
        send_to_char("{jSyntax: {*QEDIT CREATE [VNUM]{x\n\r", ch);
        return false;
    }

    if (get_quest(vnum)) {
        send_to_char("{jQEDIT: That quest already exists.{x\n\r", ch);
        return false;
    }

    if ((area = get_vnum_area(vnum)) == NULL) {
        send_to_char("{jQEDIT: That vnum has not been assigned to an area.{x\n\r", ch);
        return false;
    }

    if (!IS_BUILDER(ch, area)) {
        send_to_char("{jQEDIT: You do not have access to this area.{x\n\r", ch);
        return false;
    }

    if (ch->pcdata->security < MIN_QEDIT_SECURITY) {
        send_to_char("{jQEDIT: You do not have enough security to create quests.{x\n\r", ch);
        return false;
    }

    quest = new_quest();
    quest->vnum = vnum;
    quest->area = area;
    quest->area->low_range;

    ORDERED_INSERT(Quest, quest, area->quests, vnum);

    set_editor(ch->desc, ED_QUEST, U(quest));

    send_to_char("{jNew quest created.{x\n\r", ch);

    return true;
}

static const char* get_targ_name(Quest* quest, VNUM vnum)
{
    MobPrototype* mob;
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

    printf_to_char(ch, "VNUM:       {|[{*%"PRVNUM"{|]{x\n\r", quest->vnum);
    printf_to_char(ch, "Name:       {T%s{x\n\r", quest->name && quest->name[0] ? quest->name : "(none)");
    printf_to_char(ch, "Area:       {*%s{x\n\r", quest->area->name);
    printf_to_char(ch, "Type:       {|[{*%s{|]{x\n\r", quest_type_table[quest->type].name);
    printf_to_char(ch, "Level:      {|[{*%d{|]{x\n\r", quest->level);
    printf_to_char(ch, "End:        {|[{*%d{|]{x {_%s{x\n\r", quest->end, end_name);
    printf_to_char(ch, "Target:     {|[{*%d{|]{x {_%s{x", quest->target, get_targ_name(quest, quest->target));
    if (quest->type == QUEST_KILL_MOB) {
        if (quest->target_upper <= quest->target)
            printf_to_char(ch, " {j(set {*UPPER{j to use a range of VNUMs){x");
        else {
            MobPrototype* targ_mob;
            for (VNUM i = quest->target+1; i < quest->target_upper; ++i)
                if ((targ_mob = get_mob_prototype(i)) != NULL)
                    printf_to_char(ch, "\n\r            {|[{*%d{|]{x {_%s{x", 
                        targ_mob->vnum, targ_mob->short_descr);
            printf_to_char(ch, "\n\rUpper:      {|[{*%d{|]{x ", quest->target_upper);
            if ((targ_mob = get_mob_prototype(quest->target_upper)) != NULL)
                printf_to_char(ch, "{_%s{x", targ_mob->short_descr);
        }
        printf_to_char(ch, "\n\rAmount:     {|[{*%d{|]{x\n\r", quest->amount);
    }
    printf_to_char(ch, "XP:         {|[{*%d{|]{x\n\r", quest->xp);
    if (quest->entry && quest->entry[0])
        printf_to_char(ch, "Entry:\n\r{_%s{x\n\r", quest->entry);
    else
        printf_to_char(ch, "Entry:      {|[{*none{|]{x\n\r");

    return false;
}

QEDIT(qedit_target)
{
    Quest* quest;
    char arg[MAX_INPUT_LENGTH];
    MobPrototype* mob;

    EDIT_QUEST(ch, quest);

    READ_ARG(arg);

    if (arg[0] == '\0' || !is_number(arg)) {
        send_to_char("{jSyntax:  {*TARGET <VNUM>{x\n\r", ch);
        return false;
    }

    // No area validation. Some quests can lead to other areas.
    VNUM vnum = STRTOVNUM(arg);

    switch (quest->type) {
    case QUEST_VISIT_MOB:
    case QUEST_KILL_MOB:
        if ((mob = get_mob_prototype(vnum)) == NULL) {
            send_to_char("{jQEDIT: No mobile has that VNUM.{x\n\r", ch);
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
        send_to_char("{jSyntax:  {*UPPER <VNUM>{x\n\r", ch);
        return false;
    }

    VNUM vnum = STRTOVNUM(arg);

    if (vnum < quest->target && vnum > 0) {
        printf_to_char(ch, "{jUpper-bound on VNUM range must by greater than lower-bound %d.\n\r", quest->target);
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
        send_to_char("{jSyntax:  {*END <VNUM>{x\n\r", ch);
        return false;
    }

    VNUM vnum = STRTOVNUM(arg);

    if ((mob = get_mob_prototype(vnum)) == NULL) {
        send_to_char("{jQEDIT: No mobile has that VNUM.{x\n\r", ch);
        return false;
    }

    quest->end = mob->vnum;

    return true;
}