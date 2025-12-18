////////////////////////////////////////////////////////////////////////////////
// combat_rom.c
// ROM 2.4 / THAC0 combat implementation.
//
// This implements the ROM combat system using the CombatOps interface.
// Combat logic has been extracted from fight.c to enable testing and alternate
// combat systems.
////////////////////////////////////////////////////////////////////////////////

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

#include "combat_ops.h"

#include "act_comm.h"
#include "act_info.h"
#include "comm.h"
#include "db.h"
#include "fight.h"
#include "handler.h"
#include "interp.h"
#include "lookup.h"
#include "recycle.h"
#include "rng.h"
#include "update.h"

#include <entities/affect.h>
#include <entities/descriptor.h>
#include <entities/event.h>
#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/player_data.h>
#include <entities/room.h>

#include <data/class.h>
#include <data/events.h>
#include <data/quest.h>
#include <data/race.h>

#include <stdio.h>
#include <string.h>

// Forward declarations of fight.c helper functions
extern void dam_message(Mobile* ch, Mobile* victim, int dam, int16_t dt, bool immune);
extern void death_cry(Mobile* ch);
extern void make_corpse(Mobile* ch);
extern void update_pos(Mobile* victim);
extern void group_gain(Mobile* ch, Mobile* victim);
extern void wiznet(const char* string, Mobile* ch, Object* obj, FLAGS flag, FLAGS flag_skip, LEVEL min_level);

////////////////////////////////////////////////////////////////////////////////
// ROM COMBAT OPERATIONS
////////////////////////////////////////////////////////////////////////////////

