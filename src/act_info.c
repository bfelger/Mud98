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

#include "act_info.h"

#include "act_comm.h"
#include "act_move.h"
#include "comm.h"
#include "config.h"
#include "db.h"
#include "digest.h"
#include "fight.h"
#include "handler.h"
#include "interp.h"
#include "lookup.h"
#include "magic.h"
#include "spell_list.h"
#include "recycle.h"
#include "save.h"
#include "skill_ops.h"
#include "skills.h"
#include "stringbuffer.h"
#include "stringutils.h"
#include "tables.h"
#include "weather.h"

#include <entities/area.h>
#include <entities/descriptor.h>
#include <entities/faction.h>
#include <entities/room.h>
#include <entities/object.h>
#include <entities/player_data.h>

#include <data/class.h>
#include <data/direction.h>
#include <data/item.h>
#include <data/mobile_data.h>
#include <data/player.h>
#include <data/race.h>
#include <data/skill.h>
#include <data/social.h>

#include <olc/bit.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifndef _MSC_VER 
#include <sys/time.h>
#include <unistd.h>
    #ifndef _XOPEN_CRYPT
    #include <crypt.h>
    #endif
#endif

/* for  keeping track of the player count */
int max_on = 0;

// Local functions.
void format_obj_to_char(StringBuffer* sb, Object* obj, Mobile* ch, bool fShort);
void show_list_to_char(List* list, Mobile* ch, bool fShort, bool fShowNothing);
void show_char_to_char_0(Mobile * victim, Mobile* ch);
void show_char_to_char_1(Mobile * victim, Mobile* ch);
void show_char_to_char(List* list, Mobile* ch);
bool check_blind(Mobile * ch);

void format_obj_to_char(StringBuffer* sb, Object* obj, Mobile* ch, bool fShort)
{
    if ((fShort && (obj->short_descr == NULL || obj->short_descr[0] == '\0'))
        || (obj->description == NULL || obj->description[0] == '\0'))
        return;

    if (IS_OBJ_STAT(obj, ITEM_INVIS)) 
        sb_append(sb, COLOR_ALT_TEXT_1 "(Invis)" COLOR_CLEAR " ");
    if (IS_AFFECTED(ch, AFF_DETECT_EVIL) && IS_OBJ_STAT(obj, ITEM_EVIL))
        sb_append(sb, COLOR_RED "(Red Aura)" COLOR_CLEAR " ");
    if (IS_AFFECTED(ch, AFF_DETECT_GOOD) && IS_OBJ_STAT(obj, ITEM_BLESS))
        sb_append(sb, COLOR_B_BLUE "(Blue Aura)" COLOR_CLEAR " ");
    if (IS_AFFECTED(ch, AFF_DETECT_MAGIC) && IS_OBJ_STAT(obj, ITEM_MAGIC))
        sb_append(sb, COLOR_ALT_TEXT_1 "(Magical)" COLOR_CLEAR " ");
    if (IS_OBJ_STAT(obj, ITEM_GLOW)) 
        sb_append(sb, COLOR_ALT_TEXT_2 "(Glowing)" COLOR_CLEAR " ");
    if (IS_OBJ_STAT(obj, ITEM_HUM)) 
        sb_append(sb, COLOR_ALT_TEXT_2 "(Humming)" COLOR_CLEAR " ");

    if (fShort) {
        if (obj->short_descr != NULL) 
            sb_append(sb, obj->short_descr);
    }
    else {
        if (obj->description != NULL) 
            sb_append(sb, obj->description);
    }
}

/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char(List* list, Mobile* ch, bool fShort,
                       bool fShowNothing)
{
    char buf[MAX_STRING_LENGTH];
    char** prgpstrShow;
    int* prgnShow;
    const char* pstrShow;
    Object* obj;
    int nShow;
    int iShow;
    int count;
    bool fCombine;

    if (ch->desc == NULL) return;

    // Alloc space for output lines.
    count = list->count;
    prgpstrShow = alloc_mem(count * sizeof(char*));
    prgnShow = alloc_mem(count * sizeof(int));
    nShow = 0;

    // Format the list of objects.
    StringBuffer* sb = sb_new();
    for (Node* node = list->front; node != NULL; node = node->next) {
        obj = AS_OBJECT(node->value);
        if (obj->wear_loc == WEAR_UNHELD && can_see_obj(ch, obj)) {
            sb_clear(sb);
            format_obj_to_char(sb, obj, ch, fShort);
            pstrShow = sb_string(sb);

            fCombine = false;

            if (IS_NPC(ch) || IS_SET(ch->comm_flags, COMM_COMBINE)) {
                /*
                 * Look for duplicates, case sensitive.
                 * Matches tend to be near end so run loop backwords.
                 */
                for (iShow = nShow - 1; iShow >= 0; iShow--) {
                    if (!strcmp(prgpstrShow[iShow], pstrShow)) {
                        prgnShow[iShow]++;
                        fCombine = true;
                        break;
                    }
                }
            }

            // Couldn't combine, or didn't want to.
            if (!fCombine) {
                prgpstrShow[nShow] = str_dup(pstrShow);
                prgnShow[nShow] = 1;
                nShow++;
            }
        }
    }

    // Output the formatted list.
    for (iShow = 0; iShow < nShow; iShow++) {
        if (prgpstrShow[iShow][0] == '\0') {
            free_string(prgpstrShow[iShow]);
            continue;
        }

        if (IS_NPC(ch) || IS_SET(ch->comm_flags, COMM_COMBINE)) {
            if (prgnShow[iShow] != 1) {
                sprintf(buf, "(%2d) ", prgnShow[iShow]);
                send_to_char(buf, ch);
            }
            else {
                send_to_char("     ", ch);
            }
        }
        send_to_char(prgpstrShow[iShow], ch);
        send_to_char("\n\r", ch);
        free_string(prgpstrShow[iShow]);
    }

    if (fShowNothing && nShow == 0) {
        if (IS_NPC(ch) || IS_SET(ch->comm_flags, COMM_COMBINE))
            send_to_char("     ", ch);
        send_to_char("Nothing.\n\r", ch);
    }

    // Clean up.  The local data will disappear on its own.
    sb_free(sb);
    free_mem(prgpstrShow, count * sizeof(char*));
    free_mem(prgnShow, count * sizeof(int));

    return;
}

void show_char_to_char_0(Mobile* victim, Mobile* ch)
{
    StringBuffer* sb = sb_new();
    
    sb_append(sb, "  ");

    bool is_npc = IS_NPC(victim);

    if (is_npc) {
        QuestTarget* qt = get_quest_targ_end(ch, VNUM_FIELD(victim->prototype));
        if (qt && can_finish_quest(ch, qt->quest_vnum)) {
            sb_append(sb, COLOR_ALT_TEXT_1 "[" COLOR_B_YELLOW "?" COLOR_CLEAR COLOR_ALT_TEXT_1 "]" COLOR_CLEAR " ");
        }
        else if ((qt = get_quest_targ_mob(ch, VNUM_FIELD(victim->prototype))) != NULL) {
            if (qt->type != QUEST_KILL_MOB)
                sb_append(sb, COLOR_ALT_TEXT_1 "[" COLOR_B_YELLOW "!" COLOR_CLEAR COLOR_ALT_TEXT_1 "]" COLOR_CLEAR " ");
            else
                sb_append(sb, COLOR_ALT_TEXT_1 "[" COLOR_B_RED "X" COLOR_CLEAR COLOR_ALT_TEXT_1 "]" COLOR_CLEAR " ");
        }
    }

    if (IS_SET(victim->comm_flags, COMM_AFK)) sb_append(sb, COLOR_ALT_TEXT_2 "[AFK]" COLOR_CLEAR " ");
    if (IS_AFFECTED(victim, AFF_INVISIBLE)) sb_append(sb, COLOR_ALT_TEXT_1 "(Invis)" COLOR_CLEAR " ");
    if (victim->invis_level >= LEVEL_HERO) sb_append(sb, COLOR_ALT_TEXT_1 "(Wizi)" COLOR_CLEAR " ");
    if (IS_AFFECTED(victim, AFF_HIDE)) sb_append(sb, COLOR_ALT_TEXT_1 "(Hide)" COLOR_CLEAR " ");
    if (IS_AFFECTED(victim, AFF_CHARM)) sb_append(sb, COLOR_ALT_TEXT_1 "(Charmed)" COLOR_CLEAR " ");
    if (IS_AFFECTED(victim, AFF_PASS_DOOR)) sb_append(sb, COLOR_ALT_TEXT_1 "(Translucent)" COLOR_CLEAR " ");
    if (IS_AFFECTED(victim, AFF_FAERIE_FIRE)) sb_append(sb, COLOR_B_RED "(Pink Aura)" COLOR_CLEAR " ");
    if (IS_EVIL(victim) && IS_AFFECTED(ch, AFF_DETECT_EVIL))
        sb_append(sb, COLOR_RED "(Red Aura)" COLOR_CLEAR " ");
    if (IS_GOOD(victim) && IS_AFFECTED(ch, AFF_DETECT_GOOD))
        sb_append(sb, COLOR_B_YELLOW "(Golden Aura)" COLOR_CLEAR " ");
    if (IS_AFFECTED(victim, AFF_SANCTUARY)) sb_append(sb, COLOR_B_WHITE "(White Aura)" COLOR_CLEAR " ");
    if (!is_npc && IS_SET(victim->act_flags, PLR_KILLER))
        sb_append(sb, COLOR_B_RED "(KILLER)" COLOR_CLEAR " ");
    if (!is_npc && IS_SET(victim->act_flags, PLR_THIEF))
        sb_append(sb, COLOR_B_RED "(THIEF)" COLOR_CLEAR " ");
    if (victim->position == victim->start_pos
        && victim->long_descr[0] != '\0') {
        sb_append(sb, victim->long_descr);
        printf_to_char(ch, "%s" COLOR_EOL, sb_string(sb));
        sb_free(sb);
        return;
    }

    sb_append(sb, PERS(victim, ch));
    if (!is_npc && !IS_SET(ch->comm_flags, COMM_BRIEF)
        && victim->position == POS_STANDING && ch->on == NULL)
        sb_append(sb, victim->pcdata->title);

    switch (victim->position) {
    case POS_DEAD:
        sb_append(sb, " is DEAD!!");
        break;
    case POS_MORTAL:
        sb_append(sb, " is mortally wounded.");
        break;
    case POS_INCAP:
        sb_append(sb, " is incapacitated.");
        break;
    case POS_STUNNED:
        sb_append(sb, " is lying here stunned.");
        break;
    case POS_SLEEPING:
        if (victim->on != NULL) {
            if (IS_SET(victim->on->furniture.flags, SLEEP_AT)) {
                sb_appendf(sb, " is sleeping at %s.",
                        victim->on->short_descr);
            }
            else if (IS_SET(victim->on->furniture.flags, SLEEP_ON)) {
                sb_appendf(sb, " is sleeping on %s.",
                        victim->on->short_descr);
            }
            else {
                sb_appendf(sb, " is sleeping in %s.",
                        victim->on->short_descr);
            }
        }
        else
            sb_append(sb, " is sleeping here.");
        break;
    case POS_RESTING:
        if (victim->on != NULL) {
            if (IS_SET(victim->on->furniture.flags, REST_AT)) {
                sb_appendf(sb, " is resting at %s.", victim->on->short_descr);
            }
            else if (IS_SET(victim->on->furniture.flags, REST_ON)) {
                sb_appendf(sb, " is resting on %s.", victim->on->short_descr);
            }
            else {
                sb_appendf(sb, " is resting in %s.", victim->on->short_descr);
            }
        }
        else
            sb_append(sb, " is resting here.");
        break;
    case POS_SITTING:
        if (victim->on != NULL) {
            if (IS_SET(victim->on->furniture.flags, SIT_AT)) {
                sb_appendf(sb, " is sitting at %s.", victim->on->short_descr);
            }
            else if (IS_SET(victim->on->furniture.flags, SIT_ON)) {
                sb_appendf(sb, " is sitting on %s.", victim->on->short_descr);
            }
            else {
                sb_appendf(sb, " is sitting in %s.", victim->on->short_descr);
            }
        }
        else
            sb_append(sb, " is sitting here.");
        break;
    case POS_STANDING:
        if (victim->on != NULL) {
            if (IS_SET(victim->on->furniture.flags, STAND_AT)) {
                sb_appendf(sb, " is standing at %s.",
                        victim->on->short_descr);
            }
            else if (IS_SET(victim->on->furniture.flags, STAND_ON)) {
                sb_appendf(sb, " is standing on %s.",
                        victim->on->short_descr);
            }
            else {
                sb_appendf(sb, " is standing in %s.",
                        victim->on->short_descr);
            }
        }
        else
            sb_append(sb, " is here.");
        break;
    case POS_FIGHTING:
        sb_append(sb, " is here, fighting ");
        if (victim->fighting == NULL)
            sb_append(sb, "thin air??");
        else if (victim->fighting == ch)
            sb_append(sb, "YOU!");
        else if (victim->in_room == victim->fighting->in_room) {
            sb_append(sb, PERS(victim->fighting, ch));
            sb_append(sb, ".");
        }
        else
            sb_append(sb, "someone who left??");
        break;
    //case POS_UNKNOWN:
    //    break;
    }

    sb_append(sb, "\n\r");
    const char* text = sb_string(sb);
    char first_char = UPPER(text[0]);
    send_to_char(&first_char, ch);
    send_to_char(text + 1, ch);
    sb_free(sb);
    return;
}

