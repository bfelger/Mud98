////////////////////////////////////////////////////////////////////////////////
// aedit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "array.h"
#include "bit.h"
#include "comm.h"
#include "config.h"
#include "db.h"
#include "handler.h"
#include "lookup.h"
#include "magic.h"
#include "olc.h"

#include <color.h>
#include "recycle.h"
#include "save.h"
#include "string_edit.h"
#include "stringutils.h"
#include "tables.h"

#include "entities/area.h"
#include "entities/faction.h"
#include "entities/mob_prototype.h"

#define AEDIT(fun) bool fun( Mobile *ch, char *argument )

AreaData xArea;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

static Faction* find_faction_any(const char* identifier);
static Faction* find_area_faction(AreaData* area, const char* identifier);
static void aedit_print_faction_summary(Mobile* ch, AreaData* area);
static void aedit_list_factions(Mobile* ch, AreaData* area);
static void aedit_show_faction_detail(Mobile* ch, Faction* faction);
static void format_relation_list(Buffer* buffer, const char* label, ValueArray* relations);
static void clear_mob_faction_references(VNUM vnum);
static void aedit_send_faction_syntax(Mobile* ch);
static void aedit_print_story_beats(Mobile* ch, AreaData* area);
static void aedit_print_checklist(Mobile* ch, AreaData* area);
static ChecklistStatus checklist_status_from_name(const char* name, ChecklistStatus def);
static const char* checklist_status_name(ChecklistStatus status);
static StoryBeat* story_beat_by_index(AreaData* area, int index, StoryBeat** out_prev);
static ChecklistItem* checklist_by_index(AreaData* area, int index, ChecklistItem** out_prev);
static void aedit_seed_default_checklist(AreaData* area);

AEDIT(aedit_story);
AEDIT(aedit_checklist);

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
    { "story",          0,                      ed_olded,           U(aedit_story)      },
    { "checklist",      0,                      ed_olded,           U(aedit_checklist)  },
    { "faction",        0,                      ed_olded,           U(aedit_faction)    },
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

static Faction* find_faction_any(const char* identifier)
{
    if (IS_NULLSTR(identifier))
        return NULL;

    if (is_number(identifier))
        return get_faction((VNUM)atoi(identifier));

    return get_faction_by_name(identifier);
}

static Faction* find_area_faction(AreaData* area, const char* identifier)
{
    Faction* faction = find_faction_any(identifier);
    if (faction == NULL || faction->area != area)
        return NULL;
    return faction;
}

static void format_relation_list(Buffer* buffer, const char* label, ValueArray* relations)
{
    addf_buf(buffer, COLOR_INFO "%-10s" COLOR_CLEAR ": ", label);

    bool printed = false;
    if (relations != NULL) {
        for (int i = 0; i < relations->count; ++i) {
            Value entry = relations->values[i];
            if (!IS_INT(entry))
                continue;

            Faction* rel = get_faction((VNUM)AS_INT(entry));
            if (rel != NULL)
                addf_buf(buffer, COLOR_ALT_TEXT_2 "%s" COLOR_CLEAR " (#%" PRVNUM ")  ",
                    NAME_STR(rel), VNUM_FIELD(rel));
            else
                addf_buf(buffer, COLOR_ALT_TEXT_2 "Unknown (#%" PRVNUM ")" COLOR_CLEAR "  ",
                    (VNUM)AS_INT(entry));
            printed = true;
        }
    }

    if (!printed)
        add_buf(buffer, COLOR_ALT_TEXT_2 "(none)" COLOR_CLEAR);

    add_buf(buffer, COLOR_EOL);
}

static const char* checklist_status_name(ChecklistStatus status)
{
    switch (status) {
    case CHECK_TODO: return "To-Do";
    case CHECK_IN_PROGRESS: return "In Progress";
    case CHECK_DONE: return "Done";
    default: return "Unknown";
    }
}

static ChecklistStatus checklist_status_from_name(const char* name, ChecklistStatus def)
{
    if (IS_NULLSTR(name))
        return def;
    if (!str_prefix(name, "todo"))
        return CHECK_TODO;
    if (!str_prefix(name, "inprogress") || !str_prefix(name, "progress"))
        return CHECK_IN_PROGRESS;
    if (!str_prefix(name, "done") || !str_prefix(name, "complete"))
        return CHECK_DONE;
    return def;
}

