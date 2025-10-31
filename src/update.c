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

#include "update.h"

#include "act_info.h"
#include "act_move.h"
#include "act_obj.h"
#include "act_wiz.h"
#include "comm.h"
#include "config.h"
#include "db.h"
#include "fight.h"
#include "handler.h"
#include "interp.h"
#include "magic.h"
#include "mob_prog.h"
#include "music.h"
#include "save.h"
#include "skills.h"
#include "weather.h"

#include <entities/descriptor.h>
#include <entities/event.h>
#include <entities/object.h>
#include <entities/player_data.h>

#include <data/mobile_data.h>
#include <data/race.h>
#include <data/skill.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

// Local functions.
int hit_gain args((Mobile * ch));
int mana_gain args((Mobile * ch));
int move_gain args((Mobile * ch));
void mobile_update args((void));
void char_update args((void));
void obj_update args((void));
void aggr_update();

/* used for saving */

int save_number = 0;

// Advancement stuff.
void advance_level(Mobile* ch, bool hide)
{
    char buf[MAX_STRING_LENGTH];
    int16_t add_hp;
    int16_t add_mana;
    int16_t add_move;
    int16_t add_prac;

    ch->pcdata->last_level
        = (int16_t)((ch->played + (current_time - ch->logon)) / 3600);

    const char* title = class_table[ch->ch_class].titles[ch->level][ch->sex == SEX_FEMALE ? 1 : 0];
    if (title && title[0]) {
        sprintf(buf, "the %s", title);
        set_title(ch, buf);
    }

    add_hp = con_mod[get_curr_stat(ch, STAT_CON)].hitp
             + (int16_t)number_range(class_table[ch->ch_class].hp_min,
                            class_table[ch->ch_class].hp_max);
    add_mana = (int16_t)number_range(2, 
        (2 * get_curr_stat(ch, STAT_INT) + get_curr_stat(ch, STAT_WIS)) / 5);
    if (!class_table[ch->ch_class].fMana) 
        add_mana /= 2;
    add_move = (int16_t)number_range(
        1, (get_curr_stat(ch, STAT_CON) + get_curr_stat(ch, STAT_DEX)) / 6);
    add_prac = wis_mod[get_curr_stat(ch, STAT_WIS)].practice;

    add_hp = add_hp * 9 / 10;
    add_mana = add_mana * 9 / 10;
    add_move = add_move * 9 / 10;

    add_hp = UMAX(2, add_hp);
    add_mana = UMAX(2, add_mana);
    add_move = UMAX(6, add_move);

    ch->max_hit += add_hp;
    ch->max_mana += add_mana;
    ch->max_move += add_move;
    ch->practice += add_prac;
    ch->train += 1;

    ch->pcdata->perm_hit += add_hp;
    ch->pcdata->perm_mana += add_mana;
    ch->pcdata->perm_move += add_move;

    if (!hide) {
        sprintf(
            buf,
            "You gain %d hit point%s, %d mana, %d move, and %d practice%s.\n\r",
            add_hp, add_hp == 1 ? "" : "s", add_mana, add_move, add_prac,
            add_prac == 1 ? "" : "s");
        send_to_char(buf, ch);
    }
    return;
}

void gain_exp(Mobile* ch, int gain)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch) || ch->level >= LEVEL_HERO) 
        return;

    ch->exp = UMAX(exp_per_level(ch, ch->pcdata->points), ch->exp + gain);
    while (ch->level < LEVEL_HERO
           && ch->exp
                  >= exp_per_level(ch, ch->pcdata->points) * (ch->level + 1)) {
        send_to_char("You raise a level!!  ", ch);
        ch->level += 1;
        sprintf(buf, "%s gained level %d", NAME_STR(ch), ch->level);
        log_string(buf);
        sprintf(buf, "$N has attained level %d!", ch->level);
        wiznet(buf, ch, NULL, WIZ_LEVELS, 0, 0);
        advance_level(ch, false);
        save_char_obj(ch);
    }

    return;
}

