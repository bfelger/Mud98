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

#include "handler.h"

#include "act_comm.h"
#include "act_wiz.h"
#include "color.h"
#include "comm.h"
#include "config.h"
#include "db.h"
#include "fight.h"
#include "interp.h"
#include "lookup.h"
#include "magic.h"
#include "recycle.h"
#include "skills.h"
#include "spell_list.h"
#include "tables.h"
#include "vt.h"
#include "weather.h"

#include "entities/area.h"
#include "entities/descriptor.h"
#include "entities/object.h"
#include "entities/player_data.h"

#include "data/class.h"
#include "data/item.h"
#include "data/mobile_data.h"
#include "data/player.h"
#include "data/race.h"
#include "data/skill.h"
#include "data/spell.h"

#include "lox/lox.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

/* friend stuff -- for NPC's mostly */
bool is_friend(Mobile* ch, Mobile* victim)
{
    if (is_same_group(ch, victim)) 
        return true;

    if (!IS_NPC(ch)) 
        return false;

    if (!IS_NPC(victim)) {
        if (IS_SET(ch->atk_flags, ASSIST_PLAYERS))
            return true;
        else
            return false;
    }

    if (IS_AFFECTED(ch, AFF_CHARM)) 
        return false;

    if (IS_SET(ch->atk_flags, ASSIST_ALL)) 
        return true;

    if (ch->group && ch->group == victim->group) 
        return true;

    if (IS_SET(ch->atk_flags, ASSIST_VNUM)
        && ch->prototype == victim->prototype)
        return true;

    if (IS_SET(ch->atk_flags, ASSIST_RACE) && ch->race == victim->race)
        return true;

    if (IS_SET(ch->atk_flags, ASSIST_ALIGN) && !IS_SET(ch->act_flags, ACT_NOALIGN)
        && !IS_SET(victim->act_flags, ACT_NOALIGN)
        && ((IS_GOOD(ch) && IS_GOOD(victim)) || (IS_EVIL(ch) && IS_EVIL(victim))
            || (IS_NEUTRAL(ch) && IS_NEUTRAL(victim))))
        return true;

    return false;
}

/* returns number of people on an object */
int count_users(Object* obj)
{
    Mobile* fch;
    int count = 0;

    if (obj->in_room == NULL) 
        return 0;

    FOR_EACH_ROOM_MOB(fch, obj->in_room)
        if (fch->on == obj) 
            count++;

    return count;
}

/* returns material number */
int material_lookup(const char* name)
{
    return 0;
}

WeaponType weapon_lookup(const char* name)
{
    for (int type = 0; type < WEAPON_TYPE_COUNT; type++) {
        if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
            && !str_prefix(name, weapon_table[type].name))
            return weapon_table[type].type;
    }

    return WEAPON_EXOTIC;
}

int attack_lookup(const char* name)
{
    int att;

    for (att = 0; att < ATTACK_COUNT; att++) {
        if (LOWER(name[0]) == LOWER(attack_table[att].name[0])
            && !str_prefix(name, attack_table[att].name))
            return att;
    }

    return 0;
}

/* returns a flag for wiznet */
int wiznet_lookup(const char* name)
{
    int flag;

    for (flag = 0; wiznet_table[flag].name != NULL; flag++) {
        if (LOWER(name[0]) == LOWER(wiznet_table[flag].name[0])
            && !str_prefix(name, wiznet_table[flag].name))
            return flag;
    }

    return -1;
}

/* returns class number */
int16_t class_lookup(const char* name)
{
    int16_t class;

    for (class = 0; class < class_count; class++) {
        if (LOWER(name[0]) == LOWER(class_table[class].name[0])
            && !str_prefix(name, class_table[class].name))
            return class;
    }

    return -1;
}

/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.c */

ResistType check_immune(Mobile* ch, DamageType dam_type)
{
    ResistType immune, def;
    FLAGS bit;

    immune = IS_NORMAL;
    def = IS_NORMAL;

    if (dam_type == DAM_NONE)
        return immune;

    if (dam_type <= (DamageType)3) {
        // Physical attack
        if (IS_SET(ch->imm_flags, IMM_WEAPON))
            def = IS_IMMUNE;
        else if (IS_SET(ch->res_flags, RES_WEAPON))
            def = IS_RESISTANT;
        else if (IS_SET(ch->vuln_flags, VULN_WEAPON))
            def = IS_VULNERABLE;
    }
    else {
        // Magic attack
        if (IS_SET(ch->imm_flags, IMM_MAGIC))
            def = IS_IMMUNE;
        else if (IS_SET(ch->res_flags, RES_MAGIC))
            def = IS_RESISTANT;
        else if (IS_SET(ch->vuln_flags, VULN_MAGIC))
            def = IS_VULNERABLE;
    }

    /* set bits to check -- VULN etc. must ALL be the same or this will fail */
    switch (dam_type) {
    case DAM_BASH:
        bit = IMM_BASH;
        break;
    case DAM_PIERCE:
        bit = IMM_PIERCE;
        break;
    case DAM_SLASH:
        bit = IMM_SLASH;
        break;
    case DAM_FIRE:
        bit = IMM_FIRE;
        break;
    case DAM_COLD:
        bit = IMM_COLD;
        break;
    case DAM_LIGHTNING:
        bit = IMM_LIGHTNING;
        break;
    case DAM_ACID:
        bit = IMM_ACID;
        break;
    case DAM_POISON:
        bit = IMM_POISON;
        break;
    case DAM_NEGATIVE:
        bit = IMM_NEGATIVE;
        break;
    case DAM_HOLY:
        bit = IMM_HOLY;
        break;
    case DAM_ENERGY:
        bit = IMM_ENERGY;
        break;
    case DAM_MENTAL:
        bit = IMM_MENTAL;
        break;
    case DAM_DISEASE:
        bit = IMM_DISEASE;
        break;
    case DAM_DROWNING:
        bit = IMM_DROWNING;
        break;
    case DAM_LIGHT:
        bit = IMM_LIGHT;
        break;
    case DAM_CHARM:
        bit = IMM_CHARM;
        break;
    case DAM_SOUND:
        bit = IMM_SOUND;
        break;
    default:
        return def;
    }

    if (IS_SET(ch->imm_flags, bit))
        immune = IS_IMMUNE;
    else if (IS_SET(ch->res_flags, bit) && immune != IS_IMMUNE)
        immune = IS_RESISTANT;
    else if (IS_SET(ch->vuln_flags, bit)) {
        if (immune == IS_IMMUNE)
            immune = IS_RESISTANT;
        else if (immune == IS_RESISTANT)
            immune = IS_NORMAL;
        else
            immune = IS_VULNERABLE;
    }

    if (immune == IS_NORMAL)
        return def;
    else
        return immune;
}

bool is_clan(Mobile* ch)
{
    return ch->clan;
}

bool is_same_clan(Mobile* ch, Mobile* victim)
{
    if (clan_table[ch->clan].independent)
        return false;
    else
        return (ch->clan == victim->clan);
}

/* for returning skill information */
int get_skill(Mobile* ch, SKNUM sn)
{
    int skill;

    if (sn == -1) /* shorthand for level based skills */
    {
        skill = ch->level * 5 / 2;
    }
    else if (sn < -1 || sn > skill_count) {
        bug("Bad sn %d in get_skill.", sn);
        skill = 0;
    }
    else if (!IS_NPC(ch)) {
        if (ch->level < SKILL_LEVEL(sn, ch))
            skill = 0;
        else
            skill = ch->pcdata->learned[sn];
    }
    else {
        // Mobiles
        if (skill_table[sn].spell_fun != spell_null)
            skill = 40 + 2 * ch->level;

        else if (sn == gsn_sneak || sn == gsn_hide)
            skill = ch->level * 2 + 20;

        else if ((sn == gsn_dodge && IS_SET(ch->atk_flags, ATK_DODGE))
                 || (sn == gsn_parry && IS_SET(ch->atk_flags, ATK_PARRY)))
            skill = ch->level * 2;

        else if (sn == gsn_shield_block)
            skill = 10 + 2 * ch->level;

        else if (sn == gsn_second_attack
                 && (IS_SET(ch->act_flags, ACT_WARRIOR)
                     || IS_SET(ch->act_flags, ACT_THIEF)))
            skill = 10 + 3 * ch->level;

        else if (sn == gsn_third_attack && IS_SET(ch->act_flags, ACT_WARRIOR))
            skill = 4 * ch->level - 40;

        else if (sn == gsn_hand_to_hand)
            skill = 40 + 2 * ch->level;

        else if (sn == gsn_trip && IS_SET(ch->atk_flags, ATK_TRIP))
            skill = 10 + 3 * ch->level;

        else if (sn == gsn_bash && IS_SET(ch->atk_flags, ATK_BASH))
            skill = 10 + 3 * ch->level;

        else if (sn == gsn_disarm
                 && (IS_SET(ch->atk_flags, ATK_DISARM)
                     || IS_SET(ch->act_flags, ACT_WARRIOR)
                     || IS_SET(ch->act_flags, ACT_THIEF)))
            skill = 20 + 3 * ch->level;

        else if (sn == gsn_berserk && IS_SET(ch->atk_flags, ATK_BERSERK))
            skill = 3 * ch->level;

        else if (sn == gsn_kick)
            skill = 10 + 3 * ch->level;

        else if (sn == gsn_backstab && IS_SET(ch->act_flags, ACT_THIEF))
            skill = 20 + 2 * ch->level;

        else if (sn == gsn_rescue)
            skill = 40 + ch->level;

        else if (sn == gsn_recall)
            skill = 40 + ch->level;

        else if (sn == gsn_sword || sn == gsn_dagger || sn == gsn_spear
                 || sn == gsn_mace || sn == gsn_axe || sn == gsn_flail
                 || sn == gsn_whip || sn == gsn_polearm)
            skill = 40 + 5 * ch->level / 2;

        else
            skill = 0;
    }

    if (ch->daze > 0) {
        if (skill_table[sn].spell_fun != spell_null)
            skill /= 2;
        else
            skill = 2 * skill / 3;
    }

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
        skill = 9 * skill / 10;

    return URANGE(0, skill, 100);
}

