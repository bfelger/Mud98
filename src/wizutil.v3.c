/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvements copyright (C) 1992, 1993 by Michael         *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefiting.  We hope that you share your changes too.  What goes       *
 *  around, comes around.                                                  *
 ***************************************************************************/

#include "merc.h"

#include "comm.h"
#include "interp.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>

#ifndef _MSC_VER
#include <unistd.h>
#else
#define unlink _unlink
#endif

const char wizutil_id[] = "$Id: wizutil.c,v 1.6 1996/01/04 21:30:45 root Exp root $";

/*
===========================================================================
This snippet was written by Erwin S. Andreasen, erwin@andreasen.org. You may
use this code freely, as long as you retain my name in all of the files. You
also have to mail me telling that you are using it. I am giving this,
hopefully useful, piece of source code to you for free, and all I require
from you is some feedback.

Please mail me if you find any bugs or have any new ideas or just comments.

All my snippets are publically available at:

http://www.andreasen.org/
===========================================================================

  Various administrative utility commands.
  Version: 3 - Last update: January 1996.

  To use these 2 commands you will have to add a filename field to AREA_DATA.
  This value can be found easily in load_area while booting - the filename
  of the current area boot_db is reading from is in the strArea global.

  Since version 2 following was added:

  A rename command which renames a player. Search for do_rename to see
  more info on it.

  A FOR command which executes a command at/on every player/mob/location.

  Fixes since last release: None.
*/


// To have VLIST show more than vnum 0 - 9900, change the number below:
#define MAX_SHOW_VNUM   99 // Show only 1 - 100*100 */

extern ROOM_INDEX_DATA* room_index_hash[];	// db.c

/* opposite directions */
const int16_t opposite_dir[6] = {
    DIR_SOUTH, DIR_WEST, DIR_NORTH, DIR_EAST, DIR_DOWN, DIR_UP
};

// Cut the 'short' name of an area (e.g. MIDGAARD, MIRROR, etc.)
// Assumes that the filename saved in the AREA_DATA struct is something like 
// "midgaard.are"
char* get_area_name(AREA_DATA* pArea)
{
    static char buffer[256];
    char* period;

    assert(pArea != NULL);

    // Terminate the string at the period, if it exists
    sprintf(buffer, "%s", pArea->file_name);
    period = strchr(buffer, '.');
    if (period)
        *period = '\0';

    return buffer;
}

typedef enum {
    exit_from, exit_to, exit_both
} exit_status;

// Depending on status print > or < or <> between the 2 rooms
void room_pair(ROOM_INDEX_DATA* left, ROOM_INDEX_DATA* right, exit_status ex,
    char* buffer)
{
    char* sExit;

    switch (ex) {
    default:
        sExit = "??"; break; // Invalid usage
    case exit_from:
        sExit = "< "; break;
    case exit_to:
        sExit = " >"; break;
    case exit_both:
        sExit = "<>"; break;
    }

    sprintf(buffer, "%10"PRVNUM" %-26.26s %s %"PRVNUM" %-26.26s(%-8.8s)\n\r",
        left->vnum,
        left->name,
        sExit,
        right->vnum,
        right->name,
        get_area_name(right->area)
    );
}

// For every exit in 'room' which leads to or from pArea but NOT both, print it
void check_exits(ROOM_INDEX_DATA* room, AREA_DATA* pArea, char* buffer)
{
    char buf[MAX_STRING_LENGTH];
    int i;
    EXIT_DATA* exit;
    ROOM_INDEX_DATA* to_room;

    strcpy(buffer, "");
    for (i = 0; i < 6; i++) {
        exit = room->exit[i];
        if (!exit)
            continue;
        else
            to_room = exit->u1.to_room;

        if (to_room) {
            // There is something on the other side
            if ((room->area == pArea) && (to_room->area != pArea)) {
                // An exit from our area to another area/
                // check first if it is a two-way exit */
                if (to_room->exit[opposite_dir[i]] &&
                    to_room->exit[opposite_dir[i]]->u1.to_room == room)
                    room_pair(room, to_room, exit_both, buf);	// <>
                else
                    room_pair(room, to_room, exit_to, buf);		// >

                strcat(buffer, buf);
            }
            else if ((room->area != pArea) && (exit->u1.to_room->area == pArea)) {
                // an exit from another area to our area */
                if (!(to_room->exit[opposite_dir[i]] &&
                    to_room->exit[opposite_dir[i]]->u1.to_room == room)) {
                    // two-way exits are handled in the other if 
                    room_pair(to_room, room, exit_from, buf);
                    strcat(buffer, buf);
                }

            }
        }
    }
}