void show_char_to_char_1(Mobile* victim, Mobile* ch)
{
    Object* obj;
    int iWear;
    int percent;
    bool found;

    if (can_see(victim, ch)) {
        if (ch == victim)
            act("$n looks at $mself.", ch, NULL, NULL, TO_ROOM);
        else {
            act("$n looks at you.", ch, NULL, victim, TO_VICT);
            act("$n looks at $N.", ch, NULL, victim, TO_NOTVICT);
        }
    }

    if (victim->description[0] != '\0') {
        send_to_char(victim->description, ch);
    }
    else {
        act("You see nothing special about $M.", ch, NULL, victim, TO_CHAR);
    }

    if (victim->max_hit > 0)
        percent = (100 * victim->hit) / victim->max_hit;
    else
        percent = -1;

    StringBuffer* sb = sb_new();
    sb_append(sb, PERS(victim, ch));

    if (percent >= 100)
        sb_append(sb, " is in excellent condition.\n\r");
    else if (percent >= 90)
        sb_append(sb, " has a few scratches.\n\r");
    else if (percent >= 75)
        sb_append(sb, " has some small wounds and bruises.\n\r");
    else if (percent >= 50)
        sb_append(sb, " has quite a few wounds.\n\r");
    else if (percent >= 30)
        sb_append(sb, " has some big nasty wounds and scratches.\n\r");
    else if (percent >= 15)
        sb_append(sb, " looks pretty hurt.\n\r");
    else if (percent >= 0)
        sb_append(sb, " is in awful condition.\n\r");
    else
        sb_append(sb, " is bleeding to death.\n\r");

    const char* text = sb_string(sb);
    char first_char = UPPER(text[0]);
    send_to_char(&first_char, ch);
    send_to_char(text + 1, ch);
    sb_free(sb);

    found = false;
    sb = sb_new();
    for (iWear = 0; iWear < WEAR_LOC_COUNT; iWear++) {
        if ((obj = get_eq_char(victim, iWear)) != NULL
            && can_see_obj(ch, obj)) {
            if (!found) {
                send_to_char("\n\r", ch);
                act("$N is using:", ch, NULL, victim, TO_CHAR);
                found = true;
            }
            send_to_char(where_name[iWear], ch);
            sb_clear(sb);
            format_obj_to_char(sb, obj, ch, true);
            send_to_char(sb_string(sb), ch);
            send_to_char("\n\r", ch);
        }
    }
    sb_free(sb);

    // Use skill check seam for testability
    if (victim != ch && !IS_NPC(ch)
        && skill_ops->check_simple(ch, gsn_peek)) {
        send_to_char("\n\rYou peek at the inventory:\n\r", ch);
        check_improve(ch, gsn_peek, true, 4);
        show_list_to_char(&victim->objects, ch, true, true);
    }

    return;
}

void show_char_to_char(List* list, Mobile* ch)
{
    if (ch == NULL)
        return;

    Mobile* rch;

    for (Node* node = list->front; node != NULL; node = node->next) {
        rch = AS_MOBILE(node->value);

        if (rch == ch)
            continue;

        if (get_trust(ch) < rch->invis_level) 
            continue;

        if (can_see(ch, rch)) { 
            show_char_to_char_0(rch, ch); 
        }
        else if (room_is_dark(ch->in_room) && IS_AFFECTED(rch, AFF_INFRARED)) {
            send_to_char("You see glowing red eyes watching YOU!\n\r", ch);
        }
    }

    return;
}

bool check_blind(Mobile* ch)
{
    if (!IS_NPC(ch) && IS_SET(ch->act_flags, PLR_HOLYLIGHT)) 
        return true;

    if (IS_AFFECTED(ch, AFF_BLIND)) {
        send_to_char("You can't see a thing!\n\r", ch);
        return false;
    }

    return true;
}

/* changes your scroll */
void do_scroll(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int lines;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        if (ch->lines == 0)
            send_to_char("You do not page long messages.\n\r", ch);
        else {
            sprintf(buf, "You currently display %d lines per page.\n\r",
                    ch->lines + 2);
            send_to_char(buf, ch);
        }
        return;
    }

    if (!is_number(arg)) {
        send_to_char("You must provide a number.\n\r", ch);
        return;
    }

    lines = atoi(arg);

    if (lines == 0) {
        send_to_char("Paging disabled.\n\r", ch);
        ch->lines = 0;
        return;
    }

    if (lines < 10 || lines > 100) {
        send_to_char("You must provide a reasonable number.\n\r", ch);
        return;
    }

    sprintf(buf, "Scroll set to %d lines.\n\r", lines);
    send_to_char(buf, ch);
    ch->lines = lines - 2;
}

/* RT does socials */
void do_socials(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    int iSocial;
    int col;

    col = 0;

    for (iSocial = 0; !IS_NULLSTR(social_table[iSocial].name); iSocial++) {
        sprintf(buf, "%-12s", social_table[iSocial].name);
        send_to_char(buf, ch);
        if (++col % 6 == 0) send_to_char("\n\r", ch);
    }

    if (col % 6 != 0) send_to_char("\n\r", ch);
    return;
}

/* RT Commands to replace news, motd, imotd, etc from ROM */

void do_motd(Mobile* ch, char* argument)
{
    do_function(ch, &do_help, "motd");
}

void do_imotd(Mobile* ch, char* argument)
{
    do_function(ch, &do_help, "imotd");
}

void do_rules(Mobile* ch, char* argument)
{
    do_function(ch, &do_help, "rules");
}

void do_story(Mobile* ch, char* argument)
{
    do_function(ch, &do_help, "story");
}

void do_wizlist(Mobile* ch, char* argument)
{
    do_function(ch, &do_help, "wizlist");
}

/* RT this following section holds all the auto commands from ROM, as well as
   replacements for config */

