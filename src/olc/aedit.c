////////////////////////////////////////////////////////////////////////////////
// aedit.c
////////////////////////////////////////////////////////////////////////////////

#include <merc.h>

#include "bit.h"
#include "event_edit.h"
#include "lox_edit.h"
#include "loot_edit.h"
#include "olc.h"
#include "period_edit.h"
#include "string_edit.h"

#include <craft/recipe.h>
#include <craft/recedit.h>

#include <array.h>
#include <color.h>
#include <comm.h>
#include <config.h>
#include <db.h>
#include <format.h>
#include <handler.h>
#include <lookup.h>
#include <magic.h>
#include <recycle.h>
#include <save.h>
#include <stringbuffer.h>
#include <stringutils.h>
#include <tables.h>

#include <entities/area.h>
#include <entities/faction.h>
#include <entities/mobile.h>
#include <entities/mob_prototype.h>
#include <entities/object.h>
#include <entities/obj_prototype.h>
#include <entities/player_data.h>
#include <entities/reset.h>
#include <entities/room.h>
#include <entities/room_exit.h>
#include <entities/shop_data.h>

#include <data/loot.h>
#include <data/class.h>
#include <data/quest.h>
#include <data/race.h>
#include <mob_prog.h>

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
static int count_area_recipes(AreaData* area);

AEDIT(aedit_story);
AEDIT(aedit_checklist);
AEDIT(aedit_period);
AEDIT(aedit_recipe);
AEDIT(aedit_movevnums);

const OlcCmdEntry area_olc_comm_table[] = {
    { "name", 	        U(&xArea.header.name),  ed_line_lox_string, 0                   },
    { "min_vnum", 	    0,                      ed_olded,           U(aedit_lvnum)	    },
    { "max_vnum", 	    0,                      ed_olded,           U(aedit_uvnum)	    },
    { "min_level", 	    U(&xArea.low_range),    ed_number_level,    0	                },
    { "max_level", 	    U(&xArea.high_range),   ed_number_level,    0	                },
    { "reset", 		    U(&xArea.reset_thresh), ed_number_s_pos,    0	                },
    { "sector",	        U(&xArea.sector),	    ed_flag_set_sh,		U(sector_flag_table)},
    { "credits", 	    U(&xArea.credits),      ed_line_string,     0                   },
    { "loot_table",     U(&xArea.loot_table),   ed_loot_string,     0                   },
    { "alwaysreset",    U(&xArea.always_reset), ed_bool,            0                   },
    { "instancetype",   U(&xArea.inst_type),    ed_flag_set_sh,     U(inst_type_table)  },
    { "period",         0,                      ed_olded,           U(aedit_period)     },
    { "event",          0,                      ed_olded,           U(olc_edit_event)   },
    { "lox",            0,                      ed_olded,           U(olc_edit_lox)     },
    { "loot_edit",      0,                      ed_olded,           U(olc_edit_loot)    },
    { "story",          0,                      ed_olded,           U(aedit_story)      },
    { "checklist",      0,                      ed_olded,           U(aedit_checklist)  },
    { "period",         0,                      ed_olded,           U(aedit_period)     },
    { "faction",        0,                      ed_olded,           U(aedit_faction)    },
    { "gather",         0,                      ed_olded,           U(aedit_gather)     },
    { "recipe",         0,                      ed_olded,           U(aedit_recipe)     },
    { "builder", 	    0,                      ed_olded,           U(aedit_builder)	},
    { "commands", 	    0,                      ed_olded,           U(show_commands)	},
    { "create", 	    0,                      ed_olded,           U(aedit_create)	    },
    { "filename", 	    0,                      ed_olded,           U(aedit_file)	    },
    { "reset", 	        0,                      ed_olded,           U(aedit_reset)	    },
    { "security", 	    0,                      ed_olded,           U(aedit_security)	},
    { "show", 	        0,                      ed_olded,           U(aedit_show)	    },
    { "vnums", 	        0,                      ed_olded,           U(aedit_vnums)	    },
    { "movevnums",      0,                      ed_olded,           U(aedit_movevnums) },
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
            send_to_char(COLOR_INFO "That area vnum does not exist." COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "You do not have enough security to edit areas." COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "Insufficient security to modify area." COLOR_EOL, ch);
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
    case CHECK_TODO: return "^wTo-Do";
    case CHECK_IN_PROGRESS: return "^YIn Progress";
    case CHECK_DONE: return "^GDone";
    default: return "^DUnknown";
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
    if (!area->story_beats) {
        olc_print_str_box(ch, "Story Beats", "(none)", "Type '" COLOR_TITLE "STORYBEAT" COLOR_ALT_TEXT_2 "' to create one.");
        return;
    }
    send_to_char(COLOR_TEXT "Story Beats:" COLOR_EOL, ch);

    int idx = 0;
    for (StoryBeat* beat = area->story_beats; beat; beat = beat->next, ++idx) {
        printf_to_char(ch, COLOR_ALT_TEXT_1 "  %d)" COLOR_TITLE " %s" COLOR_EOL, idx + 1, beat->title);
        if (!IS_NULLSTR(beat->description))
            printf_to_char(ch, COLOR_ALT_TEXT_2"     %s" COLOR_EOL, beat->description);
    }
}

static void aedit_print_checklist(Mobile* ch, AreaData* area)
{    if (!area->checklist) {
        olc_print_str_box(ch, "Checklist", "(none)", "Type '" COLOR_TITLE "CHECKLIST" COLOR_ALT_TEXT_2 "' to create one.");
        return;
    }
    send_to_char(COLOR_TEXT "Checklist:" COLOR_EOL, ch);

    int idx = 0;
    for (ChecklistItem* item = area->checklist; item; item = item->next, ++idx) {
        printf_to_char(ch, COLOR_ALT_TEXT_1 "  %d) " COLOR_DECOR_1 
            "[" COLOR_INFO "%s" COLOR_DECOR_1 "] " COLOR_TITLE "%s\n\r", 
            idx + 1, checklist_status_name(item->status), item->title);
        if (!IS_NULLSTR(item->description))
            printf_to_char(ch, COLOR_ALT_TEXT_2 "     %s" COLOR_EOL, item->description);
    }
}