// Regeneration stuff.
int hit_gain(Mobile* ch)
{
    int gain;
    int number;

    if (ch->in_room == NULL) return 0;

    if (IS_NPC(ch)) {
        gain = 5 + ch->level;
        if (IS_AFFECTED(ch, AFF_REGENERATION)) gain *= 2;

        switch (ch->position) {
        default:
            gain /= 2;
            break;
        case POS_SLEEPING:
            gain = 3 * gain / 2;
            break;
        case POS_RESTING:
            break;
        case POS_FIGHTING:
            gain /= 3;
            break;
        }
    }
    else {
        gain = UMAX(3, get_curr_stat(ch, STAT_CON) - 3 + ch->level / 2);
        gain += class_table[ch->ch_class].hp_max - 10;
        number = number_percent();
        if (number < get_skill(ch, gsn_fast_healing)) {
            gain += number * gain / 100;
            if (ch->hit < ch->max_hit)
                check_improve(ch, gsn_fast_healing, true, 8);
        }

        switch (ch->position) {
        default:
            gain /= 4;
            break;
        case POS_SLEEPING:
            break;
        case POS_RESTING:
            gain /= 2;
            break;
        case POS_FIGHTING:
            gain /= 6;
            break;
        }

        if (ch->pcdata->condition[COND_HUNGER] == 0) 
            gain /= 2;

        if (ch->pcdata->condition[COND_THIRST] == 0) 
            gain /= 2;
    }

    gain = gain * ch->in_room->data->heal_rate / 100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        gain = gain * ch->on->value[3] / 100;

    if (IS_AFFECTED(ch, AFF_POISON)) 
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE)) 
        gain /= 8;

    if (IS_AFFECTED(ch, AFF_HASTE) || IS_AFFECTED(ch, AFF_SLOW)) 
        gain /= 2;

    return UMIN(gain, ch->max_hit - ch->hit);
}

int mana_gain(Mobile* ch)
{
    int gain;
    int number;

    if (ch->in_room == NULL) return 0;

    if (IS_NPC(ch)) {
        gain = 5 + ch->level;
        switch (ch->position) {
        default:
            gain /= 2;
            break;
        case POS_SLEEPING:
            gain = 3 * gain / 2;
            break;
        case POS_RESTING:
            break;
        case POS_FIGHTING:
            gain /= 3;
            break;
        }
    }
    else {
        gain = (get_curr_stat(ch, STAT_WIS) + get_curr_stat(ch, STAT_INT)
                + ch->level)
               / 2;
        number = number_percent();
        if (number < get_skill(ch, gsn_meditation)) {
            gain += number * gain / 100;
            if (ch->mana < ch->max_mana)
                check_improve(ch, gsn_meditation, true, 8);
        }
        if (!class_table[ch->ch_class].fMana) gain /= 2;

        switch (ch->position) {
        default:
            gain /= 4;
            break;
        case POS_SLEEPING:
            break;
        case POS_RESTING:
            gain /= 2;
            break;
        case POS_FIGHTING:
            gain /= 6;
            break;
        }

        if (ch->pcdata->condition[COND_HUNGER] == 0) gain /= 2;

        if (ch->pcdata->condition[COND_THIRST] == 0) gain /= 2;
    }

    gain = gain * ch->in_room->data->mana_rate / 100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        gain = gain * ch->on->value[4] / 100;

    if (IS_AFFECTED(ch, AFF_POISON)) gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE)) gain /= 8;

    if (IS_AFFECTED(ch, AFF_HASTE) || IS_AFFECTED(ch, AFF_SLOW)) gain /= 2;

    return UMIN(gain, ch->max_mana - ch->mana);
}

int move_gain(Mobile* ch)
{
    int gain;

    if (ch->in_room == NULL) return 0;

    if (IS_NPC(ch)) { gain = ch->level; }
    else {
        gain = UMAX(15, ch->level);

        switch (ch->position) {
        case POS_SLEEPING:
            gain += get_curr_stat(ch, STAT_DEX);
            break;
        case POS_RESTING:
            gain += get_curr_stat(ch, STAT_DEX) / 2;
            break;
        default:
            break;
        }

        if (ch->pcdata->condition[COND_HUNGER] == 0) 
            gain /= 2;

        if (ch->pcdata->condition[COND_THIRST] == 0) 
            gain /= 2;
    }

    gain = gain * ch->in_room->data->heal_rate / 100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        gain = gain * ch->on->value[3] / 100;

    if (IS_AFFECTED(ch, AFF_POISON)) 
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE)) 
        gain /= 8;

    if (IS_AFFECTED(ch, AFF_HASTE) || IS_AFFECTED(ch, AFF_SLOW)) 
        gain /= 2;

    return UMIN(gain, ch->max_move - ch->move);
}

