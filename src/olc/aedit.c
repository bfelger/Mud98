////////////////////////////////////////////////////////////////////////////////
// aedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "array.h"
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

#include "entities/area.h"

#define AEDIT(fun) bool fun( Mobile *ch, char *argument )

AreaData xArea;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const OlcCmdEntry area_olc_comm_table[] = {
    { "name", 	        U(&xArea.header.name),  ed_line_lox_string, 0                   },
    { "min_vnum", 	    0,                      ed_olded,           U(aedit_lvnum)	    },
    { "max_vnum", 	    0,                      ed_olded,           U(aedit_uvnum)	    },
    { "min_level", 	    U(&xArea.low_range),    ed_number_level,    0	                },
    { "max_level", 	    U(&xArea.high_range),   ed_number_level,    0	                },
    { "reset", 		    U(&xArea.reset_thresh), ed_number_s_pos,    0	                },
    { "sector",	        U(&xArea.sector),	    ed_flag_set_sh,		U(sector_flag_table)},
    { "credits", 	    U(&xArea.credits),      ed_line_string,     0                   },
    { "alwaysreset",    U(&xArea.always_reset), ed_bool,            0                   },
    { "instancetype",   U(&xArea.inst_type),    ed_flag_set_sh,     U(inst_type_table)  },
    { "builder", 	    0,                      ed_olded,           U(aedit_builder)	},
    { "commands", 	    0,                      ed_olded,           U(show_commands)	},
    { "create", 	    0,                      ed_olded,           U(aedit_create)	    },
    { "filename", 	    0,                      ed_olded,           U(aedit_file)	    },
    { "reset", 	        0,                      ed_olded,           U(aedit_reset)	    },
    { "security", 	    0,                      ed_olded,           U(aedit_security)	},
    { "show", 	        0,                      ed_olded,           U(aedit_show)	    },
    { "vnums", 	        0,                      ed_olded,           U(aedit_vnums)	    },
    { "levels",         0,                      ed_olded,           U(aedit_levels)     },
    { "?",		        0,                      ed_olded,           U(show_help)	    },
    { "version", 	    0,                      ed_olded,           U(show_version)	    },
    { NULL, 		    0,                      NULL,               0		            }
};

void do_aedit(Mobile* ch, char* argument)
{
    AreaData* area;
    VNUM vnum;
    char arg[MAX_STRING_LENGTH];

    area = ch->in_room->area->data;

    READ_ARG(arg);
    if (is_number(arg)) {
        vnum = STRTOVNUM(arg);
        if (!(area = get_area_data(vnum))) {
            send_to_char("That area vnum does not exist.\n\r", ch);
            return;
        }
    }
    else if (!str_cmp(arg, "create")) {
        if (!aedit_create(ch, argument))
            return;
        else
            area = LAST_AREA_DATA;
    }

    if (!IS_BUILDER(ch, area) || ch->pcdata->security < MIN_AEDIT_SECURITY) {
        send_to_char("You do not have enough security to edit areas.\n\r", ch);
        return;
    }

    set_editor(ch->desc, ED_AREA, U(area));

    aedit_show(ch, "");

    return;
}

