/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.   *
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

#include "skills.h"

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interp.h"
#include "magic.h"
#include "spell_list.h"
#include "recycle.h"
#include "update.h"

#include "entities/mobile.h"
#include "entities/descriptor.h"
#include "entities/player_data.h"

#include "data/mobile_data.h"
#include "data/race.h"
#include "data/skill.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef _MSC_VER 
#include <sys/time.h>
#endif

/* used to get new skills */
void do_gain(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Mobile* trainer = NULL;
    SKNUM gn = 0, sn = 0;

    if (IS_NPC(ch)) 
        return;

    /* find a trainer */
    FOR_EACH_ROOM_MOB(trainer, ch->in_room)
        if (IS_NPC(trainer) && IS_SET(trainer->act_flags, ACT_GAIN)) 
            break;

    if (trainer == NULL || !can_see(ch, trainer)) {
        send_to_char("You can't do that here.\n\r", ch);
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        do_function(trainer, &do_say, "Pardon me?");
        return;
    }

    if (!str_prefix(arg, "list")) {
        int col = 0;

        sprintf(buf, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "group", "cost",
                "group", "cost", "group", "cost");
        send_to_char(buf, ch);

        for (gn = 0; gn < skill_group_count; gn++) {
            if (skill_group_table[gn].name == NULL) break;

            if (!ch->pcdata->group_known[gn]
                && SKILL_GROUP_RATING(gn, ch) > 0) {
                sprintf(buf, "%-18s %-5d ", skill_group_table[gn].name,
                    SKILL_GROUP_RATING(gn, ch));
                send_to_char(buf, ch);
                if (++col % 3 == 0) send_to_char("\n\r", ch);
            }
        }
        if (col % 3 != 0) 
            send_to_char("\n\r", ch);

        send_to_char("\n\r", ch);

        col = 0;

        sprintf(buf, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "skill", "cost",
                "skill", "cost", "skill", "cost");
        send_to_char(buf, ch);

        for (sn = 0; sn < skill_count; sn++) {
            if (skill_table[sn].name == NULL) break;

            if (!ch->pcdata->learned[sn]
                && SKILL_RATING(sn, ch) > 0
                && !HAS_SPELL_FUNC(sn)) {
                sprintf(buf, "%-18s %-5d ", skill_table[sn].name,
                        SKILL_RATING(sn, ch));
                send_to_char(buf, ch);
                if (++col % 3 == 0) 
                    send_to_char("\n\r", ch);
            }
        }
        if (col % 3 != 0) 
            send_to_char("\n\r", ch);
        return;
    }

    if (!str_prefix(arg, "convert")) {
        if (ch->practice < 10) {
            act("$N tells you 'You are not yet ready.'", ch, NULL, trainer,
                TO_CHAR);
            return;
        }

        act("$N helps you apply your practice to training", ch, NULL, trainer,
            TO_CHAR);
        ch->practice -= 10;
        ch->train += 1;
        return;
    }

    if (!str_prefix(arg, "points")) {
        if (ch->train < 2) {
            act("$N tells you 'You are not yet ready.'", ch, NULL, trainer,
                TO_CHAR);
            return;
        }

        if (ch->pcdata->points <= 40) {
            act("$N tells you 'There would be no point in that.'", ch, NULL,
                trainer, TO_CHAR);
            return;
        }

        act("$N trains you, and you feel more at ease with your skills.", ch,
            NULL, trainer, TO_CHAR);

        ch->train -= 2;
        ch->pcdata->points -= 1;
        ch->exp = exp_per_level(ch, ch->pcdata->points) * ch->level;
        return;
    }

    /* else add a group/skill */

    gn = group_lookup(argument);
    if (gn > 0) {
        if (ch->pcdata->group_known[gn]) {
            act("$N tells you 'You already know that group!'", ch, NULL,
                trainer, TO_CHAR);
            return;
        }

        if (SKILL_GROUP_RATING(gn, ch) <= 0) {
            act("$N tells you 'That group is beyond your powers.'", ch, NULL,
                trainer, TO_CHAR);
            return;
        }

        if (ch->train < SKILL_GROUP_RATING(gn, ch)) {
            act("$N tells you 'You are not yet ready for that group.'", ch,
                NULL, trainer, TO_CHAR);
            return;
        }

        /* add the group */
        gn_add(ch, gn);
        act("$N trains you in the art of $t", ch, skill_group_table[gn].name, trainer,
            TO_CHAR);
        ch->train -= SKILL_GROUP_RATING(gn, ch);
        return;
    }

    sn = skill_lookup(argument);
    if (sn > -1) {
        if (HAS_SPELL_FUNC(sn)) {
            act("$N tells you 'You must learn the full group.'", ch, NULL,
                trainer, TO_CHAR);
            return;
        }

        if (ch->pcdata->learned[sn]) {
            act("$N tells you 'You already know that skill!'", ch, NULL,
                trainer, TO_CHAR);
            return;
        }

        if (SKILL_RATING(sn, ch) <= 0) {
            act("$N tells you 'That skill is beyond your powers.'", ch, NULL,
                trainer, TO_CHAR);
            return;
        }

        if (ch->train < SKILL_RATING(sn, ch)) {
            act("$N tells you 'You are not yet ready for that skill.'", ch,
                NULL, trainer, TO_CHAR);
            return;
        }

        /* add the skill */
        ch->pcdata->learned[sn] = 1;
        act("$N trains you in the art of $t", ch, skill_table[sn].name, trainer,
            TO_CHAR);
        ch->train -= SKILL_RATING(sn, ch);
        return;
    }

    act("$N tells you 'I do not understand...'", ch, NULL, trainer, TO_CHAR);
}

