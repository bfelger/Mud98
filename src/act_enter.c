/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
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

#include "act_enter.h"

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interp.h"
#include "mob_prog.h"

#include "entities/descriptor.h"
#include "entities/object.h"
#include "entities/player_data.h"

#include "data/mobile_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef _MSC_VER 
#include <sys/time.h>
#endif

/* random room generation procedure */
Room* get_random_room(Mobile* ch)
{
    RoomData* room_data;
    Room* room;

    for (;;) {
        room_data = get_room_data(number_range(0, MAX_VNUM));
        if (room_data != NULL) {
            // No instanced rooms. I really don't want to deal with that headache.
            if (room_data->area_data->inst_type == AREA_INST_MULTI)
                continue;
            room = room_data->instances;
            if (can_see_room(ch, room_data) && !room_is_private(room)
                && !IS_SET(room_data->room_flags, ROOM_PRIVATE)
                && !IS_SET(room_data->room_flags, ROOM_SOLITARY)
                && !IS_SET(room_data->room_flags, ROOM_SAFE)
                && (IS_NPC(ch) || IS_SET(ch->act_flags, ACT_AGGRESSIVE)
                    || !IS_SET(room_data->room_flags, ROOM_LAW)))
                break;
        }
    }

    return room;
}

/* RT Enter portals */
void do_enter(Mobile* ch, char* argument)
{
    Room* location;

    if (ch->fighting != NULL)
        return;

    /* nifty portal stuff */
    if (argument[0] != '\0') {
        Room* old_room;
        Object* portal;
        Mobile* fch;

        old_room = ch->in_room;

        portal = get_obj_list(ch, argument, &ch->in_room->objects);

        if (portal == NULL) {
            send_to_char("You don't see that here.\n\r", ch);
            return;
        }

        if (portal->item_type != ITEM_PORTAL
            || (IS_SET(portal->value[1], EX_CLOSED)
                && !IS_TRUSTED(ch, ANGEL))) {
            send_to_char("You can't seem to find a way in.\n\r", ch);
            return;
        }

        if (!IS_TRUSTED(ch, ANGEL) && !IS_SET(portal->value[2], PORTAL_NOCURSE)
            && (IS_AFFECTED(ch, AFF_CURSE)
                || IS_SET(old_room->data->room_flags, ROOM_NO_RECALL))) {
            send_to_char("Something prevents you from leaving...\n\r", ch);
            return;
        }

        if (IS_SET(portal->value[2], PORTAL_RANDOM) || portal->value[3] == -1) {
            location = get_random_room(ch);
            portal->value[3] = VNUM_FIELD(location); /* for record keeping :) */
        }
        else if (IS_SET(portal->value[2], PORTAL_BUGGY) && (number_percent() < 5))
            location = get_random_room(ch);
        else
            location = get_room(old_room->area, portal->value[3]);

        if (location == NULL || location == old_room
            || !can_see_room(ch, location->data)
            || (room_is_private(location) && !IS_TRUSTED(ch, IMPLEMENTOR))) {
            act("$p doesn't seem to go anywhere.", ch, portal, NULL, TO_CHAR);
            return;
        }

        if (IS_NPC(ch) && IS_SET(ch->act_flags, ACT_AGGRESSIVE)
            && IS_SET(location->data->room_flags, ROOM_LAW)) {
            send_to_char("Something prevents you from leaving...\n\r", ch);
            return;
        }

        act("$n steps into $p.", ch, portal, NULL, TO_ROOM);

        if (IS_SET(portal->value[2], PORTAL_NORMAL_EXIT))
            act("You enter $p.", ch, portal, NULL, TO_CHAR);
        else
            act("You walk through $p and find yourself somewhere else...", ch,
                portal, NULL, TO_CHAR);

        transfer_mob(ch, location);

        if (IS_SET(portal->value[2], PORTAL_GOWITH)) /* take the gate along */
        {
            obj_from_room(portal);
            obj_to_room(portal, location);
        }

        if (IS_SET(portal->value[2], PORTAL_NORMAL_EXIT))
            act("$n has arrived.", ch, portal, NULL, TO_ROOM);
        else
            act("$n has arrived through $p.", ch, portal, NULL, TO_ROOM);

        do_function(ch, &do_look, "auto");

        /* charges */
        if (portal->value[0] > 0) {
            portal->value[0]--;
            if (portal->value[0] == 0) 
                portal->value[0] = -1;
        }

        /* protect against circular follows */
        if (old_room == location) 
            return;

        FOR_EACH_ROOM_MOB(fch, old_room) {
            if (portal == NULL || portal->value[0] == -1)
                /* no following through dead portals */
                continue;

            if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM)
                && fch->position < POS_STANDING)
                do_function(fch, &do_stand, "");

            if (fch->master == ch && fch->position == POS_STANDING) {
                if (IS_SET(ch->in_room->data->room_flags, ROOM_LAW)
                    && (IS_NPC(fch) && IS_SET(fch->act_flags, ACT_AGGRESSIVE))) {
                    act("You can't bring $N into the city.", ch, NULL, fch,
                        TO_CHAR);
                    act("You aren't allowed in the city.", fch, NULL, NULL,
                        TO_CHAR);
                    continue;
                }

                act("You follow $N.", fch, NULL, ch, TO_CHAR);
                do_function(fch, &do_enter, argument);
            }
        }

        if (portal != NULL && portal->value[0] == -1) {
            act("$p fades out of existence.", ch, portal, NULL, TO_CHAR);
            if (ch->in_room == old_room)
                act("$p fades out of existence.", ch, portal, NULL, TO_ROOM);
            else if (ROOM_HAS_MOBS(old_room)) {
                act("$p fades out of existence.", old_room, portal, NULL, TO_ROOM);
            }
            extract_obj(portal);
        }

        // If someone is following the char, these triggers get activated for 
        // the followers before the char, but it's safer this way...
        if (IS_NPC(ch) && HAS_TRIGGER(ch, TRIG_ENTRY))
            mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_ENTRY);
        if (!IS_NPC(ch))
            mp_greet_trigger(ch);

        return;
    }

    send_to_char("Nope, can't do it.\n\r", ch);
    return;
}