// For now, no arguments, just list the current area
void do_exlist(CHAR_DATA* ch, char* argument)
{
    ROOM_INDEX_DATA* room;
    char buffer[MAX_STRING_LENGTH];

    AREA_DATA* pArea = ch->in_room->area; // This is the area we want info on 
    for (int i = 0; i < MAX_KEY_HASH; i++) {
        // Room index hash table
        for (room = room_index_hash[i]; room != NULL; room = room->next) {
            // Run through all the rooms on the MUD
            check_exits(room, pArea, buffer);
            send_to_char(buffer, ch);
        }
    }
}

// Show a list of all used VNUMS 
#define COLUMNS 5   // number of columns */
#define MAX_ROW ((MAX_SHOW_VNUM / COLUMNS)+1) // rows

void do_vlist(CHAR_DATA* ch, char* argument)
{
    VNUM i;
    VNUM j;
    VNUM vnum;
    ROOM_INDEX_DATA* room;
    char buffer[MAX_ROW * 100]; // Should be plenty */
    char buf2[100];

    for (i = 0; i < MAX_ROW; i++) {
        strcpy(buffer, "");
        for (j = 0; j < COLUMNS; j++) {
            vnum = ((j * MAX_ROW) + i); /* find a vnum which should be there */
            if (vnum < MAX_SHOW_VNUM) {
                /* each zone has to have a XXX01 room */
                room = get_room_index(vnum * 100 + 1);
                sprintf(buf2, "%"PRVNUM" %-8.8s  ", vnum,
                    room ? get_area_name(room->area) : "-");
                /* something there or unused ? */
                strcat(buffer, buf2);
            }
        }

        send_to_char(buffer, ch);
        send_to_char("\n\r", ch);
    } /* for rows */
}

/*
 * do_rename renames a player to another name.
 * PCs only. Previous file is deleted, if it exists.
 * Char is then saved to new file.
 * New name is checked against std. checks, existing offline players and
 * online players.
 * .gz files are checked for too, just in case.
 */

bool check_parse_name(char* name);  // comm.c