void gain_condition(Mobile* ch, int iCond, int value)
{
    int condition;

    if (value == 0 || IS_NPC(ch) || ch->level >= LEVEL_IMMORTAL) 
        return;

    condition = ch->pcdata->condition[iCond];
    if (condition == -1) 
        return;
    ch->pcdata->condition[iCond] = URANGE(0, (int16_t)(condition + value), 48);

    if (ch->pcdata->condition[iCond] == 0) {
        switch (iCond) {
        case COND_HUNGER:
            send_to_char("You are hungry.\n\r", ch);
            break;

        case COND_THIRST:
            send_to_char("You are thirsty.\n\r", ch);
            break;

        case COND_DRUNK:
            if (condition != 0) 
                send_to_char("You are sober.\n\r", ch);
            break;
        }
    }

    return;
}

static void update_msdp_vars(Descriptor* d)
{
    msdp_update_var(d, "ALIGNMENT", "%d", CH(d)->alignment);
    msdp_update_var(d, "EXPERIENCE", "%d", CH(d)->exp);
    //msdp_update_var(d, "EXPERIENCE_MAX", "%d", exp_per_level(CH(d)->ch_class, CH(d)->level) - exp_per_level(CH(d)->ch_class, CH(d)->level - 1));
    msdp_update_var(d, "EXPERIENCE_MAX", "%d", exp_per_level(CH(d), CH(d)->pcdata->points));
    msdp_update_var(d, "HEALTH", "%d", CH(d)->hit);
    msdp_update_var(d, "HEALTH_MAX", "%d", CH(d)->max_hit);
    msdp_update_var(d, "LEVEL", "%d", CH(d)->level);
    msdp_update_var(d, "MANA", "%d", CH(d)->mana);
    msdp_update_var(d, "MANA_MAX", "%d", CH(d)->max_mana);
    //msdp_update_var(d, "MONEY", "%d", CH(d)->gold);
    msdp_update_var(d, "MONEY", "%f", (float)CH(d)->gold + ((float)CH(d)->silver) / 100.0f);
    msdp_update_var(d, "MOVEMENT", "%d", CH(d)->move);
    msdp_update_var(d, "MOVEMENT_MAX", "%d", CH(d)->max_move);

    msdp_send_update(d);
}

