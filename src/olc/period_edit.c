////////////////////////////////////////////////////////////////////////////////
// olc/period_edit.c
// Shared period editing functionality for areas and rooms
////////////////////////////////////////////////////////////////////////////////

#include "period_edit.h"

#include <entities/area.h>
#include <entities/daycycle_period.h>
#include <entities/room.h>

#include <olc/olc.h>

#include <color.h>
#include <comm.h>
#include <db.h>
#include <format.h>
#include <stringbuffer.h>

#include "string_edit.h"

#include <string.h>

// Period edit operations for polymorphic area/room handling
typedef struct period_ops_t {
    DayCyclePeriod* (*find)(void* entity, const char* name);
    DayCyclePeriod* (*add)(void* entity, const char* name, int start, int end);
    bool (*remove)(void* entity, const char* name);
    DayCyclePeriod* (*get_periods)(void* entity);
    bool (*get_suppress)(void* entity);
    void (*set_suppress)(void* entity, bool suppress);
    void (*mark_changed)(void* entity);
    const char* (*get_entity_type_name)(void);
} PeriodOps;

// Get operations table for entity type
const PeriodOps* get_period_ops_for_area(void);
const PeriodOps* get_period_ops_for_room(void);

typedef struct period_preset_t {
    const char* name;
    int start;
    int end;
} PeriodPreset;

static const PeriodPreset period_presets[] = {
    { "day",        6,      18  },      // 6 AM to 6 PM
    { "dusk",       19,     19  },      // 7 PM
    { "night",      20,     4   },      // 8 PM to 4 AM
    { "dawn",       5,      5   },      // 5 AM
    { NULL,         0,      0   },
};

// Helper functions
static bool find_period_preset(const char* name, int* start, int* end)
{
    if (!name || name[0] == '\0')
        return false;

    for (int i = 0; period_presets[i].name != NULL; ++i) {
        if (!str_cmp(name, period_presets[i].name)) {
            if (start)
                *start = period_presets[i].start;
            if (end)
                *end = period_presets[i].end;
            return true;
        }
    }

    return false;
}

static bool parse_period_hours(Mobile* ch, const char* start_arg, const char* end_arg, const char* preset_name, int* start, int* end)
{
    if (start_arg == NULL || start_arg[0] == '\0') {
        if (preset_name && find_period_preset(preset_name, start, end))
            return true;

        send_to_char(COLOR_INFO "Specify start and end hours between 0 and 23, or use one of the presets: day, dusk, night, dawn." COLOR_EOL, ch);
        return false;
    }

    if (end_arg == NULL || end_arg[0] == '\0') {
        send_to_char(COLOR_INFO "Specify both a start and end hour between 0 and 23." COLOR_EOL, ch);
        return false;
    }

    if (!is_number(start_arg) || !is_number(end_arg)) {
        send_to_char(COLOR_INFO "Hours must be numeric values between 0 and 23." COLOR_EOL, ch);
        return false;
    }

    int s = atoi(start_arg);
    int e = atoi(end_arg);
    if (s < 0 || s > 23 || e < 0 || e > 23) {
        send_to_char(COLOR_INFO "Hours must be within the 0-23 range." COLOR_EOL, ch);
        return false;
    }

    if (start)
        *start = s;
    if (end)
        *end = e;
    return true;
}

static void build_period_preview(const DayCyclePeriod* period, char* out, size_t outlen, bool prefer_desc)
{
    if (!period || !out || outlen == 0)
        return;

    const char* text = NULL;
    
    if (prefer_desc && period->description && period->description[0] != '\0')
        text = period->description;
    else if (period->enter_message && period->enter_message[0] != '\0')
        text = period->enter_message;
    else if (period->exit_message && period->exit_message[0] != '\0')
        text = period->exit_message;
    else if (period->description && period->description[0] != '\0')
        text = period->description;
    else
        text = "(no content)";
    
    size_t len = strcspn(text, "\r\n");
    if (len >= outlen)
        len = outlen - 1;
    strncpy(out, text, len);
    out[len] = '\0';
}

