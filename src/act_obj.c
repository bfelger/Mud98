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

#include "act_obj.h"

#include "act_comm.h"
#include "act_wiz.h"
#include "comm.h"
#include "db.h"
#include "fight.h"
#include "handler.h"
#include "interp.h"
#include "magic.h"
#include "mob_prog.h"
#include "note.h"
#include "save.h"
#include "skills.h"
#include "update.h"
#include "weather.h"

#include <entities/descriptor.h>
#include <entities/event.h>
#include <entities/faction.h>
#include <entities/object.h>
#include <entities/player_data.h>

#include <data/events.h>
#include <data/mobile_data.h>
#include <data/player.h>
#include <data/skill.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>

#ifndef _MSC_VER 
#include <sys/time.h>
#endif

// Local functions.
#define CD Mobile
#define OD Object

void wear_obj args((Mobile * ch, Object* obj, bool fReplace));
CD* find_keeper args((Mobile * ch));
int get_cost args((Mobile * keeper, Object* obj, bool fBuy));
void obj_to_keeper args((Object * obj, Mobile* ch));
OD* get_obj_keeper args((Mobile * ch, Mobile* keeper, char* argument));

static bool split_money(Mobile* ch, long amount, bool allow_partial)
{
    if (amount <= 0)
        return false;

    Mobile* gch;
    int members = 0;

    FOR_EACH_ROOM_MOB(gch, ch->in_room) {
        if (is_same_group(gch, ch) && !IS_AFFECTED(gch, AFF_CHARM))
            members++;
    }

    if (members < 2)
        return false;

    long available = mobile_total_copper(ch);
    if (!allow_partial && available < amount)
        return false;
    if (allow_partial && available < amount)
        amount = available;

    if (amount <= 0)
        return false;

    long share = amount / members;
    if (share == 0)
        return false;
    long extra = amount % members;

    mobile_set_money_from_copper(ch, available - amount);
    long ch_share = share + extra;
    mobile_set_money_from_copper(ch, (available - amount) + ch_share);

    int16_t g, s, c;
    char amount_buf[64];
    char share_self_buf[64];
    char share_member_buf[64];

    convert_copper_to_money(amount, &g, &s, &c);
    format_money_string(amount_buf, sizeof(amount_buf), g, s, c, false);

    convert_copper_to_money(ch_share, &g, &s, &c);
    format_money_string(share_self_buf, sizeof(share_self_buf), g, s, c, false);

    convert_copper_to_money(share, &g, &s, &c);
    format_money_string(share_member_buf, sizeof(share_member_buf), g, s, c, false);

    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "You split %s. Your share is %s.\n\r", amount_buf, share_self_buf);
    send_to_char(buf, ch);

    sprintf(buf, "$n splits %s. Your share is %s.", amount_buf, share_member_buf);
    FOR_EACH_ROOM_MOB(gch, ch->in_room) {
        if (gch == ch || !is_same_group(gch, ch) || IS_AFFECTED(gch, AFF_CHARM))
            continue;

        mobile_set_money_from_copper(gch, mobile_total_copper(gch) + share);
        act(buf, ch, NULL, gch, TO_VICT);
    }

    return true;
}

static void describe_money_amount(long copper, char* buf, size_t len, bool compact)
{
    int16_t g = 0, s = 0, c = 0;
    convert_copper_to_money(copper, &g, &s, &c);
    format_money_string(buf, len, g, s, c, compact);
}

void steal_coins_transfer(Mobile* ch, Mobile* victim, long amount)
{
    if (amount <= 0)
        return;

    mobile_set_money_from_copper(ch, mobile_total_copper(ch) + amount);
    mobile_set_money_from_copper(victim, mobile_total_copper(victim) - amount);

    char amount_desc[64];
    describe_money_amount(amount, amount_desc, sizeof(amount_desc), false);

    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "Bingo!  You got %s.\n\r", amount_desc);
    send_to_char(buf, ch);
}



#undef OD
#undef CD

/* RT part of the corpse looting code */

bool can_loot(Mobile* ch, Object* obj)
{
    Mobile *owner, *wch;

    if (IS_IMMORTAL(ch)) return true;

    if (!obj->owner || obj->owner == NULL) return true;

    owner = NULL;
    FOR_EACH_GLOBAL_MOB(wch) {
        if (lox_streq(NAME_FIELD(wch), obj->owner)) {
            owner = wch;
        }
    }

    if (owner == NULL)
        return true;

    if (lox_streq(NAME_FIELD(ch), NAME_FIELD(owner)))
        return true;

    if (!IS_NPC(owner) && IS_SET(owner->act_flags, PLR_CANLOOT))
        return true;

    if (is_same_group(ch, owner))
        return true;

    return false;
}

void get_obj(Mobile* ch, Object* obj, Object* container)
{
    /* variables for AUTOSPLIT */
    Mobile* gch;

    if (!CAN_WEAR(obj, ITEM_TAKE)) {
        send_to_char("You can't take that.\n\r", ch);
        return;
    }

    if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch)) {
        act("$d: you can't carry that many items.", ch, NULL, NAME_STR(obj),
            TO_CHAR);
        return;
    }

    if ((!obj->in_obj || obj->in_obj->carried_by != ch)
        && (get_carry_weight(ch) + get_obj_weight(obj) > can_carry_w(ch))) {
        act("$d: you can't carry that much weight.", ch, NULL, NAME_STR(obj),
            TO_CHAR);
        return;
    }

    if (!can_loot(ch, obj)) {
        act("Corpse looting is not permitted.", ch, NULL, NULL, TO_CHAR);
        return;
    }

    if (obj->in_room != NULL) {
        FOR_EACH_ROOM_MOB(gch, obj->in_room)
            if (gch->on == obj) {
                act("$N appears to be using $p.", ch, obj, gch, TO_CHAR);
                return;
            }
    }

    if (container != NULL) {
        if (VNUM_FIELD(container->prototype) == OBJ_VNUM_PIT
            && get_trust(ch) < obj->level) {
            send_to_char("You are not powerful enough to use it.\n\r", ch);
            return;
        }

        if (VNUM_FIELD(container->prototype) == OBJ_VNUM_PIT
            && !CAN_WEAR(container, ITEM_TAKE)
            && !IS_OBJ_STAT(obj, ITEM_HAD_TIMER))
            obj->timer = 0;
        act("You get $p from $P.", ch, obj, container, TO_CHAR);
        act("$n gets $p from $P.", ch, obj, container, TO_ROOM);
        REMOVE_BIT(obj->extra_flags, ITEM_HAD_TIMER);
        obj_from_obj(obj);
    }
    else {
        act("You get $p.", ch, obj, container, TO_CHAR);
        act("$n gets $p.", ch, obj, container, TO_ROOM);
        obj_from_room(obj);
    }

    if (obj->item_type == ITEM_MONEY) {
        long amount = convert_money_to_copper(
            obj->value[MONEY_VALUE_GOLD],
            obj->value[MONEY_VALUE_SILVER],
            obj->value[MONEY_VALUE_COPPER]);

        if (amount > 0) {
            long total = mobile_total_copper(ch) + amount;
            mobile_set_money_from_copper(ch, total);

            if (IS_SET(ch->act_flags, PLR_AUTOSPLIT))
                split_money(ch, amount, true);
        }

        extract_obj(obj);
    }
    else {
        obj_to_char(obj, ch);
        raise_object_taken_event(obj, ch);
    }

    return;
}

void do_get(Mobile* ch, char* argument)
{
    char arg1[MAX_INPUT_LENGTH] = { 0 };
    char arg2[MAX_INPUT_LENGTH] = { 0 };
    Object* obj;
    Object* container;
    bool found;

    READ_ARG(arg1);
    READ_ARG(arg2);

    if (!str_cmp(arg2, "from")) 
        READ_ARG(arg2);

    /* Get type. */
    if (arg1[0] == '\0') {
        send_to_char("Get what?\n\r", ch);
        return;
    }

    if (arg2[0] == '\0') {
        if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
            /* 'get obj' */
            obj = get_obj_list(ch, arg1, &ch->in_room->objects);
            if (obj == NULL) {
                act("I see no $T here.", ch, NULL, arg1, TO_CHAR);
                return;
            }

            get_obj(ch, obj, NULL);
        }
        else {
            /* 'get all' or 'get all.obj' */
            found = false;
            FOR_EACH_ROOM_OBJ(obj, ch->in_room) {
                if ((arg1[3] == '\0' || is_name(&arg1[4], NAME_STR(obj)))
                    && can_see_obj(ch, obj)) {
                    found = true;
                    get_obj(ch, obj, NULL);
                }
            }

            if (!found) {
                if (arg1[3] == '\0')
                    send_to_char("I see nothing here.\n\r", ch);
                else
                    act("I see no $T here.", ch, NULL, &arg1[4], TO_CHAR);
            }
        }
    }
    else {
        /* 'get ... container' */
        if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2)) {
            send_to_char("You can't do that.\n\r", ch);
            return;
        }

        if ((container = get_obj_here(ch, arg2)) == NULL) {
            act("I see no $T here.", ch, NULL, arg2, TO_CHAR);
            return;
        }

        switch (container->item_type) {
        default:
            send_to_char("That's not a container.\n\r", ch);
            return;

        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
            break;

        case ITEM_CORPSE_PC: {
            if (!can_loot(ch, container)) {
                send_to_char("You can't do that.\n\r", ch);
                return;
            }
        }
        }

        if (IS_SET(container->value[1], CONT_CLOSED)) {
            act("The $d is closed.", ch, NULL, NAME_STR(container), TO_CHAR);
            return;
        }

        if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
            /* 'get obj container' */
            obj = get_obj_list(ch, arg1, &container->objects);
            if (obj == NULL) {
                act("I see nothing like that in the $T.", ch, NULL, arg2,
                    TO_CHAR);
                return;
            }
            get_obj(ch, obj, container);
        }
        else {
            /* 'get all container' or 'get all.obj container' */
            found = false;
            FOR_EACH_OBJ_CONTENT(obj, container) {
                if ((arg1[3] == '\0' || is_name(&arg1[4], NAME_STR(obj)))
                    && can_see_obj(ch, obj)) {
                    found = true;
                    if (VNUM_FIELD(container->prototype) == OBJ_VNUM_PIT
                        && !IS_IMMORTAL(ch)) {
                        send_to_char("Don't be so greedy!\n\r", ch);
                        return;
                    }
                    get_obj(ch, obj, container);
                }
            }

            if (!found) {
                if (arg1[3] == '\0')
                    act("I see nothing in the $T.", ch, NULL, arg2, TO_CHAR);
                else
                    act("I see nothing like that in the $T.", ch, NULL, arg2,
                        TO_CHAR);
            }
        }
    }

    return;
}

