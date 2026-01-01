////////////////////////////////////////////////////////////////////////////////
// olc/redit.c
////////////////////////////////////////////////////////////////////////////////

#include <merc.h>

#include "bit.h"
#include "event_edit.h"
#include "lox_edit.h"
#include "olc.h"
#include "period_edit.h"
#include "string_edit.h"

#include <act_move.h>
#include <comm.h>
#include <db.h>
#include <format.h>
#include <handler.h>
#include <lookup.h>
#include <magic.h>
#include <recycle.h>
#include <save.h>
#include <stringbuffer.h>
#include <tables.h>
#include <string.h>

#include <entities/event.h>
#include <entities/object.h>
#include <entities/room.h>

#define REDIT(fun) bool fun( Mobile *ch, char *argument )

void display_exits(Mobile* ch, RoomData* pRoom);
void display_resets(Mobile* ch, RoomData* pRoom);

RoomData xRoom;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

REDIT(redit_period);

const OlcCmdEntry room_olc_comm_table[] = {
    { "name",	    U(&xRoom.header.name),  ed_line_lox_string, 0		        },
    { "desc",	    U(&xRoom.description),	ed_desc,		    0		        },
    { "ed",	        U(&xRoom.extra_desc),	ed_ed,			    0		        },
    { "heal",	    U(&xRoom.heal_rate),	ed_number_s_pos,	0		        },
    { "mana",	    U(&xRoom.mana_rate),	ed_number_s_pos,	0		        },
    { "owner",	    U(&xRoom.owner),		ed_line_string,		0		        },
    { "roomflags",	U(&xRoom.room_flags),	ed_flag_toggle,		U(room_flag_table)	},
    { "flags",	    U(&xRoom.room_flags),	ed_flag_toggle,		U(room_flag_table)	},
    { "room_flags",	U(&xRoom.room_flags),	ed_flag_toggle,		U(room_flag_table)	},
    { "clan",	    U(&xRoom.clan),		    ed_int16lookup,		U(clan_lookup)	},
    { "sector",	    U(&xRoom.sector_type),	ed_flag_set_sh,		U(sector_flag_table)},
    { "north",	    0,				        ed_direction,		DIR_NORTH	    },
    { "south",	    0,				        ed_direction,		DIR_SOUTH	    },
    { "east",	    0,				        ed_direction,		DIR_EAST	    },
    { "west",	    0,				        ed_direction,		DIR_WEST	    },
    { "up",	        0,				        ed_direction,		DIR_UP		    },
    { "down",	    0,				        ed_direction,		DIR_DOWN	    },
    { "rlist",	    0,				        ed_olded,		    U(redit_rlist)	},
    { "mlist",	    0,				        ed_olded,		    U(redit_mlist)	},
    { "olist",	    U(&xRoom.area_data),    ed_olist,           0               },
    { "copy",	    0,				        ed_olded,		    U(redit_copy)	},
    { "period",     0,                      ed_olded,           U(redit_period) },
    { "event",      0,                      ed_olded,           U(olc_edit_event)   },
    { "lox",        0,                      ed_olded,           U(olc_edit_lox)     },
    { "listreset",	0,				        ed_olded,		    U(redit_listreset)  },
    { "checkobj",	0,				        ed_olded,		    U(redit_checkobj)	},
    { "checkmob",	0,				        ed_olded,		    U(redit_checkmob)	},
    { "checkrooms", 0,				        ed_olded,		    U(redit_checkrooms) },
    { "mreset",	    0,				        ed_olded,		    U(redit_mreset)	},
    { "oreset",	    0,				        ed_olded,		    U(redit_oreset)	},
    { "create",	    0,				        ed_olded,		    U(redit_create)	},
    { "format",	    0,				        ed_olded,		    U(redit_format)	},
    { "mshow",	    0,				        ed_olded,		    U(medit_show)	},
    { "oshow",	    0,				        ed_olded,		    U(oedit_show)	},
    { "purge",	    0,				        ed_docomm,		    U(do_purge)	    },
    { "clear",	    0,				        ed_olded,		    U(redit_clear)},
    { "commands",	0,				        ed_olded,		    U(show_commands)},
    { "?",		    0,				        ed_olded,		    U(show_help)	},
    { "version",	0,				        ed_olded,		    U(show_version)	},
    { NULL,	        0,				        NULL,			    0		        }
};

/* Entry point for editing room_index_data. */
void do_redit(Mobile* ch, char* argument)
{
    RoomData* room_data = NULL;
    char arg1[MIL];

    READ_ARG(arg1);

    room_data = ch->in_room->data;

    if (!str_cmp(arg1, "reset")) {
        if (!IS_BUILDER(ch, room_data->area_data)) {
            send_to_char(COLOR_INFO "You do not have enough security to edit rooms." COLOR_EOL, ch);
            return;
        }

        Room* room;
        FOR_EACH_ROOM_INST(room, room_data)
            reset_room(room);
        send_to_char(COLOR_INFO "Room reset." COLOR_EOL, ch);
        return;
    }
    else if (!str_cmp(arg1, "create")) {
        if (argument[0] == '\0' || atoi(argument) == 0) {
            send_to_char(COLOR_INFO "Syntax: edit room create [vnum]" COLOR_EOL, ch);
            return;
        }

        if (redit_create(ch, argument)) {
            mob_from_room(ch);
            room_data = (RoomData*)get_pEdit(ch->desc);
            Room* room = get_room_for_player(ch, VNUM_FIELD(room_data));
            mob_to_room(ch, room);
            SET_BIT(room_data->area_data->area_flags, AREA_CHANGED);
        }
    }
    else if (!IS_NULLSTR(arg1)) {
        room_data = get_room_data(atoi(arg1));

        if (room_data == NULL) {
            send_to_char(COLOR_INFO "That room does not exist." COLOR_EOL, ch);
            return;
        }
    }

    if (!IS_BUILDER(ch, room_data->area_data)) {
        send_to_char(COLOR_INFO "You do not have enough security to edit rooms." COLOR_EOL, ch);
        return;
    }

    if (room_data == NULL) {
        bugf("redit: NULL room_data, ch %s!\n\r", NAME_STR(ch));
        return;
    }

    if (ch->in_room->data != room_data) {
        mob_from_room(ch);
        Room* room = get_room_for_player(ch, VNUM_FIELD(room_data));
        mob_to_room(ch, room);
    }

    set_editor(ch->desc, ED_ROOM, U(room_data));

    redit_show(ch, "");

    return;
}

/* Room Interpreter, called by do_redit. */
void redit(Mobile* ch, char* argument)
{
    RoomData* pRoom;
    AreaData* area;

    EDIT_ROOM(ch, pRoom);
    area = pRoom->area_data;

    if (!IS_BUILDER(ch, area)) {
        send_to_char(COLOR_INFO "Insufficient security to modify room." COLOR_EOL, ch);
        edit_done(ch);
        return;
    }

    if (!str_cmp(argument, "done")) {
        edit_done(ch);
        return;
    }

    if (emptystring(argument)) {
        redit_show(ch, argument);
        return;
    }

    /* Search Table and Dispatch Command. */
    if (!process_olc_command(ch, argument, room_olc_comm_table))
        interpret(ch, argument);

    return;
}

REDIT(redit_period)
{
    RoomData* room;
    EDIT_ROOM(ch, room);
    return olc_edit_period(ch, argument, &room->header /*, get_period_ops_for_room()*/);
}

REDIT(redit_rlist)
{
    RoomData* pRoomIndex;
    AreaData* area;
    StringBuffer* sb;
    char arg[MAX_INPUT_LENGTH]  = { 0 };
    bool found;
    VNUM vnum;
    int  col = 0;

    one_argument(argument, arg);

    area = ch->in_room->area->data;
    sb = sb_new();
    found = false;

    for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
        if ((pRoomIndex = get_room_data(vnum))) {
            found = true;
            sb_appendf(sb, COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR " %-17.17s ",
                vnum, capitalize(NAME_STR(pRoomIndex)));
            if (++col % 3 == 0)
                sb_append(sb, "\n\r");
        }
    }

    if (!found) {
        sb_free(sb);
        send_to_char(COLOR_INFO "Room(s) not found in this area." COLOR_EOL, ch);
        return false;
    }

    if (col % 3 != 0)
        sb_append(sb, "\n\r");

    page_to_char(sb_string(sb), ch);
    sb_free(sb);
    return false;
}

