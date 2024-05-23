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

#include "act_comm.h"

#include "merc.h"

#include "act_wiz.h"
#include "color.h"
#include "comm.h"
#include "config.h"
#include "db.h"
#include "fight.h"
#include "handler.h"
#include "interp.h"
#include "mob_prog.h"
#include "recycle.h"
#include "save.h"
#include "tables.h"
#include "vt.h"

#include "olc/screen.h"

#include "entities/descriptor.h"
#include "entities/player_data.h"

#include "data/class.h"
#include "data/mobile_data.h"
#include "data/player.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifndef _MSC_VER 
#include <sys/time.h>
#include <unistd.h>
#else
#define unlink _unlink
#endif

/* RT code to delete yourself */

void do_delet(Mobile* ch, char* argument)
{
    send_to_char("You must type the full command to delete yourself.\n\r", ch);
}

void do_delete(Mobile* ch, char* argument)
{
    char strsave[MAX_INPUT_LENGTH];

    if (IS_NPC(ch)) return;

    if (ch->pcdata->confirm_delete) {
        if (argument[0] != '\0') {
            send_to_char("Delete status removed.\n\r", ch);
            ch->pcdata->confirm_delete = false;
            return;
        }
        else {
            sprintf(strsave, "%s%s", cfg_get_player_dir(), capitalize(NAME_STR(ch)));
            wiznet("$N turns $Mself into line noise.", ch, NULL, 0, 0, 0);
            stop_fighting(ch, true);
            do_function(ch, &do_quit, "");
            unlink(strsave);
            return;
        }
    }

    if (argument[0] != '\0') {
        send_to_char("Just type delete. No argument.\n\r", ch);
        return;
    }

    send_to_char("Type delete again to confirm this command.\n\r", ch);
    send_to_char("WARNING: this command is irreversible.\n\r", ch);
    send_to_char("Typing delete with an argument will undo delete status.\n\r",
                 ch);
    ch->pcdata->confirm_delete = true;
    wiznet("$N is contemplating deletion.", ch, NULL, 0, 0, get_trust(ch));
}

/* RT code to display channel status */

void do_channels(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];

    /* lists all channels and their status */
    send_to_char("{T   channel     status\n\r", ch);
    send_to_char("{=---------------------\n\r", ch);

    send_to_char("{dgossip{x         ", ch);
    if (!IS_SET(ch->comm_flags, COMM_NOGOSSIP))
        send_to_char("{GON{x\n\r", ch);
    else
        send_to_char("{ROFF{x\n\r", ch);

    send_to_char("{aauction{x        ", ch);
    if (!IS_SET(ch->comm_flags, COMM_NOAUCTION))
        send_to_char("{GON{x\n\r", ch);
    else
        send_to_char("{ROFF{x\n\r", ch);

    send_to_char("{emusic{x          ", ch);
    if (!IS_SET(ch->comm_flags, COMM_NOMUSIC))
        send_to_char("{GON{x\n\r", ch);
    else
        send_to_char("{ROFF{x\n\r", ch);

    send_to_char("{qQ{x/{fA{x            ", ch);
    if (!IS_SET(ch->comm_flags, COMM_NOQUESTION))
        send_to_char("{GON{x\n\r", ch);
    else
        send_to_char("{ROFF{x\n\r", ch);

    send_to_char("{hQuote{x          ", ch);
    if (!IS_SET(ch->comm_flags, COMM_NOQUOTE))
        send_to_char("{GON{x\n\r", ch);
    else
        send_to_char("{ROFF{x\n\r", ch);

    send_to_char("{tgrats{x          ", ch);
    if (!IS_SET(ch->comm_flags, COMM_NOGRATS))
        send_to_char("{GON{x\n\r", ch);
    else
        send_to_char("{ROFF{x\n\r", ch);

    if (IS_IMMORTAL(ch)) {
        send_to_char("{igod channel{x    ", ch);
        if (!IS_SET(ch->comm_flags, COMM_NOWIZ))
            send_to_char("{GON{x\n\r", ch);
        else
            send_to_char("{ROFF{x\n\r", ch);
    }

    send_to_char("{tshouts{x         ", ch);
    if (!IS_SET(ch->comm_flags, COMM_SHOUTSOFF))
        send_to_char("{GON{x\n\r", ch);
    else
        send_to_char("{ROFF{x\n\r", ch);

    send_to_char("{ktells{x          ", ch);
    if (!IS_SET(ch->comm_flags, COMM_DEAF))
        send_to_char("{GON{x\n\r", ch);
    else
        send_to_char("{ROFF{x\n\r", ch);

    send_to_char("{tquiet mode{x     ", ch);
    if (IS_SET(ch->comm_flags, COMM_QUIET))
        send_to_char("{GON{x\n\r", ch);
    else
        send_to_char("{ROFF{x\n\r", ch);

    if (IS_SET(ch->comm_flags, COMM_AFK)) 
        send_to_char("You are AFK.\n\r", ch);

    if (IS_SET(ch->comm_flags, COMM_SNOOP_PROOF))
        send_to_char("You are immune to snooping.\n\r", ch);

    if (ch->lines != PAGELEN) {
        if (ch->lines) {
            sprintf(buf, "You display %d lines of scroll.\n\r", ch->lines + 2);
            send_to_char(buf, ch);
        }
        else
            send_to_char("Scroll buffering is off.\n\r", ch);
    }

    if (ch->prompt != NULL) {
        sprintf(buf, "Your current prompt is: %s\n\r", ch->prompt);
        send_to_char(buf, ch);
    }

    if (IS_SET(ch->comm_flags, COMM_NOSHOUT))
        send_to_char("You cannot shout.\n\r", ch);

    if (IS_SET(ch->comm_flags, COMM_NOTELL))
        send_to_char("You cannot use tell.\n\r", ch);

    if (IS_SET(ch->comm_flags, COMM_NOCHANNELS))
        send_to_char("You cannot use channels.\n\r", ch);

    if (IS_SET(ch->comm_flags, COMM_NOEMOTE))
        send_to_char("You cannot show emotions.\n\r", ch);
}

/* RT deaf blocks out all shouts */

void do_deaf(Mobile* ch, char* argument)
{
    if (IS_SET(ch->comm_flags, COMM_DEAF)) {
        send_to_char("You can now hear tells again.\n\r", ch);
        REMOVE_BIT(ch->comm_flags, COMM_DEAF);
    }
    else {
        send_to_char("From now on, you won't hear tells.\n\r", ch);
        SET_BIT(ch->comm_flags, COMM_DEAF);
    }
}