/* RT spells and skills show the players spells (or skills) */
void do_spells(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    char spell_columns[LEVEL_HERO + 1] = { 0 };
    SKNUM sn;
    LEVEL level, min_lev = 1, max_lev = LEVEL_HERO;
    int mana;
    bool fAll = false, found = false;
    char buf[MAX_STRING_LENGTH];
    int slot;

    if (IS_NPC(ch) || ch->desc == NULL) 
        return;

    if (argument[0] != '\0') {
        fAll = true;

        if (str_prefix(argument, "all")) {
            READ_ARG(arg);
            if (!is_number(arg)) {
                send_to_char("Arguments must be numerical or all.\n\r", ch);
                return;
            }
            max_lev = (LEVEL)atoi(arg);

            if (max_lev < 1 || max_lev > LEVEL_HERO) {
                sprintf(buf, "Levels must be between 1 and %d.\n\r",
                        LEVEL_HERO);
                send_to_char(buf, ch);
                return;
            }

            if (argument[0] != '\0') {
                READ_ARG(arg);
                if (!is_number(arg)) {
                    send_to_char("Arguments must be numerical or all.\n\r", ch);
                    return;
                }
                min_lev = max_lev;
                max_lev = (LEVEL)atoi(arg);

                if (max_lev < 1 || max_lev > LEVEL_HERO) {
                    sprintf(buf, "Levels must be between 1 and %d.\n\r",
                            LEVEL_HERO);
                    send_to_char(buf, ch);
                    return;
                }

                if (min_lev > max_lev) {
                    send_to_char("That would be silly.\n\r", ch);
                    return;
                }
            }
        }
    }

    int range = 1 + max_lev - min_lev;
    Buffer** spell_buf;
    if ((spell_buf = calloc(sizeof(Buffer*), range)) == 0) {
        send_to_char("Can't do that right now.\n\r", ch);
        perror("do_spells: Cannot allocate spell bufs!");
        return;
    }

    for (int i = 0; i < range; i++)
        spell_buf[i] = new_buf();
    
    for (level = 0; level < LEVEL_HERO + 1; level++) {
        spell_columns[level] = 0;
    }

    for (sn = 0; sn < skill_count; sn++) {
        if (skill_table[sn].name == NULL) 
            break;

        if ((level = SKILL_LEVEL(sn, ch)) < LEVEL_HERO + 1
            && (fAll || level <= ch->level) && level >= min_lev
            && level <= max_lev && HAS_SPELL_FUNC(sn)
            && ch->pcdata->learned[sn] > 0) {
            found = true;

            slot = level - min_lev;

            if (BUF(spell_buf[slot])[0] == '\0') {
                sprintf(buf, "\n\rLevel %2d: ", level);
                add_buf(spell_buf[slot], buf);
            }
            else if (++spell_columns[level] % 2 == 0) {
                add_buf(spell_buf[slot], "\n\r          ");
            }

            if (ch->level < level)
                sprintf(buf, "%-18s n/a      ", skill_table[sn].name);
            else {
                mana = UMAX(skill_table[sn].min_mana,
                            100 / (2 + ch->level - level));
                sprintf(buf, "%-18s  %3d mana  ", skill_table[sn].name, mana);
            }
            add_buf(spell_buf[slot], buf);
        } 
    }
    
    /* return results */

    if (!found) {
        send_to_char("No spells found.\n\r", ch);
    }
    else {
        Buffer* buffer = new_buf();
        for (level = min_lev; level <= max_lev; level++) {
            slot = level - min_lev;
            if (BUF(spell_buf[slot])[0] != '\0')
                add_buf(buffer, BUF(spell_buf[slot]));
        }
        add_buf(buffer, "\n\r");
        page_to_char(BUF(buffer), ch);
        free_buf(buffer);
    }
    for (int i = 0; i < range; i++)
        free_buf(spell_buf[i]);
    free(spell_buf);
}

