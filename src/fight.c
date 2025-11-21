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

#include "fight.h"

#include "act_comm.h"
#include "act_move.h"
#include "act_obj.h"
#include "act_wiz.h"
#include "comm.h"
#include "db.h"
#include "effects.h"
#include "handler.h"
#include "interp.h"
#include "magic.h"
#include "mob_prog.h"
#include "save.h"
#include "skills.h"
#include "update.h"

#include <entities/descriptor.h>
#include <entities/event.h>
#include <entities/faction.h>
#include <entities/object.h>
#include <entities/player_data.h>

#include <data/class.h>
#include <data/events.h>
#include <data/mobile_data.h>
#include <data/player.h>
#include <data/race.h>
#include <data/skill.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

typedef struct {
    const char* short_desc;
    const char* desc1;
    const char* desc2;
} CorpseDesc;

static const CorpseDesc corpse_descs[] = {
    { "corpse",   "The corpse of ",             " is lying here."           },
    { "corpse",   "The corpse of ",             " is lying here."           },
    { "head",     "The severed head of ",       " is lying here."           },
    { "heart",    "The torn-out heart of ",     " is lying here."           },
    { "arm",      "The sliced-off arm of ",     " is lying here."           },
    { "leg",      "The sliced-off leg of ",     " is lying here."           },
    { "guts",     "A steaming pile of ",        "'s entrails is lying here."},
    { "brains",   "The splattered brains of ",  " are lying here."          },
};

#define DESC_CORPSE         0
#define DESC_CORPSE_PC      1
#define DESC_SEVERED_HEAD   2
#define DESC_TORN_HEART     3
#define DESC_SLICED_ARM     4
#define DESC_SLICED_LEG     5
#define DESC_GUTS           6
#define DESC_BRAINS         7

#define SPRINTF_CORPSE_SDESC(b, t, n)   sprintf(b, "the %s of %s", corpse_descs[t].short_desc, n);
#define SPRINTF_CORPSE_DESC(b, t, n)   sprintf(b, "%s%s%s", corpse_descs[t].desc1, n, corpse_descs[DESC_CORPSE].desc2);

// Local functions.
void check_assist args((Mobile * ch, Mobile* victim));
bool check_dodge args((Mobile * ch, Mobile* victim));
void check_killer args((Mobile * ch, Mobile* victim));
bool check_parry args((Mobile * ch, Mobile* victim));
bool check_shield_block args((Mobile * ch, Mobile* victim));
void dam_message(Mobile * ch, Mobile* victim, int dam, int dt, bool immune);
void death_cry args((Mobile * ch));
void group_gain args((Mobile * ch, Mobile* victim));
int xp_compute args((Mobile * gch, Mobile* victim, int total_levels));
bool is_safe args((Mobile * ch, Mobile* victim));
void make_corpse args((Mobile * ch));
void one_hit(Mobile * ch, Mobile* victim, int16_t dt, bool secondary);
void mob_hit(Mobile * ch, Mobile* victim, int16_t dt);
void raw_kill args((Mobile * victim));
void set_fighting args((Mobile * ch, Mobile* victim));
void disarm args((Mobile * ch, Mobile* victim));

/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update()
{
    Mobile* ch;
    Mobile* victim;

    FOR_EACH_GLOBAL_MOB(ch) {
        if (ch->fighting == NULL || ch->in_room == NULL) 
            continue;

        victim = ch->fighting;

        if (IS_AWAKE(ch) && ch->in_room == victim->in_room)
            multi_hit(ch, victim, TYPE_UNDEFINED);
        else
            stop_fighting(ch, false);

        if ((victim = ch->fighting) == NULL)
            continue;

        // Fun for the whole family!
        check_assist(ch, victim);

        if (IS_NPC(ch)) {
            if (HAS_MPROG_TRIGGER(ch, TRIG_FIGHT))
                mp_percent_trigger(ch, victim, NULL, NULL, TRIG_FIGHT);
            // This looks wrong; should it be checking victim's HP?
            if (HAS_MPROG_TRIGGER(ch, TRIG_HPCNT))
                mp_hprct_trigger(ch, victim);
        }

        if (IS_NPC(ch)) {
            if (HAS_EVENT_TRIGGER(ch, TRIG_FIGHT))
                raise_fight_event(ch, victim, number_percent());
            if (HAS_EVENT_TRIGGER(victim, TRIG_HPCNT))
                raise_hpcnt_event(victim, ch);
        }
    }

    return;
}

/* for auto assisting */
void check_assist(Mobile* ch, Mobile* victim)
{
    Mobile* rch;

    FOR_EACH_ROOM_MOB(rch, ch->in_room) {

        if (IS_AWAKE(rch) && rch->fighting == NULL) {
            /* quick check for ASSIST_PLAYER */
            if (!IS_NPC(ch) && IS_NPC(rch)
                && IS_SET(rch->atk_flags, ASSIST_PLAYERS)
                && rch->level + 6 > victim->level) {
                do_function(rch, &do_emote, "screams and attacks!");
                multi_hit(rch, victim, TYPE_UNDEFINED);
                continue;
            }

            /* PCs next */
            if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
                if (((!IS_NPC(rch) && IS_SET(rch->act_flags, PLR_AUTOASSIST))
                     || IS_AFFECTED(rch, AFF_CHARM))
                    && is_same_group(ch, rch) && !is_safe(rch, victim))
                    multi_hit(rch, victim, TYPE_UNDEFINED);

                continue;
            }

            /* now check the NPC cases */

            if (IS_NPC(ch) && !IS_AFFECTED(ch, AFF_CHARM))

            {
                if ((IS_NPC(rch) && IS_SET(rch->atk_flags, ASSIST_ALL))

                    || (IS_NPC(rch) && rch->group && rch->group == ch->group)

                    || (IS_NPC(rch) && rch->race == ch->race
                        && IS_SET(rch->atk_flags, ASSIST_RACE))

                    || (IS_NPC(rch) && IS_SET(rch->atk_flags, ASSIST_ALIGN)
                        && ((IS_GOOD(rch) && IS_GOOD(ch))
                            || (IS_EVIL(rch) && IS_EVIL(ch))
                            || (IS_NEUTRAL(rch) && IS_NEUTRAL(ch))))

                    || (rch->prototype == ch->prototype
                        && IS_SET(rch->atk_flags, ASSIST_VNUM)))

                {
                    Mobile* vch;
                    Mobile* target;
                    int number;

                    if (number_bits(1) == 0) continue;

                    target = NULL;
                    number = 0;
                    FOR_EACH_ROOM_MOB(vch, ch->in_room) {
                        if (can_see(rch, vch) && is_same_group(vch, victim)
                            && number_range(0, number) == 0) {
                            target = vch;
                            number++;
                        }
                    }

                    if (target != NULL) {
                        do_function(rch, &do_emote, "screams and attacks!");
                        multi_hit(rch, target, TYPE_UNDEFINED);
                    }
                }
            }
        }
    }
}

// Do one group of attacks.
void multi_hit(Mobile* ch, Mobile* victim, int16_t dt)
{
    int chance;

    /* decrement the wait */
    if (ch->desc == NULL) {
        ch->wait = UMAX(0, ch->wait - PULSE_VIOLENCE);
        ch->daze = UMAX(0, ch->daze - PULSE_VIOLENCE);
    }

    /* no attacks for stunnies -- just a check */
    if (ch->position < POS_RESTING)
        return;

    if (IS_NPC(ch)) {
        mob_hit(ch, victim, dt);
        return;
    }

    one_hit(ch, victim, dt, false);

    // No double-dipping on backstab.
    if (ch->fighting != victim || dt == gsn_backstab)
        return;

    if (get_eq_char(ch, WEAR_WIELD_OH)) {
        chance = get_skill(ch, gsn_dual_wield) / 2;

        if (IS_AFFECTED(ch, AFF_SLOW))
            chance /= 2;

        if (number_percent() < chance) {
            one_hit(ch, victim, dt, true);
            check_improve(ch, gsn_dual_wield, true, 5);
            if (ch->fighting != victim)
                return;
        }
        else
            check_improve(ch, gsn_dual_wield, false, 5);
    }

    if (ch->fighting != victim)
        return;

    if (IS_AFFECTED(ch, AFF_HASTE))
        one_hit(ch, victim, dt, false);

    chance = get_skill(ch, gsn_second_attack) / 2;

    if (IS_AFFECTED(ch, AFF_SLOW)) 
        chance /= 2;

    if (number_percent() < chance) {
        one_hit(ch, victim, dt, false);
        check_improve(ch, gsn_second_attack, true, 5);
        if (ch->fighting != victim) 
            return;
    }
    else
        check_improve(ch, gsn_second_attack, false, 5);

    chance = get_skill(ch, gsn_third_attack) / 4;

    if (IS_AFFECTED(ch, AFF_SLOW)) 
        chance = 0;

    if (number_percent() < chance) {
        one_hit(ch, victim, dt, false);
        check_improve(ch, gsn_third_attack, true, 6);
        if (ch->fighting != victim)
            return;
    }
    else
        check_improve(ch, gsn_third_attack, false, 5);

    return;
}