/* RT quiet blocks out all communication */

void do_quiet(Mobile* ch, char* argument)
{
    if (IS_SET(ch->comm_flags, COMM_QUIET)) {
        send_to_char("Quiet mode removed.\n\r", ch);
        REMOVE_BIT(ch->comm_flags, COMM_QUIET);
    }
    else {
        send_to_char("From now on, you will only hear says and emotes.\n\r",
                     ch);
        SET_BIT(ch->comm_flags, COMM_QUIET);
    }
}

/* afk command */

void do_afk(Mobile* ch, char* argument)
{
    if (IS_SET(ch->comm_flags, COMM_AFK)) {
        send_to_char("AFK mode removed. Type 'replay' to see tells.\n\r", ch);
        REMOVE_BIT(ch->comm_flags, COMM_AFK);
    }
    else {
        send_to_char("You are now in AFK mode.\n\r", ch);
        SET_BIT(ch->comm_flags, COMM_AFK);
    }
}

void do_replay(Mobile* ch, char* argument)
{
    if (IS_NPC(ch)) {
        send_to_char("You can't replay.\n\r", ch);
        return;
    }

    if (BUF(ch->pcdata->buffer)[0] == '\0') {
        send_to_char("You have no tells to replay.\n\r", ch);
        return;
    }

    page_to_char(BUF(ch->pcdata->buffer), ch);
    clear_buf(ch->pcdata->buffer);
}

/* RT auction rewritten in ROM style */
void do_auction(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    Descriptor* d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm_flags, COMM_NOAUCTION)) {
            send_to_char("{aAuction channel is now ON.{x\n\r", ch);
            REMOVE_BIT(ch->comm_flags, COMM_NOAUCTION);
        }
        else {
            send_to_char("{aAuction channel is now OFF.{x\n\r", ch);
            SET_BIT(ch->comm_flags, COMM_NOAUCTION);
        }
    }
    else /* auction message sent, turn auction on if it is off */
    {
        if (IS_SET(ch->comm_flags, COMM_QUIET)) {
            send_to_char("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET(ch->comm_flags, COMM_NOCHANNELS)) {
            send_to_char("The gods have revoked your channel priviliges.\n\r",
                         ch);
            return;
        }

        REMOVE_BIT(ch->comm_flags, COMM_NOAUCTION);
    }

    sprintf(buf, "{aYou auction '{A%s{a'{x\n\r", argument);
    send_to_char(buf, ch);
    FOR_EACH(d, descriptor_list) {
        Mobile* victim;

        victim = d->original ? d->original : d->character;

        if (d->connected == CON_PLAYING && d->character != ch
            && !IS_SET(victim->comm_flags, COMM_NOAUCTION)
            && !IS_SET(victim->comm_flags, COMM_QUIET)) {
            act_new("{a$n auctions '{A$t{a'{x", ch, argument, d->character,
                    TO_VICT, POS_DEAD);
        }
    }
}

/* RT chat replaced with ROM gossip */
void do_gossip(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    Descriptor* d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm_flags, COMM_NOGOSSIP)) {
            send_to_char("Gossip channel is now ON.\n\r", ch);
            REMOVE_BIT(ch->comm_flags, COMM_NOGOSSIP);
        }
        else {
            send_to_char("Gossip channel is now OFF.\n\r", ch);
            SET_BIT(ch->comm_flags, COMM_NOGOSSIP);
        }
    }
    else /* gossip message sent, turn gossip on if it isn't already */
    {
        if (IS_SET(ch->comm_flags, COMM_QUIET)) {
            send_to_char("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET(ch->comm_flags, COMM_NOCHANNELS)) {
            send_to_char("The gods have revoked your channel priviliges.\n\r",
                         ch);
            return;
        }

        REMOVE_BIT(ch->comm_flags, COMM_NOGOSSIP);

        sprintf(buf, "{dYou gossip '{9%s{d'{x\n\r", argument);
        send_to_char(buf, ch);
        FOR_EACH(d, descriptor_list) {
            Mobile* victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING && d->character != ch
                && !IS_SET(victim->comm_flags, COMM_NOGOSSIP)
                && !IS_SET(victim->comm_flags, COMM_QUIET)) {
                act_new("{d$n gossips '{9$t{d'{x", ch, argument, d->character,
                        TO_VICT, POS_SLEEPING);
            }
        }
    }
}

void do_grats(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    Descriptor* d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm_flags, COMM_NOGRATS)) {
            send_to_char("Grats channel is now ON.\n\r", ch);
            REMOVE_BIT(ch->comm_flags, COMM_NOGRATS);
        }
        else {
            send_to_char("Grats channel is now OFF.\n\r", ch);
            SET_BIT(ch->comm_flags, COMM_NOGRATS);
        }
    }
    else /* grats message sent, turn grats on if it isn't already */
    {
        if (IS_SET(ch->comm_flags, COMM_QUIET)) {
            send_to_char("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET(ch->comm_flags, COMM_NOCHANNELS)) {
            send_to_char("The gods have revoked your channel priviliges.\n\r",
                         ch);
            return;
        }

        REMOVE_BIT(ch->comm_flags, COMM_NOGRATS);

        sprintf(buf, "{tYou grats '%s'{x\n\r", argument);
        send_to_char(buf, ch);
        FOR_EACH(d, descriptor_list) {
            Mobile* victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING && d->character != ch
                && !IS_SET(victim->comm_flags, COMM_NOGRATS)
                && !IS_SET(victim->comm_flags, COMM_QUIET)) {
                act_new("{t$n grats '$t'{x", ch, argument, d->character,
                        TO_VICT, POS_SLEEPING);
            }
        }
    }
}

void do_quote(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    Descriptor* d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm_flags, COMM_NOQUOTE)) {
            send_to_char("{hQuote channel is now ON.{x\n\r", ch);
            REMOVE_BIT(ch->comm_flags, COMM_NOQUOTE);
        }
        else {
            send_to_char("{hQuote channel is now OFF.{x\n\r", ch);
            SET_BIT(ch->comm_flags, COMM_NOQUOTE);
        }
    }
    else /* quote message sent, turn quote on if it isn't already */
    {
        if (IS_SET(ch->comm_flags, COMM_QUIET)) {
            send_to_char("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET(ch->comm_flags, COMM_NOCHANNELS)) {
            send_to_char("The gods have revoked your channel priviliges.\n\r",
                         ch);
            return;
        }

        REMOVE_BIT(ch->comm_flags, COMM_NOQUOTE);

        sprintf(buf, "{hYou quote '{H%s{h'{x\n\r", argument);
        send_to_char(buf, ch);
        FOR_EACH(d, descriptor_list) {
            Mobile* victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING && d->character != ch
                && !IS_SET(victim->comm_flags, COMM_NOQUOTE)
                && !IS_SET(victim->comm_flags, COMM_QUIET)) {
                act_new("{h$n quotes '{H$t{h'{x", ch, argument, d->character,
                        TO_VICT, POS_SLEEPING);
            }
        }
    }
}