/* for returning weapon information */
SKNUM get_weapon_sn(Mobile* ch)
{
    Object* wield;

    wield = get_eq_char(ch, WEAR_WIELD);
    if (wield == NULL || wield->item_type != ITEM_WEAPON)
        return gsn_hand_to_hand;
    
    return *(weapon_table[wield->value[0]].gsn);
}

int get_weapon_skill(Mobile* ch, SKNUM sn)
{
    int skill;

    /* -1 is exotic */
    if (IS_NPC(ch)) {
        if (sn == -1)
            skill = 3 * ch->level;
        else if (sn == gsn_hand_to_hand)
            skill = 40 + 2 * ch->level;
        else
            skill = 40 + 5 * ch->level / 2;
    }

    else {
        if (sn == -1)
            skill = 3 * ch->level;
        else
            skill = ch->pcdata->learned[sn];
    }

    return URANGE(0, skill, 100);
}

/* used to de-screw characters */
void reset_char(Mobile* ch)
{
    int loc;
    int stat;
    int16_t mod;
    Object* obj;
    Affect* af;
    int i;

    if (IS_NPC(ch)) return;

    if (ch->pcdata->perm_hit == 0 || ch->pcdata->perm_mana == 0
        || ch->pcdata->perm_move == 0 || ch->pcdata->last_level == 0) {
        /* do a FULL reset */
        for (loc = 0; loc < WEAR_LOC_COUNT; loc++) {
            obj = get_eq_char(ch, loc);
            if (obj == NULL) continue;
            if (!obj->enchanted)
                for (af = obj->prototype->affected; af != NULL;
                     NEXT_LINK(af)) {
                    mod = af->modifier;
                    switch (af->location) {
                    case APPLY_SEX:
                        ch->sex -= mod;
                        if (ch->sex < 0 || ch->sex >= SEX_COUNT)
                            ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
                        break;
                    case APPLY_MANA:
                        ch->max_mana -= mod;
                        break;
                    case APPLY_HIT:
                        ch->max_hit -= mod;
                        break;
                    case APPLY_MOVE:
                        ch->max_move -= mod;
                        break;
                    default:
                        break;
                    }
                }

            FOR_EACH(af, obj->affected) {
                mod = af->modifier;
                switch (af->location) {
                case APPLY_SEX:
                    ch->sex -= mod;
                    break;
                case APPLY_MANA:
                    ch->max_mana -= mod;
                    break;
                case APPLY_HIT:
                    ch->max_hit -= mod;
                    break;
                case APPLY_MOVE:
                    ch->max_move -= mod;
                    break;
                default:
                    break;
                }
            }
        }
        /* now reset the permanent stats */
        ch->pcdata->perm_hit = ch->max_hit;
        ch->pcdata->perm_mana = ch->max_mana;
        ch->pcdata->perm_move = ch->max_move;
        ch->pcdata->last_level = (int16_t)(ch->played / 3600);
        if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2) {
            if (ch->sex >= 0 && ch->sex < SEX_COUNT)
                ch->pcdata->true_sex = ch->sex;
            else
                ch->pcdata->true_sex = SEX_NEUTRAL;
        }
    }

    /* now restore the character to his/her true condition */
    for (stat = 0; stat < STAT_COUNT; stat++)
        ch->mod_stat[stat] = 0;

    if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
        ch->pcdata->true_sex = 0;
    ch->sex = ch->pcdata->true_sex;
    ch->max_hit = ch->pcdata->perm_hit;
    ch->max_mana = ch->pcdata->perm_mana;
    ch->max_move = ch->pcdata->perm_move;

    for (i = 0; i < 4; i++)
        ch->armor[i] = 100;

    ch->hitroll = 0;
    ch->damroll = 0;
    ch->saving_throw = 0;

    /* now start adding back the effects */
    for (loc = 0; loc < WEAR_LOC_COUNT; loc++) {
        obj = get_eq_char(ch, loc);
        if (obj == NULL)
            continue;
        for (i = 0; i < 4; i++)
            ch->armor[i] -= (int16_t)apply_ac(obj, loc, i);

        if (!obj->enchanted) {
            FOR_EACH(af, obj->prototype->affected) {
                mod = af->modifier;
                switch (af->location) {
                case APPLY_STR:
                    ch->mod_stat[STAT_STR] += mod;
                    break;
                case APPLY_DEX:
                    ch->mod_stat[STAT_DEX] += mod;
                    break;
                case APPLY_INT:
                    ch->mod_stat[STAT_INT] += mod;
                    break;
                case APPLY_WIS:
                    ch->mod_stat[STAT_WIS] += mod;
                    break;
                case APPLY_CON:
                    ch->mod_stat[STAT_CON] += mod;
                    break;
                case APPLY_SEX:
                    ch->sex += mod;
                    break;
                case APPLY_MANA:
                    ch->max_mana += mod;
                    break;
                case APPLY_HIT:
                    ch->max_hit += mod;
                    break;
                case APPLY_MOVE:
                    ch->max_move += mod;
                    break;
                case APPLY_AC:
                    for (i = 0; i < 4; i++) ch->armor[i] += mod;
                    break;
                case APPLY_HITROLL:
                    ch->hitroll += mod;
                    break;
                case APPLY_DAMROLL:
                    ch->damroll += mod;
                    break;
                case APPLY_SAVES:
                    ch->saving_throw += mod;
                    break;
                case APPLY_SAVING_ROD:
                    ch->saving_throw += mod;
                    break;
                case APPLY_SAVING_PETRI:
                    ch->saving_throw += mod;
                    break;
                case APPLY_SAVING_BREATH:
                    ch->saving_throw += mod;
                    break;
                case APPLY_SAVING_SPELL:
                    ch->saving_throw += mod;
                    break;
                default:
                    break;
                }
            }
        }

        FOR_EACH(af, obj->affected) {
            mod = af->modifier;
            switch (af->location) {
            case APPLY_STR:
                ch->mod_stat[STAT_STR] += mod;
                break;
            case APPLY_DEX:
                ch->mod_stat[STAT_DEX] += mod;
                break;
            case APPLY_INT:
                ch->mod_stat[STAT_INT] += mod;
                break;
            case APPLY_WIS:
                ch->mod_stat[STAT_WIS] += mod;
                break;
            case APPLY_CON:
                ch->mod_stat[STAT_CON] += mod;
                break;

            case APPLY_SEX:
                ch->sex += mod;
                break;
            case APPLY_MANA:
                ch->max_mana += mod;
                break;
            case APPLY_HIT:
                ch->max_hit += mod;
                break;
            case APPLY_MOVE:
                ch->max_move += mod;
                break;

            case APPLY_AC:
                for (i = 0; i < 4; i++) ch->armor[i] += mod;
                break;
            case APPLY_HITROLL:
                ch->hitroll += mod;
                break;
            case APPLY_DAMROLL:
                ch->damroll += mod;
                break;

            case APPLY_SAVES:
                ch->saving_throw += mod;
                break;
            case APPLY_SAVING_ROD:
                ch->saving_throw += mod;
                break;
            case APPLY_SAVING_PETRI:
                ch->saving_throw += mod;
                break;
            case APPLY_SAVING_BREATH:
                ch->saving_throw += mod;
                break;
            case APPLY_SAVING_SPELL:
                ch->saving_throw += mod;
                break;
            default:
                break;
            }
        }
    }

    /* now add back spell effects */
    FOR_EACH(af, ch->affected) {
        mod = af->modifier;
        switch (af->location) {
        case APPLY_STR:
            ch->mod_stat[STAT_STR] += mod;
            break;
        case APPLY_DEX:
            ch->mod_stat[STAT_DEX] += mod;
            break;
        case APPLY_INT:
            ch->mod_stat[STAT_INT] += mod;
            break;
        case APPLY_WIS:
            ch->mod_stat[STAT_WIS] += mod;
            break;
        case APPLY_CON:
            ch->mod_stat[STAT_CON] += mod;
            break;

        case APPLY_SEX:
            ch->sex += mod;
            break;
        case APPLY_MANA:
            ch->max_mana += mod;
            break;
        case APPLY_HIT:
            ch->max_hit += mod;
            break;
        case APPLY_MOVE:
            ch->max_move += mod;
            break;

        case APPLY_AC:
            for (i = 0; i < 4; i++) ch->armor[i] += mod;
            break;
        case APPLY_HITROLL:
            ch->hitroll += mod;
            break;
        case APPLY_DAMROLL:
            ch->damroll += mod;
            break;

        case APPLY_SAVES:
            ch->saving_throw += mod;
            break;
        case APPLY_SAVING_ROD:
            ch->saving_throw += mod;
            break;
        case APPLY_SAVING_PETRI:
            ch->saving_throw += mod;
            break;
        case APPLY_SAVING_BREATH:
            ch->saving_throw += mod;
            break;
        case APPLY_SAVING_SPELL:
            ch->saving_throw += mod;
            break;
        default:
            break;
        }
    }

    /* make sure sex is RIGHT!!!! */
    if (ch->sex < 0 || ch->sex >= SEX_COUNT) 
        ch->sex = ch->pcdata->true_sex;
}