static void aedit_print_faction_summary(Mobile* ch, AreaData* area)
{
    Buffer* buffer = new_buf();
    bool found = false;

    add_buf(buffer, COLOR_TEXT "Factions:" COLOR_CLEAR "\n\r");

    if (faction_table.capacity > 0 && faction_table.entries != NULL) {
        for (int idx = 0; idx < faction_table.capacity; ++idx) {
            Entry* entry = &faction_table.entries[idx];
            if (IS_NIL(entry->value) || !IS_FACTION(entry->value))
                continue;

            Faction* faction = AS_FACTION(entry->value);
            if (faction->area != area)
                continue;

            addf_buf(buffer,
                COLOR_DECOR_1 "  [" COLOR_ALT_TEXT_1 "%" PRVNUM COLOR_DECOR_1 "] " COLOR_TITLE "%-16s"
                COLOR_INFO " Default Rep: " COLOR_ALT_TEXT_1 "%-6d"
                COLOR_INFO " Allies: " COLOR_ALT_TEXT_1 "%-2d"
                COLOR_INFO " Enemies: " COLOR_ALT_TEXT_1 "%-2d" COLOR_EOL,
                VNUM_FIELD(faction),
                NAME_STR(faction),
                faction->default_standing,
                faction->allies.count,
                faction->enemies.count);
            found = true;
        }
    }

    if (!found)
        olc_print_str_box(ch, "Factions", "(none)", "Type '" COLOR_TITLE "FACTION" COLOR_ALT_TEXT_2 "' to create one.");
    else
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
    olc_print_str(ch, "Loot Table", area->loot_table ? area->loot_table : "(none)");
    
    Entity* entity = &area->header;
    olc_display_entity_class_info(ch, entity);
    olc_display_events(ch, entity);

    olc_show_periods(ch, &area->header);
    olc_print_yesno_ex(ch, "Day-cycle Msgs", !area->suppress_daycycle_messages,
        !area->suppress_daycycle_messages ? "Day-cycle messages are shown." :
        "Day-cycle messages are suppressed.");

    // Recipe count
    int rcount = count_area_recipes(area);
    olc_print_num_str(ch, "Recipes", rcount, rcount > 0 ? "" : "Type '" COLOR_TITLE "RECIPE" COLOR_ALT_TEXT_2 "' to add some.");

    olc_show_gather_spawns(ch, area);

    aedit_print_faction_summary(ch, area);
    aedit_print_story_beats(ch, area);
    aedit_print_checklist(ch, area);

    olc_print_flags(ch, "Flags", area_flag_table, area->area_flags);

    return false;
}

AEDIT(aedit_period)
{
    AreaData* area;
    EDIT_AREA(ch, area);
    return olc_edit_period(ch, argument, &area->header /*, get_period_ops_for_area()*/);
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

AEDIT(aedit_gather)
{
    AreaData* area;
    EDIT_AREA(ch, area);

    char first_arg[MIL];
    argument = one_argument(argument, first_arg);

    if (IS_NULLSTR(first_arg) || !str_prefix(first_arg, "list") || !str_prefix(first_arg, "show")) {
        if (area->gather_spawns.count == 0) {
            send_to_char(COLOR_INFO "No gather spawns defined for this area." COLOR_EOL, ch);
            send_to_char(COLOR_INFO "Use 'gather help' to see gather subcommands." COLOR_EOL, ch);
            return false;
        }
    }
    else if (!str_prefix(first_arg, "help")) {
        send_to_char(COLOR_INFO "Gather subcommands:\n\r" COLOR_CLEAR, ch);
        send_to_char("  gather list|show\n\r", ch);
        send_to_char("  gather add <vnum> <count> [sector] [respawn_timer]\n\r", ch);
        send_to_char("  gather remove <index>\n\r", ch);
        send_to_char("  gather clear\n\r", ch);
        return false;
    }
    else if (!str_prefix(first_arg, "list") || !str_prefix(first_arg, "show")) {
        olc_show_gather_spawns(ch, area);
        return false;
    }
    else if (!str_prefix(first_arg, "add")) {
        static const char* gather_add_syntax =
        COLOR_INFO "Syntax: gather add <vnum> <count> [sector] [respawn_timer] " COLOR_EOL;
        char vnum_arg[MIL];
        char count_arg[MIL];
        char sector_arg[MIL];
        char timer_arg[MIL];

        int reset = 6;
        int sector_type = area->sector;

        argument = one_argument(argument, vnum_arg);
        argument = one_argument(argument, count_arg);
        argument = one_argument(argument, sector_arg);
        argument = one_argument(argument, timer_arg);

        if (!is_number(vnum_arg) || !is_number(count_arg)) {
            send_to_char(gather_add_syntax, ch);
            return false;
        }

        VNUM vnum = (VNUM)strtol(vnum_arg, NULL, 10);
        int count = strtol(count_arg, NULL, 10);
        if (count < 1) {
            send_to_char(COLOR_INFO "Count must be at least 1." COLOR_EOL, ch);
            return false;
        }

        if (!IS_NULLSTR(sector_arg)) {
            int sector = flag_value(sector_flag_table, sector_arg);
            if (sector == NO_FLAG) {
                send_to_char(COLOR_INFO "Invalid sector type." COLOR_EOL, ch);
                send_to_char(gather_add_syntax, ch);
                return false;
            }
            sector_type = sector;
        }

        if (!IS_NULLSTR(timer_arg)) {
            if (!is_number(timer_arg)) {
                send_to_char(COLOR_INFO "Respawn timer must be a number." COLOR_EOL, ch);
                send_to_char(gather_add_syntax, ch);
                return false;
            }
            reset = atoi(timer_arg);
            if (reset < 1) {
                send_to_char(COLOR_INFO "Respawn timer must be at least 1." COLOR_EOL, ch);
                send_to_char(gather_add_syntax, ch);
                return false;
            }
        }

        add_gather_spawn(&area->gather_spawns, sector_type, vnum, count, reset);
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char(COLOR_INFO "Gather spawn added." COLOR_EOL, ch);
        return true;
    }
    else if (!str_prefix(first_arg, "remove")) {
        // This won't take effect until the area resets, but that's acceptable.
        char arg2[MIL];
        argument = one_argument(argument, arg2);

        if (!is_number(arg2)) {
            send_to_char(COLOR_INFO "Syntax: gather remove <index>" COLOR_EOL, ch);
            return false;
        }

        int index = strtol(arg2, NULL, 10) - 1;
        if (index < 0 || index >= (int)area->gather_spawns.count) {
            send_to_char(COLOR_INFO "No such gather spawn." COLOR_EOL, ch);
            return false;
        }
        
        for (int i = index; i < (int)area->gather_spawns.count - 1; ++i) {
            area->gather_spawns.spawns[i] = area->gather_spawns.spawns[i + 1];
        }
        area->gather_spawns.count--;

        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char(COLOR_INFO "Gather spawn removed." COLOR_EOL, ch);
        return true;
    }
    else if (!str_prefix(first_arg, "clear")) {
        // This won't take effect until the area resets, but that's acceptable.
        if (area->gather_spawns.count == 0) {
            send_to_char(COLOR_INFO "No gather spawns to clear." COLOR_EOL, ch);
            return false;
        }
        
        free_gather_spawn_array(&area->gather_spawns);

        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char(COLOR_INFO "All gather spawns cleared." COLOR_EOL, ch);
        return true;
    }

    send_to_char(COLOR_INFO "Unknown gather command. Use 'gather help' for syntax." COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "Syntax: checklist list\n\r"
            "        checklist add [status] <title> [description]\n\r"
            "        checklist title <index> <title>\n\r"
            "        checklist desc <index> <description>\n\r"
            "        checklist status <index> <todo|progress|done>\n\r"
            "        checklist del <index>\n\r"
            "        checklist clear\n\r"
            "        (indexes are 1-based)" COLOR_EOL, ch);
        return false;
    }

    if (!str_prefix(arg1, "list")) {
        aedit_print_checklist(ch, area);
        return false;
    }

    if (!str_prefix(arg1, "add")) {
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
            send_to_char(COLOR_INFO "Checklist add requires a title." COLOR_EOL, ch);
            return false;
        }

        add_checklist_item(area, title, argument, status);
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char(COLOR_INFO "Checklist item added." COLOR_EOL, ch);
        return true;
    }

    if (!str_cmp(arg1, "clear")) {
        free_checklist(area->checklist);
        area->checklist = NULL;
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char(COLOR_INFO "Checklist cleared." COLOR_EOL, ch);
        return true;
    }

    if (!str_prefix(arg1, "title") || !str_prefix(arg1, "desc") || !str_prefix(arg1, "status")) {
        argument = one_argument(argument, arg2);
        if (!is_number(arg2)) {
            send_to_char(COLOR_INFO "Checklist requires a numeric index." COLOR_EOL, ch);
            return false;
        }
        int idx = atoi(arg2) - 1;
        if (idx < 0) {
            send_to_char(COLOR_INFO "No such checklist item." COLOR_EOL, ch);
            return false;
        }
        ChecklistItem* item = checklist_by_index(area, idx, NULL);
        if (!item) {
            send_to_char(COLOR_INFO "No such checklist item." COLOR_EOL, ch);
            return false;
        }

        if (!str_prefix(arg1, "status")) {
            ChecklistStatus status = checklist_status_from_name(argument, item->status);
            item->status = status;
            SET_BIT(area->area_flags, AREA_CHANGED);
            send_to_char(COLOR_INFO "Checklist status updated." COLOR_EOL, ch);
            return true;
        }

        if (IS_NULLSTR(argument)) {
            send_to_char(COLOR_INFO "Checklist title/desc requires text." COLOR_EOL, ch);
            return false;
        }

        if (!str_prefix(arg1, "title")) {
            free_string(item->title);
            item->title = str_dup(argument);
        }
        else {
            free_string(item->description);
            item->description = str_dup(argument);
        }
        SET_BIT(area->area_flags, AREA_CHANGED);
        send_to_char(COLOR_INFO "Checklist item updated." COLOR_EOL, ch);
        return true;
    }

    if (!str_cmp(arg1, "del")) {
        argument = one_argument(argument, arg2);
        if (!is_number(arg2)) {
            send_to_char(COLOR_INFO "Syntax: checklist del <index>" COLOR_EOL, ch);
            return false;
        }
        int idx = atoi(arg2) - 1;
        if (idx < 0) {
            send_to_char(COLOR_INFO "No such checklist item." COLOR_EOL, ch);
            return false;
        }
        ChecklistItem* prev = NULL;
        ChecklistItem* item = checklist_by_index(area, idx, &prev);
        if (!item) {
            send_to_char(COLOR_INFO "No such checklist item." COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "Checklist item removed." COLOR_EOL, ch);
        return true;
    }

    send_to_char(COLOR_INFO "Unknown checklist command." COLOR_EOL, ch);
    return false;
}