/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void mobile_update()
{
    Mobile* ch = NULL;
    RoomExit* room_exit = NULL;
    int door;

    bool msdp_enabled = cfg_get_msdp_enabled();

    /* Examine all mobs. */
    FOR_EACH_GLOBAL_MOB(ch) {
        if (ch->desc && ch->desc->mth->msdp_data && msdp_enabled) {
            update_msdp_vars(ch->desc);
        }

        if (ch->pcdata || ch->in_room == NULL || IS_AFFECTED(ch, AFF_CHARM))
            continue;

        if (ch->in_room->area->empty && !IS_SET(ch->act_flags, ACT_UPDATE_ALWAYS))
            continue;

        /* Examine call for special procedure */
        if (ch->spec_fun != NULL) {
            if ((*ch->spec_fun)(ch)) 
                continue;
        }

        if (ch->prototype->pShop != NULL) /* give him some gold */
            if (((int)ch->gold * 100 + (int)ch->silver) < ch->prototype->wealth) {
                ch->gold
                    += (int16_t)(ch->prototype->wealth * number_range(1, 20) / 5000000);
                ch->silver
                    += (int16_t)(ch->prototype->wealth * number_range(1, 20) / 50000);
            }

        /*
            * Check triggers only if mobile still in default position
            */
        if (ch->position == ch->prototype->default_pos) {
            /* Delay */
            if (HAS_TRIGGER(ch, TRIG_DELAY)
                && ch->mprog_delay > 0) {
                if (--ch->mprog_delay <= 0) {
                    mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_DELAY);
                    continue;
                }
            }
            if (HAS_TRIGGER(ch, TRIG_RANDOM)) {
                if (mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_RANDOM))
                    continue;
            }
        }

        /* That's all for sleeping / busy monster, and empty zones */
        if (ch->position != POS_STANDING)
            continue;

        /* Scavenge */
        if (IS_SET(ch->act_flags, ACT_SCAVENGER) && ROOM_HAS_OBJS(ch->in_room)
            && number_bits(6) == 0) {
            Object* obj;
            Object* obj_best;
            int max;

            max = 1;
            obj_best = 0;
            FOR_EACH_ROOM_OBJ(obj, ch->in_room) {
                if (CAN_WEAR(obj, ITEM_TAKE) && can_loot(ch, obj)
                    && obj->cost > max && obj->cost > 0) {
                    obj_best = obj;
                    max = obj->cost;
                }
            }

            if (obj_best) {
                obj_from_room(obj_best);
                obj_to_char(obj_best, ch);
                act("$n gets $p.", ch, obj_best, NULL, TO_ROOM);
            }
        }

        /* Wander */
        if (!IS_SET(ch->act_flags, ACT_SENTINEL) && number_bits(3) == 0
            && (door = number_bits(5)) <= 5
            && (room_exit = ch->in_room->exit[door]) != NULL
            && room_exit->to_room != NULL && !IS_SET(room_exit->exit_flags, EX_CLOSED)
            && !IS_SET(room_exit->to_room->data->room_flags, ROOM_NO_MOB)
            && ((!IS_SET(ch->act_flags, ACT_STAY_AREA) 
                // Don't let mobs wander out of, or into, multi-instances
                || ch->in_room->area->data->inst_type == AREA_INST_MULTI
                || room_exit->to_room->area->data->inst_type == AREA_INST_MULTI)
                || room_exit->to_room->area == ch->in_room->area)
            && (!IS_SET(ch->act_flags, ACT_OUTDOORS)
                || !IS_SET(room_exit->data->to_room->room_flags, ROOM_INDOORS))
            && (!IS_SET(ch->act_flags, ACT_INDOORS)
                || IS_SET(room_exit->data->to_room->room_flags, ROOM_INDOORS))) {
            move_char(ch, door, false);
        }
    }
}