// Retrieve a character's trusted level for permission checking.
LEVEL get_trust(Mobile* ch)
{
    if (ch->desc != NULL && ch->desc->original != NULL) ch = ch->desc->original;

    if (ch->trust) return ch->trust;

    if (IS_NPC(ch) && ch->level >= LEVEL_HERO)
        return LEVEL_HERO - 1;
    else
        return ch->level;
}

// Retrieve a character's age.
int get_age(Mobile* ch)
{
    return 17 + (int)(ch->played + (current_time - ch->logon)) / 72000;
}

/* command for retrieving stats */
int get_curr_stat(Mobile* ch, Stat stat)
{
    int max_score;
    int i = (int)stat;
    
    if (i < 0)
        i = 0;
    if (i >= STAT_COUNT)
        i = STAT_COUNT - 1;

    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
        max_score = STAT_MAX;

    else {
        max_score = race_table[ch->race].max_stats[i] + 4;

        if (class_table[ch->ch_class].prime_stat == stat)
            max_score += 2;

        if (ch->race == race_lookup("human")) 
            max_score += 1;

        max_score = UMIN(max_score, STAT_MAX);
    }

    return URANGE(3, ch->perm_stat[i] + ch->mod_stat[i], max_score);
}

/* command for returning max training score */
int get_max_train(Mobile* ch, Stat stat)
{
    int max;

    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
        return STAT_MAX;

    max = race_table[ch->race].max_stats[stat];

    if (class_table[ch->ch_class].prime_stat == stat) {
        if (ch->race == race_lookup("human"))
            max += 3;
        else
            max += 2;
    }

    return UMIN(max, STAT_MAX);
}

// Retrieve a character's carry capacity.
int can_carry_n(Mobile* ch)
{
    if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL)
        return 1000;

    if (IS_NPC(ch) && IS_SET(ch->act_flags, ACT_PET))
        return 0;

    return WEAR_LOC_COUNT + 2 * get_curr_stat(ch, STAT_DEX) + ch->level;
}

// Retrieve a character's carry capacity.
int can_carry_w(Mobile* ch)
{
    if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL)
        return 10000000;

    if (IS_NPC(ch) && IS_SET(ch->act_flags, ACT_PET))
        return 0;

    return str_mod[get_curr_stat(ch, STAT_STR)].carry * 10 + ch->level * 25;
}

// See if a string is one of the names of an object.

bool is_name(char* str, char* namelist)
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    char *list, *string;

    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0') 
        return false;

    /* fixed to prevent is_name on "" returning true */
    if (str[0] == '\0') 
        return false;

    string = str;
    /* we need ALL parts of string to match part of namelist */
    // start parsing string
    for (;;) {
        str = one_argument(str, part);

        if (part[0] == '\0')
            return true;

        /* check to see if this is part of namelist */
        list = namelist;
        // Start parsing namelist
        for (;;) {
            list = one_argument(list, name);
            if (name[0] == '\0') /* this name was not found */
                return false;

            if (!str_prefix(string, name)) 
                return true; /* full pattern match */

            if (!str_prefix(part, name)) 
                break;
        }
    }
}

bool is_exact_name(char* str, char* namelist)
{
    char name[MAX_INPUT_LENGTH];

    if (namelist == NULL) 
        return false;

    for (;;) {
        namelist = one_argument(namelist, name);

        if (name[0] == '\0') 
            return false;

        if (!str_cmp(str, name)) 
            return true;
    }
}

// Move a char out of a room.
void mob_from_room(Mobile* ch)
{
    Object* obj;

    if (ch->in_room == NULL) {
        bug("Char_from_room: NULL.", 0);
        return;
    }

    if (!IS_NPC(ch)) 
        --ch->in_room->area->nplayer;

    if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
        && obj->item_type == ITEM_LIGHT && obj->value[2] != 0
        && ch->in_room->light > 0)
        --ch->in_room->light;

    Node* node = list_find(&ch->in_room->mobiles, OBJ_VAL(ch));

    if (node == NULL)
        bug("Char_from_room: ch not found.", 0);
    else
        list_remove_node(&ch->in_room->mobiles, node);

    ch->in_room = NULL;
    ch->on = NULL; /* sanity check! */
    return;
}

static void update_mdsp_room(Mobile* ch)
{
    char exits[MAX_INPUT_LENGTH];
    RoomExit* room_exit;

    sprintf(exits, "%c", MSDP_ARRAY_OPEN);
    for (int door = 0; door < DIR_MAX; door++) {
        if ((room_exit = ch->in_room->exit[door]) != NULL
            && room_exit->to_room != NULL
            && (can_see_room(ch, room_exit->to_room->data)
                || (IS_AFFECTED(ch, AFF_INFRARED) 
                    && !IS_AFFECTED(ch, AFF_BLIND)))) {
            cat_sprintf(exits, "\001%s\002%d", dir_list[door].name_abbr, 
                VNUM_FIELD(room_exit->to_room));
        }
    }
    cat_sprintf(exits, "%c", MSDP_ARRAY_CLOSE);

    msdp_update_var_instant(ch->desc, "ROOM", 
        "%c\001%s\002%d\001%s\002%s\001%s\002%s\001%s\002%s\001%s\002%s%c",
        MSDP_TABLE_OPEN,
        "VNUM", VNUM_FIELD(ch->in_room),
        "NAME", NAME_STR(ch->in_room),
        "AREA", NAME_STR(ch->in_room->area),
        "TERRAIN", sector_flag_table[ch->in_room->data->sector_type].name,
        "EXITS", exits,
        MSDP_TABLE_CLOSE);
}

void transfer_mob(Mobile* ch, Room* room)
{
    mob_from_room(ch);
    mob_to_room(ch, room);
}

// Move a char into a room.
void mob_to_room(Mobile* ch, Room* room)
{
    Object* obj;

    if (room == NULL) {
        Room* temple;

        bug("Char_to_room: NULL.", 0);

        VNUM recall = IS_NPC(ch) ? cfg_get_default_recall() : ch->pcdata->recall;

        if ((temple = get_room(NULL, recall)) != NULL)
            mob_to_room(ch, temple);

        return;
    }

    ch->in_room = room;
    list_push_back(&room->mobiles, OBJ_VAL(ch));

    if (!IS_NPC(ch) && ch->desc->mth->msdp_data && cfg_get_msdp_enabled())
        update_mdsp_room(ch);

    if (!IS_NPC(ch)) {
        Area* area = ch->in_room->area;
        if (area->empty) {
            area->empty = false;
            if (!area->data->always_reset)
                area->reset_timer = 0;
        }
        ++area->nplayer;
    }

    if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
        && obj->item_type == ITEM_LIGHT && obj->value[2] != 0)
        ++ch->in_room->light;

    if (IS_AFFECTED(ch, AFF_PLAGUE)) {
        Affect* af;
        Affect plague = { 0 };
        Mobile* vch;

        FOR_EACH(af, ch->affected) {
            if (af->type == gsn_plague) break;
        }

        if (af == NULL) {
            REMOVE_BIT(ch->affect_flags, AFF_PLAGUE);
            return;
        }

        if (af->level == 1)
            return;

        plague.where = TO_AFFECTS;
        plague.type = gsn_plague;
        plague.level = af->level - 1;
        plague.duration = (int16_t)number_range(1, 2 * plague.level);
        plague.location = APPLY_STR;
        plague.modifier = -5;
        plague.bitvector = AFF_PLAGUE;

        FOR_EACH_ROOM_MOB(vch, ch->in_room) {
            if (!saves_spell(plague.level - 2, vch, DAM_DISEASE)
                && !IS_IMMORTAL(vch) && !IS_AFFECTED(vch, AFF_PLAGUE)
                && number_bits(6) == 0) {
                send_to_char("You feel hot and feverish.\n\r", vch);
                act("$n shivers and looks very ill.", vch, NULL, NULL, TO_ROOM);
                affect_join(vch, &plague);
            }
        }
    }

    return;
}