AEDIT(aedit_reset)
{
    AreaData* area_data;

    EDIT_AREA(ch, area_data);

    Area* area;

    FOR_EACH_AREA_INST(area, area_data)
        reset_area(area);
    send_to_char(COLOR_INFO "Area reset." COLOR_EOL, ch);

    return false;
}

AEDIT(aedit_create)
{
    AreaData* area_data;

    if (IS_NPC(ch) || ch->pcdata->security < MIN_AEDIT_SECURITY) {
        send_to_char(COLOR_INFO "You do not have enough security to edit areas." COLOR_EOL, ch);
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
    send_to_char(COLOR_INFO "Area Created." COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "Syntax:  filename <file>" COLOR_EOL, ch);
        return false;
    }

    // Simple Syntax Check.
    length = strlen(argument);
    if (length > 8) {
        send_to_char(COLOR_INFO "No more than eight characters allowed." COLOR_EOL, ch);
        return false;
    }

    // Allow only letters and numbers.
    for (i = 0; i < (int)length; i++) {
        if (!ISALNUM(file[i])) {
            send_to_char(COLOR_INFO "Only letters and numbers are valid." COLOR_EOL, ch);
            return false;
        }
    }

    free_string(area->file_name);
    const char* def_fmt = cfg_get_default_format();
    const char* ext = (def_fmt && !str_cmp(def_fmt, "json")) ? ".json" : ".are";
    strcat(file, ext);
    area->file_name = str_dup(file);

    send_to_char(COLOR_INFO "Filename set." COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "Syntax:  levels <lower> <upper>" COLOR_EOL, ch);
        return false;
    }

    if ((lower = (LEVEL)atoi(lower_str)) > (upper = (LEVEL)atoi(upper_str))) {
        send_to_char(COLOR_INFO "Upper must be larger than lower." COLOR_EOL, ch);
        return false;
    }

    area->low_range = lower;
    area->high_range = upper;

    printf_to_char(ch, COLOR_INFO "Level range set to %d-%d." COLOR_EOL, lower, upper);

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
        send_to_char(COLOR_INFO "Syntax:  security <level>" COLOR_EOL, ch);
        return false;
    }

    value = atoi(sec);

    if (value > ch->pcdata->security || value < 0) {
        if (ch->pcdata->security != 0) {
            sprintf(buf, COLOR_INFO "Security is 0-%d." COLOR_EOL, ch->pcdata->security);
            send_to_char(buf, ch);
        }
        else
            send_to_char(COLOR_INFO "Security is 0 only." COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "Syntax:  builder [$name]  - toggles builder" COLOR_EOL, ch);
        send_to_char(COLOR_INFO "Syntax:  builder All      - allows everyone" COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "Builder removed." COLOR_EOL, ch);
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

    send_to_char(COLOR_INFO "Builder added." COLOR_EOL, ch);
    send_to_char(area->builders, ch);
    send_to_char(COLOR_EOL, ch);
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

static void mark_area_changed(AreaData* area)
{
    if (area != NULL)
        SET_BIT(area->area_flags, AREA_CHANGED);
}

static bool range_overlaps_other_areas(AreaData* area, VNUM lower, VNUM upper)
{
    AreaData* other;

    FOR_EACH_AREA(other) {
        if (other == area)
            continue;

        if (lower <= other->max_vnum && upper >= other->min_vnum)
            return true;
    }

    return false;
}

static bool vnum_in_shift_range(VNUM vnum, VNUM lower, VNUM upper)
{
    return vnum >= lower && vnum <= upper;
}

static VNUM shift_vnum_value(VNUM vnum, VNUM lower, VNUM upper, int32_t delta)
{
    if (vnum <= 0)
        return vnum;

    if (!vnum_in_shift_range(vnum, lower, upper))
        return vnum;

    return (VNUM)(vnum + delta);
}

static bool shift_vnum_field(VNUM* field, VNUM lower, VNUM upper, int32_t delta)
{
    VNUM old_vnum = *field;
    VNUM new_vnum = shift_vnum_value(old_vnum, lower, upper, delta);

    if (new_vnum == old_vnum)
        return false;

    *field = new_vnum;
    return true;
}

static bool shift_int_vnum_field(int* field, VNUM lower, VNUM upper, int32_t delta)
{
    VNUM old_vnum = (VNUM)*field;
    VNUM new_vnum = shift_vnum_value(old_vnum, lower, upper, delta);

    if (new_vnum == old_vnum)
        return false;

    *field = (int)new_vnum;
    return true;
}

static bool shift_vnum_array(VNUM* list, int count, VNUM lower, VNUM upper, int32_t delta)
{
    bool changed = false;

    if (list == NULL || count <= 0)
        return false;

    for (int i = 0; i < count; ++i) {
        VNUM new_vnum = shift_vnum_value(list[i], lower, upper, delta);
        if (new_vnum != list[i]) {
            list[i] = new_vnum;
            changed = true;
        }
    }

    return changed;
}

static bool shift_value_array_vnums(ValueArray* array, VNUM lower, VNUM upper, int32_t delta)
{
    bool changed = false;

    if (array == NULL || array->count <= 0)
        return false;

    for (int i = 0; i < array->count; ++i) {
        Value entry = array->values[i];
        if (!IS_INT(entry))
            continue;

        VNUM old_vnum = (VNUM)AS_INT(entry);
        VNUM new_vnum = shift_vnum_value(old_vnum, lower, upper, delta);
        if (new_vnum != old_vnum) {
            array->values[i] = INT_VAL(new_vnum);
            changed = true;
        }
    }

    return changed;
}

static void set_entity_vnum(Entity* entity, VNUM vnum)
{
    entity->vnum = vnum;
    SET_NATIVE_FIELD(entity, entity->vnum, vnum, I32);
}

static AreaData* loot_owner_area_data(Entity* owner)
{
    if (owner == NULL)
        return NULL;

    switch (owner->obj.type) {
    case OBJ_AREA_DATA:
        return (AreaData*)owner;
    case OBJ_MOB_PROTO:
        return ((MobPrototype*)owner)->area;
    case OBJ_OBJ_PROTO:
        return ((ObjPrototype*)owner)->area;
    case OBJ_ROOM_DATA:
        return ((RoomData*)owner)->area_data;
    default:
        return NULL;
    }
}

static QuestTarget* shift_quest_target_list(QuestTarget* head, VNUM lower, VNUM upper, int32_t delta, bool* changed)
{
    QuestTarget* new_head = NULL;
    QuestTarget* current = head;

    while (current != NULL) {
        QuestTarget* next = current->next;
        bool updated = false;

        updated |= shift_vnum_field(&current->quest_vnum, lower, upper, delta);
        updated |= shift_vnum_field(&current->target_vnum, lower, upper, delta);
        updated |= shift_vnum_field(&current->target_upper, lower, upper, delta);

        if (updated && changed != NULL)
            *changed = true;

        current->next = NULL;
        ORDERED_INSERT(QuestTarget, current, new_head, target_vnum);
        current = next;
    }

    return new_head;
}

static QuestStatus* shift_quest_status_list(QuestStatus* head, VNUM lower, VNUM upper, int32_t delta, bool* changed)
{
    QuestStatus* new_head = NULL;
    QuestStatus* current = head;

    while (current != NULL) {
        QuestStatus* next = current->next;
        bool updated = false;

        updated |= shift_vnum_field(&current->vnum, lower, upper, delta);

        if (updated && changed != NULL)
            *changed = true;

        current->next = NULL;
        ORDERED_INSERT(QuestStatus, current, new_head, vnum);
        current = next;
    }

    return new_head;
}

static Quest* shift_quest_list(Quest* head, VNUM lower, VNUM upper, int32_t delta, bool* changed)
{
    Quest* new_head = NULL;
    Quest* current = head;

    while (current != NULL) {
        Quest* next = current->next;
        bool updated = false;

        updated |= shift_vnum_field(&current->vnum, lower, upper, delta);
        updated |= shift_vnum_field(&current->end, lower, upper, delta);
        updated |= shift_vnum_field(&current->target, lower, upper, delta);
        updated |= shift_vnum_field(&current->target_upper, lower, upper, delta);
        updated |= shift_vnum_field(&current->reward_faction_vnum, lower, upper, delta);
        updated |= shift_vnum_array(current->reward_obj_vnum, QUEST_MAX_REWARD_ITEMS,
            lower, upper, delta);

        if (updated && changed != NULL)
            *changed = true;

        current->next = NULL;
        ORDERED_INSERT(Quest, current, new_head, vnum);
        current = next;
    }

    return new_head;
}

static void recompute_top_vnums(void)
{
    RoomData* room_data;
    MobPrototype* mob_proto;
    ObjPrototype* obj_proto;

    top_vnum_room = 0;
    FOR_EACH_GLOBAL_ROOM(room_data) {
        if (VNUM_FIELD(room_data) > top_vnum_room)
            top_vnum_room = VNUM_FIELD(room_data);
    }

    top_vnum_mob = 0;
    FOR_EACH_MOB_PROTO(mob_proto) {
        if (VNUM_FIELD(mob_proto) > top_vnum_mob)
            top_vnum_mob = VNUM_FIELD(mob_proto);
    }

    top_vnum_obj = 0;
    FOR_EACH_OBJ_PROTO(obj_proto) {
        if (VNUM_FIELD(obj_proto) > top_vnum_obj)
            top_vnum_obj = VNUM_FIELD(obj_proto);
    }
}

static void aedit_apply_movevnums(AreaData* area, VNUM old_min, VNUM old_max, int32_t delta)
{
    ValueArray room_data_to_rekey;
    ValueArray mob_proto_to_rekey;
    ValueArray obj_proto_to_rekey;
    ValueArray recipe_to_rekey;
    ValueArray faction_to_rekey;

    init_value_array(&room_data_to_rekey);
    init_value_array(&mob_proto_to_rekey);
    init_value_array(&obj_proto_to_rekey);
    init_value_array(&recipe_to_rekey);
    init_value_array(&faction_to_rekey);
    (void)area;

    RoomData* room_data;
    FOR_EACH_GLOBAL_ROOM(room_data) {
        bool changed = false;

        if (vnum_in_shift_range(VNUM_FIELD(room_data), old_min, old_max))
            write_value_array(&room_data_to_rekey, OBJ_VAL(room_data));

        for (int door = 0; door < DIR_MAX; ++door) {
            RoomExitData* exit_data = room_data->exit_data[door];
            if (exit_data == NULL)
                continue;

            changed |= shift_vnum_field(&exit_data->to_vnum, old_min, old_max, delta);
            changed |= shift_vnum_field(&exit_data->key, old_min, old_max, delta);
        }

        for (Reset* reset = room_data->reset_first; reset != NULL; reset = reset->next) {
            switch (reset->command) {
            case 'M':
            case 'O':
            case 'P':
                changed |= shift_vnum_field(&reset->arg1, old_min, old_max, delta);
                changed |= shift_vnum_field(&reset->arg3, old_min, old_max, delta);
                break;
            case 'G':
            case 'E':
            case 'D':
            case 'R':
                changed |= shift_vnum_field(&reset->arg1, old_min, old_max, delta);
                break;
            default:
                break;
            }
        }

        if (changed)
            mark_area_changed(room_data->area_data);
    }

    for (MobProgCode* prog = mprog_list; prog != NULL; prog = prog->next) {
        if (shift_vnum_field(&prog->vnum, old_min, old_max, delta))
            prog->changed = true;
    }

    MobPrototype* mob_proto;
    FOR_EACH_MOB_PROTO(mob_proto) {
        bool changed = false;

        if (vnum_in_shift_range(VNUM_FIELD(mob_proto), old_min, old_max))
            write_value_array(&mob_proto_to_rekey, OBJ_VAL(mob_proto));

        changed |= shift_vnum_field(&mob_proto->faction_vnum, old_min, old_max, delta);
        changed |= shift_vnum_field(&mob_proto->group, old_min, old_max, delta);
        changed |= shift_vnum_array(mob_proto->craft_mats, mob_proto->craft_mat_count,
            old_min, old_max, delta);

        for (MobProg* prog = mob_proto->mprogs; prog != NULL; prog = prog->next) {
            changed |= shift_vnum_field(&prog->vnum, old_min, old_max, delta);
        }

        if (changed)
            mark_area_changed(mob_proto->area);
    }

    ObjPrototype* obj_proto;
    FOR_EACH_OBJ_PROTO(obj_proto) {
        bool changed = false;

        if (vnum_in_shift_range(VNUM_FIELD(obj_proto), old_min, old_max))
            write_value_array(&obj_proto_to_rekey, OBJ_VAL(obj_proto));

        if (obj_proto->item_type == ITEM_CONTAINER)
            changed |= shift_int_vnum_field(&obj_proto->container.key_vnum, old_min, old_max, delta);
        else if (obj_proto->item_type == ITEM_PORTAL) {
            changed |= shift_int_vnum_field(&obj_proto->portal.destination, old_min, old_max, delta);
            changed |= shift_int_vnum_field(&obj_proto->portal.key_vnum, old_min, old_max, delta);
        }

        changed |= shift_vnum_array(obj_proto->salvage_mats, obj_proto->salvage_mat_count,
            old_min, old_max, delta);

        if (changed)
            mark_area_changed(obj_proto->area);
    }

    RecipeIter recipe_iter = make_recipe_iter();
    Recipe* recipe;
    while ((recipe = recipe_iter_next(&recipe_iter)) != NULL) {
        bool changed = false;

        if (vnum_in_shift_range(VNUM_FIELD(recipe), old_min, old_max))
            write_value_array(&recipe_to_rekey, OBJ_VAL(recipe));

        changed |= shift_vnum_field(&recipe->station_vnum, old_min, old_max, delta);
        changed |= shift_vnum_field(&recipe->product_vnum, old_min, old_max, delta);
        for (int i = 0; i < recipe->ingredient_count; ++i) {
            changed |= shift_vnum_field(&recipe->ingredients[i].mat_vnum, old_min, old_max, delta);
        }

        if (changed)
            mark_area_changed(recipe->area);
    }

    AreaData* area_data;
    FOR_EACH_AREA(area_data) {
        bool changed = false;
        area_data->quests = shift_quest_list(area_data->quests, old_min, old_max, delta, &changed);
        if (changed)
            mark_area_changed(area_data);
    }

    if (faction_table.capacity > 0 && faction_table.entries != NULL) {
        for (int idx = 0; idx < faction_table.capacity; ++idx) {
            Entry* entry = &faction_table.entries[idx];
            if (IS_NIL(entry->value) || !IS_FACTION(entry->value))
                continue;

            Faction* faction = AS_FACTION(entry->value);
            bool changed = false;

            if (vnum_in_shift_range(VNUM_FIELD(faction), old_min, old_max))
                write_value_array(&faction_to_rekey, entry->value);

            changed |= shift_value_array_vnums(&faction->allies, old_min, old_max, delta);
            changed |= shift_value_array_vnums(&faction->enemies, old_min, old_max, delta);

            if (changed)
                mark_area_changed(faction->area);
        }
    }

    if (global_loot_db != NULL) {
        bool resolve_needed = false;

        for (int i = 0; i < global_loot_db->group_count; ++i) {
            LootGroup* group = &global_loot_db->groups[i];
            bool changed = false;

            for (int j = 0; j < group->entry_count; ++j) {
                LootEntry* entry = &group->entries[j];
                if (entry->type != LOOT_ITEM)
                    continue;

                VNUM new_vnum = shift_vnum_value(entry->item_vnum, old_min, old_max, delta);
                if (new_vnum != entry->item_vnum) {
                    entry->item_vnum = new_vnum;
                    changed = true;
                }
            }

            if (changed)
                mark_area_changed(loot_owner_area_data(group->owner));
        }

        for (int i = 0; i < global_loot_db->table_count; ++i) {
            LootTable* table = &global_loot_db->tables[i];
            bool changed = false;

            for (int j = 0; j < table->op_count; ++j) {
                LootOp* op = &table->ops[j];
                if (op->type != LOOT_OP_ADD_ITEM && op->type != LOOT_OP_REMOVE_ITEM)
                    continue;

                VNUM new_vnum = shift_vnum_value((VNUM)op->a, old_min, old_max, delta);
                if (new_vnum != op->a) {
                    op->a = (int)new_vnum;
                    changed = true;
                }
            }

            if (changed) {
                mark_area_changed(loot_owner_area_data(table->owner));
                resolve_needed = true;
            }
        }

        if (resolve_needed)
            resolve_all_loot_tables(global_loot_db);
    }

    for (ShopData* shop = shop_first; shop != NULL; shop = shop->next) {
        VNUM old_keeper = shop->keeper;
        if (shift_vnum_field(&shop->keeper, old_min, old_max, delta)) {
            MobPrototype* keeper = get_mob_prototype(old_keeper);
            if (keeper != NULL)
                mark_area_changed(keeper->area);
        }
    }

    for (int i = 0; i < class_count; ++i) {
        Class* class_ = &class_table[i];
        shift_vnum_field(&class_->start_loc, old_min, old_max, delta);
        shift_vnum_field(&class_->weapon, old_min, old_max, delta);
        for (int j = 0; j < MAX_GUILD; ++j)
            shift_vnum_field(&class_->guild[j], old_min, old_max, delta);
    }

    for (int i = 0; i < race_count; ++i) {
        Race* race = &race_table[i];
        shift_vnum_field(&race->start_loc, old_min, old_max, delta);
        for (size_t j = 0; j < race->class_start.count; ++j) {
            VNUM vnum = race->class_start.elems[j];
            VNUM new_vnum = shift_vnum_value(vnum, old_min, old_max, delta);
            if (new_vnum != vnum)
                race->class_start.elems[j] = new_vnum;
        }
    }

    for (PlayerData* pcdata = player_data_list; pcdata != NULL; pcdata = pcdata->next) {
        shift_vnum_field(&pcdata->recall, old_min, old_max, delta);

        if (pcdata->reputations.entries != NULL) {
            for (size_t i = 0; i < pcdata->reputations.count; ++i) {
                shift_vnum_field(&pcdata->reputations.entries[i].vnum, old_min, old_max, delta);
            }
        }

        if (pcdata->quest_log != NULL) {
            bool changed = false;
            pcdata->quest_log->target_mobs = shift_quest_target_list(pcdata->quest_log->target_mobs,
                old_min, old_max, delta, &changed);
            pcdata->quest_log->target_objs = shift_quest_target_list(pcdata->quest_log->target_objs,
                old_min, old_max, delta, &changed);
            pcdata->quest_log->target_ends = shift_quest_target_list(pcdata->quest_log->target_ends,
                old_min, old_max, delta, &changed);
            pcdata->quest_log->quests = shift_quest_status_list(pcdata->quest_log->quests,
                old_min, old_max, delta, &changed);
        }
    }

    for (int i = 0; i < room_data_to_rekey.count; ++i) {
        RoomData* rekey_room = AS_ROOM_DATA(room_data_to_rekey.values[i]);
        VNUM old_vnum = VNUM_FIELD(rekey_room);
        VNUM new_vnum = shift_vnum_value(old_vnum, old_min, old_max, delta);

        global_room_remove(old_vnum);
        set_entity_vnum(&rekey_room->header, new_vnum);
        global_room_set(rekey_room);
        mark_area_changed(rekey_room->area_data);
    }

    for (int i = 0; i < mob_proto_to_rekey.count; ++i) {
        MobPrototype* rekey_mob = AS_MOB_PROTO(mob_proto_to_rekey.values[i]);
        VNUM old_vnum = VNUM_FIELD(rekey_mob);
        VNUM new_vnum = shift_vnum_value(old_vnum, old_min, old_max, delta);

        global_mob_proto_remove(old_vnum);
        set_entity_vnum(&rekey_mob->header, new_vnum);
        global_mob_proto_set(rekey_mob);
        mark_area_changed(rekey_mob->area);
    }

    for (int i = 0; i < obj_proto_to_rekey.count; ++i) {
        ObjPrototype* rekey_obj = AS_OBJ_PROTO(obj_proto_to_rekey.values[i]);
        VNUM old_vnum = VNUM_FIELD(rekey_obj);
        VNUM new_vnum = shift_vnum_value(old_vnum, old_min, old_max, delta);

        global_obj_proto_remove(old_vnum);
        set_entity_vnum(&rekey_obj->header, new_vnum);
        global_obj_proto_set(rekey_obj);
        mark_area_changed(rekey_obj->area);
    }

    for (int i = 0; i < recipe_to_rekey.count; ++i) {
        Recipe* rekey_recipe = (Recipe*)AS_OBJ(recipe_to_rekey.values[i]);
        VNUM old_vnum = VNUM_FIELD(rekey_recipe);
        VNUM new_vnum = shift_vnum_value(old_vnum, old_min, old_max, delta);

        remove_recipe(old_vnum);
        set_entity_vnum(&rekey_recipe->header, new_vnum);
        add_recipe(rekey_recipe);
        mark_area_changed(rekey_recipe->area);
    }

    for (int i = 0; i < faction_to_rekey.count; ++i) {
        Faction* rekey_faction = AS_FACTION(faction_to_rekey.values[i]);
        VNUM old_vnum = VNUM_FIELD(rekey_faction);
        VNUM new_vnum = shift_vnum_value(old_vnum, old_min, old_max, delta);

        table_delete_vnum(&faction_table, old_vnum);
        set_entity_vnum(&rekey_faction->header, new_vnum);
        table_set_vnum(&faction_table, new_vnum, OBJ_VAL(rekey_faction));
        mark_area_changed(rekey_faction->area);
    }

    FOR_EACH_AREA(area_data) {
        Area* area_inst;
        FOR_EACH_AREA_INST(area_inst, area_data) {
            ValueArray rooms_to_rekey;
            init_value_array(&rooms_to_rekey);

            Room* room;
            FOR_EACH_AREA_ROOM(room, area_inst) {
                if (vnum_in_shift_range(VNUM_FIELD(room), old_min, old_max))
                    write_value_array(&rooms_to_rekey, OBJ_VAL(room));
            }

            for (int i = 0; i < rooms_to_rekey.count; ++i) {
                Room* rekey_room = AS_ROOM(rooms_to_rekey.values[i]);
                VNUM old_vnum = VNUM_FIELD(rekey_room);
                VNUM new_vnum = shift_vnum_value(old_vnum, old_min, old_max, delta);

                table_delete_vnum(&area_inst->rooms, old_vnum);
                set_entity_vnum(&rekey_room->header, new_vnum);
                table_set_vnum(&area_inst->rooms, VNUM_FIELD(rekey_room), OBJ_VAL(rekey_room));
            }

            free_value_array(&rooms_to_rekey);
        }
    }

    Mobile* mob;
    FOR_EACH_GLOBAL_MOB(mob) {
        shift_vnum_field(&mob->faction_vnum, old_min, old_max, delta);
        shift_vnum_field(&mob->group, old_min, old_max, delta);

        if (vnum_in_shift_range(VNUM_FIELD(mob), old_min, old_max)) {
            VNUM new_vnum = shift_vnum_value(VNUM_FIELD(mob), old_min, old_max, delta);
            set_entity_vnum(&mob->header, new_vnum);
        }
    }

    Object* obj;
    FOR_EACH_GLOBAL_OBJ(obj) {
        if (obj->item_type == ITEM_CONTAINER)
            shift_int_vnum_field(&obj->container.key_vnum, old_min, old_max, delta);
        else if (obj->item_type == ITEM_PORTAL) {
            shift_int_vnum_field(&obj->portal.destination, old_min, old_max, delta);
            shift_int_vnum_field(&obj->portal.key_vnum, old_min, old_max, delta);
        }

        shift_vnum_array(obj->craft_mats, obj->craft_mat_count, old_min, old_max, delta);
        shift_vnum_array(obj->salvage_mats, obj->salvage_mat_count, old_min, old_max, delta);

        if (vnum_in_shift_range(VNUM_FIELD(obj), old_min, old_max)) {
            VNUM new_vnum = shift_vnum_value(VNUM_FIELD(obj), old_min, old_max, delta);
            set_entity_vnum(&obj->header, new_vnum);
        }
    }

    free_value_array(&room_data_to_rekey);
    free_value_array(&mob_proto_to_rekey);
    free_value_array(&obj_proto_to_rekey);
    free_value_array(&recipe_to_rekey);
    free_value_array(&faction_to_rekey);
}

AEDIT(aedit_movevnums)
{
    AreaData* area;
    char arg[MAX_STRING_LENGTH];

    EDIT_AREA(ch, area);

    one_argument(argument, arg);
    if (!is_number(arg) || arg[0] == '\0') {
        send_to_char(COLOR_INFO "Syntax:  movevnums <lower>" COLOR_EOL, ch);
        return false;
    }

    VNUM new_min = STRTOVNUM(arg);
    VNUM old_min = area->min_vnum;
    VNUM old_max = area->max_vnum;
    int64_t delta64 = (int64_t)new_min - (int64_t)old_min;

    if (old_min > old_max) {
        send_to_char(COLOR_INFO "Area VNUM range is invalid." COLOR_EOL, ch);
        return false;
    }

    if (delta64 == 0) {
        send_to_char(COLOR_INFO "Area VNUMs are already at that range." COLOR_EOL, ch);
        return false;
    }

    int64_t new_max64 = (int64_t)old_max + delta64;
    if (new_min < 0 || new_max64 < 0 || new_max64 > MAX_VNUM) {
        send_to_char(COLOR_INFO "Target VNUM range is out of bounds." COLOR_EOL, ch);
        return false;
    }

    VNUM new_max = (VNUM)new_max64;
    if (range_overlaps_other_areas(area, new_min, new_max)) {
        send_to_char(COLOR_INFO "Target range overlaps another area." COLOR_EOL, ch);
        return false;
    }

    aedit_apply_movevnums(area, old_min, old_max, (int32_t)delta64);

    area->min_vnum = new_min;
    area->max_vnum = new_max;
    mark_area_changed(area);
    recompute_top_vnums();

    printf_to_char(ch, COLOR_INFO "Area VNUMs moved to %" PRVNUM "-%" PRVNUM " (delta %+d)." COLOR_EOL,
        area->min_vnum, area->max_vnum, (int32_t)delta64);
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
        send_to_char(COLOR_INFO "Syntax:  VNUMS <lower> <upper>" COLOR_EOL, ch);
        return false;
    }

    if ((ilower = atoi(lower)) > (iupper = atoi(upper))) {
        send_to_char(COLOR_INFO "Upper must be larger then lower." COLOR_EOL, ch);
        return false;
    }

    if (!check_range(ilower, iupper)) {
        send_to_char(COLOR_INFO "Range must include only this area." COLOR_EOL, ch);
        return false;
    }

    if (get_vnum_area(ilower)
        && get_vnum_area(ilower) != area) {
        send_to_char(COLOR_INFO "Lower VNUM already assigned." COLOR_EOL, ch);
        return false;
    }

    area->min_vnum = ilower;
    send_to_char(COLOR_INFO "Lower VNUM set." COLOR_EOL, ch);

    if (get_vnum_area(iupper)
        && get_vnum_area(iupper) != area) {
        send_to_char(COLOR_INFO "Upper VNUM already assigned." COLOR_EOL, ch);
        return true;    /* The lower value has been set. */
    }

    area->max_vnum = iupper;
    send_to_char(COLOR_INFO "VNUMs set." COLOR_EOL, ch);

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
        send_to_char(COLOR_INFO "Syntax:  min_vnum <vnum>" COLOR_EOL, ch);
        return false;
    }

    if ((ilower = atoi(lower)) > (iupper = area->max_vnum)) {
        send_to_char(COLOR_INFO "Value must be less than the max_vnum." COLOR_EOL, ch);
        return false;
    }

    if (!check_range(ilower, iupper)) {
        send_to_char(COLOR_INFO "Range must include only this area." COLOR_EOL, ch);
        return false;
    }

    if (get_vnum_area(ilower)
        && get_vnum_area(ilower) != area) {
        send_to_char(COLOR_INFO "Lower VNUM already assigned." COLOR_EOL, ch);
        return false;
    }

    area->min_vnum = ilower;
    send_to_char(COLOR_INFO "Lower VNUM set." COLOR_EOL, ch);
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
        send_to_char(COLOR_INFO "Syntax:  max_vnum <vnum>" COLOR_EOL, ch);
        return false;
    }

    if ((ilower = area->min_vnum) > (iupper = atoi(upper))) {
        send_to_char(COLOR_INFO "Upper must be larger then lower." COLOR_EOL, ch);
        return false;
    }

    if (!check_range(ilower, iupper)) {
        send_to_char(COLOR_INFO "Range must include only this area." COLOR_EOL, ch);
        return false;
    }

    if (get_vnum_area(iupper)
        && get_vnum_area(iupper) != area) {
        send_to_char(COLOR_INFO "Upper VNUM already assigned." COLOR_EOL, ch);
        return false;
    }

    area->max_vnum = iupper;
    send_to_char(COLOR_INFO "Upper VNUM set." COLOR_EOL, ch);

    return true;
}