/* procedure for all mobile attacks */
void mob_hit(Mobile* ch, Mobile* victim, int16_t dt)
{
    int chance, number;
    Mobile* vch;

    one_hit(ch, victim, dt, false);

    if (ch->fighting != victim) return;

    /* Area attack -- BALLS nasty! */

    if (IS_SET(ch->atk_flags, ATK_AREA_ATTACK)) {
        FOR_EACH_ROOM_MOB(vch, ch->in_room) {
            if ((vch != victim && vch->fighting == ch))
                one_hit(ch, vch, dt, false);
        }
    }

    if (IS_AFFECTED(ch, AFF_HASTE)
        || (IS_SET(ch->atk_flags, ATK_FAST) && !IS_AFFECTED(ch, AFF_SLOW)))
        one_hit(ch, victim, dt, false);

    if (ch->fighting != victim || dt == gsn_backstab) 
        return;

    chance = get_skill(ch, gsn_second_attack) / 2;

    if (IS_AFFECTED(ch, AFF_SLOW) && !IS_SET(ch->atk_flags, ATK_FAST))
        chance /= 2;

    if (number_percent() < chance) {
        one_hit(ch, victim, dt, false);
        if (ch->fighting != victim) 
            return;
    }

    chance = get_skill(ch, gsn_third_attack) / 4;

    if (IS_AFFECTED(ch, AFF_SLOW) && !IS_SET(ch->atk_flags, ATK_FAST))
        chance = 0;

    if (number_percent() < chance) {
        one_hit(ch, victim, dt, false);
        if (ch->fighting != victim) 
            return;
    }

    /* oh boy!  Fun stuff! */

    if (ch->wait > 0)
        return;

    number = number_range(0, 2);

    if (number == 1 && IS_SET(ch->act_flags, ACT_MAGE)) {
        /*  { mob_cast_mage(ch,victim); return; } */;
    }

    if (number == 2 && IS_SET(ch->act_flags, ACT_CLERIC)) {
        /* { mob_cast_cleric(ch,victim); return; } */;
    }

    /* now for the skills */

    number = number_range(0, 8);

    switch (number) {
    case (0):
        if (IS_SET(ch->atk_flags, ATK_BASH)) do_function(ch, &do_bash, "");
        break;

    case (1):
        if (IS_SET(ch->atk_flags, ATK_BERSERK) && !IS_AFFECTED(ch, AFF_BERSERK))
            do_function(ch, &do_berserk, "");
        break;

    case (2):
        if (IS_SET(ch->atk_flags, ATK_DISARM)
            || (get_weapon_sn(ch) != gsn_hand_to_hand
                && (IS_SET(ch->act_flags, ACT_WARRIOR)
                    || IS_SET(ch->act_flags, ACT_THIEF))))
            do_function(ch, &do_disarm, "");
        break;

    case (3):
        if (IS_SET(ch->atk_flags, ATK_KICK)) 
            do_function(ch, &do_kick, "");
        break;

    case (4):
        if (IS_SET(ch->atk_flags, ATK_KICK_DIRT)) 
            do_function(ch, &do_dirt, "");
        break;

    case (5):
        if (IS_SET(ch->atk_flags, ATK_TAIL)) {
            /* do_function(ch, &do_tail, "") */;
        }
        break;

    case (6):
        if (IS_SET(ch->atk_flags, ATK_TRIP)) 
            do_function(ch, &do_trip, "");
        break;

    case (7):
        if (IS_SET(ch->atk_flags, ATK_CRUSH)) {
            /* do_function(ch, &do_crush, "") */;
        }
        break;
    case (8):
        if (IS_SET(ch->atk_flags, ATK_BACKSTAB)) {
            do_function(ch, &do_backstab, "");
        }
    }
}

// Hit one guy once.
void one_hit(Mobile* ch, Mobile* victim, int16_t dt, bool secondary)
{
    Object* wield;
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int dam;
    int diceroll;
    SKNUM sn;
    int skill;
    DamageType dam_type;
    bool result;

    sn = -1;

    /* just in case */
    if (victim == ch || ch == NULL || victim == NULL) 
        return;

    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if (victim->position == POS_DEAD || ch->in_room != victim->in_room) 
        return;

    // Figure out the type of damage message.
    // If secondary == true, use the second weapon.
    if (!secondary)
        wield = get_eq_char(ch, WEAR_WIELD);
    else
        wield = get_eq_char(ch, WEAR_WIELD_OH);

    if (dt == TYPE_UNDEFINED) {
        dt = TYPE_HIT;
        if (wield != NULL && wield->item_type == ITEM_WEAPON)
            dt += (int16_t)wield->value[3];
        else
            dt += (int16_t)ch->dam_type;
    }

    if (dt < TYPE_HIT)
        if (wield != NULL)
            dam_type = attack_table[wield->value[3]].damage;
        else
            dam_type = attack_table[ch->dam_type].damage;
    else
        dam_type = attack_table[dt - TYPE_HIT].damage;

    if (dam_type == DAM_NONE) 
        dam_type = DAM_BASH;

    /* get the weapon skill */
    sn = get_weapon_sn(ch);
    skill = 20 + get_weapon_skill(ch, sn);

    // Calculate to-hit-armor-class-0 versus armor.
    if (IS_NPC(ch)) {
        thac0_00 = 20;
        thac0_32 = -4; /* as good as a thief */
        if (IS_SET(ch->act_flags, ACT_WARRIOR))
            thac0_32 = -10;
        else if (IS_SET(ch->act_flags, ACT_THIEF))
            thac0_32 = -4;
        else if (IS_SET(ch->act_flags, ACT_CLERIC))
            thac0_32 = 2;
        else if (IS_SET(ch->act_flags, ACT_MAGE))
            thac0_32 = 6;
    }
    else {
        thac0_00 = class_table[ch->ch_class].thac0_00;
        thac0_32 = class_table[ch->ch_class].thac0_32;
    }
    thac0 = interpolate(ch->level, thac0_00, thac0_32);

    if (thac0 < 0) 
        thac0 = thac0 / 2;

    if (thac0 < -5) 
        thac0 = -5 + (thac0 + 5) / 2;

    thac0 -= GET_HITROLL(ch) * skill / 100;
    thac0 += 5 * (100 - skill) / 100;

    if (dt == gsn_backstab) 
        thac0 -= 10 * (100 - get_skill(ch, gsn_backstab));

    switch (dam_type) {
    case DAM_PIERCE:
        victim_ac = GET_AC(victim, AC_PIERCE) / 10;
        break;
    case DAM_BASH:
        victim_ac = GET_AC(victim, AC_BASH) / 10;
        break;
    case DAM_SLASH:
        victim_ac = GET_AC(victim, AC_SLASH) / 10;
        break;
    default:
        victim_ac = GET_AC(victim, AC_EXOTIC) / 10;
        break;
    };

    if (victim_ac < -15) 
        victim_ac = (victim_ac + 15) / 5 - 15;

    if (!can_see(ch, victim)) 
        victim_ac -= 4;

    if (victim->position < POS_FIGHTING)
        victim_ac += 4;

    if (victim->position < POS_RESTING)
        victim_ac += 6;

    // The moment of excitement!
    while ((diceroll = number_bits(5)) >= 20)
        ;

    if (diceroll == 0 || (diceroll != 19 && diceroll < thac0 - victim_ac)) {
        /* Miss. */
        damage(ch, victim, 0, dt, dam_type, true);
        return;
    }

    /*
     * Hit.
     * Calc damage.
     */
    if (IS_NPC(ch) && wield == NULL) {
        dam = dice(ch->damage[DICE_NUMBER], ch->damage[DICE_TYPE]);
    }
    else {
        if (sn != -1)
            check_improve(ch, sn, true, 5);
        if (wield != NULL) {
            dam = dice(wield->value[1], wield->value[2]) * skill / 100;

            if (get_eq_char(ch, WEAR_SHIELD) == NULL) /* no shield = more */
                dam = dam * 11 / 10;

            /* sharpness! */
            if (IS_WEAPON_STAT(wield, WEAPON_SHARP)) {
                int percent;

                if ((percent = number_percent()) <= (skill / 8))
                    dam = 2 * dam + (dam * 2 * percent / 100);
            }
        }
        else {
            dam = number_range(1 + 4 * skill / 100,
                               2 * ch->level / 3 * skill / 100);
        }
    }

    // Bonuses.
    if (get_skill(ch, gsn_enhanced_damage) > 0) {
        diceroll = number_percent();
        if (diceroll <= get_skill(ch, gsn_enhanced_damage)) {
            check_improve(ch, gsn_enhanced_damage, true, 6);
            dam += 2 * (dam * diceroll / 300);
        }
    }

    if (!IS_AWAKE(victim))
        dam *= 2;
    else if (victim->position < POS_FIGHTING)
        dam = dam * 3 / 2;

    if (dt == gsn_backstab && wield != NULL) {
        if (wield->value[0] != 2)
            dam *= 2 + (ch->level / 10);
        else
            dam *= 2 + (ch->level / 8);
    }

    dam += GET_DAMROLL(ch) * UMIN(100, skill) / 100;

    if (dam <= 0) 
        dam = 1;

    result = damage(ch, victim, dam, dt, dam_type, true);

    /* but do we have a funky weapon? */
    if (result && wield != NULL) {

        if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_POISON)) {
            LEVEL level;
            Affect* poison;
            Affect af = { 0 };

            if ((poison = affect_find(wield->affected, gsn_poison)) == NULL)
                level = wield->level;
            else
                level = poison->level;

            if (!saves_spell(level / 2, victim, DAM_POISON)) {
                send_to_char("You feel poison coursing through your veins.",
                             victim);
                act("$n is poisoned by the venom on $p.", victim, wield, NULL,
                    TO_ROOM);

                af.where = TO_AFFECTS;
                af.type = gsn_poison;
                af.level = (int16_t)level * 3 / 4;
                af.duration = (int16_t)level / 2;
                af.location = APPLY_STR;
                af.modifier = -1;
                af.bitvector = AFF_POISON;
                affect_join(victim, &af);
            }

            /* weaken the poison if it's temporary */
            if (poison != NULL) {
                poison->level = UMAX(0, poison->level - 2);
                poison->duration = UMAX(0, poison->duration - 1);

                if (poison->level == 0 || poison->duration == 0)
                    act("The poison on $p has worn off.", ch, wield, NULL,
                        TO_CHAR);
            }
        }

        if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_VAMPIRIC)) {
            dam = number_range(1, wield->level / 5 + 1);
            act("$p draws life from $n.", victim, wield, NULL, TO_ROOM);
            act("You feel $p drawing your life away.", victim, wield, NULL,
                TO_CHAR);
            damage(ch, victim, dam, 0, DAM_NEGATIVE, false);
            ch->alignment = UMAX(-1000, ch->alignment - 1);
            ch->hit += (int16_t)dam / 2;
        }

        if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_FLAMING)) {
            dam = number_range(1, wield->level / 4 + 1);
            act("$n is burned by $p.", victim, wield, NULL, TO_ROOM);
            act("$p sears your flesh.", victim, wield, NULL, TO_CHAR);
            fire_effect((void*)victim, wield->level / 2, dam, SPELL_TARGET_CHAR);
            damage(ch, victim, dam, 0, DAM_FIRE, false);
        }

        if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_FROST)) {
            dam = number_range(1, wield->level / 6 + 2);
            act("$p freezes $n.", victim, wield, NULL, TO_ROOM);
            act("The cold touch of $p surrounds you with ice.", victim, wield,
                NULL, TO_CHAR);
            cold_effect(victim, wield->level / 2, dam, SPELL_TARGET_CHAR);
            damage(ch, victim, dam, 0, DAM_COLD, false);
        }

        if (ch->fighting == victim && IS_WEAPON_STAT(wield, WEAPON_SHOCKING)) {
            dam = number_range(1, wield->level / 5 + 2);
            act("$n is struck by lightning from $p.", victim, wield, NULL,
                TO_ROOM);
            act("You are shocked by $p.", victim, wield, NULL, TO_CHAR);
            shock_effect(victim, wield->level / 2, dam, SPELL_TARGET_CHAR);
            damage(ch, victim, dam, 0, DAM_LIGHTNING, false);
        }
    }

    return;
}