void do_put(Mobile* ch, char* argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Object* container;
    Object* obj;

    READ_ARG(arg1);
    READ_ARG(arg2);

    if (!str_cmp(arg2, "in") || !str_cmp(arg2, "on"))
        READ_ARG(arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Put what in what?\n\r", ch);
        return;
    }

    if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2)) {
        send_to_char("You can't do that.\n\r", ch);
        return;
    }

    if ((container = get_obj_here(ch, arg2)) == NULL) {
        act("I see no $T here.", ch, NULL, arg2, TO_CHAR);
        return;
    }

    if (container->item_type != ITEM_CONTAINER) {
        send_to_char("That's not a container.\n\r", ch);
        return;
    }

    if (IS_SET(container->value[1], CONT_CLOSED)) {
        act("The $d is closed.", ch, NULL, NAME_STR(container), TO_CHAR);
        return;
    }

    if (str_cmp(arg1, "all") && str_prefix("all.", arg1)) {
        /* 'put obj container' */
        if ((obj = get_obj_carry(ch, arg1)) == NULL) {
            send_to_char("You do not have that item.\n\r", ch);
            return;
        }

        if (obj == container) {
            send_to_char("You can't fold it into itself.\n\r", ch);
            return;
        }

        if (!can_drop_obj(ch, obj)) {
            send_to_char("You can't let go of it.\n\r", ch);
            return;
        }

        if (WEIGHT_MULT(obj) != 100) {
            send_to_char("You have a feeling that would be a bad idea.\n\r",
                         ch);
            return;
        }

        if (get_obj_weight(obj) + get_true_weight(container)
                > (container->value[0] * 10)
            || get_obj_weight(obj) > (container->value[3] * 10)) {
            send_to_char("It won't fit.\n\r", ch);
            return;
        }

        if (VNUM_FIELD(container->prototype) == OBJ_VNUM_PIT 
            && !CAN_WEAR(container, ITEM_TAKE)) {
            if (obj->timer)
                SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
            else
                obj->timer = (int16_t)number_range(100, 200);
        }

        obj_from_char(obj);
        obj_to_obj(obj, container);

        if (IS_SET(container->value[1], CONT_PUT_ON)) {
            act("$n puts $p on $P.", ch, obj, container, TO_ROOM);
            act("You put $p on $P.", ch, obj, container, TO_CHAR);
        }
        else {
            act("$n puts $p in $P.", ch, obj, container, TO_ROOM);
            act("You put $p in $P.", ch, obj, container, TO_CHAR);
        }
    }
    else {
        /* 'put all container' or 'put all.obj container' */
        FOR_EACH_MOB_OBJ(obj, ch) {

            if ((arg1[3] == '\0' || is_name(&arg1[4], NAME_STR(obj)))
                && can_see_obj(ch, obj) && WEIGHT_MULT(obj) == 100
                && obj->wear_loc == WEAR_UNHELD && obj != container
                && can_drop_obj(ch, obj)
                && get_obj_weight(obj) + get_true_weight(container)
                       <= (container->value[0] * 10)
                && get_obj_weight(obj) < (container->value[3] * 10)) {
                if (VNUM_FIELD(container->prototype) == OBJ_VNUM_PIT && !CAN_WEAR(
                    obj, ITEM_TAKE)) {
                    if (obj->timer)
                        SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
                    else
                        obj->timer = (int16_t)number_range(100, 200);
                }
                obj_from_char(obj);
                obj_to_obj(obj, container);

                if (IS_SET(container->value[1], CONT_PUT_ON)) {
                    act("$n puts $p on $P.", ch, obj, container, TO_ROOM);
                    act("You put $p on $P.", ch, obj, container, TO_CHAR);
                }
                else {
                    act("$n puts $p in $P.", ch, obj, container, TO_ROOM);
                    act("You put $p in $P.", ch, obj, container, TO_CHAR);
                }
            }
        }
    }

    return;
}

void do_drop(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    Object* obj;
    bool found;

    READ_ARG(arg);

    if (arg[0] == '\0') {
        send_to_char("Drop what?\n\r", ch);
        return;
    }

    if (is_number(arg)) {
        int amount = atoi(arg);
        READ_ARG(arg);

        if (amount <= 0) {
            send_to_char("Sorry, you can't do that.\n\r", ch);
            return;
        }

        if (arg[0] == '\0') {
            send_to_char("Specify cp, sp, or gp when dropping coins.\n\r", ch);
            return;
        }

        if (!str_cmp(arg, "coins") || !str_cmp(arg, "coin")) {
            send_to_char("Specify cp, sp, or gp when dropping coins.\n\r", ch);
            return;
        }

        MoneyType money_type;
        if (!parse_money_type(arg, &money_type)) {
            send_to_char("You can only drop copper (cp), silver (sp), or gold (gp).\n\r", ch);
            return;
        }

        int16_t* reserve = money_type_ptr(ch, money_type);
        if (*reserve < amount) {
            sprintf(buf, "You don't have that much %s.\n\r", money_type_token(money_type));
            send_to_char(buf, ch);
            return;
        }

        *reserve -= (int16_t)amount;

        long total_copper = (long)amount * money_type_value(money_type);
        FOR_EACH_ROOM_OBJ(obj, ch->in_room) {
            if (obj->item_type != ITEM_MONEY)
                continue;

            total_copper += convert_money_to_copper(
                obj->value[MONEY_VALUE_GOLD],
                obj->value[MONEY_VALUE_SILVER],
                obj->value[MONEY_VALUE_COPPER]);
            extract_obj(obj);
        }

        int16_t gold, silver, copper;
        convert_copper_to_money(total_copper, &gold, &silver, &copper);

        Object* coins = create_money(gold, silver, copper);
        obj_to_room(coins, ch->in_room);

        char amount_desc[64];
        sprintf(amount_desc, "%d %s", amount, money_type_label(money_type, amount));

        sprintf(buf, "$n drops %s.", amount_desc);
        act(buf, ch, NULL, NULL, TO_ROOM);
        sprintf(buf, "You drop %s.\n\r", amount_desc);
        send_to_char(buf, ch);
        return;
    }

    if (str_cmp(arg, "all") && str_prefix("all.", arg)) {
        /* 'drop obj' */
        if ((obj = get_obj_carry(ch, arg)) == NULL) {
            send_to_char("You do not have that item.\n\r", ch);
            return;
        }

        if (!can_drop_obj(ch, obj)) {
            send_to_char("You can't let go of it.\n\r", ch);
            return;
        }

        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
        raise_object_dropped_event(obj, ch);
        act("$n drops $p.", ch, obj, NULL, TO_ROOM);
        act("You drop $p.", ch, obj, NULL, TO_CHAR);
        if (IS_OBJ_STAT(obj, ITEM_MELT_DROP)) {
            act("$p dissolves into smoke.", ch, obj, NULL, TO_ROOM);
            act("$p dissolves into smoke.", ch, obj, NULL, TO_CHAR);
            extract_obj(obj);
        }
    }
    else {
        /* 'drop all' or 'drop all.obj' */
        found = false;
        FOR_EACH_MOB_OBJ(obj, ch) {
            if ((arg[3] == '\0' || is_name(&arg[4], NAME_STR(obj)))
                && can_see_obj(ch, obj) && obj->wear_loc == WEAR_UNHELD
                && can_drop_obj(ch, obj)) {
                found = true;
                obj_from_char(obj);
                obj_to_room(obj, ch->in_room);
                raise_object_dropped_event(obj, ch);
                act("$n drops $p.", ch, obj, NULL, TO_ROOM);
                act("You drop $p.", ch, obj, NULL, TO_CHAR);
                if (IS_OBJ_STAT(obj, ITEM_MELT_DROP)) {
                    act("$p dissolves into smoke.", ch, obj, NULL, TO_ROOM);
                    act("$p dissolves into smoke.", ch, obj, NULL, TO_CHAR);
                    extract_obj(obj);
                }
            }
        }

        if (!found) {
            if (arg[3] == '\0')
                act("You are not carrying anything.", ch, NULL, arg, TO_CHAR);
            else
                act("You are not carrying any $T.", ch, NULL, &arg[4], TO_CHAR);
        }
    }

    return;
}