void do_skills(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    char skill_columns[LEVEL_HERO + 1] = { 0 };
    SKNUM sn;
    LEVEL level, min_lev = 1, max_lev = LEVEL_HERO;
    bool fAll = false, found = false;
    char buf[MAX_STRING_LENGTH];
    int slot;

    if (IS_NPC(ch))
        return;

    if (argument[0] != '\0') {
        fAll = true;

        if (str_prefix(argument, "all")) {
            READ_ARG(arg);
            if (!is_number(arg)) {
                send_to_char("Arguments must be numerical or all.\n\r", ch);
                return;
            }
            max_lev = (LEVEL)atoi(arg);

            if (max_lev < 1 || max_lev > LEVEL_HERO) {
                sprintf(buf, "Levels must be between 1 and %d.\n\r",
                        LEVEL_HERO);
                send_to_char(buf, ch);
                return;
            }

            if (argument[0] != '\0') {
                READ_ARG(arg);
                if (!is_number(arg)) {
                    send_to_char("Arguments must be numerical or all.\n\r", ch);
                    return;
                }
                min_lev = max_lev;
                max_lev = (LEVEL)atoi(arg);

                if (max_lev < 1 || max_lev > LEVEL_HERO) {
                    sprintf(buf, "Levels must be between 1 and %d.\n\r",
                            LEVEL_HERO);
                    send_to_char(buf, ch);
                    return;
                }

                if (min_lev > max_lev) {
                    send_to_char("That would be silly.\n\r", ch);
                    return;
                }
            }
        }
    }

    int range = 1 + max_lev - min_lev;
    Buffer** skill_buf;
    if ((skill_buf = calloc(sizeof(Buffer*), range)) == 0) {
        send_to_char("Can't do that right now.\n\r", ch);
        perror("do_skills: Cannot allocate skill bufs!");
        return;
    }

    for (int i = 0; i < range; i++)
        skill_buf[i] = new_buf();

    for (level = 0; level < LEVEL_HERO + 1; level++) {
        skill_columns[level] = 0;
    }

    for (sn = 0; sn < skill_count; sn++) {
        if (skill_table[sn].name == NULL) break;

        if ((level = SKILL_RATING(sn, ch)) < LEVEL_HERO + 1
            && (fAll || level <= ch->level) && level >= min_lev
            && level <= max_lev && !HAS_SPELL_FUNC(sn)
            && ch->pcdata->learned[sn] > 0) {
            found = true;

            slot = level - min_lev;

            if (BUF(skill_buf[slot])[0] == '\0') {
                sprintf(buf, "\n\rLevel %2d: ", level);
                add_buf(skill_buf[slot], buf);
            }
            else if (++skill_columns[level] % 2 == 0) {
                add_buf(skill_buf[slot], "\n\r          ");
            }

            if (ch->level < level)
                sprintf(buf, "%-18s n/a      ", skill_table[sn].name);
            else {
                sprintf(buf, "%-18s %3d%%      ", skill_table[sn].name,
                    ch->pcdata->learned[sn]);
            }
            add_buf(skill_buf[slot], buf);
        }
    }

    /* return results */

    if (!found) {
        send_to_char("No skills found.\n\r", ch);
    }
    else {
        Buffer* buffer = new_buf();
        for (level = min_lev; level <= max_lev; level++) {
            slot = level - min_lev;
            if (BUF(skill_buf[slot])[0] != '\0')
                add_buf(buffer, BUF(skill_buf[slot]));
        }
        add_buf(buffer, "\n\r");
        page_to_char(BUF(buffer), ch);
        free_buf(buffer);
    }

    for (int i = 0; i < range; i++)
        free_buf(skill_buf[i]);
    free(skill_buf);
}

