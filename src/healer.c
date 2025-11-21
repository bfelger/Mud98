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

#include "merc.h"

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "magic.h"
#include "spell_list.h"

#include "entities/mobile.h"
#include "entities/descriptor.h"

#include "data/damage.h"
#include "data/mobile_data.h"
#include "data/skill.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef _MSC_VER 
#include <sys/time.h>
#endif

typedef struct heal_option_t {
    const char* name;
    const char* desc;
    SpellFunc* spell;
    char* words;
    int cost;
} HealOption;

#define HEAL_OPTION_COUNT 10

static const HealOption heal_options[HEAL_OPTION_COUNT] = {
    {"light",       "cure light wounds",    spell_cure_light,       "judicandus dies",      10 },
    {"serious",     "cure serious wounds",  spell_cure_serious,     "judicandus gzfuajg",   16 },
    {"critical",    "cure critical wounds", spell_cure_critical,    "judicandus qfuhuqar",  25 },
    {"heal",        "healing spell",        spell_heal,             "pzar",                 50 },
    {"blindness",   "cure blindness",       spell_cure_blindness,   "judicandus noselacri", 20 },
    {"disease",     "cure disease",         spell_cure_disease,     "judicandus eugzagz",   15 },
    {"poison",      "cure poison",          spell_cure_poison,      "judicandus sausabru",  25 },
    {"uncurse",     "remove curse",         spell_remove_curse,     "candussido judifgz",   50 },
    {"mana",        "restore mana",         NULL,                   "judicandus energize",  10 },
    {"refresh",     "restore movement",     spell_refresh,          "candusima",            5  },
};

static void heal_send_price(Mobile* ch, const char* label, const char* desc, int cost)
{
    char price_buf[64];
    char line[MAX_STRING_LENGTH];
    int16_t g = 0, s = 0, c = 0;
    convert_copper_to_money((long)cost * COPPER_PER_SILVER, &g, &s, &c);
    format_money_string(price_buf, sizeof(price_buf), g, s, c, false);
    sprintf(line, "  %-10s: %-23s %s\n\r", label, desc, price_buf);
    send_to_char(line, ch);
}

void do_heal(Mobile* ch, char* argument)
{
    char buf[MAX_STRING_LENGTH];
    Mobile* mob = NULL;
    char arg[MAX_INPUT_LENGTH];
    int cost = 0;
    SKNUM sn = 0;
    SpellFunc* spell = NULL;
    char* words = NULL;

    /* check for healer */
    FOR_EACH_ROOM_MOB(mob, ch->in_room) {
        if (IS_NPC(mob) && IS_SET(mob->act_flags, ACT_IS_HEALER)) break;
    }

    if (mob == NULL) {
        send_to_char("You can't do that here.\n\r", ch);
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0') {
        /* display price list */
        act("$N says 'I offer the following spells:'", ch, NULL, mob, TO_CHAR);
        for (int i = 0; i < HEAL_OPTION_COUNT; i++) {
            heal_send_price(ch, heal_options[i].name, heal_options[i].desc, heal_options[i].cost);
        }
        send_to_char("Type heal <type> to be healed.\n\r", ch);
        return;
    }

    bool found = false;
    for (int i = 0; i < HEAL_OPTION_COUNT; i++) {
        if (!str_prefix(arg, heal_options[i].name)) {
            spell = heal_options[i].spell;
            sn = skill_lookup(heal_options[i].desc);
            words = (char*)heal_options[i].words;
            cost = heal_options[i].cost;
            found = true;
            break;
        }
    }

    if (!found) {
        sprintf(buf, "$N says 'I do not offer that service. Type 'heal' for a list.'");
        act(buf, ch, NULL, mob, TO_CHAR);
        return;
    }

    long price = (long)cost * COPPER_PER_SILVER;
    int16_t price_gold = 0, price_silver = 0, price_copper = 0;
    convert_copper_to_money(price, &price_gold, &price_silver, &price_copper);
    char price_buf_local[64];
    format_money_string(price_buf_local, sizeof(price_buf_local), price_gold, price_silver, price_copper, false);
    if (mobile_total_copper(ch) < price) {
        sprintf(buf, " says 'You do not have enough coin for my services. (%s)'", price_buf_local);
        act(buf, ch, NULL, mob, TO_CHAR);
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);

    deduct_cost(ch, price);
    mobile_set_money_from_copper(mob, mobile_total_copper(mob) + price);
    sprintf(buf, "$N accepts %s for the service.", price_buf_local);
    act(buf, ch, NULL, mob, TO_CHAR);
    act("$n utters the words ''.", mob, NULL, words, TO_ROOM);

    if (spell == NULL) /* restore mana trap...kinda hackish */
    {
        ch->mana += (int16_t)dice(2, 8) + mob->level / 3;
        ch->mana = UMIN(ch->mana, ch->max_mana);
        send_to_char("A warm glow passes through you.\n\r", ch);
        return;
    }

    if (sn == -1)
        return;

    spell(sn, mob->level, mob, ch, SPELL_TARGET_CHAR);
}