void do_give(Mobile* ch, char* argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    Mobile* victim;
    Object* obj;

    READ_ARG(arg1);

    READ_ARG(arg2);    
    if (!str_cmp(arg2, "to"))
        READ_ARG(arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Give what to whom?\n\r", ch);
        return;
    }

    if (is_number(arg1)) {
        int amount = atoi(arg1);
        if (amount <= 0) {
            send_to_char("Sorry, you can't do that.\n\r", ch);
            return;
        }

        if (!str_cmp(arg2, "coin") || !str_cmp(arg2, "coins")) {
            send_to_char("Specify cp, sp, or gp when giving coins.\n\r", ch);
            return;
        }

        MoneyType money_type;
        if (!parse_money_type(arg2, &money_type)) {
            send_to_char("You can only give copper (cp), silver (sp), or gold (gp).\n\r", ch);
            return;
        }

        READ_ARG(arg2);
        if (!str_cmp(arg2, "to"))
            READ_ARG(arg2);

        if (arg2[0] == '\0') {
            send_to_char("Give what to whom?\n\r", ch);
            return;
        }

        if ((victim = get_mob_room(ch, arg2)) == NULL) {
            send_to_char("They aren't here.\n\r", ch);
            return;
        }

        int16_t* giver_amt = money_type_ptr(ch, money_type);
        if (*giver_amt < amount) {
            send_to_char("You haven't got that much.\n\r", ch);
            return;
        }

        *giver_amt -= (int16_t)amount;
        int16_t* victim_amt = money_type_ptr(victim, money_type);
        *victim_amt += (int16_t)amount;

        char amount_desc[64];
        sprintf(amount_desc, "%d %s", amount, money_type_label(money_type, amount));

        sprintf(buf, "$n gives you %s.", amount_desc);
        act(buf, ch, NULL, victim, TO_VICT);
        sprintf(buf, "$n gives $N %s.", amount_desc);
        act(buf, ch, NULL, victim, TO_NOTVICT);
        sprintf(buf, "You give $N %s.", amount_desc);
        act(buf, ch, NULL, victim, TO_CHAR);

        long copper_amount = (long)amount * money_type_value(money_type);
        int silver_units = (int)UMIN(INT_MAX, copper_amount / COPPER_PER_SILVER);

        if (silver_units > 0 && IS_NPC(victim)) {
            if (HAS_MPROG_TRIGGER(victim, TRIG_BRIBE))
                mp_bribe_trigger(victim, ch, silver_units);
            if (HAS_EVENT_TRIGGER(victim, TRIG_BRIBE))
                raise_bribe_event(victim, ch, silver_units);
        }

        if (IS_NPC(victim) && IS_SET(victim->act_flags, ACT_IS_CHANGER)) {
            if (money_type == MONEY_TYPE_COPPER) {
                act("$n tells you 'I only exchange silver and gold.'", victim, NULL, ch, TO_VICT);
                ch->reply = victim;
                *giver_amt += (int16_t)amount;
                *victim_amt -= (int16_t)amount;
                return;
            }

            long payout_copper = copper_amount * 95 / 100;
            MoneyType target_type = money_type == MONEY_TYPE_SILVER
                ? MONEY_TYPE_GOLD
                : MONEY_TYPE_SILVER;
            long minimum_unit = money_type_value(target_type);

            if (payout_copper < minimum_unit) {
                act("$n tells you 'I'm sorry, you did not give me enough to change.'",
                    victim, NULL, ch, TO_VICT);
                ch->reply = victim;
                *giver_amt += (int16_t)amount;
                *victim_amt -= (int16_t)amount;
                return;
            }

            long player_total = mobile_total_copper(ch);
            mobile_set_money_from_copper(ch, player_total + payout_copper);

            long victim_total = mobile_total_copper(victim);
            if (victim_total < payout_copper)
                victim_total = payout_copper;
            mobile_set_money_from_copper(victim, victim_total - payout_copper);

            int16_t pg, ps, pc;
            convert_copper_to_money(payout_copper, &pg, &ps, &pc);
            char payout_desc[64];
            format_money_string(payout_desc, sizeof(payout_desc), pg, ps, pc, false);

            act("$n tells you 'Thank you, come again.'", victim, NULL, ch, TO_VICT);
            sprintf(buf, "$n gives you %s in change.", payout_desc);
            act(buf, victim, NULL, ch, TO_VICT);
            ch->reply = victim;
        }

        return;
    }

    if ((obj = get_obj_carry(ch, arg1)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    if (obj->wear_loc != WEAR_UNHELD) {
        send_to_char("You must remove it first.\n\r", ch);
        return;
    }

    if ((victim = get_mob_room(ch, arg2)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC(victim) && victim->prototype->pShop != NULL) {
        act("$N tells you 'Sorry, you'll have to sell that.'", ch, NULL, victim,
            TO_CHAR);
        ch->reply = victim;
        return;
    }

    if (!can_drop_obj(ch, obj)) {
        send_to_char("You can't let go of it.\n\r", ch);
        return;
    }

    if (victim->carry_number + get_obj_number(obj) > can_carry_n(victim)) {
        act("$N has $S hands full.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (get_carry_weight(victim) + get_obj_weight(obj) > can_carry_w(victim)) {
        act("$N can't carry that much weight.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (!can_see_obj(victim, obj)) {
        act("$N can't see it.", ch, NULL, victim, TO_CHAR);
        return;
    }

    obj_from_char(obj);
    obj_to_char(obj, victim);
    events_enabled = false;
    act("$n gives $p to $N.", ch, obj, victim, TO_NOTVICT);
    act("$n gives you $p.", ch, obj, victim, TO_VICT);
    act("You give $p to $N.", ch, obj, victim, TO_CHAR);
    events_enabled = true;

    raise_object_given_event(obj, ch, victim);

    // Give trigger
    if (IS_NPC(victim) && HAS_MPROG_TRIGGER(victim, TRIG_GIVE))
        mp_give_trigger(victim, ch, obj);

    if (IS_NPC(victim) && HAS_EVENT_TRIGGER(victim, TRIG_GIVE))
        raise_give_event(victim, ch, obj);

    return;
}

/* for poisoning weapons and food/drink */
void do_envenom(Mobile* ch, char* argument)
{
    Object* obj;
    Affect af = { 0 };
    int percent, skill;

    /* find out what */
    if (argument[0] == '\0') {
        send_to_char("Envenom what item?\n\r", ch);
        return;
    }

    obj = get_obj_list(ch, argument, &ch->objects);

    if (obj == NULL) {
        send_to_char("You don't have that item.\n\r", ch);
        return;
    }

    if ((skill = get_skill(ch, gsn_envenom)) < 1) {
        send_to_char("Are you crazy? You'd poison yourself!\n\r", ch);
        return;
    }

    if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON) {
        if (IS_OBJ_STAT(obj, ITEM_BLESS) || IS_OBJ_STAT(obj, ITEM_BURN_PROOF)) {
            act("You fail to poison $p.", ch, obj, NULL, TO_CHAR);
            return;
        }

        if (number_percent() < skill) /* success! */
        {
            act("$n treats $p with deadly poison.", ch, obj, NULL, TO_ROOM);
            act("You treat $p with deadly poison.", ch, obj, NULL, TO_CHAR);
            if (!obj->value[3]) {
                obj->value[3] = 1;
                check_improve(ch, gsn_envenom, true, 4);
            }
            WAIT_STATE(ch, skill_table[gsn_envenom].beats);
            return;
        }

        act("You fail to poison $p.", ch, obj, NULL, TO_CHAR);
        if (!obj->value[3])
            check_improve(ch, gsn_envenom, false, 4);
        WAIT_STATE(ch, skill_table[gsn_envenom].beats);
        return;
    }

    if (obj->item_type == ITEM_WEAPON) {
        if (IS_WEAPON_STAT(obj, WEAPON_FLAMING)
            || IS_WEAPON_STAT(obj, WEAPON_FROST)
            || IS_WEAPON_STAT(obj, WEAPON_VAMPIRIC)
            || IS_WEAPON_STAT(obj, WEAPON_SHARP)
            || IS_WEAPON_STAT(obj, WEAPON_VORPAL)
            || IS_WEAPON_STAT(obj, WEAPON_SHOCKING)
            || IS_OBJ_STAT(obj, ITEM_BLESS)
            || IS_OBJ_STAT(obj, ITEM_BURN_PROOF)) {
            act("You can't seem to envenom $p.", ch, obj, NULL, TO_CHAR);
            return;
        }

        if (obj->value[3] < 0
            || attack_table[obj->value[3]].damage == DAM_BASH) {
            send_to_char("You can only envenom edged weapons.\n\r", ch);
            return;
        }

        if (IS_WEAPON_STAT(obj, WEAPON_POISON)) {
            act("$p is already envenomed.", ch, obj, NULL, TO_CHAR);
            return;
        }

        percent = number_percent();
        if (percent < skill) {
            af.where = TO_WEAPON;
            af.type = gsn_poison;
            af.level = ch->level * (int16_t)percent / 100;
            af.duration = ((ch->level / 2) * (int16_t)percent) / 100;
            af.location = 0;
            af.modifier = 0;
            af.bitvector = WEAPON_POISON;
            affect_to_obj(obj, &af);

            act("$n coats $p with deadly venom.", ch, obj, NULL, TO_ROOM);
            act("You coat $p with venom.", ch, obj, NULL, TO_CHAR);
            check_improve(ch, gsn_envenom, true, 3);
            WAIT_STATE(ch, skill_table[gsn_envenom].beats);
            return;
        }
        else {
            act("You fail to envenom $p.", ch, obj, NULL, TO_CHAR);
            check_improve(ch, gsn_envenom, false, 3);
            WAIT_STATE(ch, skill_table[gsn_envenom].beats);
            return;
        }
    }

    act("You can't poison $p.", ch, obj, NULL, TO_CHAR);
    return;
}

void do_fill(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    Object* obj = NULL;
    Object* fountain = NULL;
    bool found;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Fill what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    found = false;
    FOR_EACH_ROOM_OBJ(fountain, ch->in_room) {
        if (fountain->item_type == ITEM_FOUNTAIN) {
            found = true;
            break;
        }
    }

    if (!found) {
        send_to_char("There is no fountain here!\n\r", ch);
        return;
    }

    if (obj->item_type != ITEM_DRINK_CON) {
        send_to_char("You can't fill that.\n\r", ch);
        return;
    }

    if (obj->value[1] != 0 && obj->value[2] != fountain->value[2]) {
        send_to_char("There is already another liquid in it.\n\r", ch);
        return;
    }

    if (obj->value[1] >= obj->value[0]) {
        send_to_char("Your container is full.\n\r", ch);
        return;
    }

    sprintf(buf, "You fill $p with %s from $P.",
            liquid_table[fountain->value[2]].name);
    act(buf, ch, obj, fountain, TO_CHAR);
    sprintf(buf, "$n fills $p with %s from $P.",
            liquid_table[fountain->value[2]].name);
    act(buf, ch, obj, fountain, TO_ROOM);
    obj->value[2] = fountain->value[2];
    obj->value[1] = obj->value[0];
    return;
}

void do_pour(Mobile* ch, char* argument)
{
    char arg[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    Object *out, *in;
    Mobile* vch = NULL;
    int amount;

    READ_ARG(arg);

    if (arg[0] == '\0' || argument[0] == '\0') {
        send_to_char("Pour what into what?\n\r", ch);
        return;
    }

    if ((out = get_obj_carry(ch, arg)) == NULL) {
        send_to_char("You don't have that item.\n\r", ch);
        return;
    }

    if (out->item_type != ITEM_DRINK_CON) {
        send_to_char("That's not a drink container.\n\r", ch);
        return;
    }

    if (!str_cmp(argument, "out")) {
        if (out->value[1] == 0) {
            send_to_char("It's already empty.\n\r", ch);
            return;
        }

        out->value[1] = 0;
        out->value[3] = 0;
        sprintf(buf, "You invert $p, spilling %s all over the ground.",
                liquid_table[out->value[2]].name);
        act(buf, ch, out, NULL, TO_CHAR);

        sprintf(buf, "$n inverts $p, spilling %s all over the ground.",
                liquid_table[out->value[2]].name);
        act(buf, ch, out, NULL, TO_ROOM);
        return;
    }

    if ((in = get_obj_here(ch, argument)) == NULL) {
        vch = get_mob_room(ch, argument);

        if (vch == NULL) {
            send_to_char("Pour into what?\n\r", ch);
            return;
        }

        in = get_eq_char(vch, WEAR_HOLD);

        if (in == NULL) {
            send_to_char("They aren't holding anything.", ch);
            return;
        }
    }

    if (in->item_type != ITEM_DRINK_CON) {
        send_to_char("You can only pour into other drink containers.\n\r", ch);
        return;
    }

    if (in == out) {
        send_to_char("You cannot change the laws of physics!\n\r", ch);
        return;
    }

    if (in->value[1] != 0 && in->value[2] != out->value[2]) {
        send_to_char("They don't hold the same liquid.\n\r", ch);
        return;
    }

    if (out->value[1] == 0) {
        act("There's nothing in $p to pour.", ch, out, NULL, TO_CHAR);
        return;
    }

    if (in->value[1] >= in->value[0]) {
        act("$p is already filled to the top.", ch, in, NULL, TO_CHAR);
        return;
    }

    amount = UMIN(out->value[1], in->value[0] - in->value[1]);

    int liq = out->value[2];

    in->value[1] += amount;
    out->value[1] -= amount;
    in->value[2] = liq;


    if (vch == NULL) {
        sprintf(buf, "You pour %s from $p into $P.", liquid_table[liq].name);
        act(buf, ch, out, in, TO_CHAR);
        sprintf(buf, "$n pours %s from $p into $P.",  liquid_table[liq].name);
        act(buf, ch, out, in, TO_ROOM);
    }
    else {
        sprintf(buf, "You pour some %s for $N.", liquid_table[liq].name);
        act(buf, ch, NULL, vch, TO_CHAR);
        sprintf(buf, "$n pours you some %s.", liquid_table[liq].name);
        act(buf, ch, NULL, vch, TO_VICT);
        sprintf(buf, "$n pours some %s for $N.", liquid_table[liq].name);
        act(buf, ch, NULL, vch, TO_NOTVICT);
    }
}

void do_drink(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj = NULL;
    int amount;
    int liquid;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        FOR_EACH_ROOM_OBJ(obj, ch->in_room) {
            if (obj->item_type == ITEM_FOUNTAIN) 
                break;
        }

        if (obj == NULL) {
            send_to_char("Drink what?\n\r", ch);
            return;
        }
    }
    else {
        if ((obj = get_obj_here(ch, arg)) == NULL) {
            send_to_char("You can't find it.\n\r", ch);
            return;
        }
    }

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10) {
        send_to_char("You fail to reach your mouth.  *Hic*\n\r", ch);
        return;
    }

    switch (obj->item_type) {
    default:
        send_to_char("You can't drink from that.\n\r", ch);
        return;

    case ITEM_FOUNTAIN:
        if ((liquid = obj->value[2]) < 0) {
            bug("Do_drink: bad liquid number %d.", liquid);
            liquid = obj->value[2] = 0;
        }
        amount = liquid_table[liquid].sip_size * 3;
        break;

    case ITEM_DRINK_CON:
        if (obj->value[1] <= 0) {
            send_to_char("It is already empty.\n\r", ch);
            return;
        }

        if ((liquid = obj->value[2]) < 0) {
            bug("Do_drink: bad liquid number %d.", liquid);
            liquid = obj->value[2] = 0;
        }

        amount = liquid_table[liquid].sip_size;
        amount = UMIN(amount, obj->value[1]);
        break;
    }
    if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
        && ch->pcdata->condition[COND_FULL] > 45) {
        send_to_char("You're too full to drink more.\n\r", ch);
        return;
    }

    act("$n drinks $T from $p.", ch, obj, liquid_table[liquid].name, TO_ROOM);
    act("You drink $T from $p.", ch, obj, liquid_table[liquid].name, TO_CHAR);

    gain_condition(ch, COND_DRUNK, amount * liquid_table[liquid].proof / 36);
    gain_condition(ch, COND_FULL, amount * liquid_table[liquid].full / 4);
    gain_condition(ch, COND_THIRST, amount * liquid_table[liquid].thirst / 10);
    gain_condition(ch, COND_HUNGER, amount * liquid_table[liquid].food / 2);

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
        send_to_char("You feel drunk.\n\r", ch);
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 40)
        send_to_char("You are full.\n\r", ch);
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] > 40)
        send_to_char("Your thirst is quenched.\n\r", ch);

    if (obj->value[3] != 0) {
        /* The drink was poisoned ! */
        Affect af = { 0 };

        act("$n chokes and gags.", ch, NULL, NULL, TO_ROOM);
        send_to_char("You choke and gag.\n\r", ch);
        af.where = TO_AFFECTS;
        af.type = gsn_poison;
        af.level = (int16_t)number_fuzzy(amount);
        af.duration = 3 * (int16_t)amount;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_POISON;
        affect_join(ch, &af);
    }

    if (obj->value[0] > 0) obj->value[1] -= amount;

    return;
}

