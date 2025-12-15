/* -------------------------------------------
 * Counter Skill for ROM24, 1.02 - 3/30/98
 * -------------------------------------------
 * The counter skill is an advanced defensive
 * fighting technique that allows a skilled
 * fighter to reverse his opponents attack and
 * actually hit him back with it.
 *
 * As with all else, please email me if you use
 * this code so I know my time is not going
 * to waste. :) And also, with this one I could
 * really use some suggesstions for improvement!
 *
 * Brian Babey (aka Varen)
 * [bribe@erols.com]
 * ------------------------------------------- */


/* -----------------------------------------------
 * In const.c, insert this into the skill table */

//    {
//        "counter",              { 53, 53, 20, 25 },     { 0, 0, 6, 8},
//        spell_null,             TAR_IGNORE,             POS_FIGHTING,
//        &gsn_counter,           SLOT( 0),       0,      0,
//        "counterattack",        "!Counter!",   ""
//    },


/* ----------------------------------------------
 * In merc.h, under Game Parameters, increment
 * the variable MAX_SKILL by 1. */


/* ----------------------------------------------
 * In merc.h, under the new gsns section, insert
 * this line */

//extern sh_int  gsn_counter;


/* ----------------------------------------------
 * In db.c, under new gsns, insert this line: */

//sh_int                  gsn_counter;


/* ----------------------------------------------
 * In fight.c, in the local function declarations,
 * insert this line. */

//bool    check_counter   args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt ) );


/* ---------------------------------------------
 * In fight.c, insert this function wherever
 * it belongs alphabetically (or wherever you'd
 * like it to be - it really doesn't matter) */

#include <comm.h>
#include <fight.h>

#include <entities/mobile.h>
#include <entities/object.h>

#include <stdbool.h>

bool check_counter(Mobile *ch, Mobile *victim, int dam, int dt)
{
        int chance;
        int dam_type;
        Object *wield;

        // Disallow countering for certain attacks
        if (dt == gsn_backstab)
            return false;

        if ((get_eq_char(victim, WEAR_WIELD) == NULL) ||
            (!IS_AWAKE(victim)) ||
            (!can_see(victim, ch)) ||
            (get_skill(victim, gsn_counter) < 1))
           return false;

        wield = get_eq_char(victim, WEAR_WIELD);

        chance = get_skill(victim, gsn_counter) / 6;
        chance += (victim->level - ch->level) / 2;
        chance += 2 * (get_curr_stat(victim, STAT_DEX) - get_curr_stat(ch, STAT_DEX));
        chance += get_weapon_skill(victim, get_weapon_sn(victim)) -
                        get_weapon_skill(ch, get_weapon_sn(ch));
        chance += (get_curr_stat(victim, STAT_STR) - get_curr_stat(ch, STAT_STR) );

    if (number_percent() >= chance)
        return false;

    dt = gsn_counter;

    if (dt == TYPE_UNDEFINED) {
        dt = TYPE_HIT;
        if (wield != NULL && wield->item_type == ITEM_WEAPON)
            dt += wield->value[3];
        else 
                dt += ch->dam_type;
    }

    if (dt < TYPE_HIT) {
    	if (wield != NULL)
    	    dam_type = attack_table[wield->value[3]].damage;
    	else
    	    dam_type = attack_table[ch->dam_type].damage;
    }
    else
    	dam_type = attack_table[dt - TYPE_HIT].damage;

    if (dam_type == -1)
	    dam_type = DAM_BASH;

    act("You reverse $n's attack and counter with your own!", ch, NULL, victim,  TO_VICT);
    act("$N reverses your attack!", ch, NULL, victim, TO_CHAR);

    damage(victim, ch, dam / 2, gsn_counter, dam_type, true); /* DAM MSG NUMBER!! */

    check_improve(victim, gsn_counter, true, 6);

    return true;
}




/* -------------------------------------------
 * In fight.c inside the one_hit function, you
 * need to put this in to replace the line reading
 * result = damage ( ch, victim, dam, dt, da... etc. */

//    if ( !check_counter( ch, victim, dam, dt ) )
//        result = damage( ch, victim, dam, dt, dam_type, TRUE );
//
//    else return;