static void show_period_usage(Mobile* ch, bool has_desc)
{
    send_to_char(COLOR_INFO "Syntax:" COLOR_EOL, ch);
    send_to_char(COLOR_INFO "  period list" COLOR_EOL, ch);
    send_to_char(COLOR_INFO "  period add <name> [start end]" COLOR_EOL, ch);
    if (has_desc)
        send_to_char(COLOR_INFO "  period desc <name>" COLOR_EOL, ch);
    send_to_char(COLOR_INFO "  period delete <name>" COLOR_EOL, ch);
    send_to_char(COLOR_INFO "  period set <name> <start> <end>" COLOR_EOL, ch);
    send_to_char(COLOR_INFO "  period rename <name> <new name>" COLOR_EOL, ch);
    if (has_desc)
        send_to_char(COLOR_INFO "  period format <name> [desc|enter|exit]" COLOR_EOL, ch);
    else
        send_to_char(COLOR_INFO "  period format <name> [enter|exit]" COLOR_EOL, ch);
    send_to_char(COLOR_INFO "  period enter <name>" COLOR_EOL, ch);
    send_to_char(COLOR_INFO "  period exit <name>" COLOR_EOL, ch);
    send_to_char(COLOR_INFO "  period suppress <on|off>" COLOR_EOL, ch);
}

static void show_period_list(Mobile* ch, void* entity, const PeriodOps* ops, bool has_desc)
{
    DayCyclePeriod* periods = ops->get_periods(entity);
    
    if (!periods) {
        printf_to_char(ch, COLOR_INFO "No time periods are defined for this %s." COLOR_EOL, 
            ops->get_entity_type_name());
        return;
    }

    send_to_char(COLOR_TITLE "Defined Time Periods" COLOR_EOL, ch);
    send_to_char(COLOR_DECOR_2 "  Name         Hours   Msgs   Preview" COLOR_EOL, ch);

    for (DayCyclePeriod* period = periods; period != NULL; period = period->next) {
        char preview[64];
        build_period_preview(period, preview, sizeof(preview), has_desc);
        const char* name = (period->name && period->name[0] != '\0') ? period->name : "(unnamed)";
        
        char msg_flags[4] = "";
        if (has_desc && period->description && period->description[0] != '\0')
            strcat(msg_flags, "D");
        if (period->enter_message && period->enter_message[0] != '\0')
            strcat(msg_flags, "E");
        if (period->exit_message && period->exit_message[0] != '\0')
            strcat(msg_flags, "X");
        if (msg_flags[0] == '\0')
            strcpy(msg_flags, "-");
        
        printf_to_char(ch, COLOR_ALT_TEXT_1 "  %-12s" COLOR_CLEAR "  %02d-%02d   %-3s   %s" COLOR_EOL,
            name,
            period->start_hour,
            period->end_hour,
            msg_flags,
            preview);
    }
}

// Operations tables for different entity types
static DayCyclePeriod* area_find_op(void* entity, const char* name)
{
    return area_daycycle_period_find((AreaData*)entity, name);
}

static DayCyclePeriod* area_add_op(void* entity, const char* name, int start, int end)
{
    return area_daycycle_period_add((AreaData*)entity, name, start, end);
}

static bool area_remove_op(void* entity, const char* name)
{
    return area_daycycle_period_remove((AreaData*)entity, name);
}

static DayCyclePeriod* area_get_periods_op(void* entity)
{
    return ((AreaData*)entity)->periods;
}

static bool area_get_suppress_op(void* entity)
{
    return ((AreaData*)entity)->suppress_daycycle_messages;
}

static void area_set_suppress_op(void* entity, bool suppress)
{
    ((AreaData*)entity)->suppress_daycycle_messages = suppress;
}

static void area_mark_changed_op(void* entity)
{
    SET_BIT(((AreaData*)entity)->area_flags, AREA_CHANGED);
}

static const char* area_type_name_op(void)
{
    return "area";
}

static const PeriodOps area_ops = {
    .find = area_find_op,
    .add = area_add_op,
    .remove = area_remove_op,
    .get_periods = area_get_periods_op,
    .get_suppress = area_get_suppress_op,
    .set_suppress = area_set_suppress_op,
    .mark_changed = area_mark_changed_op,
    .get_entity_type_name = area_type_name_op,
};

static DayCyclePeriod* room_find_op(void* entity, const char* name)
{
    return room_daycycle_period_find((RoomData*)entity, name);
}

static DayCyclePeriod* room_add_op(void* entity, const char* name, int start, int end)
{
    return room_daycycle_period_add((RoomData*)entity, name, start, end);
}

static bool room_remove_op(void* entity, const char* name)
{
    return room_daycycle_period_remove((RoomData*)entity, name);
}

static DayCyclePeriod* room_get_periods_op(void* entity)
{
    return ((RoomData*)entity)->periods;
}

static bool room_get_suppress_op(void* entity)
{
    return ((RoomData*)entity)->suppress_daycycle_messages;
}

static void room_set_suppress_op(void* entity, bool suppress)
{
    ((RoomData*)entity)->suppress_daycycle_messages = suppress;
}

static void room_mark_changed_op(void* entity)
{
    SET_BIT(((RoomData*)entity)->area_data->area_flags, AREA_CHANGED);
}

