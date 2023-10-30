////////////////////////////////////////////////////////////////////////////////
// redit.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "act_move.h"
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
#include "tables.h"

#include "entities/object.h"
#include "entities/room.h"

#define REDIT(fun) bool fun( Mobile *ch, char *argument )

void display_resets(Mobile* ch, RoomData* pRoom);

RoomData xRoom;

#ifdef U
#define OLD_U U
#endif
#define U(x)    (uintptr_t)(x)

const OlcCmdEntry room_olc_comm_table[] = {
    { "name",	    U(&xRoom.name),		    ed_line_string,		0		        },
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
            send_to_char("{jYou do not have enough security to edit rooms.{x\n\r", ch);
            return;
        }

        Room* room;
        FOR_EACH_INSTANCE(room, room_data->instances)
            reset_room(room);
        send_to_char("{jRoom reset.{x\n\r", ch);
        return;
    }
    else if (!str_cmp(arg1, "create")) {
        if (argument[0] == '\0' || atoi(argument) == 0) {
            send_to_char("Syntax: {*edit room create [vnum]{x\n\r", ch);
            return;
        }

        if (redit_create(ch, argument)) {
            char_from_room(ch);
            room_data = (RoomData*)ch->desc->pEdit;
            char_to_room(ch, room_data->instances);
            SET_BIT(room_data->area_data->area_flags, AREA_CHANGED);
        }
    }
    else if (!IS_NULLSTR(arg1)) {
        room_data = get_room_data(atoi(arg1));

        if (room_data == NULL) {
            send_to_char("{jThat room does not exist.{x\n\r", ch);
            return;
        }
    }

    if (!IS_BUILDER(ch, room_data->area_data)) {
        send_to_char("{jYou do not have enough security to edit rooms.{x\n\r", ch);
        return;
    }

    if (room_data == NULL) {
        bugf("{jdo_redit: NULL room_data, ch %s!{x", ch->name);
        return;
    }

    if (ch->in_room->data != room_data) {
        char_from_room(ch);
        Room* room = get_room(ch->in_room->area, room_data->vnum);
        // TODO:
        //if (to_room == NULL) {
        //    // Get area_data from room_data and create a new instance with 
        //    // all rooms, mobs, and objs needed.
        //    room = create_instance(ch, room_data);
        //}
        char_to_room(ch, room);
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
        send_to_char("{jREdit:  Insufficient security to modify room.{x\n\r", ch);
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

REDIT(redit_rlist)
{
    RoomData* pRoomIndex;
    AreaData* area;
    Buffer* buf1;
    char arg[MAX_INPUT_LENGTH]  = { 0 };
    bool found;
    VNUM vnum;
    int  col = 0;

    one_argument(argument, arg);

    area = ch->in_room->area->data;
    buf1 = new_buf();
    found = false;

    for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
        if ((pRoomIndex = get_room_data(vnum))) {
            found = true;
            addf_buf(buf1, "{|[{*%5d{|]{x %-17.17s ",
                vnum, capitalize(pRoomIndex->name));
            if (++col % 3 == 0)
                add_buf(buf1, "\n\r");
        }
    }

    if (!found) {
        free_buf(buf1);
        send_to_char("{jRoom(s) not found in this area.{x\n\r", ch);
        return false;
    }

    if (col % 3 != 0)
        add_buf(buf1, "\n\r");

    page_to_char(BUF(buf1), ch);
    free_buf(buf1);
    return false;
}