// Inflict damage from a hit.
bool damage(Mobile* ch, Mobile* victim, int dam, int16_t dt, DamageType dam_type, bool show)
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
    if (!IS_AWAKE(victim)) stop_fighting(victim, false);

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

        raw_kill(victim);
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

bool is_safe(Mobile* ch, Mobile* victim)
{
    if (victim->in_room == NULL || ch->in_room == NULL) return true;

    if (victim->fighting == ch || victim == ch) return false;

    if (IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) return false;

    if (faction_block_player_attack(ch, victim))
        return true;

    /* killing mobiles */
    if (IS_NPC(victim)) {
        /* safe room? */
        if (IS_SET(victim->in_room->data->room_flags, ROOM_SAFE)) {
            send_to_char("Not in this room.\n\r", ch);
            return true;
        }

        if (victim->prototype->pShop != NULL) {
            send_to_char("The shopkeeper wouldn't like that.\n\r", ch);
            return true;
        }

        /* no killing healers, trainers, etc */
        if (IS_SET(victim->act_flags, ACT_TRAIN) || IS_SET(victim->act_flags, ACT_PRACTICE)
            || IS_SET(victim->act_flags, ACT_IS_HEALER)
            || IS_SET(victim->act_flags, ACT_IS_CHANGER)) {
            send_to_char("I don't think Mota would approve.\n\r", ch);
            return true;
        }

        if (!IS_NPC(ch)) {
            /* no pets */
            if (IS_SET(victim->act_flags, ACT_PET)) {
                act("But $N looks so cute and cuddly...", ch, NULL, victim,
                    TO_CHAR);
                return true;
            }

            /* no charmed creatures unless owner */
            if (IS_AFFECTED(victim, AFF_CHARM) && ch != victim->master) {
                send_to_char("You don't own that monster.\n\r", ch);
                return true;
            }
        }
    }
    /* killing players */
    else {
        /* NPC doing the killing */
        if (IS_NPC(ch)) {
            /* safe room check */
            if (IS_SET(victim->in_room->data->room_flags, ROOM_SAFE)) {
                send_to_char("Not in this room.\n\r", ch);
                return true;
            }

            /* charmed mobs and pets cannot attack players while owned */
            if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL
                && ch->master->fighting != victim) {
                send_to_char("Players are your friends!\n\r", ch);
                return true;
            }
        }
        /* player doing the killing */
        else {
            if (!is_clan(ch)) {
                send_to_char("Join a clan if you want to kill players.\n\r",
                             ch);
                return true;
            }

            if (IS_SET(victim->act_flags, PLR_KILLER)
                || IS_SET(victim->act_flags, PLR_THIEF))
                return false;

            if (!is_clan(victim)) {
                send_to_char("They aren't in a clan, leave them alone.\n\r",
                             ch);
                return true;
            }

            if (ch->level > victim->level + 8) {
                send_to_char("Pick on someone your own size.\n\r", ch);
                return true;
            }
        }
    }
    return false;
}

