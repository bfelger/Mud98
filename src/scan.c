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

#include "merc.h"

#include "act_move.h"
#include "comm.h"
#include "db.h"
#include "handler.h"

#include "entities/mobile.h"
#include "entities/descriptor.h"

#include "data/mobile_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

void scan_list(Room* scan_room, Mobile* ch, int16_t depth, int16_t door);
void scan_char(Mobile* victim, Mobile* ch, int16_t depth, int16_t door);

void do_scan(Mobile* ch, char* argument)
{
    char arg1[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    Room* scan_room;
    RoomExit* room_exit;
    int16_t door, depth;

    READ_ARG(arg1);

    if (arg1[0] == '\0') {
        act("$n looks all around.", ch, NULL, NULL, TO_ROOM);
        send_to_char("Looking around you see:\n\r", ch);
        scan_list(ch->in_room, ch, 0, -1);

        for (door = 0; door < 6; door++) {
            if ((room_exit = ch->in_room->exit[door]) != NULL)
                scan_list(room_exit->to_room, ch, 1, door);
        }
        return;
    }
    else if (!str_cmp(arg1, "n") || !str_cmp(arg1, "north"))
        door = 0;
    else if (!str_cmp(arg1, "e") || !str_cmp(arg1, "east"))
        door = 1;
    else if (!str_cmp(arg1, "s") || !str_cmp(arg1, "south"))
        door = 2;
    else if (!str_cmp(arg1, "w") || !str_cmp(arg1, "west"))
        door = 3;
    else if (!str_cmp(arg1, "u") || !str_cmp(arg1, "up"))
        door = 4;
    else if (!str_cmp(arg1, "d") || !str_cmp(arg1, "down"))
        door = 5;
    else {
        send_to_char("Which way do you want to scan?\n\r", ch);
        return;
    }

    act("You peer intently $T.", ch, NULL, dir_list[door].name, TO_CHAR);
    act("$n peers intently $T.", ch, NULL, dir_list[door].name, TO_ROOM);
    sprintf(buf, "Looking %s you see:\n\r", dir_list[door].name);

    scan_room = ch->in_room;

    for (depth = 1; depth < 4; depth++) {
        if ((room_exit = scan_room->exit[door]) != NULL) {
            scan_room = room_exit->to_room;
            scan_list(room_exit->to_room, ch, depth, door);
        }
    }
    return;
}

void scan_list(Room* scan_room, Mobile* ch, int16_t depth, int16_t door)
{
    Mobile* rch;

    if (scan_room == NULL)
        return;

    // Don't peek through closed doors.
    if (door != -1 && scan_room->exit[dir_list[door].rev_dir] != NULL
        && IS_SET(scan_room->exit[dir_list[door].rev_dir]->exit_flags, EX_CLOSED))
     return;

    FOR_EACH_ROOM_MOB(rch, scan_room) {
        if (rch == ch) 
            continue;
        if (!IS_NPC(rch) && rch->invis_level > get_trust(ch)) 
            continue;
        if (can_see(ch, rch)) 
            scan_char(rch, ch, depth, door);
    }
    return;
}

void scan_char(Mobile* victim, Mobile* ch, int16_t depth, int16_t door)
{
    char buf[MAX_INPUT_LENGTH] = "";
    char buf2[MAX_INPUT_LENGTH] = "";

    sprintf(buf, "%s", PERS(victim, ch));
    strcat(buf, ", ");
    switch (depth) {
    case 0:
        sprintf(buf2, "right here.");
        break;
    case 1:
        sprintf(buf2, "nearby to the %s.", dir_list[door].name);
        break;
    case 2:
        sprintf(buf2, "not far %s.", dir_list[door].name);
        break;
    case 4:
        sprintf(buf2, "off in the distance %s.", dir_list[door].name);
        break;
    }
    strcat(buf, buf2);
    strcat(buf, "\n\r");

    send_to_char(buf, ch);
    return;
}