REDIT(redit_mlist)
{
    MobPrototype* p_mob_proto;
    AreaData* area;
    StringBuffer* sb;
    char        arg[MAX_INPUT_LENGTH];
    bool fAll, found;
    VNUM vnum;
    int  col = 0;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Syntax:  " COLOR_ALT_TEXT_1 "mlist <all/name>" COLOR_EOL, ch);
        return false;
    }

    sb = sb_new();
    area = ch->in_room->area->data;
    fAll = !str_cmp(arg, "all");
    found = false;

    for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
        if ((p_mob_proto = get_mob_prototype(vnum)) != NULL) {
            if (fAll || is_name(arg, NAME_STR(p_mob_proto))) {
                found = true;
                sb_appendf(sb, COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR " %-17.17s",
                    VNUM_FIELD(p_mob_proto), capitalize(p_mob_proto->short_descr));
                if (++col % 3 == 0)
                    sb_append(sb, "\n\r");
            }
        }
    }

    if (!found) {
        sb_free(sb);
        send_to_char(COLOR_CLEAR "Mobile(s) not found in this area.\n\r" COLOR_CLEAR , ch);
        return false;
    }

    if (col % 3 != 0)
        sb_append(sb, "\n\r");

    page_to_char(sb_string(sb), ch);
    sb_free(sb);
    return false;
}

// Room Editor Functions.
REDIT(redit_show)
{
    RoomData* pRoom = NULL;
    Object* obj = NULL;
    Mobile* rch = NULL;
    bool fcnt;

    INIT_BUF(line, MAX_STRING_LENGTH);

    INIT_BUF(word, MAX_INPUT_LENGTH);
    INIT_BUF(reset_state, MAX_STRING_LENGTH);

    EDIT_ROOM(ch, pRoom);

    ////////////////////////////////////////////////////////////////////////////
    // LONG DESCRIPTION
    ////////////////////////////////////////////////////////////////////////////

    olc_print_text(ch, "Description", pRoom->description);

    ////////////////////////////////////////////////////////////////////////////
    // MAIN INFO BLOCK
    ////////////////////////////////////////////////////////////////////////////

    sprintf(BUF(line), COLOR_TITLE "%s", NAME_STR(pRoom));
    olc_print_num_str(ch, "Room", VNUM_FIELD(pRoom), BUF(line));
    olc_print_num_str(ch, "Area", VNUM_FIELD(pRoom->area_data), NAME_STR(pRoom->area_data));
    olc_print_flags(ch, "Sector", sector_flag_table, pRoom->sector_type);
    olc_print_flags(ch, "Room Flags", room_flag_table, pRoom->room_flags);
    olc_print_num(ch, "Heal Recover", pRoom->heal_rate);
    olc_print_num(ch, "Mana Recover", pRoom->mana_rate);

    if (pRoom->clan) {
        olc_print_num_str(ch, "Clan", pRoom->clan, 
            ((pRoom->clan > 0) ? clan_table[pRoom->clan].name : "(none)"));
    }

    if (pRoom->owner && pRoom->owner[0] != '\0') {
        olc_print_str(ch, "Owner", pRoom->owner);
    }

    olc_print_yesno_ex(ch, "Daycycle Msgs", pRoom->suppress_daycycle_messages,
        pRoom->suppress_daycycle_messages ? "Day-cycle messages are shown." :
        "Day-cycle messages are suppressed.");
    olc_show_periods(ch, &pRoom->header /*, get_period_ops_for_room()*/);

    ////////////////////////////////////////////////////////////////////////////
    // EXTRA DESCRIPTIONS
    ////////////////////////////////////////////////////////////////////////////

    if (pRoom->extra_desc) {
        ExtraDesc* ed;

        printf_to_char(ch, "%-14s : " COLOR_ALT_TEXT_1, "Desc Kwds");

        FOR_EACH(ed, pRoom->extra_desc) {
            printf_to_char(ch, ed->keyword);
            if (ed->next) {
                printf_to_char(ch, " ");
            }
        }
        printf_to_char(ch,  COLOR_EOL);
    }

    ////////////////////////////////////////////////////////////////////////////
    // ROOM CONTENTS
    ////////////////////////////////////////////////////////////////////////////

    printf_to_char(ch, "%-14s : " COLOR_ALT_TEXT_1, "Characters");

    fcnt = false;
    FOR_EACH_ROOM_MOB(rch, ch->in_room) {
        if (IS_NPC(rch) || can_see(ch, rch)) {
            one_argument(NAME_STR(rch), BUF(line));
            printf_to_char(ch, "%s ", BUF(line));
            fcnt = true;
        }
    }
    
    if (!fcnt) {
        printf_to_char(ch, "(none)");
    }
    
    printf_to_char(ch, COLOR_EOL);
    
    printf_to_char(ch, "%-14s : " COLOR_ALT_TEXT_1, "Objects");

    fcnt = false;
    FOR_EACH_ROOM_OBJ(obj, ch->in_room) {
        one_argument(NAME_STR(obj), BUF(line));
        printf_to_char(ch, "%s ", BUF(line));
        fcnt = true;
    }
    
    if (!fcnt) {
        printf_to_char(ch, "(none)");
    }

    printf_to_char(ch, COLOR_EOL);

    ////////////////////////////////////////////////////////////////////////////
    // EVENTS & LOX
    ////////////////////////////////////////////////////////////////////////////

    Entity* entity = &pRoom->header;
    olc_display_entity_class_info(ch, entity);
    olc_display_events(ch, entity);

    ////////////////////////////////////////////////////////////////////////////
    // EXITS
    ////////////////////////////////////////////////////////////////////////////

    display_exits(ch, pRoom);

    ////////////////////////////////////////////////////////////////////////////
    // RESETS
    ////////////////////////////////////////////////////////////////////////////

    if (ch->in_room->data->reset_first) {
        send_to_char(
            "\n\rResets: M = mobile, R = room, O = object, "
            "P = pet, S = shopkeeper\n\r", ch);
        display_resets(ch, ch->in_room->data);
    }

    free_buf(line);
    free_buf(word);
    free_buf(reset_state);

    return false;
}