static StoryBeat* story_beat_by_index(AreaData* area, int index, StoryBeat** out_prev)
{
    StoryBeat* prev = NULL;
    StoryBeat* beat = area->story_beats;
    int i = 0;
    while (beat && i < index) {
        prev = beat;
        beat = beat->next;
        ++i;
    }
    if (out_prev)
        *out_prev = prev;
    return (i == index) ? beat : NULL;
}

static ChecklistItem* checklist_by_index(AreaData* area, int index, ChecklistItem** out_prev)
{
    ChecklistItem* prev = NULL;
    ChecklistItem* item = area->checklist;
    int i = 0;
    while (item && i < index) {
        prev = item;
        item = item->next;
        ++i;
    }
    if (out_prev)
        *out_prev = prev;
    return (i == index) ? item : NULL;
}

static void aedit_print_story_beats(Mobile* ch, AreaData* area)
{
    send_to_char(COLOR_INFO "Story Beats:" COLOR_CLEAR "\n\r", ch);
    if (!area->story_beats) {
        send_to_char("  (none)\n\r", ch);
        return;
    }
    int idx = 0;
    for (StoryBeat* beat = area->story_beats; beat; beat = beat->next, ++idx) {
        printf_to_char(ch, "  %d) %s\n\r", idx + 1, beat->title);
        if (!IS_NULLSTR(beat->description))
            printf_to_char(ch, "     %s\n\r", beat->description);
    }
}

static void aedit_print_checklist(Mobile* ch, AreaData* area)
{
    send_to_char(COLOR_INFO "Checklist:" COLOR_CLEAR "\n\r", ch);
    if (!area->checklist) {
        send_to_char("  (none)\n\r", ch);
        return;
    }
    int idx = 0;
    for (ChecklistItem* item = area->checklist; item; item = item->next, ++idx) {
        printf_to_char(ch, "  %d) [%s] %s\n\r", idx + 1, checklist_status_name(item->status), item->title);
        if (!IS_NULLSTR(item->description))
            printf_to_char(ch, "     %s\n\r", item->description);
    }
}

static void aedit_print_faction_summary(Mobile* ch, AreaData* area)
{
    Buffer* buffer = new_buf();
    bool found = false;

    add_buf(buffer, COLOR_TITLE "Factions" COLOR_CLEAR "\n\r");

    if (faction_table.capacity > 0 && faction_table.entries != NULL) {
        for (int idx = 0; idx < faction_table.capacity; ++idx) {
            Entry* entry = &faction_table.entries[idx];
            if (IS_NIL(entry->value) || !IS_FACTION(entry->value))
                continue;

            Faction* faction = AS_FACTION(entry->value);
            if (faction->area != area)
                continue;

            addf_buf(buffer,
                COLOR_DECOR_1 "[%" PRVNUM "] " COLOR_ALT_TEXT_1 "%-20s"
                COLOR_CLEAR " default: " COLOR_ALT_TEXT_2 "%6d"
                COLOR_CLEAR " allies: " COLOR_ALT_TEXT_2 "%2d"
                COLOR_CLEAR " enemies: " COLOR_ALT_TEXT_2 "%2d" COLOR_EOL,
                VNUM_FIELD(faction),
                NAME_STR(faction),
                faction->default_standing,
                faction->allies.count,
                faction->enemies.count);
            found = true;
        }
    }

    if (!found)
        add_buf(buffer, COLOR_ALT_TEXT_2 "  (none)" COLOR_CLEAR "\n\r");

    send_to_char(BUF(buffer), ch);
    free_buf(buffer);
}

static void aedit_list_factions(Mobile* ch, AreaData* area)
{
    Buffer* buffer = new_buf();
    addf_buf(buffer, COLOR_INFO "Factions for %s" COLOR_EOL, NAME_STR(area));

    if (faction_table.capacity == 0 || faction_table.entries == NULL) {
        add_buf(buffer, COLOR_ALT_TEXT_2 "  (none)" COLOR_CLEAR "\n\r");
    }
    else {
        bool found = false;
        for (int idx = 0; idx < faction_table.capacity; ++idx) {
            Entry* entry = &faction_table.entries[idx];
            if (IS_NIL(entry->value) || !IS_FACTION(entry->value))
                continue;

            Faction* faction = AS_FACTION(entry->value);
            if (faction->area != area)
                continue;

            addf_buf(buffer,
                COLOR_DECOR_1 "[%" PRVNUM "] " COLOR_ALT_TEXT_1 "%s" COLOR_CLEAR
                " default: " COLOR_ALT_TEXT_2 "%d" COLOR_CLEAR "\n\r",
                VNUM_FIELD(faction), NAME_STR(faction), faction->default_standing);
            found = true;
        }

        if (!found)
            add_buf(buffer, COLOR_ALT_TEXT_2 "  (none)" COLOR_CLEAR "\n\r");
    }

    send_to_char(BUF(buffer), ch);
    free_buf(buffer);
}

