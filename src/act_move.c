/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St�rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@hypercube.org)                                *
 *      Gabrielle Taylor (gtaylor@hypercube.org)                           *
 *      Brian Moore (zump@rom.org)                                         *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

#include "act_move.h"

#include "comm.h"
#include "config.h"
#include "db.h"
#include "fight.h"
#include "handler.h"
#include "interp.h"
#include "mob_prog.h"
#include "note.h"
#include "skill_ops.h"
#include "skills.h"
#include "stringbuffer.h"
#include "update.h"

#include <entities/descriptor.h>
#include <entities/event.h>
#include <entities/room_exit.h>
#include <entities/object.h>
#include <entities/player_data.h>
#include <entities/room.h>

#include <data/class.h>
#include <data/direction.h>
#include <data/events.h>
#include <data/mobile_data.h>
#include <data/race.h>
#include <data/skill.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#ifndef _MSC_VER 
#include <sys/time.h>
#endif

// Local functions.
int find_door(Mobile * ch, char* arg);
bool has_key(Mobile * ch, int key);

void move_char(Mobile* ch, int door, bool follow)
{
    Mobile* fch;
    Room* in_room;
    Room* to_room;
    RoomExit* room_exit;

    if (door < 0 || door > 5) {
        bug("Do_move: bad door %d.", door);
        return;
    }

    // Exit trigger, if activated, bail out. Only PCs are triggered.
    if (!IS_NPC(ch) && mp_exit_trigger(ch, door))
        return;

    // Exit events only block if they return TRUE.
    if (!IS_NPC(ch) && raise_exit_event(ch, door))
        return;

    in_room = ch->in_room;
    if ((room_exit = in_room->exit[door]) == NULL
        || (room_exit->data->to_room == NULL)
        || !can_see_room(ch, room_exit->data->to_room)) {
        send_to_char("Alas, you cannot go that way.\n\r", ch);
        return;
    }

    if ((to_room = room_exit->to_room) == NULL) {
        to_room = get_room_for_player(ch, room_exit->data->to_vnum);
    }
    
    if (!to_room) {
        bugf("Room %d exit %d to %d does not exist.", VNUM_FIELD(in_room->data),
            door, room_exit->data->to_vnum);
    }

    if (IS_SET(room_exit->exit_flags, EX_CLOSED)
        && (!IS_AFFECTED(ch, AFF_PASS_DOOR)
            || IS_SET(room_exit->exit_flags, EX_NOPASS))
        && !IS_TRUSTED(ch, ANGEL)) {
        act("The $d is closed.", ch, NULL, room_exit->data->keyword, TO_CHAR);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL
        && in_room == ch->master->in_room) {
        send_to_char("What?  And leave your beloved master?\n\r", ch);
        return;
    }

    if (!is_room_owner(ch, to_room) && room_is_private(to_room)) {
        send_to_char("That room is private right now.\n\r", ch);
        return;
    }

    if (!IS_NPC(ch)) {
        int iClass, iGuild;
        int move;

        for (iClass = 0; iClass < class_count; iClass++) {
            for (iGuild = 0; iGuild < MAX_GUILD; iGuild++) {
                if (iClass != ch->ch_class
                    && VNUM_FIELD(to_room) == class_table[iClass].guild[iGuild]) {
                    send_to_char("You aren't allowed in there.\n\r", ch);
                    return;
                }
            }
        }

        if (in_room->data->sector_type == SECT_AIR
            || to_room->data->sector_type == SECT_AIR) {
            if (!IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch)) {
                send_to_char("You can't fly.\n\r", ch);
                return;
            }
        }

        if ((in_room->data->sector_type == SECT_WATER_NOSWIM
             || to_room->data->sector_type == SECT_WATER_NOSWIM)
            && !IS_AFFECTED(ch, AFF_FLYING)) {
            Object* obj;
            bool found;

            // Look for a boat.
            found = false;

            if (IS_IMMORTAL(ch))
                found = true;

            FOR_EACH_MOB_OBJ(obj, ch) {
                if (obj->item_type == ITEM_BOAT) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                send_to_char("You need a boat to go there.\n\r", ch);
                return;
            }
        }

        move = movement_loss[UMIN(SECT_MAX - 1, in_room->data->sector_type)]
               + movement_loss[UMIN(SECT_MAX - 1, to_room->data->sector_type)];

        move /= 2; /* i.e. the average */

        /* conditional effects */
        if (IS_AFFECTED(ch, AFF_FLYING) || IS_AFFECTED(ch, AFF_HASTE))
            move /= 2;

        if (IS_AFFECTED(ch, AFF_SLOW)) move *= 2;
        if (get_worn_armor_type(ch) == ARMOR_HEAVY)
            move = (move * 3) / 2;

        if (ch->move < move) {
            send_to_char("You are too exhausted.\n\r", ch);
            return;
        }

        WAIT_STATE(ch, 1);
        ch->move -= (int16_t)move;
    }

    if (!IS_AFFECTED(ch, AFF_SNEAK) && ch->invis_level < LEVEL_HERO)
        act("$n leaves $T.", ch, NULL, dir_list[door].name, TO_ROOM);

    transfer_mob(ch, to_room);

    if (!IS_AFFECTED(ch, AFF_SNEAK) && ch->invis_level < LEVEL_HERO)
        act("$n has arrived.", ch, NULL, NULL, TO_ROOM);

    do_function(ch, &do_look, "auto");

    if (in_room == to_room) /* no circular follows */
        return;

    FOR_EACH_ROOM_MOB(fch, in_room) {
        if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM)
            && fch->position < POS_STANDING)
            do_function(fch, &do_stand, "");

        if (fch->master == ch && fch->position == POS_STANDING
            && can_see_room(fch, to_room->data)) {
            if (IS_SET(ch->in_room->data->room_flags, ROOM_LAW)
                && (IS_NPC(fch) && IS_SET(fch->act_flags, ACT_AGGRESSIVE))) {
                act("You can't bring $N into the city.", ch, NULL, fch,
                    TO_CHAR);
                act("You aren't allowed in the city.", fch, NULL, NULL,
                    TO_CHAR);
                continue;
            }

            act("You follow $N.", fch, NULL, ch, TO_CHAR);
            move_char(fch, door, true);
        }
    }
    
    // If someone is following the char, these triggers get activated for the
    // followers before the char, but it's safer this way...
    if (IS_NPC(ch) && HAS_MPROG_TRIGGER(ch, TRIG_ENTRY)) {
        mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_ENTRY);
    }

    if (IS_NPC(ch) && HAS_EVENT_TRIGGER(ch, TRIG_ENTRY))
        raise_entry_event(ch, number_percent());

    if (!IS_NPC(ch)) {
        mp_greet_trigger(ch);
        raise_greet_event(ch);
    }

    return;
}