/* Local function. */
bool change_exit(Mobile* ch, char* argument, Direction door)
{
    RoomData* room_data;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int  value;

    EDIT_ROOM(ch, room_data);

    // Set the exit flags, needs full argument.
    if ((value = flag_value(exit_flag_table, argument)) != NO_FLAG) {
        RoomData* to_room_data;
        RoomExitData* room_exit_data;
        RoomExitData *pNExit;
        int16_t rev;

        room_exit_data = room_data->exit_data[door];

        if (!room_exit_data) {
            send_to_char(COLOR_INFO "There is no exit in that direction." COLOR_EOL, ch);
            return false;
        }

        // This room.
        TOGGLE_BIT(room_exit_data->exit_reset_flags, value);

        /* Don't toggle exit_flags because it can be changed by players. */
        //room_exit->exit_reset_flags = room_exit->exit_reset_flags;

        // Connected room.
        to_room_data = room_exit_data->to_room;
        rev = dir_list[door].rev_dir;
        pNExit = to_room_data->exit_data[rev];

        if (pNExit) {
            TOGGLE_BIT(pNExit->exit_reset_flags, value);
            //pNExit->exit_flags = pNExit->exit_reset_flags;
        }

        send_to_char(COLOR_INFO "Exit flag toggled." COLOR_EOL, ch);
        return true;
    }

    // Now parse the arguments.
    READ_ARG(command);
    one_argument(argument, arg);

    if (command[0] == '\0' && argument[0] == '\0')    /* Move command. */
    {
        move_char(ch, door, true);                    /* ROM OLC */
        return false;
    }
    else if (command[0] == '?') {
        do_help(ch, "EXIT");
        return false;
    }
    else if (!str_cmp(command, "delete")) {
        RoomData* to_room_data;
        RoomExitData* room_exit_data;
        RoomExitData* pNExit;
        int16_t rev;
        bool rDeleted = false;

        room_exit_data = room_data->exit_data[door];

        if (!room_exit_data) {
            send_to_char(COLOR_INFO "REdit: Cannot delete a null exit." COLOR_EOL, ch);
            return false;
        }

        to_room_data = room_exit_data->to_room;

        // Remove ToRoom Exit.
        if (str_cmp(arg, "simple") && to_room_data) {
            rev = dir_list[door].rev_dir;
            pNExit = to_room_data->exit_data[rev];

            if (pNExit) {
                if (pNExit->to_room == room_data) {
                    rDeleted = true;
                    free_room_exit_data(to_room_data->exit_data[rev]);
                    to_room_data->exit_data[rev] = NULL;
                }
                else
                    printf_to_char(ch, COLOR_INFO "Exit %d to room %d does not return to"
                        " this room, so it was not deleted." COLOR_EOL,
                        rev, VNUM_FIELD(to_room_data));
            }
        }

        // Remove this exit.
        printf_to_char(ch, COLOR_INFO "Exit %s to room %d deleted." COLOR_EOL,
            dir_list[door].name, VNUM_FIELD(room_data));
        free_room_exit_data(room_data->exit_data[door]);
        room_data->exit_data[door] = NULL;

        if (rDeleted)
            printf_to_char(ch, COLOR_INFO "Exit %s to room %d was also deleted." COLOR_EOL,
                dir_list[dir_list[door].rev_dir].name, VNUM_FIELD(to_room_data));

        return true;
    }
    else if (!str_cmp(command, "link")) {
        RoomExitData* room_exit_data;
        RoomData* to_room_data;

        if (arg[0] == '\0' || !is_number(arg)) {
            send_to_char("Syntax:  " COLOR_ALT_TEXT_1 "[direction] link [vnum]" COLOR_EOL, ch);
            return false;
        }

        to_room_data = get_room_data(atoi(arg));

        if (!to_room_data) {
            send_to_char(COLOR_INFO "REdit:  Cannot link to non-existent room." COLOR_EOL, ch);
            return false;
        }

        if (!IS_BUILDER(ch, to_room_data->area_data)) {
            send_to_char(COLOR_INFO "REdit:  Cannot link to that area." COLOR_EOL, ch);
            return false;
        }

        room_exit_data = room_data->exit_data[door];

        if (room_exit_data) {
            send_to_char(COLOR_INFO "REdit : That exit already exists." COLOR_EOL, ch);
            return false;
        }

        room_exit_data = to_room_data->exit_data[dir_list[door].rev_dir];

        if (room_exit_data) {
            send_to_char(COLOR_INFO "REdit:  Remote side's exit already exists." COLOR_EOL, ch);
            return false;
        }

        if (room_data->area_data != to_room_data->area_data
            && room_data->area_data->inst_type == AREA_INST_MULTI
            && to_room_data->area_data->inst_type == AREA_INST_MULTI) {
            send_to_char(COLOR_INFO "REdit:  You cannot link between two different "
                "multi-instance areas.." COLOR_EOL, ch);
            return false;
        }

        room_exit_data = new_room_exit_data();
        room_exit_data->to_room = to_room_data;
        room_exit_data->to_vnum = VNUM_FIELD(to_room_data);
        room_exit_data->orig_dir = door;
        room_data->exit_data[door] = room_exit_data;

        Room* from_room;
        FOR_EACH_ROOM_INST(from_room, room_data)
            from_room->exit[door] = new_room_exit(room_exit_data, from_room);

        // Now the other side
        door = dir_list[door].rev_dir;
        room_exit_data = new_room_exit_data();
        room_exit_data->to_room = room_data;
        room_exit_data->to_vnum = VNUM_FIELD(room_data);
        room_exit_data->orig_dir = door;
        to_room_data->exit_data[door] = room_exit_data;

        Room* to_room;
        FOR_EACH_ROOM_INST(to_room, to_room_data)
            to_room->exit[door] = new_room_exit(room_exit_data, to_room);

        SET_BIT(to_room_data->area_data->area_flags, AREA_CHANGED);
        SET_BIT(room_data->area_data->area_flags, AREA_CHANGED);

        send_to_char(COLOR_INFO "Two-way link established." COLOR_EOL, ch);
        return true;
    } else if (!str_cmp(command, "dig")) {
        char buf[MAX_STRING_LENGTH];

        if (arg[0] == '\0' || !is_number(arg)) {
            send_to_char("Syntax: " COLOR_ALT_TEXT_1 "[direction] dig <vnum>" COLOR_EOL, ch);
            return false;
        }

        VNUM vnum = STRTOVNUM(arg);

        // Create changes our currently edited room.
        redit_create(ch, arg);

        // Remember the new room...
        Room* new_room = get_room_for_player(ch, vnum);

        // ...go back to the old room and create the exit...
        set_pEdit(ch->desc, U(room_data));
        sprintf(buf, "link %s", arg);
        change_exit(ch, buf, door);

        // ...then jump back to editing the new room.
        transfer_mob(ch, new_room);
        set_editor(ch->desc, ED_ROOM, U(new_room->data));

        redit_show(ch, "");

        return true;
    } else if (!str_cmp(command, "room")) {
        RoomExitData* room_exit_data;
        RoomData* target;

        if (arg[0] == '\0' || !is_number(arg)) {
            send_to_char("Syntax:  " COLOR_ALT_TEXT_1 "[direction] room [vnum]" COLOR_EOL, ch);
            return false;
        }

        value = atoi(arg);

        if ((target = get_room_data(value)) == NULL) {
            send_to_char(COLOR_INFO "REdit:  Cannot link to non-existant room." COLOR_EOL, ch);
            return false;
        }

        if (!IS_BUILDER(ch, target->area_data)) {
            send_to_char(COLOR_INFO "REdit: You do not have access to the room you wish to dig to." COLOR_EOL, ch);
            return false;
        }

        if ((room_exit_data = room_data->exit_data[door]) == NULL) {
            room_exit_data = new_room_exit_data();
            room_data->exit_data[door] = room_exit_data;
        }

        room_exit_data->to_room = target;
        room_exit_data->to_vnum = VNUM_FIELD(target);
        room_exit_data->orig_dir = door;

        Room* from_room;
        FOR_EACH_ROOM_INST(from_room, room_data)
            from_room->exit[door] = new_room_exit(room_exit_data, from_room);

        if ((room_exit_data = target->exit_data[dir_list[door].rev_dir]) != NULL
            && room_exit_data->to_room != room_data)
            printf_to_char(ch, COLOR_INFO "WARNING" COLOR_CLEAR " : the exit to room %d does not return here.\n\r",
                VNUM_FIELD(target));

        send_to_char(COLOR_INFO "One-way link established." COLOR_EOL, ch);
        return true;
    } else if (!str_cmp(command, "key")) {
        RoomExitData* room_exit_data;
        ObjPrototype* obj_proto;

        if (arg[0] == '\0' || !is_number(arg)) {
            send_to_char("Syntax:  " COLOR_ALT_TEXT_1 "[direction] key [vnum]" COLOR_EOL, ch);
            return false;
        }

        if ((room_exit_data = room_data->exit_data[door]) == NULL) {
            send_to_char(COLOR_INFO "That exit does not exist." COLOR_EOL, ch);
            return false;
        }

        obj_proto = get_object_prototype(atoi(arg));

        if (!obj_proto) {
            send_to_char(COLOR_INFO "REdit:  Item doesn't exist." COLOR_EOL, ch);
            return false;
        }

        if (obj_proto->item_type != ITEM_KEY) {
            send_to_char(COLOR_INFO "REdit:  Key doesn't exist." COLOR_EOL, ch);
            return false;
        }

        room_exit_data->key = (int16_t)atoi(arg);

        send_to_char(COLOR_INFO "Exit key set." COLOR_EOL, ch);
        return true;
    } else if (!str_cmp(command, "name")) {
        RoomExitData* room_exit_data;

        if (arg[0] == '\0') {
            send_to_char("Syntax:  " COLOR_ALT_TEXT_1 "[direction] name [string]\n\r", ch);
            send_to_char("         [direction] name none" COLOR_EOL, ch);
            return false;
        }

        if ((room_exit_data = room_data->exit_data[door]) == NULL) {
            send_to_char(COLOR_INFO "That exit does not exist." COLOR_EOL, ch);
            return false;
        }

        free_string(room_exit_data->keyword);

        if (str_cmp(arg, "none"))
            room_exit_data->keyword = str_dup(arg);
        else
            room_exit_data->keyword = str_dup("");

        send_to_char(COLOR_INFO "Exit name set." COLOR_EOL, ch);
        return true;
    } else if (!str_prefix(command, "description")) {
        RoomExitData* room_exit_data;

        if (arg[0] == '\0') {
            if ((room_exit_data = room_data->exit_data[door]) == NULL) {
                send_to_char(COLOR_INFO "That exit does not exist." COLOR_EOL, ch);
                return false;
            }

            string_append(ch, &room_exit_data->description);
            return true;
        }

        send_to_char("Syntax:  " COLOR_ALT_TEXT_1 "[direction] desc" COLOR_EOL, ch);
        return false;
    }

    return false;
}

REDIT(redit_create)
{
    AreaData* area_data;
    RoomData* room_data;
    VNUM value;

    EDIT_ROOM(ch, room_data);

    value = STRTOVNUM(argument);

    if (argument[0] == '\0' || value <= 0) {
        send_to_char("Syntax:  " COLOR_ALT_TEXT_1 "create [vnum > 0]" COLOR_EOL, ch);
        return false;
    }

    area_data = get_vnum_area(value);
    if (!area_data) {
        send_to_char(COLOR_INFO "REdit:  That vnum is not assigned an area." COLOR_EOL, ch);
        return false;
    }

    if (!IS_BUILDER(ch, area_data)) {
        send_to_char(COLOR_INFO "REdit:  Vnum in an area you cannot build in." COLOR_EOL, ch);
        return false;
    }

    if (get_room_data(value)) {
        send_to_char(COLOR_INFO "REdit:  Room vnum already exists." COLOR_EOL, ch);
        return false;
    }

    room_data = new_room_data();
    room_data->area_data = area_data;
    VNUM_FIELD(room_data) = value;
    room_data->sector_type = area_data->sector;
    room_data->room_flags = 0;

    if (value > top_vnum_room)
        top_vnum_room = value;

    global_room_set(room_data);

    Area* area;
    FOR_EACH_AREA_INST(area, area_data) {
        new_room(room_data, area);
    }

    set_editor(ch->desc, ED_ROOM, U(room_data));

    send_to_char(COLOR_INFO "Room created." COLOR_EOL, ch);
    return true;
}