/* Area Interpreter, called by do_aedit. */
void aedit(Mobile* ch, char* argument)
{
    AreaData* area;

    EDIT_AREA(ch, area);

    if (!IS_BUILDER(ch, area)) {
        send_to_char("AEdit:  Insufficient security to modify area.\n\r", ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (emptystring(argument)) {
        aedit_show(ch, argument);
        return;
    }

    /* Search Table and Dispatch Command. */
    if (!process_olc_command(ch, argument, area_olc_comm_table))
        interpret(ch, argument);

    return;
}

AreaData* get_vnum_area(VNUM vnum)
{
    AreaData* area;

    FOR_EACH_AREA(area) {
        if (vnum >= area->min_vnum
            && vnum <= area->max_vnum)
            return area;
    }

    return 0;
}

// Area Editor Functions.
AEDIT(aedit_show)
{
    AreaData* area;
    char buf[MIL];

    EDIT_AREA(ch, area);

    olc_print_num_str(ch, "Area", VNUM_FIELD(area), NAME_STR(area));
    olc_print_str(ch, "File", area->file_name);
    olc_print_range(ch, "Vnums", area->min_vnum, area->max_vnum);
    olc_print_range(ch, "Levels", area->low_range, area->high_range);
    olc_print_flags(ch, "Sector", sector_flag_table, area->sector);
    sprintf(buf, "x %d minutes", (area->reset_thresh * PULSE_AREA) / 60);
    olc_print_num_str(ch, "Reset", area->reset_thresh, buf);
    olc_print_yesno(ch, "Always Reset", area->always_reset);
    olc_print_flags(ch, "Instance Type", inst_type_table, area->inst_type);
    olc_print_num(ch, "Security", area->security);
    olc_print_str(ch, "Builders", area->builders);
    olc_print_str(ch, "Credits", area->credits);
    olc_print_flags(ch, "Flags", area_flag_table, area->area_flags);
    
    return false;
}

AEDIT(aedit_reset)
{
    AreaData* area_data;

    EDIT_AREA(ch, area_data);

    Area* area;

    FOR_EACH_AREA_INST(area, area_data)
        reset_area(area);
    send_to_char("Area reset.\n\r", ch);

    return false;
}

AEDIT(aedit_create)
{
    AreaData* area_data;

    if (IS_NPC(ch) || ch->pcdata->security < MIN_AEDIT_SECURITY) {
        send_to_char("You do not have enough security to edit areas.\n\r", ch);
        return false;
    }

    area_data = new_area_data();
    free_string(area_data->builders);
    area_data->builders = str_dup(NAME_STR(ch));
    free_string(area_data->credits);
    area_data->credits = str_dup(NAME_STR(ch));
    area_data->low_range = 1;
    area_data->high_range = MAX_LEVEL;
    area_data->security = ch->pcdata->security;
    write_value_array(&global_areas, OBJ_VAL(area_data));

    set_editor(ch->desc, ED_AREA, U(area_data));

    SET_BIT(area_data->area_flags, AREA_ADDED);
    send_to_char("Area Created.\n\r", ch);
    return true;
}

AEDIT(aedit_file)
{
    AreaData* area;
    char file[MAX_STRING_LENGTH];
    int i;
    size_t length;

    EDIT_AREA(ch, area);

    one_argument(argument, file);    /* Forces Lowercase */

    if (argument[0] == '\0') {
        send_to_char("Syntax:  filename [$file]\n\r", ch);
        return false;
    }

    // Simple Syntax Check.
    length = strlen(argument);
    if (length > 8) {
        send_to_char("No more than eight characters allowed.\n\r", ch);
        return false;
    }

    // Allow only letters and numbers.
    for (i = 0; i < (int)length; i++) {
        if (!ISALNUM(file[i])) {
            send_to_char("Only letters and numbers are valid.\n\r", ch);
            return false;
        }
    }

    free_string(area->file_name);
    strcat(file, ".are");
    area->file_name = str_dup(file);

    send_to_char("Filename set.\n\r", ch);
    return true;
}

AEDIT(aedit_levels)
{
    AreaData* area;
    char lower_str[MAX_STRING_LENGTH];
    char upper_str[MAX_STRING_LENGTH];
    LEVEL  lower;
    LEVEL  upper;

    EDIT_AREA(ch, area);

    READ_ARG(lower_str);
    one_argument(argument, upper_str);

    if (lower_str[0] == '\0' || !is_number(lower_str)
        || upper_str[0] == '\0' || !is_number(upper_str)) {
        send_to_char("Syntax:  levels [#xlower] [#xupper]\n\r", ch);
        return false;
    }

    if ((lower = (LEVEL)atoi(lower_str)) > (upper = (LEVEL)atoi(upper_str))) {
        send_to_char("AEdit:  Upper must be larger than lower.\n\r", ch);
        return false;
    }

    area->high_range = lower;
    area->low_range = upper;

    printf_to_char(ch, "Level range set to %d-%d.\n\r", lower, upper);

    return true;
}

AEDIT(aedit_security)
{
    AreaData* area;
    char sec[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int  value;

    EDIT_AREA(ch, area);

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

    area->security = value;

    send_to_char("Security set.\n\r", ch);
    return true;
}

AEDIT(aedit_builder)
{
    AreaData* area;
    char name[MAX_STRING_LENGTH] = "";
    char buf[MAX_STRING_LENGTH] = "";

    EDIT_AREA(ch, area);

    first_arg(argument, name, false);

    if (name[0] == '\0') {
        send_to_char("Syntax:  builder [$name]  - toggles builder\n\r", ch);
        send_to_char("Syntax:  builder All      - allows everyone\n\r", ch);
        return false;
    }

    name[0] = UPPER(name[0]);

    if (strstr(area->builders, name) != NULL) {
        area->builders = string_replace(area->builders, name, "\0");
        area->builders = string_unpad(area->builders);

        if (area->builders[0] == '\0') {
            free_string(area->builders);
            area->builders = str_dup("None");
        }
        send_to_char("Builder removed.\n\r", ch);
        return true;
    }

    buf[0] = '\0';
    if (strstr(area->builders, "None") != NULL) {
        area->builders = string_replace(area->builders, "None", "\0");
        area->builders = string_unpad(area->builders);
    }

    if (area->builders[0] != '\0') {
        strcat(buf, area->builders);
        strcat(buf, " ");
    }
    strcat(buf, name);
    free_string(area->builders);
    area->builders = string_proper(str_dup(buf));

    send_to_char("Builder added.\n\r", ch);
    send_to_char(area->builders, ch);
    send_to_char("\n\r", ch);
    return true;
}

/*****************************************************************************
 Name:          check_range( lower vnum, upper vnum )
 Purpose:       Ensures the range spans only one area.
 Called by:     aedit_vnums (olc_act.c).
 ****************************************************************************/
bool check_range(VNUM lower, VNUM upper)
{
    AreaData* area;
    int cnt = 0;

    FOR_EACH_AREA(area) {
        // lower < area < upper
        if ((lower <= area->min_vnum && area->min_vnum <= upper)
            || (lower <= area->max_vnum && area->max_vnum <= upper))
            ++cnt;

        if (cnt > 1)
            return false;
    }
    return true;
}

AEDIT(aedit_vnums)
{
    AreaData* area;
    char lower[MAX_STRING_LENGTH];
    char upper[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;

    EDIT_AREA(ch, area);

    READ_ARG(lower);
    one_argument(argument, upper);

    if (!is_number(lower) || lower[0] == '\0'
        || !is_number(upper) || upper[0] == '\0') {
        send_to_char("Syntax:  VNUMS [#xlower] [#xupper]\n\r", ch);
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
        && get_vnum_area(ilower) != area) {
        send_to_char("AEdit:  Lower VNUM already assigned.\n\r", ch);
        return false;
    }

    area->min_vnum = ilower;
    send_to_char("Lower VNUM set.\n\r", ch);

    if (get_vnum_area(iupper)
        && get_vnum_area(iupper) != area) {
        send_to_char("AEdit:  Upper VNUM already assigned.\n\r", ch);
        return true;    /* The lower value has been set. */
    }

    area->max_vnum = iupper;
    send_to_char("VNUMs set.\n\r", ch);

    return true;
}

AEDIT(aedit_lvnum)
{
    AreaData* area;
    char lower[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;

    EDIT_AREA(ch, area);

    one_argument(argument, lower);

    if (!is_number(lower) || lower[0] == '\0') {
        send_to_char("Syntax:  min_vnum [#xlower]\n\r", ch);
        return false;
    }

    if ((ilower = atoi(lower)) > (iupper = area->max_vnum)) {
        send_to_char("AEdit:  Value must be less than the max_vnum.\n\r", ch);
        return false;
    }

    if (!check_range(ilower, iupper)) {
        send_to_char("AEdit:  Range must include only this area.\n\r", ch);
        return false;
    }

    if (get_vnum_area(ilower)
        && get_vnum_area(ilower) != area) {
        send_to_char("AEdit:  Lower VNUM already assigned.\n\r", ch);
        return false;
    }

    area->min_vnum = ilower;
    send_to_char("Lower VNUM set.\n\r", ch);
    return true;
}

AEDIT(aedit_uvnum)
{
    AreaData* area;
    char upper[MAX_STRING_LENGTH];
    int  ilower;
    int  iupper;

    EDIT_AREA(ch, area);

    one_argument(argument, upper);

    if (!is_number(upper) || upper[0] == '\0') {
        send_to_char("Syntax:  max_vnum [#xupper]\n\r", ch);
        return false;
    }

    if ((ilower = area->min_vnum) > (iupper = atoi(upper))) {
        send_to_char("AEdit:  Upper must be larger then lower.\n\r", ch);
        return false;
    }

    if (!check_range(ilower, iupper)) {
        send_to_char("AEdit:  Range must include only this area.\n\r", ch);
        return false;
    }

    if (get_vnum_area(iupper)
        && get_vnum_area(iupper) != area) {
        send_to_char("AEdit:  Upper VNUM already assigned.\n\r", ch);
        return false;
    }

    area->max_vnum = iupper;
    send_to_char("Upper VNUM set.\n\r", ch);

    return true;
}

/*****************************************************************************
 Name:		do_alist
 Purpose:	Normal command to list areas and display area information.
 Called by:	interpreter(interp.c)
 ****************************************************************************/
void do_alist(Mobile* ch, char* argument)
{
    static const char* help = "Syntax: " COLOR_ALT_TEXT_1 "ALIST\n\r"
        "        ALIST ORDERBY (VNUM|NAME)" COLOR_EOL;

    INIT_BUF(result, MAX_STRING_LENGTH);
    char arg[MIL];
    char sort[MIL];

    addf_buf(result, COLOR_DECOR_1 "[" COLOR_TITLE "%3s" COLOR_DECOR_1 "] [" COLOR_TITLE "%-27s" COLOR_DECOR_1 "] (" COLOR_TITLE "%-5s" COLOR_DECOR_1 "-" COLOR_TITLE "%5s" COLOR_DECOR_1 ") " COLOR_DECOR_1 "[" COLOR_TITLE "%-10s" COLOR_DECOR_1 "] " COLOR_TITLE "%3s " COLOR_DECOR_1 "[" COLOR_TITLE "%-10s" COLOR_DECOR_1 "]" COLOR_EOL,
        "Num", "Area Name", "lvnum", "uvnum", "Filename", "Sec", "Builders");

    size_t alist_size = sizeof(AreaData*) * global_areas.count;
    AreaData** alist = (AreaData**)alloc_mem(alist_size);

    {
        AreaData* area;
        int i = 0;
        FOR_EACH_AREA(area) {
            alist[i++] = area;
        }
    }

    READ_ARG(arg);
    if (arg[0]) {
        if (str_prefix(arg, "orderby")) {
            printf_to_char(ch, COLOR_INFO "Unknown option '" COLOR_ALT_TEXT_1 "%s" COLOR_INFO "'." COLOR_CLEAR "\n\r\n\r%s", arg, help);
            goto alist_cleanup;
        }

        READ_ARG(sort);
        if (!str_cmp(sort, "vnum")) {
            SORT_ARRAY(AreaData*, alist, global_areas.count,
                alist[i]->min_vnum < alist[lo]->min_vnum,
                alist[i]->min_vnum > alist[hi]->min_vnum);
        }
        else if (!str_cmp(sort, "name")) {
            SORT_ARRAY(AreaData*, alist, global_areas.count,
                strcasecmp(NAME_STR(alist[i]), NAME_STR(alist[lo])) < 0,
                strcasecmp(NAME_STR(alist[i]), NAME_STR(alist[hi])) > 0);
        }
        else {
            printf_to_char(ch, COLOR_INFO "Unknown sort option '" COLOR_ALT_TEXT_1 "%s" COLOR_INFO "'." COLOR_CLEAR "\n\r\n\r%s", sort, help);
            goto alist_cleanup;
        }
    }

    AreaData* area;
    FOR_EACH_AREA(area) {
        addf_buf( result, COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%3d" COLOR_DECOR_1 "]" COLOR_CLEAR " %-29.29s " COLOR_DECOR_1 "(" COLOR_ALT_TEXT_1 "%-5d" COLOR_DECOR_1 "-" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 ") " COLOR_ALT_TEXT_2 "%-12.12s " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%d" COLOR_DECOR_1 "] [" COLOR_ALT_TEXT_1 "%-10.10s" COLOR_DECOR_1 "]" COLOR_EOL,
            VNUM_FIELD(area),
            NAME_STR(area),
            area->min_vnum,
            area->max_vnum,
            area->file_name,
            area->security,
            area->builders);
    }

    send_to_char(result->string, ch);

alist_cleanup:
    free_buf(result);
    free_mem(alist, alist_size);
    return;
}

/*****************************************************************************
 Name:		get_area_data
 Purpose:	Returns pointer to area with given vnum.
 Called by:	do_aedit(olc.c).
 ****************************************************************************/
AreaData* get_area_data(VNUM vnum)
{
    AreaData* area;

    FOR_EACH_AREA(area) {
        if (VNUM_FIELD(area) == vnum)
            return area;
    }

    return 0;
}