void do_autolist(Mobile* ch, char* argument)
{
    /* lists most player flags */
    if (IS_NPC(ch)) return;

    send_to_char(COLOR_TITLE "   action     status\n\r", ch);
    send_to_char(COLOR_DECOR_2 "----------------------" COLOR_EOL, ch);

    send_to_char("autoassist     ", ch);
    if (IS_SET(ch->act_flags, PLR_AUTOASSIST))
        send_to_char(COLOR_B_GREEN "ON" COLOR_EOL, ch);
    else
        send_to_char(COLOR_B_RED "OFF" COLOR_EOL, ch);

    send_to_char("autoexit       ", ch);
    if (IS_SET(ch->act_flags, PLR_AUTOEXIT))
        send_to_char(COLOR_B_GREEN "ON" COLOR_EOL, ch);
    else
        send_to_char(COLOR_B_RED "OFF" COLOR_EOL, ch);

    send_to_char("autogold       ", ch);
    if (IS_SET(ch->act_flags, PLR_AUTOGOLD))
        send_to_char(COLOR_B_GREEN "ON" COLOR_EOL, ch);
    else
        send_to_char(COLOR_B_RED "OFF" COLOR_EOL, ch);

    send_to_char("autoloot       ", ch);
    if (IS_SET(ch->act_flags, PLR_AUTOLOOT))
        send_to_char(COLOR_B_GREEN "ON" COLOR_EOL, ch);
    else
        send_to_char(COLOR_B_RED "OFF" COLOR_EOL, ch);

    send_to_char("autosac        ", ch);
    if (IS_SET(ch->act_flags, PLR_AUTOSAC))
        send_to_char(COLOR_B_GREEN "ON" COLOR_EOL, ch);
    else
        send_to_char(COLOR_B_RED "OFF" COLOR_EOL, ch);

    send_to_char("autosplit      ", ch);
    if (IS_SET(ch->act_flags, PLR_AUTOSPLIT))
        send_to_char(COLOR_B_GREEN "ON" COLOR_EOL, ch);
    else
        send_to_char(COLOR_B_RED "OFF" COLOR_EOL, ch);

    send_to_char("compact mode   ", ch);
    if (IS_SET(ch->comm_flags, COMM_COMPACT))
        send_to_char(COLOR_B_GREEN "ON" COLOR_EOL, ch);
    else
        send_to_char(COLOR_B_RED "OFF" COLOR_EOL, ch);

    send_to_char("prompt         ", ch);
    if (IS_SET(ch->comm_flags, COMM_PROMPT))
        send_to_char(COLOR_B_GREEN "ON" COLOR_EOL, ch);
    else
        send_to_char(COLOR_B_RED "OFF" COLOR_EOL, ch);

    send_to_char("combine items  ", ch);
    if (IS_SET(ch->comm_flags, COMM_COMBINE))
        send_to_char(COLOR_B_GREEN "ON" COLOR_EOL, ch);
    else
        send_to_char(COLOR_B_RED "OFF" COLOR_EOL, ch);

    if (!IS_SET(ch->act_flags, PLR_CANLOOT))
        send_to_char("Your corpse is safe from thieves.\n\r", ch);
    else
        send_to_char("Your corpse may be looted.\n\r", ch);

    if (IS_SET(ch->act_flags, PLR_NOSUMMON))
        send_to_char("You cannot be summoned.\n\r", ch);
    else
        send_to_char("You can be summoned.\n\r", ch);

    if (IS_SET(ch->act_flags, PLR_NOFOLLOW))
        send_to_char("You do not welcome followers.\n\r", ch);
    else
        send_to_char("You accept followers.\n\r", ch);
}

void do_autoassist(Mobile* ch, char* argument)
{
    if (IS_NPC(ch)) return;

    if (IS_SET(ch->act_flags, PLR_AUTOASSIST)) {
        send_to_char("Autoassist removed.\n\r", ch);
        REMOVE_BIT(ch->act_flags, PLR_AUTOASSIST);
    }
    else {
        send_to_char("You will now assist when needed.\n\r", ch);
        SET_BIT(ch->act_flags, PLR_AUTOASSIST);
    }
}

void do_autoexit(Mobile* ch, char* argument)
{
    if (IS_NPC(ch)) return;

    if (IS_SET(ch->act_flags, PLR_AUTOEXIT)) {
        send_to_char("Exits will no longer be displayed.\n\r", ch);
        REMOVE_BIT(ch->act_flags, PLR_AUTOEXIT);
    }
    else {
        send_to_char("Exits will now be displayed.\n\r", ch);
        SET_BIT(ch->act_flags, PLR_AUTOEXIT);
    }
}

void do_autogold(Mobile* ch, char* argument)
{
    if (IS_NPC(ch)) return;

    if (IS_SET(ch->act_flags, PLR_AUTOGOLD)) {
        send_to_char("Autogold removed.\n\r", ch);
        REMOVE_BIT(ch->act_flags, PLR_AUTOGOLD);
    }
    else {
        send_to_char("Automatic gold looting set.\n\r", ch);
        SET_BIT(ch->act_flags, PLR_AUTOGOLD);
    }
}

void do_autoloot(Mobile* ch, char* argument)
{
    if (IS_NPC(ch)) return;

    if (IS_SET(ch->act_flags, PLR_AUTOLOOT)) {
        send_to_char("Autolooting removed.\n\r", ch);
        REMOVE_BIT(ch->act_flags, PLR_AUTOLOOT);
    }
    else {
        send_to_char("Automatic corpse looting set.\n\r", ch);
        SET_BIT(ch->act_flags, PLR_AUTOLOOT);
    }
}

void do_autosac(Mobile* ch, char* argument)
{
    if (IS_NPC(ch)) return;

    if (IS_SET(ch->act_flags, PLR_AUTOSAC)) {
        send_to_char("Autosacrificing removed.\n\r", ch);
        REMOVE_BIT(ch->act_flags, PLR_AUTOSAC);
    }
    else {
        send_to_char("Automatic corpse sacrificing set.\n\r", ch);
        SET_BIT(ch->act_flags, PLR_AUTOSAC);
    }
}

void do_autosplit(Mobile* ch, char* argument)
{
    if (IS_NPC(ch)) return;

    if (IS_SET(ch->act_flags, PLR_AUTOSPLIT)) {
        send_to_char("Autosplitting removed.\n\r", ch);
        REMOVE_BIT(ch->act_flags, PLR_AUTOSPLIT);
    }
    else {
        send_to_char("Automatic gold splitting set.\n\r", ch);
        SET_BIT(ch->act_flags, PLR_AUTOSPLIT);
    }
}

void do_brief(Mobile* ch, char* argument)
{
    if (IS_SET(ch->comm_flags, COMM_BRIEF)) {
        send_to_char("Full descriptions activated.\n\r", ch);
        REMOVE_BIT(ch->comm_flags, COMM_BRIEF);
    }
    else {
        send_to_char("Short descriptions activated.\n\r", ch);
        SET_BIT(ch->comm_flags, COMM_BRIEF);
    }
}

void do_compact(Mobile* ch, char* argument)
{
    if (IS_SET(ch->comm_flags, COMM_COMPACT)) {
        send_to_char("Compact mode removed.\n\r", ch);
        REMOVE_BIT(ch->comm_flags, COMM_COMPACT);
    }
    else {
        send_to_char("Compact mode set.\n\r", ch);
        SET_BIT(ch->comm_flags, COMM_COMPACT);
    }
}

void do_prompt(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0') {
        if (IS_SET(ch->comm_flags, COMM_PROMPT)) {
            send_to_char("You will no longer see prompts.\n\r", ch);
            REMOVE_BIT(ch->comm_flags, COMM_PROMPT);
        }
        else {
            send_to_char("You will now see prompts.\n\r", ch);
            SET_BIT(ch->comm_flags, COMM_PROMPT);
        }
        return;
    }

    if (!strcmp(argument, "all")) {
        snprintf(buf, sizeof(buf), "%s", "<%hhp %mm %vmv> ");
    }
    else {
        if (strlen(argument) > 150) argument[150] = '\0';
        snprintf(buf, sizeof(buf), "%s", argument);
        smash_tilde(buf);
        if (str_suffix("%c", buf)) {
            size_t len = strlen(buf);
            if (len + 2 < sizeof(buf)) {
                buf[len] = ' ';
                buf[len + 1] = '\0';
            }
        }
    }

    free_string(ch->prompt);
    ch->prompt = str_dup(buf);
    sprintf(buf, "Prompt set to %s\n\r", ch->prompt);
    send_to_char(buf, ch);
    return;
}

void do_combine(Mobile* ch, char* argument)
{
    if (IS_SET(ch->comm_flags, COMM_COMBINE)) {
        send_to_char("Long inventory selected.\n\r", ch);
        REMOVE_BIT(ch->comm_flags, COMM_COMBINE);
    }
    else {
        send_to_char("Combined inventory selected.\n\r", ch);
        SET_BIT(ch->comm_flags, COMM_COMBINE);
    }
}

void do_noloot(Mobile* ch, char* argument)
{
    if (IS_NPC(ch)) return;

    if (IS_SET(ch->act_flags, PLR_CANLOOT)) {
        send_to_char("Your corpse is now safe from thieves.\n\r", ch);
        REMOVE_BIT(ch->act_flags, PLR_CANLOOT);
    }
    else {
        send_to_char("Your corpse may now be looted.\n\r", ch);
        SET_BIT(ch->act_flags, PLR_CANLOOT);
    }
}