REDIT(redit_format)
{
    RoomData* pRoom;

    EDIT_ROOM(ch, pRoom);

    char* desc = format_string(pRoom->description);
    free_string(pRoom->description);
    pRoom->description = desc;

    send_to_char(COLOR_INFO "String formatted." COLOR_EOL, ch);
    return true;
}

REDIT(redit_mreset)
{
    RoomData* room_data;
    MobPrototype* p_mob_proto;
    Mobile* newmob;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    Reset* reset;

    EDIT_ROOM(ch, room_data);

    READ_ARG(arg);
    READ_ARG(arg2);

    if (arg[0] == '\0' || !is_number(arg)) {
        send_to_char("Syntax:  " COLOR_ALT_TEXT_1 "mreset <vnum> <world max> <room max>" COLOR_EOL, ch);
        return false;
    }

    if (!(p_mob_proto = get_mob_prototype(atoi(arg)))) {
        send_to_char(COLOR_INFO "REdit: No mobile has that vnum." COLOR_EOL, ch);
        return false;
    }

    if (p_mob_proto->area != room_data->area_data) {
        send_to_char(COLOR_INFO "REdit: No such mobile in this area." COLOR_EOL, ch);
        return false;
    }

    // Create the mobile reset.
    reset = new_reset();
    reset->command = 'M';
    reset->arg1 = VNUM_FIELD(p_mob_proto);
    reset->arg2 = is_number(arg2) ? (int16_t)atoi(arg2) : MAX_MOB;
    reset->arg3 = VNUM_FIELD(room_data);
    reset->arg4 = is_number(argument) ? (int16_t)atoi(argument) : 1;
    add_reset(room_data, reset, 0/* Last slot*/);

    // Create the mobile.
    Room* room;
    FOR_EACH_ROOM_INST(room, room_data) {
        newmob = create_mobile(p_mob_proto);
        mob_to_room(newmob, room);
        if (room == ch->in_room)
            act("$n has created $N!", ch, NULL, newmob, TO_ROOM);
        else
            act("$n has been created!", newmob, NULL, NULL, TO_ROOM);
    }

    printf_to_char(ch, COLOR_INFO "%s (%d) has been added to resets.\n\r"
        "There will be a maximum of %d in the area, and %d in this room." COLOR_EOL,
        capitalize(p_mob_proto->short_descr),
        VNUM_FIELD(p_mob_proto),
        reset->arg2,
        reset->arg4);
    return true;
}

static Mobile* get_mob_instance(Room* room, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* mob = NULL;
    int number;
    int count;

    number = number_argument(argument, arg);
    count = 0;

    FOR_EACH_ROOM_MOB(mob, room) {
        if (!is_name(arg, NAME_STR(mob))) 
            continue;
        if (++count == number)
            return mob;
    }

    return NULL;
}

REDIT(redit_oreset)
{
    RoomData* room_data = NULL;
    ObjPrototype* obj_proto = NULL;
    Object* newobj = NULL;
    Object* to_obj = NULL;
    Mobile* to_mob = NULL;
    char vnum_str[MAX_INPUT_LENGTH];
    char into_arg[MAX_INPUT_LENGTH];
    LEVEL olevel = 0;

    Reset* reset;
    char output[MAX_STRING_LENGTH];

    EDIT_ROOM(ch, room_data);

    READ_ARG(vnum_str);
    READ_ARG(into_arg);

    if (vnum_str[0] == '\0' || !is_number(vnum_str)) {
        send_to_char("Syntax:  " COLOR_ALT_TEXT_1 "oreset <vnum> <args>" COLOR_EOL, ch);
        send_to_char("        -" COLOR_ALT_TEXT_1 "no_args" COLOR_CLEAR "               = into room\n\r", ch);
        send_to_char("        -" COLOR_ALT_TEXT_1 "<obj_name>" COLOR_CLEAR "            = into obj\n\r", ch);
        send_to_char("        -" COLOR_ALT_TEXT_1 "<mob_name> <wear_loc>" COLOR_CLEAR " = into mob\n\r", ch);
        return false;
    }

    if (!(obj_proto = get_object_prototype(atoi(vnum_str)))) {
        send_to_char(COLOR_INFO "REdit: No object has that vnum." COLOR_EOL, ch);
        return false;
    }

    if (obj_proto->area != room_data->area_data) {
        send_to_char(COLOR_INFO "REdit: No such object in this area." COLOR_EOL, ch);
        return false;
    }

    // Load into room.
    if (into_arg[0] == '\0') {
        reset = new_reset();
        reset->command = 'O';
        reset->arg1 = VNUM_FIELD(obj_proto);
        reset->arg2 = 0;
        reset->arg3 = VNUM_FIELD(room_data);
        reset->arg4 = 0;
        add_reset(room_data, reset, 0/* Last slot*/);

        Room* room;
        FOR_EACH_ROOM_INST(room, room_data) {
            newobj = create_object(obj_proto, (int16_t)number_fuzzy(olevel));
            obj_to_room(newobj, room);
        }

        sprintf(output, COLOR_INFO "%s (%d) has been loaded and added to resets." COLOR_EOL,
            capitalize(obj_proto->short_descr),
            VNUM_FIELD(obj_proto));
        send_to_char(output, ch);
    }
    else if (argument[0] == '\0' && ((to_obj = get_obj_list(ch, into_arg, &AS_ROOM(room_data->instances.front->value)->objects)) != NULL)) {
        // Load into object's inventory.
        reset = new_reset();
        reset->command = 'P';
        reset->arg1 = VNUM_FIELD(obj_proto);
        reset->arg2 = 0;
        reset->arg3 = VNUM_FIELD(to_obj->prototype);
        reset->arg4 = 1;
        add_reset(room_data, reset, 0/* Last slot*/);

        Room* room;
        FOR_EACH_ROOM_INST(room, room_data) {
            to_obj = get_obj_list(ch, into_arg, &room->objects);
            newobj = create_object(obj_proto, (int16_t)number_fuzzy(olevel));
            newobj->cost = 0;
            obj_to_obj(newobj, to_obj);
        }

        if (newobj && to_obj) {
            sprintf(output, COLOR_INFO "%s (%d) has been loaded into "
                "%s (%d) and added to resets." COLOR_EOL,
                capitalize(newobj->short_descr),
                VNUM_FIELD(newobj->prototype),
                to_obj->short_descr,
                VNUM_FIELD(to_obj->prototype));
            send_to_char(output, ch);
        }
    }
    else if ((to_mob = get_mob_instance(AS_ROOM(room_data->instances.front->value), into_arg)) != NULL) {
        // Load into mobile's inventory.
        int wearloc;

        // Make sure the location on mobile is valid.
        if ((wearloc = flag_value(wear_loc_flag_table, argument)) == NO_FLAG) {
            send_to_char("REdit: Invalid wear_loc.  '? wear-loc'\n\r", ch);
            return false;
        }

        // Disallow loading a sword(WEAR_WIELD) into WEAR_HEAD.
        if (!IS_SET(obj_proto->wear_flags, wear_bit(wearloc))) {
            sprintf(output,
                "%s (%d) has wear flags: [%s]\n\r",
                capitalize(obj_proto->short_descr),
                VNUM_FIELD(obj_proto),
                flag_string(wear_flag_table, obj_proto->wear_flags));
            send_to_char(output, ch);
            return false;
        }

        // Can't load into same position.
        if (get_eq_char(to_mob, wearloc)) {
            send_to_char("REdit:  Object already equipped.\n\r", ch);
            return false;
        }

        reset = new_reset();
        reset->arg1 = VNUM_FIELD(obj_proto);
        reset->arg2 = (int16_t)wearloc;
        if (reset->arg2 == WEAR_UNHELD)
            reset->command = 'G';
        else
            reset->command = 'E';
        reset->arg3 = wearloc;

        add_reset(room_data, reset, 0/* Last slot*/);

        Room* room;
        FOR_EACH_ROOM_INST(room, room_data) {
            to_mob = get_mob_instance(room, into_arg);
            if (to_mob == NULL) {
                send_to_char("REdit:  Not all instances have that mob.\n\r", ch);
            }

            olevel = URANGE(0, to_mob->level - 2, LEVEL_HERO);
            newobj = create_object(obj_proto, (int16_t)number_fuzzy(olevel));

            if (to_mob->prototype->pShop)    /* Shop-keeper? */
            {
                switch (obj_proto->item_type) {
                default:            olevel = 0;                                break;
                case ITEM_PILL:     olevel = (LEVEL)number_range(0, 10);    break;
                case ITEM_POTION:    olevel = (LEVEL)number_range(0, 10);    break;
                case ITEM_SCROLL:    olevel = (LEVEL)number_range(5, 15);    break;
                case ITEM_WAND:        olevel = (LEVEL)number_range(10, 20);    break;
                case ITEM_STAFF:    olevel = (LEVEL)number_range(15, 25);    break;
                case ITEM_ARMOR:    olevel = (LEVEL)number_range(5, 15);    break;
                case ITEM_WEAPON:
                    if (reset->command == 'G')
                        olevel = (LEVEL)number_range(5, 15);
                    else
                        olevel = (LEVEL)number_fuzzy(olevel);
                    break;
                }

                newobj = create_object(obj_proto, olevel);
                if (reset->arg2 == WEAR_UNHELD)
                    SET_BIT(newobj->extra_flags, ITEM_INVENTORY);
            }
            else
                newobj = create_object(obj_proto, (int16_t)number_fuzzy(olevel));

            obj_to_char(newobj, to_mob);
            if (reset->command == 'E')
                equip_char(to_mob, newobj, (int16_t)reset->arg3);

            sprintf(output, "%s (%d) has been loaded "
                "%s of %s (%d) and added to resets.\n\r",
                capitalize(obj_proto->short_descr),
                VNUM_FIELD(obj_proto),
                flag_string(wear_loc_strings, reset->arg3),
                to_mob->short_descr,
                VNUM_FIELD(to_mob->prototype));
            send_to_char(output, ch);

            if (room == ch->in_room)
                act("$n has created $p!", ch, newobj, NULL, TO_ROOM);
        }
    }
    else    /* Display Syntax */
    {
        send_to_char("REdit:  That mobile isn't here.\n\r", ch);
        return false;
    }

    return true;
}