/* shows skills, groups and costs (only if not bought) */
void list_group_costs(Mobile* ch)
{
    char buf[100];
    SKNUM gn, sn;
    int col;

    if (IS_NPC(ch)) 
        return;

    col = 0;

    sprintf(buf, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "group", "cp", "group",
            "cp", "group", "cp");
    send_to_char(buf, ch);

    for (gn = 0; gn < skill_group_count; gn++) {
        if (skill_group_table[gn].name == NULL) break;

        if (!ch->gen_data->group_chosen[gn] && !ch->pcdata->group_known[gn]
            && SKILL_GROUP_RATING(gn, ch) > 0) {
            sprintf(buf, "%-18s %-5d ", skill_group_table[gn].name,
                    SKILL_GROUP_RATING(gn, ch));
            send_to_char(buf, ch);
            if (++col % 3 == 0) send_to_char("\n\r", ch);
        }
    }
    if (col % 3 != 0) send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    col = 0;

    sprintf(buf, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r", "skill", "cp", "skill",
            "cp", "skill", "cp");
    send_to_char(buf, ch);

    for (sn = 0; sn < skill_count; sn++) {
        if (skill_table[sn].name == NULL) break;

        if (!ch->gen_data->skill_chosen[sn] && ch->pcdata->learned[sn] == 0
            && !HAS_SPELL_FUNC(sn)
            && SKILL_RATING(sn, ch) > 0) {
            sprintf(buf, "%-18s %-5d ", skill_table[sn].name,
                SKILL_RATING(sn, ch));
            send_to_char(buf, ch);
            if (++col % 3 == 0) send_to_char("\n\r", ch);
        }
    }
    if (col % 3 != 0) send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    sprintf(buf, "Creation points: %d\n\r", ch->pcdata->points);
    send_to_char(buf, ch);
    sprintf(buf, "Experience per level: %d\n\r",
            exp_per_level(ch, ch->gen_data->points_chosen));
    send_to_char(buf, ch);
    return;
}

void list_group_chosen(Mobile* ch)
{
    char buf[100];
    SKNUM gn, sn;
    int col;

    if (IS_NPC(ch)) return;

    col = 0;

    sprintf(buf, "%-18s %-5s %-18s %-5s %-18s %-5s", "group", "cp", "group",
            "cp", "group", "cp\n\r");
    send_to_char(buf, ch);

    for (gn = 0; gn < skill_group_count; gn++) {
        if (skill_group_table[gn].name == NULL) break;

        if (ch->gen_data->group_chosen[gn]
            && SKILL_GROUP_RATING(gn, ch) > 0) {
            sprintf(buf, "%-18s %-5d ", skill_group_table[gn].name,
                    SKILL_GROUP_RATING(gn, ch));
            send_to_char(buf, ch);
            if (++col % 3 == 0) send_to_char("\n\r", ch);
        }
    }
    if (col % 3 != 0) send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    col = 0;

    sprintf(buf, "%-18s %-5s %-18s %-5s %-18s %-5s", "skill", "cp", "skill",
            "cp", "skill", "cp\n\r");
    send_to_char(buf, ch);

    for (sn = 0; sn < skill_count; sn++) {
        if (skill_table[sn].name == NULL) break;

        if (ch->gen_data->skill_chosen[sn]
            && SKILL_RATING(sn, ch) > 0) {
            sprintf(buf, "%-18s %-5d ", skill_table[sn].name,
                SKILL_RATING(sn, ch));
            send_to_char(buf, ch);
            if (++col % 3 == 0) send_to_char("\n\r", ch);
        }
    }
    if (col % 3 != 0) send_to_char("\n\r", ch);
    send_to_char("\n\r", ch);

    sprintf(buf, "Creation points: %d\n\r", ch->gen_data->points_chosen);
    send_to_char(buf, ch);
    sprintf(buf, "Experience per level: %d\n\r",
            exp_per_level(ch, ch->gen_data->points_chosen));
    send_to_char(buf, ch);
    return;
}