// Give an obj to a char.
void obj_to_char(Object* obj, Mobile* ch)
{
    list_push_back(&ch->objects, OBJ_VAL(obj));
    obj->carried_by = ch;
    obj->in_room = NULL;
    obj->in_obj = NULL;
    ch->carry_number += (int16_t)get_obj_number(obj);
    ch->carry_weight += (int16_t)get_obj_weight(obj);
}

// Take an obj from its character.
void obj_from_char(Object* obj)
{
    Mobile* ch;

    if ((ch = obj->carried_by) == NULL) {
        bug("Obj_from_char: null ch.", 0);
        return;
    }

    if (obj->wear_loc != WEAR_UNHELD) 
        unequip_char(ch, obj);

    Node* node = list_find(&obj->carried_by->objects, OBJ_VAL(obj));

    if (node == NULL)
        bug("Obj_from_char: obj not in list.", 0);
    else
        list_remove_node(&obj->carried_by->objects, node);

    obj->carried_by = NULL;
    ch->carry_number -= (int16_t)get_obj_number(obj);
    ch->carry_weight -= (int16_t)get_obj_weight(obj);
    return;
}

// Find the ac value of an obj, including position effect.
int apply_ac(Object* obj, int iWear, int type)
{
    if (obj->item_type != ITEM_ARMOR) 
        return 0;

    switch (iWear) {
    case WEAR_BODY:
        return 3 * obj->value[type];
    case WEAR_HEAD:
        return 2 * obj->value[type];
    case WEAR_LEGS:
        return 2 * obj->value[type];
    case WEAR_FEET:
        return obj->value[type];
    case WEAR_HANDS:
        return obj->value[type];
    case WEAR_ARMS:
        return obj->value[type];
    case WEAR_SHIELD:
        return obj->value[type];
    case WEAR_NECK_1:
        return obj->value[type];
    case WEAR_NECK_2:
        return obj->value[type];
    case WEAR_ABOUT:
        return 2 * obj->value[type];
    case WEAR_WAIST:
        return obj->value[type];
    case WEAR_WRIST_L:
        return obj->value[type];
    case WEAR_WRIST_R:
        return obj->value[type];
    case WEAR_HOLD:
        return obj->value[type];
    }

    return 0;
}

// Find a piece of eq on a character.
Object* get_eq_char(Mobile* ch, WearLocation iWear)
{
    Object* obj;

    if (ch == NULL) 
        return NULL;

    FOR_EACH_MOB_OBJ(obj, ch) {
        if (obj->wear_loc == iWear) 
            return obj;
    }

    return NULL;
}

// Equip a char with an obj.
void equip_char(Mobile* ch, Object* obj, WearLocation iWear)
{
    Affect* affect;
    int i;

    if (get_eq_char(ch, iWear) != NULL) {
        bug("Equip_char: already equipped (%d).", iWear);
        return;
    }

    if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
        || (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
        || (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))) {
        // Thanks to Morgenes for the bug fix here!
        act("You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR);
        act("$n is zapped by $p and drops it.", ch, obj, NULL, TO_ROOM);
        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
        return;
    }

    for (i = 0; i < 4; i++) ch->armor[i] -= (int16_t)apply_ac(obj, iWear, i);
    obj->wear_loc = iWear;

    if (!obj->enchanted)
        FOR_EACH(affect, obj->prototype->affected)
            if (affect->location != APPLY_SPELL_AFFECT)
                affect_modify(ch, affect, true);
    FOR_EACH(affect, obj->affected)
        if (affect->location == APPLY_SPELL_AFFECT)
            affect_to_mob(ch, affect);
        else
            affect_modify(ch, affect, true);

    if (obj->item_type == ITEM_LIGHT && obj->value[2] != 0
        && ch->in_room != NULL)
        ++ch->in_room->light;

    return;
}

// Unequip a char with an obj.
void unequip_char(Mobile* ch, Object* obj)
{
    Affect* affect = NULL;
    Affect* lpaf = NULL;
    Affect* lpaf_next = NULL;
    int i;

    if (obj->wear_loc == WEAR_UNHELD) {
        bug("Unequip_char: already unequipped.", 0);
        return;
    }

    for (i = 0; i < 4; i++) ch->armor[i] += (int16_t)apply_ac(obj, obj->wear_loc, i);
    obj->wear_loc = -1;

    if (!obj->enchanted) {
        FOR_EACH(affect, obj->prototype->affected) {
            if (affect->location == APPLY_SPELL_AFFECT) {
                for (lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next) {
                    lpaf_next = lpaf->next;
                    if ((lpaf->type == affect->type) && (lpaf->level == affect->level)
                        && (lpaf->location == APPLY_SPELL_AFFECT)) {
                        affect_remove(ch, lpaf);
                        lpaf_next = NULL;
                    }
                }
            }
            else {
                affect_modify(ch, affect, false);
                affect_check(ch, affect->where, affect->bitvector);
            }
        }
    }

    FOR_EACH(affect, obj->affected) {
        if (affect->location == APPLY_SPELL_AFFECT) {
            bug("Norm-Apply: %d", 0);
            for (lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next) {
                lpaf_next = lpaf->next;
                if ((lpaf->type == affect->type) && (lpaf->level == affect->level)
                    && (lpaf->location == APPLY_SPELL_AFFECT)) {
                    bug("location = %d", lpaf->location);
                    bug("type = %d", lpaf->type);
                    affect_remove(ch, lpaf);
                    lpaf_next = NULL;
                }
            }
        }
        else {
            affect_modify(ch, affect, false);
            affect_check(ch, affect->where, affect->bitvector);
        }
    }

    if (obj->item_type == ITEM_LIGHT && obj->value[2] != 0
        && ch->in_room != NULL && ch->in_room->light > 0)
        --ch->in_room->light;

    return;
}

// Count occurrences of an obj in a list.
int count_obj_list(ObjPrototype* obj_proto, List* list)
{
    Object* obj;
    int nMatch;

    nMatch = 0;
    FOR_EACH_LINK(obj, list, OBJECT) {
        if (obj->prototype == obj_proto)
            nMatch++;
    }

    return nMatch;
}

#define UNPARENT_OBJ(obj)                                                      \
{                                                                              \
    if ((obj)->in_room)                                                        \
        obj_from_room(obj);                                                    \
    else if ((obj)->in_obj)                                                    \
        obj_from_obj(obj);                                                     \
    else if ((obj)->carried_by)                                                \
        obj_from_char(obj);                                                    \
}

void transfer_obj(Object* obj, Room* room)
{
    UNPARENT_OBJ(obj);
    obj_to_room(obj, room);
}

// Move an obj out of a room.
void obj_from_room(Object* obj)
{
    Room* in_room;
    Mobile* ch;

    if (obj == NULL)
        return;

    if ((in_room = obj->in_room) == NULL) {
        bug("obj_from_room: NULL.", 0);
        return;
    }

    FOR_EACH_ROOM_MOB(ch, in_room)
        if (ch->on == obj)
            ch->on = NULL;

    Node* node = list_find(&in_room->objects, OBJ_VAL(obj));
    if (node == NULL)
        bug("Obj_from_room: obj not found.", 0);
    else
        list_remove_node(&in_room->objects, node);

    obj->in_room = NULL;
    return;
}

// Move an obj into a room.
void obj_to_room(Object* obj, Room* room)
{
    list_push_back(&room->objects, OBJ_VAL(obj));
    obj->in_room = room;
    obj->carried_by = NULL;
    obj->in_obj = NULL;
    return;
}

// Move an object into an object.
void obj_to_obj(Object* obj, Object* obj_to)
{
    list_push_back(&obj_to->objects, OBJ_VAL(obj));
    obj->in_obj = obj_to;
    obj->in_room = NULL;
    obj->carried_by = NULL;
    if (VNUM_FIELD(obj_to->prototype) == OBJ_VNUM_PIT)
        obj->cost = 0;

    for (; obj_to != NULL; obj_to = obj_to->in_obj) {
        if (obj_to->carried_by != NULL) {
            obj_to->carried_by->carry_number += (int16_t)get_obj_number(obj);
            obj_to->carried_by->carry_weight
                += (int16_t)get_obj_weight(obj) * (int16_t)(WEIGHT_MULT(obj_to) / 100);
        }
    }

    return;
}