/* RT question channel */
void do_question(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    Descriptor* d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm_flags, COMM_NOQUESTION)) {
            send_to_char("Q/A channel is now ON.\n\r", ch);
            REMOVE_BIT(ch->comm_flags, COMM_NOQUESTION);
        }
        else {
            send_to_char("Q/A channel is now OFF.\n\r", ch);
            SET_BIT(ch->comm_flags, COMM_NOQUESTION);
        }
    }
    else /* question sent, turn Q/A on if it isn't already */
    {
        if (IS_SET(ch->comm_flags, COMM_QUIET)) {
            send_to_char("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET(ch->comm_flags, COMM_NOCHANNELS)) {
            send_to_char("The gods have revoked your channel priviliges.\n\r",
                         ch);
            return;
        }

        REMOVE_BIT(ch->comm_flags, COMM_NOQUESTION);

        sprintf(buf, "{qYou question '{Q%s{q'{x\n\r", argument);
        send_to_char(buf, ch);
        FOR_EACH(d, descriptor_list) {
            Mobile* victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING && d->character != ch
                && !IS_SET(victim->comm_flags, COMM_NOQUESTION)
                && !IS_SET(victim->comm_flags, COMM_QUIET)) {
                act_new("{q$n questions '{Q$t{q'{x", ch, argument, d->character,
                        TO_VICT, POS_SLEEPING);
            }
        }
    }
}

/* RT answer channel - uses same line as questions */
void do_answer(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    Descriptor* d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm_flags, COMM_NOQUESTION)) {
            send_to_char("Q/A channel is now ON.\n\r", ch);
            REMOVE_BIT(ch->comm_flags, COMM_NOQUESTION);
        }
        else {
            send_to_char("Q/A channel is now OFF.\n\r", ch);
            SET_BIT(ch->comm_flags, COMM_NOQUESTION);
        }
    }
    else /* answer sent, turn Q/A on if it isn't already */
    {
        if (IS_SET(ch->comm_flags, COMM_QUIET)) {
            send_to_char("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET(ch->comm_flags, COMM_NOCHANNELS)) {
            send_to_char("The gods have revoked your channel priviliges.\n\r",
                         ch);
            return;
        }

        REMOVE_BIT(ch->comm_flags, COMM_NOQUESTION);

        sprintf(buf, "{fYou answer '{F%s{f'{x\n\r", argument);
        send_to_char(buf, ch);
        FOR_EACH(d, descriptor_list) {
            Mobile* victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING && d->character != ch
                && !IS_SET(victim->comm_flags, COMM_NOQUESTION)
                && !IS_SET(victim->comm_flags, COMM_QUIET)) {
                act_new("{f$n answers '{F$t{f'{x", ch, argument, d->character,
                        TO_VICT, POS_SLEEPING);
            }
        }
    }
}

/* RT music channel */
void do_music(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    Descriptor* d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm_flags, COMM_NOMUSIC)) {
            send_to_char("Music channel is now ON.\n\r", ch);
            REMOVE_BIT(ch->comm_flags, COMM_NOMUSIC);
        }
        else {
            send_to_char("Music channel is now OFF.\n\r", ch);
            SET_BIT(ch->comm_flags, COMM_NOMUSIC);
        }
    }
    else /* music sent, turn music on if it isn't already */
    {
        if (IS_SET(ch->comm_flags, COMM_QUIET)) {
            send_to_char("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET(ch->comm_flags, COMM_NOCHANNELS)) {
            send_to_char("The gods have revoked your channel priviliges.\n\r",
                         ch);
            return;
        }

        REMOVE_BIT(ch->comm_flags, COMM_NOMUSIC);

        sprintf(buf, "{eYou MUSIC: '{E%s{e'{x\n\r", argument);
        send_to_char(buf, ch);
        sprintf(buf, "$n MUSIC: '%s'", argument);
        FOR_EACH(d, descriptor_list) {
            Mobile* victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING && d->character != ch
                && !IS_SET(victim->comm_flags, COMM_NOMUSIC)
                && !IS_SET(victim->comm_flags, COMM_QUIET)) {
                act_new("{e$n MUSIC: '{E$t{e'{x", ch, argument, d->character,
                        TO_VICT, POS_SLEEPING);
            }
        }
    }
}

/* clan channels */
void do_clantalk(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    Descriptor* d;

    if (!is_clan(ch) || clan_table[ch->clan].independent) {
        send_to_char("You aren't in a clan.\n\r", ch);
        return;
    }
    if (argument[0] == '\0') {
        if (IS_SET(ch->comm_flags, COMM_NOCLAN)) {
            send_to_char("Clan channel is now ON\n\r", ch);
            REMOVE_BIT(ch->comm_flags, COMM_NOCLAN);
        }
        else {
            send_to_char("Clan channel is now OFF\n\r", ch);
            SET_BIT(ch->comm_flags, COMM_NOCLAN);
        }
        return;
    }

    if (IS_SET(ch->comm_flags, COMM_NOCHANNELS)) {
        send_to_char("The gods have revoked your channel priviliges.\n\r", ch);
        return;
    }

    REMOVE_BIT(ch->comm_flags, COMM_NOCLAN);

    sprintf(buf, "You clan '%s'{x\n\r", argument);
    send_to_char(buf, ch);
    sprintf(buf, "$n clans '%s'", argument);
    FOR_EACH(d, descriptor_list) {
        if (d->connected == CON_PLAYING && d->character != ch
            && is_same_clan(ch, d->character)
            && !IS_SET(d->character->comm_flags, COMM_NOCLAN)
            && !IS_SET(d->character->comm_flags, COMM_QUIET)) {
            act_new("$n clans '$t'{x", ch, argument, d->character, TO_VICT,
                    POS_DEAD);
        }
    }

    return;
}