int exp_per_level(Mobile* ch, int points)
{
    int expl, inc;

    if (IS_NPC(ch)) 
        return 1000;

    expl = 1000;
    inc = 500;

    int mult = GET_ELEM(&race_table[ch->race].class_mult, ch->ch_class);

    if (points < 40)
        return 1000 * (mult ?
            mult / 100 : 1);

    /* processing */
    points -= 40;

    while (points > 9) {
        expl += inc;
        points -= 10;
        if (points > 9) {
            expl += inc;
            inc *= 2;
            points -= 10;
        }
    }

    expl += points * inc / 10;

    return expl * mult / 100;
}

/* this procedure handles the input parsing for the skill generator */
bool parse_gen_groups(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    SKNUM gn, sn;
    int i;

    if (argument[0] == '\0')
        return false;

    READ_ARG(arg);

    if (!str_prefix(arg, "help")) {
        if (argument[0] == '\0') {
            do_function(ch, &do_help, "group help");
            return true;
        }

        do_function(ch, &do_help, argument);
        return true;
    }

    if (!str_prefix(arg, "add")) {
        if (argument[0] == '\0') {
            send_to_char("You must provide a skill name.\n\r", ch);
            return true;
        }

        gn = group_lookup(argument);
        if (gn != -1) {
            if (ch->gen_data->group_chosen[gn] || ch->pcdata->group_known[gn]) {
                send_to_char("You already know that group!\n\r", ch);
                return true;
            }

            if (SKILL_GROUP_RATING(gn, ch) < 1) {
                send_to_char("That group is not available.\n\r", ch);
                return true;
            }

            /* Close security hole */
            if (ch->gen_data->points_chosen + SKILL_GROUP_RATING(gn, ch) > 300) {
                send_to_char("You cannot take more than 300 creation points.\n\r", ch);
                return true;
            }

            sprintf(buf, "%s group added\n\r", skill_group_table[gn].name);
            send_to_char(buf, ch);
            ch->gen_data->group_chosen[gn] = true;
            ch->gen_data->points_chosen += SKILL_GROUP_RATING(gn, ch);
            gn_add(ch, gn);
            ch->pcdata->points += SKILL_GROUP_RATING(gn, ch);
            return true;
        }

        sn = skill_lookup(argument);
        if (sn != -1) {
            if (ch->gen_data->skill_chosen[sn] || ch->pcdata->learned[sn] > 0) {
                send_to_char("You already know that skill!\n\r", ch);
                return true;
            }

            if (SKILL_RATING(sn, ch) < 1
                || HAS_SPELL_FUNC(sn)) {
                send_to_char("That skill is not available.\n\r", ch);
                return true;
            }

            /* Close security hole */
            if (ch->gen_data->points_chosen + SKILL_RATING(sn, ch) > 300) {
                send_to_char("You cannot take more than 300 creation points.\n\r", ch);
                return true;
            }
            sprintf(buf, "%s skill added\n\r", skill_table[sn].name);
            send_to_char(buf, ch);
            ch->gen_data->skill_chosen[sn] = true;
            ch->gen_data->points_chosen += SKILL_RATING(sn, ch);
            ch->pcdata->learned[sn] = 1;
            ch->pcdata->points += SKILL_RATING(sn, ch);
            return true;
        }

        send_to_char("No skills or groups by that name...\n\r", ch);
        return true;
    }

    if (!strcmp(arg, "drop")) {
        if (argument[0] == '\0') {
            send_to_char("You must provide a skill to drop.\n\r", ch);
            return true;
        }

        gn = group_lookup(argument);
        if (gn != -1 && ch->gen_data->group_chosen[gn]) {
            send_to_char("Group dropped.\n\r", ch);
            ch->gen_data->group_chosen[gn] = false;
            ch->gen_data->points_chosen -= SKILL_GROUP_RATING(gn, ch);
            gn_remove(ch, gn);
            for (i = 0; i < skill_group_count; i++) {
                if (ch->gen_data->group_chosen[gn]) gn_add(ch, gn);
            }
            ch->pcdata->points -= SKILL_GROUP_RATING(gn, ch);
            return true;
        }

        sn = skill_lookup(argument);
        if (sn != -1 && ch->gen_data->skill_chosen[sn]) {
            send_to_char("Skill dropped.\n\r", ch);
            ch->gen_data->skill_chosen[sn] = false;
            ch->gen_data->points_chosen -= SKILL_RATING(sn, ch);
            ch->pcdata->learned[sn] = 0;
            ch->pcdata->points -= SKILL_RATING(sn, ch);
            return true;
        }

        send_to_char("You haven't bought any such skill or group.\n\r", ch);
        return true;
    }

    if (!str_prefix(arg, "premise")) {
        do_function(ch, &do_help, "premise");
        return true;
    }

    if (!str_prefix(arg, "list")) {
        list_group_costs(ch);
        return true;
    }

    if (!str_prefix(arg, "learned")) {
        list_group_chosen(ch);
        return true;
    }

    if (!str_prefix(arg, "info")) {
        do_function(ch, &do_groups, argument);
        return true;
    }

    return false;
}