// Comparators for qsort in do_alist
static int compare_area_by_vnum(const void* a, const void* b)
{
    const AreaData* area_a = *(const AreaData**)a;
    const AreaData* area_b = *(const AreaData**)b;
    return (area_a->min_vnum > area_b->min_vnum) - (area_a->min_vnum < area_b->min_vnum);
}

static int compare_area_by_name(const void* a, const void* b)
{
    const AreaData* area_a = *(const AreaData**)a;
    const AreaData* area_b = *(const AreaData**)b;
    return strcasecmp(NAME_STR(area_a), NAME_STR(area_b));
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

    StringBuffer* sb = sb_new();
    char arg[MIL];
    char sort[MIL];

    sb_appendf(sb, COLOR_DECOR_1 "[" COLOR_TITLE "%3s" COLOR_DECOR_1 "] [" COLOR_TITLE "%-22s" COLOR_DECOR_1 "] (" COLOR_TITLE "%6s" COLOR_DECOR_1 "-" COLOR_TITLE "%-6s" COLOR_DECOR_1 ") " COLOR_DECOR_1 "[" COLOR_TITLE "%-11s" COLOR_DECOR_1 "] " COLOR_TITLE "%3s " COLOR_DECOR_1 "[" COLOR_TITLE "%-10s" COLOR_DECOR_1 "]\n\r",
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
            qsort(alist, (size_t)global_areas.count, sizeof(AreaData*), compare_area_by_vnum);
        }
        else if (!str_cmp(sort, "name")) {
            qsort(alist, (size_t)global_areas.count, sizeof(AreaData*), compare_area_by_name);
        }
        else {
            printf_to_char(ch, COLOR_INFO "Unknown sort option '" COLOR_ALT_TEXT_1 "%s" COLOR_INFO "'." COLOR_CLEAR "\n\r\n\r%s", sort, help);
            goto alist_cleanup;
        }
    }

    AreaData* area;
    for (int i = 0; i < global_areas.count; ++i) {
        area = alist[i];
        sb_appendf(sb, "[" COLOR_ALT_TEXT_1 "%3d" COLOR_DECOR_1 "]" COLOR_CLEAR " %-24.24s " COLOR_DECOR_1 "(" COLOR_ALT_TEXT_1 "%6d" COLOR_DECOR_1 "-" COLOR_ALT_TEXT_1 "%-6d" COLOR_DECOR_1 ") " COLOR_ALT_TEXT_2 "%-13.13s " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%d" COLOR_DECOR_1 "] [" COLOR_ALT_TEXT_1 "%-10.10s" COLOR_DECOR_1 "]\n\r",
            VNUM_FIELD(area),
            NAME_STR(area),
            area->min_vnum,
            area->max_vnum,
            area->file_name,
            area->security,
            area->builders);
    }
    sb_appendf(sb, COLOR_CLEAR);

    send_to_char(sb->data, ch);