static bool rom_apply_damage(Mobile* ch, Mobile* victim, int dam, 
                              int16_t dt, DamageType dam_type, bool show)
{
    Object* corpse;
    bool immune;

    if (victim->position == POS_DEAD)
        return false;

    // Stop up any residual loopholes.
    if (dam > 1200 && dt >= TYPE_HIT) {
        bug("Damage: %d: more than 1200 points!", dam);
        dam = 1200;
        if (!IS_IMMORTAL(ch)) {
            Object* obj;
            obj = get_eq_char(ch, WEAR_WIELD);
            send_to_char("You really shouldn't cheat.\n\r", ch);
            if (obj != NULL) extract_obj(obj);
        }
    }

    /* damage reduction */
    if (dam > 35) 
        dam = (dam - 35) / 2 + 35;
    if (dam > 80) 
        dam = (dam - 80) / 2 + 80;

    if (victim != ch) {
        /*
         * Certain attacks are forbidden.
         * Most other attacks are returned.
         */
        if (is_safe(ch, victim)) 
            return false;

        check_killer(ch, victim);

        if (victim->position > POS_STUNNED) {
            if (victim->fighting == NULL) {
                set_fighting(victim, ch);
                if (IS_NPC(victim) && HAS_MPROG_TRIGGER(victim, TRIG_ATTACKED))
                    mp_percent_trigger(victim, ch, NULL, NULL, TRIG_ATTACKED);
                if (IS_NPC(victim) && HAS_EVENT_TRIGGER(victim, TRIG_ATTACKED))
                    raise_attacked_event(victim, ch, number_percent());
            }
            if (victim->timer <= 4)
                victim->position = POS_FIGHTING;
        }

        if (victim->position > POS_STUNNED) {
            if (ch->fighting == NULL) 
                set_fighting(ch, victim);
        }

        // More charm stuff.
        if (victim->master == ch) 
            stop_follower(victim);
    }

    // Inviso attacks ... not.
    if (IS_AFFECTED(ch, AFF_INVISIBLE)) {
        affect_strip(ch, gsn_invis);
        affect_strip(ch, gsn_mass_invis);
        REMOVE_BIT(ch->affect_flags, AFF_INVISIBLE);
        act("$n fades into existence.", ch, NULL, NULL, TO_ROOM);
    }

    // Damage modifiers.

    if (dam > 1 && !IS_NPC(victim)
        && victim->pcdata->condition[COND_DRUNK] > 10)
        dam = 9 * dam / 10;

    if (dam > 1 && IS_AFFECTED(victim, AFF_SANCTUARY)) 
        dam /= 2;

    if (dam > 1
        && ((IS_AFFECTED(victim, AFF_PROTECT_EVIL) && IS_EVIL(ch))
            || (IS_AFFECTED(victim, AFF_PROTECT_GOOD) && IS_GOOD(ch))))
        dam -= dam / 4;

    immune = false;

    // Check for parry, and dodge.
    if (dt >= TYPE_HIT && ch != victim) {
        if (check_parry(ch, victim)) 
            return false;
        if (check_dodge(ch, victim)) 
            return false;
        if (check_shield_block(ch, victim)) 
            return false;
    }

    switch (check_immune(victim, dam_type)) {
    case IS_IMMUNE:
        immune = true;
        dam = 0;
        break;
    case IS_RESISTANT:
        dam -= dam / 3;
        break;
    case IS_VULNERABLE:
        dam += dam / 2;
        break;
    default:
        break;
    }

    if (show) 
        dam_message(ch, victim, dam, dt, immune);

    if (dam == 0) 
        return false;

    /*
     * Hurt the victim.
     * Inform the victim of his new state.
     */
    victim->hit -= (int16_t)dam;
    if (!IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL && victim->hit < 1)
        victim->hit = 1;
    update_pos(victim);

    switch (victim->position) {
    case POS_MORTAL:
        act("$n is mortally wounded, and will die soon, if not aided.", victim,
            NULL, NULL, TO_ROOM);
        send_to_char(
            "You are mortally wounded, and will die soon, if not aided.\n\r",
            victim);
        break;

    case POS_INCAP:
        act("$n is incapacitated and will slowly die, if not aided.", victim,
            NULL, NULL, TO_ROOM);
        send_to_char(
            "You are incapacitated and will slowly die, if not aided.\n\r",
            victim);
        break;

    case POS_STUNNED:
        act("$n is stunned, but will probably recover.", victim, NULL, NULL,
            TO_ROOM);
        send_to_char("You are stunned, but will probably recover.\n\r", victim);
        break;

    case POS_DEAD:
        act("$n is DEAD!!", victim, NULL, NULL, TO_ROOM);
        send_to_char("You have been KILLED!!\n\r\n\r", victim);
        break;

    default:
        if (dam > victim->max_hit / 4)
            send_to_char("That really did HURT!\n\r", victim);
        if (victim->hit < victim->max_hit / 4)
            send_to_char("You sure are BLEEDING!\n\r", victim);
        break;
    }

    // Sleep spells and extremely wounded folks.
    if (!IS_AWAKE(victim)) 
        stop_fighting(victim, false);

    // Payoff for killing things.
    if (victim->position == POS_DEAD) {
        group_gain(ch, victim);

        if (!IS_NPC(victim)) {
            sprintf(log_buf, "%s killed by %s at %d", NAME_STR(victim),
                    (IS_NPC(ch) ? ch->short_descr : NAME_STR(ch)),
                    VNUM_FIELD(ch->in_room));
            log_string(log_buf);

            /*
             * Dying penalty:
             * 2/3 way back to previous level.
             */
            if (victim->exp
                > exp_per_level(victim, victim->pcdata->points) * victim->level)
                gain_exp(victim,
                         (2
                          * (exp_per_level(victim, victim->pcdata->points)
                                 * victim->level
                             - victim->exp)
                          / 3)
                             + 50);
        }
        else if (ch->pcdata) {
            QuestTarget* qt;
            QuestStatus* qs;
            if ((qt = get_quest_targ_mob(ch, VNUM_FIELD(victim->prototype))) != NULL
                && (qs = get_quest_status(ch, qt->quest_vnum)) != NULL) {
                if (qs->quest->type == QUEST_KILL_MOB && qs->progress < qs->amount) {
                    ++qs->progress;
                    printf_to_char(ch, COLOR_INFO "Quest Progress: " COLOR_CLEAR "%s " COLOR_DECOR_1 "(" COLOR_ALT_TEXT_1 "%d" COLOR_DECOR_1 "/" COLOR_ALT_TEXT_1 "%d" COLOR_DECOR_1 ")" COLOR_EOL,
                        qs->quest->name, qs->progress, qs->amount);
                }
            }
        }

        sprintf(log_buf, "%s got toasted by %s at %s [room %d]",
                (IS_NPC(victim) ? victim->short_descr : NAME_STR(victim)),
                (IS_NPC(ch) ? ch->short_descr : NAME_STR(ch)), 
                NAME_STR(ch->in_room),
                VNUM_FIELD(ch->in_room));

        if (IS_NPC(victim))
            wiznet(log_buf, NULL, NULL, WIZ_MOBDEATHS, 0, 0);
        else
            wiznet(log_buf, NULL, NULL, WIZ_DEATHS, 0, 0);

        // Death trigger
        if (IS_NPC(victim) && HAS_MPROG_TRIGGER(victim, TRIG_DEATH)) {
            victim->position = POS_STANDING;
            mp_percent_trigger(victim, ch, NULL, NULL, TRIG_DEATH);
        }

        if (IS_NPC(victim) && HAS_EVENT_TRIGGER(victim, TRIG_DEATH)) {
            victim->position = POS_STANDING;
            raise_death_event(victim, ch);
        }

        combat->handle_death(victim);
        
        /* dump the flags */
        if (ch != victim && !IS_NPC(ch) && !is_same_clan(ch, victim)) {
            if (IS_SET(victim->act_flags, PLR_KILLER))
                REMOVE_BIT(victim->act_flags, PLR_KILLER);
            else
                REMOVE_BIT(victim->act_flags, PLR_THIEF);
        }

        /* RT new auto commands */

        if (!IS_NPC(ch)
            && (corpse = get_obj_list(ch, "corpse", &ch->in_room->objects))
                   != NULL
            && corpse->item_type == ITEM_CORPSE_NPC
            && can_see_obj(ch, corpse)) {
            Object* coins;

            corpse = get_obj_list(ch, "corpse", &ch->in_room->objects);

            if (IS_SET(ch->act_flags, PLR_AUTOLOOT) && corpse
                && OBJ_HAS_OBJS(corpse)) /* exists and not empty */
            {
                do_function(ch, &do_get, "all corpse");
            }

            if (IS_SET(ch->act_flags, PLR_AUTOGOLD) && corpse && OBJ_HAS_OBJS(corpse)
                && /* exists and not empty */
                !IS_SET(ch->act_flags, PLR_AUTOLOOT)) {
                if ((coins = get_obj_list(ch, "gcash", &corpse->objects))
                    != NULL) {
                    do_function(ch, &do_get, "all.gcash corpse");
                }
            }

            if (IS_SET(ch->act_flags, PLR_AUTOSAC)) {
                if (IS_SET(ch->act_flags, PLR_AUTOLOOT) && corpse
                    && OBJ_HAS_OBJS(corpse)) {
                    return true; /* leave if corpse has treasure */
                }
                else {
                    do_function(ch, &do_sacrifice, "corpse");
                }
            }
        }

        return true;
    }

    if (victim == ch)
        return true;

    // Take care of link dead people.
    if (!IS_NPC(victim) && victim->desc == NULL) {
        if (number_range(0, victim->wait) == 0) {
            do_function(victim, &do_recall, "");
            return true;
        }
    }

    // Wimp out?
    if (IS_NPC(victim) && dam > 0 && victim->wait < PULSE_VIOLENCE / 2) {
        if ((IS_SET(victim->act_flags, ACT_WIMPY) && number_bits(2) == 0
             && victim->hit < victim->max_hit / 5)
            || (IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL
                && victim->master->in_room != victim->in_room)) {
            do_function(victim, &do_flee, "");
        }
    }

    if (!IS_NPC(victim) && victim->hit > 0 && victim->hit <= victim->wimpy
        && victim->wait < PULSE_VIOLENCE / 2) {
        do_function(victim, &do_flee, "");
    }

    return true;
}