REDIT(redit_mlist)
{
    MobPrototype* p_mob_proto;
    AreaData* area;
    char        buf[MAX_STRING_LENGTH];
    Buffer* buf1;
    char        arg[MAX_INPUT_LENGTH];
    bool fAll, found;
    VNUM vnum;
    int  col = 0;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Syntax:  {*mlist <all/name>{x\n\r", ch);
        return false;
    }

    buf1 = new_buf();
    area = ch->in_room->area->data;
    fAll = !str_cmp(arg, "all");
    found = false;

    for (vnum = area->min_vnum; vnum <= area->max_vnum; vnum++) {
        if ((p_mob_proto = get_mob_prototype(vnum)) != NULL) {
            if (fAll || is_name(arg, p_mob_proto->name)) {
                found = true;
                sprintf(buf, "{|[{*%5d{|]{x %-17.17s",
                    p_mob_proto->vnum, capitalize(p_mob_proto->short_descr));
                add_buf(buf1, buf);
                if (++col % 3 == 0)
                    add_buf(buf1, "\n\r");
            }
        }
    }

    if (!found) {
        free_buf(buf1);
        send_to_char("{xMobile(s) not found in this area.\n\r{x", ch);
        return false;
    }

    if (col % 3 != 0)
        add_buf(buf1, "\n\r");

    page_to_char(BUF(buf1), ch);
    free_buf(buf1);
    return false;
}

// Room Editor Functions.
REDIT(redit_show)
{
    RoomData* pRoom;
    Object* obj;
    Mobile* rch;
    int cnt = 0;
    bool fcnt;

    INIT_BUF(line, MAX_STRING_LENGTH);
    INIT_BUF(out, 2 * MAX_STRING_LENGTH);

    INIT_BUF(word, MAX_INPUT_LENGTH);
    INIT_BUF(reset_state, MAX_STRING_LENGTH);

    EDIT_ROOM(ch, pRoom);

    addf_buf(out, "Description:\n\r{_%s{x", pRoom->description);
    addf_buf(out, "Name:       {|[{*%s{|]{x\n\rArea:       {|[{*%5d{|] {_%s{x\n\r",
        pRoom->name, pRoom->area_data->vnum, pRoom->area_data->name);
    addf_buf(out, "Vnum:       {|[{*%5d{|]{x\n\rSector:     {|[{*%s{|]{x\n\r",
        pRoom->vnum, flag_string(sector_flag_table, pRoom->sector_type));
    addf_buf(out, "Room flags: {|[{*%s{|]{x\n\r",
        flag_string(room_flag_table, pRoom->room_flags));
    addf_buf(out, "Heal rec:   {|[{*%d{|]{x\n\rMana rec:   {|[{*%d{|]{x\n\r",
        pRoom->heal_rate, pRoom->mana_rate);

    if (pRoom->clan) {
        addf_buf(out, "Clan:       {|[{*%d{|] {_%s{x\n\r", pRoom->clan,
            ((pRoom->clan > 0) ? clan_table[pRoom->clan].name : "none"));
    }

    if (pRoom->owner && pRoom->owner[0] != '\0') {
        addf_buf(out, "Owner:      {|[{*%s{|]{x\n\r", pRoom->owner);
    }

    if (pRoom->extra_desc) {
        ExtraDesc* ed;

        add_buf(out, "Desc Kwds:  {|[{*");
        FOR_EACH(ed, pRoom->extra_desc) {
            add_buf(out, ed->keyword);
            if (ed->next)
                add_buf(out, " ");
        }
        add_buf(out, "{|]{x\n\r");
    }

    add_buf(out, "Characters: {|[{*");
    fcnt = false;
    FOR_EACH_IN_ROOM(rch, ch->in_room->people) {
        if (IS_NPC(rch) || can_see(ch, rch)) {
            one_argument(rch->name, BUF(line));
            if (fcnt)
                add_buf(out, " ");
            add_buf(out, BUF(line));
            fcnt = true;
        }
    }
    
    if (!fcnt)
        add_buf(out, "none");
    
    add_buf(out, "{|]{x\n\r");
    
    add_buf(out, "Objects:    {|[{*");
    fcnt = false;
    FOR_EACH_CONTENT(obj, ch->in_room->contents) {
        one_argument(obj->name, BUF(line));
            add_buf(out, " ");
        add_buf(out, BUF(line));
        fcnt = true;
    }
    
    if (!fcnt)
        add_buf(out, "none");
    
    add_buf(out, "{|]{x\n\r");

    add_buf(out, "Exits:\n\r");

    for (cnt = 0; cnt < DIR_MAX; cnt++) {
        char* state;
        int i;
        size_t length;

        if (pRoom->exit_data[cnt] == NULL)
            continue;

        addf_buf(out, "    %-5s:  {|[{*%5d{|]{x Key: {|[{*%5d{|]{x",
            capitalize(dir_list[cnt].name),
            pRoom->exit_data[cnt]->to_room ? pRoom->exit_data[cnt]->to_room->vnum : 
            0, pRoom->exit_data[cnt]->key);

    /*
     * Format up the exit info.
     * Capitalize all flags that are not part of the reset info.
     */
        strcpy(BUF(reset_state), flag_string(exit_flag_table, pRoom->exit_data[cnt]->exit_reset_flags));
        state = flag_string(exit_flag_table, pRoom->exit_data[cnt]->exit_reset_flags);
        add_buf(out, " Exit flags: {|[{*");
        fcnt = false;
        for (; ;) {
            state = one_argument(state, BUF(word));

            if (BUF(word)[0] == '\0') {
                add_buf(out, "{|]{x\n\r");
                break;
            }

            if (str_infix(BUF(word), BUF(reset_state))) {
                length = strlen(BUF(word));
                for (i = 0; i < (int)length; i++)
                    BUF(word)[i] = UPPER(BUF(word)[i]);
            }
            add_buf(out, BUF(word));
            if (fcnt)
                add_buf(out, " ");
            fcnt = true;
        }

        if (pRoom->exit_data[cnt]->keyword && pRoom->exit_data[cnt]->keyword[0] != '\0') {
            addf_buf(out, "Kwds: {|[{*%s{|]{x\n\r", pRoom->exit_data[cnt]->keyword);
        }
        if (pRoom->exit_data[cnt]->description && pRoom->exit_data[cnt]->description[0] != '\0') {
            addf_buf(out, "        {_%s{x", pRoom->exit_data[cnt]->description);
        }
    }

    send_to_char(BUF(out), ch);

    free_buf(out);
    free_buf(line);

    free_buf(word);
    free_buf(reset_state);

    if (ch->in_room->data->reset_first) {
        send_to_char(
            "Resets: M = mobile, R = room, O = object, "
            "P = pet, S = shopkeeper\n\r", ch);
        display_resets(ch, ch->in_room->data);
    }

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
        RoomExitData* room_exit;
        RoomExitData *pNExit;
        int16_t rev;

        room_exit = room_data->exit_data[door];

        if (!room_exit) {
            send_to_char("{jThere is no exit in that direction.{x\n\r", ch);
            return false;
        }

        // This room.
        TOGGLE_BIT(room_exit->exit_reset_flags, value);

        /* Don't toggle exit_flags because it can be changed by players. */
        //room_exit->exit_reset_flags = room_exit->exit_reset_flags;

        // Connected room.
        to_room_data = room_exit->to_room;
        rev = dir_list[door].rev_dir;
        pNExit = to_room_data->exit_data[rev];

        if (pNExit) {
            TOGGLE_BIT(pNExit->exit_reset_flags, value);
            //pNExit->exit_flags = pNExit->exit_reset_flags;
        }

        send_to_char("{jExit flag toggled.{x\n\r", ch);
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

    if (command[0] == '?') {
        do_help(ch, "EXIT");
        return false;
    }

    if (!str_cmp(command, "delete")) {
        RoomData* to_room_data;
        RoomExitData* room_exit;
        RoomExitData* pNExit;
        int16_t rev;
        bool rDeleted = false;

        room_exit = room_data->exit_data[door];

        if (!room_exit) {
            send_to_char("{jREdit: Cannot delete a null exit.{x\n\r", ch);
            return false;
        }

        to_room_data = room_exit->to_room;

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
                    printf_to_char(ch, "{jExit %d to room %d does not return to"
                        " this room, so it was not deleted.{x\n\r",
                        rev, to_room_data->vnum);
            }
        }

        // Remove this exit.
        printf_to_char(ch, "{jExit %s to room %d deleted.{x\n\r",
            dir_list[door].name, room_data->vnum);
        free_room_exit_data(room_data->exit_data[door]);
        room_data->exit_data[door] = NULL;

        if (rDeleted)
            printf_to_char(ch, "{jExit %s to room %d was also deleted.{x\n\r",
                dir_list[dir_list[door].rev_dir].name, to_room_data->vnum);

        return true;
    }

    if (!str_cmp(command, "link")) {
        RoomExitData* room_exit;
        RoomData* to_room_data;

        if (arg[0] == '\0' || !is_number(arg)) {
            send_to_char("Syntax:  {*[direction] link [vnum]{x\n\r", ch);
            return false;
        }

        to_room_data = get_room_data(atoi(arg));

        if (!to_room_data) {
            send_to_char("{jREdit:  Cannot link to non-existent room.{x\n\r", ch);
            return false;
        }

        if (!IS_BUILDER(ch, to_room_data->area_data)) {
            send_to_char("{jREdit:  Cannot link to that area.{x\n\r", ch);
            return false;
        }

        room_exit = room_data->exit_data[door];

        if (room_exit) {
            send_to_char("{jREdit : That exit already exists.{x\n\r", ch);
            return false;
        }

        room_exit = to_room_data->exit_data[dir_list[door].rev_dir];

        if (room_exit) {
            send_to_char("{jREdit:  Remote side's exit already exists.{x\n\r", ch);
            return false;
        }

        room_exit = new_room_exit_data();
        room_exit->to_room = to_room_data;
        room_exit->orig_dir = door;
        room_data->exit_data[door] = room_exit;

        // Now the other side
        door = dir_list[door].rev_dir;
        room_exit = new_room_exit_data();
        room_exit->to_room = room_data;
        room_exit->orig_dir = door;
        to_room_data->exit_data[door] = room_exit;

        SET_BIT(room_data->area_data->area_flags, AREA_CHANGED);

        send_to_char("{jTwo-way link established.{x\n\r", ch);
        return true;
    }

    if (!str_cmp(command, "dig")) {
        char buf[MAX_STRING_LENGTH];

        if (arg[0] == '\0' || !is_number(arg)) {
            send_to_char("Syntax: {*[direction] dig <vnum>{x\n\r", ch);
            return false;
        }

        VNUM vnum = STRTOVNUM(arg);

        // Create changes our currently edited room.
        redit_create(ch, arg);

        // Remember the new room...
        Room* new_room = get_room(ch->in_room->area, vnum);

        // ...go back to the old room and create the exit...
        ch->desc->pEdit = U(room_data);
        sprintf(buf, "link %s", arg);
        change_exit(ch, buf, door);

        // ...then jump back to editing the new room.
        char_from_room(ch);
        char_to_room(ch, new_room);
        set_editor(ch->desc, ED_ROOM, U(new_room->data));

        redit_show(ch, "");

        return true;
    }

    if (!str_cmp(command, "room")) {
        RoomExitData* room_exit;
        RoomData* target;

        if (arg[0] == '\0' || !is_number(arg)) {
            send_to_char("Syntax:  {*[direction] room [vnum]{x\n\r", ch);
            return false;
        }

        value = atoi(arg);

        if ((target = get_room_data(value)) == NULL) {
            send_to_char("{jREdit:  Cannot link to non-existant room.{x\n\r", ch);
            return false;
        }

        if (!IS_BUILDER(ch, target->area_data)) {
            send_to_char("{jREdit: You do not have access to the room you wish to dig to.{x\n\r", ch);
            return false;
        }

        if ((room_exit = room_data->exit_data[door]) == NULL) {
            room_exit = new_room_exit_data();
            room_data->exit_data[door] = room_exit;
        }

        room_exit->to_room = target;
        room_exit->orig_dir = door;

        if ((room_exit = target->exit_data[dir_list[door].rev_dir]) != NULL
            && room_exit->to_room != room_data)
            printf_to_char(ch, "{jWARNING{x : the exit to room %d does not return here.\n\r",
                target->vnum);

        send_to_char("{jOne-way link established.{x\n\r", ch);
        return true;
    }

    if (!str_cmp(command, "key")) {
        RoomExitData* room_exit;
        ObjPrototype* obj_proto;

        if (arg[0] == '\0' || !is_number(arg)) {
            send_to_char("Syntax:  {*[direction] key [vnum]{x\n\r", ch);
            return false;
        }

        if ((room_exit = room_data->exit_data[door]) == NULL) {
            send_to_char("{jThat exit does not exist.{x\n\r", ch);
            return false;
        }

        obj_proto = get_object_prototype(atoi(arg));

        if (!obj_proto) {
            send_to_char("{jREdit:  Item doesn't exist.{x\n\r", ch);
            return false;
        }

        if (obj_proto->item_type != ITEM_KEY) {
            send_to_char("{jREdit:  Key doesn't exist.{x\n\r", ch);
            return false;
        }

        room_exit->key = (int16_t)atoi(arg);

        send_to_char("{jExit key set.{x\n\r", ch);
        return true;
    }

    if (!str_cmp(command, "name")) {
        RoomExitData* room_exit;

        if (arg[0] == '\0') {
            send_to_char("Syntax:  {*[direction] name [string]\n\r", ch);
            send_to_char("         [direction] name none{x\n\r", ch);
            return false;
        }

        if ((room_exit = room_data->exit_data[door]) == NULL) {
            send_to_char("{jThat exit does not exist.{x\n\r", ch);
            return false;
        }

        free_string(room_exit->keyword);

        if (str_cmp(arg, "none"))
            room_exit->keyword = str_dup(arg);
        else
            room_exit->keyword = str_dup("");

        send_to_char("{jExit name set.{x\n\r", ch);
        return true;
    }

    if (!str_prefix(command, "description")) {
        RoomExitData* room_exit;

        if (arg[0] == '\0') {
            if ((room_exit = room_data->exit_data[door]) == NULL) {
                send_to_char("{jThat exit does not exist.{x\n\r", ch);
                return false;
            }

            string_append(ch, &room_exit->description);
            return true;
        }

        send_to_char("Syntax:  {*[direction] desc{x\n\r", ch);
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
        send_to_char("Syntax:  {*create [vnum > 0]{x\n\r", ch);
        return false;
    }

    area_data = get_vnum_area(value);
    if (!area_data) {
        send_to_char("{jREdit:  That vnum is not assigned an area.{x\n\r", ch);
        return false;
    }

    if (!IS_BUILDER(ch, area_data)) {
        send_to_char("{jREdit:  Vnum in an area you cannot build in.{x\n\r", ch);
        return false;
    }

    if (get_room_data(value)) {
        send_to_char("{jREdit:  Room vnum already exists.{x\n\r", ch);
        return false;
    }

    room_data = new_room_data();
    room_data->area_data = area_data;
    room_data->vnum = value;
    room_data->sector_type = area_data->sector;
    room_data->room_flags = 0;

    if (value > top_vnum_room)
        top_vnum_room = value;


    ORDERED_INSERT(RoomData, room_data, room_data_hash_table[value % MAX_KEY_HASH], vnum);

    Area* area;
    FOR_EACH(area, area_data->instances) {
        new_room(room_data, area);
    }

    set_editor(ch->desc, ED_ROOM, U(room_data));

    send_to_char("{jRoom created.{x\n\r", ch);
    return true;
}