// Move an object out of an object.
void obj_from_obj(Object* obj)
{
    Object* obj_from;

    if ((obj_from = obj->in_obj) == NULL) {
        bug("Obj_from_obj: null obj_from.", 0);
        return;
    }

    Node* node = list_find(&obj_from->objects, OBJ_VAL(obj));

    if (node == NULL)
        bug("Obj_from_obj: obj not found.", 0);
    else
        list_remove_node(&obj_from->objects, node);

    obj->in_obj = NULL;

    for (; obj_from != NULL; obj_from = obj_from->in_obj) {
        if (obj_from->carried_by != NULL) {
            obj_from->carried_by->carry_number -= (int16_t)get_obj_number(obj);
            obj_from->carried_by->carry_weight
                -= (int16_t)get_obj_weight(obj) * (int16_t)(WEIGHT_MULT(obj_from) / 100);
        }
    }

    return;
}

// Extract an obj from the world.
void extract_obj(Object* obj)
{
    UNPARENT_OBJ(obj);

    while (obj->objects.count > 0)
        extract_obj(AS_OBJECT(obj->objects.front->value));

    if (obj_list == obj) { 
        obj_list = obj->next; 
    }
    else {
        Object* prev;

        FOR_EACH(prev, obj_list) {
            if (prev->next == obj) {
                prev->next = obj->next;
                break;
            }
        }

        if (prev == NULL) {
            bug("Extract_obj: obj %d not found.", VNUM_FIELD(obj->prototype));
            return;
        }
    }

    --obj->prototype->count;
    free_object(obj);
    return;
}

// Extract a char from the world.
void extract_char(Mobile* ch, bool fPull)
{
    Mobile* wch;

    /* doesn't seem to be necessary
    if ( ch->in_room == NULL )
    {
        bug( "Extract_char: NULL.", 0 );
        return;
    }
    */

    if (ch == NULL)
        return;

    nuke_pets(ch);
    ch->pet = NULL; /* just in case */

    if (fPull) 
        die_follower(ch);

    stop_fighting(ch, true);

    while (ch->objects.count > 0)
        extract_obj(AS_OBJECT(ch->objects.front->value));

    if (ch->in_room != NULL)
        mob_from_room(ch);

    /* Death room is set in the clan tabe now */
    if (!fPull) {
        mob_to_room(ch, get_room(NULL, clan_table[ch->clan].hall));
        return;
    }

    if (IS_NPC(ch)) 
        --ch->prototype->count;

    if (ch->desc != NULL && ch->desc->original != NULL) {
        do_function(ch, &do_return, "");
        ch->desc = NULL;
    }

    FOR_EACH(wch, mob_list) {
        if (wch->reply == ch) 
            wch->reply = NULL;
        if (wch->mprog_target == ch)
            wch->mprog_target = NULL;
    }

    if (ch == mob_list) { 
        mob_list = ch->next; 
    }
    else {
        Mobile* prev;

        FOR_EACH(prev, mob_list) {
            if (prev->next == ch) {
                prev->next = ch->next;
                break;
            }
        }

        if (prev == NULL) {
            bug("Extract_char: char not found.", 0);
            return;
        }
    }

    if (ch->desc != NULL) 
        ch->desc->character = NULL;
    free_mobile(ch);
    return;
}

// Find a char in the room.
Mobile* get_mob_room(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* rch;
    int number;
    int count;

    number = number_argument(argument, arg);
    count = 0;
    if (!str_cmp(arg, "self")) return ch;

    FOR_EACH_ROOM_MOB(rch, ch->in_room) {
        if (!can_see(ch, rch) || !is_name(arg, NAME_STR(rch))) 
            continue;
        if (++count == number) 
            return rch;
    }

    return NULL;
}

// Find a char in the world.
Mobile* get_mob_world(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Mobile* wch;
    int number;
    int count;

    if ((wch = get_mob_room(ch, argument)) != NULL) 
        return wch;

    number = number_argument(argument, arg);
    count = 0;
    FOR_EACH(wch, mob_list) {
        if (wch->in_room == NULL || !can_see(ch, wch)
            || !is_name(arg, NAME_STR(wch)))
            continue;
        if (++count == number) 
            return wch;
    }

    return NULL;
}

/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 */
Object* get_obj_type(ObjPrototype* obj_proto)
{
    Object* obj;

    FOR_EACH(obj, obj_list) {
        if (obj->prototype == obj_proto)
            return obj;
    }

    return NULL;
}

// Find an obj in a list.
Object* get_obj_list(Mobile* ch, char* argument, List* list)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;
    int number;
    int count;

    number = number_argument(argument, arg);
    count = 0;
    FOR_EACH_LINK(obj, list, OBJECT) {
        if (can_see_obj(ch, obj) && is_name(arg, NAME_STR(obj))) {
            if (++count == number)
                return obj;
        }
    }

    return NULL;
}

// Find an obj in player's inventory.
Object* get_obj_carry(Mobile* ch, char* argument, Mobile* viewer)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;
    int number;
    int count;

    number = number_argument(argument, arg);
    count = 0;

    FOR_EACH_MOB_OBJ(obj, ch) {
        if (obj->wear_loc == WEAR_UNHELD && (can_see_obj(viewer, obj))
            && is_name(arg, NAME_STR(obj))) {
            if (++count == number) return obj;
        }
    }

    return NULL;
}

// Find an obj in player's equipment.
Object* get_obj_wear(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;
    int number;
    int count;

    number = number_argument(argument, arg);
    count = 0;
    FOR_EACH_MOB_OBJ(obj, ch) {
        if (obj->wear_loc != WEAR_UNHELD && can_see_obj(ch, obj)
            && is_name(arg, NAME_STR(obj))) {
            if (++count == number)
                return obj;
        }
    }

    return NULL;
}

// Find an obj in the room or in inventory.
Object* get_obj_here(Mobile* ch, char* argument)
{
    Object* obj;

    if ((obj = get_obj_list(ch, argument, &ch->in_room->objects)) != NULL)
        return obj;

    if ((obj = get_obj_carry(ch, argument, ch)) != NULL) 
        return obj;

    if ((obj = get_obj_wear(ch, argument)) != NULL) 
        return obj;

    return NULL;
}

// Find an obj in the world.
Object* get_obj_world(Mobile* ch, char* argument)
{
    char arg[MAX_INPUT_LENGTH];
    Object* obj;
    int number;
    int count;

    if ((obj = get_obj_here(ch, argument)) != NULL) return obj;

    number = number_argument(argument, arg);
    count = 0;
    FOR_EACH(obj, obj_list) {
        if (can_see_obj(ch, obj) && is_name(arg, NAME_STR(obj))) {
            if (++count == number) return obj;
        }
    }

    return NULL;
}

/* deduct cost from a character */

void deduct_cost(Mobile* ch, int cost)
{
    int16_t silver = 0; 
    int16_t gold = 0;

    silver = UMIN(ch->silver, (int16_t)cost);

    if (silver < cost) {
        gold = (int16_t)((cost - silver + 99) / 100);
        silver = (int16_t)(cost - 100 * gold);
    }

    ch->gold -= gold;
    ch->silver -= silver;

    if (ch->gold < 0) {
        bug("deduct costs: gold %d < 0", ch->gold);
        ch->gold = 0;
    }
    if (ch->silver < 0) {
        bug("deduct costs: silver %d < 0", ch->silver);
        ch->silver = 0;
    }
}
// Create a 'money' obj.
Object* create_money(int16_t gold, int16_t silver)
{
    char buf[MAX_STRING_LENGTH];
    Object* obj;

    if (gold < 0 || silver < 0 || (gold == 0 && silver == 0)) {
        bug("Create_money: zero or negative money.", UMIN(gold, silver));
        gold = UMAX(1, gold);
        silver = UMAX(1, silver);
    }

    if (gold == 0 && silver == 1) {
        obj = create_object(get_object_prototype(OBJ_VNUM_SILVER_ONE), 0);
    }
    else if (gold == 1 && silver == 0) {
        obj = create_object(get_object_prototype(OBJ_VNUM_GOLD_ONE), 0);
    }
    else if (silver == 0) {
        obj = create_object(get_object_prototype(OBJ_VNUM_GOLD_SOME), 0);
        sprintf(buf, "%d gold coins", gold);
        free_string(obj->short_descr);
        obj->short_descr = str_dup(buf);
        obj->value[1] = gold;
        obj->cost = gold;
        obj->weight = gold / 5;
    }
    else if (gold == 0) {
        obj = create_object(get_object_prototype(OBJ_VNUM_SILVER_SOME), 0);
        sprintf(buf, "%d silver coins", silver);
        free_string(obj->short_descr);
        obj->short_descr = str_dup(buf);
        obj->value[0] = silver;
        obj->cost = silver;
        obj->weight = silver / 20;
    }

    else {
        obj = create_object(get_object_prototype(OBJ_VNUM_COINS), 0);
        sprintf(buf, "%d gold coins and %d silver coins", gold, silver);
        free_string(obj->short_descr);
        obj->short_descr = str_dup(buf);
        obj->value[0] = silver;
        obj->value[1] = gold;
        obj->cost = 100 * gold + silver;
        obj->weight = gold / 5 + silver / 20;
    }

    return obj;
}

