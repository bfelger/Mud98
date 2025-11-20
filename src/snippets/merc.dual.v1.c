////////////////////////////////////////////////////////////////////////////////
// snippets/merc.dual.v1.c
////////////////////////////////////////////////////////////////////////////////

/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St�rfeldt, Tom Madsen, and Katja Nyboe.   *
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

/*
===========================================================================
This snippet was written by Erwin S. Andreasen, erwin@andreasen.org. You may
use this code freely, as long as you retain my name in all of the files. You
also have to mail me telling that you are using it. Note, however, that
I DO NOT OFFER ANY SUPPORT ON THIS CODE. If you have a problem installing
it, this means you probably SHOULD NOT BE RUNNING A MUD.

All my snippets are publically available at:

http://www.andreasen.org/
===========================================================================

Note that function prototypes are not included in this code - remember to add
them to merc.h yourself - also add the 'second' command to interp.c.

Secondary weapon code prototype. You should probably do a clean compile on
this one, due to the change to the MAX_WEAR define.

Last update: Oct 10, 1995
*/

#include <entities/mobile.h>
#include <entities/object.h>

#include <act_obj.h>
#include <comm.h>

/* wear object as a secondary weapon */
void do_second(Mobile* ch, char* argument)
{
    Object* obj;

    if (argument[0] == '\0') {
        send_to_char("Wear which weapon in your off-hand?\n\r", ch);
        return;
    }

    // I wanted 2WF to be a skill. -- Halivar
    if (get_skill(ch, gsn_dual_wield) <= 0) {
        send_to_char("You have no idea how to do that.\n\r", ch);
        return;
    }

    obj = get_obj_carry(ch, argument);

    if (obj == NULL) {
        send_to_char("You have no such thing in your backpack.\n\r", ch);
        return;
    }

    if ((get_eq_char(ch, WEAR_SHIELD) != NULL) ||
        (get_eq_char(ch, WEAR_HOLD) != NULL)) {
        send_to_char("You cannot use a secondary weapon while using a shield"
            " or holding an item\n\r", ch);
        return;
    }

    if (ch->level < obj->level) {
        printf_to_char(ch, "You must be level %d to use this object.\n\r",
            obj->level);
        act("$n tries to use $p, but is too inexperienced.",
            ch, obj, NULL, TO_ROOM);
        return;
    }

    if (get_eq_char(ch, WEAR_WIELD) == NULL) {
        send_to_char("You need to wield a primary weapon before wielding "
            "another in your off-hand.\n\r", ch);
        return;
    }

    // The below has been modified as per ROM's stat-mod calculation, which is 
    // normally *10 for wielding. I have halved that. -- Halivar
    if (get_obj_weight(obj) > 
        (str_mod[get_curr_stat(ch, STAT_STR)].wield * 5)) {
        send_to_char("This weapon is too heavy to be used as a secondary weapon"
            " by you.\n\r", ch);
        return;
    }

    // IMHO, the above weight limitation is sufficient to encourage most players
    // to use lighter weapons in their off-hand. For sufficiently brutish PCs,
    // we should allow for more cinematic play.

    // Just check to make sure it's not heavier than the main hand.
    if ((get_obj_weight(obj)) > get_obj_weight(get_eq_char(ch, WEAR_WIELD)))
    {
        send_to_char("Your off-hand weapon cannot be heavier than your primary "
            "weapon.\n\r", ch);
        return;
    }

    if (!remove_obj(ch, WEAR_WIELD_OH, true))
        return;


    act("$n wields $p in $s off-hand.", ch, obj, NULL, TO_ROOM);
    act("You wield $p in your off-hand.", ch, obj, NULL, TO_CHAR);
    equip_char(ch, obj, WEAR_WIELD_OH);
    return;
}