// Update all chars, including mobs.
void char_update(void)
{
    Mobile* ch;
    Mobile* ch_quit = NULL;

    ch_quit = NULL;

    /* update save counter */
    save_number++;

    if (save_number > 29)
        save_number = 0;

    FOR_EACH_GLOBAL_MOB(ch) {
        Affect* affect;
        Affect* paf_next = NULL;

        if (ch->timer > 30)
            ch_quit = ch;

        if (ch->position >= POS_STUNNED) {
            /* check to see if we need to go home */
            if (IS_NPC(ch) && ch->zone != NULL && ch->zone != ch->in_room->area
                && ch->desc == NULL && ch->fighting == NULL
                && !IS_AFFECTED(ch, AFF_CHARM) && number_percent() < 5) {
                act("$n wanders on home.", ch, NULL, NULL, TO_ROOM);
                extract_char(ch, true);
                continue;
            }

            if (ch->hit < ch->max_hit)
                ch->hit += (int16_t)hit_gain(ch);
            else
                ch->hit = ch->max_hit;

            if (ch->mana < ch->max_mana)
                ch->mana += (int16_t)mana_gain(ch);
            else
                ch->mana = ch->max_mana;

            if (ch->move < ch->max_move)
                ch->move += (int16_t)move_gain(ch);
            else
                ch->move = ch->max_move;
        }

        if (ch->position == POS_STUNNED)
            update_pos(ch);

        if (!IS_NPC(ch) && ch->level < LEVEL_IMMORTAL) {
            Object* obj;

            if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
                && obj->item_type == ITEM_LIGHT && obj->value[2] > 0) {
                if (--obj->value[2] == 0 && ch->in_room != NULL) {
                    --ch->in_room->light;
                    act("$p goes out.", ch, obj, NULL, TO_ROOM);
                    act("$p flickers and goes out.", ch, obj, NULL, TO_CHAR);
                    extract_obj(obj);
                }
                else if (obj->value[2] <= 5 && ch->in_room != NULL)
                    act("$p flickers.", ch, obj, NULL, TO_CHAR);
            }

            if (IS_IMMORTAL(ch))
                ch->timer = 0;

            if (++ch->timer >= 12) {
                if (ch->was_in_room == NULL && ch->in_room != NULL) {
                    ch->was_in_room = ch->in_room;
                    if (ch->fighting != NULL) stop_fighting(ch, true);
                    act("$n disappears into the void.", ch, NULL, NULL,
                        TO_ROOM);
                    send_to_char("You disappear into the void.\n\r", ch);
                    if (ch->level > 1)
                        save_char_obj(ch);
                    transfer_mob(ch, get_room(NULL, ROOM_VNUM_LIMBO));
                }
            }

            gain_condition(ch, COND_DRUNK, -1);
            gain_condition(ch, COND_FULL, ch->size > SIZE_MEDIUM ? -4 : -2);
            gain_condition(ch, COND_THIRST, -1);
            gain_condition(ch, COND_HUNGER, ch->size > SIZE_MEDIUM ? -2 : -1);
        }

        for (affect = ch->affected; affect != NULL; affect = paf_next) {
            paf_next = affect->next;
            if (affect->duration > 0) {
                affect->duration--;
                if (number_range(0, 4) == 0 && affect->level > 0)
                    affect->level--; /* spell strength fades with time */
            }
            else if (affect->duration < 0)
                ;
            else {
                if (paf_next == NULL || paf_next->type != affect->type
                    || paf_next->duration > 0) {
                    if (affect->type > 0 && skill_table[affect->type].msg_off) {
                        send_to_char(skill_table[affect->type].msg_off, ch);
                        send_to_char("\n\r", ch);
                    }
                }

                affect_remove(ch, affect);
            }
        }

        /*
         * Careful with the damages here,
         *   MUST NOT refer to ch after damage taken,
         *   as it may be lethal damage (on NPC).
         */

        if (is_affected(ch, gsn_plague) && ch != NULL) {
            Affect* af;
            Affect plague = { 0 };
            Mobile* vch;
            int dam;

            if (ch->in_room == NULL)
                continue;

            act("$n writhes in agony as plague sores erupt from $s skin.", ch,
                NULL, NULL, TO_ROOM);
            send_to_char("You writhe in agony from the plague.\n\r", ch);
            FOR_EACH(af, ch->affected) {
                if (af->type == gsn_plague)
                    break;
            }

            if (af == NULL) {
                REMOVE_BIT(ch->affect_flags, AFF_PLAGUE);
                continue;
            }

            if (af->level == 1)
                continue;

            plague.where = TO_AFFECTS;
            plague.type = gsn_plague;
            plague.level = af->level - 1;
            plague.duration = (int16_t)(number_range(1, 2 * plague.level));
            plague.location = APPLY_STR;
            plague.modifier = -5;
            plague.bitvector = AFF_PLAGUE;

            FOR_EACH_ROOM_MOB(vch, ch->in_room) {
                if (!saves_spell(plague.level - 2, vch, DAM_DISEASE)
                    && !IS_IMMORTAL(vch) && !IS_AFFECTED(vch, AFF_PLAGUE)
                    && number_bits(4) == 0) {
                    send_to_char("You feel hot and feverish.\n\r", vch);
                    act("$n shivers and looks very ill.", vch, NULL, NULL,
                        TO_ROOM);
                    affect_join(vch, &plague);
                }
            }

            dam = UMIN(ch->level, af->level / 5 + 1);
            ch->mana -= (int16_t)dam;
            ch->move -= (int16_t)dam;
            damage(ch, ch, dam, gsn_plague, DAM_DISEASE, false);
        }
        else if (IS_AFFECTED(ch, AFF_POISON) && ch != NULL
                 && !IS_AFFECTED(ch, AFF_SLOW)) {
            Affect* poison;

            poison = affect_find(ch->affected, gsn_poison);

            if (poison != NULL) {
                act("$n shivers and suffers.", ch, NULL, NULL, TO_ROOM);
                send_to_char("You shiver and suffer.\n\r", ch);
                damage(ch, ch, poison->level / 10 + 1, gsn_poison, DAM_POISON,
                       false);
            }
        }
        else if (ch->position == POS_INCAP && number_range(0, 1) == 0) {
            damage(ch, ch, 1, TYPE_UNDEFINED, DAM_NONE, false);
        }
        else if (ch->position == POS_MORTAL) {
            damage(ch, ch, 1, TYPE_UNDEFINED, DAM_NONE, false);
        }
    }

    /*
     * Autosave and autoquit.
     * Check that these chars still exist.
     */
    FOR_EACH_GLOBAL_MOB(ch) {
        if (ch->desc != NULL && (int)ch->desc->client->fd % 30 == save_number) {
            save_char_obj(ch);
        }

        if (ch == ch_quit) { 
            do_function(ch, &do_quit, ""); 
        }
    }

    return;
}

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update(void)
{
    Object* obj;
    Affect* affect;
    Affect* paf_next = NULL;

    FOR_EACH_GLOBAL_OBJ(obj) {
        Mobile* rch;
        char* message;

        /* go through affects and decrement */
        for (affect = obj->affected; affect != NULL; affect = paf_next) {
            paf_next = affect->next;
            if (affect->duration > 0) {
                affect->duration--;
                if (number_range(0, 4) == 0 && affect->level > 0)
                    affect->level--; /* spell strength fades with time */
            }
            else if (affect->duration < 0)
                ;
            else {
                if (paf_next == NULL || paf_next->type != affect->type
                    || paf_next->duration > 0) {
                    if (affect->type > 0 && skill_table[affect->type].msg_obj) {
                        if (obj->carried_by != NULL) {
                            rch = obj->carried_by;
                            act(skill_table[affect->type].msg_obj, rch, obj, NULL,
                                TO_CHAR);
                        }
                        if (obj->in_room != NULL
                            && ROOM_HAS_MOBS(obj->in_room)) {
                            act(skill_table[affect->type].msg_obj, obj->in_room, obj, NULL,
                                TO_ALL);
                        }
                    }
                }

                affect_remove_obj(obj, affect);
            }
        }

        if (obj->timer <= 0 || --obj->timer > 0)
            continue;

        switch (obj->item_type) {
        default:
            message = "$p crumbles into dust.";
            break;
        case ITEM_FOUNTAIN:
            message = "$p dries up.";
            break;
        case ITEM_CORPSE_NPC:
            message = "$p decays into dust.";
            break;
        case ITEM_CORPSE_PC:
            message = "$p decays into dust.";
            break;
        case ITEM_FOOD:
            message = "$p decomposes.";
            break;
        case ITEM_POTION:
            message = "$p has evaporated from disuse.";
            break;
        case ITEM_PORTAL:
            message = "$p fades out of existence.";
            break;
        case ITEM_CONTAINER:
            if (CAN_WEAR(obj, ITEM_WEAR_FLOAT))
                if (OBJ_HAS_OBJS(obj))
                    message = "$p flickers and vanishes, spilling its contents "
                              "on the floor.";
                else
                    message = "$p flickers and vanishes.";
            else
                message = "$p crumbles into dust.";
            break;
        }

        if (obj->carried_by != NULL) {
            if (IS_NPC(obj->carried_by)
                && obj->carried_by->prototype->pShop != NULL)
                obj->carried_by->silver += (int16_t)obj->cost / 5;
            else {
                act(message, obj->carried_by, obj, NULL, TO_CHAR);
                if (obj->wear_loc == WEAR_FLOAT)
                    act(message, obj->carried_by, obj, NULL, TO_ROOM);
            }
        }
        else if (obj->in_room != NULL && ROOM_HAS_MOBS(obj->in_room)) {
            if (!(obj->in_obj && VNUM_FIELD(obj->in_obj->prototype) == OBJ_VNUM_PIT
                  && !CAN_WEAR(obj->in_obj, ITEM_TAKE))) {
                act(message, obj->in_room, obj, NULL, TO_ROOM);
            }
        }

        if ((obj->item_type == ITEM_CORPSE_PC || obj->wear_loc == WEAR_FLOAT)
            && OBJ_HAS_OBJS(obj)) { 
            // save the contents
            Object* t_obj = NULL;
            FOR_EACH_OBJ_CONTENT(t_obj, obj) {
                obj_from_obj(t_obj);

                if (obj->in_obj) /* in another object */
                    obj_to_obj(t_obj, obj->in_obj);

                else if (obj->carried_by) /* carried */
                    if (obj->wear_loc == WEAR_FLOAT)
                        if (obj->carried_by->in_room == NULL)
                            extract_obj(t_obj);
                        else
                            obj_to_room(t_obj, obj->carried_by->in_room);
                    else
                        obj_to_char(t_obj, obj->carried_by);

                else if (obj->in_room == NULL) /* destroy it */
                    extract_obj(t_obj);

                else /* to a room */
                    obj_to_room(t_obj, obj->in_room);
            }
        }

        extract_obj(obj);
    }

    return;
}