/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 */
int get_obj_number(Object* obj)
{
    int number;
    Object* content;

    if (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_MONEY
        || obj->item_type == ITEM_GEM || obj->item_type == ITEM_JEWELRY)
        number = 0;
    else
        number = 1;

    FOR_EACH_OBJ_CONTENT(content, obj)
        number += get_obj_number(content);

    return number;
}

// Return weight of an object, including weight of contents.
int get_obj_weight(Object* obj)
{
    int weight;
    Object* tobj;

    weight = obj->weight;
    FOR_EACH_OBJ_CONTENT(tobj, obj)
        weight += get_obj_weight(tobj) * WEIGHT_MULT(obj) / 100;

    return weight;
}

int get_true_weight(Object* obj)
{
    int weight;
    Object* content;

    weight = obj->weight;
    FOR_EACH_OBJ_CONTENT(content, obj)
        weight += get_obj_weight(content);

    return weight;
}

// true if room is dark.
bool room_is_dark(Room* room)
{
    if (room->light > 0) 
        return false;

    if (IS_SET(room->data->room_flags, ROOM_DARK))
        return true;

    if (room->data->sector_type == SECT_INSIDE
        || room->data->sector_type == SECT_CITY)
        return false;

    if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
        return true;

    return false;
}

bool is_room_owner(Mobile* ch, Room* room)
{
    if (room->data->owner == NULL || room->data->owner[0] == '\0')
        return false;

    return is_name(NAME_STR(ch), room->data->owner);
}

// true if room is private.
bool room_is_private(Room* room)
{
    Mobile* rch;
    int count;

    if (room->data->owner != NULL && room->data->owner[0] != '\0')
        return true;

    count = 0;
    FOR_EACH_ROOM_MOB(rch, room)
        count++;

    if (IS_SET(room->data->room_flags, ROOM_PRIVATE) && count >= 2)
        return true;

    if (IS_SET(room->data->room_flags, ROOM_SOLITARY) && count >= 1)
        return true;

    if (IS_SET(room->data->room_flags, ROOM_IMP_ONLY)) 
        return true;

    return false;
}

/* visibility on a room -- for entering and exits */
bool can_see_room(Mobile* ch, RoomData* room)
{
    if (IS_SET(room->room_flags, ROOM_IMP_ONLY)
        && get_trust(ch) < MAX_LEVEL)
        return false;

    if (IS_SET(room->room_flags, ROOM_GODS_ONLY) && !IS_IMMORTAL(ch))
        return false;

    if (IS_SET(room->room_flags, ROOM_HEROES_ONLY) && !IS_IMMORTAL(ch))
        return false;

    if (IS_SET(room->room_flags, ROOM_NEWBIES_ONLY) && ch->level > 5
        && !IS_IMMORTAL(ch))
        return false;

    if (!IS_IMMORTAL(ch) && room->clan && ch->clan != room->clan)
        return false;

    return true;
}

// true if char can see victim.
bool can_see(Mobile* ch, Mobile* victim)
{
    /* RT changed so that WIZ_INVIS has levels */
    if (ch == victim)
        return true;

    if (get_trust(ch) < victim->invis_level) 
        return false;

    if (get_trust(ch) < victim->incog_level && ch->in_room != victim->in_room)
        return false;

    if ((!IS_NPC(ch) && IS_SET(ch->act_flags, PLR_HOLYLIGHT))
        || (IS_NPC(ch) && IS_IMMORTAL(ch)))
        return true;

    if (IS_AFFECTED(ch, AFF_BLIND))
        return false;

    if (room_is_dark(ch->in_room) && !IS_AFFECTED(ch, AFF_INFRARED))
        return false;

    if (IS_AFFECTED(victim, AFF_INVISIBLE)
        && !IS_AFFECTED(ch, AFF_DETECT_INVIS))
        return false;

    /* sneaking */
    if (IS_AFFECTED(victim, AFF_SNEAK) && !IS_AFFECTED(ch, AFF_DETECT_HIDDEN)
        && victim->fighting == NULL) {
        int chance;
        chance = get_skill(victim, gsn_sneak);
        chance += get_curr_stat(victim, STAT_DEX) * 3 / 2;
        chance -= get_curr_stat(ch, STAT_INT) * 2;
        chance -= ch->level - victim->level * 3 / 2;

        if (number_percent() < chance)
            return false;
    }

    if (IS_AFFECTED(victim, AFF_HIDE) && !IS_AFFECTED(ch, AFF_DETECT_HIDDEN)
        && victim->fighting == NULL)
        return false;

    return true;
}

// true if char can see obj.
bool can_see_obj(Mobile* ch, Object* obj)
{
    if (!IS_NPC(ch) && IS_SET(ch->act_flags, PLR_HOLYLIGHT)) 
        return true;

    if (IS_SET(obj->extra_flags, ITEM_VIS_DEATH)) 
        return false;

    if (IS_AFFECTED(ch, AFF_BLIND) && obj->item_type != ITEM_POTION)
        return false;

    if (obj->item_type == ITEM_LIGHT && obj->value[2] != 0) 
        return true;

    if (IS_SET(obj->extra_flags, ITEM_INVIS)
        && !IS_AFFECTED(ch, AFF_DETECT_INVIS))
        return false;

    if (IS_OBJ_STAT(obj, ITEM_GLOW))
        return true;

    if (room_is_dark(ch->in_room) && !IS_AFFECTED(ch, AFF_DARK_VISION))
        return false;

    return true;
}

// true if char can drop obj.
bool can_drop_obj(Mobile* ch, Object* obj)
{
    if (!IS_SET(obj->extra_flags, ITEM_NODROP)) 
        return true;

    if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL) 
        return true;

    return false;
}

// Return ascii name of extra flags vector.
char* extra_bit_name(int extra_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (extra_flags & ITEM_GLOW) strcat(buf, " glow");
    if (extra_flags & ITEM_HUM) strcat(buf, " hum");
    if (extra_flags & ITEM_DARK) strcat(buf, " dark");
    if (extra_flags & ITEM_LOCK) strcat(buf, " lock");
    if (extra_flags & ITEM_EVIL) strcat(buf, " evil");
    if (extra_flags & ITEM_INVIS) strcat(buf, " invis");
    if (extra_flags & ITEM_MAGIC) strcat(buf, " magic");
    if (extra_flags & ITEM_NODROP) strcat(buf, " nodrop");
    if (extra_flags & ITEM_BLESS) strcat(buf, " bless");
    if (extra_flags & ITEM_ANTI_GOOD) strcat(buf, " anti-good");
    if (extra_flags & ITEM_ANTI_EVIL) strcat(buf, " anti-evil");
    if (extra_flags & ITEM_ANTI_NEUTRAL) strcat(buf, " anti-neutral");
    if (extra_flags & ITEM_NOREMOVE) strcat(buf, " noremove");
    if (extra_flags & ITEM_INVENTORY) strcat(buf, " inventory");
    if (extra_flags & ITEM_NOPURGE) strcat(buf, " nopurge");
    if (extra_flags & ITEM_VIS_DEATH) strcat(buf, " vis_death");
    if (extra_flags & ITEM_ROT_DEATH) strcat(buf, " rot_death");
    if (extra_flags & ITEM_NOLOCATE) strcat(buf, " no_locate");
    if (extra_flags & ITEM_SELL_EXTRACT) strcat(buf, " sell_extract");
    if (extra_flags & ITEM_BURN_PROOF) strcat(buf, " burn_proof");
    if (extra_flags & ITEM_NOUNCURSE) strcat(buf, " no_uncurse");
    return (buf[0] != '\0') ? buf + 1 : "none";
}