/* shows all groups, or the sub-members of a group */
void do_groups(Mobile* ch, char* argument)
{
    char buf[100];
    SKNUM gn, sn;
    int col;

    if (IS_NPC(ch)) return;

    col = 0;

    if (argument[0] == '\0') { /* show all groups */

        for (gn = 0; gn < skill_group_count; gn++) {
            if (skill_group_table[gn].name == NULL) break;
            if (ch->pcdata->group_known[gn]) {
                sprintf(buf, "%-20s ", skill_group_table[gn].name);
                send_to_char(buf, ch);
                if (++col % 3 == 0) send_to_char("\n\r", ch);
            }
        }
        if (col % 3 != 0) send_to_char("\n\r", ch);
        sprintf(buf, "Creation points: %d\n\r", ch->pcdata->points);
        send_to_char(buf, ch);
        return;
    }

    if (!str_cmp(argument, "all")) /* show all groups */
    {
        for (gn = 0; gn < skill_group_count; gn++) {
            if (skill_group_table[gn].name == NULL) break;
            sprintf(buf, "%-20s ", skill_group_table[gn].name);
            send_to_char(buf, ch);
            if (++col % 3 == 0) send_to_char("\n\r", ch);
        }
        if (col % 3 != 0) send_to_char("\n\r", ch);
        return;
    }

    /* show the sub-members of a group */
    gn = group_lookup(argument);
    if (gn == -1) {
        send_to_char("No group of that name exist.\n\r", ch);
        send_to_char("Type 'groups all' or 'info all' for a full listing.\n\r",
                     ch);
        return;
    }

    for (sn = 0; sn < MAX_IN_GROUP; sn++) {
        if (skill_group_table[gn].skills[sn] == NULL) break;
        sprintf(buf, "%-20s ", skill_group_table[gn].skills[sn]);
        send_to_char(buf, ch);
        if (++col % 3 == 0) send_to_char("\n\r", ch);
    }
    if (col % 3 != 0) send_to_char("\n\r", ch);
}