alist_cleanup:
    sb_free(sb);
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

////////////////////////////////////////////////////////////////////////////////
// Recipe Management in Area Editor
////////////////////////////////////////////////////////////////////////////////

static int count_area_recipes(AreaData* area)
{
    int count = 0;
    RecipeIter iter = make_recipe_iter();
    Recipe* recipe;
    while ((recipe = recipe_iter_next(&iter)) != NULL) {
        if (recipe->area == area)
            count++;
    }
    return count;
}

static void aedit_list_recipes(Mobile* ch, AreaData* area)
{
    int count = 0;
    RecipeIter iter = make_recipe_iter();
    Recipe* recipe;

    printf_to_char(ch, COLOR_INFO "Recipes in [%d] %s:" COLOR_EOL "\n\r",
        VNUM_FIELD(area), NAME_STR(area));

    while ((recipe = recipe_iter_next(&iter)) != NULL) {
        if (recipe->area != area)
            continue;

        const char* skill_name = "(none)";
        if (recipe->required_skill >= 0 && recipe->required_skill < skill_count) {
            skill_name = skill_table[recipe->required_skill].name;
        }

        printf_to_char(ch, 
            COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR
            " %-30.30s %-20.20s Lvl %d" COLOR_EOL,
            VNUM_FIELD(recipe),
            NAME_STR(recipe),
            skill_name,
            recipe->min_level);
        count++;
    }

    if (count == 0) {
        send_to_char("  (none)\n\r", ch);
    }
    printf_to_char(ch, "\n\r" COLOR_INFO "%d recipe(s) in this area." COLOR_EOL, count);
}