bool is_safe_spell(Mobile* ch, Mobile* victim, bool area)
{
    if (victim->in_room == NULL || ch->in_room == NULL) return true;

    if (victim == ch && area) return true;

    if (victim->fighting == ch || victim == ch) return false;

    if (IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL && !area) return false;

    if (!IS_NPC(ch) && IS_NPC(victim)) {
        if (faction_block_player_attack(ch, victim))
            return true;
    }

    /* killing mobiles */
    if (IS_NPC(victim)) {
        /* safe room? */
        if (IS_SET(victim->in_room->data->room_flags, ROOM_SAFE)) return true;

        if (victim->prototype->pShop != NULL) return true;

        /* no killing healers, trainers, etc */
        if (IS_SET(victim->act_flags, ACT_TRAIN) || IS_SET(victim->act_flags, ACT_PRACTICE)
            || IS_SET(victim->act_flags, ACT_IS_HEALER)
            || IS_SET(victim->act_flags, ACT_IS_CHANGER))
            return true;

        if (!IS_NPC(ch)) {
            /* no pets */
            if (IS_SET(victim->act_flags, ACT_PET)) return true;

            /* no charmed creatures unless owner */
            if (IS_AFFECTED(victim, AFF_CHARM)
                && (area || ch != victim->master))
                return true;

            /* legal kill? -- cannot hit mob fighting non-group member */
            if (victim->fighting != NULL
                && !is_same_group(ch, victim->fighting))
                return true;
        }
        else {
            /* area effect spells do not hit other mobs */
            if (area && !is_same_group(victim, ch->fighting)) return true;
        }
    }
    /* killing players */
    else {
        if (area && IS_IMMORTAL(victim) && victim->level > LEVEL_IMMORTAL)
            return true;

        /* NPC doing the killing */
        if (IS_NPC(ch)) {
            /* charmed mobs and pets cannot attack players while owned */
            if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL
                && ch->master->fighting != victim)
                return true;

            /* safe room? */
            if (IS_SET(victim->in_room->data->room_flags, ROOM_SAFE)) return true;

            /* legal kill? -- mobs only hit players grouped with opponent*/
            if (ch->fighting != NULL && !is_same_group(ch->fighting, victim))
                return true;
        }

        /* player doing the killing */
        else {
            if (!is_clan(ch)) return true;

            if (IS_SET(victim->act_flags, PLR_KILLER)
                || IS_SET(victim->act_flags, PLR_THIEF))
                return false;

            if (!is_clan(victim)) return true;

            if (ch->level > victim->level + 8) return true;
        }
    }
    return false;
}
// See if an attack justifies a KILLER flag.
void check_killer(Mobile* ch, Mobile* victim)
{
    char buf[MAX_STRING_LENGTH];
    /*
     * Follow charm thread to responsible character.
     * Attacking someone's charmed char is hostile!
     */
    while (IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL)
        victim = victim->master;

    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if (IS_NPC(victim) || IS_SET(victim->act_flags, PLR_KILLER)
        || IS_SET(victim->act_flags, PLR_THIEF))
        return;

    // Charm-o-rama.
    if (IS_SET(ch->affect_flags, AFF_CHARM)) {
        if (ch->master == NULL) {
            sprintf(buf, "Check_killer: %s bad AFF_CHARM",
                    IS_NPC(ch) ? ch->short_descr : NAME_STR(ch));
            bug(buf, 0);
            affect_strip(ch, gsn_charm_person);
            REMOVE_BIT(ch->affect_flags, AFF_CHARM);
            return;
        }
        /*
                send_to_char( "*** You are now a KILLER!! ***\n\r", ch->master
           ); SET_BIT(ch->master->act_flags, PLR_KILLER);
        */

        stop_follower(ch);
        return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if (IS_NPC(ch) || ch == victim || ch->level >= LEVEL_IMMORTAL
        || !is_clan(ch) || IS_SET(ch->act_flags, PLR_KILLER)
        || ch->fighting == victim)
        return;

    send_to_char("*** You are now a KILLER!! ***\n\r", ch);
    SET_BIT(ch->act_flags, PLR_KILLER);
    sprintf(buf, "$N is attempting to murder %s", NAME_STR(victim));
    wiznet(buf, ch, NULL, WIZ_FLAGS, 0, 0);
    save_char_obj(ch);
    return;
}

// Check for parry.
bool check_parry(Mobile* ch, Mobile* victim)
{
    int chance;

    if (!IS_AWAKE(victim)) return false;

    chance = get_skill(victim, gsn_parry) / 2;

    if (get_eq_char(victim, WEAR_WIELD) == NULL) {
        if (IS_NPC(victim))
            chance /= 2;
        else
            return false;
    }

    if (!can_see(ch, victim)) chance /= 2;

    if (number_percent() >= chance + victim->level - ch->level) return false;

    act("You parry $n's attack.", ch, NULL, victim, TO_VICT);
    act("$N parries your attack.", ch, NULL, victim, TO_CHAR);
    check_improve(victim, gsn_parry, true, 6);
    return true;
}

// Check for shield block.
bool check_shield_block(Mobile* ch, Mobile* victim)
{
    int chance;

    if (!IS_AWAKE(victim)) return false;

    chance = get_skill(victim, gsn_shield_block) / 5 + 3;

    if (get_eq_char(victim, WEAR_SHIELD) == NULL) return false;

    if (number_percent() >= chance + victim->level - ch->level) return false;

    act("You block $n's attack with your shield.", ch, NULL, victim, TO_VICT);
    act("$N blocks your attack with a shield.", ch, NULL, victim, TO_CHAR);
    check_improve(victim, gsn_shield_block, true, 6);
    return true;
}

// Check for dodge.
bool check_dodge(Mobile* ch, Mobile* victim)
{
    int chance;

    if (!IS_AWAKE(victim)) return false;

    chance = get_skill(victim, gsn_dodge) / 2;

    if (!can_see(victim, ch)) chance /= 2;

    if (number_percent() >= chance + victim->level - ch->level) return false;

    act("You dodge $n's attack.", ch, NULL, victim, TO_VICT);
    act("$N dodges your attack.", ch, NULL, victim, TO_CHAR);
    check_improve(victim, gsn_dodge, true, 6);
    return true;
}

// Set position of a victim.
void update_pos(Mobile* victim)
{
    if (victim->hit > 0) {
        if (victim->position <= POS_STUNNED) victim->position = POS_STANDING;
        return;
    }

    if (IS_NPC(victim) && victim->hit < 1) {
        victim->position = POS_DEAD;
        return;
    }

    if (victim->hit <= -11) {
        victim->position = POS_DEAD;
        return;
    }

    if (victim->hit <= -6)
        victim->position = POS_MORTAL;
    else if (victim->hit <= -3)
        victim->position = POS_INCAP;
    else
        victim->position = POS_STUNNED;

    return;
}

// Start fights.
void set_fighting(Mobile* ch, Mobile* victim)
{
    if (ch->fighting != NULL) {
        bug("Set_fighting: already fighting", 0);
        return;
    }

    if (IS_AFFECTED(ch, AFF_SLEEP)) affect_strip(ch, gsn_sleep);

    ch->fighting = victim;
    ch->position = POS_FIGHTING;

    return;
}

// Stop fights.
void stop_fighting(Mobile* ch, bool fBoth)
{
    Mobile* fch;

    FOR_EACH_GLOBAL_MOB(fch) {
        if (fch == ch || (fBoth && fch->fighting == ch)) {
            fch->fighting = NULL;
            fch->position = IS_NPC(fch) ? fch->default_pos : POS_STANDING;
            update_pos(fch);
        }
    }

    return;
}

// Make a corpse out of a character.
void make_corpse(Mobile* ch)
{
    char buf[MAX_STRING_LENGTH];
    Object* corpse = NULL;
    Object* obj = NULL;
    String* name;

    if (IS_NPC(ch)) {
        name = lox_string(ch->short_descr);
        corpse = create_object(get_object_prototype(OBJ_VNUM_CORPSE_NPC), 0);
        SET_NAME(corpse, name);
        corpse->timer = (int16_t)number_range(3, 6);
        long total_copper = mobile_total_copper(ch);
        if (total_copper > 0) {
            int16_t gold = 0, silver = 0, copper = 0;
            convert_copper_to_money(total_copper, &gold, &silver, &copper);
            obj_to_obj(create_money(gold, silver, copper), corpse);
            mobile_set_money_from_copper(ch, 0);
        }
        corpse->cost = 0;
    }
    else {
        name = NAME_FIELD(ch);
        corpse = create_object(get_object_prototype(OBJ_VNUM_CORPSE_PC), 0);
        SET_NAME(corpse, name);
        corpse->timer = (int16_t)number_range(25, 40);
        REMOVE_BIT(ch->act_flags, PLR_CANLOOT);
        if (!is_clan(ch))
            corpse->owner = NAME_FIELD(ch);
        else {
            corpse->owner = NULL;
            long total_copper = mobile_total_copper(ch);
            if (total_copper > 0) {
                long drop = total_copper / 2;
                if (drop > 0) {
                    int16_t gold = 0, silver = 0, copper = 0;
                    convert_copper_to_money(drop, &gold, &silver, &copper);
                    obj_to_obj(create_money(gold, silver, copper), corpse);
                    mobile_set_money_from_copper(ch, total_copper - drop);
                }
            }
        }

        corpse->cost = 0;
    }

    corpse->level = ch->level;

    SPRINTF_CORPSE_SDESC(buf, DESC_CORPSE, C_STR(name));
    free_string(corpse->short_descr);
    corpse->short_descr = str_dup(buf);

    SPRINTF_CORPSE_DESC(buf, DESC_CORPSE, C_STR(name));
    free_string(corpse->description);
    corpse->description = str_dup(buf);

    FOR_EACH_MOB_OBJ(obj, ch) {
        bool floating = false;
        if (obj->wear_loc == WEAR_FLOAT)
            floating = true;
        obj_from_char(obj);
        if (obj->item_type == ITEM_POTION) 
            obj->timer = (int16_t)number_range(500, 1000);
        if (obj->item_type == ITEM_SCROLL)
            obj->timer = (int16_t)number_range(1000, 2500);
        if (IS_SET(obj->extra_flags, ITEM_ROT_DEATH) && !floating) {
            obj->timer = (int16_t)number_range(5, 10);
            REMOVE_BIT(obj->extra_flags, ITEM_ROT_DEATH);
        }
        REMOVE_BIT(obj->extra_flags, ITEM_VIS_DEATH);

        if (IS_SET(obj->extra_flags, ITEM_INVENTORY))
            extract_obj(obj);
        else if (floating) {
            if (IS_OBJ_STAT(obj, ITEM_ROT_DEATH)) {
                // get rid of it!
                if (OBJ_HAS_OBJS(obj)) {
                    Object* in = NULL;

                    act("$p evaporates,scattering its contents.", ch, obj, NULL,
                        TO_ROOM);
                    FOR_EACH_OBJ_CONTENT(in, obj) {
                        obj_from_obj(in);
                        obj_to_room(in, ch->in_room);
                    }
                }
                else
                    act("$p evaporates.", ch, obj, NULL, TO_ROOM);
                extract_obj(obj);
            }
            else {
                act("$p falls to the floor.", ch, obj, NULL, TO_ROOM);
                obj_to_room(obj, ch->in_room);
            }
        }
        else
            obj_to_obj(obj, corpse);
    }

    obj_to_room(corpse, ch->in_room);
    return;
}

// Improved Death_cry contributed by Diavolo.
void death_cry(Mobile* ch)
{
    Room* was_in_room;
    char* msg;
    int door;
    VNUM vnum;

    vnum = 0;
    msg = "You hear $n's death cry.";

    switch (number_bits(4)) {
    case 0:
        msg = "$n hits the ground ... DEAD.";
        break;
    case 1:
        if (ch->material == 0) {
            msg = "$n splatters blood on your armor.";
        }
        break;
    case 2:
        if (IS_SET(ch->parts, PART_GUTS)) {
            msg = "$n spills $s guts all over the floor.";
            vnum = OBJ_VNUM_GUTS;
        }
        break;
    case 3:
        if (IS_SET(ch->parts, PART_HEAD)) {
            msg = "$n's severed head plops on the ground.";
            vnum = OBJ_VNUM_SEVERED_HEAD;
        }
        break;
    case 4:
        if (IS_SET(ch->parts, PART_HEART)) {
            msg = "$n's heart is torn from $s chest.";
            vnum = OBJ_VNUM_TORN_HEART;
        }
        break;
    case 5:
        if (IS_SET(ch->parts, PART_ARMS)) {
            msg = "$n's arm is sliced from $s dead body.";
            vnum = OBJ_VNUM_SLICED_ARM;
        }
        break;
    case 6:
        if (IS_SET(ch->parts, PART_LEGS)) {
            msg = "$n's leg is sliced from $s dead body.";
            vnum = OBJ_VNUM_SLICED_LEG;
        }
        break;
    case 7:
        if (IS_SET(ch->parts, PART_BRAINS)) {
            msg = "$n's head is shattered, and $s brains splash all over you.";
            vnum = OBJ_VNUM_BRAINS;
        }
    }

    act(msg, ch, NULL, NULL, TO_ROOM);

    if (vnum != 0) {
        char buf[MAX_STRING_LENGTH];
        Object* obj;
        char* name;

        name = IS_NPC(ch) ? ch->short_descr : NAME_STR(ch);
        obj = create_object(get_object_prototype(vnum), 0);
        obj->timer = (int16_t)number_range(4, 7);

        SPRINTF_CORPSE_SDESC(buf, vnum - OBJ_VNUM_CORPSE_NPC, name);
        free_string(obj->short_descr);
        obj->short_descr = str_dup(buf);

        SPRINTF_CORPSE_DESC(buf, vnum - OBJ_VNUM_CORPSE_NPC, name);
        free_string(obj->description);
        obj->description = str_dup(buf);

        if (obj->item_type == ITEM_FOOD) {
            if (IS_SET(ch->form, FORM_POISON))
                obj->value[3] = 1;
            else if (!IS_SET(ch->form, FORM_EDIBLE))
                obj->item_type = ITEM_TRASH;
        }

        obj_to_room(obj, ch->in_room);
    }

    if (IS_NPC(ch))
        msg = "You hear something's death cry.";
    else
        msg = "You hear someone's death cry.";

    was_in_room = ch->in_room;
    for (door = 0; door <= 5; door++) {
        RoomExit* room_exit;

        if ((room_exit = was_in_room->exit[door]) != NULL
            && room_exit->to_room != NULL && room_exit->to_room != was_in_room) {
            ch->in_room = room_exit->to_room;
            act(msg, ch, NULL, NULL, TO_ROOM);
        }
    }
    ch->in_room = was_in_room;

    return;
}

void raw_kill(Mobile* victim)
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
    while (victim->affected) affect_remove(victim, victim->affected);
    victim->affect_flags = race_table[victim->race].aff;
    for (i = 0; i < 4; i++) victim->armor[i] = 100;
    victim->position = POS_RESTING;
    victim->hit = UMAX(1, victim->hit);
    victim->mana = UMAX(1, victim->mana);
    victim->move = UMAX(1, victim->move);
    /*  save_char_obj( victim ); we're stable enough to not need this :) */
    return;
}