void do_north(Mobile* ch, char* argument)
{
    move_char(ch, DIR_NORTH, false);
    return;
}

void do_east(Mobile* ch, char* argument)
{
    move_char(ch, DIR_EAST, false);
    return;
}

void do_south(Mobile* ch, char* argument)
{
    move_char(ch, DIR_SOUTH, false);
    return;
}

void do_west(Mobile* ch, char* argument)
{
    move_char(ch, DIR_WEST, false);
    return;
}

void do_up(Mobile* ch, char* argument)
{
    move_char(ch, DIR_UP, false);
    return;
}

void do_down(Mobile* ch, char* argument)
{
    move_char(ch, DIR_DOWN, false);
    return;
}

int find_door(Mobile* ch, char* arg)
{
    RoomExit* room_exit;
    int door;

    if (!str_cmp(arg, "n") || !str_cmp(arg, "north"))
        door = 0;
    else if (!str_cmp(arg, "e") || !str_cmp(arg, "east"))
        door = 1;
    else if (!str_cmp(arg, "s") || !str_cmp(arg, "south"))
        door = 2;
    else if (!str_cmp(arg, "w") || !str_cmp(arg, "west"))
        door = 3;
    else if (!str_cmp(arg, "u") || !str_cmp(arg, "up"))
        door = 4;
    else if (!str_cmp(arg, "d") || !str_cmp(arg, "down"))
        door = 5;
    else {
        for (door = 0; door <= 5; door++) {
            if ((room_exit = ch->in_room->exit[door]) != NULL
                && IS_SET(room_exit->exit_flags, EX_ISDOOR) && room_exit->data->keyword != NULL
                && is_name(arg, room_exit->data->keyword))
                return door;
        }
        act("I see no $T here.", ch, NULL, arg, TO_CHAR);
        return -1;
    }

    if ((room_exit = ch->in_room->exit[door]) == NULL) {
        act("I see no door $T here.", ch, NULL, arg, TO_CHAR);
        return -1;
    }

    if (!IS_SET(room_exit->exit_flags, EX_ISDOOR)) {
        send_to_char("You can't do that.\n\r", ch);
        return -1;
    }

    return door;
}