void do_immtalk(Mobile* ch, char* argument)
{
    Descriptor* d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm_flags, COMM_NOWIZ)) {
            send_to_char("Immortal channel is now ON\n\r", ch);
            REMOVE_BIT(ch->comm_flags, COMM_NOWIZ);
        }
        else {
            send_to_char("Immortal channel is now OFF\n\r", ch);
            SET_BIT(ch->comm_flags, COMM_NOWIZ);
        }
        return;
    }

    REMOVE_BIT(ch->comm_flags, COMM_NOWIZ);

    act_new("{i[{I$n{i]: $t{x", ch, argument, NULL, TO_CHAR, POS_DEAD);
    FOR_EACH(d, descriptor_list) {
        if (d->connected == CON_PLAYING && IS_IMMORTAL(d->character)
            && !IS_SET(d->character->comm_flags, COMM_NOWIZ)) {
            act_new("{i[{I$n{i]: $t{x", ch, argument, d->character, TO_VICT,
                    POS_DEAD);
        }
    }

    return;
}

void do_say(Mobile* ch, char* argument)
{
    if (argument[0] == '\0') {
        send_to_char("Say what?\n\r", ch);
        return;
    }

    act("{6$n says '{7$T{6'{x", ch, NULL, argument, TO_ROOM);
    act("{6You say '{7$T{6'{x", ch, NULL, argument, TO_CHAR);


    if (!IS_NPC(ch)) {
        Mobile* mob = NULL;
        FOR_EACH_ROOM_MOB(mob, ch->in_room) {
            if (IS_NPC(mob) && HAS_TRIGGER(mob, TRIG_SPEECH)
                && mob->position == mob->prototype->default_pos)
                mp_act_trigger(argument, mob, ch, NULL, NULL, TRIG_SPEECH);
        }
    }

    return;
}

// TODO: Make area-wide only. Add OOC channel? Pray? Whisper?
void do_shout(Mobile* ch, char* argument)
{
    Descriptor* d;

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm_flags, COMM_SHOUTSOFF)) {
            send_to_char("You can hear shouts again.\n\r", ch);
            REMOVE_BIT(ch->comm_flags, COMM_SHOUTSOFF);
        }
        else {
            send_to_char("You will no longer hear shouts.\n\r", ch);
            SET_BIT(ch->comm_flags, COMM_SHOUTSOFF);
        }
        return;
    }

    if (IS_SET(ch->comm_flags, COMM_NOSHOUT)) {
        send_to_char("You can't shout.\n\r", ch);
        return;
    }

    REMOVE_BIT(ch->comm_flags, COMM_SHOUTSOFF);

    WAIT_STATE(ch, 12);

    act("{dYou shout '{9$T{d'{x", ch, NULL, argument, TO_CHAR);
    FOR_EACH(d, descriptor_list) {
        Mobile* victim;

        victim = d->original ? d->original : d->character;

        if (d->connected == CON_PLAYING && d->character != ch
            && !IS_SET(victim->comm_flags, COMM_SHOUTSOFF)
            && !IS_SET(victim->comm_flags, COMM_QUIET)) {
            act("{d$n shouts '{9$t{d'{x", ch, argument, d->character, TO_VICT);
        }
    }

    return;
}

void do_tell(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    Mobile* victim;

    if (IS_SET(ch->comm_flags, COMM_NOTELL) || IS_SET(ch->comm_flags, COMM_DEAF)) {
        send_to_char("Your message didn't get through.\n\r", ch);
        return;
    }

    if (IS_SET(ch->comm_flags, COMM_QUIET)) {
        send_to_char("You must turn off quiet mode first.\n\r", ch);
        return;
    }

    if (IS_SET(ch->comm_flags, COMM_DEAF)) {
        send_to_char("You must turn off deaf mode first.\n\r", ch);
        return;
    }

    READ_ARG(arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        send_to_char("Tell whom what?\n\r", ch);
        return;
    }

    /*
     * Can tell to PC's anywhere, but NPC's only in same room.
     * -- Furey
     */
    if ((victim = get_mob_world(ch, arg)) == NULL
        || (IS_NPC(victim) && victim->in_room != ch->in_room)) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->desc == NULL && !IS_NPC(victim)) {
        act("$N seems to have misplaced $S link...try again later.", ch, NULL,
            victim, TO_CHAR);
        sprintf(buf, "{k%s tells you '{K%s{k'{x\n\r", PERS(ch, victim),
                argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer, buf);
        return;
    }

    if (!(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) && !IS_AWAKE(victim)) {
        act("$E can't hear you.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if ((IS_SET(victim->comm_flags, COMM_QUIET) || IS_SET(victim->comm_flags, COMM_DEAF))
        && !IS_IMMORTAL(ch)) {
        act("$E is not receiving tells.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (IS_SET(victim->comm_flags, COMM_AFK)) {
        if (IS_NPC(victim)) {
            act("$E is AFK, and not receiving tells.", ch, NULL, victim,
                TO_CHAR);
            return;
        }

        act("$E is AFK, but your tell will go through when $E returns.", ch,
            NULL, victim, TO_CHAR);
        sprintf(buf, "{k%s tells you '{K%s{k'{x\n\r", PERS(ch, victim),
                argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer, buf);
        return;
    }

    act("{kYou tell $N '{K$t{k'{x", ch, argument, victim, TO_CHAR);
    act_new("{k$n tells you '{K$t{k'{x", ch, argument, victim, TO_VICT,
            POS_DEAD);
    victim->reply = ch;

    if (!IS_NPC(ch) && IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_SPEECH))
        mp_act_trigger(argument, victim, ch, NULL, NULL, TRIG_SPEECH);

    return;
}

