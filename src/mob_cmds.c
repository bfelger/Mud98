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

/***************************************************************************
 *                                                                         *
 *  Based on MERC 2.2 MOBprograms by N'Atas-ha.                            *
 *  Written and adapted to ROM 2.4 by                                      *
 *          Markku Nylander (markku.nylander@uta.fi)                       *
 *                                                                         *
 ***************************************************************************/

#include "merc.h"

#include "mob_cmds.h"

#include "act_comm.h"
#include "act_move.h"
#include "act_obj.h"
#include "act_wiz.h"
#include "comm.h"
#include "db.h"
#include "fight.h"
#include "handler.h"
#include "magic.h"
#include "mob_prog.h"
#include "skills.h"

#include "entities/mobile.h"
#include "entities/descriptor.h"
#include "entities/object.h"

#include "data/mobile_data.h"
#include "data/skill.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// Command table.
const MobCmdInfo mob_cmd_table[] = {
    { "asound",     do_mpasound     },
    { "assist",     do_mpassist     },
    { "at",         do_mpat         },
    { "call",       do_mpcall       },
    { "cancel",     do_mpcancel     },
    { "cast",       do_mpcast       },
    { "damage",     do_mpdamage     },
    { "delay",      do_mpdelay      },
    { "echo",       do_mpecho       },
    { "echoaround", do_mpechoaround },
    { "echoat",     do_mpechoat     },
    { "flee",       do_mpflee       },
    { "force",      do_mpforce      },
    { "forget",     do_mpforget     },
    { "gecho",      do_mpgecho      },
    { "gforce",     do_mpgforce     },
    { "goto",       do_mpgoto       },
    { "gtransfer",  do_mpgtransfer  },
    { "junk",       do_mpjunk       },
    { "kill",       do_mpkill       },
    { "mload",      do_mpmload      },
    { "oload",      do_mpoload      },
    { "otransfer",  do_mpotransfer  },
    { "purge",      do_mppurge      },
    { "quest",      do_mpquest      },
    { "remember",   do_mpremember   },
    { "remove",     do_mpremove     },
    { "transfer",   do_mptransfer   },
    { "vforce",     do_mpvforce     },
    { "zecho",      do_mpzecho      },
    { "",           0               }
};

void do_mob(Mobile* ch, char* argument)
{
    // Security check!
    if (ch->desc != NULL && get_trust(ch) < MAX_LEVEL)
        return;
    mob_interpret(ch, argument);
}
/*
 * Mob command interpreter. Implemented separately for security and speed
 * reasons. A trivial hack of interpret()
 */
void mob_interpret(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH], command[MAX_INPUT_LENGTH];
    int cmd;

    READ_ARG(command);

    // Look for command in command table.
    for (cmd = 0; mob_cmd_table[cmd].name[0] != '\0'; cmd++) {
        if (command[0] == mob_cmd_table[cmd].name[0]
            && !str_prefix(command, mob_cmd_table[cmd].name)) {
            (*mob_cmd_table[cmd].do_fun) (ch, argument);
            return;
        }
    }
    sprintf(buf, "Mob_interpret: invalid cmd from mob %d: '%s'",
        IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0, command);
    bug(buf, 0);
}

char* mprog_type_to_name(MobProgTrigger type)
{
    switch (type) {
    case TRIG_ACT:      return "ACT";
    case TRIG_SPEECH:   return "SPEECH";
    case TRIG_RANDOM:   return "RANDOM";
    case TRIG_FIGHT:    return "FIGHT";
    case TRIG_HPCNT:    return "HPCNT";
    case TRIG_DEATH:    return "DEATH";
    case TRIG_ENTRY:    return "ENTRY";
    case TRIG_GREET:    return "GREET";
    case TRIG_GRALL:    return "GRALL";
    case TRIG_GIVE:     return "GIVE";
    case TRIG_BRIBE:    return "BRIBE";
    case TRIG_KILL:     return "KILL";
    case TRIG_DELAY:    return "DELAY";
    case TRIG_SURR:     return "SURRENDER";
    case TRIG_EXIT:     return "EXIT";
    case TRIG_EXALL:    return "EXALL";
    default:            return "ERROR";
    }
}