void do_nofollow(Mobile* ch, char* argument)
{
    if (IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) 
        return;

    if (IS_SET(ch->act_flags, PLR_NOFOLLOW)) {
        send_to_char("You now accept followers.\n\r", ch);
        REMOVE_BIT(ch->act_flags, PLR_NOFOLLOW);
    }
    else {
        send_to_char("You no longer accept followers.\n\r", ch);
        SET_BIT(ch->act_flags, PLR_NOFOLLOW);
        die_follower(ch);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Bot structured output
// Machine-readable data for automated clients
////////////////////////////////////////////////////////////////////////////////

// Send structured bot data when PLR_BOT is set
// Format: [BOT:<type>|<data>]
// This allows bots to parse room/mob/object info without text parsing
static void send_bot_room_data(Mobile* ch)
{
    char buf[MAX_STRING_LENGTH * 2];
    Room* room = ch->in_room;
    RoomData* data = room->data;
    
    // Room header: VNUM, flags, sector
    sprintf(buf, "[BOT:ROOM|vnum=%"PRVNUM"|flags=%s|sector=%s]\n\r",
            VNUM_FIELD(room),
            flag_string(room_flag_table, data->room_flags),
            flag_string(sector_flag_table, data->sector_type));
    send_to_char(buf, ch);
    
    // Exits: direction, destination vnum, exit flags
    for (int door = 0; door < DIR_MAX; ++door) {
        RoomExit* exit = room->exit[door];
        if (exit == NULL || exit->to_room == NULL)
            continue;
        
        sprintf(buf, "[BOT:EXIT|dir=%s|vnum=%"PRVNUM"|flags=%s]\n\r",
                dir_list[door].name,
                VNUM_FIELD(exit->to_room),
                flag_string(exit_flag_table, exit->exit_flags));
        send_to_char(buf, ch);
    }
    
    // Mobs in room (excluding self)
    for (Node* node = room->mobiles.front; node != NULL; node = node->next) {
        Mobile* mob = AS_MOBILE(node->value);
        if (mob == ch)
            continue;
        
        // Include key info for targeting: keywords, vnum, level, flags, HP%
        int hp_pct = mob->max_hit > 0 ? (mob->hit * 100 / mob->max_hit) : 100;
        sprintf(buf, "[BOT:MOB|name=%s|vnum=%"PRVNUM"|level=%d|flags=%s|hp=%d%%|align=%d]\n\r",
                IS_NPC(mob) ? NAME_STR(mob->prototype) : NAME_STR(mob),
                IS_NPC(mob) ? VNUM_FIELD(mob->prototype) : 0,
                mob->level,
                IS_NPC(mob) ? flag_string(act_flag_table, mob->act_flags) : "player",
                hp_pct,
                mob->alignment);
        send_to_char(buf, ch);
    }
    
    // Objects in room
    for (Node* node = room->objects.front; node != NULL; node = node->next) {
        Object* obj = AS_OBJECT(node->value);
        sprintf(buf, "[BOT:OBJ|name=%s|vnum=%"PRVNUM"|type=%s|flags=%s|wear=%s]\n\r",
                NAME_STR(obj->prototype),
                VNUM_FIELD(obj->prototype),
                flag_string(type_flag_table, obj->item_type),
                flag_string(extra_flag_table, obj->extra_flags),
                flag_string(wear_flag_table, obj->wear_flags));
        send_to_char(buf, ch);
    }
}

void do_nosummon(Mobile* ch, char* argument)
{
    if (IS_NPC(ch)) {
        if (IS_SET(ch->imm_flags, IMM_SUMMON)) {
            send_to_char("You are no longer immune to summon.\n\r", ch);
            REMOVE_BIT(ch->imm_flags, IMM_SUMMON);
        }
        else {
            send_to_char("You are now immune to summoning.\n\r", ch);
            SET_BIT(ch->imm_flags, IMM_SUMMON);
        }
    }
    else {
        if (IS_SET(ch->act_flags, PLR_NOSUMMON)) {
            send_to_char("You are no longer immune to summon.\n\r", ch);
            REMOVE_BIT(ch->act_flags, PLR_NOSUMMON);
        }
        else {
            send_to_char("You are now immune to summoning.\n\r", ch);
            SET_BIT(ch->act_flags, PLR_NOSUMMON);
        }
    }
}

void do_look(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    RoomExit* room_exit;
    Mobile* victim;
    Object* obj;
    char* pdesc;
    int door;
    int number, count;

    if (ch->desc == NULL)
        return;

    if (ch->position < POS_SLEEPING) {
        send_to_char("You can't see anything but stars!\n\r", ch);
        return;
    }

    if (ch->position == POS_SLEEPING) {
        send_to_char("You can't see anything, you're sleeping!\n\r", ch);
        return;
    }

    if (!check_blind(ch))
        return;

    if (!IS_NPC(ch) && !IS_SET(ch->act_flags, PLR_HOLYLIGHT)
        && room_is_dark(ch->in_room)) {
        send_to_char("It is pitch black ... \n\r", ch);
        show_char_to_char(&ch->in_room->mobiles, ch);
        return;
    }

    READ_ARG(arg1);
    READ_ARG(arg2);
    number = number_argument(arg1, arg3);
    count = 0;

    if (arg1[0] == '\0' || !str_cmp(arg1, "auto")) {
        /* 'look' or 'look auto' */
        sprintf(buf, COLOR_ROOM_TITLE "%s", NAME_STR(ch->in_room));
        send_to_char(buf, ch);

        if ((IS_IMMORTAL(ch) && (IS_NPC(ch) || IS_SET(ch->act_flags, PLR_HOLYLIGHT)))
            || IS_BUILDER(ch, ch->in_room->area->data)
            || IS_TESTER(ch)) {
            sprintf(buf, " " COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "Room %"PRVNUM COLOR_DECOR_1 "]" COLOR_CLEAR, VNUM_FIELD(ch->in_room));
            send_to_char(buf, ch);
        }

        send_to_char(COLOR_EOL, ch);

        // Bot clients get structured data instead of text descriptions
        if (IS_BOT(ch)) {
            send_bot_room_data(ch);
            return;
        }

        const char* room_desc = room_description_for_hour(ch->in_room->data, time_info.hour);
        if (room_desc[0] && !IS_NPC(ch) && !IS_SET(ch->comm_flags, COMM_BRIEF)) {
            if (room_desc[0] == ' ')
                sprintf(buf, COLOR_ROOM_TEXT "%s" COLOR_CLEAR, room_desc);
            else
                sprintf(buf, COLOR_ROOM_TEXT "  %s\n\r" COLOR_CLEAR, room_desc);
            send_to_char(buf, ch);
        }

        if (!IS_NPC(ch) && IS_SET(ch->act_flags, PLR_AUTOEXIT)) {
            send_to_char("\n\r", ch);
            do_function(ch, &do_exits, "auto");
        }

        show_list_to_char(&ch->in_room->objects, ch, false, false);
        show_char_to_char(&ch->in_room->mobiles, ch);
        return;
    }

    if (!str_cmp(arg1, "i") || !str_cmp(arg1, "in") || !str_cmp(arg1, "on")) {
        /* 'look in' */
        if (arg2[0] == '\0') {
            send_to_char("Look in what?\n\r", ch);
            return;
        }

        if ((obj = get_obj_here(ch, arg2)) == NULL) {
            send_to_char("You do not see that here.\n\r", ch);
            return;
        }

        switch (obj->item_type) {
        default:
            send_to_char("That is not a container.\n\r", ch);
            break;

        case ITEM_DRINK_CON:
            if (obj->drink_con.current <= 0) {
                send_to_char("It is empty.\n\r", ch);
                break;
            }

            sprintf(buf, "It's %sfilled with  a %s liquid.\n\r",
                    obj->drink_con.current < obj->drink_con.capacity / 4       ? "less than half-"
                    : obj->drink_con.current < 3 * obj->drink_con.capacity / 4 ? "about half-"
                                                            : "more than half-",
                    liquid_table[obj->drink_con.liquid_type].color);

            send_to_char(buf, ch);
            break;

        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
            if (IS_SET(obj->furniture.flags, CONT_CLOSED)) {
                send_to_char("It is closed.\n\r", ch);
                break;
            }

            act("$p holds:", ch, obj, NULL, TO_CHAR);
            show_list_to_char(&obj->objects, ch, true, true);
            break;
        }
        return;
    }

    if ((victim = get_mob_room(ch, arg1)) != NULL) {
        show_char_to_char_1(victim, ch);
        return;
    }

    FOR_EACH_MOB_OBJ(obj, ch) {
        if (can_see_obj(ch, obj)) { 
            /* player can see object */
            pdesc = get_extra_desc(arg3, obj->extra_desc);
            if (pdesc != NULL) {
                if (++count == number) {
                    send_to_char(pdesc, ch);
                    return;
                }
                else {
                    continue;
                }
            }

            pdesc = get_extra_desc(arg3, obj->prototype->extra_desc);
            if (pdesc != NULL) {
                if (++count == number) {
                    send_to_char(pdesc, ch);
                    return;
                }
                else {
                    continue;
                }
            }

            if (is_name(arg3, NAME_STR(obj))) {
                if (++count == number) {
                    send_to_char(obj->description, ch);
                    send_to_char("\n\r", ch);
                    return;
                }
            }
        }
    }

    FOR_EACH_ROOM_OBJ(obj, ch->in_room) {
        if (can_see_obj(ch, obj)) {
            pdesc = get_extra_desc(arg3, obj->extra_desc);
            if (pdesc != NULL) {
                if (++count == number) {
                    send_to_char(pdesc, ch);
                    return;
                }
            }

            pdesc = get_extra_desc(arg3, obj->prototype->extra_desc);
            if (pdesc != NULL) {
                if (++count == number) {
                    send_to_char(pdesc, ch);
                    return;
                }
            }

            if (is_name(arg3, NAME_STR(obj))) {
                if (++count == number) {
                    send_to_char(obj->description, ch);
                    send_to_char("\n\r", ch);
                    return;
                }
            }
        }
    }

    pdesc = get_extra_desc(arg3, ch->in_room->data->extra_desc);
    if (pdesc != NULL) {
        if (++count == number) {
            send_to_char(pdesc, ch);
            return;
        }
    }

    if (count > 0 && count != number) {
        if (count == 1)
            sprintf(buf, "You only see one %s here.\n\r", arg3);
        else
            sprintf(buf, "You only see %d of those here.\n\r", count);

        send_to_char(buf, ch);
        return;
    }

    if (!str_cmp(arg1, "n") || !str_cmp(arg1, "north"))
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
        send_to_char("You do not see that here.\n\r", ch);
        return;
    }

    /* 'look direction' */
    if ((room_exit = ch->in_room->exit[door]) == NULL) {
        send_to_char("Nothing special there.\n\r", ch);
        return;
    }

    if (room_exit->data->description != NULL && room_exit->data->description[0] != '\0')
        send_to_char(room_exit->data->description, ch);
    else
        send_to_char("Nothing special there.\n\r", ch);

    if (room_exit->data->keyword != NULL && room_exit->data->keyword[0] != '\0'
        && room_exit->data->keyword[0] != ' ') {
        if (IS_SET(room_exit->exit_flags, EX_CLOSED)) {
            act("The $d is closed.", ch, NULL, room_exit->data->keyword, TO_CHAR);
        }
        else if (IS_SET(room_exit->exit_flags, EX_ISDOOR)) {
            act("The $d is open.", ch, NULL, room_exit->data->keyword, TO_CHAR);
        }
    }

    return;
}

/* RT added back for the hell of it */
void do_read(Mobile* ch, char* argument)
{
    do_function(ch, &do_look, argument);
}

void do_examine(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Object* obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Examine what?\n\r", ch);
        return;
    }

    do_function(ch, &do_look, arg);

    if ((obj = get_obj_here(ch, arg)) != NULL) {
        switch (obj->item_type) {
        default:
            break;

        case ITEM_JUKEBOX:
            do_function(ch, &do_play, "list");
            break;

        case ITEM_MONEY: {
            int gold = obj->money.gold;
            int silver = obj->money.silver;
            int copper = obj->money.copper;
            if (gold == 0 && silver == 0 && copper == 0) {
                sprintf(buf, "Odd...there's no coins in the pile.\n\r");
            }
            else {
                char money_buf[64];
                format_money_string(money_buf, sizeof(money_buf), gold, silver, copper, false);
                sprintf(buf, "There are %s in the pile.\n\r", money_buf);
            }
            send_to_char(buf, ch);
            break;
        }

        case ITEM_DRINK_CON:
        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
            sprintf(buf, "in %s", argument);
            do_function(ch, &do_look, buf);
        }
    }

    return;
}