void do_eat(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;

    one_argument(argument, arg);
    if (arg[0] == '\0') {
        send_to_char("Eat what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    if (!IS_IMMORTAL(ch)) {
        if (obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL) {
            send_to_char("That's not edible.\n\r", ch);
            return;
        }

        if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 40) {
            send_to_char("You are too full to eat more.\n\r", ch);
            return;
        }
    }

    act("$n eats $p.", ch, obj, NULL, TO_ROOM);
    act("You eat $p.", ch, obj, NULL, TO_CHAR);

    if (obj->item_type == ITEM_FOOD) {
        if (!IS_NPC(ch)) {
            int condition;

            condition = ch->pcdata->condition[COND_HUNGER];
            gain_condition(ch, COND_FULL, obj->value[0]);
            gain_condition(ch, COND_HUNGER, obj->value[1]);
            if (condition == 0 && ch->pcdata->condition[COND_HUNGER] > 0)
                send_to_char("You are no longer hungry.\n\r", ch);
            else if (ch->pcdata->condition[COND_FULL] > 40)
                send_to_char("You are full.\n\r", ch);
        }

        if (obj->value[3] != 0) {
            /* The food was poisoned! */
            Affect af = { 0 };

            act("$n chokes and gags.", ch, NULL, NULL, TO_ROOM);
            send_to_char("You choke and gag.\n\r", ch);

            af.where = TO_AFFECTS;
            af.type = gsn_poison;
            af.level = (int16_t)number_fuzzy(obj->value[0]);
            af.duration = 2 * (int16_t)obj->value[0];
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = AFF_POISON;
            affect_join(ch, &af);
        }
    }
    else if (obj->item_type == ITEM_PILL) {
        obj_cast_spell((SKNUM)obj->value[1], (LEVEL)obj->value[0], ch, ch, NULL);
        obj_cast_spell((SKNUM)obj->value[2], (LEVEL)obj->value[0], ch, ch, NULL);
        obj_cast_spell((SKNUM)obj->value[3], (LEVEL)obj->value[0], ch, ch, NULL);
    }

    extract_obj(obj);
    return;
}