/*
 * Displays MOBprogram triggers of a mobile
 *
 * Syntax: mpstat [name]
 */
void do_mpstat(Mobile* ch, char* argument)
{
    char arg[MAX_STRING_LENGTH];
    MobProg* mprg;
    Mobile* victim;
    int i;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Mpstat whom?\n\r", ch);
        return;
    }

    if ((victim = get_mob_world(ch, arg)) == NULL) {
        send_to_char("No such creature.\n\r", ch);
        return;
    }

    if (!IS_NPC(victim)) {
        send_to_char("That is not a mobile.\n\r", ch);
        return;
    }

    if ((victim = get_mob_world(ch, arg)) == NULL) {
        send_to_char("No such creature visible.\n\r", ch);
        return;
    }

    sprintf(arg, "Mobile #%-6d [%s]\n\r",
        VNUM_FIELD(victim->prototype), victim->short_descr);
    send_to_char(arg, ch);

    sprintf(arg, "Delay   %-6d [%s]\n\r",
        victim->mprog_delay,
        victim->mprog_target == NULL
        ? "No target" : NAME_STR(victim->mprog_target));
    send_to_char(arg, ch);

    if (!victim->prototype->mprog_flags) {
        send_to_char("[No programs set]\n\r", ch);
        return;
    }

    i = 0;
    FOR_EACH(mprg, victim->prototype->mprogs) {
        sprintf(arg, "[%2d] Trigger [%-8s] Program [%4d] Phrase [%s]\n\r",
            ++i,
            mprog_type_to_name(mprg->trig_type),
            mprg->vnum,
            mprg->trig_phrase);
        send_to_char(arg, ch);
    }
}

/*
 * Displays the source code of a given MOBprogram
 *
 * Syntax: mpdump [vnum]
 */
void do_mpdump(Mobile* ch, char* argument)
{
    char buf[MAX_INPUT_LENGTH];
    MobProgCode* mprg;

    one_argument(argument, buf);
    if ((mprg = get_mprog_index(STRTOVNUM(buf))) == NULL) {
        send_to_char("No such MOBprogram.\n\r", ch);
        return;
    }
    page_to_char(mprg->code, ch);
}

/*
 * Prints the argument to all active players in the game
 *
 * Syntax: mob gecho [string]
 */
