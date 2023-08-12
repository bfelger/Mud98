/* The following code is based on ILAB OLC by Jason Dinkel */
/* Mobprogram code by Lordrom for Nevermore Mud */



#include "merc.h"

#include "comm.h"
#include "olc.h"
#include "recycle.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define MPEDIT( fun )           bool fun(CHAR_DATA *ch, char*argument)

extern MPROG_CODE xProg;

DECLARE_OLC_FUN(pedit_create);
DECLARE_OLC_FUN(pedit_show);

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const struct olc_comm_type prog_olc_comm_table[] =
{
    {	"create",	0,			    ed_olded,	U(pedit_create) },
    {	"code",		U(&xProg.code),	ed_desc,	0		        },
    {	"show",		0,			    ed_olded,	U(pedit_show)	},
    { 	"commands",	0,			    ed_olded,	U(show_commands)},
    { 	"?",		0,			    ed_olded,	U(show_help)	},
    { 	"version",	0,			    ed_olded,	U(show_version)	},
    { 	NULL,		0,			    0,		    0		        }
};

void pedit(CHAR_DATA* ch, char* argument)
{
    if (ch->pcdata->security < MIN_PEDIT_SECURITY) {
        send_to_char("MPEdit : You do not have enough security to edit prog.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (emptystring(argument)) {
        pedit_show(ch, argument);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (!process_olc_command(ch, argument, prog_olc_comm_table))
        interpret(ch, argument);

    return;
}

void do_pedit(CHAR_DATA* ch, char* argument)
{
    MPROG_CODE* pMcode;
    char command[MAX_INPUT_LENGTH];

    if (IS_NPC(ch) || ch->desc == NULL)
        return;

    if (ch->pcdata->security < MIN_PEDIT_SECURITY) {
        send_to_char("PEdit : You do not have enough security to edit progs.\n\r", ch);
        return;
    }

    argument = one_argument(argument, command);

    if (is_number(command)) {
        AREA_DATA* pArea;

        if ((pArea = get_vnum_area(atoi(command))) == NULL) {
            send_to_char("PEdit : That vnum is not assigned to an area.\n\r", ch);
            return;
        }
        if (!IS_BUILDER(ch, pArea)) {
            send_to_char("PEdit : You do not have access to this area.\n\r", ch);
            return;
        }
        if (!(pMcode = get_mprog_index(atoi(command)))) {
            send_to_char("PEdit : That prog does not yet exist.\n\r", ch);
            return;
        }
        ch->desc->pEdit = U(pMcode);
        ch->desc->editor = ED_PROG;
        return;
    }

    if (!str_cmp(command, "create")) {
        if (argument[0] == '\0') {
            send_to_char("Syntax: edit code create [vnum]\n\r", ch);
            return;
        }

        pedit_create(ch, argument);
        return;
    }

    send_to_char("Syntax : pedit [vnum]\n\r", ch);
    send_to_char("         pedit create [vnum]\n\r", ch);

    return;
}

MPEDIT(pedit_create)
{
    MPROG_CODE* pMcode;
    AREA_DATA* pArea;
    int value;

    value = atoi(argument);

    if (argument[0] == '\0' || value == 0) {
        send_to_char("Syntax: pedit create [vnum]\n\r", ch);
        return false;
    }

    if (get_mprog_index(value)) {
        send_to_char("PEdit: That prog already exists.\n\r", ch);
        return false;
    }

    if ((pArea = get_vnum_area(value)) == NULL) {
        send_to_char("PEdit: That vnum has not been assigned to an area.\n\r", ch);
        return false;
    }

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("PEdit: You do not have access to this area.\n\r", ch);
        return false;
    }

    if (ch->pcdata->security < MIN_PEDIT_SECURITY) {
        send_to_char("PEdit: You do not have enough security to create progs.\n\r", ch);
        return false;
    }

    pMcode = new_mpcode();
    pMcode->vnum = value;
    pMcode->next = mprog_list;
    pMcode->changed = true;
    mprog_list = pMcode;

    set_editor(ch->desc, ED_PROG, U(pMcode));

    send_to_char("New prog created.\n\r", ch);

    return true;
}

MPEDIT(pedit_show)
{
    MPROG_CODE* pMcode;
    char buf[MAX_STRING_LENGTH];

    EDIT_PROG(ch, pMcode);

    sprintf(buf,
        "Vnum:       [%d]\n\r"
        "Code:\n\r%s\n\r",
        pMcode->vnum, pMcode->code);
    send_to_char(buf, ch);

    return false;
}

void do_mplist(CHAR_DATA* ch, char* argument)
{
    int count;
    bool fAll = false;
    MPROG_CODE* mprg;
    char buf[MAX_STRING_LENGTH];
    BUFFER* buffer;
    buffer = new_buf();

    if (IS_IMMORTAL(ch) && !str_cmp(argument, "all"))
        fAll = true;

    for (count = 1, mprg = mprog_list; mprg != NULL; mprg = mprg->next) {
        if (fAll
            || (mprg->vnum >= ch->in_room->area->min_vnum
                && mprg->vnum <= ch->in_room->area->max_vnum)) {
            sprintf(buf, "[%3d] %5d ", count, mprg->vnum);
            add_buf(buffer, buf);
            if (count % 2 == 0)
                add_buf(buffer, "\n\r");
            count++;
        }
    }

    if (count % 2 == 0)
        add_buf(buffer, "\n\r");

    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);

    return;
}

#undef U
#ifdef OLD_U
#define U OLD_U
#undef OLD_U
#endif