////////////////////////////////////////////////////////////////////////////////
// aedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "bit.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "lookup.h"
#include "magic.h"
#include "olc.h"
#include "recycle.h"
#include "save.h"
#include "string_edit.h"
#include "stringutils.h"
#include "tables.h"

#include "entities/area_data.h"

#define AEDIT(fun) bool fun( CharData *ch, char *argument )

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const OlcCmd aedit_table[] = {
//   Command		Function
   { "age", 		aedit_age	    },
   { "builder", 	aedit_builder	},
   { "commands", 	show_commands	},
   { "create", 	    aedit_create	},
   { "filename", 	aedit_file	    },
   { "name", 	    aedit_name	    },
   { "reset", 	    aedit_reset	    },
   { "security", 	aedit_security	},
   { "show", 	    aedit_show	    },
   { "vnum", 	    aedit_vnum	    },
   { "lvnum", 	    aedit_lvnum	    },
   { "uvnum", 	    aedit_uvnum	    },
   { "credits", 	aedit_credits	},
   { "lowrange", 	aedit_lowrange	},
   { "highrange", 	aedit_highrange	},
   { "?",		    show_help	    },
   { "version", 	show_version	},
   { NULL, 		    0		        }
};

/* Entry point for editing area_data. */
void do_aedit(CharData* ch, char* argument)
{
    AreaData* pArea;
    VNUM vnum;
    char arg[MAX_STRING_LENGTH];

    pArea = ch->in_room->area;

    argument = one_argument(argument, arg);
    if (is_number(arg)) {
        vnum = STRTOVNUM(arg);
        if (!(pArea = get_area_data(vnum))) {
            send_to_char("That area vnum does not exist.\n\r", ch);
            return;
        }
    }
    else if (!str_cmp(arg, "create")) {
        if (!aedit_create(ch, argument))
            return;
        else
            pArea = area_last;
    }

    if (!IS_BUILDER(ch, pArea) || ch->pcdata->security < MIN_AEDIT_SECURITY) {
        send_to_char("You do not have enough security to edit areas.\n\r", ch);
        return;
    }

    set_editor(ch->desc, ED_AREA, U(pArea));
/*	ch->desc->pEdit = (void *) pArea;
    ch->desc->editor = ED_AREA; */
    return;
}