// Thanks to Zrin for auto-exit part.
void do_exits(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    RoomExit* room_exit;
    bool found;
    bool fAuto;
    int door;

    fAuto = !str_cmp(argument, "auto");

    if (!check_blind(ch)) return;

    if (fAuto)
        sprintf(buf, "[Exits:");
    else if (IS_IMMORTAL(ch))
        sprintf(buf, "Obvious exits from room %d:\n\r", VNUM_FIELD(ch->in_room));
    else
        sprintf(buf, "Obvious exits:\n\r");

    found = false;
    for (door = 0; door <= 5; door++) {
        if ((room_exit = ch->in_room->exit[door]) != NULL
            && room_exit->data->to_room != 0 && can_see_room(ch, room_exit->data->to_room)
            && !IS_SET(room_exit->exit_flags, EX_CLOSED)) {
            found = true;
            if (fAuto) {
                strcat(buf, " ");
                strcat(buf, dir_list[door].name);
            }
            else {
                sprintf(
                    buf + strlen(buf), "%-5s - %s", capitalize(dir_list[door].name),
                    (room_exit->to_room && room_is_dark(room_exit->to_room)) ? "Too dark to tell"
                                                    : NAME_STR(room_exit->data->to_room));
                if (IS_IMMORTAL(ch))
                    sprintf(buf + strlen(buf), " (room %d)\n\r",
                            room_exit->data->to_vnum);
                else
                    sprintf(buf + strlen(buf), "\n\r");
            }
        }
    }

    if (!found) 
        strcat(buf, fAuto ? " none" : "None.\n\r");

    if (fAuto) 
        strcat(buf, "]\n\r");

    send_to_char(buf, ch);
    return;
}

void do_worth(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];

    char money_buf[64];
    format_money_string(money_buf, sizeof(money_buf),
        ch->gold, ch->silver, ch->copper, false);

    if (IS_NPC(ch)) {
        sprintf(buf, "You have %s.\n\r", money_buf);
        send_to_char(buf, ch);
        return;
    }

    sprintf(buf,
            "You have %s, and %d experience (%d exp to level).\n\r",
            money_buf, ch->exp,
            (ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp);
    send_to_char(buf, ch);

    return;
}

void do_score(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    int i;

    // Subsume old do_show() functionality so I can remove do_show in favor of
    // OLC.
    if (!str_prefix(argument, "affects")) {
        if (IS_SET(ch->comm_flags, COMM_SHOW_AFFECTS)) {
            send_to_char("Affects will no longer be shown in score.\n\r", ch);
            REMOVE_BIT(ch->comm_flags, COMM_SHOW_AFFECTS);
        }
        else {
            send_to_char("Affects will now be shown in score.\n\r", ch);
            SET_BIT(ch->comm_flags, COMM_SHOW_AFFECTS);
        }
    }

    sprintf(buf, "You are %s%s, level %d, %d years old (%d hours).\n\r",
            NAME_STR(ch), IS_NPC(ch) ? "" : ch->pcdata->title, ch->level,
            get_age(ch), (int)(ch->played + (current_time - ch->logon)) / 3600);
    send_to_char(buf, ch);

    if (get_trust(ch) != ch->level) {
        sprintf(buf, "You are trusted at level %d.\n\r", get_trust(ch));
        send_to_char(buf, ch);
    }

    sprintf(buf, "Race: %s  Sex: %s  Class: %s\n\r", race_table[ch->race].name,
            sex_table[ch->sex].name,
            IS_NPC(ch) ? "mobile" : class_table[ch->ch_class].name);
    send_to_char(buf, ch);

    sprintf(buf, "You have %d/%d hit, %d/%d mana, %d/%d movement.\n\r", ch->hit,
            ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move);
    send_to_char(buf, ch);

    sprintf(buf, "You have %d practices and %d training sessions.\n\r",
            ch->practice, ch->train);
    send_to_char(buf, ch);

    sprintf(buf, "You are carrying %d/%d items with weight %d/%d pounds.\n\r",
            ch->carry_number, can_carry_n(ch), get_carry_weight(ch) / 10,
            can_carry_w(ch) / 10);
    send_to_char(buf, ch);

    sprintf(
        buf,
        "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\n\r",
        ch->perm_stat[STAT_STR], get_curr_stat(ch, STAT_STR),
        ch->perm_stat[STAT_INT], get_curr_stat(ch, STAT_INT),
        ch->perm_stat[STAT_WIS], get_curr_stat(ch, STAT_WIS),
        ch->perm_stat[STAT_DEX], get_curr_stat(ch, STAT_DEX),
        ch->perm_stat[STAT_CON], get_curr_stat(ch, STAT_CON));
    send_to_char(buf, ch);

    char money_buf[64];
    format_money_string(money_buf, sizeof(money_buf),
        ch->gold, ch->silver, ch->copper, false);

    sprintf(
        buf,
        "You have scored %d exp, and have %s.\n\r",
        ch->exp, money_buf);
    send_to_char(buf, ch);

    /* RT shows exp to level */
    if (!IS_NPC(ch) && ch->level < LEVEL_HERO) {
        sprintf(buf, "You need %d exp to level.\n\r",
                ((ch->level + 1) * exp_per_level(ch, ch->pcdata->points)
                 - ch->exp));
        send_to_char(buf, ch);
    }

    sprintf(buf, "Wimpy set to %d hit points.\n\r", ch->wimpy);
    send_to_char(buf, ch);

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
        send_to_char("You are drunk.\n\r", ch);
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] == 0)
        send_to_char("You are thirsty.\n\r", ch);
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER] == 0)
        send_to_char("You are hungry.\n\r", ch);

    switch (ch->position) {
    case POS_DEAD:
        send_to_char("You are DEAD!!\n\r", ch);
        break;
    case POS_MORTAL:
        send_to_char("You are mortally wounded.\n\r", ch);
        break;
    case POS_INCAP:
        send_to_char("You are incapacitated.\n\r", ch);
        break;
    case POS_STUNNED:
        send_to_char("You are stunned.\n\r", ch);
        break;
    case POS_SLEEPING:
        send_to_char("You are sleeping.\n\r", ch);
        break;
    case POS_RESTING:
        send_to_char("You are resting.\n\r", ch);
        break;
    case POS_SITTING:
        send_to_char("You are sitting.\n\r", ch);
        break;
    case POS_STANDING:
        send_to_char("You are standing.\n\r", ch);
        break;
    case POS_FIGHTING:
        send_to_char("You are fighting.\n\r", ch);
        break;
    //case POS_UNKNOWN:
    //    break;
    }

    /* print AC values */
    if (ch->level >= 25) {
        sprintf(buf, "Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r",
                GET_AC(ch, AC_PIERCE), GET_AC(ch, AC_BASH),
                GET_AC(ch, AC_SLASH), GET_AC(ch, AC_EXOTIC));
        send_to_char(buf, ch);
    }

    for (i = 0; i < 4; i++) {
        char* temp;

        switch (i) {
        case (AC_PIERCE):
            temp = "piercing";
            break;
        case (AC_BASH):
            temp = "bashing";
            break;
        case (AC_SLASH):
            temp = "slashing";
            break;
        case (AC_EXOTIC):
            temp = "magic";
            break;
        default:
            temp = "error";
            break;
        }

        send_to_char("You are ", ch);

        if (GET_AC(ch, i) >= 101)
            sprintf(buf, "hopelessly vulnerable to %s.\n\r", temp);
        else if (GET_AC(ch, i) >= 80)
            sprintf(buf, "defenseless against %s.\n\r", temp);
        else if (GET_AC(ch, i) >= 60)
            sprintf(buf, "barely protected from %s.\n\r", temp);
        else if (GET_AC(ch, i) >= 40)
            sprintf(buf, "slightly armored against %s.\n\r", temp);
        else if (GET_AC(ch, i) >= 20)
            sprintf(buf, "somewhat armored against %s.\n\r", temp);
        else if (GET_AC(ch, i) >= 0)
            sprintf(buf, "armored against %s.\n\r", temp);
        else if (GET_AC(ch, i) >= -20)
            sprintf(buf, "well-armored against %s.\n\r", temp);
        else if (GET_AC(ch, i) >= -40)
            sprintf(buf, "very well-armored against %s.\n\r", temp);
        else if (GET_AC(ch, i) >= -60)
            sprintf(buf, "heavily armored against %s.\n\r", temp);
        else if (GET_AC(ch, i) >= -80)
            sprintf(buf, "superbly armored against %s.\n\r", temp);
        else if (GET_AC(ch, i) >= -100)
            sprintf(buf, "almost invulnerable to %s.\n\r", temp);
        else
            sprintf(buf, "divinely armored against %s.\n\r", temp);

        send_to_char(buf, ch);
    }

    /* RT wizinvis and holy light */
    if (IS_IMMORTAL(ch)) {
        send_to_char("Holy Light: ", ch);
        if (IS_SET(ch->act_flags, PLR_HOLYLIGHT))
            send_to_char("on", ch);
        else
            send_to_char("off", ch);

        if (ch->invis_level) {
            sprintf(buf, "  Invisible: level %d", ch->invis_level);
            send_to_char(buf, ch);
        }

        if (ch->incog_level) {
            sprintf(buf, "  Incognito: level %d", ch->incog_level);
            send_to_char(buf, ch);
        }
        send_to_char("\n\r", ch);
    }

    if (ch->level >= 15) {
        sprintf(buf, "Hitroll: %d  Damroll: %d.\n\r", GET_HITROLL(ch),
                GET_DAMROLL(ch));
        send_to_char(buf, ch);
    }

    if (ch->level >= 10) {
        sprintf(buf, "Alignment: %d.  ", ch->alignment);
        send_to_char(buf, ch);
    }

    send_to_char("You are ", ch);
    if (ch->alignment > 900)
        send_to_char("angelic.\n\r", ch);
    else if (ch->alignment > 700)
        send_to_char("saintly.\n\r", ch);
    else if (ch->alignment > 350)
        send_to_char("good.\n\r", ch);
    else if (ch->alignment > 100)
        send_to_char("kind.\n\r", ch);
    else if (ch->alignment > -100)
        send_to_char("neutral.\n\r", ch);
    else if (ch->alignment > -350)
        send_to_char("mean.\n\r", ch);
    else if (ch->alignment > -700)
        send_to_char("evil.\n\r", ch);
    else if (ch->alignment > -900)
        send_to_char("demonic.\n\r", ch);
    else
        send_to_char("satanic.\n\r", ch);

    if (IS_SET(ch->comm_flags, COMM_SHOW_AFFECTS)) 
        do_function(ch, &do_affects, "");
}