void do_reply(Mobile* ch, char* argument)
{
    Mobile* victim;
    char buf[MAX_STRING_LENGTH];

    if (IS_SET(ch->comm_flags, COMM_NOTELL)) {
        send_to_char("Your message didn't get through.\n\r", ch);
        return;
    }

    if ((victim = ch->reply) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->desc == NULL && !IS_NPC(victim)) {
        act("$N seems to have misplaced $S link...try again later.", ch, NULL,
            victim, TO_CHAR);
        sprintf(buf, "{k%s tells you '{K%s{k'{x\n\r", PERS(ch, victim),
                argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer, buf);
        return;
    }

    if (!IS_IMMORTAL(ch) && !IS_AWAKE(victim)) {
        act("$E can't hear you.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if ((IS_SET(victim->comm_flags, COMM_QUIET) || IS_SET(victim->comm_flags, COMM_DEAF))
        && !IS_IMMORTAL(ch) && !IS_IMMORTAL(victim)) {
        act_new("$E is not receiving tells.", ch, NULL, victim, TO_CHAR, POS_DEAD);
        return;
    }

    if (!IS_IMMORTAL(victim) && !IS_AWAKE(ch)) {
        send_to_char("In your dreams, or what?\n\r", ch);
        return;
    }

    if (IS_SET(victim->comm_flags, COMM_AFK)) {
        if (IS_NPC(victim)) {
            act_new("$E is AFK, and not receiving tells.", ch, NULL, victim,
                    TO_CHAR, POS_DEAD);
            return;
        }

        act_new("$E is AFK, but your tell will go through when $E returns.", ch,
                NULL, victim, TO_CHAR, POS_DEAD);
        sprintf(buf, "{k%s tells you '{K%s{k'{x\n\r", PERS(ch, victim),
                argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer, buf);
        return;
    }

    act_new("{kYou tell $N '{K$t{k'{x", ch, argument, victim, TO_CHAR,
            POS_DEAD);
    act_new("{k$n tells you '{K$t{k'{x", ch, argument, victim, TO_VICT,
            POS_DEAD);
    victim->reply = ch;

    return;
}

void do_yell(Mobile* ch, char* argument)
{
    Descriptor* d;

    if (IS_SET(ch->comm_flags, COMM_NOSHOUT)) {
        send_to_char("You can't yell.\n\r", ch);
        return;
    }

    if (argument[0] == '\0') {
        send_to_char("Yell what?\n\r", ch);
        return;
    }

    act("{dYou yell '{9$t{d'{x", ch, argument, NULL, TO_CHAR);
    FOR_EACH(d, descriptor_list) {
        if (d->connected == CON_PLAYING && d->character != ch
            && d->character->in_room != NULL
            && d->character->in_room->area == ch->in_room->area
            && !IS_SET(d->character->comm_flags, COMM_QUIET)) {
            act("$n yells '$t'", ch, argument, d->character, TO_VICT);
        }
    }

    return;
}

void do_emote(Mobile* ch, char* argument)
{
    if (!IS_NPC(ch) && IS_SET(ch->comm_flags, COMM_NOEMOTE)) {
        send_to_char("You can't show your emotions.\n\r", ch);
        return;
    }

    if (argument[0] == '\0') {
        send_to_char("Emote what?\n\r", ch);
        return;
    }

    MOBtrigger = false;
    act("$n $T", ch, NULL, argument, TO_ROOM);
    act("$n $T", ch, NULL, argument, TO_CHAR);
    MOBtrigger = true;
    return;
}

void do_pmote(Mobile* ch, char* argument)
{
    Mobile* vch;
    char *letter;
    char last[MAX_INPUT_LENGTH] = "";
    char temp[MAX_STRING_LENGTH] = "";
    int matches = 0;

    if (!IS_NPC(ch) && IS_SET(ch->comm_flags, COMM_NOEMOTE)) {
        send_to_char("You can't show your emotions.\n\r", ch);
        return;
    }

    if (argument[0] == '\0') {
        send_to_char("Emote what?\n\r", ch);
        return;
    }

    act("$n $t", ch, argument, NULL, TO_CHAR);

    FOR_EACH_ROOM_MOB(vch, ch->in_room) {
        if (vch->desc == NULL || vch == ch) continue;

        if ((letter = strstr(argument, NAME_STR(vch))) == NULL) {
            MOBtrigger = false;
            act("$N $t", vch, argument, ch, TO_CHAR);
            MOBtrigger = true;
            continue;
        }

        strcpy(temp, argument);
        temp[strlen(argument) - strlen(letter)] = '\0';
        last[0] = '\0';
        char* name = NAME_STR(vch);
        String* vch_name = NAME_FIELD(vch);

        for (; *letter != '\0'; letter++) {
            if (*letter == '\'' && matches == vch_name->length) {
                strcat(temp, "r");
                continue;
            }

            if (*letter == 's' && matches == vch_name->length) {
                matches = 0;
                continue;
            }

            if (matches == vch_name->length) { matches = 0; }

            if (*letter == *name) {
                matches++;
                name++;
                if (matches == vch_name->length) {
                    strcat(temp, "you");
                    last[0] = '\0';
                    name = NAME_STR(vch);
                    continue;
                }
                strncat(last, letter, 1);
                continue;
            }

            matches = 0;
            strcat(temp, last);
            strncat(temp, letter, 1);
            last[0] = '\0';
            name = NAME_STR(vch);
        }

        MOBtrigger = false;
        act("$N $t", vch, temp, ch, TO_CHAR);
        MOBtrigger = true;
    }

    return;
}

// All the posing stuff.
struct pose_table_type {
    char* message[2 * 4];
};

const struct pose_table_type pose_table[] = {
    {{"You sizzle with energy.", "$n sizzles with energy.",
      "You feel very holy.", "$n looks very holy.",
      "You perform a small card trick.", "$n performs a small card trick.",
      "You show your bulging muscles.", "$n shows $s bulging muscles."}},

    {{"You turn into a butterfly, then return to your normal shape.",
      "$n turns into a butterfly, then returns to $s normal shape.",
      "You nonchalantly turn wine into water.",
      "$n nonchalantly turns wine into water.",
      "You wiggle your ears alternately.", "$n wiggles $s ears alternately.",
      "You crack nuts between your fingers.",
      "$n cracks nuts between $s fingers."}},

    {{"Blue sparks fly from your fingers.",
      "Blue sparks fly from $n's fingers.", "A halo appears over your head.",
      "A halo appears over $n's head.", "You nimbly tie yourself into a knot.",
      "$n nimbly ties $mself into a knot.",
      "You grizzle your teeth and look mean.",
      "$n grizzles $s teeth and looks mean."}},

    {{"Little red lights dance in your eyes.",
      "Little red lights dance in $n's eyes.", "You recite words of wisdom.",
      "$n recites words of wisdom.",
      "You juggle with daggers, apples, and eyeballs.",
      "$n juggles with daggers, apples, and eyeballs.",
      "You hit your head, and your eyes roll.",
      "$n hits $s head, and $s eyes roll."}},

    {{"A slimy green monster appears before you and bows.",
      "A slimy green monster appears before $n and bows.",
      "Deep in prayer, you levitate.", "Deep in prayer, $n levitates.",
      "You steal the underwear off every person in the room.",
      "Your underwear is gone!  $n stole it!",
      "Crunch, crunch -- you munch a bottle.",
      "Crunch, crunch -- $n munches a bottle."}},

    {{"You turn everybody into a little pink elephant.",
      "You are turned into a little pink elephant by $n.",
      "An angel consults you.", "An angel consults $n.",
      "The dice roll ... and you win again.",
      "The dice roll ... and $n wins again.",
      "... 98, 99, 100 ... you do pushups.",
      "... 98, 99, 100 ... $n does pushups."}},

    {{"A small ball of light dances on your fingertips.",
      "A small ball of light dances on $n's fingertips.",
      "Your body glows with an unearthly light.",
      "$n's body glows with an unearthly light.",
      "You count the money in everyone's pockets.",
      "Check your money, $n is counting it.",
      "Arnold Schwarzenegger admires your physique.",
      "Arnold Schwarzenegger admires $n's physique."}},

    {{"Smoke and fumes leak from your nostrils.",
      "Smoke and fumes leak from $n's nostrils.", "A spot light hits you.",
      "A spot light hits $n.", "You balance a pocket knife on your tongue.",
      "$n balances a pocket knife on your tongue.",
      "Watch your feet, you are juggling granite boulders.",
      "Watch your feet, $n is juggling granite boulders."}},

    {{"The light flickers as you rap in magical languages.",
      "The light flickers as $n raps in magical languages.",
      "Everyone levitates as you pray.", "You levitate as $n prays.",
      "You produce a coin from everyone's ear.",
      "$n produces a coin from your ear.",
      "Oomph!  You squeeze water out of a granite boulder.",
      "Oomph!  $n squeezes water out of a granite boulder."}},

    {{"Your head disappears.", "$n's head disappears.",
      "A cool breeze refreshes you.", "A cool breeze refreshes $n.",
      "You step behind your shadow.", "$n steps behind $s shadow.",
      "You pick your teeth with a spear.", "$n picks $s teeth with a spear."}},

    {{"A fire elemental singes your hair.",
      "A fire elemental singes $n's hair.",
      "The sun pierces through the clouds to illuminate you.",
      "The sun pierces through the clouds to illuminate $n.",
      "Your eyes dance with greed.", "$n's eyes dance with greed.",
      "Everyone is swept off their foot by your hug.",
      "You are swept off your feet by $n's hug."}},

    {{"The sky changes color to match your eyes.",
      "The sky changes color to match $n's eyes.",
      "The ocean parts before you.", "The ocean parts before $n.",
      "You deftly steal everyone's weapon.", "$n deftly steals your weapon.",
      "Your karate chop splits a tree.", "$n's karate chop splits a tree."}},

    {{"The stones dance to your command.", "The stones dance to $n's command.",
      "A thunder cloud kneels to you.", "A thunder cloud kneels to $n.",
      "The Grey Mouser buys you a beer.", "The Grey Mouser buys $n a beer.",
      "A strap of your armor breaks over your mighty thews.",
      "A strap of $n's armor breaks over $s mighty thews."}},

    {{"The heavens and grass change colour as you smile.",
      "The heavens and grass change colour as $n smiles.",
      "The Burning Man speaks to you.", "The Burning Man speaks to $n.",
      "Everyone's pocket explodes with your fireworks.",
      "Your pocket explodes with $n's fireworks.",
      "A boulder cracks at your frown.", "A boulder cracks at $n's frown."}},

    {{"Everyone's clothes are transparent, and you are laughing.",
      "Your clothes are transparent, and $n is laughing.",
      "An eye in a pyramid winks at you.", "An eye in a pyramid winks at $n.",
      "Everyone discovers your dagger a centimeter from their eye.",
      "You discover $n's dagger a centimeter from your eye.",
      "Mercenaries arrive to do your bidding.",
      "Mercenaries arrive to do $n's bidding."}},

    {{"A black hole swallows you.", "A black hole swallows $n.",
      "Valentine Michael Smith offers you a glass of water.",
      "Valentine Michael Smith offers $n a glass of water.",
      "Where did you go?", "Where did $n go?",
      "Four matched Percherons bring in your chariot.",
      "Four matched Percherons bring in $n's chariot."}},

    {{"The world shimmers in time with your whistling.",
      "The world shimmers in time with $n's whistling.",
      "The great god Mota gives you a staff.",
      "The great god Mota gives $n a staff.", "Click.", "Click.",
      "Atlas asks you to relieve him.", "Atlas asks $n to relieve him."}}};

void do_pose(Mobile* ch, char* argument)
{
    LEVEL level;
    int pose;

    if (IS_NPC(ch)) return;

    level = UMIN(ch->level, 16);
    pose = number_range(0, level);

    // Yeah, it's random, now. 
    int class_ = number_range(0, 4);

    act(pose_table[pose].message[2 * class_ + 0], ch, NULL, NULL, TO_CHAR);
    act(pose_table[pose].message[2 * class_ + 1], ch, NULL, NULL, TO_ROOM);

    return;
}

void do_bug(Mobile* ch, char* argument)
{
    char bug_file[256];
    sprintf(bug_file, "%s%s", cfg_get_area_dir(), cfg_get_bug_file());
    append_file(ch, bug_file, argument);
    send_to_char("Bug logged.\n\r", ch);
    return;
}

void do_typo(Mobile* ch, char* argument)
{
    char typo_file[256];
    sprintf(typo_file, "%s%s", cfg_get_area_dir(), cfg_get_typo_file());
    append_file(ch, typo_file, argument);
    send_to_char("Typo logged.\n\r", ch);
    return;
}

void do_rent(Mobile* ch, char* argument)
{
    send_to_char("There is no rent here.  Just save and quit.\n\r", ch);
    return;
}

void do_qui(Mobile* ch, char* argument)
{
    send_to_char("If you want to QUIT, you have to spell it out.\n\r", ch);
    return;
}

void do_quit(Mobile* ch, char* argument)
{
    Descriptor* d;
    Descriptor* d_next = NULL;
    int id;

    if (IS_NPC(ch)) return;

    if (ch->position == POS_FIGHTING) {
        send_to_char("No way! You are fighting.\n\r", ch);
        return;
    }

    if (ch->position < POS_STUNNED) {
        send_to_char("You're not DEAD yet.\n\r", ch);
        return;
    }
    send_to_char("Alas, all good things must come to an end.\n\r", ch);
    act("$n has left the game.", ch, NULL, NULL, TO_ROOM);
    sprintf(log_buf, "%s has quit.", NAME_STR(ch));
    log_string(log_buf);
    wiznet("$N rejoins the real world.", ch, NULL, WIZ_LOGINS, 0, get_trust(ch));

    // After extract_char the ch is no longer valid!
    save_char_obj(ch);
    id = ch->id;
    d = ch->desc;
    extract_char(ch, true);
    if (d != NULL)
        close_socket(d);

    /* toast evil cheating bastards */
    for (d = descriptor_list; d != NULL; d = d_next) {
        Mobile* tch;

        d_next = d->next;
        tch = d->original ? d->original : d->character;
        if (tch && tch->id == id) {
            extract_char(tch, true);
            close_socket(d);
        }
    }

    return;
}

void do_save(Mobile* ch, char* argument)
{
    if (IS_NPC(ch))
        return;

    save_char_obj(ch);
    printf_to_char(ch, "Saving. Remember that %s has automatic saving.\n\r", cfg_get_mud_name());
    WAIT_STATE(ch, 4 * PULSE_VIOLENCE);
    return;
}

void do_follow(Mobile* ch, char* argument)
{
    /* RT changed to allow unlimited following and follow the NOFOLLOW rules */
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Follow whom?\n\r", ch);
        return;
    }

    if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL) {
        act("But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR);
        return;
    }

    if (victim == ch) {
        if (ch->master == NULL) {
            send_to_char("You already follow yourself.\n\r", ch);
            return;
        }
        stop_follower(ch);
        return;
    }

    if (!IS_NPC(victim) && IS_SET(victim->act_flags, PLR_NOFOLLOW)
        && !IS_IMMORTAL(ch)) {
        act("$N doesn't seem to want any followers.\n\r", ch, NULL, victim, TO_CHAR);
        return;
    }

    REMOVE_BIT(ch->act_flags, PLR_NOFOLLOW);

    if (ch->master != NULL) stop_follower(ch);

    add_follower(ch, victim);
    return;
}