/* Area Interpreter, called by do_aedit. */
void aedit(CharData* ch, char* argument)
{
    AreaData* pArea;
    char    command[MAX_INPUT_LENGTH];
    char    arg[MAX_INPUT_LENGTH];
    int     cmd;
    FLAGS     value;

    EDIT_AREA(ch, pArea);

    strcpy(arg, argument);
    argument = one_argument(argument, command);

    if (!IS_BUILDER(ch, pArea)) {
        send_to_char("AEdit:  Insufficient security to modify area.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(command, "done")) {
        edit_done(ch);
        return;
    }

    if (command[0] == '\0') {
        aedit_show(ch, argument);
        return;
    }

    if ((value = flag_value(area_flag_table, command)) != NO_FLAG) {
        TOGGLE_BIT(pArea->area_flags, value);

        send_to_char("Flag toggled.\n\r", ch);
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; aedit_table[cmd].name != NULL; cmd++) {
        if (!str_prefix(command, aedit_table[cmd].name)) {
            if ((*aedit_table[cmd].olc_fun) (ch, argument)) {
                SET_BIT(pArea->area_flags, AREA_CHANGED);
                return;
            }
            else
                return;
        }
    }

    /* Default to Standard Interpreter. */
    interpret(ch, arg);
    return;
}

AreaData* get_vnum_area(VNUM vnum)
{
    AreaData* pArea;

    for (pArea = area_first; pArea; pArea = pArea->next) {
        if (vnum >= pArea->min_vnum
            && vnum <= pArea->max_vnum)
            return pArea;
    }

    return 0;
}

/*
 * Area Editor Functions.
 */
AEDIT(aedit_show)
{
    AreaData* pArea;
    char buf[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    sprintf(buf, "Name:     [%"PRVNUM"] %s\n\r", pArea->vnum, pArea->name);
    send_to_char(buf, ch);

    sprintf(buf, "File:     %s\n\r", pArea->file_name);
    send_to_char(buf, ch);

    sprintf(buf, "Vnums:    [%d-%d]\n\r", pArea->min_vnum, pArea->max_vnum);
    send_to_char(buf, ch);

    sprintf(buf, "Age:      [%d]\n\r", pArea->age);
    send_to_char(buf, ch);

    sprintf(buf, "Players:  [%d]\n\r", pArea->nplayer);
    send_to_char(buf, ch);

    sprintf(buf, "Security: [%d]\n\r", pArea->security);
    send_to_char(buf, ch);

    sprintf(buf, "Builders: [%s]\n\r", pArea->builders);
    send_to_char(buf, ch);

    sprintf(buf, "Credits : [%s]\n\r", pArea->credits);
    send_to_char(buf, ch);

    sprintf(buf, "Lo range: [%d]\n\r", pArea->low_range);
    send_to_char(buf, ch);

    sprintf(buf, "Hi range: [%d]\n\r", pArea->high_range);
    send_to_char(buf, ch);

    sprintf(buf, "Flags:    [%s]\n\r", flag_string(area_flag_table, pArea->area_flags));
    send_to_char(buf, ch);

    return false;
}

AEDIT(aedit_reset)
{
    AreaData* pArea;

    EDIT_AREA(ch, pArea);

    reset_area(pArea);
    send_to_char("Area reset.\n\r", ch);

    return false;
}
AEDIT(aedit_create)
{
    AreaData* pArea;

    if (IS_NPC(ch) || ch->pcdata->security < MIN_AEDIT_SECURITY) {
        send_to_char("You do not have enough security to edit areas.\n\r", ch);
        return false;
    }

    pArea = new_area();
    area_last->next = pArea;
    area_last = pArea;

    set_editor(ch->desc, ED_AREA, U(pArea));

    SET_BIT(pArea->area_flags, AREA_ADDED);
    send_to_char("Area Created.\n\r", ch);
    return true;
}

AEDIT(aedit_name)
{
    AreaData* pArea;

    EDIT_AREA(ch, pArea);

    if (argument[0] == '\0') {
        send_to_char("Syntax:   name [$name]\n\r", ch);
        return false;
    }

    free_string(pArea->name);
    pArea->name = str_dup(argument);

    send_to_char("Name set.\n\r", ch);
    return true;
}

AEDIT(aedit_credits)
{
    AreaData* pArea;

    EDIT_AREA(ch, pArea);

    if (argument[0] == '\0') {
        send_to_char("Syntax:   credits [$credits]\n\r", ch);
        return false;
    }

    free_string(pArea->credits);
    pArea->credits = str_dup(argument);

    send_to_char("Credits set.\n\r", ch);
    return true;
}


AEDIT(aedit_file)
{
    AreaData* pArea;
    char file[MAX_STRING_LENGTH];
    int i;
    size_t length;

    EDIT_AREA(ch, pArea);

    one_argument(argument, file);    /* Forces Lowercase */

    if (argument[0] == '\0') {
        send_to_char("Syntax:  filename [$file]\n\r", ch);
        return false;
    }

    /*
     * Simple Syntax Check.
     */
    length = strlen(argument);
    if (length > 8) {
        send_to_char("No more than eight characters allowed.\n\r", ch);
        return false;
    }

    /*
     * Allow only letters and numbers.
     */
    for (i = 0; i < (int)length; i++) {
        if (!ISALNUM(file[i])) {
            send_to_char("Only letters and numbers are valid.\n\r", ch);
            return false;
        }
    }

    free_string(pArea->file_name);
    strcat(file, ".are");
    pArea->file_name = str_dup(file);

    send_to_char("Filename set.\n\r", ch);
    return true;
}

AEDIT(aedit_lowrange)
{
    AreaData* pArea;
    LEVEL low_r;

    EDIT_AREA(ch, pArea);

    if (argument[0] == '\0' || !is_number(argument)) {
        send_to_char("Syntax : lowrange [min level]\n\r", ch);
        return false;
    }

    low_r = (LEVEL)atoi(argument);

    if (low_r < 0 || low_r > MAX_LEVEL) {
        send_to_char("Value must be between 0 and MAX_LEVEL.\n\r", ch);
        return false;
    }

    if (low_r > pArea->high_range) {
        send_to_char("Value must be lower than the max level.\n\r", ch);
        return false;
    }

    pArea->low_range = low_r;

    send_to_char("Minimum level set.\n\r", ch);
    return true;
}

AEDIT(aedit_highrange)
{
    AreaData* pArea;
    LEVEL high_r;

    EDIT_AREA(ch, pArea);

    if (argument[0] == '\0' || !is_number(argument)) {
        send_to_char("Syntax : highrange [max level]\n\r", ch);
        return false;
    }

    high_r = (LEVEL)atoi(argument);

    if (high_r < 0 || high_r > MAX_LEVEL) {
        send_to_char("Value must be between 0 and MAX_LEVEL.\n\r", ch);
        return false;
    }

    if (high_r < pArea->low_range) {
        send_to_char("Value must be higher than the min level.\n\r", ch);
        return false;
    }

    pArea->high_range = high_r;

    send_to_char("Maximum level set.\n\r", ch);
    return true;
}

AEDIT(aedit_age)
{
    AreaData* pArea;
    char age[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    one_argument(argument, age);

    if (!is_number(age) || age[0] == '\0') {
        send_to_char("Syntax:  age [#xage]\n\r", ch);
        return false;
    }

    pArea->age = (int16_t)atoi(age);

    send_to_char("Age set.\n\r", ch);
    return true;
}

AEDIT(aedit_security)
{
    AreaData* pArea;
    char sec[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int  value;

    EDIT_AREA(ch, pArea);

    one_argument(argument, sec);

    if (!is_number(sec) || sec[0] == '\0') {
        send_to_char("Syntax:  security [#xlevel]\n\r", ch);
        return false;
    }

    value = atoi(sec);

    if (value > ch->pcdata->security || value < 0) {
        if (ch->pcdata->security != 0) {
            sprintf(buf, "Security is 0-%d.\n\r", ch->pcdata->security);
            send_to_char(buf, ch);
        }
        else
            send_to_char("Security is 0 only.\n\r", ch);
        return false;
    }

    pArea->security = value;

    send_to_char("Security set.\n\r", ch);
    return true;
}

AEDIT(aedit_builder)
{
    AreaData* pArea;
    char name[MAX_STRING_LENGTH] = "";
    char buf[MAX_STRING_LENGTH] = "";

    EDIT_AREA(ch, pArea);

    first_arg(argument, name, false);

    if (name[0] == '\0') {
        send_to_char("Syntax:  builder [$name]  - toggles builder\n\r", ch);
        send_to_char("Syntax:  builder All      - allows everyone\n\r", ch);
        return false;
    }

    name[0] = UPPER(name[0]);

    if (strstr(pArea->builders, name) != NULL) {
        pArea->builders = string_replace(pArea->builders, name, "\0");
        pArea->builders = string_unpad(pArea->builders);

        if (pArea->builders[0] == '\0') {
            free_string(pArea->builders);
            pArea->builders = str_dup("None");
        }
        send_to_char("Builder removed.\n\r", ch);
        return true;
    }

    buf[0] = '\0';
    if (strstr(pArea->builders, "None") != NULL) {
        pArea->builders = string_replace(pArea->builders, "None", "\0");
        pArea->builders = string_unpad(pArea->builders);
    }

    if (pArea->builders[0] != '\0') {
        strcat(buf, pArea->builders);
        strcat(buf, " ");
    }
    strcat(buf, name);
    free_string(pArea->builders);
    pArea->builders = string_proper(str_dup(buf));

    send_to_char("Builder added.\n\r", ch);
    send_to_char(pArea->builders, ch);
    send_to_char("\n\r", ch);
    return true;
}

/*****************************************************************************
 Name:          check_range( lower vnum, upper vnum )
 Purpose:       Ensures the range spans only one area.
 Called by:     aedit_vnum (olc_act.c).
 ****************************************************************************/
bool check_range(VNUM lower, VNUM upper)
{
    AreaData* pArea;
    int cnt = 0;

    for (pArea = area_first; pArea; pArea = pArea->next) {
        // lower < area < upper
        if ((lower <= pArea->min_vnum && pArea->min_vnum <= upper)
            || (lower <= pArea->max_vnum && pArea->max_vnum <= upper))
            ++cnt;

        if (cnt > 1)
            return false;
    }
    return true;
}

AEDIT(aedit_vnum)
{
    AreaData* pArea;
    char lower[MAX_STRING_LENGTH];
    char upper[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;

    EDIT_AREA(ch, pArea);

    argument = one_argument(argument, lower);
    one_argument(argument, upper);

    if (!is_number(lower) || lower[0] == '\0'
        || !is_number(upper) || upper[0] == '\0') {
        send_to_char("Syntax:  vnum [#xlower] [#xupper]\n\r", ch);
        return false;
    }

    if ((ilower = atoi(lower)) > (iupper = atoi(upper))) {
        send_to_char("AEdit:  Upper must be larger then lower.\n\r", ch);
        return false;
    }

    if (!check_range(ilower, iupper)) {
        send_to_char("AEdit:  Range must include only this area.\n\r", ch);
        return false;
    }

    if (get_vnum_area(ilower)
        && get_vnum_area(ilower) != pArea) {
        send_to_char("AEdit:  Lower vnum already assigned.\n\r", ch);
        return false;
    }

    pArea->min_vnum = ilower;
    send_to_char("Lower vnum set.\n\r", ch);

    if (get_vnum_area(iupper)
        && get_vnum_area(iupper) != pArea) {
        send_to_char("AEdit:  Upper vnum already assigned.\n\r", ch);
        return true;    /* The lower value has been set. */
    }

    pArea->max_vnum = iupper;
    send_to_char("Upper vnum set.\n\r", ch);

    return true;
}

AEDIT(aedit_lvnum)
{
    AreaData* pArea;
    char lower[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;

    EDIT_AREA(ch, pArea);

    one_argument(argument, lower);

    if (!is_number(lower) || lower[0] == '\0') {
        send_to_char("Syntax:  min_vnum [#xlower]\n\r", ch);
        return false;
    }

    if ((ilower = atoi(lower)) > (iupper = pArea->max_vnum)) {
        send_to_char("AEdit:  Value must be less than the max_vnum.\n\r", ch);
        return false;
    }

    if (!check_range(ilower, iupper)) {
        send_to_char("AEdit:  Range must include only this area.\n\r", ch);
        return false;
    }

    if (get_vnum_area(ilower)
        && get_vnum_area(ilower) != pArea) {
        send_to_char("AEdit:  Lower vnum already assigned.\n\r", ch);
        return false;
    }

    pArea->min_vnum = ilower;
    send_to_char("Lower vnum set.\n\r", ch);
    return true;
}

AEDIT(aedit_uvnum)
{
    AreaData* pArea;
    char upper[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;

    EDIT_AREA(ch, pArea);

    one_argument(argument, upper);

    if (!is_number(upper) || upper[0] == '\0') {
        send_to_char("Syntax:  max_vnum [#xupper]\n\r", ch);
        return false;
    }

    if ((ilower = pArea->min_vnum) > (iupper = atoi(upper))) {
        send_to_char("AEdit:  Upper must be larger then lower.\n\r", ch);
        return false;
    }

    if (!check_range(ilower, iupper)) {
        send_to_char("AEdit:  Range must include only this area.\n\r", ch);
        return false;
    }

    if (get_vnum_area(iupper)
        && get_vnum_area(iupper) != pArea) {
        send_to_char("AEdit:  Upper vnum already assigned.\n\r", ch);
        return false;
    }

    pArea->max_vnum = iupper;
    send_to_char("Upper vnum set.\n\r", ch);

    return true;
}

/*****************************************************************************
 Name:		do_alist
 Purpose:	Normal command to list areas and display area information.
 Called by:	interpreter(interp.c)
 ****************************************************************************/
void do_alist(CharData* ch, char* argument)
{
    char    buf[MAX_STRING_LENGTH];
    char    result[MAX_STRING_LENGTH * 2];	/* May need tweaking. */
    AreaData* pArea;

    sprintf(result, "[%3s] [%-27s] (%-5s-%5s) [%-10s] %3s [%-10s]\n\r",
        "Num", "Area Name", "lvnum", "uvnum", "Filename", "Sec", "Builders");

    for (pArea = area_first; pArea; pArea = pArea->next) {
        sprintf(buf, "[%3d] %-29.29s (%-5d-%5d) %-12.12s [%d] [%-10.10s]\n\r",
            pArea->vnum,
            pArea->name,
            pArea->min_vnum,
            pArea->max_vnum,
            pArea->file_name,
            pArea->security,
            pArea->builders);
        strcat(result, buf);
    }

    send_to_char(result, ch);
    return;
}

/*****************************************************************************
 Name:		get_area_data
 Purpose:	Returns pointer to area with given vnum.
 Called by:	do_aedit(olc.c).
 ****************************************************************************/
AreaData* get_area_data(VNUM vnum)
{
    AreaData* pArea;

    for (pArea = area_first; pArea; pArea = pArea->next) {
        if (pArea->vnum == vnum)
            return pArea;
    }

    return 0;
}