static void aedit_show_faction_detail(Mobile* ch, Faction* faction)
{
    Buffer* buffer = new_buf();

    addf_buf(buffer,
        COLOR_DECOR_1 "[%" PRVNUM "] " COLOR_ALT_TEXT_1 "%s" COLOR_CLEAR "\n\r"
        "Default standing: " COLOR_ALT_TEXT_2 "%d" COLOR_CLEAR "\n\r",
        VNUM_FIELD(faction), NAME_STR(faction), faction->default_standing);

    format_relation_list(buffer, "Allies", &faction->allies);
    format_relation_list(buffer, "Enemies", &faction->enemies);

    send_to_char(BUF(buffer), ch);
    free_buf(buffer);
}

static void clear_mob_faction_references(VNUM vnum)
{
    MobPrototype* mob;

    FOR_EACH_MOB_PROTO(mob) {
        if (mob->faction_vnum == vnum) {
            mob->faction_vnum = 0;
            if (mob->area != NULL)
                SET_BIT(mob->area->area_flags, AREA_CHANGED);
        }
    }
}

static void aedit_send_faction_syntax(Mobile* ch)
{
    send_to_char(COLOR_INFO "Syntax:\n\r" COLOR_CLEAR, ch);
    send_to_char("  faction list\n\r", ch);
    send_to_char("  faction show <vnum|name>\n\r", ch);
    send_to_char("  faction create <vnum> <name>\n\r", ch);
    send_to_char("  faction delete <vnum|name>\n\r", ch);
    send_to_char("  faction name <vnum|name> <new name>\n\r", ch);
    send_to_char("  faction default <vnum|name> <value>\n\r", ch);
    send_to_char("  faction ally <vnum|name> <add|remove> <vnum|name>\n\r", ch);
    send_to_char("  faction enemy <vnum|name> <add|remove> <vnum|name>\n\r", ch);
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
    aedit_print_faction_summary(ch, area);
    aedit_print_story_beats(ch, area);
    aedit_print_checklist(ch, area);

    return false;
}