void do_affects(Mobile* ch, char* argument)
{
    Affect* affect = NULL;
    Affect* affect_last = NULL;
    char buf[MAX_STRING_LENGTH];

    if (ch->affected != NULL) {
        send_to_char("You are affected by the following spells:\n\r", ch);
        FOR_EACH(affect, ch->affected) {
            if (affect_last != NULL && affect->type == affect_last->type) {
                if (ch->level >= 20)
                    sprintf(buf, "                      ");
                else
                    continue;
            } else {
                sprintf(buf, "Spell: %-15s", skill_table[affect->type].name);
            }

            send_to_char(buf, ch);

            if (ch->level >= 20) {
                sprintf(buf, ": modifies %s by %d ",
                        affect_loc_name(affect->location), affect->modifier);
                send_to_char(buf, ch);
                if (affect->duration == -1)
                    sprintf(buf, "permanently");
                else
                    sprintf(buf, "for %d hours", affect->duration);
                send_to_char(buf, ch);
            }

            send_to_char("\n\r", ch);
            affect_last = affect;
        }
    }
    else {
        send_to_char("You are not affected by any spells.\n\r", ch);
    }

    return;
}

char* const day_name[] = { "the Moon", "the Bull",       "Deception", "Thunder",
                           "Freedom",  "the Great Gods", "the Sun" };

char* const month_name[] = {
    "Winter",
    "the Winter Wolf",
    "the Frost Giant",
    "the Old Forces",
    "the Grand Struggle",
    "the Spring",
    "Nature",
    "Futility",
    "the Dragon",
    "the Sun",
    "the Heat",
    "the Battle",
    "the Dark Shades",
    "the Shadows",
    "the Long Shadows",
    "the Ancient Darkness",
    "the Great Evil"
};

void do_time(Mobile* ch, char* argument)
{
    extern char str_boot_time[];
    char buf[MAX_STRING_LENGTH];
    char* suf;
    int day;

    day = time_info.day + 1;

    if (day > 4 && day < 20)
        suf = "th";
    else if (day % 10 == 1)
        suf = "st";
    else if (day % 10 == 2)
        suf = "nd";
    else if (day % 10 == 3)
        suf = "rd";
    else
        suf = "th";

    sprintf(buf, "It is %d o'clock %s, Day of %s, %d%s the Month of %s.\n\r",
            (time_info.hour % 12 == 0) ? 12 : time_info.hour % 12,
            time_info.hour >= 12 ? "pm" : "am", day_name[day % 7], day, suf,
            month_name[time_info.month]);
    send_to_char(buf, ch);
    sprintf(buf, "Mud98 started up at %s\n\rThe system time is %s.\n\r",
            str_boot_time, (char*)ctime(&current_time));

    send_to_char(buf, ch);
    return;
}

void do_weather(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];

    static char* const sky_look[4]
        = { "cloudless", "cloudy", "rainy", "lit by flashes of lightning" };

    if (!IS_OUTSIDE(ch)) {
        send_to_char("You can't see the weather indoors.\n\r", ch);
        return;
    }

    sprintf(buf, "The sky is %s and %s.\n\r", sky_look[weather_info.sky],
            weather_info.change >= 0 ? "a warm southerly breeze blows"
                                     : "a cold northern gust blows");
    send_to_char(buf, ch);
    return;
}

void do_help(Mobile* ch, char* argument)
{
    HelpData* pHelp;
    Buffer* output;
    bool found = false;
    char argone[MAX_INPUT_LENGTH] = "";
    LEVEL level;

    output = new_buf();

    if (argument[0] == '\0') 
        argument = "summary";

    /* this parts handles help a b so that it returns help 'a b' */
    StringBuffer* argall_sb = sb_new();
    while (argument[0] != '\0') {
        READ_ARG(argone);
        if (sb_length(argall_sb) > 0) 
            sb_append(argall_sb, " ");
        sb_append(argall_sb, argone);
    }
    const char* argall = sb_string(argall_sb);

    FOR_EACH(pHelp, help_first) {
        level = (pHelp->level < 0) ? -1 * pHelp->level - 1 : pHelp->level;

        if (level > get_trust(ch)) continue;

        if (is_name((char*)argall, pHelp->keyword)) {
            /* add seperator if found */
            if (found)
                add_buf(output, "\n\r" COLOR_DECOR_2 "========================="
                    "===================================" COLOR_CLEAR "\n\r\n\r");
            if (pHelp->level >= 0 && str_cmp(argall, "imotd")) {
                add_buf(output, pHelp->keyword);
                add_buf(output, "\n\r");
            }

            // Strip leading '.' to allow initial blanks.
            if (pHelp->text[0] == '.')
                add_buf(output, pHelp->text + 1);
            else
                add_buf(output, pHelp->text);
            found = true;
            /* small hack :) */
            if (ch->desc != NULL && ch->desc->connected != CON_PLAYING
                && ch->desc->connected != CON_GEN_GROUPS)
                break;
        }
    }

    if (!found)
        send_to_char("No help on that word.\n\r", ch);
    else
        page_to_char(BUF(output), ch);
    free_buf(output);
    sb_free(argall_sb);
}

/* whois command */
void do_whois(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Buffer* output;
    char buf[MAX_STRING_LENGTH];
    Descriptor* d;
    bool found = false;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("You must provide a name.\n\r", ch);
        return;
    }

    output = new_buf();

    FOR_EACH(d, descriptor_list) {
        Mobile* wch;
        char const* class_;

        if (d->connected != CON_PLAYING || !can_see(ch, d->character)) continue;

        wch = (d->original != NULL) ? d->original : d->character;

        if (!can_see(ch, wch)) continue;

        if (!str_prefix(arg, NAME_STR(wch))) {
            found = true;

            /* work out the printing */
            class_ = class_table[wch->ch_class].who_name;
            switch (wch->level) {
            case MAX_LEVEL - 0: class_ = COLOR_DECOR_2 "IMP"; break;
            case MAX_LEVEL - 1: class_ = COLOR_DECOR_2 "CRE"; break;
            case MAX_LEVEL - 2: class_ = COLOR_DECOR_2 "SUP"; break;
            case MAX_LEVEL - 3: class_ = COLOR_DECOR_2 "DEI"; break;
            case MAX_LEVEL - 4: class_ = COLOR_DECOR_2 "GOD"; break;
            case MAX_LEVEL - 5: class_ = COLOR_DECOR_2 "IMM"; break;
            case MAX_LEVEL - 6: class_ = COLOR_DECOR_2 "DEM"; break;
            case MAX_LEVEL - 7: class_ = COLOR_DECOR_2 "ANG"; break;
            case MAX_LEVEL - 8: class_ = COLOR_DECOR_2 "AVA"; break;
            default: break;
            }

            /* a little formatting */
            sprintf(buf, COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%2d %6s" COLOR_ALT_TEXT_1 " %s" COLOR_DECOR_1 "]" COLOR_CLEAR " %s%s%s%s%s%s%s%s\n\r", 
                wch->level, race_table[wch->race].who_name, class_, 
                wch->incog_level >= LEVEL_HERO ? COLOR_ALT_TEXT_2 "(Incog)" COLOR_CLEAR " " : "",
                wch->invis_level >= LEVEL_HERO ? COLOR_ALT_TEXT_2 "(Wizi)" COLOR_CLEAR " " : "",
                clan_table[wch->clan].who_name,
                IS_SET(wch->comm_flags, COMM_AFK) ? "[AFK] " : "",
                IS_SET(wch->act_flags, PLR_KILLER) ? COLOR_ALT_TEXT_2 "(KILLER)" COLOR_CLEAR " " : "",
                IS_SET(wch->act_flags, PLR_THIEF) ? COLOR_ALT_TEXT_2 "(THIEF)" COLOR_CLEAR " " : "", 
                NAME_STR(wch), IS_NPC(wch) ? "" : wch->pcdata->title);
            add_buf(output, buf);
        }
    }

    if (!found) {
        send_to_char("No one of that name is playing.\n\r", ch);
        return;
    }

    page_to_char(BUF(output), ch);
    free_buf(output);
}