void add_follower(Mobile* ch, Mobile* master)
{
    if (ch->master != NULL) {
        bug("Add_follower: non-null master.", 0);
        return;
    }

    ch->master = master;
    ch->leader = NULL;

    if (can_see(master, ch))
        act("$n now follows you.", ch, NULL, master, TO_VICT);

    act("You now follow $N.", ch, NULL, master, TO_CHAR);

    return;
}

void stop_follower(Mobile* ch)
{
    if (ch->master == NULL) {
        bug("Stop_follower: null master.", 0);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM)) {
        REMOVE_BIT(ch->affect_flags, AFF_CHARM);
        affect_strip(ch, gsn_charm_person);
    }

    if (can_see(ch->master, ch) && ch->in_room != NULL) {
        act("$n stops following you.", ch, NULL, ch->master, TO_VICT);
        act("You stop following $N.", ch, NULL, ch->master, TO_CHAR);
    }
    if (ch->master->pet == ch) ch->master->pet = NULL;

    ch->master = NULL;
    ch->leader = NULL;
    return;
}

/* nukes charmed monsters and pets */
void nuke_pets(Mobile* ch)
{
    Mobile* pet;

    if ((pet = ch->pet) != NULL) {
        stop_follower(pet);
        if (pet->in_room != NULL)
            act("$N slowly fades away.", ch, NULL, pet, TO_NOTVICT);
        extract_char(pet, true);
    }
    ch->pet = NULL;

    return;
}