// Remove an object.
bool remove_obj(Mobile* ch, WearLocation iWear, bool fReplace)
{
    Object* obj;

    if ((obj = get_eq_char(ch, iWear)) == NULL) 
        return true;

    if (!fReplace) 
        return false;

    if (IS_SET(obj->extra_flags, ITEM_NOREMOVE)) {
        act("You can't remove $p.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    unequip_char(ch, obj);
    act("$n stops using $p.", ch, obj, NULL, TO_ROOM);
    act("You stop using $p.", ch, obj, NULL, TO_CHAR);
    return true;
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void wear_obj(Mobile* ch, Object* obj, bool fReplace)
{
    char buf[MAX_STRING_LENGTH];

    if (ch->level < obj->level) {
        sprintf(buf, "You must be level %d to use this object.\n\r", obj->level);
        send_to_char(buf, ch);
        act("$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if (obj->item_type == ITEM_LIGHT) {
        if (!remove_obj(ch, WEAR_LIGHT, fReplace))
            return;
        act("$n lights $p and holds it.", ch, obj, NULL, TO_ROOM);
        act("You light $p and hold it.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_LIGHT);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_FINGER)) {
        if (get_eq_char(ch, WEAR_FINGER_L) != NULL
            && get_eq_char(ch, WEAR_FINGER_R) != NULL
            && !remove_obj(ch, WEAR_FINGER_L, fReplace)
            && !remove_obj(ch, WEAR_FINGER_R, fReplace))
            return;

        if (get_eq_char(ch, WEAR_FINGER_L) == NULL) {
            act("$n wears $p on $s left finger.", ch, obj, NULL, TO_ROOM);
            act("You wear $p on your left finger.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_FINGER_L);
            return;
        }

        if (get_eq_char(ch, WEAR_FINGER_R) == NULL) {
            act("$n wears $p on $s right finger.", ch, obj, NULL, TO_ROOM);
            act("You wear $p on your right finger.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_FINGER_R);
            return;
        }

        bug("Wear_obj: no free finger.", 0);
        send_to_char("You already wear two rings.\n\r", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_NECK)) {
        if (get_eq_char(ch, WEAR_NECK_1) != NULL
            && get_eq_char(ch, WEAR_NECK_2) != NULL
            && !remove_obj(ch, WEAR_NECK_1, fReplace)
            && !remove_obj(ch, WEAR_NECK_2, fReplace))
            return;

        if (get_eq_char(ch, WEAR_NECK_1) == NULL) {
            act("$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM);
            act("You wear $p around your neck.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_NECK_1);
            return;
        }

        if (get_eq_char(ch, WEAR_NECK_2) == NULL) {
            act("$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM);
            act("You wear $p around your neck.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_NECK_2);
            return;
        }

        bug("Wear_obj: no free neck.", 0);
        send_to_char("You already wear two neck items.\n\r", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_BODY)) {
        if (!remove_obj(ch, WEAR_BODY, fReplace))
            return;
        act("$n wears $p on $s torso.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your torso.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_BODY);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_HEAD)) {
        if (!remove_obj(ch, WEAR_HEAD, fReplace))
            return;
        act("$n wears $p on $s head.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your head.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_HEAD);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_LEGS)) {
        if (!remove_obj(ch, WEAR_LEGS, fReplace))
            return;
        act("$n wears $p on $s legs.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your legs.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_LEGS);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_FEET)) {
        if (!remove_obj(ch, WEAR_FEET, fReplace))
            return;
        act("$n wears $p on $s feet.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your feet.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_FEET);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_HANDS)) {
        if (!remove_obj(ch, WEAR_HANDS, fReplace))
            return;
        act("$n wears $p on $s hands.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your hands.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_HANDS);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_ARMS)) {
        if (!remove_obj(ch, WEAR_ARMS, fReplace))
            return;
        act("$n wears $p on $s arms.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your arms.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_ARMS);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT)) {
        if (!remove_obj(ch, WEAR_ABOUT, fReplace))
            return;
        act("$n wears $p about $s torso.", ch, obj, NULL, TO_ROOM);
        act("You wear $p about your torso.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_ABOUT);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_WAIST)) {
        if (!remove_obj(ch, WEAR_WAIST, fReplace)) 
            return;
        act("$n wears $p about $s waist.", ch, obj, NULL, TO_ROOM);
        act("You wear $p about your waist.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_WAIST);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_WRIST)) {
        if (get_eq_char(ch, WEAR_WRIST_L) != NULL
            && get_eq_char(ch, WEAR_WRIST_R) != NULL
            && !remove_obj(ch, WEAR_WRIST_L, fReplace)
            && !remove_obj(ch, WEAR_WRIST_R, fReplace))
            return;

        if (get_eq_char(ch, WEAR_WRIST_L) == NULL) {
            act("$n wears $p around $s left wrist.", ch, obj, NULL, TO_ROOM);
            act("You wear $p around your left wrist.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_WRIST_L);
            return;
        }

        if (get_eq_char(ch, WEAR_WRIST_R) == NULL) {
            act("$n wears $p around $s right wrist.", ch, obj, NULL, TO_ROOM);
            act("You wear $p around your right wrist.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_WRIST_R);
            return;
        }

        bug("Wear_obj: no free wrist.", 0);
        send_to_char("You already wear two wrist items.\n\r", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD)) {
        Object* weapon;

        if (!remove_obj(ch, WEAR_SHIELD, fReplace)
            && !remove_obj(ch, WEAR_WIELD_OH, fReplace))
            return;

        weapon = get_eq_char(ch, WEAR_WIELD);
        if (weapon != NULL && ch->size < SIZE_LARGE
            && IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS)) {
            send_to_char("Your hands are tied up with your weapon!\n\r", ch);
            return;
        }

        act("$n wears $p as a shield.", ch, obj, NULL, TO_ROOM);
        act("You wear $p as a shield.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_SHIELD);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WIELD)) {
        SKNUM sn;
        int skill;

        if (!remove_obj(ch, WEAR_WIELD, fReplace)) 
            return;

        if (!IS_NPC(ch)
            && get_obj_weight(obj)
                   > (str_mod[get_curr_stat(ch, STAT_STR)].wield * 10)) {
            send_to_char("It is too heavy for you to wield.\n\r", ch);
            return;
        }

        if (!IS_NPC(ch) && ch->size < SIZE_LARGE
            && IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)
            && get_eq_char(ch, WEAR_SHIELD) != NULL) {
            send_to_char("You need two hands free for that weapon.\n\r", ch);
            return;
        }

        act("$n wields $p.", ch, obj, NULL, TO_ROOM);
        act("You wield $p.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_WIELD);

        sn = get_weapon_sn(ch);

        if (sn == gsn_hand_to_hand) 
            return;

        skill = get_weapon_skill(ch, sn);

        if (skill >= 100)
            act("$p feels like a part of you!", ch, obj, NULL, TO_CHAR);
        else if (skill > 85)
            act("You feel quite confident with $p.", ch, obj, NULL, TO_CHAR);
        else if (skill > 70)
            act("You are skilled with $p.", ch, obj, NULL, TO_CHAR);
        else if (skill > 50)
            act("Your skill with $p is adequate.", ch, obj, NULL, TO_CHAR);
        else if (skill > 25)
            act("$p feels a little clumsy in your hands.", ch, obj, NULL, TO_CHAR);
        else if (skill > 1)
            act("You fumble and almost drop $p.", ch, obj, NULL, TO_CHAR);
        else
            act("You don't even know which end is up on $p.", ch, obj, NULL, TO_CHAR);

        return;
    }

    if (CAN_WEAR(obj, ITEM_HOLD)) {

        if (!remove_obj(ch, WEAR_HOLD, fReplace) 
            && !remove_obj(ch, WEAR_WIELD_OH, fReplace))
            return;

        act("$n holds $p in $s hand.", ch, obj, NULL, TO_ROOM);
        act("You hold $p in your hand.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_HOLD);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_FLOAT)) {
        if (!remove_obj(ch, WEAR_FLOAT, fReplace)) 
            return;
        act("$n releases $p to float next to $m.", ch, obj, NULL, TO_ROOM);
        act("You release $p and it floats next to you.", ch, obj, NULL,
            TO_CHAR);
        equip_char(ch, obj, WEAR_FLOAT);
        return;
    }

    if (fReplace) 
        send_to_char("You can't wear, wield, or hold that.\n\r", ch);

    return;
}

void do_wear(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Wear, wield, or hold what?\n\r", ch);
        return;
    }

    if (!str_cmp(arg, "all")) {
        FOR_EACH_MOB_OBJ(obj, ch) {
            if (obj->wear_loc == WEAR_UNHELD && can_see_obj(ch, obj))
                wear_obj(ch, obj, false);
        }
        return;
    }
    else {
        if ((obj = get_obj_carry(ch, arg)) == NULL) {
            send_to_char("You do not have that item.\n\r", ch);
            return;
        }

        wear_obj(ch, obj, true);
    }

    return;
}

void do_remove(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Remove what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_wear(ch, arg)) == NULL) {
        send_to_char("You do not have that item.\n\r", ch);
        return;
    }

    remove_obj(ch, obj->wear_loc, true);
    return;
}

void do_sacrifice(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    Object* obj;
    long reward_copper;
    Mobile* gch;

    one_argument(argument, arg);

    if (arg[0] == '\0' || !str_cmp(arg, NAME_STR(ch))) {
        act("$n offers $mself to Mota, who graciously declines.", ch, NULL,
            NULL, TO_ROOM);
        send_to_char("Mota appreciates your offer and may accept it later.\n\r",
                     ch);
        return;
    }

    obj = get_obj_list(ch, arg, &ch->in_room->objects);
    if (obj == NULL) {
        send_to_char("You can't find it.\n\r", ch);
        return;
    }

    if (obj->item_type == ITEM_CORPSE_PC) {
        if (OBJ_HAS_OBJS(obj)) {
            send_to_char("Mota wouldn't like that.\n\r", ch);
            return;
        }
    }

    if (!CAN_WEAR(obj, ITEM_TAKE) || CAN_WEAR(obj, ITEM_NO_SAC)) {
        act("$p is not an acceptable sacrifice.", ch, obj, NULL, TO_CHAR);
        return;
    }

    if (obj->in_room != NULL) {
        FOR_EACH_ROOM_MOB(gch, obj->in_room)
            if (gch->on == obj) {
                act("$N appears to be using $p.", ch, obj, gch, TO_CHAR);
                return;
            }
    }

    reward_copper = UMAX(1, obj->level * 3);

    if (obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC)
        reward_copper = UMIN(reward_copper, obj->cost);

    char reward_desc[64];
    describe_money_amount(reward_copper, reward_desc, sizeof(reward_desc), false);
    sprintf(buf, "Mota gives you %s for your sacrifice.\n\r", reward_desc);
    send_to_char(buf, ch);

    mobile_set_money_from_copper(ch, mobile_total_copper(ch) + reward_copper);

    if (IS_SET(ch->act_flags, PLR_AUTOSPLIT) && reward_copper > 0)
        split_money(ch, reward_copper, true);

    act("$n sacrifices $p to Mota.", ch, obj, NULL, TO_ROOM);
    wiznet("$N sends up $p as a burnt offering.", ch, obj, WIZ_SACCING, 0, 0);
    extract_obj(obj);
    return;
}

void do_quaff(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Quaff what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg)) == NULL) {
        send_to_char("You do not have that potion.\n\r", ch);
        return;
    }

    if (obj->item_type != ITEM_POTION) {
        send_to_char("You can quaff only potions.\n\r", ch);
        return;
    }

    if (ch->level < obj->level) {
        send_to_char("This liquid is too powerful for you to drink.\n\r", ch);
        return;
    }

    act("$n quaffs $p.", ch, obj, NULL, TO_ROOM);
    act("You quaff $p.", ch, obj, NULL, TO_CHAR);

    obj_cast_spell((SKNUM)obj->value[1], (LEVEL)obj->value[0], ch, ch, NULL);
    obj_cast_spell((SKNUM)obj->value[2], (LEVEL)obj->value[0], ch, ch, NULL);
    obj_cast_spell((SKNUM)obj->value[3], (LEVEL)obj->value[0], ch, ch, NULL);

    extract_obj(obj);
    return;
}

void do_recite(Mobile* ch, char* argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Mobile* victim;
    Object* scroll;
    Object* obj;

    READ_ARG(arg1);
    READ_ARG(arg2);

    if ((scroll = get_obj_carry(ch, arg1)) == NULL) {
        send_to_char("You do not have that scroll.\n\r", ch);
        return;
    }

    if (scroll->item_type != ITEM_SCROLL) {
        send_to_char("You can recite only scrolls.\n\r", ch);
        return;
    }

    if (ch->level < scroll->level) {
        send_to_char("This scroll is too complex for you to comprehend.\n\r",
                     ch);
        return;
    }

    obj = NULL;
    if (arg2[0] == '\0') { victim = ch; }
    else {
        if ((victim = get_mob_room(ch, arg2)) == NULL
            && (obj = get_obj_here(ch, arg2)) == NULL) {
            send_to_char("You can't find it.\n\r", ch);
            return;
        }
    }

    act("$n recites $p.", ch, scroll, NULL, TO_ROOM);
    act("You recite $p.", ch, scroll, NULL, TO_CHAR);

    if (number_percent() >= 20 + get_skill(ch, gsn_scrolls) * 4 / 5) {
        send_to_char("You mispronounce a syllable.\n\r", ch);
        check_improve(ch, gsn_scrolls, false, 2);
    }

    else {
        obj_cast_spell((SKNUM)scroll->value[1], (LEVEL)scroll->value[0], ch, victim, obj);
        obj_cast_spell((SKNUM)scroll->value[2], (LEVEL)scroll->value[0], ch, victim, obj);
        obj_cast_spell((SKNUM)scroll->value[3], (LEVEL)scroll->value[0], ch, victim, obj);
        check_improve(ch, gsn_scrolls, true, 2);
    }

    extract_obj(scroll);
    return;
}

void do_brandish(Mobile* ch, char* argument)
{
    Mobile* vch;
    Object* staff;
    SKNUM sn;

    if ((staff = get_eq_char(ch, WEAR_HOLD)) == NULL) {
        send_to_char("You hold nothing in your hand.\n\r", ch);
        return;
    }

    if (staff->item_type != ITEM_STAFF) {
        send_to_char("You can brandish only with a staff.\n\r", ch);
        return;
    }

    if ((sn = (SKNUM)staff->value[3]) < 0 || sn >= skill_count
        || skill_table[sn].spell_fun == 0) {
        bug("Do_brandish: bad sn %d.", sn);
        return;
    }

    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

    if (staff->value[2] > 0) {
        act("$n brandishes $p.", ch, staff, NULL, TO_ROOM);
        act("You brandish $p.", ch, staff, NULL, TO_CHAR);
        if (ch->level < staff->level
            || number_percent() >= 20 + get_skill(ch, gsn_staves) * 4 / 5) {
            act("You fail to invoke $p.", ch, staff, NULL, TO_CHAR);
            act("...and nothing happens.", ch, NULL, NULL, TO_ROOM);
            check_improve(ch, gsn_staves, false, 2);
        }

        else
            FOR_EACH_ROOM_MOB(vch, ch->in_room) {
                switch (skill_table[sn].target) {
                default:
                    bug("Do_brandish: bad target for sn %d.", sn);
                    return;

                case SKILL_TARGET_IGNORE:
                    if (vch != ch) continue;
                    break;

                case SKILL_TARGET_CHAR_OFFENSIVE:
                    if (IS_NPC(ch) ? IS_NPC(vch) : !IS_NPC(vch)) continue;
                    break;

                case SKILL_TARGET_CHAR_DEFENSIVE:
                    if (IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch)) continue;
                    break;

                case SKILL_TARGET_CHAR_SELF:
                    if (vch != ch) continue;
                    break;
                }

                obj_cast_spell((SKNUM)staff->value[3], (LEVEL)staff->value[0], ch, vch, NULL);
                check_improve(ch, gsn_staves, true, 2);
            }
    }

    if (--staff->value[2] <= 0) {
        act("$n's $p blazes bright and is gone.", ch, staff, NULL, TO_ROOM);
        act("Your $p blazes bright and is gone.", ch, staff, NULL, TO_CHAR);
        extract_obj(staff);
    }

    return;
}

void do_zap(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* victim;
    Object* wand;
    Object* obj;

    one_argument(argument, arg);
    if (arg[0] == '\0' && ch->fighting == NULL) {
        send_to_char("Zap whom or what?\n\r", ch);
        return;
    }

    if ((wand = get_eq_char(ch, WEAR_HOLD)) == NULL) {
        send_to_char("You hold nothing in your hand.\n\r", ch);
        return;
    }

    if (wand->item_type != ITEM_WAND) {
        send_to_char("You can zap only with a wand.\n\r", ch);
        return;
    }

    obj = NULL;
    if (arg[0] == '\0') {
        if (ch->fighting != NULL) { victim = ch->fighting; }
        else {
            send_to_char("Zap whom or what?\n\r", ch);
            return;
        }
    }
    else {
        if ((victim = get_mob_room(ch, arg)) == NULL
            && (obj = get_obj_here(ch, arg)) == NULL) {
            send_to_char("You can't find it.\n\r", ch);
            return;
        }
    }

    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

    if (wand->value[2] > 0) {
        if (victim != NULL) {
            act("$n zaps $N with $p.", ch, wand, victim, TO_NOTVICT);
            act("You zap $N with $p.", ch, wand, victim, TO_CHAR);
            act("$n zaps you with $p.", ch, wand, victim, TO_VICT);
        }
        else {
            act("$n zaps $P with $p.", ch, wand, obj, TO_ROOM);
            act("You zap $P with $p.", ch, wand, obj, TO_CHAR);
        }

        if (ch->level < wand->level
            || number_percent() >= 20 + get_skill(ch, gsn_wands) * 4 / 5) {
            act("Your efforts with $p produce only smoke and sparks.", ch, wand,
                NULL, TO_CHAR);
            act("$n's efforts with $p produce only smoke and sparks.", ch, wand,
                NULL, TO_ROOM);
            check_improve(ch, gsn_wands, false, 2);
        }
        else {
            obj_cast_spell((SKNUM)wand->value[3], (LEVEL)wand->value[0], ch, victim, obj);
            check_improve(ch, gsn_wands, true, 2);
        }
    }

    if (--wand->value[2] <= 0) {
        act("$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM);
        act("Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR);
        extract_obj(wand);
    }

    return;
}

void do_steal(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Mobile* victim;
    Object* obj;
    int percent;

    READ_ARG(arg1);
    READ_ARG(arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0') {
        send_to_char("Steal what from whom?\n\r", ch);
        return;
    }

    if ((victim = get_mob_room(ch, arg2)) == NULL) {
        send_to_char("They aren't here.\n\r", ch);
        return;
    }

    if (victim == ch) {
        send_to_char("That's pointless.\n\r", ch);
        return;
    }

    if (is_safe(ch, victim)) return;

    if (IS_NPC(victim) && victim->position == POS_FIGHTING) {
        send_to_char("Kill stealing is not permitted.\n\r"
                     "You'd better not -- you might get hit.\n\r",
                     ch);
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_steal].beats);
    percent = number_percent();

    if (!IS_AWAKE(victim))
        percent -= 10;
    else if (!can_see(victim, ch))
        percent += 25;
    else
        percent += 50;

    if (((ch->level + 7 < victim->level || ch->level - 7 > victim->level)
         && !IS_NPC(victim) && !IS_NPC(ch))
        || (!IS_NPC(ch) && percent > get_skill(ch, gsn_steal))
        || (!IS_NPC(ch) && !is_clan(ch))) {
        // Failure.
        send_to_char("Oops.\n\r", ch);
        affect_strip(ch, gsn_sneak);
        REMOVE_BIT(ch->affect_flags, AFF_SNEAK);

        act("$n tried to steal from you.\n\r", ch, NULL, victim, TO_VICT);
        act("$n tried to steal from $N.\n\r", ch, NULL, victim, TO_NOTVICT);
        switch (number_range(0, 3)) {
        case 0:
            sprintf(buf, "%s is a lousy thief!", NAME_STR(ch));
            break;
        case 1:
            sprintf(buf, "%s couldn't rob %s way out of a paper bag!", NAME_STR(ch),
                    sex_table[ch->sex].poss);
            break;
        case 2:
            sprintf(buf, "%s tried to rob me!", NAME_STR(ch));
            break;
        case 3:
            sprintf(buf, "Keep your hands out of there, %s!", NAME_STR(ch));
            break;
        }
        if (!IS_AWAKE(victim)) do_function(victim, &do_wake, "");
        if (IS_AWAKE(victim)) do_function(victim, &do_yell, buf);
        if (!IS_NPC(ch)) {
            if (IS_NPC(victim)) {
                check_improve(ch, gsn_steal, false, 2);
                multi_hit(victim, ch, TYPE_UNDEFINED);
            }
            else {
                sprintf(buf, "$N tried to steal from %s.", NAME_STR(victim));
                wiznet(buf, ch, NULL, WIZ_FLAGS, 0, 0);
                if (!IS_SET(ch->act_flags, PLR_THIEF)) {
                    SET_BIT(ch->act_flags, PLR_THIEF);
                    send_to_char("*** You are now a THIEF!! ***\n\r", ch);
                    save_char_obj(ch);
                }
            }
        }

        return;
    }

    if (!str_cmp(arg1, "coin") || !str_cmp(arg1, "coins")
        || !str_cmp(arg1, "gold") || !str_cmp(arg1, "silver")) {
        long victim_copper = mobile_total_copper(victim);
        long amount = victim_copper * number_range(1, ch->level) / MAX_LEVEL;
        if (amount <= 0 && victim_copper > 0)
            amount = 1;
        if (amount <= 0) {
            send_to_char("You couldn't get any coins.\n\r", ch);
            return;
        }

        steal_coins_transfer(ch, victim, amount);
        check_improve(ch, gsn_steal, true, 2);
        return;
    }

    if ((obj = get_obj_carry(victim, arg1)) == NULL) {
        send_to_char("You can't find it.\n\r", ch);
        return;
    }

    if (!can_drop_obj(ch, obj) || IS_SET(obj->extra_flags, ITEM_INVENTORY)
        || obj->level > ch->level) {
        send_to_char("You can't pry it away.\n\r", ch);
        return;
    }

    if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch)) {
        send_to_char("You have your hands full.\n\r", ch);
        return;
    }

    if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch)) {
        send_to_char("You can't carry that much weight.\n\r", ch);
        return;
    }

    obj_from_char(obj);
    obj_to_char(obj, ch);
    act("You pocket $p.", ch, obj, NULL, TO_CHAR);
    check_improve(ch, gsn_steal, true, 2);
    send_to_char("Got it!\n\r", ch);
    return;
}

// Shopping commands.
Mobile* find_keeper(Mobile* ch)
{
    /*char buf[MAX_STRING_LENGTH];*/
    Mobile* keeper = NULL;
    ShopData* pShop = NULL;

    pShop = NULL;
    FOR_EACH_ROOM_MOB(keeper, ch->in_room) {
        if (IS_NPC(keeper) && (pShop = keeper->prototype->pShop) != NULL)
            break;
    }

    if (pShop == NULL) {
        send_to_char("You can't do that here.\n\r", ch);
        return NULL;
    }

    /*
     * Undesirables.
     *
    if ( !IS_NPC(ch) && IS_SET(ch->act_flags, PLR_KILLER) )
    {
        do_function(keeper, &do_say, "Killers are not welcome!");
        sprintf(buf, "%s the KILLER is over here!\n\r", ch->name);
        do_function(keeper, &do_yell, buf );
        return NULL;
    }

    if ( !IS_NPC(ch) && IS_SET(ch->act_flags, PLR_THIEF) )
    {
        do_function(keeper, &do_say, "Thieves are not welcome!");
        sprintf(buf, "%s the THIEF is over here!\n\r", ch->name);
        do_function(keeper, &do_yell, buf );
        return NULL;
    }
        */
    // Shop hours.
    if (time_info.hour < pShop->open_hour) {
        do_function(keeper, &do_say, "Sorry, I am closed. Come back later.");
        return NULL;
    }

    if (time_info.hour > pShop->close_hour) {
        do_function(keeper, &do_say, "Sorry, I am closed. Come back tomorrow.");
        return NULL;
    }

    // Invisible or hidden people.
    if (!can_see(keeper, ch)) {
        do_function(keeper, &do_say, "I don't trade with folks I can't see.");
        return NULL;
    }

    if (faction_block_shopkeeper(keeper, ch))
        return NULL;

    return keeper;
}

/* insert an object at the right spot for the keeper */
void obj_to_keeper(Object* obj, Mobile* ch)
{
    Object* t_obj;
    Node* node;
    Node* node_next = NULL;

    /* see if any duplicates are found */
    for (node = ch->in_room->objects.front; node != NULL; node = node_next) {
        node_next = node->next;
        t_obj = AS_OBJECT(node->value);
        if (obj->prototype == t_obj->prototype
            && !str_cmp(obj->short_descr, t_obj->short_descr)) {
            /* if this is an unlimited item, destroy the new one */
            if (IS_OBJ_STAT(t_obj, ITEM_INVENTORY)) {
                extract_obj(obj);
                return;
            }
            obj->cost = t_obj->cost; /* keep it standard */
            break;
        }
    }

    if (node == NULL) {
        list_push(&ch->objects, OBJ_VAL(obj));
    }
    else {
        list_insert_after(&ch->objects, node, OBJ_VAL(obj));
    }

    obj->carried_by = ch;
    obj->in_room = NULL;
    obj->in_obj = NULL;
    ch->carry_number += (int16_t)get_obj_number(obj);
    ch->carry_weight += (int16_t)get_obj_weight(obj);
}

/* get an object from a shopkeeper's list */
Object* get_obj_keeper(Mobile* ch, Mobile* keeper, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;
    Object* obj_next;
    int number;
    int count;

    number = number_argument(argument, arg);
    count = 0;
    FOR_EACH_MOB_OBJ(obj, keeper) {
        if (obj->wear_loc == WEAR_UNHELD && can_see_obj(keeper, obj)
            && can_see_obj(ch, obj) && is_name(arg, NAME_STR(obj))) {
            if (++count == number) 
                return obj;

            // skip other objects of the same name
            while (obj_loop.next != NULL
                    && (obj_next = AS_OBJECT(obj_loop.next->value)) != NULL
                    && obj->prototype == obj_next->prototype
                    && !str_cmp(obj->short_descr, obj_next->short_descr)) {
                obj_loop.node = obj_loop.next;
                obj_loop.next = obj_loop.next->next;
            }
        }
    }

    return NULL;
}

int get_cost(Mobile* keeper, Object* obj, bool fBuy)
{
    ShopData* pShop;
    int cost;

    if (obj == NULL || (pShop = keeper->prototype->pShop) == NULL) return 0;

    if (fBuy) {
        cost = obj->cost * pShop->profit_buy / 100; 
    }
    else {
        Object* obj2;
        int itype;

        cost = 0;
        for (itype = 0; itype < MAX_TRADE; itype++) {
            if (obj->item_type == pShop->buy_type[itype]) {
                cost = obj->cost * pShop->profit_sell / 100;
                break;
            }
        }

        if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT)) {
            FOR_EACH_MOB_OBJ(obj2, keeper) {
                if (obj->prototype == obj2->prototype 
                && !str_cmp(obj->short_descr, obj2->short_descr)) {
                    if (IS_OBJ_STAT(obj2, ITEM_INVENTORY))
                        cost /= 2;
                    else
                        cost = cost * 3 / 4;
                }
            }
        }
    }

    if (obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND) {
        if (obj->value[1] == 0)
            cost /= 4;
        else
            cost = cost * obj->value[2] / obj->value[1];
    }

    return cost;
}

void do_buy(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    //char buf[MAX_STRING_LENGTH];
    int cost, roll;

    if (argument[0] == '\0') {
        send_to_char("Buy what?\n\r", ch);
        return;
    }

    if (IS_SET(ch->in_room->data->room_flags, ROOM_PET_SHOP)) {
        char arg[MAX_INPUT_LENGTH];
        Mobile* pet;
        Room* petshop_inv;
        Room* plr_in_room;

        smash_tilde(argument);

        if (IS_NPC(ch)) 
            return;

        READ_ARG(arg);

        /* hack to make new thalos pets work */
        if (VNUM_FIELD(ch->in_room) == ROOM_VNUM_PETSHOP)
            petshop_inv = get_room(ch->in_room->area, ROOM_VNUM_PETSHOP_INV);
        else
            petshop_inv = get_room(ch->in_room->area, VNUM_FIELD(ch->in_room) + 1);
        if (petshop_inv == NULL) {
            bug("Do_buy: bad pet shop at vnum %"PRVNUM".", VNUM_FIELD(ch->in_room));
            send_to_char("Sorry, you can't buy that here.\n\r", ch);
            return;
        }

        plr_in_room = ch->in_room;
        ch->in_room = petshop_inv;
        pet = get_mob_room(ch, arg);
        ch->in_room = plr_in_room;

        if (pet == NULL || !IS_SET(pet->act_flags, ACT_PET)) {
            send_to_char("Sorry, you can't buy that here.\n\r", ch);
            return;
        }

        if (ch->pet != NULL) {
            send_to_char("You already own a pet.\n\r", ch);
            return;
        }

        cost = 10 * pet->level * pet->level;

        long cost_copper = (long)cost * COPPER_PER_SILVER;

        if (mobile_total_copper(ch) < cost_copper) {
            send_to_char("You can't afford it.\n\r", ch);
            return;
        }

        if (ch->level < pet->level) {
            send_to_char("You're not powerful enough to master this pet.\n\r",
                         ch);
            return;
        }

        /* haggle */
        roll = number_percent();
        if (roll < get_skill(ch, gsn_haggle)) {
            cost -= cost / 2 * roll / 100;
            sprintf(buf, "You haggle the price down to %d coins.\n\r", cost);
            send_to_char(buf, ch);
            check_improve(ch, gsn_haggle, true, 4);
        }

        cost_copper = (long)cost * COPPER_PER_SILVER;
        deduct_cost(ch, cost_copper);
        pet = create_mobile(pet->prototype);
        SET_BIT(pet->act_flags, ACT_PET);
        SET_BIT(pet->affect_flags, AFF_CHARM);
        pet->comm_flags = COMM_NOTELL | COMM_NOSHOUT | COMM_NOCHANNELS;

        READ_ARG(arg);
        if (arg[0] != '\0') {
            sprintf(buf, "%s %s", NAME_STR(pet), arg);
            NAME_FIELD(pet) = lox_string(buf);
        }

        sprintf(buf, "%sA neck tag says 'I belong to %s'.\n\r",
                pet->description, NAME_STR(ch));
        free_string(pet->description);
        pet->description = str_dup(buf);

        mob_to_room(pet, ch->in_room);
        add_follower(pet, ch);
        pet->leader = ch;
        ch->pet = pet;
        send_to_char("Enjoy your pet.\n\r", ch);
        act("$n bought $N as a pet.", ch, NULL, pet, TO_ROOM);
        return;
    }
    else {
        Mobile* keeper;
        Object *obj, *t_obj;
        char arg[MAX_INPUT_LENGTH];
        int number;
        int count = 0;

        if ((keeper = find_keeper(ch)) == NULL)
            return;

        number = mult_argument(argument, arg);
        obj = get_obj_keeper(ch, keeper, arg);
        cost = get_cost(keeper, obj, true);

        if (number < 1 || number > 99) {
            act("$n tells you 'Get real!", keeper, NULL, ch, TO_VICT);
            return;
        }

        if (cost <= 0 || !can_see_obj(ch, obj)) {
            act("$n tells you 'I don't sell that -- try 'list''.", keeper, NULL,
                ch, TO_VICT);
            ch->reply = keeper;
            return;
        }

        if (!IS_OBJ_STAT(obj, ITEM_INVENTORY)) {
            FOR_EACH_MOB_OBJ(t_obj, keeper) {
                if (t_obj->prototype == obj->prototype
                    && !str_cmp(t_obj->short_descr, obj->short_descr))
                    count++;
                
                if (count >= number)
                    break;
            }

            if (count < number) {
                act("$n tells you 'I don't have that many in stock.", keeper,
                    NULL, ch, TO_VICT);
                ch->reply = keeper;
                return;
            }
        }

        long price_per_item = (long)cost * COPPER_PER_SILVER;
        long total_price = price_per_item * number;

        if (mobile_total_copper(ch) < total_price) {
            if (number > 1)
                act("$n tells you 'You can't afford to buy that many.", keeper,
                    obj, ch, TO_VICT);
            else
                act("$n tells you 'You can't afford to buy $p'.", keeper, obj,
                    ch, TO_VICT);
            ch->reply = keeper;
            return;
        }

        if (obj->level > ch->level) {
            act("$n tells you 'You can't use $p yet'.", keeper, obj, ch,
                TO_VICT);
            ch->reply = keeper;
            return;
        }

        if (ch->carry_number + number * get_obj_number(obj) > can_carry_n(ch)) {
            send_to_char("You can't carry that many items.\n\r", ch);
            return;
        }

        if (ch->carry_weight + number * get_obj_weight(obj) > can_carry_w(ch)) {
            send_to_char("You can't carry that much weight.\n\r", ch);
            return;
        }

        /* haggle */
        roll = number_percent();
        if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT)
            && roll < get_skill(ch, gsn_haggle)) {
            cost -= obj->cost / 2 * roll / 100;
            act("You haggle with $N.", ch, NULL, keeper, TO_CHAR);
            check_improve(ch, gsn_haggle, true, 4);
        }

        price_per_item = (long)cost * COPPER_PER_SILVER;
        total_price = price_per_item * number;

        if (number > 1) {
            sprintf(buf, "$n buys $p[%d].", number);
            act(buf, ch, obj, NULL, TO_ROOM);
            char total_cost_buf[64];
            describe_money_amount(total_price, total_cost_buf, sizeof(total_cost_buf), false);
            sprintf(buf, "You buy $p[%d] for %s.", number, total_cost_buf);
            act(buf, ch, obj, NULL, TO_CHAR);
        }
        else {
            act("$n buys $p.", ch, obj, NULL, TO_ROOM);
            char single_cost_buf[64];
            describe_money_amount(price_per_item, single_cost_buf, sizeof(single_cost_buf), false);
            sprintf(buf, "You buy $p for %s.", single_cost_buf);
            act(buf, ch, obj, NULL, TO_CHAR);
        }
        deduct_cost(ch, total_price);
        mobile_set_money_from_copper(keeper, mobile_total_copper(keeper) + total_price);

        Node* obj_list_node = list_find(&keeper->objects, OBJ_VAL(obj));
        Node* current_node = obj_list_node;

        for (count = 0; count < number; count++) {
            if (IS_SET(obj->extra_flags, ITEM_INVENTORY))
                t_obj = create_object(obj->prototype, obj->level);
            else {
                if (current_node == NULL) {
                    bug("do_buy: ran out of objects while purchasing.", 0);
                    break;
                }
                t_obj = AS_OBJECT(current_node->value);
                current_node = current_node->next;
                obj_from_char(t_obj);
            }

            if (t_obj->timer > 0 && !IS_OBJ_STAT(t_obj, ITEM_HAD_TIMER))
                t_obj->timer = 0;
            REMOVE_BIT(t_obj->extra_flags, ITEM_HAD_TIMER);
            obj_to_char(t_obj, ch);
            if (cost < t_obj->cost) 
                t_obj->cost = cost;
        }
    }
}

void do_list(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_SET(ch->in_room->data->room_flags, ROOM_PET_SHOP)) {
        Room* petshop_inv;
        Mobile* pet;
        bool found;

        /* hack to make new thalos pets work */
        if (VNUM_FIELD(ch->in_room) == ROOM_VNUM_PETSHOP)
            petshop_inv = get_room(ch->in_room->area, ROOM_VNUM_PETSHOP_INV);
        else
            petshop_inv = get_room(ch->in_room->area, VNUM_FIELD(ch->in_room) + 1);

        if (petshop_inv == NULL) {
            bug("Do_list: bad pet shop at vnum %"PRVNUM".", VNUM_FIELD(ch->in_room));
            send_to_char("You can't do that here.\n\r", ch);
            return;
        }

        found = false;
        FOR_EACH_ROOM_MOB(pet, petshop_inv) {
            if (IS_SET(pet->act_flags, ACT_PET)) {
                if (!found) {
                    found = true;
                    send_to_char("Pets for sale:\n\r", ch);
                }
                sprintf(buf, "[%2d] %8d - %s\n\r", pet->level,
                        10 * pet->level * pet->level, pet->short_descr);
                send_to_char(buf, ch);
            }
        }
        if (!found) send_to_char("Sorry, we're out of pets right now.\n\r", ch);
        return;
    }
    else {
        Mobile* keeper;
        Object* obj;
        int cost, count;
        bool found;
        char arg[MAX_INPUT_LENGTH];

        if ((keeper = find_keeper(ch)) == NULL) 
            return;
        one_argument(argument, arg);

        found = false;
        FOR_EACH_MOB_OBJ(obj, keeper) {
            if (obj->wear_loc == WEAR_UNHELD && can_see_obj(ch, obj)
                && (cost = get_cost(keeper, obj, true)) > 0
                && (arg[0] == '\0' || is_name(arg, NAME_STR(obj)))) {
                if (!found) {
                    found = true;
                    send_to_char("[Lv      Price Qty] Item\n\r", ch);
                }

                char price_buf[32];
                describe_money_amount((long)cost * COPPER_PER_SILVER, price_buf, sizeof(price_buf), true);

                if (IS_OBJ_STAT(obj, ITEM_INVENTORY))
                    sprintf(buf, "[%2d %10s -- ] %s\n\r", obj->level, price_buf,
                            obj->short_descr);
                else {
                    count = 1;

                    Object* obj_next;
                    while (obj_loop.next != NULL
                        && (obj_next = AS_OBJECT(obj_loop.next->value)) != NULL
                        && obj->prototype == obj_next->prototype
                        && !str_cmp(obj->short_descr, obj_next->short_descr)) {
                        obj_loop.node = obj_loop.next;
                        obj_loop.next = obj_loop.next->next;
                        count++;
                    }
                    sprintf(buf, "[%2d %10s %2d ] %s\n\r", obj->level, price_buf,
                            count, obj->short_descr);
                }
                send_to_char(buf, ch);
            }
        }

        if (!found) 
            send_to_char("You can't buy anything here.\n\r", ch);
        return;
    }
}

void do_sell(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Mobile* keeper;
    Object* obj;
    int cost, roll;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Sell what?\n\r", ch);
        return;
    }

    if ((keeper = find_keeper(ch)) == NULL)
        return;

    if ((obj = get_obj_carry(ch, arg)) == NULL) {
        act("$n tells you 'You don't have that item'.", keeper, NULL, ch,
            TO_VICT);
        ch->reply = keeper;
        return;
    }

    if (!can_drop_obj(ch, obj)) {
        send_to_char("You can't let go of it.\n\r", ch);
        return;
    }

    if (!can_see_obj(keeper, obj)) {
        act("$n doesn't see what you are offering.", keeper, NULL, ch, TO_VICT);
        return;
    }

    if ((cost = get_cost(keeper, obj, false)) <= 0) {
        act("$n looks uninterested in $p.", keeper, obj, ch, TO_VICT);
        return;
    }

    long cost_copper = (long)cost * COPPER_PER_SILVER;
    long keeper_funds = mobile_total_copper(keeper);
    if (cost_copper > keeper_funds) {
        act("$n tells you 'I'm afraid I don't have enough wealth to buy $p.",
            keeper, obj, ch, TO_VICT);
        return;
    }

    act("$n sells $p.", ch, obj, NULL, TO_ROOM);
    /* haggle */
    roll = number_percent();
    if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT)
        && roll < get_skill(ch, gsn_haggle)) {
        send_to_char("You haggle with the shopkeeper.\n\r", ch);
        cost += obj->cost / 2 * roll / 100;
        cost = UMIN(cost, 95 * get_cost(keeper, obj, true) / 100);
        int max_affordable = (int)(keeper_funds / COPPER_PER_SILVER);
        cost = UMIN(cost, max_affordable);
        check_improve(ch, gsn_haggle, true, 4);
    }
    cost_copper = (long)cost * COPPER_PER_SILVER;
    char payout_buf[64];
    describe_money_amount(cost_copper, payout_buf, sizeof(payout_buf), false);
    sprintf(buf, "You sell $p for %s.", payout_buf);
    act(buf, ch, obj, NULL, TO_CHAR);
    mobile_set_money_from_copper(ch, mobile_total_copper(ch) + cost_copper);
    deduct_cost(keeper, cost_copper);

    if (obj->item_type == ITEM_TRASH || IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT)) {
        extract_obj(obj);
    }
    else {
        obj_from_char(obj);
        if (obj->timer)
            SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
        else
            obj->timer = (int16_t)number_range(50, 100);
        obj_to_keeper(obj, keeper);
    }

    return;
}

void do_value(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Mobile* keeper;
    Object* obj;
    int cost;

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        send_to_char("Value what?\n\r", ch);
        return;
    }

    if ((keeper = find_keeper(ch)) == NULL) return;

    if ((obj = get_obj_carry(ch, arg)) == NULL) {
        act("$n tells you 'You don't have that item'.", keeper, NULL, ch,
            TO_VICT);
        ch->reply = keeper;
        return;
    }

    if (!can_see_obj(keeper, obj)) {
        act("$n doesn't see what you are offering.", keeper, NULL, ch, TO_VICT);
        return;
    }

    if (!can_drop_obj(ch, obj)) {
        send_to_char("You can't let go of it.\n\r", ch);
        return;
    }

    if ((cost = get_cost(keeper, obj, false)) <= 0) {
        act("$n looks uninterested in $p.", keeper, obj, ch, TO_VICT);
        return;
    }

    char value_buf[64];
    describe_money_amount((long)cost * COPPER_PER_SILVER, value_buf, sizeof(value_buf), false);
    sprintf(buf, "$n tells you 'I'll give you %s for $p'.", value_buf);
    act(buf, keeper, obj, ch, TO_VICT);
    ch->reply = keeper;

    return;
}