void do_rename(CHAR_DATA* ch, char* argument)
{
    char old_name[MAX_INPUT_LENGTH] = { 0 };
    char new_name[MAX_INPUT_LENGTH] = { 0 };
    char strsave[MAX_INPUT_LENGTH] = { 0 };

    CHAR_DATA* victim;
    FILE* file;

    argument = one_argument(argument, old_name); /* find new/old name */
    one_argument(argument, new_name);

    /* Trivial checks */
    if (!old_name[0]) {
        send_to_char("Rename who?\n\r", ch);
        return;
    }

    victim = get_char_world(ch, old_name);

    if (!victim) {
        send_to_char("There is no such a person online.\n\r", ch);
        return;
    }

    if (IS_NPC(victim)) {
        send_to_char("You cannot use Rename on NPCs.\n\r", ch);
        return;
    }

    /* allow rename self new_name,but otherwise only lower level */
    if ((victim != ch) && (get_trust(victim) >= get_trust(ch))) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    if (!victim->desc || (victim->desc->connected != CON_PLAYING)) {
        send_to_char("This player has lost his link or is inside a pager or the like.\n\r", ch);
        return;
    }

    if (!new_name[0]) {
        send_to_char("Rename to what new name?\n\r", ch);
        return;
    }

    /* Insert check for clan here!! */
    /*

    if (victim->clan)
    {
        send_to_char ("This player is member of a clan, remove him from there first.\n\r",ch);
        return;
    }
    */

    if (!check_parse_name(new_name)) {
        send_to_char("The new name is illegal.\n\r", ch);
        return;
    }

    /* First, check if there is a player named that off-line */
    sprintf(strsave, "%s%s%s", area_dir, PLAYER_DIR, capitalize(new_name));

    fclose(fpReserve); /* close the reserve file */
    if ((file = fopen(strsave, "r")) != NULL) {
        send_to_char("A player with that name already exists!\n\r", ch);
        fclose(file);
        fpReserve = fopen(NULL_FILE, "r"); /* is this really necessary these days? */
        return;
    }
    fpReserve = fopen(NULL_FILE, "r");  /* reopen the extra file */

    /* Check .gz file ! */
    sprintf(strsave, "%s%s%s%s", area_dir, PLAYER_DIR, capitalize(new_name), ".gz");

    fclose(fpReserve);
    if ((file = fopen(strsave, "r")) != NULL) {
        send_to_char("A player with that name already exists in a compressed file!\n\r", ch);
        fclose(file);
        fpReserve = fopen(NULL_FILE, "r");
        return;
    }
    fpReserve = fopen(NULL_FILE, "r");  /* reopen the extra file */

    /* check for playing level-1 non-saved */
    if (get_char_world(ch, new_name)) {
        send_to_char("A player with the name you specified already exists!\n\r", ch);
        return;
    }

    // Save the filename of the old name
    sprintf(strsave, "%s%s%s", area_dir, PLAYER_DIR, capitalize(new_name));

    // Rename the character and save him to a new file.
    // NOTE: Players who are level 1 do NOT get saved under a new name.

    free_string(victim->name);
    victim->name = str_dup(capitalize(new_name));

    save_char_obj(victim);

    /* unlink the old file */
    unlink(strsave); /* unlink does return a value.. but we do not care */

    /* That's it! */

    send_to_char("Character renamed.\n\r", ch);

    victim->position = POS_STANDING; /* I am laaazy */
    act("$n has renamed you to $N!", ch, NULL, victim, TO_VICT);
}

/* Super-AT command:

FOR ALL <action>
FOR MORTALS <action>
FOR GODS <action>
FOR MOBS <action>
FOR EVERYWHERE <action>

Executes action several times, either on ALL players (not including yourself),
MORTALS (including trusted characters), GODS (characters with level higher than
L_HERO), MOBS (Not recommended) or every room (not recommended either!)

If you insert a # in the action, it will be replaced by the name of the target.

If # is a part of the action, the action will be executed for every target
in game. If there is no #, the action will be executed for every room containg
at least one target, but only once per room. # cannot be used with FOR EVERY-
WHERE. # can be anywhere in the action.

Example:

FOR ALL SMILE -> you will only smile once in a room with 2 players.
FOR ALL TWIDDLE # -> In a room with A and B, you will twiddle A then B.

Destroying the characters this command acts upon MAY cause it to fail. Try to
avoid something like FOR MOBS PURGE (although it actually works at my MUD).

FOR MOBS TRANS 3054 (transfer ALL the mobs to Midgaard temple) does NOT work
though :)

The command works by transporting the character to each of the rooms with
target in them. Private rooms are not violated.

*/

// Expand the name of a character into a string that identifies THAT character 
// within a room. E.g. the second 'guard' -> 2. guard 
const char* name_expand(CHAR_DATA* ch)
{
    int count = 1;
    CHAR_DATA* rch;
    char name[MAX_INPUT_LENGTH]; /*  HOPEFULLY no mob has a name longer than THAT */

    static char outbuf[MAX_INPUT_LENGTH * 2];

    if (!IS_NPC(ch))
        return ch->name;

    one_argument(ch->name, name); /* copy the first word into name */

    if (!name[0]) {
        // weird mob .. no keywords
        strcpy(outbuf, ""); // Do not return NULL, just an empty buffer */
        return outbuf;
    }

    for (rch = ch->in_room->people; rch && (rch != ch); rch = rch->next_in_room)
        if (is_name(name, rch->name))
            count++;

    sprintf(outbuf, "%d.%s", count, name);
    return outbuf;
}