AEDIT(aedit_faction)
{
    AreaData* area;
    char arg1[MIL];
    char arg2[MIL];
    char arg3[MIL];
    char arg4[MIL];

    EDIT_AREA(ch, area);

    argument = one_argument(argument, arg1);

    if (IS_NULLSTR(arg1)) {
        aedit_send_faction_syntax(ch);
        return false;
    }

    if (!str_cmp(arg1, "list")) {
        aedit_list_factions(ch, area);
        return false;
    }

    if (!str_cmp(arg1, "show")) {
        argument = one_argument(argument, arg2);
        if (IS_NULLSTR(arg2)) {
            aedit_send_faction_syntax(ch);
            return false;
        }

        Faction* faction = find_area_faction(area, arg2);
        if (faction == NULL) {
            send_to_char(COLOR_INFO "That faction is not defined in this area." COLOR_EOL, ch);
            return false;
        }

        aedit_show_faction_detail(ch, faction);
        return false;
    }

    if (!str_cmp(arg1, "create")) {
        argument = one_argument(argument, arg2);
        char* name = argument;

        if (IS_NULLSTR(arg2) || IS_NULLSTR(name)) {
            aedit_send_faction_syntax(ch);
            return false;
        }

        VNUM vnum = (VNUM)atoi(arg2);
        if (vnum <= 0 || vnum < area->min_vnum || vnum > area->max_vnum) {
            send_to_char(COLOR_INFO "Faction vnum must fall within the area's vnum range." COLOR_EOL, ch);
            return false;
        }

        if (get_faction(vnum) != NULL) {
            send_to_char(COLOR_INFO "A faction with that vnum already exists." COLOR_EOL, ch);
            return false;
        }

        AreaData* prev = current_area_data;
        current_area_data = area;
        Faction* faction = faction_create(vnum);
        current_area_data = prev;

        if (faction == NULL) {
            send_to_char(COLOR_INFO "Unable to create faction." COLOR_EOL, ch);
            return false;
        }

        faction->area = area;
        SET_NAME(faction, lox_string(name));

        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char(COLOR_INFO "Faction created." COLOR_EOL, ch);
        return true;
    }

    if (!str_cmp(arg1, "delete")) {
        argument = one_argument(argument, arg2);
        if (IS_NULLSTR(arg2)) {
            aedit_send_faction_syntax(ch);
            return false;
        }

        Faction* faction = find_area_faction(area, arg2);
        if (faction == NULL) {
            send_to_char(COLOR_INFO "That faction is not defined in this area." COLOR_EOL, ch);
            return false;
        }

        VNUM vnum = VNUM_FIELD(faction);
        faction_delete(faction);
        clear_mob_faction_references(vnum);
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char(COLOR_INFO "Faction deleted." COLOR_EOL, ch);
        return true;
    }

    if (!str_cmp(arg1, "name")) {
        argument = one_argument(argument, arg2);
        char* new_name = argument;

        if (IS_NULLSTR(arg2) || IS_NULLSTR(new_name)) {
            aedit_send_faction_syntax(ch);
            return false;
        }

        Faction* faction = find_area_faction(area, arg2);
        if (faction == NULL) {
            send_to_char(COLOR_INFO "That faction is not defined in this area." COLOR_EOL, ch);
            return false;
        }

        SET_NAME(faction, lox_string(new_name));
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char(COLOR_INFO "Faction renamed." COLOR_EOL, ch);
        return true;
    }

    if (!str_cmp(arg1, "default")) {
        argument = one_argument(argument, arg2);
        argument = one_argument(argument, arg3);
        if (IS_NULLSTR(arg2) || IS_NULLSTR(arg3)) {
            aedit_send_faction_syntax(ch);
            return false;
        }

        Faction* faction = find_area_faction(area, arg2);
        if (faction == NULL) {
            send_to_char(COLOR_INFO "That faction is not defined in this area." COLOR_EOL, ch);
            return false;
        }

        int value = faction_clamp_value(atoi(arg3));
        faction->default_standing = value;
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char(COLOR_INFO "Default standing updated." COLOR_EOL, ch);
        return true;
    }

    if (!str_cmp(arg1, "ally") || !str_cmp(arg1, "enemy") || !str_cmp(arg1, "opposition") || !str_cmp(arg1, "oppose")) {
        argument = one_argument(argument, arg2);
        argument = one_argument(argument, arg3);
        argument = one_argument(argument, arg4);

        if (IS_NULLSTR(arg2) || IS_NULLSTR(arg3) || IS_NULLSTR(arg4)) {
            aedit_send_faction_syntax(ch);
            return false;
        }

        bool is_enemy = str_cmp(arg1, "ally");
        bool add = false;
        if (!str_cmp(arg3, "add"))
            add = true;
        else if (!str_cmp(arg3, "remove") || !str_cmp(arg3, "delete"))
            add = false;
        else {
            send_to_char(COLOR_INFO "Specify 'add' or 'remove'." COLOR_EOL, ch);
            return false;
        }

        Faction* faction = find_area_faction(area, arg2);
        if (faction == NULL) {
            send_to_char(COLOR_INFO "That faction is not defined in this area." COLOR_EOL, ch);
            return false;
        }

        Faction* relation = find_faction_any(arg4);
        if (relation == NULL) {
            send_to_char(COLOR_INFO "The specified faction does not exist." COLOR_EOL, ch);
            return false;
        }

        if (faction == relation) {
            send_to_char(COLOR_INFO "A faction cannot reference itself." COLOR_EOL, ch);
            return false;
        }

        bool changed = false;
        if (!is_enemy) {
            changed = add
                ? faction_add_ally(faction, VNUM_FIELD(relation))
                : faction_remove_ally(faction, VNUM_FIELD(relation));
        }
        else {
            changed = add
                ? faction_add_enemy(faction, VNUM_FIELD(relation))
                : faction_remove_enemy(faction, VNUM_FIELD(relation));
        }

        if (!changed) {
            send_to_char(COLOR_INFO "No changes were made to that relationship." COLOR_EOL, ch);
            return false;
        }

        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char(COLOR_INFO "Faction relationships updated." COLOR_EOL, ch);
        return true;
    }

    aedit_send_faction_syntax(ch);
    return false;
}