void group_gain(Mobile* ch, Mobile* victim)
{
    char buf[MAX_STRING_LENGTH];
    Mobile* gch = NULL;
    int xp;
    int members;
    int group_levels;

    /*
     * Monsters don't get kill xp's or alignment changes.
     * P-killing doesn't help either.
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if (victim == ch || ch == NULL || ch->in_room == NULL)
        return;

    members = 0;
    group_levels = 0;
    FOR_EACH_ROOM_MOB(gch, ch->in_room) {
        if (is_same_group(gch, ch)) {
            members++;
            group_levels += IS_NPC(gch) ? gch->level / 2 : gch->level;
        }
    }

    if (members == 0) {
        bug("Group_gain: members.", members);
        members = 1;
        group_levels = ch->level;
    }

    FOR_EACH_ROOM_MOB(gch, ch->in_room) {
        Object* obj = NULL;

        if (!is_same_group(gch, ch) || IS_NPC(gch))
            continue;

        faction_handle_kill(gch, victim);

        /*	Taken out, add it back if you want it
                if ( gch->level - lch->level >= 5 )
                {
                    send_to_char( "You are too high for this group.\n\r", gch );
                    continue;
                }

                if ( gch->level - lch->level <= -5 )
                {
                    send_to_char( "You are too low for this group.\n\r", gch );
                    continue;
                }
        */

        xp = xp_compute(gch, victim, group_levels);
        sprintf(buf, "You receive %d experience points.\n\r", xp);
        send_to_char(buf, gch);
        gain_exp(gch, xp);

        FOR_EACH_MOB_OBJ(obj, ch) {
            if (obj->wear_loc == WEAR_UNHELD) continue;

            if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
                || (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
                || (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) {
                act("You are zapped by $p.", ch, obj, NULL, TO_CHAR);
                act("$n is zapped by $p.", ch, obj, NULL, TO_ROOM);
                obj_from_char(obj);
                obj_to_room(obj, ch->in_room);
            }
        }
    }

    return;
}

/*
 * Compute xp for a kill.
 * Also adjust alignment of killer.
 * Edit this function to change xp computations.
 */
int xp_compute(Mobile* gch, Mobile* victim, int total_levels)
{
    int xp, base_exp;
    int align, level_range;
    int change;
    int time_per_level;

    level_range = victim->level - gch->level;

    /* compute the base exp */
    switch (level_range) {
    default:
        base_exp = 0;
        break;
    case -9:
        base_exp = 1;
        break;
    case -8:
        base_exp = 2;
        break;
    case -7:
        base_exp = 5;
        break;
    case -6:
        base_exp = 9;
        break;
    case -5:
        base_exp = 11;
        break;
    case -4:
        base_exp = 22;
        break;
    case -3:
        base_exp = 33;
        break;
    case -2:
        base_exp = 50;
        break;
    case -1:
        base_exp = 66;
        break;
    case 0:
        base_exp = 83;
        break;
    case 1:
        base_exp = 99;
        break;
    case 2:
        base_exp = 121;
        break;
    case 3:
        base_exp = 143;
        break;
    case 4:
        base_exp = 165;
        break;
    }

    if (level_range > 4) base_exp = 160 + 20 * (level_range - 4);

    /* do alignment computations */

    align = victim->alignment - gch->alignment;

    if (IS_SET(victim->act_flags, ACT_NOALIGN)) { /* no change */
    }

    else if (align > 500) /* monster is more good than slayer */
    {
        change = (align - 500) * base_exp / 500 * gch->level / total_levels;
        change = UMAX(1, change);
        gch->alignment = UMAX(-1000, gch->alignment - (int16_t)change);
    }

    else if (align < -500) /* monster is more evil than slayer */
    {
        change
            = (-1 * align - 500) * base_exp / 500 * gch->level / total_levels;
        change = UMAX(1, change);
        gch->alignment = UMIN(1000, gch->alignment + (int16_t)change);
    }

    else /* improve this someday */
    {
        change = gch->alignment * base_exp / 500 * gch->level / total_levels;
        gch->alignment -= (int16_t)change;
    }

    /* calculate exp multiplier */
    if (IS_SET(victim->act_flags, ACT_NOALIGN))
        xp = base_exp;

    else if (gch->alignment > 500) /* for goodie two shoes */
    {
        if (victim->alignment < -750)
            xp = (base_exp * 4) / 3;

        else if (victim->alignment < -500)
            xp = (base_exp * 5) / 4;

        else if (victim->alignment > 750)
            xp = base_exp / 4;

        else if (victim->alignment > 500)
            xp = base_exp / 2;

        else if (victim->alignment > 250)
            xp = (base_exp * 3) / 4;

        else
            xp = base_exp;
    }

    else if (gch->alignment < -500) /* for baddies */
    {
        if (victim->alignment > 750)
            xp = (base_exp * 5) / 4;

        else if (victim->alignment > 500)
            xp = (base_exp * 11) / 10;

        else if (victim->alignment < -750)
            xp = base_exp / 2;

        else if (victim->alignment < -500)
            xp = (base_exp * 3) / 4;

        else if (victim->alignment < -250)
            xp = (base_exp * 9) / 10;

        else
            xp = base_exp;
    }

    else if (gch->alignment > 200) /* a little good */
    {
        if (victim->alignment < -500)
            xp = (base_exp * 6) / 5;

        else if (victim->alignment > 750)
            xp = base_exp / 2;

        else if (victim->alignment > 0)
            xp = (base_exp * 3) / 4;

        else
            xp = base_exp;
    }

    else if (gch->alignment < -200) /* a little bad */
    {
        if (victim->alignment > 500)
            xp = (base_exp * 6) / 5;

        else if (victim->alignment < -750)
            xp = base_exp / 2;

        else if (victim->alignment < 0)
            xp = (base_exp * 3) / 4;

        else
            xp = base_exp;
    }

    else /* neutral */
    {
        if (victim->alignment > 500 || victim->alignment < -500)
            xp = (base_exp * 4) / 3;

        else if (victim->alignment < 200 && victim->alignment > -200)
            xp = base_exp / 2;

        else
            xp = base_exp;
    }

    /* more exp at the low levels */
    if (gch->level < 6) xp = 10 * xp / (gch->level + 4);

    /* less at high */
    if (gch->level > 35) xp = 15 * xp / (gch->level - 25);

    /* reduce for playing time */

    {
        /* compute quarter-hours per level */
        time_per_level = 4 * (int)((gch->played + (current_time - gch->logon))
                         / 3600 / gch->level);

        time_per_level = URANGE(2, time_per_level, 12);
        if (gch->level < 15) /* make it a curve */
            time_per_level = UMAX(time_per_level, (15 - gch->level));
        xp = xp * time_per_level / 12;
    }

    /* randomize the rewards */
    xp = number_range(xp * 3 / 4, xp * 5 / 4);

    /* adjust for grouping */
    xp = xp * gch->level / (UMAX(1, total_levels - 1));

    return xp;
}