static void rom_apply_healing(Mobile* ch, int amount)
{
    if (amount <= 0)
        return;
    
    ch->hit += (int16_t)amount;
    if (ch->hit > ch->max_hit)
        ch->hit = ch->max_hit;
    
    update_pos(ch);
}

static void rom_apply_regen(Mobile* ch, int hp_gain, int mana_gain, int move_gain)
{
    // HP regeneration
    if (hp_gain > 0) {
        ch->hit += (int16_t)hp_gain;
        if (ch->hit > ch->max_hit)
            ch->hit = ch->max_hit;
    }
    
    // Mana regeneration
    if (mana_gain > 0) {
        ch->mana += (int16_t)mana_gain;
        if (ch->mana > ch->max_mana)
            ch->mana = ch->max_mana;
    }
    
    // Move regeneration
    if (move_gain > 0) {
        ch->move += (int16_t)move_gain;
        if (ch->move > ch->max_move)
            ch->move = ch->max_move;
    }
    
    update_pos(ch);
}

static void rom_drain_mana(Mobile* ch, int amount)
{
    ch->mana -= (int16_t)amount;
    if (ch->mana < 0)
        ch->mana = 0;
}

static void rom_drain_move(Mobile* ch, int amount)
{
    ch->move -= (int16_t)amount;
    if (ch->move < 0)
        ch->move = 0;
}

