////////////////////////////////////////////////////////////////////////////////
// olc/event_edit.c
////////////////////////////////////////////////////////////////////////////////

#include "event_edit.h"

#include "bit.h"
#include "olc.h"

#include <data/events.h>

#include <entities/event.h>

#include <comm.h>
#include <db.h>
#include <lookup.h>
#include <tables.h>

bool olc_edit_event(Mobile* ch, char* argument)
{
    static const char* help =
        COLOR_INFO "Syntax: EVENT SET <trigger> [<callback> [<criteria>]]\n\r"
        "        EVENT DELETE <trigger>" COLOR_EOL;

    Entity* entity;

    EDIT_ENTITY(ch, entity);

    EventEnts ent_type = get_entity_type(entity);

    if (ent_type == 0) {
        send_to_char(COLOR_INFO "Cannot attach events to that entity." COLOR_EOL, ch);
        return false;
    }

    if (IS_NULLSTR(argument)) {
        send_to_char(help, ch);
        return false;
    }

    char cmd_arg[MIL];
    char trig_arg[MIL];

    READ_ARG(cmd_arg);
    READ_ARG(trig_arg);

    if (IS_NULLSTR(cmd_arg)) {
        send_to_char(help, ch);
        return false;
    }

    // The criteria expression, if any, will be in the remainder in "argument".
    FLAGS trig = (FLAGS)get_event_type_info_by_name(trig_arg);
    if (trig == NO_FLAG) {
        if (!IS_NULLSTR(trig_arg))
            printf_to_char(ch, COLOR_INFO "No trigger '%s' found. ", trig_arg);
        printf_to_char(ch, COLOR_INFO "These are the available event triggers for this object:\n\r" COLOR_ALT_TEXT_1);
        int j = 0;
        for (int i = 0; i < TRIG_COUNT; ++i)
            if (HAS_BIT(event_type_info_table[i].valid_ents, ent_type)) {
                printf_to_char(ch, "%10s%s", event_type_info_table[i].name,
                    (++j % 3 == 0) ? "\n\r" : "");
            }
        send_to_char(COLOR_EOL, ch);
        return false;
    }

    Event* event = get_event_by_trigger(entity, trig);
    const EventTypeInfo* trig_info = get_event_type_info((EventTrigger)trig);
    if (trig_info == NULL) {
        printf_to_char(ch, COLOR_INFO "No event type metadata for trigger '%s'." COLOR_EOL, trig_arg);
        return false;
    }

    if (!str_prefix(cmd_arg, "set")) {
        Event* event_new = NULL;
        char cb_arg[MIL];
        char criteria_arg[MIL];
        READ_ARG(cb_arg);

        if (event == NULL) {
            if ((event_new = new_event()) == NULL) {
                perror("load_event: could not allocate new Event!");
                exit(-1);
            }
            event = event_new;
        }

        event->trigger = trig;

        if (!IS_NULLSTR(cb_arg)) {
            event->method_name = lox_string(cb_arg);
        }
        else {
            event->method_name = lox_string(get_event_default_callback(trig));
        }

        READ_ARG(criteria_arg);
        if (criteria_arg[0] != '\0') {
            // Validate provided criteria against the expected type.
            switch (trig_info->criteria_type) {
            case CRIT_NONE:
                printf_to_char(ch, COLOR_INFO "Trigger '%s' does not accept a criteria." COLOR_EOL, trig_arg);
                return false;
            case CRIT_INT:
                if (!is_number(criteria_arg)) {
                    printf_to_char(ch, COLOR_INFO "Trigger '%s' expects a numeric criteria (e.g. 100)." COLOR_EOL, trig_arg);
                    return false;
                }
                event->criteria = INT_VAL(atoi(criteria_arg));
                break;
            case CRIT_INT_OR_STR:
                if (is_number(criteria_arg))
                    event->criteria = INT_VAL(atoi(criteria_arg));
                else
                    event->criteria = OBJ_VAL(lox_string(criteria_arg));
                break;
            case CRIT_STR:
            default:
                event->criteria = OBJ_VAL(lox_string(criteria_arg));
                break;
            }
        }
        else {
            // No criteria supplied: use sensible default if available, else 
            // error for required types.
            if (trig_info->criteria_type == CRIT_NONE) {
                event->criteria = NIL_VAL;
            }
            else if (trig_info->default_criteria != NULL) {
                if (trig_info->criteria_type == CRIT_INT) {
                    event->criteria = INT_VAL(atoi(trig_info->default_criteria));
                }
                else {
                    event->criteria = OBJ_VAL(lox_string(trig_info->default_criteria));
                }
                printf_to_char(ch, COLOR_INFO "No criteria specified; using default '%s'." COLOR_EOL, trig_info->default_criteria);
            }
            else {
                printf_to_char(ch, COLOR_INFO "Trigger '%s' requires a criteria of type %s. Please supply one." COLOR_EOL,
                    trig_arg,
                    (trig_info->criteria_type == CRIT_INT) ? "number" : "string");
                if (cb_arg[0] != '\0')
                printf_to_char(ch, COLOR_ALT_TEXT_2 "    (Did you maybe mean to use the default callback '%s'? Default callbacks aren't available if you need to set criteria. Try " COLOR_INFO "'EVENT SET %s %s'" COLOR_ALT_TEXT_2 ".)" COLOR_EOL,
                    trig_info->default_callback, trig_info->default_callback, cb_arg);
                return false;
            }
        }

        if (event_new != NULL) {
            add_event(entity, event_new);
            send_to_char(COLOR_INFO "Event created." COLOR_EOL, ch);
        }
        else {
            send_to_char(COLOR_INFO "Event changed." COLOR_EOL, ch);
        }

        return true;
    }
    else if (!str_prefix(cmd_arg, "delete")) {
        if (event != NULL) {
            send_to_char(COLOR_INFO "That trigger does not exist." COLOR_EOL,
                ch);
            return false;
        }

        remove_event(entity, event);
        send_to_char(COLOR_INFO "Event deleted." COLOR_EOL, ch);

        return true;
    }

    send_to_char(help, ch);
    return false;
}

