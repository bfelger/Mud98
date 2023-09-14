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

// Don't put any extra code or includes, here other than more SPELL() entries. 
// OLC pulls in headers willy-nilly and splices them into list initializors.

#ifndef SPELL
#include "merc.h"
#define SPELL(spell)		    DECLARE_SPELL_FUN(spell);
#endif

// Header guard is also removed. We're ridin' dirty.

/*
 * Spell functions.
 * Defined in magic.c.
 */
SPELL(spell_null)
SPELL(spell_acid_blast)
SPELL(spell_armor)
SPELL(spell_bless)
SPELL(spell_blindness)
SPELL(spell_burning_hands)
SPELL(spell_call_lightning)
SPELL(spell_calm)
SPELL(spell_cancellation)
SPELL(spell_cause_critical)
SPELL(spell_cause_light)
SPELL(spell_cause_serious)
SPELL(spell_change_sex)
SPELL(spell_chain_lightning)
SPELL(spell_charm_person)
SPELL(spell_chill_touch)
SPELL(spell_colour_spray)
SPELL(spell_continual_light)
SPELL(spell_control_weather)
SPELL(spell_create_food)
SPELL(spell_create_rose)
SPELL(spell_create_spring)
SPELL(spell_create_water)
SPELL(spell_cure_blindness)
SPELL(spell_cure_critical)
SPELL(spell_cure_disease)
SPELL(spell_cure_light)
SPELL(spell_cure_poison)
SPELL(spell_cure_serious)
SPELL(spell_curse)
SPELL(spell_demonfire)
SPELL(spell_detect_evil)
SPELL(spell_detect_good)
SPELL(spell_detect_hidden)
SPELL(spell_detect_invis)
SPELL(spell_detect_magic)
SPELL(spell_detect_poison)
SPELL(spell_dispel_evil)
SPELL(spell_dispel_good)
SPELL(spell_dispel_magic)
SPELL(spell_earthquake)
SPELL(spell_enchant_armor)
SPELL(spell_enchant_weapon)
SPELL(spell_energy_drain)
SPELL(spell_faerie_fire)
SPELL(spell_faerie_fog)
SPELL(spell_farsight)
SPELL(spell_fireball)
SPELL(spell_fireproof)
SPELL(spell_flamestrike)
SPELL(spell_floating_disc)
SPELL(spell_fly)
SPELL(spell_frenzy)
SPELL(spell_gate)
SPELL(spell_giant_strength)
SPELL(spell_harm)
SPELL(spell_haste)
SPELL(spell_heal)
SPELL(spell_heat_metal)
SPELL(spell_holy_word)
SPELL(spell_identify)
SPELL(spell_infravision)
SPELL(spell_invis)
SPELL(spell_know_alignment)
SPELL(spell_lightning_bolt)
SPELL(spell_locate_object)
SPELL(spell_magic_missile)
SPELL(spell_mass_healing)
SPELL(spell_mass_invis)
SPELL(spell_nexus)
SPELL(spell_pass_door)
SPELL(spell_plague)
SPELL(spell_poison)
SPELL(spell_portal)
SPELL(spell_protection_evil)
SPELL(spell_protection_good)
SPELL(spell_ray_of_truth)
SPELL(spell_recharge)
SPELL(spell_refresh)
SPELL(spell_remove_curse)
SPELL(spell_sanctuary)
SPELL(spell_shocking_grasp)
SPELL(spell_shield)
SPELL(spell_sleep)
SPELL(spell_slow)
SPELL(spell_stone_skin)
SPELL(spell_summon)
SPELL(spell_teleport)
SPELL(spell_ventriloquate)
SPELL(spell_weaken)
SPELL(spell_word_of_recall)
SPELL(spell_acid_breath)
SPELL(spell_fire_breath)
SPELL(spell_frost_breath)
SPELL(spell_gas_breath)
SPELL(spell_lightning_breath)
SPELL(spell_general_purpose)
SPELL(spell_high_explosive)

#undef SPELL

// Header guard removed