AEDIT(aedit_recipe)
{
    AreaData* area;
    char cmd[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    VNUM vnum;

    EDIT_AREA(ch, area);

    READ_ARG(cmd);
    READ_ARG(arg);

    // No argument - list recipes
    if (IS_NULLSTR(cmd) || !str_cmp(cmd, "list")) {
        aedit_list_recipes(ch, area);
        return false;
    }

    // Create new recipe
    if (!str_cmp(cmd, "create")) {
        if (IS_NULLSTR(arg) || !is_number(arg)) {
            send_to_char(COLOR_INFO "Syntax: recipe create <vnum>" COLOR_EOL, ch);
            return false;
        }

        vnum = (VNUM)atoi(arg);

        // Validate VNUM is in area range
        if (vnum < area->min_vnum || vnum > area->max_vnum) {
            printf_to_char(ch, COLOR_INFO "Recipe VNUM must be in area range %d-%d." COLOR_EOL,
                area->min_vnum, area->max_vnum);
            return false;
        }

        // Check if already exists
        if (get_recipe(vnum)) {
            send_to_char(COLOR_INFO "A recipe with that VNUM already exists." COLOR_EOL, ch);
            return false;
        }

        // Create the recipe
        Recipe* recipe = new_recipe();
        recipe->header.vnum = vnum;
        recipe->area = area;
        recipe->required_skill = -1;
        recipe->min_skill = 0;
        recipe->min_level = 1;
        recipe->station_type = WORK_NONE;
        recipe->station_vnum = VNUM_NONE;
        recipe->discovery = DISC_KNOWN;
        recipe->product_vnum = VNUM_NONE;
        recipe->product_quantity = 1;

        if (!add_recipe(recipe)) {
            send_to_char(COLOR_INFO "Failed to add recipe." COLOR_EOL, ch);
            free_recipe(recipe);
            return false;
        }

        // Enter recipe editor
        set_editor(ch->desc, ED_RECIPE, (uintptr_t)recipe);
        printf_to_char(ch, COLOR_INFO "Recipe %d created. Entering recipe editor." COLOR_EOL, vnum);
        SET_BIT(area->area_flags, AREA_CHANGED);
        return true;
    }

    // Edit existing recipe
    if (!str_cmp(cmd, "edit")) {
        if (IS_NULLSTR(arg) || !is_number(arg)) {
            send_to_char(COLOR_INFO "Syntax: recipe edit <vnum>" COLOR_EOL, ch);
            return false;
        }

        vnum = (VNUM)atoi(arg);
        Recipe* recipe = get_recipe(vnum);

        if (!recipe) {
            send_to_char(COLOR_INFO "No recipe with that VNUM exists." COLOR_EOL, ch);
            return false;
        }

        if (recipe->area != area) {
            send_to_char(COLOR_INFO "That recipe belongs to a different area." COLOR_EOL, ch);
            return false;
        }

        // Enter recipe editor
        set_editor(ch->desc, ED_RECIPE, (uintptr_t)recipe);
        printf_to_char(ch, COLOR_INFO "Editing recipe %d." COLOR_EOL, vnum);
        return true;
    }

    // Delete recipe
    if (!str_cmp(cmd, "delete")) {
        if (IS_NULLSTR(arg) || !is_number(arg)) {
            send_to_char(COLOR_INFO "Syntax: recipe delete <vnum>" COLOR_EOL, ch);
            return false;
        }

        vnum = (VNUM)atoi(arg);
        Recipe* recipe = get_recipe(vnum);

        if (!recipe) {
            send_to_char(COLOR_INFO "No recipe with that VNUM exists." COLOR_EOL, ch);
            return false;
        }

        if (recipe->area != area) {
            send_to_char(COLOR_INFO "That recipe belongs to a different area." COLOR_EOL, ch);
            return false;
        }

        remove_recipe(vnum);
        printf_to_char(ch, COLOR_INFO "Recipe %d deleted." COLOR_EOL, vnum);
        SET_BIT(area->area_flags, AREA_CHANGED);
        return true;
    }

    // Unknown subcommand
    send_to_char(COLOR_INFO "Syntax:" COLOR_EOL
                 "  recipe                  - List recipes in this area" COLOR_EOL
                 "  recipe list             - List recipes in this area" COLOR_EOL
                 "  recipe create <vnum>    - Create new recipe" COLOR_EOL
                 "  recipe edit <vnum>      - Edit existing recipe" COLOR_EOL
                 "  recipe delete <vnum>    - Delete recipe" COLOR_EOL, ch);
    return false;
}