// New 'who' command originally by Alander of Rivers of Mud.
void do_who(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH] = "";
    char buf2[MAX_STRING_LENGTH] = "";
    Buffer* output;
    Descriptor* d;
    int iClass;
    int iRace;
    int iClan;
    int iLevelLower;
    int iLevelUpper;
    int nNumber;
    int nMatch;
    bool rgfClass[1000] = { 0 };     // I hope you never need more than this?
    bool rgfRace[1000] = { 0 };
    bool rgfClan[MAX_CLAN] = { 0 };
    bool fClassRestrict = false;
    bool fClanRestrict = false;
    bool fClan = false;
    bool fRaceRestrict = false;
    bool fImmortalOnly = false;

    // Set default arguments.
    iLevelLower = 0;
    iLevelUpper = MAX_LEVEL;

    // Parse arguments.
    nNumber = 0;
    for (;;) {
        char arg[MAX_STRING_LENGTH];

        READ_ARG(arg);
        if (arg[0] == '\0') break;

        if (is_number(arg)) {
            switch (++nNumber) {
            case 1:
                iLevelLower = atoi(arg);
                break;
            case 2:
                iLevelUpper = atoi(arg);
                break;
            default:
                send_to_char("Only two level numbers allowed.\n\r", ch);
                return;
            }
        }
        else {
            // Look for classes to turn on.
            if (!str_prefix(arg, "immortals")) { 
                fImmortalOnly = true; 
            }
            else if ((iClass = class_lookup(arg)) == -1) {
                if ((iRace = race_lookup(arg)) == 0 || iRace >= race_count) {
                    if (!str_prefix(arg, "clan")) {
                        fClan = true;
                    }
                    else if ((iClan = clan_lookup(arg)) > 0) {
                        fClanRestrict = true;
                        rgfClan[iClan] = true;
                    }
                    else {
                        send_to_char("That's not a valid race, class, or clan.\n\r", ch);
                        return;
                    }
                }
                else {
                    fRaceRestrict = true;
                    rgfRace[iRace] = true;
                }
            }
            else {
                fClassRestrict = true;
                rgfClass[iClass] = true;
            }
        }
    }

    // Now show matching chars.
    nMatch = 0;
    buf[0] = '\0';
    output = new_buf();
    FOR_EACH(d, descriptor_list) {
        Mobile* wch;
        char const* class_;

        /*
         * Check for match against restrictions.
         * Don't use trust as that exposes trusted mortals.
         */
        if (d->connected != CON_PLAYING || !can_see(ch, d->character)) 
            continue;

        wch = (d->original != NULL) ? d->original : d->character;

        if (!can_see(ch, wch)) continue;

        if (wch->level < iLevelLower || wch->level > iLevelUpper
            || (fImmortalOnly && wch->level < LEVEL_IMMORTAL)
            || (fClassRestrict && !rgfClass[wch->ch_class])
            || (fRaceRestrict && !rgfRace[wch->race])
            || (fClan && !is_clan(wch))
            || (fClanRestrict && !rgfClan[wch->clan]))
            continue;

        nMatch++;

        // Figure out what to print for class.
        class_ = class_table[wch->ch_class].who_name;
        switch (wch->level) {
        case MAX_LEVEL - 0: class_ = COLOR_DECOR_2 "IMP"; break;
        case MAX_LEVEL - 1: class_ = COLOR_DECOR_2 "CRE"; break;
        case MAX_LEVEL - 2: class_ = COLOR_DECOR_2 "SUP"; break;
        case MAX_LEVEL - 3: class_ = COLOR_DECOR_2 "DEI"; break;
        case MAX_LEVEL - 4: class_ = COLOR_DECOR_2 "GOD"; break;
        case MAX_LEVEL - 5: class_ = COLOR_DECOR_2 "IMM"; break;
        case MAX_LEVEL - 6: class_ = COLOR_DECOR_2 "DEM"; break;
        case MAX_LEVEL - 7: class_ = COLOR_DECOR_2 "ANG"; break;
        case MAX_LEVEL - 8: class_ = COLOR_DECOR_2 "AVA"; break;
        default: break;
        }

        // Format it up.
        sprintf(buf, COLOR_DECOR_1 "[" COLOR_ALT_TEXT_1 "%2d %6s" COLOR_ALT_TEXT_1 " %s" COLOR_DECOR_1 "]" COLOR_CLEAR " %s%s%s%s%s%s%s%s\n\r", wch->level,
            race_table[wch->race].who_name,
            class_, wch->incog_level >= LEVEL_HERO ? COLOR_ALT_TEXT_2 "(Incog)" COLOR_CLEAR " " : "",
            wch->invis_level >= LEVEL_HERO ? COLOR_ALT_TEXT_2 "(Wizi)" COLOR_CLEAR " " : "",
            clan_table[wch->clan].who_name,
            IS_SET(wch->comm_flags, COMM_AFK) ? "[AFK] " : "",
            IS_SET(wch->act_flags, PLR_KILLER) ? COLOR_ALT_TEXT_2 "(KILLER)" COLOR_CLEAR " " : "",
            IS_SET(wch->act_flags, PLR_THIEF) ? COLOR_ALT_TEXT_2 "(THIEF)" COLOR_CLEAR " " : "", 
            NAME_STR(wch), IS_NPC(wch) ? "" : wch->pcdata->title);
        add_buf(output, buf);
    }

    sprintf(buf2, "\n\rPlayers found: %d\n\r", nMatch);
    add_buf(output, buf2);
    page_to_char(BUF(output), ch);
    free_buf(output);
    return;
}

void do_count(Mobile* ch, char* argument)
{
    int count;
    Descriptor* d;
    char buf[MAX_STRING_LENGTH];

    count = 0;

    FOR_EACH(d, descriptor_list) {
        if (d->connected == CON_PLAYING && can_see(ch, d->character))
            count++;
    }

    max_on = UMAX(count, max_on);

    if (max_on == count) {
        sprintf(buf,
                "There are %d characters on, the most so far today.\n\r",
                count);
    }
    else {
        sprintf(buf,
                "There are %d characters on, the most on today was %d.\n\r",
                count, max_on);
    }

    send_to_char(buf, ch);
}

void do_inventory(Mobile* ch, char* argument)
{
    send_to_char("You are carrying:\n\r", ch);
    show_list_to_char(&ch->objects, ch, true, true);
    return;
}

void do_equipment(Mobile* ch, char* argument)
{
    Object* obj;
    int iWear;
    bool found;

    send_to_char("You are using:\n\r", ch);
    found = false;
    StringBuffer* sb = sb_new();
    for (iWear = 0; iWear < WEAR_LOC_COUNT; iWear++) {
        if ((obj = get_eq_char(ch, iWear)) == NULL) 
            continue;

        send_to_char(where_name[iWear], ch);
        if (can_see_obj(ch, obj)) {
            sb_clear(sb);
            format_obj_to_char(sb, obj, ch, true);
            send_to_char(sb_string(sb), ch);
            send_to_char("\n\r", ch);
        }
        else {
            send_to_char("something.\n\r", ch);
        }
        found = true;
    }
    sb_free(sb);

    if (!found)
        send_to_char("Nothing.\n\r", ch);

    return;
}