static const char* room_type_name_op(void)
{
    return "room";
}

static const PeriodOps room_ops = {
    .find = room_find_op,
    .add = room_add_op,
    .remove = room_remove_op,
    .get_periods = room_get_periods_op,
    .get_suppress = room_get_suppress_op,
    .set_suppress = room_set_suppress_op,
    .mark_changed = room_mark_changed_op,
    .get_entity_type_name = room_type_name_op,
};

const PeriodOps* get_period_ops_for_area(void)
{
    return &area_ops;
}

const PeriodOps* get_period_ops_for_room(void)
{
    return &room_ops;
}

// Main editing function
bool olc_edit_period(Mobile* ch, char* argument, Entity* entity/*, const PeriodOps* ops*/)
{
    const PeriodOps* ops = entity->obj.type == OBJ_AREA_DATA ? get_period_ops_for_area() :
        entity->obj.type == OBJ_ROOM_DATA ? get_period_ops_for_room() : NULL;

    if (!ops) {
        bugf("olc_show_periods: Unsupported entity type %d", entity->obj.type);
        return false;
    }

    // Room periods support descriptions, area periods do not (currently)
    bool has_desc = (ops == &room_ops);
    
    char command[MIL];
    READ_ARG(command);

    if (command[0] == '\0') {
        show_period_usage(ch, has_desc);
        return false;
    }

    if (!str_prefix(command, "list")) {
        show_period_list(ch, entity, ops, has_desc);
        return false;
    }

    if (!str_prefix(command, "suppress")) {
        char value[MIL];
        READ_ARG(value);
        if (value[0] == '\0') {
            ops->set_suppress(entity, !ops->get_suppress(entity));
            ops->mark_changed(entity);
            printf_to_char(ch, COLOR_INFO "Daycycle messages are now %s." COLOR_EOL,
                ops->get_suppress(entity) ? "suppressed" : "shown");
            return true;
        }

        bool suppress = false;
        if (!str_prefix(value, "on") || !str_prefix(value, "yes") || !str_prefix(value, "true"))
            suppress = true;
        else if (!str_prefix(value, "off") || !str_prefix(value, "no") || !str_prefix(value, "false"))
            suppress = false;
        else {
            show_period_usage(ch, has_desc);
            return false;
        }

        ops->set_suppress(entity, suppress);
        ops->mark_changed(entity);            
        printf_to_char(ch, COLOR_INFO "Daycycle messages are now %s." COLOR_EOL,
            ops->get_suppress(entity) ? "suppressed" : "shown");
        return true;
    }

    char name[MIL];
    READ_ARG(name);

    if (!str_prefix(command, "add")) {
        if (name[0] == '\0') {
            printf_to_char(ch, COLOR_INFO "Specify the name of the time period to add to this %s." COLOR_EOL,
                ops->get_entity_type_name());
            printf_to_char(ch, COLOR_INFO "You may optionally specify start and end hours, or use one of the presets: day, dusk, night, dawn." COLOR_EOL);
            return false;
        }

        if (ops->find(entity, name) != NULL) {
            printf_to_char(ch, COLOR_INFO "A time period named '" COLOR_ALT_TEXT_1 "%s" COLOR_INFO "' already exists." COLOR_EOL, name);
            return false;
        }

        char start_arg[MIL], end_arg[MIL];
        READ_ARG(start_arg);
        READ_ARG(end_arg);

        int start = 0, end = 0;
        if (!parse_period_hours(ch, start_arg, end_arg, name, &start, &end))
            return false;

        DayCyclePeriod* period = ops->add(entity, name, start, end);
        if (!period) {
            send_to_char(COLOR_INFO "Unable to create time period." COLOR_EOL, ch);
            return false;
        }

        ops->mark_changed(entity);
        
        if (has_desc) {
            send_to_char(COLOR_INFO "Enter the room description for this period. End with '@' on a blank line." COLOR_EOL, ch);
            string_append(ch, &period->description);
        } else {
            send_to_char(COLOR_INFO "Time period created. Use " COLOR_ALT_TEXT_1
                "'PERIOD ENTER'" COLOR_INFO " and " COLOR_ALT_TEXT_1 
                "'PERIOD EXIT'" COLOR_INFO " to define the messages." COLOR_EOL,
                ch);
        }
        return true;
    }

    if (!str_prefix(command, "delete") || !str_prefix(command, "remove")) {
        if (name[0] == '\0') {
            show_period_usage(ch, has_desc);
            return false;
        }

        if (!ops->remove(entity, name)) {
            printf_to_char(ch, COLOR_INFO "No time period named '" COLOR_ALT_TEXT_1 "%s" COLOR_INFO "' exists." COLOR_EOL, name);
            return false;
        }

        ops->mark_changed(entity);
        send_to_char(COLOR_INFO "Time period removed." COLOR_EOL, ch);
        return true;
    }

    DayCyclePeriod* period = ops->find(entity, name);
    if (!period) {
        printf_to_char(ch, COLOR_INFO "No time period named '" COLOR_ALT_TEXT_1 "%s" COLOR_INFO "' exists." COLOR_EOL, name);
        return false;
    }

    if (!str_prefix(command, "enter")) {
        ops->mark_changed(entity);
        send_to_char(COLOR_INFO "Enter the message shown when this period begins." COLOR_EOL, ch);
        string_append(ch, &period->enter_message);
        return true;
    }

    if (!str_prefix(command, "exit")) {
        ops->mark_changed(entity);
        send_to_char(COLOR_INFO "Enter the message shown when this period ends." COLOR_EOL, ch);
        string_append(ch, &period->exit_message);
        return true;
    }

    if (has_desc && !str_prefix(command, "desc")) {
        ops->mark_changed(entity);
        send_to_char(COLOR_INFO "Enter the description for this period. End with '@' on a blank line." COLOR_EOL, ch);
        string_append(ch, &period->description);
        return true;
    }

    if (!str_prefix(command, "set") || !str_prefix(command, "range")) {
        char start_arg[MIL], end_arg[MIL];
        READ_ARG(start_arg);
        READ_ARG(end_arg);

        int start = 0, end = 0;
        if (!parse_period_hours(ch, start_arg, end_arg, NULL, &start, &end))
            return false;

        period->start_hour = (int8_t)start;
        period->end_hour = (int8_t)end;
        ops->mark_changed(entity);
        send_to_char(COLOR_INFO "Time period hours updated." COLOR_EOL, ch);
        return true;
    }

    if (!str_prefix(command, "rename")) {
        char new_name[MIL];
        READ_ARG(new_name);

        if (new_name[0] == '\0') {
            send_to_char(COLOR_INFO "Specify the new name." COLOR_EOL, ch);
            return false;
        }

        if (ops->find(entity, new_name) != NULL) {
            printf_to_char(ch, COLOR_INFO "A time period named '" COLOR_ALT_TEXT_1 "%s" COLOR_INFO "' already exists." COLOR_EOL, new_name);
            return false;
        }

        free_string(period->name);
        period->name = str_dup(new_name);
        ops->mark_changed(entity);
        send_to_char(COLOR_INFO "Time period renamed." COLOR_EOL, ch);
        return true;
    }

    if (!str_prefix(command, "format")) {
        char target[MIL];
        READ_ARG(target);
        char** field = NULL;
        
        if (target[0] == '\0' || !str_prefix(target, "enter"))
            field = &period->enter_message;
        else if (!str_prefix(target, "exit"))
            field = &period->exit_message;
        else if (has_desc && !str_prefix(target, "desc"))
            field = &period->description;
        else {
            show_period_usage(ch, has_desc);
            return false;
        }

        char* formatted = format_string(*field);
        free_string(*field);
        *field = formatted;
        ops->mark_changed(entity);
        send_to_char(COLOR_INFO "Time period text formatted." COLOR_EOL, ch);
        return true;
    }

    show_period_usage(ch, has_desc);
    return false;
}

void olc_show_periods(Mobile* ch, Entity* entity /*, const PeriodOps* ops*/)
{
    const PeriodOps* ops = entity->obj.type == OBJ_AREA_DATA ? get_period_ops_for_area() :
        entity->obj.type == OBJ_ROOM_DATA ? get_period_ops_for_room() : NULL;

    if (!ops) {
        bugf("olc_show_periods: Unsupported entity type %d", entity->obj.type);
        return;
    }

    DayCyclePeriod* periods = ops->get_periods(entity);
    
    if (!periods) {
        olc_print_str_box(ch, "Time Periods", "(none)", 
            "Type '" COLOR_TITLE "PERIOD" COLOR_ALT_TEXT_2 "' to add some.");
        return;
    }

    StringBuffer* buf = sb_new();
    for (DayCyclePeriod* period = periods; period != NULL; period = period->next) {
        const char* name = (period->name && period->name[0] != '\0') ? period->name : "(unnamed)";
        sb_appendf(buf, "%s (%02d-%02d)%s",
            name,
            period->start_hour,
            period->end_hour,
            period->next ? ", " : "");
    }

    olc_print_str(ch, "Time Periods", buf->data);
    sb_free(buf);
}