static bool rom_check_hit(Mobile* ch, Mobile* victim, Object* weapon, int16_t sn)
{
    // TODO: Extract THAC0 logic from one_hit() in fight.c
    // For now, return true (we're not changing behavior yet)
    (void)ch;
    (void)victim;
    (void)weapon;
    (void)sn;
    return true;
}

static int rom_calculate_damage(Mobile* ch, Mobile* victim, Object* weapon, 
                                int16_t sn, bool is_offhand)
{
    // TODO: Extract damage calculation from one_hit() in fight.c
    // For now, return 0 (we're not changing behavior yet)
    (void)ch;
    (void)victim;
    (void)weapon;
    (void)sn;
    (void)is_offhand;
    return 0;
}

static void rom_handle_death(Mobile* victim)
{
    int i;

    stop_fighting(victim, true);
    death_cry(victim);
    make_corpse(victim);

    if (IS_NPC(victim)) {
        victim->prototype->killed++;
        kill_table[URANGE(0, victim->level, MAX_LEVEL - 1)].killed++;
        extract_char(victim, true);
        return;
    }

    extract_char(victim, false);
    while (victim->affected) 
        affect_remove(victim, victim->affected);
    victim->affect_flags = race_table[victim->race].aff;
    for (i = 0; i < 4; i++) 
        victim->armor[i] = 100;
    victim->position = POS_RESTING;
    victim->hit = UMAX(1, victim->hit);
    victim->mana = UMAX(1, victim->mana);
    victim->move = UMAX(1, victim->move);
    /*  save_char_obj( victim ); we're stable enough to not need this :) */
}

////////////////////////////////////////////////////////////////////////////////
// ROM COMBAT OPS TABLE
////////////////////////////////////////////////////////////////////////////////

CombatOps combat_rom = {
    .apply_damage = rom_apply_damage,
    .apply_healing = rom_apply_healing,
    .apply_regen = rom_apply_regen,
    .drain_mana = rom_drain_mana,
    .drain_move = rom_drain_move,
    .check_hit = rom_check_hit,
    .calculate_damage = rom_calculate_damage,
    .handle_death = rom_handle_death,
};

// Global combat pointer (defaults to ROM implementation)
CombatOps* combat = &combat_rom;