REDIT(redit_owner)
{
    RoomData* pRoom;

    EDIT_ROOM(ch, pRoom);

    if (argument[0] == '\0') {
        send_to_char("Syntax:  owner [owner]\n\r", ch);
        send_to_char("         owner none\n\r", ch);
        return false;
    }

    free_string(pRoom->owner);
    if (!str_cmp(argument, "none"))
        pRoom->owner = str_dup("");
    else
        pRoom->owner = str_dup(argument);

    send_to_char("Owner set.\n\r", ch);
    return true;
}

void showresets(Mobile* ch, Buffer* buf, AreaData* area, MobPrototype* mob, ObjPrototype* obj)
{
    RoomData* room;
    MobPrototype* pLastMob;
    Reset* reset;
    char buf2[MIL];
    int lastmob;

    FOR_EACH_GLOBAL_ROOM(room) {
        if (room->area_data != area)
            continue;

        lastmob = -1;
        pLastMob = NULL;

        FOR_EACH(reset, room->reset_first) {
            if (reset->command == 'M') {
                lastmob = reset->arg1;
                pLastMob = get_mob_prototype(lastmob);
                if (pLastMob == NULL) {
                    bugf("Showresets : invalid reset (mob %d) in room %d",
                        lastmob, VNUM_FIELD(room));
                    return;
                }
                if (mob && lastmob == VNUM_FIELD(mob)) {
                    sprintf(buf2, "%-5d %-15.15s %-5d\n\r", lastmob,
                        NAME_STR(mob), VNUM_FIELD(room));
                    add_buf(buf, buf2);
                }
            }
            if (obj && reset->command == 'O' && reset->arg1 == VNUM_FIELD(obj)) {
                sprintf(buf2, "%-5d %-15.15s %-5d\n\r", VNUM_FIELD(obj),
                    NAME_STR(obj), VNUM_FIELD(room));
                add_buf(buf, buf2);
            }
            if (obj && (reset->command == 'G' || reset->command == 'E')
                && reset->arg1 == VNUM_FIELD(obj)) {
                sprintf(buf2, "%-5d %-15.15s %-5d %-5d %-15.15s\n\r",
                    VNUM_FIELD(obj), NAME_STR(obj), VNUM_FIELD(room), lastmob,
                    pLastMob ? NAME_STR(pLastMob) : "");
                add_buf(buf, buf2);
            }
        }
    }
}

void listobjreset(Mobile* ch, Buffer* buf, AreaData* area)
{
    ObjPrototype* obj;

    add_buf(buf, COLOR_TITLE "Vnum  Name            Room  On mob" COLOR_EOL);

    FOR_EACH_OBJ_PROTO(obj)
        if (obj->area == area)
            showresets(ch, buf, area, 0, obj);
}

void listmobreset(Mobile* ch, Buffer* buf, AreaData* area)
{
    MobPrototype* mob;

    add_buf(buf, COLOR_TITLE "Vnum  Name            Room " COLOR_EOL);

    FOR_EACH_MOB_PROTO(mob)
        if (mob->area == area)
            showresets(ch, buf, area, mob, 0);
}

REDIT(redit_listreset)
{
    AreaData* area;
    RoomData* pRoom;
    Buffer* buf;

    EDIT_ROOM(ch, pRoom);

    area = pRoom->area_data;

    if (IS_NULLSTR(argument)) {
        send_to_char("Syntax : listreset [obj/mob]\n\r", ch);
        return false;
    }

    buf = new_buf();

    if (!str_cmp(argument, "obj"))
        listobjreset(ch, buf, area);
    else if (!str_cmp(argument, "mob"))
        listmobreset(ch, buf, area);
    else {
        send_to_char("AEdit : Argument must be either 'obj' or 'mob'.\n\r", ch);
        free_buf(buf);
        return false;
    }

    page_to_char(BUF(buf), ch);

    return false;
}

REDIT(redit_checkobj)
{
    ObjPrototype* obj;
    bool fAll = !str_cmp(argument, "all");
    RoomData* room;

    EDIT_ROOM(ch, room);

    FOR_EACH_OBJ_PROTO(obj)
        if (obj->reset_num == 0 && (fAll || obj->area == room->area_data))
            printf_to_char(ch, "Obj " COLOR_ALT_TEXT_1 "%-5.5d" COLOR_CLEAR " [%-20.20s] is not reset.\n\r", VNUM_FIELD(obj), NAME_STR(obj));

    return false;
}

REDIT(redit_checkrooms)
{
    RoomData* room;
    RoomData* thisroom;
    bool fAll = false;

    if (!str_cmp(argument, "all"))
        fAll = true;
    else
        if (!IS_NULLSTR(argument)) {
            send_to_char("Syntax : checkrooms\n\r"
                "         checkrooms all\n\r", ch);
            return false;
        }

    EDIT_ROOM(ch, thisroom);

    FOR_EACH_GLOBAL_ROOM(room)
        if (room->reset_num == 0 
            && (fAll || room->area_data == thisroom->area_data))
            printf_to_char(ch, "Room %d has no resets.\n\r", VNUM_FIELD(room));

    return false;
}

REDIT(redit_checkmob)
{
    MobPrototype* mob;
    RoomData* room;
    bool fAll = !str_cmp(argument, "all");

    EDIT_ROOM(ch, room);

    FOR_EACH_MOB_PROTO(mob)
        if (mob->reset_num == 0 && (fAll || mob->area == room->area_data))
            printf_to_char(ch, "Mob " COLOR_ALT_TEXT_1 "%-5.5d" COLOR_CLEAR " [%-20.20s] has no resets.\n\r", VNUM_FIELD(mob), NAME_STR(mob));

    return false;
}

REDIT(redit_copy)
{
    RoomData* this;
    RoomData* that;
    VNUM vnum;

    EDIT_ROOM(ch, this);

    if (IS_NULLSTR(argument) || !is_number(argument)) {
        send_to_char("Syntax : copy [vnum]\n\r", ch);
        return false;
    }

    vnum = atoi(argument);

    if (vnum < 1 || vnum >= MAX_VNUM) {
        send_to_char("REdit : Vnum must be between 1 and MAX_VNUM.\n\r", ch);
        return false;
    }

    that = get_room_data(vnum);

    if (!that || !IS_BUILDER(ch, that->area_data) || that == this) {
        send_to_char("REdit : That room does not exist, or cannot be copied by you.\n\r", ch);
        return false;
    }

    free_string(this->description);
    free_string(this->owner);

    SET_NAME(this, NAME_FIELD(that));
    this->description = str_dup(that->description);
    this->owner = str_dup(that->owner);

    this->room_flags = that->room_flags;
    this->sector_type = that->sector_type;
    this->clan = that->clan;
    this->heal_rate = that->heal_rate;
    this->mana_rate = that->mana_rate;
    room_daycycle_period_clear(this);
    this->periods = room_daycycle_period_clone(that->periods);

    send_to_char("Ok. Room copied.\n\r", ch);
    return true;
}