void olc_display_events(Mobile* ch, Entity* entity)
{
    if (entity->events.count == 0) {
        printf_to_char(ch, "%-14s : " COLOR_ALT_TEXT_1 "(none)" 
            COLOR_ALT_TEXT_2 " Type '" COLOR_INFO "EVENT" COLOR_ALT_TEXT_2 
            "' to create one." COLOR_EOL, "Events");
        return;
    }

    send_to_char(
        "Events:\n\r"
        COLOR_TITLE   "  Trigger           Callback (Args)     Criteria\n\r"
        COLOR_DECOR_2 "=========== ================ ========== ========\n\r", ch);

    Node* node = entity->events.front;
    while (node != NULL) {
        Event* event = AS_EVENT(node->value);

        char* trigger = capitalize(get_event_name((EventTrigger)event->trigger));
        char* callback = (event->method_name != NULL) ? event->method_name->chars : "(none)";
        char* args = "(mob)";
        char* criteria = (!IS_NIL(event->criteria)) ? string_value(event->criteria) : "";

        const EventTypeInfo* info = get_event_type_info((EventTrigger)event->trigger);
        if (info != NULL) {
            if (info->trigger == TRIG_ACT || info->trigger == TRIG_SPEECH)
                args = "(mob, msg)";
            else if (info->trigger == TRIG_GIVE)
                args = "(mob, obj)";
            else if (info->trigger == TRIG_PRDSTART || info->trigger == TRIG_PRDSTOP) {
                // Determine args based on entity type
                EventEnts ent_type = get_entity_type(entity);
                if (ent_type & ENT_AREA)
                    args = "(area)";
                else if (ent_type & ENT_ROOM)
                    args = "(room)";
                else
                    args = "()";
            }
            else if (info->criteria_type == CRIT_INT)
                args = "(mob, num)";
            else if (info->criteria_type == CRIT_STR)
                args = "(mob, str)";
            else if (info->criteria_type == CRIT_INT_OR_STR)
                args = "(mob, num|str)";
            else
                args = "(mob)";
        }

        printf_to_char(ch, COLOR_TITLE "" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%9.9s" COLOR_DECOR_1 "]"
            COLOR_ALT_TEXT_2 " %16.16s %-10.10s "
            COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%6.6s" COLOR_DECOR_1 "]"
            COLOR_EOL,
            trigger, callback, args, criteria);

        node = node->next;
    }
}