void do_for(CHAR_DATA* ch, char* argument)
{
    char range[MAX_INPUT_LENGTH] = { 0 };
    char buf[MAX_STRING_LENGTH] = { 0 };
    bool fGods = false, fMortals = false, fMobs = false, fEverywhere = false, found;
    ROOM_INDEX_DATA* room, * old_room;
    CHAR_DATA* p;
    CHAR_DATA* p_next = NULL;
    int i;

    argument = one_argument(argument, range);

    /* invalid usage? */
    if (!range[0] || !argument[0]) {
        do_help(ch, "for");
        return;
    }

    if (!str_prefix("quit", argument)) {
        send_to_char("Are you trying to crash the MUD or something?\n\r", ch);
        return;
    }

    if (!str_cmp(range, "all")) {
        fMortals = true;
        fGods = true;
    }
    else if (!str_cmp(range, "gods"))
        fGods = true;
    else if (!str_cmp(range, "mortals"))
        fMortals = true;
    else if (!str_cmp(range, "mobs"))
        fMobs = true;
    else if (!str_cmp(range, "everywhere"))
        fEverywhere = true;
    else
        do_help(ch, "for"); /* show syntax */

    /* do not allow # to make it easier */
    if (fEverywhere && strchr(argument, '#')) {
        send_to_char("Cannot use FOR EVERYWHERE with the # thingie.\n\r", ch);
        return;
    }

    if (strchr(argument, '#')) {
        // replace # ?
        for (p = char_list; p; p = p_next) {
            p_next = p->next; /* In case someone DOES try to AT MOBS SLAY # */
            found = false;

            if (!(p->in_room) || room_is_private(p->in_room) || (p == ch))
                continue;

            if (IS_NPC(p) && fMobs)
                found = true;
            else if (!IS_NPC(p) && p->level >= LEVEL_IMMORTAL && fGods)
                found = true;
            else if (!IS_NPC(p) && p->level < LEVEL_IMMORTAL && fMortals)
                found = true;

            /* It looks ugly to me.. but it works :) */
            if (found) {
                 // p is 'appropriate'
                char* pSource = argument; /* head of buffer to be parsed */
                char* pDest = buf; /* parse into this */

                while (*pSource) {
                    if (*pSource == '#') {
                        // Replace # with name of target 
                        const char* namebuf = name_expand(p);

                        if (namebuf) {
                            // in case there is no mob name ?? 
                            while (*namebuf) {
                                // copy name over
                                *(pDest++) = *(namebuf++);
                            }
                        }

                        pSource++;
                    }
                    else
                        *(pDest++) = *(pSource++);
                } /* while */
                *pDest = '\0'; /* Terminate */

                /* Execute */
                old_room = ch->in_room;
                char_from_room(ch);
                char_to_room(ch, p->in_room);
                interpret(ch, buf);
                char_from_room(ch);
                char_to_room(ch, old_room);

            } /* if found */
        } /* for every char */
    }
    else {
         /* just for every room with the appropriate people in it */
        for (i = 0; i < MAX_KEY_HASH; i++) {
            /* run through all the buckets */
            for (room = room_index_hash[i]; room; room = room->next) {
                found = false;
                /* Anyone in here at all? */
                if (fEverywhere)
                    /* Everywhere executes always */
                    found = true;
                else if (!room->people)
                    /* Skip it if room is empty */
                    continue;

                /* Check if there is anyone here of the requried type */
                /* Stop as soon as a match is found or there are no more ppl in room */
                for (p = room->people; p && !found; p = p->next_in_room) {
                    /* do not execute on oneself */
                    if (p == ch)
                        continue;

                    if (IS_NPC(p) && fMobs)
                        found = true;
                    else if (!IS_NPC(p) && (p->level >= LEVEL_IMMORTAL) && fGods)
                        found = true;
                    else if (!IS_NPC(p) && (p->level <= LEVEL_IMMORTAL) && fMortals)
                        found = true;
                } /* for everyone inside the room */

                /* Any of the required type here AND room not private? */
                if (found && !room_is_private(room)) {
                    /* This may be ineffective. Consider moving character out of old_room
                       once at beginning of command then moving back at the end.
                       This however, is more safe?
                    */

                    old_room = ch->in_room;
                    char_from_room(ch);
                    char_to_room(ch, room);
                    interpret(ch, argument);
                    char_from_room(ch);
                    char_to_room(ch, old_room);
                } /* if found */
            } /* for every room in a bucket */
        }
    } /* if strchr */
} /* do_for */