ED_FUN_DEC(ed_direction)
{
    return change_exit(ch, argument, (Direction)par);
}

REDIT(redit_clear)
{
    RoomData* pRoom;
    int i;

    if (!IS_NULLSTR(argument)) {
        send_to_char("Syntax : clear\n\r", ch);
        return false;
    }

    EDIT_ROOM(ch, pRoom);

    pRoom->sector_type = SECT_INSIDE;
    pRoom->heal_rate = 100;
    pRoom->mana_rate = 100;
    pRoom->clan = 0;
    pRoom->room_flags = 0;
    free_string(pRoom->owner);
    free_string(pRoom->description);
    pRoom->owner = str_dup("");
    SET_NAME(pRoom, lox_empty_string);
    pRoom->description = str_dup("");

    for (i = 0; i < DIR_MAX; i++) {
        free_room_exit_data(pRoom->exit_data[i]);
        pRoom->exit_data[i] = NULL;
    }

    room_daycycle_period_clear(pRoom);

    send_to_char("Room cleared.\n\r", ch);
    return true;
}

void display_exits(Mobile* ch, RoomData* room)
{
    send_to_char(
        "\n\rExits:\n\r"
        COLOR_TITLE   "  Dir   To Vnum    Room Desc      Key     Reset Flags       Kwds\n\r"
        COLOR_DECOR_2 "======= ======= =============== ======= =============== ============\n\r", ch);

    for (int i = 0; i < DIR_MAX; i++) {
        RoomExitData* exit = room->exit_data[i];
        if (exit == NULL)
            continue;

        const char* dir = capitalize(dir_list[i].name);
        VNUM tgt_vnum = (exit->to_room != NULL) ? VNUM_FIELD(exit->to_room) : exit->to_vnum;
        char* tgt_desc = (exit->to_room != NULL) ? NAME_STR(exit->to_room) : "";
        VNUM key = exit->key;
        char* flags = flag_string(exit_flag_table, exit->exit_reset_flags);
        char* kwds = (exit->keyword != NULL) ? exit->keyword : "";

        printf_to_char(ch,
            /* Dir */   COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5s" COLOR_DECOR_1 "] "
            /* Vnum */  "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "] "
            /* Desc */  COLOR_TEXT "%15.15s "
            /* Key */   COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "] ["
            /* Flags */ COLOR_ALT_TEXT_1 "%13.13s" COLOR_DECOR_1 "] "
            /* Kwds */  COLOR_ALT_TEXT_2 "%s" COLOR_EOL,
            dir, tgt_vnum, tgt_desc, key, flags, kwds);

        char* desc = exit->description;
        if (desc && desc[0] != '\0') {
            printf_to_char(ch, COLOR_ALT_TEXT_2 "%s" COLOR_CLEAR, desc);
        }
    }
}