void do_mpgecho(Mobile* ch, char* argument)
{
    Descriptor* d;

    if (argument[0] == '\0') {
        bug("MpGEcho: missing argument from vnum %"PRVNUM"",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    FOR_EACH(d, descriptor_list) {
        if (d->connected == CON_PLAYING) {
            if (IS_IMMORTAL(d->character))
                send_to_char("Mob echo> ", d->character);
            send_to_char(argument, d->character);
            send_to_char("\n\r", d->character);
        }
    }
}

/*
 * Prints the argument to all players in the same area as the mob
 *
 * Syntax: mob zecho [string]
 */
void do_mpzecho(Mobile* ch, char* argument)
{
    Descriptor* d;

    if (argument[0] == '\0') {
        bug("MpZEcho: missing argument from vnum %"PRVNUM"",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    if (ch->in_room == NULL)
        return;

    FOR_EACH(d, descriptor_list) {
        if (d->connected == CON_PLAYING
            && d->character->in_room != NULL
            && d->character->in_room->area == ch->in_room->area) {
            if (IS_IMMORTAL(d->character))
                send_to_char("Mob echo> ", d->character);
            send_to_char(argument, d->character);
            send_to_char("\n\r", d->character);
        }
    }
}

/*
 * Prints the argument to all the rooms aroud the mobile
 *
 * Syntax: mob asound [string]
 */
void do_mpasound(Mobile* ch, char* argument)
{

    Room* was_in_room;
    int              door;

    if (argument[0] == '\0')
        return;

    was_in_room = ch->in_room;
    for (door = 0; door < 6; door++) {
        RoomExit* room_exit;

        if ((room_exit = was_in_room->exit[door]) != NULL
            && room_exit->to_room != NULL
            && room_exit->to_room != was_in_room) {
            ch->in_room = room_exit->to_room;
            MOBtrigger = false;
            act(argument, ch, NULL, NULL, TO_ROOM);
            MOBtrigger = true;
        }
    }
    ch->in_room = was_in_room;
    return;

}

/*
 * Lets the mobile kill any player or mobile without murder
 *
 * Syntax: mob kill [victim]
 */
void do_mpkill(Mobile* ch, char* argument)
{
    char      arg[MAX_INPUT_LENGTH];
    Mobile* victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
        return;

    if ((victim = get_mob_room(ch, arg)) == NULL)
        return;

    if (victim == ch || IS_NPC(victim) || ch->position == POS_FIGHTING)
        return;

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
        bug("MpKill - Charmed mob attacking master from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    multi_hit(ch, victim, TYPE_UNDEFINED);
    return;
}

/*
 * Lets the mobile assist another mob or player
 *
 * Syntax: mob assist [character]
 */
void do_mpassist(Mobile* ch, char* argument)
{
    char      arg[MAX_INPUT_LENGTH];
    Mobile* victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
        return;

    if ((victim = get_mob_room(ch, arg)) == NULL)
        return;

    if (victim == ch || ch->fighting != NULL || victim->fighting == NULL)
        return;

    multi_hit(ch, victim->fighting, TYPE_UNDEFINED);
    return;
}


/*
 * Lets the mobile destroy an object in its inventory
 * it can also destroy a worn object and it can destroy
 * items using all.xxxxx or just plain all of them
 *
 * Syntax: mob junk [item]
 */

void do_mpjunk(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;

    one_argument(argument, arg);

    if (arg[0] == '\0')
        return;

    if (str_cmp(arg, "all") && str_prefix("all.", arg)) {
        if ((obj = get_obj_wear(ch, arg)) != NULL) {
            unequip_char(ch, obj);
            extract_obj(obj);
            return;
        }
        if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
            return;
        extract_obj(obj);
    }
    else
        FOR_EACH_MOB_OBJ(obj, ch) {
            if (arg[3] == '\0' || is_name(&arg[4], NAME_STR(obj))) {
                if (obj->wear_loc != WEAR_UNHELD)
                    unequip_char(ch, obj);
                extract_obj(obj);
            }
        }

    return;

}

/*
 * Prints the message to everyone in the room other than the mob and victim
 *
 * Syntax: mob echoaround [victim] [string]
 */

void do_mpechoaround(Mobile* ch, char* argument)
{
    char       arg[MAX_INPUT_LENGTH];
    Mobile* victim;

    READ_ARG(arg);

    if (arg[0] == '\0')
        return;

    if ((victim = get_mob_room(ch, arg)) == NULL)
        return;

    act(argument, ch, NULL, victim, TO_NOTVICT);
}

/*
 * Prints the message to only the victim
 *
 * Syntax: mob echoat [victim] [string]
 */
void do_mpechoat(Mobile* ch, char* argument)
{
    char       arg[MAX_INPUT_LENGTH];
    Mobile* victim;

    READ_ARG(arg);

    if (arg[0] == '\0' || argument[0] == '\0')
        return;

    if ((victim = get_mob_room(ch, arg)) == NULL)
        return;

    act(argument, ch, NULL, victim, TO_VICT);
}

/*
 * Prints the message to the room at large
 *
 * Syntax: mpecho [string]
 */
void do_mpecho(Mobile* ch, char* argument)
{
    if (argument[0] == '\0')
        return;
    act(argument, ch, NULL, NULL, TO_ROOM);
}

/*
 * Lets the mobile load another mobile.
 *
 * Syntax: mob mload [vnum]
 */
void do_mpmload(Mobile* ch, char* argument)
{
    char            arg[MAX_INPUT_LENGTH];
    MobPrototype* p_mob_proto;
    Mobile* victim;
    VNUM vnum;

    one_argument(argument, arg);

    if (ch->in_room == NULL || arg[0] == '\0' || !is_number(arg))
        return;

    vnum = STRTOVNUM(arg);
    if ((p_mob_proto = get_mob_prototype(vnum)) == NULL) {
        sprintf(arg, "Mpmload: bad mob index (%"PRVNUM") from mob %"PRVNUM".",
            vnum, IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        bug(arg, 0);
        return;
    }
    victim = create_mobile(p_mob_proto);
    mob_to_room(victim, ch->in_room);
    return;
}

/*
 * Lets the mobile load an object
 *
 * Syntax: mob oload [vnum] [level] {R}
 */
void do_mpoload(Mobile* ch, char* argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    ObjPrototype* obj_proto;
    Object* obj;
    LEVEL level;
    bool fToroom = false;
    bool fWear = false;

    READ_ARG(arg1);
    READ_ARG(arg2);
    one_argument(argument, arg3);

    if (arg1[0] == '\0' || !is_number(arg1)) {
        bug("Mpoload - Bad syntax from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    if (arg2[0] == '\0') {
        level = get_trust(ch);
    }
    else {
    // New feature from Alander.
        if (!is_number(arg2)) {
            bug("Mpoload - Bad syntax from vnum %"PRVNUM".",
                IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
            return;
        }
        level = (LEVEL)atoi(arg2);
        if (level < 0 || level > get_trust(ch)) {
            bug("Mpoload - Bad level from vnum %"PRVNUM".",
                IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
            return;
        }
    }

    /*
     * Added 3rd argument
     * omitted - load to mobile's inventory
     * 'R'     - load to room
     * 'W'     - load to mobile and force wear
     */
    if (arg3[0] == 'R' || arg3[0] == 'r')
        fToroom = true;
    else if (arg3[0] == 'W' || arg3[0] == 'w')
        fWear = true;

    if ((obj_proto = get_object_prototype(STRTOVNUM(arg1))) == NULL) {
        bug("Mpoload - Bad vnum arg from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    obj = create_object(obj_proto, level);
    if ((fWear || !fToroom) && CAN_WEAR(obj, ITEM_TAKE)) {
        obj_to_char(obj, ch);
        if (fWear)
            wear_obj(ch, obj, true);
    }
    else {
        obj_to_room(obj, ch->in_room);
    }

    return;
}

/*
 * Lets the mobile purge all objects and other npcs in the room,
 * or purge a specified object or mob in the room. The mobile cannot
 * purge itself for safety reasons.
 *
 * syntax mob purge {target}
 */
void do_mppurge(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;
    Object* obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        /* 'purge' */

        FOR_EACH_ROOM_MOB(victim, ch->in_room) {
            if (IS_NPC(victim) && victim != ch
                && !IS_SET(victim->act_flags, ACT_NOPURGE))
                extract_char(victim, true);
        }

        FOR_EACH_ROOM_OBJ(obj, ch->in_room) {
            if (!IS_SET(obj->extra_flags, ITEM_NOPURGE))
                extract_obj(obj);
        }

        return;
    }

    if ((victim = get_mob_room(ch, arg)) == NULL) {
        if ((obj = get_obj_here(ch, arg))) {
            extract_obj(obj);
        }
        else {
            bug("Mppurge - Bad argument from vnum %"PRVNUM".",
                IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        }
        return;
    }

    if (!IS_NPC(victim)) {
        bug("Mppurge - Purging a PC from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }
    extract_char(victim, true);
    return;
}


/*
 * Lets the mobile goto any location it wishes that is not private.
 *
 * Syntax: mob goto [location]
 */
void do_mpgoto(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Room* location;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        bug("Mpgoto - No argument from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    if ((location = find_location(ch, arg)) == NULL) {
        bug("Mpgoto - No such location from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    if (ch->fighting != NULL)
        stop_fighting(ch, true);

    transfer_mob(ch, location);

    return;
}

/*
 * Lets the mobile do a command at another location.
 *
 * Syntax: mob at [location] [commands]
 */
void do_mpat(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Room* location;
    Room* original;
    Mobile* wch;
    Object* on;

    if (ch == NULL)
        return;

    READ_ARG(arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        bug("Mpat - Bad argument from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    if ((location = find_location(ch, arg)) == NULL) {
        bug("Mpat - No such location from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    original = ch->in_room;
    on = ch->on;
    transfer_mob(ch, location);
    interpret(ch, argument);

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    FOR_EACH_GLOBAL_MOB(wch) {
        if (wch == ch) {
            transfer_mob(ch, original);
            ch->on = on;
            break;
        }
    }

    return;
}

/*
 * Lets the mobile transfer people.  The 'all' argument transfers
 *  everyone in the current room to the specified location
 *
 * Syntax: mob transfer [target|'all'] [location]
 */
void do_mptransfer(Mobile* ch, char* argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    Room* location;
    Mobile* victim;

    READ_ARG(arg1);
    READ_ARG(arg2);

    if (arg1[0] == '\0') {
        bug("Mptransfer - Bad syntax from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    if (!str_cmp(arg1, "all")) {
        FOR_EACH_ROOM_MOB(victim, ch->in_room) {
            if (!IS_NPC(victim)) {
                sprintf(buf, "%s %s", NAME_STR(victim), arg2);
                do_mptransfer(ch, buf);
            }
        }
        return;
    }

    // Thanks to Grodyn for the optional location parameter.
    if (arg2[0] == '\0') {
        location = ch->in_room;
    }
    else {
        if ((location = find_location(ch, arg2)) == NULL) {
            bug("Mptransfer - No such location from vnum %"PRVNUM".",
                IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
            return;
        }

        if (room_is_private(location))
            return;
    }

    if ((victim = get_mob_world(ch, arg1)) == NULL)
        return;

    if (victim->in_room == NULL)
        return;

    if (victim->fighting != NULL)
        stop_fighting(victim, true);
    transfer_mob(victim, location);
    do_look(victim, "auto");

    return;
}

/*
 * Lets the mobile transfer all chars in same group as the victim.
 *
 * Syntax: mob gtransfer [victim] [location]
 */
void do_mpgtransfer(Mobile* ch, char* argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    Mobile* who; 
    Mobile* victim;

    READ_ARG(arg1);
    READ_ARG(arg2);

    if (arg1[0] == '\0') {
        bug("Mpgtransfer - Bad syntax from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    if ((who = get_mob_room(ch, arg1)) == NULL)
        return;

    FOR_EACH_ROOM_MOB(victim, ch->in_room) {
        if (is_same_group(who, victim)) {
            sprintf(buf, "%s %s", NAME_STR(victim), arg2);
            do_mptransfer(ch, buf);
        }
    }
    return;
}

/*
 * Lets the mobile force someone to do something. Must be mortal level
 * and the all argument only affects those in the room with the mobile.
 *
 * Syntax: mob force [victim] [commands]
 */
void do_mpforce(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];

    READ_ARG(arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        bug("Mpforce - Bad syntax from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    if (!str_cmp(arg, "all")) {
        Mobile* vch;

        FOR_EACH_GLOBAL_MOB(vch) {
            if (vch->in_room == ch->in_room
                && get_trust(vch) < get_trust(ch)
                && can_see(ch, vch)) {
                interpret(vch, argument);
            }
        }
    }
    else {
        Mobile* victim;

        if ((victim = get_mob_room(ch, arg)) == NULL)
            return;

        if (victim == ch)
            return;

        interpret(victim, argument);
    }

    return;
}

/*
 * Lets the mobile force a group something. Must be mortal level.
 *
 * Syntax: mob gforce [victim] [commands]
 */
void do_mpgforce(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;
    Mobile* vch;

    READ_ARG(arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        bug("MpGforce - Bad syntax from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    if ((victim = get_mob_room(ch, arg)) == NULL)
        return;

    if (victim == ch)
        return;

    FOR_EACH_ROOM_MOB(vch, victim->in_room) {
        if (is_same_group(victim, vch)) {
            interpret(vch, argument);
        }
    }
    return;
}

/*
 * Forces all mobiles of certain vnum to do something (except ch)
 *
 * Syntax: mob vforce [vnum] [commands]
 */
void do_mpvforce(Mobile* ch, char* argument)
{
    Mobile* victim;
    char arg[MAX_INPUT_LENGTH];
    VNUM vnum;

    READ_ARG(arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        bug("MpVforce - Bad syntax from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    if (!is_number(arg)) {
        bug("MpVforce - Non-number argument vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    vnum = STRTOVNUM(arg);

    FOR_EACH_GLOBAL_MOB(victim) {
        if (IS_NPC(victim) && VNUM_FIELD(victim->prototype) == vnum
            && ch != victim && victim->fighting == NULL)
            interpret(victim, argument);
    }
    return;
}

/*
 * Lets the mobile cast spells --
 * Beware: this does only crude checking on the target validity
 * and does not account for mana etc., so you should do all the
 * necessary checking in your mob program before issuing this cmd!
 *
 * Syntax: mob cast [spell] {target}
 */

void do_mpcast(Mobile* ch, char* argument)
{
    Mobile* vch;
    Object* obj;
    void* victim = NULL;
    char arg_spell[MAX_INPUT_LENGTH];
    char arg_target[MAX_INPUT_LENGTH];
    SKNUM sn;
    SpellTarget spell_target = SPELL_TARGET_NONE;

    READ_ARG(arg_spell);
    one_argument(argument, arg_target);

    if (arg_spell[0] == '\0') {
        bug("MpCast - Bad syntax from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }

    if ((sn = skill_lookup(arg_spell)) < 0) {
        bug("MpCast - No such spell from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }
    vch = get_mob_room(ch, arg_target);
    obj = get_obj_here(ch, arg_target);
    switch (skill_table[sn].target) {
    default: 
        return;
    case SKILL_TARGET_IGNORE:
        break;
    case SKILL_TARGET_CHAR_OFFENSIVE:
        if (vch == NULL || vch == ch)
            return;
        victim = (void*)vch;
        spell_target = SPELL_TARGET_CHAR;
        break;
    case SKILL_TARGET_CHAR_DEFENSIVE:
        victim = vch == NULL ? (void*)ch : (void*)vch; 
        spell_target = SPELL_TARGET_CHAR;
        break;
    case SKILL_TARGET_CHAR_SELF:
        victim = (void*)ch; 
        spell_target = SPELL_TARGET_CHAR;
        break;
    case SKILL_TARGET_OBJ_CHAR_DEF:
    case SKILL_TARGET_OBJ_CHAR_OFF:
        if (vch != NULL) {
            victim = (void*)vch;
            spell_target = SPELL_TARGET_CHAR;
            break;
        }
        victim = (void*)obj;
        spell_target = SPELL_TARGET_OBJ;
        break;
    case SKILL_TARGET_OBJ_INV:
        if ((obj = get_obj_carry(ch, arg_target, ch)) == NULL) 
            return;
        victim = (void*)obj;
        spell_target = SPELL_TARGET_OBJ;
    }
    invoke_spell_func(sn, ch->level, ch, victim, spell_target);
    return;
}

/*
 * Lets mob cause unconditional damage to someone. Nasty, use with caution.
 * Also, this is silent, you must show your own damage message...
 *
 * Syntax: mob damage [victim] [min] [max] {kill}
 */
void do_mpdamage(Mobile* ch, char* argument)
{
    Mobile* victim = NULL;
    char target[MAX_INPUT_LENGTH];
    char min[MAX_INPUT_LENGTH];
    char max[MAX_INPUT_LENGTH];
    VNUM low;
    VNUM high;
    bool fAll = false, fKill = false;

    READ_ARG(target);
    READ_ARG(min);
    READ_ARG(max);

    if (target[0] == '\0') {
        bug("MpDamage - Bad syntax from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }
    if (!str_cmp(target, "all"))
        fAll = true;
    else if ((victim = get_mob_room(ch, target)) == NULL)
        return;

    if (is_number(min))
        low = STRTOVNUM(min);
    else {
        bug("MpDamage - Bad damage min vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }
    if (is_number(max))
        high = STRTOVNUM(max);
    else {
        bug("MpDamage - Bad damage max vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }
    one_argument(argument, target);

    /*
     * If kill parameter is omitted, this command is "safe" and will not
     * kill the victim.
     */

    if (target[0] != '\0')
        fKill = true;
    if (fAll) {
        FOR_EACH_ROOM_MOB(victim, ch->in_room) {
            if (victim != ch)
                damage(victim, victim,
                    fKill ?
                    number_range(low, high) : UMIN(victim->hit, number_range(low, high)),
                    TYPE_UNDEFINED, DAM_NONE, false);
        }
    }
    else
        damage(victim, victim,
            fKill ?
            number_range(low, high) : UMIN(victim->hit, number_range(low, high)),
            TYPE_UNDEFINED, DAM_NONE, false);
    return;
}

/*
 * Lets the mobile to remember a target. The target can be referred to
 * with $q and $Q codes in MOBprograms. See also "mob forget".
 *
 * Syntax: mob remember [victim]
 */
void do_mpremember(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);
    if (arg[0] != '\0')
        ch->mprog_target = get_mob_world(ch, arg);
    else
        bug("MpRemember: missing argument from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
}

/*
 * Reverse of "mob remember".
 *
 * Syntax: mob forget
 */
void do_mpforget(Mobile* ch, char* argument)
{
    ch->mprog_target = NULL;
}

/*
 * Sets a delay for MOBprogram execution. When the delay time expires,
 * the mobile is checked for a MObprogram with DELAY trigger, and if
 * one is found, it is executed. Delay is counted in PULSE_MOBILE
 *
 * Syntax: mob delay [pulses]
 */
void do_mpdelay(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (!is_number(arg)) {
        bug("MpDelay: invalid arg from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }
    ch->mprog_delay = (int16_t)atoi(arg);
}

/*
 * Reverse of "mob delay", deactivates the timer.
 *
 * Syntax: mob cancel
 */
void do_mpcancel(Mobile* ch, char* argument)
{
    ch->mprog_delay = -1;
}
/*
 * Lets the mobile to call another MOBprogram withing a MOBprogram.
 * This is a crude way to implement subroutines/functions. Beware of
 * nested loops and unwanted triggerings... Stack usage might be a problem.
 * Characters and objects referred to must be in the same room with the
 * mobile.
 *
 * Syntax: mob call [vnum] [victim|'null'] [object1|'null'] [object2|'null']
 *
 */
void do_mpcall(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* vch;
    Object* obj1, * obj2;
    MobProgCode* prg;

    READ_ARG(arg);
    if (arg[0] == '\0') {
        bug("MpCall: missing arguments from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }
    if ((prg = get_mprog_index(STRTOVNUM(arg))) == NULL) {
        bug("MpCall: invalid prog from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }
    vch = NULL;
    obj1 = obj2 = NULL;
    READ_ARG(arg);
    if (arg[0] != '\0')
        vch = get_mob_room(ch, arg);
    READ_ARG(arg);
    if (arg[0] != '\0')
        obj1 = get_obj_here(ch, arg);
    READ_ARG(arg);
    if (arg[0] != '\0')
        obj2 = get_obj_here(ch, arg);
    program_flow(prg->vnum, prg->code, ch, vch, (void*)obj1, (void*)obj2);
}

/*
 * Forces the mobile to flee.
 *
 * Syntax: mob flee
 *
 */
void do_mpflee(Mobile* ch, char* argument)
{
    Room* was_in;
    RoomExit* room_exit;
    int door, attempt;

    if (ch->fighting != NULL)
        return;

    if ((was_in = ch->in_room) == NULL)
        return;

    for (attempt = 0; attempt < 6; attempt++) {
        door = number_door();
        if ((room_exit = was_in->exit[door]) == 0
            || room_exit->to_room == NULL
            || IS_SET(room_exit->exit_flags, EX_CLOSED)
            || (IS_NPC(ch)
                && IS_SET(room_exit->to_room->data->room_flags, ROOM_NO_MOB)))
            continue;

        move_char(ch, door, false);
        if (ch->in_room != was_in)
            return;
    }
}

/*
 * Lets the mobile to transfer an object. The object must be in the same
 * room with the mobile.
 *
 * Syntax: mob otransfer [item name] [location]
 */
void do_mpotransfer(Mobile* ch, char* argument)
{
    Object* obj;
    Room* location;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];

    READ_ARG(arg);
    if (arg[0] == '\0') {
        bug("MpOTransfer - Missing argument from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }
    one_argument(argument, buf);
    if ((location = find_location(ch, buf)) == NULL) {
        bug("MpOTransfer - No such location from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }
    if ((obj = get_obj_here(ch, arg)) == NULL)
        return;
    if (obj->carried_by == NULL)
        obj_from_room(obj);
    else {
        if (obj->wear_loc != WEAR_UNHELD)
            unequip_char(ch, obj);
        obj_from_char(obj);
    }
    obj_to_room(obj, location);
}

/*
 * Lets the mobile to strip an object or all objects from the victim.
 * Useful for removing e.g. quest objects from a character.
 *
 * Syntax: mob remove [victim] [object vnum|'all']
 */
void do_mpremove(Mobile* ch, char* argument)
{
    Mobile* victim;
    Object* obj;
    VNUM vnum = 0;
    bool fAll = false;
    char arg[MAX_INPUT_LENGTH];

    READ_ARG(arg);
    if ((victim = get_mob_room(ch, arg)) == NULL)
        return;

    one_argument(argument, arg);
    if (!str_cmp(arg, "all"))
        fAll = true;
    else if (!is_number(arg)) {
        bug("MpRemove: Invalid object from vnum %"PRVNUM".",
            IS_NPC(ch) ? VNUM_FIELD(ch->prototype) : 0);
        return;
    }
    else
        vnum = STRTOVNUM(arg);

    FOR_EACH_MOB_OBJ(obj, victim) {
        if (fAll || VNUM_FIELD(obj->prototype) == vnum) {
            unequip_char(ch, obj);
            obj_from_char(obj);
            extract_obj(obj);
        }
    }
}

void do_mpquest(Mobile* ch, char* argument)
{
    char cmd[MIL];
    char name[MIL];
    char vnum_str[MIL];
    Mobile* vch;

    READ_ARG(cmd);
    if (!cmd[0])
        return;

    READ_ARG(name);
    if ((vch = get_mob_room(ch, name)) == NULL)
        return;

    if (!vch->pcdata)
        return;

    READ_ARG(vnum_str);
    if (!vnum_str[0] || !is_number(vnum_str))
        return;

    VNUM vnum = STRTOVNUM(vnum_str);

    Quest* q = get_quest(vnum);
    QuestStatus* qs = get_quest_status(vch, vnum);

    if (!str_cmp(cmd, "grant")) {
        if (qs)
            return;
        grant_quest(vch, q);
    }
    else if (!str_cmp(cmd, "finish")) {
        if (!qs)
            return;
        finish_quest(vch, q, qs);
    }
}