AEDIT(aedit_story)
{
    AreaData* area;
    char arg1[MIL];
    char arg2[MIL];

    EDIT_AREA(ch, area);

    argument = one_argument(argument, arg1);
    if (IS_NULLSTR(arg1)) {
        send_to_char("Syntax: story list\n\r"
            "        story add <title> [description]\n\r"
            "        story title <index> <title>\n\r"
            "        story desc <index> <description>\n\r"
            "        story del <index>\n\r"
            "        (indexes are 1-based)\n\r", ch);
        return false;
    }

    if (!str_cmp(arg1, "list")) {
        aedit_print_story_beats(ch, area);
        return false;
    }

    if (!str_cmp(arg1, "add")) {
        argument = first_arg(argument, arg2, false); // preserve case for title
        if (IS_NULLSTR(arg2)) {
            send_to_char("Story add requires a title.\n\r", ch);
            return false;
        }
        add_story_beat(area, arg2, argument);
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char("Story beat added.\n\r", ch);
        return true;
    }

    if (!str_cmp(arg1, "title") || !str_cmp(arg1, "desc")) {
        argument = one_argument(argument, arg2);
        if (!is_number(arg2) || IS_NULLSTR(argument)) {
            send_to_char("Syntax: story title|desc <index> <text>\n\r", ch);
            return false;
        }
        int idx = atoi(arg2) - 1;
        if (idx < 0) {
            send_to_char("No such story beat.\n\r", ch);
            return false;
        }
        StoryBeat* beat = story_beat_by_index(area, idx, NULL);
        if (!beat) {
            send_to_char("No such story beat.\n\r", ch);
            return false;
        }
        if (!str_cmp(arg1, "title")) {
            free_string(beat->title);
            beat->title = str_dup(argument);
        }
        else {
            free_string(beat->description);
            beat->description = str_dup(argument);
        }
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char("Story beat updated.\n\r", ch);
        return true;
    }

    if (!str_cmp(arg1, "del")) {
        argument = one_argument(argument, arg2);
        if (!is_number(arg2)) {
            send_to_char("Syntax: story del <index>\n\r", ch);
            return false;
        }
        int idx = atoi(arg2) - 1;
        if (idx < 0) {
            send_to_char("No such story beat.\n\r", ch);
            return false;
        }
        StoryBeat* prev = NULL;
        StoryBeat* beat = story_beat_by_index(area, idx, &prev);
        if (!beat) {
            send_to_char("No such story beat.\n\r", ch);
            return false;
        }
        if (prev)
            prev->next = beat->next;
        else
            area->story_beats = beat->next;
        free_string(beat->title);
        free_string(beat->description);
        free(beat);
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char("Story beat removed.\n\r", ch);
        return true;
    }

    send_to_char("Unknown story command.\n\r", ch);
    return false;
}