void display_resets(Mobile* ch, RoomData* pRoom)
{
    Reset* reset;
    MobPrototype* pMob = NULL;
    char    buf[MAX_STRING_LENGTH] = "";
    char    final[MAX_STRING_LENGTH] = "";
    int     iReset = 0;

    final[0] = '\0';

    send_to_char(
        COLOR_TITLE " No.  Loads    Description       Location         Vnum   Ar Rm Description"
        "\n\r"
        COLOR_DECOR_2 "==== ======== ============= =================== ======== ===== ==========="
        "\n\r", ch);

    FOR_EACH(reset, pRoom->reset_first) {
        ObjPrototype* pObj;
        MobPrototype* p_mob_proto;
        ObjPrototype* obj_proto;
        ObjPrototype* pObjToIndex;
        RoomData* pRoomIndex;

        final[0] = '\0';
        sprintf(final, COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%2d" COLOR_DECOR_1 "]" COLOR_CLEAR " ", ++iReset);

        switch (reset->command) {
        default:
            sprintf(buf, "Bad reset command: %c.", reset->command);
            strcat(final, buf);
            break;

        case 'M':
            if (!(p_mob_proto = get_mob_prototype(reset->arg1))) {
                sprintf(buf, "Load Mobile - Bad Mob %d\n\r", reset->arg1);
                strcat(final, buf);
                continue;
            }

            if (!(pRoomIndex = get_room_data(reset->arg3))) {
                sprintf(buf, "Load Mobile - Bad Room %d\n\r", reset->arg3);
                strcat(final, buf);
                continue;
            }

            pMob = p_mob_proto;
            sprintf(buf, "M" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR " %-13.13s                     R" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR " %2d-%-2d" COLOR_ALT_TEXT_1 " %-15.15s" COLOR_EOL,
                reset->arg1, pMob->short_descr, reset->arg3,
                reset->arg2, reset->arg4, NAME_STR(pRoomIndex));
            strcat(final, buf);

            // Check for pet shop.
            {
                RoomData* pRoomIndexPrev;

                pRoomIndexPrev = get_room_data(VNUM_FIELD(pRoomIndex) - 1);
                if (pRoomIndexPrev
                    && IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
                    final[5] = 'P';
            }

            break;

        case 'O':
            if (!(obj_proto = get_object_prototype(reset->arg1))) {
                sprintf(buf, "Load Object - Bad Object %d\n\r",
                    reset->arg1);
                strcat(final, buf);
                continue;
            }

            pObj = obj_proto;

            if (!(pRoomIndex = get_room_data(reset->arg3))) {
                sprintf(buf, "Load Object - Bad Room %d\n\r", reset->arg3);
                strcat(final, buf);
                continue;
            }

            sprintf(buf, "O" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR " %-13.13s                     "
                "R" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR "       %-15.15s\n\r",
                reset->arg1, pObj->short_descr,
                reset->arg3, NAME_STR(pRoomIndex));
            strcat(final, buf);

            break;

        case 'P':
            if (!(obj_proto = get_object_prototype(reset->arg1))) {
                sprintf(buf, "Put Object - Bad Object %d\n\r",
                    reset->arg1);
                strcat(final, buf);
                continue;
            }

            pObj = obj_proto;

            if (!(pObjToIndex = get_object_prototype(reset->arg3))) {
                sprintf(buf, "Put Object - Bad To Object %d\n\r",
                    reset->arg3);
                strcat(final, buf);
                continue;
            }

            sprintf(buf,
                "O" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR " %-13.13s inside              O" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR " %2d-%-2d %-15.15s\n\r",
                reset->arg1,
                pObj->short_descr,
                reset->arg3,
                reset->arg2,
                reset->arg4,
                pObjToIndex->short_descr);
            strcat(final, buf);

            break;

        case 'G':
        case 'E':
            if (!(obj_proto = get_object_prototype(reset->arg1))) {
                sprintf(buf, "Give/Equip Object - Bad Object %d\n\r",
                    reset->arg1);
                strcat(final, buf);
                continue;
            }

            pObj = obj_proto;

            if (!pMob) {
                sprintf(buf, "Give/Equip Object - No Previous Mobile\n\r");
                strcat(final, buf);
                break;
            }

            if (pMob->pShop) {
                sprintf(buf,
                    "O" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR " %-13.13s in the inventory of S" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR "       %-15.15s\n\r",
                    reset->arg1,
                    pObj->short_descr,
                    VNUM_FIELD(pMob),
                    pMob->short_descr);
            }
            else
                sprintf(buf,
                    "O" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR " %-13.13s %-19.19s M" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR "       %-15.15s\n\r",
                    reset->arg1,
                    pObj->short_descr,
                    (reset->command == 'G') ?
                    flag_string(wear_loc_strings, WEAR_UNHELD)
                    : flag_string(wear_loc_strings, reset->arg3),
                    VNUM_FIELD(pMob),
                    pMob->short_descr);
            strcat(final, buf);
            break;

            /*
             * Doors are set in exit_reset_flags don't need to be displayed.
             * If you want to display them then uncomment the new_reset
             * line in the case 'D' in load_resets in db.c and here.
             */
        case 'D':
            pRoomIndex = get_room_data(reset->arg1);
            sprintf(buf, "R" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR " %s door of %-19.19s reset to %s\n\r",
                reset->arg1,
                capitalize(dir_list[reset->arg2].name),
                NAME_STR(pRoomIndex),
                flag_string(door_resets, reset->arg3));
            strcat(final, buf);

            break;
            // End Doors Comment.
        case 'R':
            if (!(pRoomIndex = get_room_data(reset->arg1))) {
                sprintf(buf, "Randomize Exits - Bad Room %d\n\r", reset->arg1);
                strcat(final, buf);
                continue;
            }

            sprintf(buf, "R" COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "]" COLOR_CLEAR " Exits are randomized in %s\n\r",
                reset->arg1, NAME_STR(pRoomIndex));
            strcat(final, buf);

            break;
        }
        send_to_char(final, ch);
    }

    return;
}

/*****************************************************************************
 Name:		add_reset
 Purpose:	Inserts a new reset in the given index slot.
 Called by:	do_resets(olc.c).
 ****************************************************************************/
void add_reset(RoomData* room, Reset* reset, int index)
{
    Reset* room_reset;
    int curr_idx = 0;

    if (!room->reset_first) {
        room->reset_first = reset;
        room->reset_last = reset;
        reset->next = NULL;
        return;
    }

    index--;

    if (index == 0) {
        // First slot (1) selected.
        reset->next = room->reset_first;
        room->reset_first = reset;
        return;
    }

    // If negative slot( <= 0 selected) then this will find the last.
    FOR_EACH(room_reset, room->reset_first) {
        if (++curr_idx == index)
            break;
    }

    reset->next = reset->next;
    reset->next = reset;
    if (!reset->next)
        room->reset_last = reset;
    return;
}

void do_resets(Mobile* ch, char* argument)
{
    static const char* help =
        "Syntax: " COLOR_ALT_TEXT_1 "RESET <number> OBJ <vnum> <wear_loc>\n\r"
        "        RESET <number> OBJ <vnum> inside <vnum> [limit] [count]\n\r"
        "        RESET <number> OBJ <vnum> room\n\r"
        "        RESET <number> MOB <vnum> [max #x area] [max #x room]\n\r"
        "        RESET <number> DELETE\n\r"
        "        RESET <number> RANDOM [#x exits]" COLOR_EOL;

    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char arg4[MAX_INPUT_LENGTH];
    char arg5[MAX_INPUT_LENGTH];
    char arg6[MAX_INPUT_LENGTH];
    char arg7[MAX_INPUT_LENGTH];
    Reset* reset = NULL;

    READ_ARG(arg1);
    READ_ARG(arg2);
    READ_ARG(arg3);
    READ_ARG(arg4);
    READ_ARG(arg5);
    READ_ARG(arg6);
    READ_ARG(arg7);

    if (!IS_BUILDER(ch, ch->in_room->area->data)) {
        send_to_char("Resets: Invalid security for editing this area.\n\r",
            ch);
        return;
    }

    // Display resets in current room.
    if (arg1[0] == '\0') {
        if (ch->in_room->data->reset_first) {
            send_to_char(
                "Resets: M = mobile, R = room, O = object, "
                "P = pet, S = shopkeeper\n\r", ch);
            display_resets(ch, ch->in_room->data);
        }
        else
            send_to_char("No resets in this room.\n\r", ch);
        return;
    }

    // Take index number and search for commands.
    if (is_number(arg1)) {
        RoomData* pRoom = ch->in_room->data;

        // Delete a reset.
        if (!str_cmp(arg2, "delete")) {
            int insert_loc = atoi(arg1);

            if (!pRoom->reset_first) {
                send_to_char("No resets in this area.\n\r", ch);
                return;
            }

            if (insert_loc - 1 <= 0) {
                reset = pRoom->reset_first;
                NEXT_LINK(pRoom->reset_first);
                if (!pRoom->reset_first)
                    pRoom->reset_last = NULL;
            }
            else {
                int iReset = 0;
                Reset* prev = NULL;

                FOR_EACH(reset, pRoom->reset_first) {
                    if (++iReset == insert_loc)
                        break;
                    prev = reset;
                }

                if (!reset) {
                    send_to_char("Reset not found.\n\r", ch);
                    return;
                }

                if (prev)
                    NEXT_LINK(prev->next);
                else
                    NEXT_LINK(pRoom->reset_first);

                for (pRoom->reset_last = pRoom->reset_first;
                    pRoom->reset_last->next;
                    NEXT_LINK(pRoom->reset_last));
            }

            free_reset(reset);
            send_to_char("Reset deleted.\n\r", ch);
            SET_BIT(ch->in_room->area->data->area_flags, AREA_CHANGED);
        }
        else
            // Add a reset.
            if ((!str_cmp(arg2, "mob") && is_number(arg3))
                || (!str_cmp(arg2, "obj") && is_number(arg3))) {
                 // Check for Mobile reset.
                if (!str_cmp(arg2, "mob")) {
                    if (get_mob_prototype(is_number(arg3) ? atoi(arg3) : 1) == NULL) {
                        send_to_char("That mob does not exist.\n\r", ch);
                        return;
                    }
                    reset = new_reset();
                    reset->command = 'M';
                    reset->arg1 = (VNUM)atoi(arg3);
                    reset->arg2 = is_number(arg4) ? (int16_t)atoi(arg4) : 1;	/* Max # */
                    reset->arg3 = VNUM_FIELD(ch->in_room);
                    reset->arg4 = is_number(arg5) ? (int16_t)atoi(arg5) : 1;	/* Min # */
                }
                else
                    // Check for Object reset.
                    if (!str_cmp(arg2, "obj")) {
                        // Inside another object.
                        if (!str_prefix(arg4, "inside")) {
                            ObjPrototype* temp;

                            temp = get_object_prototype(is_number(arg5) ? (VNUM)atoi(arg5) : 1);
                            if ((temp->item_type != ITEM_CONTAINER) &&
                                (temp->item_type != ITEM_CORPSE_NPC)) {
                                send_to_char("Object 2 is not a container.\n\r", ch);
                                return;
                            }
                            reset = new_reset();
                            reset->arg1 = (VNUM)atoi(arg3);
                            reset->command = 'P';
                            reset->arg2 = is_number(arg6) ? (int16_t)atoi(arg6) : 1;
                            reset->arg3 = is_number(arg5) ? (VNUM)atoi(arg5) : 1;
                            reset->arg4 = is_number(arg7) ? (int16_t)atoi(arg7) : 1;
                        }
                        else if (!str_cmp(arg4, "room")) {
                            // Inside the room.
                            if (get_object_prototype(atoi(arg3)) == NULL) {
                                send_to_char("That object does not exist.\n\r", ch);
                                return;
                            }
                            reset = new_reset();
                            reset->arg1 = (VNUM)atoi(arg3);
                            reset->command = 'O';
                            reset->arg2 = 0;
                            reset->arg3 = VNUM_FIELD(ch->in_room);
                            reset->arg4 = 0;
                        }
                        else {
                            // Into a Mobile's inventory.
                            FLAGS blah = flag_value(wear_loc_flag_table, arg4);

                            if (blah == NO_FLAG) {
                                send_to_char("Resets: '? wear-loc'\n\r", ch);
                                return;
                            }
                            if (get_object_prototype(atoi(arg3)) == NULL) {
                                send_to_char("That vnum does not exist.\n\r", ch);
                                return;
                            }
                            reset = new_reset();
                            reset->arg1 = atoi(arg3);
                            reset->arg3 = blah;
                            if (reset->arg3 == WEAR_UNHELD)
                                reset->command = 'G';
                            else
                                reset->command = 'E';
                        }
                    }
                add_reset(ch->in_room->data, reset, atoi(arg1));
                SET_BIT(ch->in_room->area->data->area_flags, AREA_CHANGED);
                send_to_char("Reset added.\n\r", ch);
            }
            else if (!str_cmp(arg2, "random") && is_number(arg3)) {
                if (atoi(arg3) < 1 || atoi(arg3) > 6) {
                    send_to_char("Invalid argument.\n\r", ch);
                    return;
                }
                reset = new_reset();
                reset->command = 'R';
                reset->arg1 = VNUM_FIELD(ch->in_room);
                reset->arg2 = (int16_t)atoi(arg3);
                add_reset(ch->in_room->data, reset, atoi(arg1));
                SET_BIT(ch->in_room->area->data->area_flags, AREA_CHANGED);
                send_to_char("Random exits reset added.\n\r", ch);
            }
            else {
                send_to_char(help, ch);
            }
    }
    if (!str_cmp(arg1, "add")) {
        int tvar = 0, found = 0;
        char* arg;

        if (is_number(arg2)) {
            send_to_char(COLOR_INFO "Invalid syntax." COLOR_EOL
                "Your options are:\n\r"
                COLOR_ALT_TEXT_1 "reset add mob [vnum/name]\n\r"
                "reset add obj [vnum/name]\n\r"
                "reset add [name]" COLOR_EOL, ch);
            return;
        }

        if (!str_cmp(arg2, "mob")) {
            tvar = 1;
            arg = arg3;
        }
        else
            if (!str_cmp(arg2, "obj")) {
                tvar = 2;
                arg = arg3;
            }
            else
                arg = arg2;

        if (tvar == 0 || tvar == 1) {
            if (is_number(arg))
                found = get_mob_prototype(atoi(arg)) ? atoi(arg) : 0;
            else
                found = get_vnum_mob_name_area(arg, ch->in_room->area->data);
            if (found)
                tvar = 1;
        }

        if (found == 0 && (tvar == 0 || tvar == 2)) {
            if (is_number(arg))
                found = get_object_prototype(atoi(arg)) ? atoi(arg) : 0;
            else
                found = get_vnum_obj_name_area(arg, ch->in_room->area->data);
            if (found)
                tvar = 2;
        }

        if (found == 0) {
            printf_to_char(ch, "%s was not found in the area.\n\r",
                (tvar == 0) ? "Mob/object" : ((tvar == 1) ? "Mob" : "Object"));
            return;
        }
        reset = new_reset();
        reset->command = tvar == 1 ? 'M' : 'O';
        reset->arg1 = found;
        reset->arg2 = (tvar == 2) ? 0 : MAX_MOB;	/* Max # */
        reset->arg3 = VNUM_FIELD(ch->in_room);
        reset->arg4 = (tvar == 2) ? 0 : MAX_MOB;	/* Min # */

        printf_to_char(ch, "Added reset of %s %d...", tvar == 1 ? "mob" : "object", found);
        add_reset(ch->in_room->data, reset, -1); // al final
        SET_BIT(ch->in_room->area->data->area_flags, AREA_CHANGED);
        send_to_char("Done.\n\r", ch);
    } 
    else if (!str_cmp(arg1, "help") || !str_cmp(arg1, "?")) {
        send_to_char(help, ch);
    }

    return;
}

void do_objlist(Mobile* ch, char* argument)
{
    static const char* help = COLOR_INFO "Syntax: OBJLIST AREA\n\r"
        "          OBJLIST WORLD [low-level] [high-level]" COLOR_EOL;
    static const int max_disp = 100;
    char opt[MIL] = { 0 };
    char lo_str[MIL] = { 0 };
    char hi_str[MIL] = { 0 };
    char type_str[MIL] = { 0 };
    int count = 0;
    LEVEL lo_lvl = 0;
    LEVEL hi_lvl = MAX_LEVEL;
    bool world = false;

    INIT_BUF(out, MSL);

    if (!IS_BUILDER(ch, ch->in_room->area->data)) {
        send_to_char(COLOR_ALT_TEXT_1 "Invalid security for editing this area." COLOR_EOL, ch);
        return;
    }

    READ_ARG(opt);

    if (!opt[0]) {
        send_to_char(help, ch);
        return;
    }

    if (!str_prefix(opt, "world")) {
        READ_ARG(lo_str);
        READ_ARG(hi_str);

        if (!lo_str[0] || !is_number(lo_str) || !hi_str[0] || !is_number(hi_str)) {
            send_to_char(help, ch);
            return;
        }

        lo_lvl = (LEVEL)atoi(lo_str);
        hi_lvl = (LEVEL)atoi(hi_str);
        world = true;
    }
    else if (str_prefix(opt, "area")) {
        send_to_char(help, ch);
        return;
    }

    READ_ARG(type_str);
    ItemType type = -1;

    if (type_str[0]) {
        for (int i = 0; i < ITEM_TYPE_COUNT; ++i)
            if (!str_prefix(type_str, item_type_table[i].name)) {
                type = item_type_table[i].type;
                break;
            }
    }

    //               ################################################################################
    switch (type) {
    case ITEM_ARMOR:
        addf_buf(out, COLOR_TITLE "VNUM   Lvl Name                AC:  Pierce  Bash    Slash   Exotic        " COLOR_EOL);
        break;    
    case ITEM_CONTAINER:
        addf_buf(out, COLOR_TITLE "VNUM   Lvl Name                     Wght    Flags   Key     Cpcty   WtMult" COLOR_EOL);
        break;
    default:
        addf_buf(out, COLOR_TITLE "VNUM   Lvl Name       Type          Val0    Val1    Val2    Val3    Val4  " COLOR_EOL);
    }
    addf_buf(out, COLOR_DECOR_2 "===========================================================================" COLOR_EOL);

    VNUM hi_vnum = ch->in_room->area->data->max_vnum;
    VNUM lo_vnum = ch->in_room->area->data->min_vnum;

    ObjPrototype* obj;
    FOR_EACH_OBJ_PROTO(obj) {
        if (count > max_disp) {
            addf_buf(out, "Max display threshold reached.\n\r");
            goto max_disp_reached;
        }
        if (!world && (VNUM_FIELD(obj) < lo_vnum || VNUM_FIELD(obj) > hi_vnum))
            continue;
        if (world && (obj->level < lo_lvl || obj->level > hi_lvl))
            continue;
        if (type > 0 && obj->item_type != type)
            continue;

        addf_buf(out, COLOR_ALT_TEXT_1 "%-6d " COLOR_ALT_TEXT_1 "%-3d" COLOR_CLEAR " ", VNUM_FIELD(obj), obj->level);

        switch (type) {
        case ITEM_ARMOR:
        case ITEM_CONTAINER:
            addf_buf(out, "%-23.23s  " COLOR_DECOR_1 "", obj->short_descr);
            break;
        default:
            addf_buf(out, "%-10.10s %-12.12s " COLOR_DECOR_1 "", obj->short_descr, flag_string(type_flag_table, obj->item_type));
        }

        for (int i = 0; i < 5; ++i) {
            addf_buf(out, "[" COLOR_ALT_TEXT_1 "%5d" COLOR_DECOR_1 "] ", obj->value[i]);
        }

        addf_buf(out, COLOR_EOL);
    }
max_disp_reached:
    addf_buf(out, "\n\r");
    page_to_char(BUF(out), ch);
    free_buf(out);
}

void do_moblist(Mobile* ch, char* argument)
{
    static const char* help = COLOR_INFO "Syntax: MOBLIST AREA\n\r"
        "          MOBLIST WORLD [low-level] [high-level]" COLOR_EOL;
    static const int max_disp = 100;
    char opt[MIL] = { 0 };
    char lo_str[MIL] = { 0 };
    char hi_str[MIL] = { 0 };
    char buf[MSL];
    int count = 0;
    LEVEL lo_lvl = 0;
    LEVEL hi_lvl = MAX_LEVEL;
    bool world = false;

    INIT_BUF(out, MSL);

    if (!IS_BUILDER(ch, ch->in_room->area->data)) {
        send_to_char(COLOR_ALT_TEXT_1 "Invalid security for editing this area." COLOR_EOL, ch);
        return;
    }

    READ_ARG(opt);

    if (!opt[0]) {
        send_to_char(help, ch);
        return;
    }

    if (!str_prefix(opt, "world")) {
        READ_ARG(lo_str);
        READ_ARG(hi_str);

        if (!lo_str[0] || !is_number(lo_str) || !hi_str[0] || !is_number(hi_str)) {
            send_to_char(help, ch);
            return;
        }

        lo_lvl = (LEVEL)atoi(lo_str);
        hi_lvl = (LEVEL)atoi(hi_str);
        world = true;
    }
    else if (str_prefix(opt, "area")) {
        send_to_char(help, ch);
        return;
    }
    //               ################################################################################
    addf_buf(out, COLOR_TITLE "VNUM   Name       Lvl Hit Dice     Hit   Dam      Mana       Pie  Bas  Sla  Mag" COLOR_EOL);
    addf_buf(out, COLOR_DECOR_2 "===============================================================================" COLOR_EOL);

    VNUM hi_vnum = ch->in_room->area->data->max_vnum;
    VNUM lo_vnum = ch->in_room->area->data->min_vnum;

    MobPrototype* mob;

    FOR_EACH_MOB_PROTO(mob) {
        if (count > max_disp) {
            addf_buf(out, "Max display threshold reached.\n\r");
            goto max_disp_reached;
        }
        if (!world && (VNUM_FIELD(mob) < lo_vnum || VNUM_FIELD(mob) > hi_vnum))
            continue;
        if (world && (mob->level < lo_lvl || mob->level > hi_lvl))
            continue;

        //VNUM   Name       Lvl Hit Dice   Hit   Dam Dice   Mana       Pie Bas Sla Mag
        //###### ########## ### ########## ##### ######## ########## ### ### ### ###
        addf_buf(out, COLOR_ALT_TEXT_1 "%-6d" COLOR_CLEAR " %-10.10s " COLOR_ALT_TEXT_1 "%-3d ",
            VNUM_FIELD(mob), mob->short_descr, mob->level);
        sprintf(buf, "%dd%d+%d", mob->hit[0], mob->hit[1], mob->hit[2]);
        addf_buf(out, "%-12.12s %-5d ", buf, mob->hitroll);
        sprintf(buf, "%dd%d+%d", mob->damage[0], mob->damage[1], mob->damage[2]);
        addf_buf(out, "%-8.8s ", buf);
        sprintf(buf, "%dd%d+%d", mob->mana[0], mob->mana[1], mob->mana[2]);
        addf_buf(out, "%-9.9s ", buf);
        addf_buf(out, "%4d %4d %4d %4d" COLOR_EOL, mob->ac[0], mob->ac[1], mob->ac[2], mob->ac[3]);
    }
max_disp_reached:
    addf_buf(out, "\n\r");
    page_to_char(BUF(out), ch);
    free_buf(out);
}