/* return ascii name of an act vector */
char* act_bit_name(int act_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (IS_SET(act_flags, ACT_IS_NPC)) {
        strcat(buf, " npc");
        if (act_flags & ACT_SENTINEL) strcat(buf, " sentinel");
        if (act_flags & ACT_SCAVENGER) strcat(buf, " scavenger");
        if (act_flags & ACT_AGGRESSIVE) strcat(buf, " aggressive");
        if (act_flags & ACT_STAY_AREA) strcat(buf, " stay_area");
        if (act_flags & ACT_WIMPY) strcat(buf, " wimpy");
        if (act_flags & ACT_PET) strcat(buf, " pet");
        if (act_flags & ACT_TRAIN) strcat(buf, " train");
        if (act_flags & ACT_PRACTICE) strcat(buf, " practice");
        if (act_flags & ACT_UNDEAD) strcat(buf, " undead");
        if (act_flags & ACT_CLERIC) strcat(buf, " cleric");
        if (act_flags & ACT_MAGE) strcat(buf, " mage");
        if (act_flags & ACT_THIEF) strcat(buf, " thief");
        if (act_flags & ACT_WARRIOR) strcat(buf, " warrior");
        if (act_flags & ACT_NOALIGN) strcat(buf, " no_align");
        if (act_flags & ACT_NOPURGE) strcat(buf, " no_purge");
        if (act_flags & ACT_IS_HEALER) strcat(buf, " healer");
        if (act_flags & ACT_IS_CHANGER) strcat(buf, " changer");
        if (act_flags & ACT_GAIN) strcat(buf, " skill_train");
        if (act_flags & ACT_UPDATE_ALWAYS) strcat(buf, " update_always");
    }
    else {
        strcat(buf, " player");
        if (act_flags & PLR_AUTOASSIST) strcat(buf, " autoassist");
        if (act_flags & PLR_AUTOEXIT) strcat(buf, " autoexit");
        if (act_flags & PLR_AUTOLOOT) strcat(buf, " autoloot");
        if (act_flags & PLR_AUTOSAC) strcat(buf, " autosac");
        if (act_flags & PLR_AUTOGOLD) strcat(buf, " autogold");
        if (act_flags & PLR_AUTOSPLIT) strcat(buf, " autosplit");
        if (act_flags & PLR_HOLYLIGHT) strcat(buf, " holy_light");
        if (act_flags & PLR_CANLOOT) strcat(buf, " loot_corpse");
        if (act_flags & PLR_NOSUMMON) strcat(buf, " no_summon");
        if (act_flags & PLR_NOFOLLOW) strcat(buf, " no_follow");
        if (act_flags & PLR_FREEZE) strcat(buf, " frozen");
        if (act_flags & PLR_THIEF) strcat(buf, " thief");
        if (act_flags & PLR_KILLER) strcat(buf, " killer");
    }
    return (buf[0] != '\0') ? buf + 1 : "none";
}