/* checks for skill improvement */
void check_improve(Mobile* ch, SKNUM sn, bool success, int multiplier)
{
    int chance;
    char buf[100];

    if (IS_NPC(ch))
        return;

    if (ch->level < SKILL_RATING(sn, ch)
        || SKILL_RATING(sn, ch) == 0
        || ch->pcdata->learned[sn] == 0 || ch->pcdata->learned[sn] == 100)
        return; /* skill is not known */

    /* check to see if the character has a chance to learn */
    chance = 10 * int_mod[get_curr_stat(ch, STAT_INT)].learn;
    chance /= (multiplier * SKILL_RATING(sn, ch) * 4);
    chance += ch->level;

    if (number_range(1, 1000) > chance) 
        return;

    /* now that the character has a CHANCE to learn, see if they really have */

    if (success) {
        chance = URANGE(5, 100 - ch->pcdata->learned[sn], 95);
        if (number_percent() < chance) {
            sprintf(buf, "You have become better at %s!\n\r",
                    skill_table[sn].name);
            send_to_char(buf, ch);
            ch->pcdata->learned[sn]++;
            gain_exp(ch, 2 * SKILL_RATING(sn, ch));
        }
    }

    else {
        chance = URANGE(5, ch->pcdata->learned[sn] / 2, 30);
        if (number_percent() < chance) {
            sprintf(
                buf,
                "You learn from your mistakes, and your %s skill improves.\n\r",
                skill_table[sn].name);
            send_to_char(buf, ch);
            ch->pcdata->learned[sn] += (int16_t)number_range(1, 3);
            ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn], 100);
            gain_exp(ch, 2 * SKILL_RATING(sn, ch));
        }
    }
}

/* returns a group index number given the name */
SKNUM group_lookup(const char* name)
{
    SKNUM gn;

    for (gn = 0; gn < skill_group_count; gn++) {
        if (skill_group_table[gn].name == NULL) break;
        if (LOWER(name[0]) == LOWER(skill_group_table[gn].name[0])
            && !str_prefix(name, skill_group_table[gn].name))
            return gn;
    }

    return -1;
}

/* recursively adds a group given its number -- uses group_add */
void gn_add(Mobile* ch, SKNUM gn)
{
    int i;

    ch->pcdata->group_known[gn] = true;
    for (i = 0; i < MAX_IN_GROUP; i++) {
        if (skill_group_table[gn].skills[i] == NULL) 
            break;
        group_add(ch, skill_group_table[gn].skills[i], false);
    }
}

/* recusively removes a group given its number -- uses group_remove */
void gn_remove(Mobile* ch, SKNUM gn)
{
    int i;

    ch->pcdata->group_known[gn] = false;

    for (i = 0; i < MAX_IN_GROUP; i++) {
        if (skill_group_table[gn].skills[i] == NULL) 
            break;
        group_remove(ch, skill_group_table[gn].skills[i]);
    }
}

/* use for processing a skill or group for addition  */
void group_add(Mobile* ch, const char* name, bool deduct)
{
    SKNUM sn, gn;

    if (IS_NPC(ch)) /* NPCs do not have skills */
        return;

    sn = skill_lookup(name);

    if (sn != -1) {
        if (ch->pcdata->learned[sn] == 0) {
            /* i.e. not known */
            ch->pcdata->learned[sn] = 1;
            if (deduct) 
                ch->pcdata->points += SKILL_RATING(sn, ch);
        }
        return;
    }

    /* now check groups */

    gn = group_lookup(name);

    if (gn != -1) {
        if (ch->pcdata->group_known[gn] == false) {
            ch->pcdata->group_known[gn] = true;
            if (deduct) 
                ch->pcdata->points += SKILL_GROUP_RATING(gn, ch);
        }
        gn_add(ch, gn); /* make sure all skills in the group are known */
    }
}

/* used for processing a skill or group for deletion -- no points back! */

void group_remove(Mobile* ch, const char* name)
{
    SKNUM sn, gn;

    sn = skill_lookup(name);

    if (sn != -1) {
        ch->pcdata->learned[sn] = 0;
        return;
    }

    /* now check groups */

    gn = group_lookup(name);

    if (gn != -1 && ch->pcdata->group_known[gn] == true) {
        ch->pcdata->group_known[gn] = false;
        gn_remove(ch, gn); /* be sure to call gn_add on all remaining groups */
    }
}

int race_exp_per_level(int race, int class_, int points)
{
    int expl, inc;
    expl = 1000;
    inc = 500;
    int mult = GET_ELEM(&race_table[race].class_mult, class_);
    
    if (points < 40)
        return 1000 * (mult ? mult / 100 : 1);

 /* processing */
    points -= 40;

    while (points > 9) {
        expl += inc;
        points -= 10;
        if (points > 9) {
            expl += inc;
            inc *= 2;
            points -= 10;
        }
    }

    expl += points * inc / 10;

    return expl * mult / 100;
}