REDIT(redit_format)
{
    RoomData* pRoom;

    EDIT_ROOM(ch, pRoom);

    pRoom->description = format_string(pRoom->description);

    send_to_char("{jString formatted.{x\n\r", ch);
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
        send_to_char("Syntax:  {*mreset <vnum> <max #x> <min #x>{x\n\r", ch);
        return false;
    }

    if (!(p_mob_proto = get_mob_prototype(atoi(arg)))) {
        send_to_char("{jREdit: No mobile has that vnum.{x\n\r", ch);
        return false;
    }

    if (p_mob_proto->area != room_data->area_data) {
        send_to_char("{jREdit: No such mobile in this area.{x\n\r", ch);
        return false;
    }

    // Create the mobile reset.
    reset = new_reset();
    reset->command = 'M';
    reset->arg1 = p_mob_proto->vnum;
    reset->arg2 = is_number(arg2) ? (int16_t)atoi(arg2) : MAX_MOB;
    reset->arg3 = room_data->vnum;
    reset->arg4 = is_number(argument) ? (int16_t)atoi(argument) : 1;
    add_reset(room_data, reset, 0/* Last slot*/);

    // Create the mobile.
    Room* room;
    FOR_EACH_INSTANCE(room, room_data->instances) {
        newmob = create_mobile(p_mob_proto);
        char_to_room(newmob, room);
        if (room == ch->in_room)
            act("$n has created $N!", ch, NULL, newmob, TO_ROOM);
        else
            act("$n has been created!", newmob, NULL, NULL, TO_ROOM);
    }

    printf_to_char(ch, "{j%s (%d) has been added to resets.\n\r"
        "There will be a maximum of %d in the area, and %d in this room.{x\n\r",
        capitalize(p_mob_proto->short_descr),
        p_mob_proto->vnum,
        reset->arg2,
        reset->arg4);
    return true;
}