void do_compare(Mobile* ch, char* argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Object* obj1;
    Object* obj2 = NULL;
    int value1;
    int value2;
    char* msg;

    READ_ARG(arg1);
    READ_ARG(arg2);
    if (arg1[0] == '\0') {
        send_to_char("Compare what to what?\n\r", ch);
        return;
    }

    if ((obj1 = get_obj_carry(ch, arg1)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    if (arg2[0] == '\0') {
        FOR_EACH_MOB_OBJ(obj2, ch) {
            if (obj2->wear_loc != WEAR_UNHELD && can_see_obj(ch, obj2)
                && obj1->item_type == obj2->item_type
                && (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0)
                break;
        }

        if (obj2 == NULL) {
            send_to_char("You aren't wearing anything comparable.\n\r", ch);
            return;
        }
    }
    else if ((obj2 = get_obj_carry(ch, arg2)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    msg = NULL;
    value1 = 0;
    value2 = 0;

    if (obj1 == obj2) {
        msg = "You compare $p to itself.  It looks about the same.";
    }
    else if (obj1->item_type != obj2->item_type) {
        msg = "You can't compare $p and $P.";
    }
    else {
        switch (obj1->item_type) {
        case ITEM_ARMOR:
            value1 = obj1->armor.ac_bash + obj1->armor.ac_slash + obj1->armor.ac_pierce;
            value2 = obj2->armor.ac_bash + obj2->armor.ac_slash + obj2->armor.ac_pierce;
            break;

        case ITEM_WEAPON:
            value1 = (1 + obj1->weapon.size_dice) * obj1->weapon.num_dice;
            value2 = (1 + obj2->weapon.size_dice) * obj2->weapon.num_dice;
            break;
            
        default:
            msg = "You can't compare $p and $P.";
            break;
        }
    }

    if (msg == NULL) {
        if (value1 == value2)
            msg = "$p and $P look about the same.";
        else if (value1 > value2)
            msg = "$p looks better than $P.";
        else
            msg = "$p looks worse than $P.";
    }

    act(msg, ch, obj1, obj2, TO_CHAR);
    return;
}

void do_credits(Mobile* ch, char* argument)
{
    do_function(ch, &do_help, "diku");
    return;
}

void do_where(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;
    Descriptor* d;
    bool found;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Players near you:\n\r", ch);
        found = false;
        FOR_EACH(d, descriptor_list) {
            if (d->connected == CON_PLAYING && (victim = d->character) != NULL
                && !IS_NPC(victim) && victim->in_room != NULL
                && !IS_SET(victim->in_room->data->room_flags, ROOM_NOWHERE)
                && !room_is_private(victim->in_room)
                && victim->in_room->area == ch->in_room->area
                && can_see(ch, victim)) {
                found = true;
                sprintf(buf, "%-28s %s\n\r", NAME_STR(victim),
                        NAME_STR(victim->in_room));
                send_to_char(buf, ch);
            }
        }
        if (!found) send_to_char("None\n\r", ch);
    }
    else {
        found = false;
        FOR_EACH_GLOBAL_MOB(victim) {
            if (victim->in_room != NULL
                && victim->in_room->area == ch->in_room->area
                && !IS_AFFECTED(victim, AFF_HIDE)
                && !IS_AFFECTED(victim, AFF_SNEAK) && can_see(ch, victim)
                && is_name(arg, NAME_STR(victim))) {
                found = true;
                sprintf(buf, "%-28s %s\n\r", PERS(victim, ch),
                        NAME_STR(victim->in_room));
                send_to_char(buf, ch);
                break;
            }
        }
        if (!found) 
            act("You didn't find any $T.", ch, NULL, arg, TO_CHAR);
    }

    return;
}

void do_consider(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;
    char* msg;
    int diff;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Consider killing whom?\n\r", ch);
        return;
    }

    if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They're not here.\n\r", ch);
        return;
    }

    if (is_safe(ch, victim)) {
        send_to_char("Don't even think about it.\n\r", ch);
        return;
    }

    diff = victim->level - ch->level;

    if (diff <= -10)
        msg = "You can kill $N naked and weaponless.";
    else if (diff <= -5)
        msg = "$N is no match for you.";
    else if (diff <= -2)
        msg = "$N looks like an easy kill.";
    else if (diff <= 1)
        msg = "The perfect match!";
    else if (diff <= 4)
        msg = "$N says 'Do you feel lucky, punk?'.";
    else if (diff <= 9)
        msg = "$N laughs at you mercilessly.";
    else
        msg = "Death will thank you for your gift.";

    act(msg, ch, NULL, victim, TO_CHAR);
    return;
}

void set_title(Mobile* ch, char* title)
{
    char buf[MAX_STRING_LENGTH] = "";

    if (IS_NPC(ch)) {
        bug("Set_title: NPC.", 0);
        return;
    }

    // Support sparse title lists
    if (!title || !title[0])
        return;

    if (title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?') {
        buf[0] = ' ';
        snprintf(buf + 1, sizeof(buf) - 1, "%s", title);
    }
    else {
        snprintf(buf, sizeof(buf), "%s", title);
    }

    free_string(ch->pcdata->title);
    ch->pcdata->title = str_dup(buf);
    return;
}

void do_title(Mobile* ch, char* argument)
{
    if (IS_NPC(ch)) return;

    if (argument[0] == '\0') {
        send_to_char("Change your title to what?\n\r", ch);
        return;
    }

    if (strlen(argument) > 45) argument[45] = '\0';

    smash_tilde(argument);
    set_title(ch, argument);
    send_to_char("Ok.\n\r", ch);
}

void do_description(Mobile* ch, char* argument)
{
    if (argument[0] != '\0') {
        smash_tilde(argument);

        if (argument[0] == '-') {
            bool found = false;

            if (ch->description == NULL || ch->description[0] == '\0') {
                send_to_char("No lines left to remove.\n\r", ch);
                return;
            }

            StringBuffer* sb = sb_new();
            sb_append(sb, ch->description);
            const char* text = sb_string(sb);
            size_t len = strlen(text);

            for (; len > 0; len--) {
                if (text[len] == '\r') {
                    if (!found) {
                        /* back it up */
                        if (len > 0) 
                            len--;
                        found = true;
                    }
                    else  {
                        /* found the second one */
                        char* result = strndup(text, len + 1);
                        free_string(ch->description);
                        ch->description = str_dup(result);
                        free(result);
                        sb_free(sb);
                        send_to_char("Your description is:\n\r", ch);
                        send_to_char(ch->description ? ch->description
                                                     : "(None).\n\r",
                                     ch);
                        return;
                    }
                }
            }
            free_string(ch->description);
            ch->description = str_dup("");
            sb_free(sb);
            send_to_char("Description cleared.\n\r", ch);
            return;
        }
        
        StringBuffer* sb = sb_new();
        if (argument[0] == '+') {
            if (ch->description != NULL) {
                sb_append(sb, ch->description);
            }
            argument++;
            while (ISSPACE(*argument))
                argument++;
        }

        if (sb_length(sb) + strlen(argument) >= 1024) {
            send_to_char("Description too long.\n\r", ch);
            sb_free(sb);
            return;
        }

        sb_append(sb, argument);
        sb_append(sb, "\n\r");
        free_string(ch->description);
        ch->description = str_dup(sb_string(sb));
        sb_free(sb);
    }

    send_to_char("Your description is:\n\r", ch);
    send_to_char(ch->description ? ch->description : "(None).\n\r", ch);
    return;
}

void do_report(Mobile* ch, char* argument)
{
    char buf[MAX_INPUT_LENGTH];

    sprintf(buf, "You say 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'\n\r",
            ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move,
            ch->max_move, ch->exp);

    send_to_char(buf, ch);

    sprintf(buf, "$n says 'I have %d/%d hp %d/%d mana %d/%d mv %d xp.'",
            ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move,
            ch->max_move, ch->exp);

    act(buf, ch, NULL, NULL, TO_ROOM);

    return;
}

void do_practice(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    SKNUM sn;

    if (IS_NPC(ch))
        return;

    // Bot-only reset command: restores skills and practice points to starting values
    if (!str_cmp(argument, "reset") && (IS_BOT(ch) || IS_TESTER(ch))) {

        // Clear all skills/groups and set to 0 (unknown)
        memset(ch->pcdata->learned, 0, sizeof(int16_t) * skill_count);
        memset(ch->pcdata->group_known, 0, sizeof(bool) * skill_group_count);
        
        // Re-add racial skills/groups
        for (int i = 0; i < RACE_NUM_SKILLS; i++) {
            if (race_table[ch->race].skills[i] == NULL)
                break;
            group_add(ch, race_table[ch->race].skills[i], false);
        }
        
        // Re-add class base group
        if (class_table[ch->ch_class].base_group != NULL) {
            group_add(ch, class_table[ch->ch_class].base_group, false);
        }
        
        // Recall skill is always given at 50%
        ch->pcdata->learned[gsn_recall] = 50;
        
        // Reset practice sessions to starting value
        ch->practice = 5;
        
        send_to_char("Your skills and practice sessions have been reset to starting values.\n\r", ch);
        return;
    }

    if (argument[0] == '\0') {
        int col;

        col = 0;
        for (sn = 0; sn < skill_count; sn++) {
            if (skill_table[sn].name == NULL)
                break;
            if (ch->level < SKILL_LEVEL(sn, ch)
                || ch->pcdata->learned[sn] < 1 /* skill is not known */)
                continue;

            sprintf(buf, "%-18s %3d%%  ", skill_table[sn].name,
                    ch->pcdata->learned[sn]);
            send_to_char(buf, ch);
            if (++col % 3 == 0) 
                send_to_char("\n\r", ch);
        }

        if (col % 3 != 0) send_to_char("\n\r", ch);

        sprintf(buf, "You have %d practice sessions left.\n\r", ch->practice);
        send_to_char(buf, ch);
    }
    else {
        int16_t adept;

        if (!IS_AWAKE(ch)) {
            send_to_char("In your dreams, or what?\n\r", ch);
            return;
        }

        if (!cfg_get_practice_anywhere()) {
            Mobile* mob = NULL;
            FOR_EACH_ROOM_MOB(mob, ch->in_room) {
                if (IS_NPC(mob) && IS_SET(mob->act_flags, ACT_PRACTICE))
                    break;
            }

            if (mob == NULL) {
                send_to_char("You can't do that here.\n\r", ch);
                return;
            }
        }

        if (ch->practice <= 0) {
            send_to_char("You have no practice sessions left.\n\r", ch);
            return;
        }

        if ((sn = find_spell(ch, argument)) < 0
            || (!IS_NPC(ch)
                && (ch->level < SKILL_LEVEL(sn, ch)
                    || ch->pcdata->learned[sn] < 1 /* skill is not known */
                    || SKILL_RATING(sn, ch) == 0))) {
            send_to_char("You can't practice that.\n\r", ch);
            return;
        }

        adept = IS_NPC(ch) ? 100 : class_table[ch->ch_class].skill_cap;

        if (ch->pcdata->learned[sn] >= adept) {
            sprintf(buf, "You are already learned at %s.\n\r",
                    skill_table[sn].name);
            send_to_char(buf, ch);
        }
        else {
            ch->practice--;
            ch->pcdata->learned[sn] += 
                int_mod[get_curr_stat(ch, STAT_INT)].learn 
                / SKILL_RATING(sn, ch);
            if (ch->pcdata->learned[sn] < adept) {
                act("You practice $T.", ch, NULL, skill_table[sn].name,
                    TO_CHAR);
                act("$n practices $T.", ch, NULL, skill_table[sn].name,
                    TO_ROOM);
            }
            else {
                ch->pcdata->learned[sn] = adept;
                act("You are now learned at $T.", ch, NULL,
                    skill_table[sn].name, TO_CHAR);
                act("$n is now learned at $T.", ch, NULL, skill_table[sn].name,
                    TO_ROOM);
            }
        }
    }
    return;
}

// 'Wimpy' originally by Dionysos.
void do_wimpy(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int wimpy;

    one_argument(argument, arg);

    if (arg[0] == '\0')
        wimpy = ch->max_hit / 5;
    else
        wimpy = atoi(arg);

    if (wimpy < 0) {
        send_to_char("Your courage exceeds your wisdom.\n\r", ch);
        return;
    }

    if (wimpy > ch->max_hit / 2) {
        send_to_char("Such cowardice ill becomes you.\n\r", ch);
        return;
    }

    ch->wimpy = (int16_t)wimpy;
    sprintf(buf, "Wimpy set to %d hit points.\n\r", wimpy);
    send_to_char(buf, ch);
    return;
}

void do_password(Mobile* ch, char* argument)
{
    char arg1[MAX_INPUT_LENGTH] = "";
    char arg2[MAX_INPUT_LENGTH] = "";
    char* pArg;
    char cEnd;

    if (IS_NPC(ch))
        return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while (ISSPACE(*argument))
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while (*argument != '\0') {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while (ISSPACE(*argument))
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while (*argument != '\0') {
        if (*argument == cEnd) {
            argument++;
            break;
        }
        *pArg++ = *argument++;
    }
    *pArg = '\0';

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Syntax: password <old> <new>.\n\r", ch);
        return;
    }

    if (!validate_password(arg1, ch)) {
        WAIT_STATE(ch, 40);
        send_to_char("Wrong password.  Wait 10 seconds.\n\r", ch);
        return;
    }

    if (strlen(arg2) < 5) {
        send_to_char("New password must be at least five characters long.\n\r",
                     ch);
        return;
    }

    if (!set_password(arg2, ch)) {
        perror("do_password: Could not get digest.");
    }

    save_char_obj(ch);
    send_to_char("Ok.\n\r", ch);
    return;
}

void do_reputation(Mobile* ch, char* argument)
{
    if (IS_NPC(ch)) {
        send_to_char("Mobiles have no reputations.\n\r", ch);
        return;
    }

    faction_show_reputations(ch);
}