void do_open(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Open what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != NULL) {
        /* open portal */
        if (obj->item_type == ITEM_PORTAL) {
            if (!IS_SET(obj->portal.exit_flags, EX_ISDOOR)) {
                send_to_char("You can't do that.\n\r", ch);
                return;
            }

            if (!IS_SET(obj->portal.exit_flags, EX_CLOSED)) {
                send_to_char("It's already open.\n\r", ch);
                return;
            }

            if (IS_SET(obj->portal.exit_flags, EX_LOCKED)) {
                send_to_char("It's locked.\n\r", ch);
                return;
            }

            REMOVE_BIT(obj->portal.exit_flags, EX_CLOSED);
            act("You open $p.", ch, obj, NULL, TO_CHAR);
            act("$n opens $p.", ch, obj, NULL, TO_ROOM);
            return;
        }

        /* 'open object' */
        if (obj->item_type != ITEM_CONTAINER) {
            send_to_char("That's not a container.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->container.flags, CONT_CLOSED)) {
            send_to_char("It's already open.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->container.flags, CONT_CLOSEABLE)) {
            send_to_char("You can't do that.\n\r", ch);
            return;
        }
        if (IS_SET(obj->container.flags, CONT_LOCKED)) {
            send_to_char("It's locked.\n\r", ch);
            return;
        }

        REMOVE_BIT(obj->container.flags, CONT_CLOSED);
        act("You open $p.", ch, obj, NULL, TO_CHAR);
        act("$n opens $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg)) >= 0) {
        /* 'open door' */
        Room* to_room;
        RoomExit* room_exit;
        RoomExit* room_exit_rev;

        room_exit = ch->in_room->exit[door];
        if (!IS_SET(room_exit->exit_flags, EX_CLOSED)) {
            send_to_char("It's already open.\n\r", ch);
            return;
        }
        if (IS_SET(room_exit->exit_flags, EX_LOCKED)) {
            send_to_char("It's locked.\n\r", ch);
            return;
        }

        REMOVE_BIT(room_exit->exit_flags, EX_CLOSED);
        act("$n opens the $d.", ch, NULL, room_exit->data->keyword, TO_ROOM);
        send_to_char("Ok.\n\r", ch);

        /* open the other side */
        if ((to_room = room_exit->to_room) != NULL
            && (room_exit_rev = to_room->exit[dir_list[door].rev_dir]) != NULL
            && room_exit_rev->to_room == ch->in_room) {
            Mobile* rch;

            REMOVE_BIT(room_exit_rev->exit_flags, EX_CLOSED);
            FOR_EACH_ROOM_MOB(rch, to_room)
                act("The $d opens.", rch, NULL, room_exit_rev->data->keyword, TO_CHAR);
        }
    }

    return;
}

void do_close(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Close what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != NULL) {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL) {
            if (!IS_SET(obj->portal.exit_flags, EX_ISDOOR)
                || IS_SET(obj->portal.exit_flags, EX_NOCLOSE)) {
                send_to_char("You can't do that.\n\r", ch);
                return;
            }

            if (IS_SET(obj->portal.exit_flags, EX_CLOSED)) {
                send_to_char("It's already closed.\n\r", ch);
                return;
            }

            SET_BIT(obj->portal.exit_flags, EX_CLOSED);
            act("You close $p.", ch, obj, NULL, TO_CHAR);
            act("$n closes $p.", ch, obj, NULL, TO_ROOM);
            return;
        }

        /* 'close object' */
        if (obj->item_type != ITEM_CONTAINER) {
            send_to_char("That's not a container.\n\r", ch);
            return;
        }
        if (IS_SET(obj->container.flags, CONT_CLOSED)) {
            send_to_char("It's already closed.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->container.flags, CONT_CLOSEABLE)) {
            send_to_char("You can't do that.\n\r", ch);
            return;
        }

        SET_BIT(obj->container.flags, CONT_CLOSED);
        act("You close $p.", ch, obj, NULL, TO_CHAR);
        act("$n closes $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg)) >= 0) {
        /* 'close door' */
        Room* to_room;
        RoomExit* room_exit;
        RoomExit* room_exit_rev;

        room_exit = ch->in_room->exit[door];
        if (IS_SET(room_exit->exit_flags, EX_CLOSED)) {
            send_to_char("It's already closed.\n\r", ch);
            return;
        }

        SET_BIT(room_exit->exit_flags, EX_CLOSED);
        act("$n closes the $d.", ch, NULL, room_exit->data->keyword, TO_ROOM);
        send_to_char("Ok.\n\r", ch);

        /* close the other side */
        if ((to_room = room_exit->to_room) != NULL
            && (room_exit_rev = to_room->exit[dir_list[door].rev_dir]) != 0
            && room_exit_rev->to_room == ch->in_room) {
            Mobile* rch;

            SET_BIT(room_exit_rev->exit_flags, EX_CLOSED);
            FOR_EACH_ROOM_MOB(rch, to_room)
                act("The $d closes.", rch, NULL, room_exit_rev->data->keyword, TO_CHAR);
        }
    }

    return;
}

bool has_key(Mobile* ch, int key)
{
    Object* obj;

    FOR_EACH_MOB_OBJ(obj, ch) {
        if (VNUM_FIELD(obj->prototype) == key) 
            return true;
    }

    return false;
}

void do_lock(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Lock what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != NULL) {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL) {
            if (!IS_SET(obj->portal.exit_flags, EX_ISDOOR)
                || IS_SET(obj->portal.exit_flags, EX_NOCLOSE)) {
                send_to_char("You can't do that.\n\r", ch);
                return;
            }
            if (!IS_SET(obj->portal.exit_flags, EX_CLOSED)) {
                send_to_char("It's not closed.\n\r", ch);
                return;
            }

            if (obj->portal.key_vnum < 0 || IS_SET(obj->portal.exit_flags, EX_NOLOCK)) {
                send_to_char("It can't be locked.\n\r", ch);
                return;
            }

            if (!has_key(ch, obj->portal.key_vnum)) {
                send_to_char("You lack the key.\n\r", ch);
                return;
            }

            if (IS_SET(obj->portal.exit_flags, EX_LOCKED)) {
                send_to_char("It's already locked.\n\r", ch);
                return;
            }

            SET_BIT(obj->portal.exit_flags, EX_LOCKED);
            act("You lock $p.", ch, obj, NULL, TO_CHAR);
            act("$n locks $p.", ch, obj, NULL, TO_ROOM);
            return;
        }

        /* 'lock object' */
        if (obj->item_type != ITEM_CONTAINER) {
            send_to_char("That's not a container.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->container.flags, CONT_CLOSED)) {
            send_to_char("It's not closed.\n\r", ch);
            return;
        }
        if (obj->container.key_vnum < 0) {
            send_to_char("It can't be locked.\n\r", ch);
            return;
        }
        if (!has_key(ch, obj->container.key_vnum)) {
            send_to_char("You lack the key.\n\r", ch);
            return;
        }
        if (IS_SET(obj->container.flags, CONT_LOCKED)) {
            send_to_char("It's already locked.\n\r", ch);
            return;
        }

        SET_BIT(obj->container.flags, CONT_LOCKED);
        act("You lock $p.", ch, obj, NULL, TO_CHAR);
        act("$n locks $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg)) >= 0) {
        /* 'lock door' */
        Room* to_room;
        RoomExit* room_exit;
        RoomExit* room_exit_rev;

        room_exit = ch->in_room->exit[door];
        if (!IS_SET(room_exit->exit_flags, EX_CLOSED)) {
            send_to_char("It's not closed.\n\r", ch);
            return;
        }
        if (room_exit->data->key < 0) {
            send_to_char("It can't be locked.\n\r", ch);
            return;
        }
        if (!has_key(ch, room_exit->data->key)) {
            send_to_char("You lack the key.\n\r", ch);
            return;
        }
        if (IS_SET(room_exit->exit_flags, EX_LOCKED)) {
            send_to_char("It's already locked.\n\r", ch);
            return;
        }

        SET_BIT(room_exit->exit_flags, EX_LOCKED);
        send_to_char("*Click*\n\r", ch);
        act("$n locks the $d.", ch, NULL, room_exit->data->keyword, TO_ROOM);

        /* lock the other side */
        if ((to_room = room_exit->to_room) != NULL
            && (room_exit_rev = to_room->exit[dir_list[door].rev_dir]) != 0
            && room_exit_rev->to_room == ch->in_room) {
            SET_BIT(room_exit_rev->exit_flags, EX_LOCKED);
        }
    }

    return;
}

void do_unlock(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Unlock what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != NULL) {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL) {
            if (!IS_SET(obj->portal.exit_flags, EX_ISDOOR)) {
                send_to_char("You can't do that.\n\r", ch);
                return;
            }

            if (!IS_SET(obj->portal.exit_flags, EX_CLOSED)) {
                send_to_char("It's not closed.\n\r", ch);
                return;
            }

            if (obj->portal.key_vnum < 0) {
                send_to_char("It can't be unlocked.\n\r", ch);
                return;
            }

            if (!has_key(ch, obj->portal.key_vnum)) {
                send_to_char("You lack the key.\n\r", ch);
                return;
            }

            if (!IS_SET(obj->portal.exit_flags, EX_LOCKED)) {
                send_to_char("It's already unlocked.\n\r", ch);
                return;
            }

            REMOVE_BIT(obj->portal.exit_flags, EX_LOCKED);
            act("You unlock $p.", ch, obj, NULL, TO_CHAR);
            act("$n unlocks $p.", ch, obj, NULL, TO_ROOM);
            return;
        }

        /* 'unlock object' */
        if (obj->item_type != ITEM_CONTAINER) {
            send_to_char("That's not a container.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->container.flags, CONT_CLOSED)) {
            send_to_char("It's not closed.\n\r", ch);
            return;
        }
        if (obj->container.key_vnum < 0) {
            send_to_char("It can't be unlocked.\n\r", ch);
            return;
        }
        if (!has_key(ch, obj->container.key_vnum)) {
            send_to_char("You lack the key.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->container.flags, CONT_LOCKED)) {
            send_to_char("It's already unlocked.\n\r", ch);
            return;
        }

        REMOVE_BIT(obj->container.flags, CONT_LOCKED);
        act("You unlock $p.", ch, obj, NULL, TO_CHAR);
        act("$n unlocks $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg)) >= 0) {
        /* 'unlock door' */
        Room* to_room;
        RoomExit* room_exit;
        RoomExit* room_exit_rev;

        room_exit = ch->in_room->exit[door];
        if (!IS_SET(room_exit->exit_flags, EX_CLOSED)) {
            send_to_char("It's not closed.\n\r", ch);
            return;
        }
        if (room_exit->data->key < 0) {
            send_to_char("It can't be unlocked.\n\r", ch);
            return;
        }
        if (!has_key(ch, room_exit->data->key)) {
            send_to_char("You lack the key.\n\r", ch);
            return;
        }
        if (!IS_SET(room_exit->exit_flags, EX_LOCKED)) {
            send_to_char("It's already unlocked.\n\r", ch);
            return;
        }

        REMOVE_BIT(room_exit->exit_flags, EX_LOCKED);
        send_to_char("*Click*\n\r", ch);
        act("$n unlocks the $d.", ch, NULL, room_exit->data->keyword, TO_ROOM);

        /* unlock the other side */
        if ((to_room = room_exit->to_room) != NULL
            && (room_exit_rev = to_room->exit[dir_list[door].rev_dir]) != NULL
            && room_exit_rev->to_room == ch->in_room) {
            REMOVE_BIT(room_exit_rev->exit_flags, EX_LOCKED);
        }
    }

    return;
}

void do_pick(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* gch;
    Object* obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Pick what?\n\r", ch);
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_pick_lock].beats);

    /* look for guards */
    FOR_EACH_ROOM_MOB(gch, ch->in_room) {
        if (IS_NPC(gch) && IS_AWAKE(gch) && ch->level + 5 < gch->level) {
            act("$N is standing too close to the lock.", ch, NULL, gch, TO_CHAR);
            return;
        }
    }

    /* Use skill check seam for testability */
    if (!IS_NPC(ch) && !skill_ops->check_simple(ch, gsn_pick_lock)) {
        send_to_char("You failed.\n\r", ch);
        check_improve(ch, gsn_pick_lock, false, 2);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != NULL) {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL) {
            if (!IS_SET(obj->portal.exit_flags, EX_ISDOOR)) {
                send_to_char("You can't do that.\n\r", ch);
                return;
            }

            if (!IS_SET(obj->portal.exit_flags, EX_CLOSED)) {
                send_to_char("It's not closed.\n\r", ch);
                return;
            }

            if (obj->portal.key_vnum < 0) {
                send_to_char("It can't be unlocked.\n\r", ch);
                return;
            }

            if (IS_SET(obj->portal.exit_flags, EX_PICKPROOF)) {
                send_to_char("You failed.\n\r", ch);
                return;
            }

            REMOVE_BIT(obj->portal.exit_flags, EX_LOCKED);
            act("You pick the lock on $p.", ch, obj, NULL, TO_CHAR);
            act("$n picks the lock on $p.", ch, obj, NULL, TO_ROOM);
            check_improve(ch, gsn_pick_lock, true, 2);
            return;
        }

        /* 'pick object' */
        if (obj->item_type != ITEM_CONTAINER) {
            send_to_char("That's not a container.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->container.flags, CONT_CLOSED)) {
            send_to_char("It's not closed.\n\r", ch);
            return;
        }
        if (obj->container.key_vnum < 0) {
            send_to_char("It can't be unlocked.\n\r", ch);
            return;
        }
        if (!IS_SET(obj->container.flags, CONT_LOCKED)) {
            send_to_char("It's already unlocked.\n\r", ch);
            return;
        }
        if (IS_SET(obj->container.flags, CONT_PICKPROOF)) {
            send_to_char("You failed.\n\r", ch);
            return;
        }

        REMOVE_BIT(obj->container.flags, CONT_LOCKED);
        act("You pick the lock on $p.", ch, obj, NULL, TO_CHAR);
        act("$n picks the lock on $p.", ch, obj, NULL, TO_ROOM);
        check_improve(ch, gsn_pick_lock, true, 2);
        return;
    }

    if ((door = find_door(ch, arg)) >= 0) {
        /* 'pick door' */
        Room* to_room;
        RoomExit* room_exit;
        RoomExit* room_exit_rev;

        room_exit = ch->in_room->exit[door];
        if (!IS_SET(room_exit->exit_flags, EX_CLOSED) && !IS_IMMORTAL(ch)) {
            send_to_char("It's not closed.\n\r", ch);
            return;
        }
        if (room_exit->data->key < 0 && !IS_IMMORTAL(ch)) {
            send_to_char("It can't be picked.\n\r", ch);
            return;
        }
        if (!IS_SET(room_exit->exit_flags, EX_LOCKED)) {
            send_to_char("It's already unlocked.\n\r", ch);
            return;
        }
        if (IS_SET(room_exit->exit_flags, EX_PICKPROOF) && !IS_IMMORTAL(ch)) {
            send_to_char("You failed.\n\r", ch);
            return;
        }

        REMOVE_BIT(room_exit->exit_flags, EX_LOCKED);
        send_to_char("*Click*\n\r", ch);
        act("$n picks the $d.", ch, NULL, room_exit->data->keyword, TO_ROOM);
        check_improve(ch, gsn_pick_lock, true, 2);

        /* pick the other side */
        if ((to_room = room_exit->to_room) != NULL
            && (room_exit_rev = to_room->exit[dir_list[door].rev_dir]) != NULL
            && room_exit_rev->to_room == ch->in_room) {
            REMOVE_BIT(room_exit_rev->exit_flags, EX_LOCKED);
        }
    }

    return;
}

void do_stand(Mobile* ch, char* argument)
{
    Object* obj = NULL;

    if (argument[0] != '\0') {
        if (ch->position == POS_FIGHTING) {
            send_to_char("Maybe you should finish fighting first?\n\r", ch);
            return;
        }
        obj = get_obj_list(ch, argument, &ch->in_room->objects);
        if (obj == NULL) {
            send_to_char("You don't see that here.\n\r", ch);
            return;
        }
        if (obj->item_type != ITEM_FURNITURE
            || (!IS_SET(obj->furniture.flags, STAND_AT)
                && !IS_SET(obj->furniture.flags, STAND_ON)
                && !IS_SET(obj->furniture.flags, STAND_IN))) {
            send_to_char("You can't seem to find a place to stand.\n\r", ch);
            return;
        }
        if (ch->on != obj && count_users(obj) >= obj->furniture.max_people) {
            act_pos("There's no room to stand on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
            return;
        }
        ch->on = obj;
    }

    switch (ch->position) {
    case POS_SLEEPING:
        if (IS_AFFECTED(ch, AFF_SLEEP)) {
            send_to_char("You can't wake up!\n\r", ch);
            return;
        }

        if (obj == NULL) {
            send_to_char("You wake and stand up.\n\r", ch);
            act("$n wakes and stands up.", ch, NULL, NULL, TO_ROOM);
            ch->on = NULL;
        }
        else if (IS_SET(obj->furniture.flags, STAND_AT)) {
            act_pos("You wake and stand at $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
            act("$n wakes and stands at $p.", ch, obj, NULL, TO_ROOM);
        }
        else if (IS_SET(obj->furniture.flags, STAND_ON)) {
            act_pos("You wake and stand on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
            act("$n wakes and stands on $p.", ch, obj, NULL, TO_ROOM);
        }
        else {
            act_pos("You wake and stand in $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
            act("$n wakes and stands in $p.", ch, obj, NULL, TO_ROOM);
        }
        ch->position = POS_STANDING;
        do_function(ch, &do_look, "auto");
        break;

    case POS_RESTING:
    case POS_SITTING:
        if (obj == NULL) {
            send_to_char("You stand up.\n\r", ch);
            act("$n stands up.", ch, NULL, NULL, TO_ROOM);
            ch->on = NULL;
        }
        else if (IS_SET(obj->furniture.flags, STAND_AT)) {
            act("You stand at $p.", ch, obj, NULL, TO_CHAR);
            act("$n stands at $p.", ch, obj, NULL, TO_ROOM);
        }
        else if (IS_SET(obj->furniture.flags, STAND_ON)) {
            act("You stand on $p.", ch, obj, NULL, TO_CHAR);
            act("$n stands on $p.", ch, obj, NULL, TO_ROOM);
        }
        else {
            act("You stand in $p.", ch, obj, NULL, TO_CHAR);
            act("$n stands on $p.", ch, obj, NULL, TO_ROOM);
        }
        ch->position = POS_STANDING;
        break;

    case POS_STANDING:
        send_to_char("You are already standing.\n\r", ch);
        break;

    case POS_FIGHTING:
        send_to_char("You are already fighting!\n\r", ch);
        break;
    
    case POS_STUNNED:
        send_to_char("You are stunned.\n\r", ch);
        break;

    case POS_INCAP:
        send_to_char("You are incapacitated.\n\r", ch);
        break;

    case POS_DEAD:
        send_to_char("You are dead.\n\r", ch);
        break;

    case POS_MORTAL:
        send_to_char("You are mortally wounded, and cannot stand on your own.\n\r", ch);
        break;

    //case POS_UNKNOWN:
    //    break;
    }

    return;
}

void do_rest(Mobile* ch, char* argument)
{
    Object* obj = NULL;

    if (ch->position == POS_FIGHTING) {
        send_to_char("You are already fighting!\n\r", ch);
        return;
    }

    /* okay, now that we know we can rest, find an object to rest on */
    if (argument[0] != '\0') {
        obj = get_obj_list(ch, argument, &ch->in_room->objects);
        if (obj == NULL) {
            send_to_char("You don't see that here.\n\r", ch);
            return;
        }
    }
    else
        obj = ch->on;

    if (obj != NULL) {
        if (obj->item_type != ITEM_FURNITURE
            || (!IS_SET(obj->furniture.flags, REST_ON)
                && !IS_SET(obj->furniture.flags, REST_IN)
                && !IS_SET(obj->furniture.flags, REST_AT))) {
            send_to_char("You can't rest on that.\n\r", ch);
            return;
        }

        if (obj != NULL && ch->on != obj && count_users(obj) >= obj->furniture.max_people) {
            act_pos("There's no more room on $p.", ch, obj, NULL, TO_CHAR,
                    POS_DEAD);
            return;
        }

        ch->on = obj;
    }

    switch (ch->position) {
    case POS_SLEEPING:
        if (IS_AFFECTED(ch, AFF_SLEEP)) {
            send_to_char("You can't wake up!\n\r", ch);
            return;
        }

        if (obj == NULL) {
            send_to_char("You wake up and start resting.\n\r", ch);
            act("$n wakes up and starts resting.", ch, NULL, NULL, TO_ROOM);
        }
        else if (IS_SET(obj->furniture.flags, REST_AT)) {
            act_pos("You wake up and rest at $p.", ch, obj, NULL, TO_CHAR,
                    POS_SLEEPING);
            act("$n wakes up and rests at $p.", ch, obj, NULL, TO_ROOM);
        }
        else if (IS_SET(obj->furniture.flags, REST_ON)) {
            act_pos("You wake up and rest on $p.", ch, obj, NULL, TO_CHAR,
                    POS_SLEEPING);
            act("$n wakes up and rests on $p.", ch, obj, NULL, TO_ROOM);
        }
        else {
            act_pos("You wake up and rest in $p.", ch, obj, NULL, TO_CHAR,
                    POS_SLEEPING);
            act("$n wakes up and rests in $p.", ch, obj, NULL, TO_ROOM);
        }
        ch->position = POS_RESTING;
        break;

    case POS_RESTING:
        send_to_char("You are already resting.\n\r", ch);
        break;

    case POS_STANDING:
        if (obj == NULL) {
            send_to_char("You rest.\n\r", ch);
            act("$n sits down and rests.", ch, NULL, NULL, TO_ROOM);
        }
        else if (IS_SET(obj->furniture.flags, REST_AT)) {
            act("You sit down at $p and rest.", ch, obj, NULL, TO_CHAR);
            act("$n sits down at $p and rests.", ch, obj, NULL, TO_ROOM);
        }
        else if (IS_SET(obj->furniture.flags, REST_ON)) {
            act("You sit on $p and rest.", ch, obj, NULL, TO_CHAR);
            act("$n sits on $p and rests.", ch, obj, NULL, TO_ROOM);
        }
        else {
            act("You rest in $p.", ch, obj, NULL, TO_CHAR);
            act("$n rests in $p.", ch, obj, NULL, TO_ROOM);
        }
        ch->position = POS_RESTING;
        break;

    case POS_SITTING:
        if (obj == NULL) {
            send_to_char("You rest.\n\r", ch);
            act("$n rests.", ch, NULL, NULL, TO_ROOM);
        }
        else if (IS_SET(obj->furniture.flags, REST_AT)) {
            act("You rest at $p.", ch, obj, NULL, TO_CHAR);
            act("$n rests at $p.", ch, obj, NULL, TO_ROOM);
        }
        else if (IS_SET(obj->furniture.flags, REST_ON)) {
            act("You rest on $p.", ch, obj, NULL, TO_CHAR);
            act("$n rests on $p.", ch, obj, NULL, TO_ROOM);
        }
        else {
            act("You rest in $p.", ch, obj, NULL, TO_CHAR);
            act("$n rests in $p.", ch, obj, NULL, TO_ROOM);
        }
        ch->position = POS_RESTING;
        break;

    case POS_STUNNED:
        send_to_char("You are stunned.\n\r", ch);
        break;

    case POS_INCAP:
        send_to_char("You are incapacitated.\n\r", ch);
        break;

    case POS_DEAD:
        send_to_char("You are dead.\n\r", ch);
        break;

    case POS_FIGHTING:
        send_to_char("You can rest when you're dead.\n\r", ch);
        break;

    case POS_MORTAL:
        send_to_char("You are mortally wounded. Soon you will rest forever.\n\r", ch);
        break;

    //case POS_UNKNOWN:
    //    break;
    }

    return;
}

void do_sit(Mobile* ch, char* argument)
{
    Object* obj = NULL;

    if (ch->position == POS_FIGHTING) {
        send_to_char("Maybe you should finish this fight first?\n\r", ch);
        return;
    }

    /* okay, now that we know we can sit, find an object to sit on */
    if (argument[0] != '\0') {
        obj = get_obj_list(ch, argument, &ch->in_room->objects);
        if (obj == NULL) {
            send_to_char("You don't see that here.\n\r", ch);
            return;
        }
    }
    else
        obj = ch->on;

    if (obj != NULL) {
        if (obj->item_type != ITEM_FURNITURE
            || (!IS_SET(obj->furniture.flags, SIT_ON) && !IS_SET(obj->furniture.flags, SIT_IN)
                && !IS_SET(obj->furniture.flags, SIT_AT))) {
            send_to_char("You can't sit on that.\n\r", ch);
            return;
        }

        if (obj != NULL && ch->on != obj && count_users(obj) >= obj->furniture.max_people) {
            act_pos("There's no more room on $p.", ch, obj, NULL, TO_CHAR,
                    POS_DEAD);
            return;
        }

        ch->on = obj;
    }
    switch (ch->position) {
    case POS_SLEEPING:
        if (IS_AFFECTED(ch, AFF_SLEEP)) {
            send_to_char("You can't wake up!\n\r", ch);
            return;
        }

        if (obj == NULL) {
            send_to_char("You wake and sit up.\n\r", ch);
            act("$n wakes and sits up.", ch, NULL, NULL, TO_ROOM);
        }
        else if (IS_SET(obj->furniture.flags, SIT_AT)) {
            act_pos("You wake and sit at $p.", ch, obj, NULL, TO_CHAR,
                    POS_DEAD);
            act("$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM);
        }
        else if (IS_SET(obj->furniture.flags, SIT_ON)) {
            act_pos("You wake and sit on $p.", ch, obj, NULL, TO_CHAR,
                    POS_DEAD);
            act("$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM);
        }
        else {
            act_pos("You wake and sit in $p.", ch, obj, NULL, TO_CHAR,
                    POS_DEAD);
            act("$n wakes and sits in $p.", ch, obj, NULL, TO_ROOM);
        }

        ch->position = POS_SITTING;
        break;
    case POS_RESTING:
        if (obj == NULL)
            send_to_char("You stop resting.\n\r", ch);
        else if (IS_SET(obj->furniture.flags, SIT_AT)) {
            act("You sit at $p.", ch, obj, NULL, TO_CHAR);
            act("$n sits at $p.", ch, obj, NULL, TO_ROOM);
        }

        else if (IS_SET(obj->furniture.flags, SIT_ON)) {
            act("You sit on $p.", ch, obj, NULL, TO_CHAR);
            act("$n sits on $p.", ch, obj, NULL, TO_ROOM);
        }
        ch->position = POS_SITTING;
        break;
    case POS_SITTING:
        send_to_char("You are already sitting down.\n\r", ch);
        break;
    case POS_MORTAL:
        send_to_char("You can do little but bleed out on the ground.\n\r", ch);
        break;
    case POS_DEAD:
        send_to_char("You are dead.\n\r", ch);
        break;
    case POS_INCAP:
        send_to_char("You are incapacitated.\n\r", ch);
        break;
    case POS_STANDING:
        if (obj == NULL) {
            send_to_char("You sit down.\n\r", ch);
            act("$n sits down on the ground.", ch, NULL, NULL, TO_ROOM);
        }
        else if (IS_SET(obj->furniture.flags, SIT_AT)) {
            act("You sit down at $p.", ch, obj, NULL, TO_CHAR);
            act("$n sits down at $p.", ch, obj, NULL, TO_ROOM);
        }
        else if (IS_SET(obj->furniture.flags, SIT_ON)) {
            act("You sit on $p.", ch, obj, NULL, TO_CHAR);
            act("$n sits on $p.", ch, obj, NULL, TO_ROOM);
        }
        else {
            act("You sit down in $p.", ch, obj, NULL, TO_CHAR);
            act("$n sits down in $p.", ch, obj, NULL, TO_ROOM);
        }
        ch->position = POS_SITTING;
        break;
    case POS_STUNNED:
        send_to_char("You are stunned.\n\r", ch);
        break;
    case POS_FIGHTING:
        send_to_char("Finish your fight, then sit down.\n\r", ch);
        break;
    //case POS_UNKNOWN:
    //    break;
    }
    return;
}

void do_sleep(Mobile* ch, char* argument)
{
    Object* obj = NULL;

    switch (ch->position) {
    case POS_SLEEPING:
        send_to_char("You are already sleeping.\n\r", ch);
        break;

    case POS_RESTING:
    case POS_SITTING:
    case POS_STANDING:
        if (argument[0] == '\0' && ch->on == NULL) {
            send_to_char("You go to sleep.\n\r", ch);
            act("$n goes to sleep.", ch, NULL, NULL, TO_ROOM);
            ch->position = POS_SLEEPING;
        }
        else /* find an object and sleep on it */
        {
            if (argument[0] == '\0')
                obj = ch->on;
            else
                obj = get_obj_list(ch, argument, &ch->in_room->objects);

            if (obj == NULL) {
                send_to_char("You don't see that here.\n\r", ch);
                return;
            }
            if (obj->item_type != ITEM_FURNITURE
                || (!IS_SET(obj->furniture.flags, SLEEP_ON)
                    && !IS_SET(obj->furniture.flags, SLEEP_IN)
                    && !IS_SET(obj->furniture.flags, SLEEP_AT))) {
                send_to_char("You can't sleep on that!\n\r", ch);
                return;
            }

            if (ch->on != obj && count_users(obj) >= obj->furniture.max_people) {
                act_pos("There is no room on $p for you.", ch, obj, NULL,
                        TO_CHAR, POS_DEAD);
                return;
            }

            ch->on = obj;
            if (IS_SET(obj->furniture.flags, SLEEP_AT)) {
                act("You go to sleep at $p.", ch, obj, NULL, TO_CHAR);
                act("$n goes to sleep at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET(obj->furniture.flags, SLEEP_ON)) {
                act("You go to sleep on $p.", ch, obj, NULL, TO_CHAR);
                act("$n goes to sleep on $p.", ch, obj, NULL, TO_ROOM);
            }
            else {
                act("You go to sleep in $p.", ch, obj, NULL, TO_CHAR);
                act("$n goes to sleep in $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_SLEEPING;
        }
        break;

    case POS_STUNNED:
        send_to_char("You are stunned.\n\r", ch);
        break;

    case POS_INCAP:
        send_to_char("You are incapacitated.\n\r", ch);
        break;

    case POS_FIGHTING:
        send_to_char("You can rest when you're dead.\n\r", ch);
        break;

    case POS_MORTAL:
        send_to_char("You can do little but bleed out on the ground.\n\r", ch);
        break;

    case POS_DEAD:
        send_to_char("You are already sleeping. Permanently.\n\r", ch);
        break;

    //case POS_UNKNOWN:
    //    break;
    }

    return;
}

void do_wake(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        do_function(ch, &do_stand, "");
        return;
    }

    if (!IS_AWAKE(ch)) {
        send_to_char("You are asleep yourself!\n\r", ch);
        return;
    }

    if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_AWAKE(victim)) {
        act("$N is already awake.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (IS_AFFECTED(victim, AFF_SLEEP)) {
        act("You can't wake $M!", ch, NULL, victim, TO_CHAR);
        return;
    }

    act_pos("$n wakes you.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
    do_function(ch, &do_stand, "");
    return;
}

void do_sneak(Mobile* ch, char* argument)
{
    Affect af = { 0 };

    if (get_worn_armor_type(ch) >= ARMOR_MEDIUM) {
        send_to_char("Your armor is too bulky to move silently.\n\r", ch);
        return;
    }

    send_to_char("You attempt to move silently.\n\r", ch);
    affect_strip(ch, gsn_sneak);

    if (IS_AFFECTED(ch, AFF_SNEAK)) return;

    /* Use skill check seam for testability */
    if (skill_ops->check_simple(ch, gsn_sneak)) {
        check_improve(ch, gsn_sneak, true, 3);
        af.where = TO_AFFECTS;
        af.type = gsn_sneak;
        af.level = ch->level;
        af.duration = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_SNEAK;
        affect_to_mob(ch, &af);
    }
    else
        check_improve(ch, gsn_sneak, false, 3);

    return;
}

void do_hide(Mobile* ch, char* argument)
{
    if (get_worn_armor_type(ch) >= ARMOR_MEDIUM) {
        send_to_char("Your armor is too bulky to hide.\n\r", ch);
        return;
    }

    send_to_char("You attempt to hide.\n\r", ch);

    if (IS_AFFECTED(ch, AFF_HIDE)) REMOVE_BIT(ch->affect_flags, AFF_HIDE);

    /* Use skill check seam for testability */
    if (skill_ops->check_simple(ch, gsn_hide)) {
        SET_BIT(ch->affect_flags, AFF_HIDE);
        check_improve(ch, gsn_hide, true, 3);
    }
    else
        check_improve(ch, gsn_hide, false, 3);

    return;
}

// Contributed by Alander.
void do_visible(Mobile* ch, char* argument)
{
    affect_strip(ch, gsn_invis);
    affect_strip(ch, gsn_mass_invis);
    affect_strip(ch, gsn_sneak);
    REMOVE_BIT(ch->affect_flags, AFF_HIDE);
    REMOVE_BIT(ch->affect_flags, AFF_INVISIBLE);
    REMOVE_BIT(ch->affect_flags, AFF_SNEAK);
    send_to_char("Ok.\n\r", ch);
    return;
}

void do_recall(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    Mobile* victim;
    Room* location;

    if (IS_NPC(ch) && !IS_SET(ch->act_flags, ACT_PET)) {
        send_to_char("Only players can recall.\n\r", ch);
        return;
    }

    if (!str_cmp(argument, "set") && ch->pcdata) {
        if (IS_SET(ch->in_room->data->room_flags, ROOM_RECALL)) {
            printf_to_char(ch, "Your recall point is now set to %s.\n",
                NAME_STR(ch->in_room));
            ch->pcdata->recall = VNUM_FIELD(ch->in_room);
        }
        else {
            send_to_char("You can't use this location as your recall point.\n", ch);
        }
        return;
    }

    act("$n prays for transportation!", ch, NULL, NULL, TO_ROOM);

    VNUM recall = cfg_get_default_recall();

    if (IS_NPC(ch) && IS_SET(ch->act_flags, ACT_PET) && !IS_NPC(ch->master))
        recall = ch->master->pcdata->recall;
    else if (ch->pcdata)
        recall = ch->pcdata->recall;

    if ((location = get_room(NULL, recall)) == NULL) {
        send_to_char("You are completely lost.\n\r", ch);
        return;
    }

    if (ch->in_room == location) 
        return;

    if (IS_SET(ch->in_room->data->room_flags, ROOM_NO_RECALL)
        || IS_AFFECTED(ch, AFF_CURSE)) {
        send_to_char("Mota has forsaken you.\n\r", ch);
        return;
    }

    if ((victim = ch->fighting) != NULL) {
        int lose, skill;

        skill = get_skill(ch, gsn_recall);

        if (number_percent() < 80 * skill / 100) {
            check_improve(ch, gsn_recall, false, 6);
            WAIT_STATE(ch, 4);
            sprintf(buf, "You failed!.\n\r");
            send_to_char(buf, ch);
            return;
        }

        lose = (ch->desc != NULL) ? 25 : 50;
        gain_exp(ch, 0 - lose);
        check_improve(ch, gsn_recall, true, 4);
        sprintf(buf, "You recall from combat!  You lose %d exps.\n\r", lose);
        send_to_char(buf, ch);
        stop_fighting(ch, true);
    }

    if (IS_BOT(ch))
        ch->move = ch->max_move;
    else
        ch->move /= 2;

    act("$n disappears.", ch, NULL, NULL, TO_ROOM);
    transfer_mob(ch, location);
    act("$n appears in the room.", ch, NULL, NULL, TO_ROOM);
    do_function(ch, &do_look, "auto");

    if (ch->pet != NULL) 
        do_function(ch->pet, &do_recall, "");

    return;
}

void do_train(Mobile* ch, char* argument)
{
    int16_t stat = -1;
    char* pOutput = NULL;
    int cost;

    if (IS_NPC(ch))
        return;

    // Bot-only reset command: restores stats and training points to starting values
    if (!str_cmp(argument, "reset") && (IS_BOT(ch) || IS_IMMORTAL(ch))) {

        // Reset permanent stats to racial base + class prime stat bonus
        for (int i = 0; i < STAT_COUNT; i++) {
            ch->perm_stat[i] = race_table[ch->race].stats[i];
        }
        // Add prime stat bonus (given at level 1)
        ch->perm_stat[class_table[ch->ch_class].prime_stat] += 3;

        // Calculate how much HP/mana was added through training
        // Original perm values are based on class roll + level bonuses
        // For simplicity, we reset perm_hit and perm_mana to base values
        // Level 1 base: class dice roll (average) 
        int16_t base_hp = class_table[ch->ch_class].hp_min 
            + (class_table[ch->ch_class].hp_max - class_table[ch->ch_class].hp_min) / 2;
        int16_t base_mana = (class_table[ch->ch_class].fMana) 
            ? (100 + ch->perm_stat[STAT_INT] + ch->perm_stat[STAT_WIS]) / 2
            : 0;
        
        // For level 1, these are the starting values
        if (ch->level == 1) {
            ch->pcdata->perm_hit = base_hp;
            ch->pcdata->perm_mana = base_mana + 100;  // Base mana pool
        }
        // For higher levels, just reset to current max (preserving level gains)
        // and subtract any trained bonuses beyond natural progression
        // This is complex, so we just reset trains and let the player re-train
        
        // Reset training sessions to starting value
        ch->train = 3;
        
        // Recalculate current HP/mana/move
        reset_char(ch);
        
        send_to_char("Your stats and training sessions have been reset to starting values.\n\r", ch);
        return;
    }

    if (!cfg_get_train_anywhere()) {
        // Check for trainer.
        Mobile* mob = NULL;
        FOR_EACH_ROOM_MOB(mob, ch->in_room) {
            if (IS_NPC(mob) && IS_SET(mob->act_flags, ACT_TRAIN))
                break;
        }

        if (mob == NULL) {
            send_to_char("You can't do that here.\n\r", ch);
            return;
        }
    }

    if (argument[0] == '\0') {
        printf_to_char(ch, "You have %d training sessions.\n\r", ch->train);
        argument = "foo";
    }

    cost = 1;

    if (!str_cmp(argument, "str")) {
        if (class_table[ch->ch_class].prime_stat == STAT_STR) 
            cost = 1;
        stat = STAT_STR;
        pOutput = "strength";
    }

    else if (!str_cmp(argument, "int")) {
        if (class_table[ch->ch_class].prime_stat == STAT_INT) 
            cost = 1;
        stat = STAT_INT;
        pOutput = "intelligence";
    }

    else if (!str_cmp(argument, "wis")) {
        if (class_table[ch->ch_class].prime_stat == STAT_WIS) 
            cost = 1;
        stat = STAT_WIS;
        pOutput = "wisdom";
    }

    else if (!str_cmp(argument, "dex")) {
        if (class_table[ch->ch_class].prime_stat == STAT_DEX) 
            cost = 1;
        stat = STAT_DEX;
        pOutput = "dexterity";
    }

    else if (!str_cmp(argument, "con")) {
        if (class_table[ch->ch_class].prime_stat == STAT_CON) 
            cost = 1;
        stat = STAT_CON;
        pOutput = "constitution";
    }

    else if (!str_cmp(argument, "hp"))
        cost = 1;

    else if (!str_cmp(argument, "mana"))
        cost = 1;

    else {
        StringBuffer* sb = sb_new();
        sb_append(sb, "You can train:");
        if (ch->perm_stat[STAT_STR] < get_max_train(ch, STAT_STR))
            sb_append(sb, " str");
        if (ch->perm_stat[STAT_INT] < get_max_train(ch, STAT_INT))
            sb_append(sb, " int");
        if (ch->perm_stat[STAT_WIS] < get_max_train(ch, STAT_WIS))
            sb_append(sb, " wis");
        if (ch->perm_stat[STAT_DEX] < get_max_train(ch, STAT_DEX))
            sb_append(sb, " dex");
        if (ch->perm_stat[STAT_CON] < get_max_train(ch, STAT_CON))
            sb_append(sb, " con");
        sb_append(sb, " hp mana");

        if (sb->length > 0 && sb->data[sb->length - 1] != ':') {
            sb_append(sb, ".\n\r");
            send_to_char(sb_string(sb), ch);
        }
        else {
            send_to_char("You have nothing left to train.\n\r", ch);
        }
        sb_free(sb);

        return;
    }

    if (!str_cmp("hp", argument)) {
        if (cost > ch->train) {
            send_to_char("You don't have enough training sessions.\n\r", ch);
            return;
        }

        ch->train -= (int16_t)cost;
        ch->pcdata->perm_hit += 10;
        ch->max_hit += 10;
        ch->hit += 10;
        act("Your durability increases!", ch, NULL, NULL, TO_CHAR);
        act("$n's durability increases!", ch, NULL, NULL, TO_ROOM);
        return;
    }

    if (!str_cmp("mana", argument)) {
        if (cost > ch->train) {
            send_to_char("You don't have enough training sessions.\n\r", ch);
            return;
        }

        ch->train -= (int16_t)cost;
        ch->pcdata->perm_mana += 10;
        ch->max_mana += 10;
        ch->mana += 10;
        act("Your power increases!", ch, NULL, NULL, TO_CHAR);
        act("$n's power increases!", ch, NULL, NULL, TO_ROOM);
        return;
    }

    // Just in case.
    if (stat < 0) {
        bug("do_train: Bad index %d for option '%s'!", stat, argument);
        return;
    }

    if (ch->perm_stat[stat] >= get_max_train(ch, stat)) {
        act("Your $T is already at maximum.", ch, NULL, pOutput, TO_CHAR);
        return;
    }

    if (cost > ch->train) {
        send_to_char("You don't have enough training sessions.\n\r", ch);
        return;
    }

    ch->train -= (int16_t)cost;

    ch->perm_stat[stat] += 1;
    act("Your $T increases!", ch, NULL, pOutput, TO_CHAR);
    act("$n's $T increases!", ch, NULL, pOutput, TO_ROOM);
    return;
}