void die_follower(Mobile* ch)
{
    Mobile* fch;

    if (ch->master != NULL) {
        if (ch->master->pet == ch) ch->master->pet = NULL;
        stop_follower(ch);
    }

    ch->leader = NULL;

    FOR_EACH_GLOBAL_MOB(fch) {
        if (fch->master == ch)
            stop_follower(fch);
        if (fch->leader == ch)
            fch->leader = fch;
    }

    return;
}

void do_order(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Mobile* victim;
    Mobile* och;
    bool found;
    bool fAll;

    READ_ARG(arg);
    one_argument(argument, arg2);

    if (!str_cmp(arg2, "delete") || !str_cmp(arg2, "mob")) {
        send_to_char("That will NOT be done.\n\r", ch);
        return;
    }

    if (arg[0] == '\0' || argument[0] == '\0') {
        send_to_char("Order whom to do what?\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM)) {
        send_to_char("You feel like taking, not giving, orders.\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "all")) {
        fAll = true;
        victim = NULL;
    }
    else {
        fAll = false;
        if ((victim = get_mob_room(ch, arg)) == NULL) {
            send_to_char("They aren't here.\n\r", ch);
            return;
        }

        if (victim == ch) {
            send_to_char("Aye aye, right away!\n\r", ch);
            return;
        }

        if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch
            || (IS_IMMORTAL(victim) && victim->trust >= ch->trust)) {
            send_to_char("Do it yourself!\n\r", ch);
            return;
        }
    }

    found = false;
    FOR_EACH_ROOM_MOB(och, ch->in_room) {
        if (IS_AFFECTED(och, AFF_CHARM) && och->master == ch 
            && (fAll || och == victim)) {
            found = true;
            sprintf(buf, "$n orders you to '%s'.", argument);
            act(buf, ch, NULL, och, TO_VICT);
            interpret(och, argument);
        }
    }

    if (found) {
        WAIT_STATE(ch, PULSE_VIOLENCE);
        send_to_char("Ok.\n\r", ch);
    }
    else
        send_to_char("You have no followers here.\n\r", ch);
    return;
}

void do_group(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        Mobile* gch;
        Mobile* leader;

        leader = (ch->leader != NULL) ? ch->leader : ch;
        sprintf(buf, "{T%s's group:{x\n\r", PERS(leader, ch));
        send_to_char(buf, ch);

        FOR_EACH_GLOBAL_MOB(gch) {
            if (is_same_group(gch, ch)) {
                sprintf(buf,
                        "{|[{*%2d %s{|]{x %-16s {_%4d/%4d hp %4d/%4d mana %4d/%4d mv %5d xp{x\n\r",
                        gch->level,
                        IS_NPC(gch) ? "Mob" : class_table[gch->ch_class].who_name,
                        capitalize(PERS(gch, ch)), gch->hit, gch->max_hit,
                        gch->mana, gch->max_mana, gch->move, gch->max_move,
                        gch->exp);
                send_to_char(buf, ch);
            }
        }
        return;
    }

    if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (ch->master != NULL || (ch->leader != NULL && ch->leader != ch)) {
        send_to_char("But you are following someone else!\n\r", ch);
        return;
    }

    if (victim->master != ch && ch != victim) {
        act_new("$N isn't following you.", ch, NULL, victim, TO_CHAR,
                POS_SLEEPING);
        return;
    }

    if (IS_AFFECTED(victim, AFF_CHARM)) {
        send_to_char("You can't remove charmed mobs from your group.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM)) {
        act_new("You like your master too much to leave $m!", ch, NULL, victim,
                TO_VICT, POS_SLEEPING);
        return;
    }

    if (is_same_group(victim, ch) && ch != victim) {
        victim->leader = NULL;
        act_new("$n removes $N from $s group.", ch, NULL, victim, TO_NOTVICT,
                POS_RESTING);
        act_new("$n removes you from $s group.", ch, NULL, victim, TO_VICT,
                POS_SLEEPING);
        act_new("You remove $N from your group.", ch, NULL, victim, TO_CHAR,
                POS_SLEEPING);
        return;
    }

    victim->leader = ch;
    act_new("$N joins $n's group.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
    act_new("You join $n's group.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
    act_new("$N joins your group.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);
    return;
}

// 'Split' originally by Gnort, God of Chaos.
void do_split(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    Mobile* gch;
    int16_t members;
    int16_t amount_gold = 0, amount_silver = 0;
    int16_t share_gold, share_silver;
    int16_t extra_gold, extra_silver;

    READ_ARG(arg1);
    one_argument(argument, arg2);

    if (arg1[0] == '\0') {
        send_to_char("Split how much?\n\r", ch);
        return;
    }

    amount_silver = (int16_t)atoi(arg1);

    if (arg2[0] != '\0') 
        amount_gold = (int16_t)atoi(arg2);

    if (amount_gold < 0 || amount_silver < 0) {
        send_to_char("Your group wouldn't like that.\n\r", ch);
        return;
    }

    if (amount_gold == 0 && amount_silver == 0) {
        send_to_char("You hand out zero coins, but no one notices.\n\r", ch);
        return;
    }

    if (ch->gold < amount_gold || ch->silver < amount_silver) {
        send_to_char("You don't have that much to split.\n\r", ch);
        return;
    }

    members = 0;
    FOR_EACH_ROOM_MOB(gch, ch->in_room) {
        if (is_same_group(gch, ch) && !IS_AFFECTED(gch, AFF_CHARM)) members++;
    }

    if (members < 2) {
        send_to_char("Just keep it all.\n\r", ch);
        return;
    }

    share_silver = amount_silver / members;
    extra_silver = amount_silver % members;

    share_gold = amount_gold / members;
    extra_gold = amount_gold % members;

    if (share_gold == 0 && share_silver == 0) {
        send_to_char("Don't even bother, cheapskate.\n\r", ch);
        return;
    }

    ch->silver -= amount_silver;
    ch->silver += share_silver + extra_silver;
    ch->gold -= amount_gold;
    ch->gold += share_gold + extra_gold;

    if (share_silver > 0) {
        sprintf(buf, "You split %d silver coins. Your share is %d silver.\n\r",
                amount_silver, share_silver + extra_silver);
        send_to_char(buf, ch);
    }

    if (share_gold > 0) {
        sprintf(buf, "You split %d gold coins. Your share is %d gold.\n\r",
                amount_gold, share_gold + extra_gold);
        send_to_char(buf, ch);
    }

    if (share_gold == 0) {
        sprintf(buf, "$n splits %d silver coins. Your share is %d silver.",
                amount_silver, share_silver);
    }
    else if (share_silver == 0) {
        sprintf(buf, "$n splits %d gold coins. Your share is %d gold.",
                amount_gold, share_gold);
    }
    else {
        sprintf(
            buf,
            "$n splits %d silver and %d gold coins, giving you %d silver and "
            "%d gold.\n\r",
            amount_silver, amount_gold, share_silver, share_gold);
    }

    FOR_EACH_ROOM_MOB(gch, ch->in_room) {
        if (gch != ch && is_same_group(gch, ch)
            && !IS_AFFECTED(gch, AFF_CHARM)) {
            act(buf, ch, NULL, gch, TO_VICT);
            gch->gold += share_gold;
            gch->silver += share_silver;
        }
    }

    return;
}

void do_gtell(Mobile* ch, char* argument)
{
    Mobile* gch;

    if (argument[0] == '\0') {
        send_to_char("Tell your group what?\n\r", ch);
        return;
    }

    if (IS_SET(ch->comm_flags, COMM_NOTELL)) {
        send_to_char("Your message didn't get through!\n\r", ch);
        return;
    }

    FOR_EACH_GLOBAL_MOB(gch) {
        if (is_same_group(gch, ch))
            act_new("$n tells the group '$t'", ch, argument, gch, TO_VICT,
                    POS_SLEEPING);
    }

    return;
}

/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group(Mobile* ach, Mobile* bch)
{
    if (ach == NULL || bch == NULL)
        return false;

    if (ach->leader != NULL)
        ach = ach->leader;
    if (bch->leader != NULL)
        bch = bch->leader;
    return ach == bch;
}

/*
 * ColoUr setting and unsetting, way cool, Ant Oct 94
 *        revised to include config colour, Ant Feb 95
 */
void do_colour(Mobile* ch, char* argument)
{
    char arg[MAX_STRING_LENGTH];

    if (IS_NPC(ch)) {
        return;
    }

    READ_ARG(arg);

    if (!*arg) {
        if (!IS_SET(ch->act_flags, PLR_COLOUR)) {
            SET_BIT(ch->act_flags, PLR_COLOUR);
            send_to_char(
                "ColoUr is now ON, Way Cool!\n\r"
                "Further syntax:\n\r   colour {c<{xfield{c> <{xcolour{c>{x\n\r"
                "   colour {c<{xfield{c>{x {cbeep{x|{cnobeep{x\n\r"
                "Type help {ccolour{x and {ccolour2{x for details.\n\r"
                "ColoUr is brought to you by Lope, ant@solace.mh.se.\n\r",
                ch);
        }
        else {
            send_to_char_bw("ColoUr is now OFF, <sigh>\n\r", ch);
            REMOVE_BIT(ch->act_flags, PLR_COLOUR);
        }
        return;
    }

    send_to_char("{jUse the {*THEME{j command to change colors.\n\r", ch);
}

void do_clear(Mobile* ch, char* argument)
{
    if (!str_cmp(argument, "reset"))
        //		send_to_char(VT_SETWIN_CLEAR VT_HOMECLR, ch);
        send_to_char(VT_CLENSEQ, ch);
    else
        send_to_char(VT_CLEAR_SCREEN, ch);

    if (ch->desc && ch->desc->screenmap)
        InitScreenMap(ch->desc);

    return;
}

void do_olcx(Mobile* ch, char* argument)
{
    TOGGLE_BIT(ch->comm_flags, COMM_OLCX);
    if (IS_SET(ch->comm_flags, COMM_OLCX))
        send_to_char("VT100 extensions for OLC activated.\n\r", ch);
    else
        send_to_char("VT100 extensions for OLC deactivated.\n\r", ch);
}
