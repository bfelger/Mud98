////////////////////////////////////////////////////////////////////////////////
// olc/event_edit.c
////////////////////////////////////////////////////////////////////////////////

#include "event_edit.h"

#include "bit.h"
#include "olc.h"

#include <entities/event.h>

#include <comm.h>
#include <db.h>
#include <lookup.h>
#include <tables.h>

bool olc_edit_event(Mobile* ch, char* argument)
{
    static const char* help =
        COLOR_INFO "Syntax: event set <trigger> [<callback> [<criteria>]]\n\r"
        "        event delete <trigger>" COLOR_EOL;

    Entity* entity;

    EDIT_ENTITY(ch, entity);

    EventEnts ent_type = get_entity_type(entity);

    if (ent_type == 0) {
        send_to_char(COLOR_INFO "Cannot attach events to that entity."
            COLOR_EOL, ch);
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

    if (IS_NULLSTR(cmd_arg) || IS_NULLSTR(trig_arg)) {
        send_to_char(help, ch);
        return false;
    }

    // The criteria expression, if any, will be in the remainder in "argument".
    FLAGS trig = flag_lookup(trig_arg, mprog_flag_table);
    if (trig == NO_FLAG) {
        printf_to_char(ch, COLOR_INFO "No trigger '%s' found. These are the "
            "available options:\n\r" COLOR_ALT_TEXT_1, trig_arg);
        int j = 0;
        for (int i = 0; mprog_flag_table[i].name != NULL; ++i)
            if (HAS_BIT(event_default_configs[i].valid_ents, ent_type)) {
                printf_to_char(ch, "%10s%s", mprog_flag_table[i],
                    (j++ % 3 == 0) ? "\n\r" : "");
            }
        send_to_char(COLOR_EOL, ch);
        return false;
    }

    Event* event = get_event_by_trigger(entity, trig);

    if (!str_prefix(cmd_arg, "set")) {
        Event* event_new = NULL;
        char cb_arg[MIL];
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

        // TODO: Read the remainder of the 'argument' as a Criteria Lox Value.
        event->criteria = NIL_VAL;

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
        "\n\rEvents:\n\r"
        COLOR_TITLE   " Trigger          Callback (Args)     Criteria\n\r"
        COLOR_DECOR_2 "========= ================ ========== ========\n\r", ch);

    Node* node = entity->events.front;
    while (node != NULL) {
        Event* event = AS_EVENT(node->value);

        char* trigger = capitalize(flag_string(mprog_flag_table, event->trigger));
        char* callback = (event->method_name != NULL) ? event->method_name->chars : "(none)";
        char* args = "";
        char* criteria = (!IS_NIL(event->criteria)) ? string_value(event->criteria) : "";

        switch (event->trigger) {
        case TRIG_ACT:
        case TRIG_SPEECH:
            args = "(mob, msg)";
            break;
        case TRIG_BRIBE:
        case TRIG_HPCNT:
            args = "(mob, amt)";
            break;
        case TRIG_GIVE:
            args = "(mob, obj)";
            break;
        case TRIG_DEATH:
        case TRIG_ENTRY:
        case TRIG_FIGHT:
        case TRIG_GREET:
        case TRIG_GRALL:
        case TRIG_KILL:
        case TRIG_RANDOM:
        case TRIG_EXIT:
        case TRIG_EXALL:
        case TRIG_DELAY:
        case TRIG_SURR:
        case TRIG_LOGIN:
        default:
            args = "(mob)";
            break;
        }

        printf_to_char(ch, COLOR_TITLE "" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%7.7s" COLOR_DECOR_1 "]"
            COLOR_ALT_TEXT_2 " %16.16s %-10.10s "
            COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%6.6s" COLOR_DECOR_1 "]"
            COLOR_EOL,
            trigger, callback, args, criteria);

        node = node->next;
    }
}