void dam_message(Mobile* ch, Mobile* victim, int dam, int dt, bool immune)
{
    char buf1[256], buf2[256], buf3[256];
    const char* vs;
    const char* vp;
    const char* attack;
    char punct;

    if (ch == NULL || victim == NULL) return;

    if (dam == 0) {
        vs = "miss";
        vp = "misses";
    }
    else if (dam <= 4) {
        vs = "scratch";
        vp = "scratches";
    }
    else if (dam <= 8) {
        vs = "graze";
        vp = "grazes";
    }
    else if (dam <= 12) {
        vs = "hit";
        vp = "hits";
    }
    else if (dam <= 16) {
        vs = "injure";
        vp = "injures";
    }
    else if (dam <= 20) {
        vs = "wound";
        vp = "wounds";
    }
    else if (dam <= 24) {
        vs = "maul";
        vp = "mauls";
    }
    else if (dam <= 28) {
        vs = "decimate";
        vp = "decimates";
    }
    else if (dam <= 32) {
        vs = "devastate";
        vp = "devastates";
    }
    else if (dam <= 36) {
        vs = "maim";
        vp = "maims";
    }
    else if (dam <= 40) {
        vs = "MUTILATE";
        vp = "MUTILATES";
    }
    else if (dam <= 44) {
        vs = "DISEMBOWEL";
        vp = "DISEMBOWELS";
    }
    else if (dam <= 48) {
        vs = "DISMEMBER";
        vp = "DISMEMBERS";
    }
    else if (dam <= 52) {
        vs = "MASSACRE";
        vp = "MASSACRES";
    }
    else if (dam <= 56) {
        vs = "MANGLE";
        vp = "MANGLES";
    }
    else if (dam <= 60) {
        vs = "*** DEMOLISH ***";
        vp = "*** DEMOLISHES ***";
    }
    else if (dam <= 75) {
        vs = "*** DEVASTATE ***";
        vp = "*** DEVASTATES ***";
    }
    else if (dam <= 100) {
        vs = "=== OBLITERATE ===";
        vp = "=== OBLITERATES ===";
    }
    else if (dam <= 125) {
        vs = ">>> ANNIHILATE <<<";
        vp = ">>> ANNIHILATES <<<";
    }
    else if (dam <= 150) {
        vs = "<<< ERADICATE >>>";
        vp = "<<< ERADICATES >>>";
    }
    else {
        vs = "do UNSPEAKABLE things to";
        vp = "does UNSPEAKABLE things to";
    }

    punct = (dam <= 24) ? '.' : '!';

    if (dt == TYPE_HIT) {
        if (ch == victim) {
            sprintf(buf1, COLOR_FIGHT_OHIT "$n %s $melf%c" COLOR_CLEAR , vp, punct);
            sprintf(buf2, COLOR_FIGHT_YHIT "You %s yourself%c" COLOR_CLEAR , vs, punct);
        }
        else {
            sprintf(buf1, COLOR_FIGHT_OHIT "$n %s $N%c" COLOR_CLEAR , vp, punct);
            sprintf(buf2, COLOR_FIGHT_YHIT "You %s $N%c" COLOR_CLEAR , vs, punct);
            sprintf(buf3, COLOR_FIGHT_THIT "$n %s you%c" COLOR_CLEAR , vp, punct);
        }
    }
    else {
        if (dt >= 0 && dt < skill_count)
            attack = skill_table[dt].noun_damage;
        else if (dt >= TYPE_HIT && dt < TYPE_HIT + ATTACK_COUNT)
            attack = attack_table[dt - TYPE_HIT].noun;
        else {
            bug("Dam_message: bad dt %d.", dt);
            dt = TYPE_HIT;
            attack = attack_table[0].name;
        }

        if (immune) {
            if (ch == victim) {
                sprintf(buf1, COLOR_FIGHT_OHIT "$n is unaffected by $s own %s." COLOR_CLEAR , attack);
                sprintf(buf2, COLOR_FIGHT_YHIT "Luckily, you are immune to that." COLOR_CLEAR );
            }
            else {
                sprintf(buf1, COLOR_FIGHT_OHIT "$N is unaffected by $n's %s!" COLOR_CLEAR , attack);
                sprintf(buf2, COLOR_FIGHT_YHIT "$N is unaffected by your %s!" COLOR_CLEAR , attack);
                sprintf(buf3, COLOR_FIGHT_THIT "$n's %s is powerless against you." COLOR_CLEAR , attack);
            }
        }
        else {
            if (ch == victim) {
                sprintf(buf1, COLOR_FIGHT_OHIT "$n's %s %s $m%c" COLOR_CLEAR , attack, vp, punct);
                sprintf(buf2, COLOR_FIGHT_YHIT "Your %s %s you%c" COLOR_CLEAR , attack, vp, punct);
            }
            else {
                sprintf(buf1, COLOR_FIGHT_OHIT "$n's %s %s $N%c" COLOR_CLEAR , attack, vp, punct);
                sprintf(buf2, COLOR_FIGHT_YHIT "Your %s %s $N%c" COLOR_CLEAR , attack, vp, punct);
                sprintf(buf3, COLOR_FIGHT_THIT "$n's %s %s you%c" COLOR_CLEAR , attack, vp, punct);
            }
        }
    }

    if (ch == victim) {
        act(buf1, ch, NULL, NULL, TO_ROOM);
        act(buf2, ch, NULL, NULL, TO_CHAR);
    }
    else {
        act(buf1, ch, NULL, victim, TO_NOTVICT);
        act(buf2, ch, NULL, victim, TO_CHAR);
        act(buf3, ch, NULL, victim, TO_VICT);
    }

    return;
}

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void disarm(Mobile* ch, Mobile* victim)
{
    Object* obj;

    if ((obj = get_eq_char(victim, WEAR_WIELD)) == NULL) return;

    if (IS_OBJ_STAT(obj, ITEM_NOREMOVE)) {
        act(COLOR_FIGHT_SKILL "$S weapon won't budge!" COLOR_CLEAR , ch, NULL, victim, TO_CHAR);
        act(COLOR_FIGHT_SKILL "$n tries to disarm you, but your weapon won't budge!" COLOR_CLEAR , ch,
            NULL, victim, TO_VICT);
        act(COLOR_FIGHT_SKILL "$n tries to disarm $N, but fails." COLOR_CLEAR , ch, NULL, victim,
            TO_NOTVICT);
        return;
    }

    act(COLOR_FIGHT_SKILL "$n DISARMS you and sends your weapon flying!" COLOR_CLEAR , ch, NULL, victim,
        TO_VICT);
    act(COLOR_FIGHT_SKILL "You disarm $N!" COLOR_CLEAR , ch, NULL, victim, TO_CHAR);
    act(COLOR_FIGHT_SKILL "$n disarms $N!" COLOR_CLEAR , ch, NULL, victim, TO_NOTVICT);

    obj_from_char(obj);
    if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_INVENTORY))
        obj_to_char(obj, victim);
    else {
        obj_to_room(obj, victim->in_room);
        if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim, obj))
            get_obj(victim, obj, NULL);
    }

    return;
}