static Mobile* get_mob_instance(Room* room, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* mob;
    int number;
    int count;

    number = number_argument(argument, arg);
    count = 0;

    FOR_EACH_IN_ROOM(mob, room->people) {
        if (!is_name(arg, mob->name)) 
            continue;
        if (++count == number)
            return mob;
    }

    return NULL;
}

REDIT(redit_oreset)
{
    RoomData* room_data;
    ObjPrototype* obj_proto;
    Object* newobj = NULL;
    Object* to_obj = NULL;
    Mobile* to_mob;
    char vnum_str[MAX_INPUT_LENGTH];
    char into_arg[MAX_INPUT_LENGTH];
    LEVEL olevel = 0;

    Reset* reset;
    char output[MAX_STRING_LENGTH];

    EDIT_ROOM(ch, room_data);

    READ_ARG(vnum_str);
    READ_ARG(into_arg);

    if (vnum_str[0] == '\0' || !is_number(vnum_str)) {
        send_to_char("Syntax:  {*oreset <vnum> <args>{x\n\r", ch);
        send_to_char("        -{*no_args{x               = into room\n\r", ch);
        send_to_char("        -{*<obj_name>{x            = into obj\n\r", ch);
        send_to_char("        -{*<mob_name> <wear_loc>{x = into mob\n\r", ch);
        return false;
    }

    if (!(obj_proto = get_object_prototype(atoi(vnum_str)))) {
        send_to_char("{jREdit: No object has that vnum.{x\n\r", ch);
        return false;
    }

    if (obj_proto->area != room_data->area_data) {
        send_to_char("{jREdit: No such object in this area.{x\n\r", ch);
        return false;
    }

    // Load into room.
    if (into_arg[0] == '\0') {
        reset = new_reset();
        reset->command = 'O';
        reset->arg1 = obj_proto->vnum;
        reset->arg2 = 0;
        reset->arg3 = room_data->vnum;
        reset->arg4 = 0;
        add_reset(room_data, reset, 0/* Last slot*/);

        Room* room;
        FOR_EACH_INSTANCE(room, room_data->instances) {
            newobj = create_object(obj_proto, (int16_t)number_fuzzy(olevel));
            obj_to_room(newobj, room);
        }

        sprintf(output, "{j%s (%d) has been loaded and added to resets.{x\n\r",
            capitalize(obj_proto->short_descr),
            obj_proto->vnum);
        send_to_char(output, ch);
    }
    else if (argument[0] == '\0' && ((to_obj = get_obj_list(ch, into_arg, room_data->instances->contents)) != NULL)) {
        // Load into object's inventory.
        reset = new_reset();
        reset->command = 'P';
        reset->arg1 = obj_proto->vnum;
        reset->arg2 = 0;
        reset->arg3 = to_obj->prototype->vnum;
        reset->arg4 = 1;
        add_reset(room_data, reset, 0/* Last slot*/);

        Room* room;
        FOR_EACH_INSTANCE(room, room_data->instances) {
            to_obj = get_obj_list(ch, into_arg, room->contents);
            newobj = create_object(obj_proto, (int16_t)number_fuzzy(olevel));
            newobj->cost = 0;
            obj_to_obj(newobj, to_obj);
        }

        if (newobj && to_obj) {
            sprintf(output, "{j%s (%d) has been loaded into "
                "%s (%d) and added to resets.{x\n\r",
                capitalize(newobj->short_descr),
                newobj->prototype->vnum,
                to_obj->short_descr,
                to_obj->prototype->vnum);
            send_to_char(output, ch);
        }
    }
    else if ((to_mob = get_mob_instance(room_data->instances, into_arg)) != NULL) {
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
                obj_proto->vnum,
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
        reset->arg1 = obj_proto->vnum;
        reset->arg2 = (int16_t)wearloc;
        if (reset->arg2 == WEAR_UNHELD)
            reset->command = 'G';
        else
            reset->command = 'E';
        reset->arg3 = wearloc;

        add_reset(room_data, reset, 0/* Last slot*/);

        Room* room;
        FOR_EACH_INSTANCE(room, room_data->instances) {
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
                obj_proto->vnum,
                flag_string(wear_loc_strings, reset->arg3),
                to_mob->short_descr,
                to_mob->prototype->vnum);
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
    int key, lastmob;

    for (key = 0; key < MAX_KEY_HASH; ++key)
        FOR_EACH(room, room_data_hash_table[key])
            if (room->area_data == area) {
                lastmob = -1;
                pLastMob = NULL;

                FOR_EACH(reset, room->reset_first) {
                    if (reset->command == 'M') {
                        lastmob = reset->arg1;
                        pLastMob = get_mob_prototype(lastmob);
                        if (pLastMob == NULL) {
                            bugf("Showresets : invalid reset (mob %d) in room %d", lastmob, room->vnum);
                            return;
                        }
                        if (mob && lastmob == mob->vnum) {
                            sprintf(buf2, "%-5d %-15.15s %-5d\n\r", lastmob, mob->name, room->vnum);
                            add_buf(buf, buf2);
                        }
                    }
                    if (obj && reset->command == 'O' && reset->arg1 == obj->vnum) {
                        sprintf(buf2, "%-5d %-15.15s %-5d\n\r", obj->vnum, obj->name, room->vnum);
                        add_buf(buf, buf2);
                    }
                    if (obj && (reset->command == 'G' || reset->command == 'E') && reset->arg1 == obj->vnum) {
                        sprintf(buf2, "%-5d %-15.15s %-5d %-5d %-15.15s\n\r", obj->vnum, obj->name, room->vnum, lastmob, pLastMob ? pLastMob->name : "");
                        add_buf(buf, buf2);
                    }
                }
            }
}

void listobjreset(Mobile* ch, Buffer* buf, AreaData* area)
{
    ObjPrototype* obj;
    int key;

    add_buf(buf, "{TVnum  Name            Room  On mob{x\n\r");

    for (key = 0; key < MAX_KEY_HASH; ++key)
        FOR_EACH(obj, obj_proto_hash[key])
            if (obj->area == area)
                showresets(ch, buf, area, 0, obj);
}

void listmobreset(Mobile* ch, Buffer* buf, AreaData* area)
{
    MobPrototype* mob;
    int key;

    add_buf(buf, "{TVnum  Name            Room {x\n\r");

    for (key = 0; key < MAX_KEY_HASH; ++key)
        FOR_EACH(mob, mob_proto_hash[key])
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
    int key;
    bool fAll = !str_cmp(argument, "all");
    RoomData* room;

    EDIT_ROOM(ch, room);

    for (key = 0; key < MAX_KEY_HASH; ++key)
        FOR_EACH(obj, obj_proto_hash[key])
            if (obj->reset_num == 0 && (fAll || obj->area == room->area_data))
                printf_to_char(ch, "Obj {*%-5.5d{x [%-20.20s] is not reset.\n\r", obj->vnum, obj->name);

    return false;
}

REDIT(redit_checkrooms)
{
    RoomData* room;
    RoomData* thisroom;
    int hash;
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

    for (hash = 0; hash < MAX_KEY_HASH; hash++)
        FOR_EACH(room, room_data_hash_table[hash])
            if (room->reset_num == 0 
                && (fAll || room->area_data == thisroom->area_data))
                printf_to_char(ch, "Room %d has no resets.\n\r", room->vnum);

    return false;
}

REDIT(redit_checkmob)
{
    MobPrototype* mob;
    RoomData* room;
    int key;
    bool fAll = !str_cmp(argument, "all");

    EDIT_ROOM(ch, room);

    for (key = 0; key < MAX_KEY_HASH; ++key)
        FOR_EACH(mob, mob_proto_hash[key])
            if (mob->reset_num == 0 && (fAll || mob->area == room->area_data))
                printf_to_char(ch, "Mob {*%-5.5d{x [%-20.20s] has no resets.\n\r", mob->vnum, mob->name);

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

    free_string(this->name);
    free_string(this->description);
    free_string(this->owner);

    this->name = str_dup(that->name);
    this->description = str_dup(that->description);
    this->owner = str_dup(that->owner);

    this->room_flags = that->room_flags;
    this->sector_type = that->sector_type;
    this->clan = that->clan;
    this->heal_rate = that->heal_rate;
    this->mana_rate = that->mana_rate;

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
    free_string(pRoom->name);
    free_string(pRoom->description);
    pRoom->owner = str_dup("");
    pRoom->name = str_dup("");
    pRoom->description = str_dup("");

    for (i = 0; i < DIR_MAX; i++) {
        free_room_exit_data(pRoom->exit_data[i]);
        pRoom->exit_data[i] = NULL;
    }

    send_to_char("Room cleared.\n\r", ch);
    return true;
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
        "{T No.  Loads    Description       Location         Vnum   Ar Rm Description"
        "\n\r"
        "{===== ======== ============= =================== ======== ===== ==========="
        "\n\r", ch);

    FOR_EACH(reset, pRoom->reset_first) {
        ObjPrototype* pObj;
        MobPrototype* p_mob_proto;
        ObjPrototype* obj_proto;
        ObjPrototype* pObjToIndex;
        RoomData* pRoomIndex;

        final[0] = '\0';
        sprintf(final, "{|[{*%2d{|]{x ", ++iReset);

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
            sprintf(buf, "M{|[{*%5d{|]{x %-13.13s                     R{|[{*%5d{|]{x %2d-%2d %-15.15s\n\r",
                reset->arg1, pMob->short_descr, reset->arg3,
                reset->arg2, reset->arg4, pRoomIndex->name);
            strcat(final, buf);

            // Check for pet shop.
            {
                RoomData* pRoomIndexPrev;

                pRoomIndexPrev = get_room_data(pRoomIndex->vnum - 1);
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

            sprintf(buf, "O{|[{*%5d{|]{x %-13.13s                     "
                "R{|[{*%5d{|]{x       %-15.15s\n\r",
                reset->arg1, pObj->short_descr,
                reset->arg3, pRoomIndex->name);
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
                "O{|[{*%5d{|]{x %-13.13s inside              O{|[{*%5d{|]{x %2d-%2d %-15.15s\n\r",
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
                    "O{|[{*%5d{|]{x %-13.13s in the inventory of S{|[{*%5d{|]{x       %-15.15s\n\r",
                    reset->arg1,
                    pObj->short_descr,
                    pMob->vnum,
                    pMob->short_descr);
            }
            else
                sprintf(buf,
                    "O{|[{*%5d{|]{x %-13.13s %-19.19s M{|[{*%5d{|]{x       %-15.15s\n\r",
                    reset->arg1,
                    pObj->short_descr,
                    (reset->command == 'G') ?
                    flag_string(wear_loc_strings, WEAR_UNHELD)
                    : flag_string(wear_loc_strings, reset->arg3),
                    pMob->vnum,
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
            sprintf(buf, "R{|[{*%5d{|]{x %s door of %-19.19s reset to %s\n\r",
                reset->arg1,
                capitalize(dir_list[reset->arg2].name),
                pRoomIndex->name,
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

            sprintf(buf, "R{|[{*%5d{|]{x Exits are randomized in %s\n\r",
                reset->arg1, pRoomIndex->name);
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
        "Syntax: {*RESET <number> OBJ <vnum> <wear_loc>\n\r"
        "        RESET <number> OBJ <vnum> inside <vnum> [limit] [count]\n\r"
        "        RESET <number> OBJ <vnum> room\n\r"
        "        RESET <number> MOB <vnum> [max #x area] [max #x room]\n\r"
        "        RESET <number> DELETE\n\r"
        "        RESET <number> RANDOM [#x exits]{x\n\r";

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
                    reset->arg3 = ch->in_room->vnum;
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
                            reset->arg3 = ch->in_room->vnum;
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
                reset->arg1 = ch->in_room->vnum;
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
            send_to_char("{jInvalid syntax.{x\n\r"
                "Your options are:\n\r"
                "{*reset add mob [vnum/name]\n\r"
                "reset add obj [vnum/name]\n\r"
                "reset add [name]{x\n\r", ch);
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
        reset->arg3 = ch->in_room->vnum;
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
    static const char* help = "{jSyntax: OBJLIST AREA\n\r"
        "          OBJLIST WORLD [low-level] [high-level]{x\n\r";
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
        send_to_char("{*Invalid security for editing this area.{x\n\r", ch);
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
    int type = -1;

    if (type_str[0]) {
        for (int i = 0; i < ITEM_TYPE_COUNT; ++i)
            if (!str_prefix(type_str, item_table[i].name)) {
                type = item_table[i].type;
                break;
            }
    }

    //               ################################################################################
    switch (type) {
    case ITEM_ARMOR:
        addf_buf(out, "{TVNUM   Lvl Name                AC:  Pierce  Bash    Slash   Exotic        {x\n\r");
        break;    
    case ITEM_CONTAINER:
        addf_buf(out, "{TVNUM   Lvl Name                     Wght    Flags   Key     Cpcty   WtMult{x\n\r");
        break;
    default:
        addf_buf(out, "{TVNUM   Lvl Name       Type          Val0    Val1    Val2    Val3    Val4  {x\n\r");
    }
    addf_buf(out, "{============================================================================{x\n\r");

    VNUM hi_vnum = ch->in_room->area->data->max_vnum;
    VNUM lo_vnum = ch->in_room->area->data->min_vnum;

    for (int h = 0; h < MAX_KEY_HASH; ++h) {
        ObjPrototype* obj;
        FOR_EACH(obj, obj_proto_hash[h]) {
            if (count > max_disp) {
                addf_buf(out, "Max display threshold reached.\n\r");
                goto max_disp_reached;
            }
            if (!world && (obj->vnum < lo_vnum || obj->vnum > hi_vnum))
                continue;
            if (world && (obj->level < lo_lvl || obj->level > hi_lvl))
                continue;
            if (type > 0 && obj->item_type != type)
                continue;

            addf_buf(out, "{*%-6d {*%-3d{x ", obj->vnum, obj->level);

            switch (type) {
            case ITEM_ARMOR:
            case ITEM_CONTAINER:
                addf_buf(out, "%-23.23s  {|", obj->short_descr);
                break;
            default:
                addf_buf(out, "%-10.10s %-12.12s {|", obj->short_descr, flag_string(type_flag_table, obj->item_type));
            }

            for (int i = 0; i < 5; ++i) {
                addf_buf(out, "[{*%5d{|] ", obj->value[i]);
            }

            addf_buf(out, "{x\n\r");
        }
    }
max_disp_reached:
    addf_buf(out, "\n\r");
    page_to_char(BUF(out), ch);
    free_buf(out);
}

void do_moblist(Mobile* ch, char* argument)
{
    static const char* help = "{jSyntax: MOBLIST AREA\n\r"
        "          MOBLIST WORLD [low-level] [high-level]{x\n\r";
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
        send_to_char("{*Invalid security for editing this area.{x\n\r", ch);
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
    addf_buf(out, "{TVNUM   Name       Lvl Hit Dice     Hit   Dam      Mana       Pie  Bas  Sla  Mag{x\n\r");
    addf_buf(out, "{================================================================================{x\n\r");

    VNUM hi_vnum = ch->in_room->area->data->max_vnum;
    VNUM lo_vnum = ch->in_room->area->data->min_vnum;

    for (int h = 0; h < MAX_KEY_HASH; ++h) {
        MobPrototype* mob;
        FOR_EACH(mob, mob_proto_hash[h])
        {
            if (count > max_disp) {
                addf_buf(out, "Max display threshold reached.\n\r");
                goto max_disp_reached;
            }
            if (!world && (mob->vnum < lo_vnum || mob->vnum > hi_vnum))
                continue;
            if (world && (mob->level < lo_lvl || mob->level > hi_lvl))
                continue;

            //VNUM   Name       Lvl Hit Dice   Hit   Dam Dice   Mana       Pie Bas Sla Mag
            //###### ########## ### ########## ##### ######## ########## ### ### ### ###
            addf_buf(out, "{*%-6d{x %-10.10s {*%-3d ",
                mob->vnum, mob->short_descr, mob->level);
            sprintf(buf, "%dd%d+%d", mob->hit[0], mob->hit[1], mob->hit[2]);
            addf_buf(out, "%-12.12s %-5d ", buf, mob->hitroll);
            sprintf(buf, "%dd%d+%d", mob->damage[0], mob->damage[1], mob->damage[2]);
            addf_buf(out, "%-8.8s ", buf);
            sprintf(buf, "%dd%d+%d", mob->mana[0], mob->mana[1], mob->mana[2]);
            addf_buf(out, "%-9.9s ", buf);
            addf_buf(out, "%4d %4d %4d %4d{x\n\r", mob->ac[0], mob->ac[1], mob->ac[2], mob->ac[3]);
        }
    }
max_disp_reached:
    addf_buf(out, "\n\r");
    page_to_char(BUF(out), ch);
    free_buf(out);
}