/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
void aggr_update(void)
{
    for (PlayerData* wpc = player_data_list; wpc != NULL; NEXT_LINK(wpc)) {
        Mobile* wch = wpc->ch;

        if (wch->level >= LEVEL_IMMORTAL || wch->in_room == NULL)
            continue;

        Mobile* ch = NULL;
        FOR_EACH_ROOM_MOB(ch, wch->in_room) {
            int count;

            if (!IS_NPC(ch) || !IS_SET(ch->act_flags, ACT_AGGRESSIVE)
                || IS_SET(ch->in_room->data->room_flags, ROOM_SAFE)
                || IS_AFFECTED(ch, AFF_CALM) || ch->fighting != NULL
                || IS_AFFECTED(ch, AFF_CHARM) || !IS_AWAKE(ch)
                || (IS_SET(ch->act_flags, ACT_WIMPY) && IS_AWAKE(wch))
                || !can_see(ch, wch) || number_bits(1) == 0)
                continue;

            /*
             * Ok we have a 'wch' player character and a 'ch' npc aggressor.
             * Now make the aggressor fight a RANDOM pc victim in the room,
             *   giving each 'vch' an equal chance of selection.
             */
            count = 0;
            Mobile* victim = NULL;
            Mobile* vch = NULL;
            FOR_EACH_ROOM_MOB(vch, wch->in_room) {
                if (!IS_NPC(vch) && vch->level < LEVEL_IMMORTAL
                    && ch->level >= vch->level - 5
                    && (!IS_SET(ch->act_flags, ACT_WIMPY) || !IS_AWAKE(vch))
                    && can_see(ch, vch)) {
                    if (number_range(0, count) == 0)
                        victim = vch;
                    count++;
                }
            }

            if (victim == NULL)
                continue;

            multi_hit(ch, victim, TYPE_UNDEFINED);
        }
    }

    return;
}

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */

void update_handler()
{
    static int pulse_area;
    static int pulse_mobile;
    static int pulse_violence;
    static int pulse_point;
    static int pulse_music;

    if (--pulse_area <= 0) {
        pulse_area = PULSE_AREA;
        /* number_range( PULSE_AREA / 2, 3 * PULSE_AREA / 2 ); */
        area_update();
    }

    if (--pulse_music <= 0) {
        pulse_music = PULSE_MUSIC;
        song_update();
    }

    if (--pulse_mobile <= 0) {
        pulse_mobile = PULSE_MOBILE;
        mobile_update();
    }

    if (--pulse_violence <= 0) {
        pulse_violence = PULSE_VIOLENCE;
        violence_update();
    }

    if (--pulse_point <= 0) {
        wiznet("TICK!", NULL, NULL, WIZ_TICKS, 0, 0);
        pulse_point = PULSE_TICK;
        /* number_range( PULSE_TICK / 2, 3 * PULSE_TICK / 2 ); */
        update_weather_info();
        char_update();
        obj_update();
    }

    event_timer_tick();
    aggr_update();

    return;
}