void do_berserk(Mobile* ch, char* argument)
{
    int chance, hp_percent;
    

    if ((chance = get_skill(ch, gsn_berserk)) == 0
        || (IS_NPC(ch) && !IS_SET(ch->atk_flags, ATK_BERSERK))
        || (!IS_NPC(ch)
            && ch->level < SKILL_LEVEL(gsn_berserk, ch))) {
        send_to_char("You turn red in the face, but nothing happens.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_BERSERK) || is_affected(ch, gsn_berserk)
        || is_affected(ch, skill_lookup("frenzy"))) {
        send_to_char("You get a little madder.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CALM)) {
        send_to_char("You're feeling to mellow to berserk.\n\r", ch);
        return;
    }

    if (ch->mana < 50) {
        send_to_char("You can't get up enough energy.\n\r", ch);
        return;
    }

    /* modifiers */

    /* fighting */
    if (ch->position == POS_FIGHTING) chance += 10;

    /* damage -- below 50% of hp helps, above hurts */
    hp_percent = 100 * ch->hit / ch->max_hit;
    chance += 25 - hp_percent / 2;

    if (number_percent() < chance) {
        Affect af = { 0 };

        WAIT_STATE(ch, PULSE_VIOLENCE);
        ch->mana -= 50;
        ch->move /= 2;

        /* heal a little damage */
        ch->hit += ch->level * 2;
        ch->hit = UMIN(ch->hit, ch->max_hit);

        send_to_char("Your pulse races as you are consumed by rage!\n\r", ch);
        act("$n gets a wild look in $s eyes.", ch, NULL, NULL, TO_ROOM);
        check_improve(ch, gsn_berserk, true, 2);

        af.where = TO_AFFECTS;
        af.type = gsn_berserk;
        af.level = ch->level;
        af.duration = (int16_t)number_fuzzy(ch->level / 8);
        af.modifier = UMAX(1, ch->level / 5);
        af.bitvector = AFF_BERSERK;

        af.location = APPLY_HITROLL;
        affect_to_mob(ch, &af);

        af.location = APPLY_DAMROLL;
        affect_to_mob(ch, &af);

        af.modifier = UMAX(10, 10 * (ch->level / 5));
        af.location = APPLY_AC;
        affect_to_mob(ch, &af);
    }

    else {
        WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
        ch->mana -= 25;
        ch->move /= 2;

        send_to_char("Your pulse speeds up, but nothing happens.\n\r", ch);
        check_improve(ch, gsn_berserk, false, 2);
    }
}

void do_bash(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;
    int chance;

    one_argument(argument, arg);

    if ((chance = get_skill(ch, gsn_bash)) == 0
        || (IS_NPC(ch) && !IS_SET(ch->atk_flags, ATK_BASH))
        || (!IS_NPC(ch)
            && ch->level < SKILL_LEVEL(gsn_bash, ch))) {
        send_to_char("Bashing? What's that?\n\r", ch);
        return;
    }

    if (arg[0] == '\0') {
        victim = ch->fighting;
        if (victim == NULL) {
            send_to_char("But you aren't fighting anyone!\n\r", ch);
            return;
        }
    }

    else if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim->position < POS_FIGHTING) {
        act("You'll have to let $M get back up first.", ch, NULL, victim,
            TO_CHAR);
        return;
    }

    if (victim == ch) {
        send_to_char("You try to bash your brains out, but fail.\n\r", ch);
        return;
    }

    if (is_safe(ch, victim)) return;

    if (IS_NPC(victim) && victim->fighting != NULL
        && !is_same_group(ch, victim->fighting)) {
        send_to_char("Kill stealing is not permitted.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
        act("But $N is your friend!", ch, NULL, victim, TO_CHAR);
        return;
    }

    /* modifiers */

    /* size  and weight */
    chance += ch->carry_weight / 250;
    chance -= victim->carry_weight / 200;

    if (ch->size < victim->size)
        chance += (ch->size - victim->size) * 15;
    else
        chance += (ch->size - victim->size) * 10;

    /* stats */
    chance += get_curr_stat(ch, STAT_STR);
    chance -= (get_curr_stat(victim, STAT_DEX) * 4) / 3;
    chance -= GET_AC(victim, AC_BASH) / 25;
    /* speed */
    if (IS_SET(ch->atk_flags, ATK_FAST) || IS_AFFECTED(ch, AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->atk_flags, ATK_FAST) || IS_AFFECTED(victim, AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    if (!IS_NPC(victim) && chance < get_skill(victim, gsn_dodge)) {
        chance -= 3 * (get_skill(victim, gsn_dodge) - chance);
    }

    /* now the attack */
    if (number_percent() < chance) {
        act(COLOR_FIGHT_SKILL "$n sends you sprawling with a powerful bash!" COLOR_CLEAR , ch, NULL,
            victim, TO_VICT);
        act(COLOR_FIGHT_SKILL "You slam into $N, and send $M flying!" COLOR_CLEAR , ch, NULL, victim,
            TO_CHAR);
        act(COLOR_FIGHT_SKILL "$n sends $N sprawling with a powerful bash." COLOR_CLEAR , ch, NULL, victim,
            TO_NOTVICT);
        check_improve(ch, gsn_bash, true, 1);

        DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
        WAIT_STATE(ch, skill_table[gsn_bash].beats);
        victim->position = POS_RESTING;
        damage(ch, victim, number_range(2, 2 + 2 * ch->size + chance / 20),
               gsn_bash, DAM_BASH, false);
    }
    else {
        damage(ch, victim, 0, gsn_bash, DAM_BASH, false);
        act(COLOR_FIGHT_SKILL "You fall flat on your face!" COLOR_CLEAR , ch, NULL, victim, TO_CHAR);
        act(COLOR_FIGHT_SKILL "$n falls flat on $s face." COLOR_CLEAR , ch, NULL, victim, TO_NOTVICT);
        act(COLOR_FIGHT_SKILL "You evade $n's bash, causing $m to fall flat on $s face." COLOR_CLEAR , ch,
            NULL, victim, TO_VICT);
        check_improve(ch, gsn_bash, false, 1);
        ch->position = POS_RESTING;
        WAIT_STATE(ch, skill_table[gsn_bash].beats * 3 / 2);
    }
    check_killer(ch, victim);
}

void do_dirt(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;
    int chance;

    one_argument(argument, arg);

    if ((chance = get_skill(ch, gsn_dirt)) == 0
        || (IS_NPC(ch) && !IS_SET(ch->atk_flags, ATK_KICK_DIRT))
        || (!IS_NPC(ch)
            && ch->level < SKILL_LEVEL(gsn_dirt, ch))) {
        send_to_char("You get your feet dirty.\n\r", ch);
        return;
    }

    if (arg[0] == '\0') {
        victim = ch->fighting;
        if (victim == NULL) {
            send_to_char("But you aren't in combat!\n\r", ch);
            return;
        }
    }

    else if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(victim, AFF_BLIND)) {
        act("$E's already been blinded.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (victim == ch) {
        send_to_char("Very funny.\n\r", ch);
        return;
    }

    if (is_safe(ch, victim)) return;

    if (IS_NPC(victim) && victim->fighting != NULL
        && !is_same_group(ch, victim->fighting)) {
        send_to_char("Kill stealing is not permitted.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
        act("But $N is such a good friend!", ch, NULL, victim, TO_CHAR);
        return;
    }

    /* modifiers */

    /* dexterity */
    chance += get_curr_stat(ch, STAT_DEX);
    chance -= 2 * get_curr_stat(victim, STAT_DEX);

    /* speed  */
    if (IS_SET(ch->atk_flags, ATK_FAST) || IS_AFFECTED(ch, AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->atk_flags, ATK_FAST) || IS_AFFECTED(victim, AFF_HASTE))
        chance -= 25;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* sloppy hack to prevent false zeroes */
    if (chance % 5 == 0) chance += 1;

    /* terrain */

    switch (ch->in_room->data->sector_type) {
    case SECT_INSIDE:
        chance -= 20;
        break;
    case SECT_CITY:
        chance -= 10;
        break;
    case SECT_FIELD:
        chance += 5;
        break;
    case SECT_FOREST:
        break;
    case SECT_HILLS:
        break;
    case SECT_MOUNTAIN:
        chance -= 10;
        break;
    case SECT_WATER_SWIM:
        chance = 0;
        break;
    case SECT_WATER_NOSWIM:
        chance = 0;
        break;
    case SECT_AIR:
        chance = 0;
        break;
    case SECT_DESERT:
        chance += 10;
        break;
    default:
        break;
    }

    if (chance == 0) {
        send_to_char("There isn't any dirt to kick.\n\r", ch);
        return;
    }

    /* now the attack */
    if (number_percent() < chance) {
        Affect af = { 0 };
        act(COLOR_FIGHT_SKILL "$n is blinded by the dirt in $s eyes!" COLOR_CLEAR , victim, NULL, NULL,
            TO_ROOM);
        act(COLOR_FIGHT_SKILL "$n kicks dirt in your eyes!" COLOR_CLEAR , ch, NULL, victim, TO_VICT);
        damage(ch, victim, number_range(2, 5), gsn_dirt, DAM_NONE, false);
        send_to_char(COLOR_FIGHT_SKILL "You can't see a thing!" COLOR_EOL, victim);
        check_improve(ch, gsn_dirt, true, 2);
        WAIT_STATE(ch, skill_table[gsn_dirt].beats);

        af.where = TO_AFFECTS;
        af.type = gsn_dirt;
        af.level = ch->level;
        af.duration = 0;
        af.location = APPLY_HITROLL;
        af.modifier = -4;
        af.bitvector = AFF_BLIND;

        affect_to_mob(victim, &af);
    }
    else {
        damage(ch, victim, 0, gsn_dirt, DAM_NONE, true);
        check_improve(ch, gsn_dirt, false, 2);
        WAIT_STATE(ch, skill_table[gsn_dirt].beats);
    }
    check_killer(ch, victim);
}

void do_trip(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;
    int chance;

    one_argument(argument, arg);

    if ((chance = get_skill(ch, gsn_trip)) == 0
        || (IS_NPC(ch) && !IS_SET(ch->atk_flags, ATK_TRIP))
        || (!IS_NPC(ch)
            && ch->level < SKILL_LEVEL(gsn_trip, ch))) {
        send_to_char("Tripping?  What's that?\n\r", ch);
        return;
    }

    if (arg[0] == '\0') {
        victim = ch->fighting;
        if (victim == NULL) {
            send_to_char("But you aren't fighting anyone!\n\r", ch);
            return;
        }
    }

    else if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (is_safe(ch, victim)) return;

    if (IS_NPC(victim) && victim->fighting != NULL
        && !is_same_group(ch, victim->fighting)) {
        send_to_char("Kill stealing is not permitted.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(victim, AFF_FLYING)) {
        act("$S feet aren't on the ground.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (victim->position < POS_FIGHTING) {
        act("$N is already down.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (victim == ch) {
        send_to_char(COLOR_FIGHT_SKILL "You fall flat on your face!" COLOR_EOL, ch);
        WAIT_STATE(ch, 2 * skill_table[gsn_trip].beats);
        act(COLOR_FIGHT_SKILL "$n trips over $s own feet!" COLOR_CLEAR , ch, NULL, NULL, TO_ROOM);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
        act("$N is your beloved master.", ch, NULL, victim, TO_CHAR);
        return;
    }

    /* modifiers */

    /* size */
    if (ch->size < victim->size)
        chance += (ch->size - victim->size) * 10; /* bigger = harder to trip */

    /* dex */
    chance += get_curr_stat(ch, STAT_DEX);
    chance -= get_curr_stat(victim, STAT_DEX) * 3 / 2;

    /* speed */
    if (IS_SET(ch->atk_flags, ATK_FAST) || IS_AFFECTED(ch, AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->atk_flags, ATK_FAST) || IS_AFFECTED(victim, AFF_HASTE))
        chance -= 20;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* now the attack */
    if (number_percent() < chance) {
        act(COLOR_FIGHT_SKILL "$n trips you and you go down!" COLOR_CLEAR , ch, NULL, victim, TO_VICT);
        act(COLOR_FIGHT_SKILL "You trip $N and $N goes down!" COLOR_CLEAR , ch, NULL, victim, TO_CHAR);
        act(COLOR_FIGHT_SKILL "$n trips $N, sending $M to the ground." COLOR_CLEAR , ch, NULL, victim,
            TO_NOTVICT);
        check_improve(ch, gsn_trip, true, 1);

        DAZE_STATE(victim, 2 * PULSE_VIOLENCE);
        WAIT_STATE(ch, skill_table[gsn_trip].beats);
        victim->position = POS_RESTING;
        damage(ch, victim, number_range(2, 2 + 2 * victim->size), gsn_trip,
               DAM_BASH, true);
    }
    else {
        damage(ch, victim, 0, gsn_trip, DAM_BASH, true);
        WAIT_STATE(ch, skill_table[gsn_trip].beats * 2 / 3);
        check_improve(ch, gsn_trip, false, 1);
    }
    check_killer(ch, victim);
}

void do_kill(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Kill whom?\n\r", ch);
        return;
    }

    if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }
    /*  Allow player killing
        if ( !IS_NPC(victim) )
        {
            if ( !IS_SET(victim->act_flags, PLR_KILLER)
            &&   !IS_SET(victim->act_flags, PLR_THIEF) )
            {
                send_to_char( "You must MURDER a player.\n\r", ch );
                return;
            }
        }
    */
    if (victim == ch) {
        send_to_char("You hit yourself.  Ouch!\n\r", ch);
        multi_hit(ch, ch, TYPE_UNDEFINED);
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (victim->fighting != NULL && !is_same_group(ch, victim->fighting)) {
        send_to_char("Kill stealing is not permitted.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
        act("$N is your beloved master.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (ch->position == POS_FIGHTING) {
        send_to_char("You do the best you can!\n\r", ch);
        return;
    }

    WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
    check_killer(ch, victim);
    multi_hit(ch, victim, TYPE_UNDEFINED);
    return;
}

void do_murde(Mobile* ch, char* argument)
{
    send_to_char("If you want to MURDER, spell it out.\n\r", ch);
    return;
}

void do_murder(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Murder whom?\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) || (IS_NPC(ch) && IS_SET(ch->act_flags, ACT_PET)))
        return;

    if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim == ch) {
        send_to_char("Suicide is a mortal sin.\n\r", ch);
        return;
    }

    if (is_safe(ch, victim)) return;

    if (IS_NPC(victim) && victim->fighting != NULL
        && !is_same_group(ch, victim->fighting)) {
        send_to_char("Kill stealing is not permitted.\n\r", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
        act("$N is your beloved master.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (ch->position == POS_FIGHTING) {
        send_to_char("You do the best you can!\n\r", ch);
        return;
    }

    WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
    if (IS_NPC(ch))
        sprintf(buf, "Help! I am being attacked by %s!", ch->short_descr);
    else
        sprintf(buf, "Help!  I am being attacked by %s!", NAME_STR(ch));
    do_function(victim, &do_yell, buf);
    check_killer(ch, victim);
    multi_hit(ch, victim, TYPE_UNDEFINED);
    return;
}

void do_backstab(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;
    Object* obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Backstab whom?\n\r", ch);
        return;
    }

    if (ch->fighting != NULL) {
        send_to_char("You're facing the wrong end.\n\r", ch);
        return;
    }

    else if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim == ch) {
        send_to_char("How can you sneak up on yourself?\n\r", ch);
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (IS_NPC(victim) && victim->fighting != NULL
        && !is_same_group(ch, victim->fighting)) {
        send_to_char("Kill stealing is not permitted.\n\r", ch);
        return;
    }

    if ((obj = get_eq_char(ch, WEAR_WIELD)) == NULL) {
        send_to_char("You need to wield a weapon to backstab.\n\r", ch);
        return;
    }

    if (victim->hit < victim->max_hit / 3) {
        act("$N is hurt and suspicious ... you can't sneak up.", ch, NULL,
            victim, TO_CHAR);
        return;
    }

    check_killer(ch, victim);
    WAIT_STATE(ch, skill_table[gsn_backstab].beats);
    if (number_percent() < get_skill(ch, gsn_backstab)
        || (get_skill(ch, gsn_backstab) >= 2 && !IS_AWAKE(victim))) {
        check_improve(ch, gsn_backstab, true, 1);
        multi_hit(ch, victim, gsn_backstab);
    }
    else {
        check_improve(ch, gsn_backstab, false, 1);
        damage(ch, victim, 0, gsn_backstab, DAM_NONE, true);
    }

    return;
}

void do_flee(Mobile* ch, char* argument)
{
    Room* was_in;
    Room* now_in;
    Mobile* victim;
    int attempt;

    if ((victim = ch->fighting) == NULL) {
        if (ch->position == POS_FIGHTING) ch->position = POS_STANDING;
        send_to_char("You aren't fighting anyone.\n\r", ch);
        return;
    }

    was_in = ch->in_room;
    for (attempt = 0; attempt < DIR_MAX; attempt++) {
        RoomExit* room_exit;
        int door;

        door = number_door();
        if ((room_exit = was_in->exit[door]) == 0 || room_exit->to_room == NULL
            || IS_SET(room_exit->exit_flags, EX_CLOSED)
            || number_range(0, ch->daze) != 0
            || (IS_NPC(ch)
                && IS_SET(room_exit->to_room->data->room_flags, ROOM_NO_MOB)))
            continue;

        move_char(ch, door, false);
        if ((now_in = ch->in_room) == was_in) continue;

        ch->in_room = was_in;
        act("$n has fled!", ch, NULL, NULL, TO_ROOM);
        ch->in_room = now_in;

        if (!IS_NPC(ch)) {
            send_to_char("You flee from combat!\n\r", ch);
            if ((ch->ch_class == 2) && (number_percent() < 3 * (ch->level / 2)))
                send_to_char("You snuck away safely.\n\r", ch);
            else {
                send_to_char("You lost 10 exp.\n\r", ch);
                gain_exp(ch, -10);
            }
        }

        stop_fighting(ch, true);
        return;
    }

    send_to_char("PANIC! You couldn't escape!\n\r", ch);
    return;
}

void do_rescue(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;
    Mobile* fch;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Rescue whom?\n\r", ch);
        return;
    }

    if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim == ch) {
        send_to_char("What about fleeing instead?\n\r", ch);
        return;
    }

    if (!IS_NPC(ch) && IS_NPC(victim)) {
        send_to_char("Doesn't need your help!\n\r", ch);
        return;
    }

    if (ch->fighting == victim) {
        send_to_char("Too late.\n\r", ch);
        return;
    }

    if ((fch = victim->fighting) == NULL) {
        send_to_char("That person is not fighting right now.\n\r", ch);
        return;
    }

    if (IS_NPC(fch) && !is_same_group(ch, victim)) {
        send_to_char("Kill stealing is not permitted.\n\r", ch);
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_rescue].beats);
    if (number_percent() > get_skill(ch, gsn_rescue)) {
        send_to_char("You fail the rescue.\n\r", ch);
        check_improve(ch, gsn_rescue, false, 1);
        return;
    }

    act(COLOR_FIGHT_SKILL "You rescue $N!" COLOR_CLEAR , ch, NULL, victim, TO_CHAR);
    act(COLOR_FIGHT_SKILL "$n rescues you!" COLOR_CLEAR , ch, NULL, victim, TO_VICT);
    act(COLOR_FIGHT_SKILL "$n rescues $N!" COLOR_CLEAR , ch, NULL, victim, TO_NOTVICT);
    check_improve(ch, gsn_rescue, true, 1);

    stop_fighting(fch, false);
    stop_fighting(victim, false);

    check_killer(ch, fch);
    set_fighting(ch, fch);
    set_fighting(fch, ch);
    return;
}

void do_kick(Mobile* ch, char* argument)
{
    Mobile* victim;

    if (!IS_NPC(ch)
        && ch->level < SKILL_LEVEL(gsn_kick, ch)) {
        send_to_char("You better leave the martial arts to fighters.\n\r", ch);
        return;
    }

    if (IS_NPC(ch) && !IS_SET(ch->atk_flags, ATK_KICK)) return;

    if ((victim = ch->fighting) == NULL) {
        send_to_char("You aren't fighting anyone.\n\r", ch);
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_kick].beats);
    if (get_skill(ch, gsn_kick) > number_percent()) {
        damage(ch, victim, number_range(1, ch->level), gsn_kick, DAM_BASH,
               true);
        check_improve(ch, gsn_kick, true, 1);
    }
    else {
        damage(ch, victim, 0, gsn_kick, DAM_BASH, true);
        check_improve(ch, gsn_kick, false, 1);
    }
    check_killer(ch, victim);
    return;
}

void do_disarm(Mobile* ch, char* argument)
{
    Mobile* victim;
    Object* obj;
    int chance, hth, ch_weapon, vict_weapon, ch_vict_weapon;

    hth = 0;

    if ((chance = get_skill(ch, gsn_disarm)) == 0) {
        send_to_char("You don't know how to disarm opponents.\n\r", ch);
        return;
    }

    if (get_eq_char(ch, WEAR_WIELD) == NULL
        && ((hth = get_skill(ch, gsn_hand_to_hand)) == 0
            || (IS_NPC(ch) && !IS_SET(ch->atk_flags, ATK_DISARM)))) {
        send_to_char("You must wield a weapon to disarm.\n\r", ch);
        return;
    }

    if ((victim = ch->fighting) == NULL) {
        send_to_char("You aren't fighting anyone.\n\r", ch);
        return;
    }

    if ((obj = get_eq_char(victim, WEAR_WIELD)) == NULL) {
        send_to_char("Your opponent is not wielding a weapon.\n\r", ch);
        return;
    }

    /* find weapon skills */
    ch_weapon = get_weapon_skill(ch, get_weapon_sn(ch));
    vict_weapon = get_weapon_skill(victim, get_weapon_sn(victim));
    ch_vict_weapon = get_weapon_skill(ch, get_weapon_sn(victim));

    /* modifiers */

    /* skill */
    if (get_eq_char(ch, WEAR_WIELD) == NULL)
        chance = chance * hth / 150;
    else
        chance = chance * ch_weapon / 100;

    chance += (ch_vict_weapon / 2 - vict_weapon) / 2;

    /* dex vs. strength */
    chance += get_curr_stat(ch, STAT_DEX);
    chance -= 2 * get_curr_stat(victim, STAT_STR);

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* and now the attack */
    if (number_percent() < chance) {
        WAIT_STATE(ch, skill_table[gsn_disarm].beats);
        disarm(ch, victim);
        check_improve(ch, gsn_disarm, true, 1);
    }
    else {
        WAIT_STATE(ch, skill_table[gsn_disarm].beats);
        act(COLOR_FIGHT_SKILL "You fail to disarm $N." COLOR_CLEAR , ch, NULL, victim, TO_CHAR);
        act(COLOR_FIGHT_SKILL "$n tries to disarm you, but fails." COLOR_CLEAR , ch, NULL, victim,
            TO_VICT);
        act(COLOR_FIGHT_SKILL "$n tries to disarm $N, but fails." COLOR_CLEAR , ch, NULL, victim,
            TO_NOTVICT);
        check_improve(ch, gsn_disarm, false, 1);
    }
    check_killer(ch, victim);
    return;
}

void do_sla(Mobile* ch, char* argument)
{
    send_to_char("If you want to SLAY, spell it out.\n\r", ch);
    return;
}

void do_slay(Mobile* ch, char* argument)
{
    Mobile* victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Slay whom?\n\r", ch);
        return;
    }

    if ((victim = get_mob_room(ch, arg)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (ch == victim) {
        send_to_char("Suicide is a mortal sin.\n\r", ch);
        return;
    }

    if (!IS_NPC(victim) && victim->level >= get_trust(ch)) {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    act(COLOR_FIGHT_DEATH "You slay $M in cold blood!" COLOR_CLEAR , ch, NULL, victim, TO_CHAR);
    act(COLOR_FIGHT_DEATH "$n slays you in cold blood!" COLOR_CLEAR , ch, NULL, victim, TO_VICT);
    act(COLOR_FIGHT_DEATH "$n slays $N in cold blood!" COLOR_CLEAR , ch, NULL, victim, TO_NOTVICT);
    raw_kill(victim);
    return;
}

void do_surrender(Mobile* ch, char* argument)
{
    Mobile* mob;
    if ((mob = ch->fighting) == NULL) {
        send_to_char("But you're not fighting!\n\r", ch);
        return;
    }
    act("You surrender to $N!", ch, NULL, mob, TO_CHAR);
    act("$n surrenders to you!", ch, NULL, mob, TO_VICT);
    act("$n tries to surrender to $N!", ch, NULL, mob, TO_NOTVICT);
    stop_fighting(ch, true);

    if (!IS_NPC(ch) && IS_NPC(mob)) {
        if (HAS_MPROG_TRIGGER(mob, TRIG_SURR) && mp_percent_trigger(mob, ch, NULL, NULL, TRIG_SURR))
            return;
        if (HAS_EVENT_TRIGGER(mob, TRIG_SURR) && raise_surrender_event(ch, mob, number_percent()))
            return;
    }

    act("$N seems to ignore your cowardly act!", ch, NULL, mob, TO_CHAR);
    multi_hit(mob, ch, TYPE_UNDEFINED);
}