AEDIT(aedit_checklist)
{
    AreaData* area;
    char arg1[MIL];
    char arg2[MIL];

    EDIT_AREA(ch, area);

    argument = one_argument(argument, arg1);
    if (IS_NULLSTR(arg1)) {
        send_to_char("Syntax: checklist list\n\r"
            "        checklist add [status] <title> [description]\n\r"
            "        checklist title <index> <title>\n\r"
            "        checklist desc <index> <description>\n\r"
            "        checklist status <index> <todo|progress|done>\n\r"
            "        checklist del <index>\n\r"
            "        checklist clear\n\r"
            "        (indexes are 1-based)\n\r", ch);
        return false;
    }

    if (!str_cmp(arg1, "list")) {
        aedit_print_checklist(ch, area);
        return false;
    }

    if (!str_cmp(arg1, "add")) {
        argument = one_argument(argument, arg2);
        ChecklistStatus status = checklist_status_from_name(arg2, CHECK_TODO);
        const char* title = NULL;
        if (status == CHECK_TODO) {
            title = arg2;
        }
        else {
            title = argument;
        }

        if (IS_NULLSTR(title)) {
            send_to_char("Checklist add requires a title.\n\r", ch);
            return false;
        }

        add_checklist_item(area, title, argument, status);
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char("Checklist item added.\n\r", ch);
        return true;
    }

    if (!str_cmp(arg1, "clear")) {
        free_checklist(area->checklist);
        area->checklist = NULL;
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char("Checklist cleared.\n\r", ch);
        return true;
    }

    if (!str_cmp(arg1, "title") || !str_cmp(arg1, "desc") || !str_cmp(arg1, "status")) {
        argument = one_argument(argument, arg2);
        if (!is_number(arg2)) {
            send_to_char("Checklist requires a numeric index.\n\r", ch);
            return false;
        }
        int idx = atoi(arg2) - 1;
        if (idx < 0) {
            send_to_char("No such checklist item.\n\r", ch);
            return false;
        }
        ChecklistItem* item = checklist_by_index(area, idx, NULL);
        if (!item) {
            send_to_char("No such checklist item.\n\r", ch);
            return false;
        }

        if (!str_cmp(arg1, "status")) {
            ChecklistStatus status = checklist_status_from_name(argument, item->status);
            item->status = status;
            SET_BIT(area->area_flags, AREA_CHANGED);
            send_to_char("Checklist status updated.\n\r", ch);
            return true;
        }

        if (IS_NULLSTR(argument)) {
            send_to_char("Checklist title/desc requires text.\n\r", ch);
            return false;
        }

        if (!str_cmp(arg1, "title")) {
            free_string(item->title);
            item->title = str_dup(argument);
        }
        else {
            free_string(item->description);
            item->description = str_dup(argument);
        }
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char("Checklist item updated.\n\r", ch);
        return true;
    }

    if (!str_cmp(arg1, "del")) {
        argument = one_argument(argument, arg2);
        if (!is_number(arg2)) {
            send_to_char("Syntax: checklist del <index>\n\r", ch);
            return false;
        }
        int idx = atoi(arg2) - 1;
        if (idx < 0) {
            send_to_char("No such checklist item.\n\r", ch);
            return false;
        }
        ChecklistItem* prev = NULL;
        ChecklistItem* item = checklist_by_index(area, idx, &prev);
        if (!item) {
            send_to_char("No such checklist item.\n\r", ch);
            return false;
        }
        if (prev)
            prev->next = item->next;
        else
            area->checklist = item->next;
        free_string(item->title);
        free_string(item->description);
        free(item);
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char("Checklist item removed.\n\r", ch);
        return true;
    }

    send_to_char("Unknown checklist command.\n\r", ch);
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
    aedit_seed_default_checklist(area_data);

    set_editor(ch->desc, ED_AREA, U(area_data));

    SET_BIT(area_data->area_flags, AREA_ADDED);
    send_to_char("Area Created.\n\r", ch);
    return true;
}

static void aedit_seed_default_checklist(AreaData* area)
{
    static const char* default_checklist[] = {
        "Write-up story beats",
        "Sketch out rooms",
        "Populate mobiles",
        "Populate objects",
        "Add descriptions",
        "Wire-up events",
    };

    for (size_t i = 0; i < sizeof(default_checklist) / sizeof(default_checklist[0]); ++i) {
        add_checklist_item(area, default_checklist[i], "", CHECK_TODO);
    }
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
    const char* def_fmt = cfg_get_default_format();
    const char* ext = (def_fmt && !str_cmp(def_fmt, "json")) ? ".json" : ".are";
    strcat(file, ext);
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

    area->low_range = lower;
    area->high_range = upper;

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

    addf_buf(result, COLOR_DECOR_1 "[" COLOR_TITLE "%3s" COLOR_DECOR_1 "] [" COLOR_TITLE "%-22s" COLOR_DECOR_1 "] (" COLOR_TITLE "%6s" COLOR_DECOR_1 "-" COLOR_TITLE "%-6s" COLOR_DECOR_1 ") " COLOR_DECOR_1 "[" COLOR_TITLE "%-11s" COLOR_DECOR_1 "] " COLOR_TITLE "%3s " COLOR_DECOR_1 "[" COLOR_TITLE "%-10s" COLOR_DECOR_1 "]" COLOR_EOL,
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
    //FOR_EACH_AREA(area) {
    for (int i = 0; i < global_areas.count; ++i) {
        area = alist[i];
        addf_buf( result, COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%3d" COLOR_DECOR_1 "]" COLOR_CLEAR " %-24.24s " COLOR_DECOR_1 "(" COLOR_ALT_TEXT_1 "%6d" COLOR_DECOR_1 "-" COLOR_ALT_TEXT_1 "%-6d" COLOR_DECOR_1 ") " COLOR_ALT_TEXT_2 "%-13.13s " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%d" COLOR_DECOR_1 "] [" COLOR_ALT_TEXT_1 "%-10.10s" COLOR_DECOR_1 "]" COLOR_EOL,
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