char* comm_bit_name(int comm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (comm_flags & COMM_QUIET) strcat(buf, " quiet");
    if (comm_flags & COMM_DEAF) strcat(buf, " deaf");
    if (comm_flags & COMM_NOWIZ) strcat(buf, " no_wiz");
    if (comm_flags & COMM_NOAUCTION) strcat(buf, " no_auction");
    if (comm_flags & COMM_NOGOSSIP) strcat(buf, " no_gossip");
    if (comm_flags & COMM_NOQUESTION) strcat(buf, " no_question");
    if (comm_flags & COMM_NOMUSIC) strcat(buf, " no_music");
    if (comm_flags & COMM_NOQUOTE) strcat(buf, " no_quote");
    if (comm_flags & COMM_COMPACT) strcat(buf, " compact");
    if (comm_flags & COMM_BRIEF) strcat(buf, " brief");
    if (comm_flags & COMM_PROMPT) strcat(buf, " prompt");
    if (comm_flags & COMM_COMBINE) strcat(buf, " combine");
    if (comm_flags & COMM_NOEMOTE) strcat(buf, " no_emote");
    if (comm_flags & COMM_NOSHOUT) strcat(buf, " no_shout");
    if (comm_flags & COMM_NOTELL) strcat(buf, " no_tell");
    if (comm_flags & COMM_NOCHANNELS) strcat(buf, " no_channels");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

char* imm_bit_name(int imm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (imm_flags & IMM_SUMMON) strcat(buf, " summon");
    if (imm_flags & IMM_CHARM) strcat(buf, " charm");
    if (imm_flags & IMM_MAGIC) strcat(buf, " magic");
    if (imm_flags & IMM_WEAPON) strcat(buf, " weapon");
    if (imm_flags & IMM_BASH) strcat(buf, " blunt");
    if (imm_flags & IMM_PIERCE) strcat(buf, " piercing");
    if (imm_flags & IMM_SLASH) strcat(buf, " slashing");
    if (imm_flags & IMM_FIRE) strcat(buf, " fire");
    if (imm_flags & IMM_COLD) strcat(buf, " cold");
    if (imm_flags & IMM_LIGHTNING) strcat(buf, " lightning");
    if (imm_flags & IMM_ACID) strcat(buf, " acid");
    if (imm_flags & IMM_POISON) strcat(buf, " poison");
    if (imm_flags & IMM_NEGATIVE) strcat(buf, " negative");
    if (imm_flags & IMM_HOLY) strcat(buf, " holy");
    if (imm_flags & IMM_ENERGY) strcat(buf, " energy");
    if (imm_flags & IMM_MENTAL) strcat(buf, " mental");
    if (imm_flags & IMM_DISEASE) strcat(buf, " disease");
    if (imm_flags & IMM_DROWNING) strcat(buf, " drowning");
    if (imm_flags & IMM_LIGHT) strcat(buf, " light");
    if (imm_flags & VULN_IRON) strcat(buf, " iron");
    if (imm_flags & VULN_WOOD) strcat(buf, " wood");
    if (imm_flags & VULN_SILVER) strcat(buf, " silver");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

char* wear_bit_name(int wear_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (wear_flags & ITEM_TAKE) strcat(buf, " take");
    if (wear_flags & ITEM_WEAR_FINGER) strcat(buf, " finger");
    if (wear_flags & ITEM_WEAR_NECK) strcat(buf, " neck");
    if (wear_flags & ITEM_WEAR_BODY) strcat(buf, " torso");
    if (wear_flags & ITEM_WEAR_HEAD) strcat(buf, " head");
    if (wear_flags & ITEM_WEAR_LEGS) strcat(buf, " legs");
    if (wear_flags & ITEM_WEAR_FEET) strcat(buf, " feet");
    if (wear_flags & ITEM_WEAR_HANDS) strcat(buf, " hands");
    if (wear_flags & ITEM_WEAR_ARMS) strcat(buf, " arms");
    if (wear_flags & ITEM_WEAR_SHIELD) strcat(buf, " shield");
    if (wear_flags & ITEM_WEAR_ABOUT) strcat(buf, " body");
    if (wear_flags & ITEM_WEAR_WAIST) strcat(buf, " waist");
    if (wear_flags & ITEM_WEAR_WRIST) strcat(buf, " wrist");
    if (wear_flags & ITEM_WIELD) strcat(buf, " wield");
    if (wear_flags & ITEM_HOLD) strcat(buf, " hold");
    if (wear_flags & ITEM_NO_SAC) strcat(buf, " nosac");
    if (wear_flags & ITEM_WEAR_FLOAT) strcat(buf, " float");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

char* form_bit_name(int form_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (form_flags & FORM_POISON)
        strcat(buf, " poison");
    else if (form_flags & FORM_EDIBLE)
        strcat(buf, " edible");
    if (form_flags & FORM_MAGICAL) strcat(buf, " magical");
    if (form_flags & FORM_INSTANT_DECAY) strcat(buf, " instant_rot");
    if (form_flags & FORM_OTHER) strcat(buf, " other");
    if (form_flags & FORM_ANIMAL) strcat(buf, " animal");
    if (form_flags & FORM_SENTIENT) strcat(buf, " sentient");
    if (form_flags & FORM_UNDEAD) strcat(buf, " undead");
    if (form_flags & FORM_CONSTRUCT) strcat(buf, " construct");
    if (form_flags & FORM_MIST) strcat(buf, " mist");
    if (form_flags & FORM_INTANGIBLE) strcat(buf, " intangible");
    if (form_flags & FORM_BIPED) strcat(buf, " biped");
    if (form_flags & FORM_CENTAUR) strcat(buf, " centaur");
    if (form_flags & FORM_INSECT) strcat(buf, " insect");
    if (form_flags & FORM_SPIDER) strcat(buf, " spider");
    if (form_flags & FORM_CRUSTACEAN) strcat(buf, " crustacean");
    if (form_flags & FORM_WORM) strcat(buf, " worm");
    if (form_flags & FORM_BLOB) strcat(buf, " blob");
    if (form_flags & FORM_MAMMAL) strcat(buf, " mammal");
    if (form_flags & FORM_BIRD) strcat(buf, " bird");
    if (form_flags & FORM_REPTILE) strcat(buf, " reptile");
    if (form_flags & FORM_SNAKE) strcat(buf, " snake");
    if (form_flags & FORM_DRAGON) strcat(buf, " dragon");
    if (form_flags & FORM_AMPHIBIAN) strcat(buf, " amphibian");
    if (form_flags & FORM_FISH) strcat(buf, " fish");
    if (form_flags & FORM_COLD_BLOOD) strcat(buf, " cold_blooded");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

char* part_bit_name(int part_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (part_flags & PART_HEAD) strcat(buf, " head");
    if (part_flags & PART_ARMS) strcat(buf, " arms");
    if (part_flags & PART_LEGS) strcat(buf, " legs");
    if (part_flags & PART_HEART) strcat(buf, " heart");
    if (part_flags & PART_BRAINS) strcat(buf, " brains");
    if (part_flags & PART_GUTS) strcat(buf, " guts");
    if (part_flags & PART_HANDS) strcat(buf, " hands");
    if (part_flags & PART_FEET) strcat(buf, " feet");
    if (part_flags & PART_FINGERS) strcat(buf, " fingers");
    if (part_flags & PART_EAR) strcat(buf, " ears");
    if (part_flags & PART_EYE) strcat(buf, " eyes");
    if (part_flags & PART_LONG_TONGUE) strcat(buf, " long_tongue");
    if (part_flags & PART_EYESTALKS) strcat(buf, " eyestalks");
    if (part_flags & PART_TENTACLES) strcat(buf, " tentacles");
    if (part_flags & PART_FINS) strcat(buf, " fins");
    if (part_flags & PART_WINGS) strcat(buf, " wings");
    if (part_flags & PART_TAIL) strcat(buf, " tail");
    if (part_flags & PART_CLAWS) strcat(buf, " claws");
    if (part_flags & PART_FANGS) strcat(buf, " fangs");
    if (part_flags & PART_HORNS) strcat(buf, " horns");
    if (part_flags & PART_SCALES) strcat(buf, " scales");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

char* weapon_bit_name(int weapon_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (weapon_flags & WEAPON_FLAMING) strcat(buf, " flaming");
    if (weapon_flags & WEAPON_FROST) strcat(buf, " frost");
    if (weapon_flags & WEAPON_VAMPIRIC) strcat(buf, " vampiric");
    if (weapon_flags & WEAPON_SHARP) strcat(buf, " sharp");
    if (weapon_flags & WEAPON_VORPAL) strcat(buf, " vorpal");
    if (weapon_flags & WEAPON_TWO_HANDS) strcat(buf, " two-handed");
    if (weapon_flags & WEAPON_SHOCKING) strcat(buf, " shocking");
    if (weapon_flags & WEAPON_POISON) strcat(buf, " poison");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

char* cont_bit_name(int cont_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (cont_flags & CONT_CLOSEABLE) strcat(buf, " closable");
    if (cont_flags & CONT_PICKPROOF) strcat(buf, " pickproof");
    if (cont_flags & CONT_CLOSED) strcat(buf, " closed");
    if (cont_flags & CONT_LOCKED) strcat(buf, " locked");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

char* off_bit_name(int atk_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (atk_flags & ATK_AREA_ATTACK) strcat(buf, " area attack");
    if (atk_flags & ATK_BACKSTAB) strcat(buf, " backstab");
    if (atk_flags & ATK_BASH) strcat(buf, " bash");
    if (atk_flags & ATK_BERSERK) strcat(buf, " berserk");
    if (atk_flags & ATK_DISARM) strcat(buf, " disarm");
    if (atk_flags & ATK_DODGE) strcat(buf, " dodge");
    if (atk_flags & ATK_FADE) strcat(buf, " fade");
    if (atk_flags & ATK_FAST) strcat(buf, " fast");
    if (atk_flags & ATK_KICK) strcat(buf, " kick");
    if (atk_flags & ATK_KICK_DIRT) strcat(buf, " kick_dirt");
    if (atk_flags & ATK_PARRY) strcat(buf, " parry");
    if (atk_flags & ATK_RESCUE) strcat(buf, " rescue");
    if (atk_flags & ATK_TAIL) strcat(buf, " tail");
    if (atk_flags & ATK_TRIP) strcat(buf, " trip");
    if (atk_flags & ATK_CRUSH) strcat(buf, " crush");
    if (atk_flags & ASSIST_ALL) strcat(buf, " assist_all");
    if (atk_flags & ASSIST_ALIGN) strcat(buf, " assist_align");
    if (atk_flags & ASSIST_RACE) strcat(buf, " assist_race");
    if (atk_flags & ASSIST_PLAYERS) strcat(buf, " assist_players");
    if (atk_flags & ASSIST_GUARD) strcat(buf, " assist_guard");
    if (atk_flags & ASSIST_VNUM) strcat(buf, " assist_vnum");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

// Config Colour stuff
void all_colour(Mobile* ch, char* argument)
{
    char buf[132];
    char buf2[100];
    uint8_t colour;
    uint8_t bright;

    if (IS_NPC(ch) || !ch->pcdata) return;

    if (!*argument) return;

    if (!str_prefix(argument, "red")) {
        colour = (RED);
        bright = NORMAL;
        sprintf(buf2, "Red");
    }
    if (!str_prefix(argument, "hi-red")) {
        colour = (RED);
        bright = BRIGHT;
        sprintf(buf2, "Red");
    }
    else if (!str_prefix(argument, "green")) {
        colour = (GREEN);
        bright = NORMAL;
        sprintf(buf2, "Green");
    }
    else if (!str_prefix(argument, "hi-green")) {
        colour = (GREEN);
        bright = BRIGHT;
        sprintf(buf2, "Green");
    }
    else if (!str_prefix(argument, "yellow")) {
        colour = (YELLOW);
        bright = NORMAL;
        sprintf(buf2, "Yellow");
    }
    else if (!str_prefix(argument, "hi-yellow")) {
        colour = (YELLOW);
        bright = BRIGHT;
        sprintf(buf2, "Yellow");
    }
    else if (!str_prefix(argument, "blue")) {
        colour = (BLUE);
        bright = NORMAL;
        sprintf(buf2, "Blue");
    }
    else if (!str_prefix(argument, "hi-blue")) {
        colour = (BLUE);
        bright = BRIGHT;
        sprintf(buf2, "Blue");
    }
    else if (!str_prefix(argument, "magenta")) {
        colour = (MAGENTA);
        bright = NORMAL;
        sprintf(buf2, "Magenta");
    }
    else if (!str_prefix(argument, "hi-magenta")) {
        colour = (MAGENTA);
        bright = BRIGHT;
        sprintf(buf2, "Magenta");
    }
    else if (!str_prefix(argument, "cyan")) {
        colour = (CYAN);
        bright = NORMAL;
        sprintf(buf2, "Cyan");
    }
    else if (!str_prefix(argument, "hi-cyan")) {
        colour = (CYAN);
        bright = BRIGHT;
        sprintf(buf2, "Cyan");
    }
    else if (!str_prefix(argument, "white")) {
        colour = (WHITE);
        bright = NORMAL;
        sprintf(buf2, "White");
    }
    else if (!str_prefix(argument, "hi-white")) {
        colour = (WHITE);
        bright = BRIGHT;
        sprintf(buf2, "White");
    }
    else if (!str_prefix(argument, "gray")) {
        colour = (BLACK);
        bright = BRIGHT;
        sprintf(buf2, "White");
    }
    else {
        send_to_char_bw("Unrecognised color, unchanged.\n\r", ch);
        return;
    }

    Color color;
    set_color_ansi(&color, bright, colour);

    ColorTheme* theme = new_color_theme();
    theme->name = str_dup(buf2);
    theme->banner = str_dup("");

    theme->palette[0] = color;
    theme->palette_max = 1;

    for (int i = 0; i < SLOT_MAX; ++i) {
        theme->channels[i] = color;
    }

    sprintf(buf, "All Color settings set to %s.\n\r", buf2);
    send_to_char(buf, ch);

    return;
}

bool emptystring(const char* str)
{
    int i = 0;

    for (; str[i]; i++)
        if (str[i] != ' ')
            return false;

    return true;
}

char* itos(int temp)
{
    static char buf[64];

    sprintf(buf, "%d", temp);

    return buf;
}

int get_vnum_mob_name_area(char* name, AreaData* area)
{
    int hash;
    MobPrototype* mob;

    for (hash = 0; hash < MAX_KEY_HASH; hash++)
        FOR_EACH(mob, mob_proto_hash[hash])
            if (mob->area == area
                && !str_prefix(name, NAME_STR(mob)))
                return VNUM_FIELD(mob);

    return 0;
}

int get_vnum_obj_name_area(char* name, AreaData* area)
{
    int hash;
    ObjPrototype* obj;

    for (hash = 0; hash < MAX_KEY_HASH; hash++)
        FOR_EACH(obj, obj_proto_hash[hash])
            if (obj->area == area
                && !str_prefix(name, NAME_STR(obj)))
                return VNUM_FIELD(obj);

    return 0;
}

int get_points(int race, int class_)
{
    int x;

    x = group_lookup(class_table[class_].default_group);

    if (x == -1) {
        bugf("get_points : skill group %s doesn't exist, race %d, class %d",
            class_table[class_].default_group, race, class_);
        return -1;
    }

    return GET_ELEM(&skill_group_table[x].rating, class_) + race_table[race].points;
}
