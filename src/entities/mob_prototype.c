////////////////////////////////////////////////////////////////////////////////
// mob_prototype.c
// Prototype data for mobile (NPC) Mobile
////////////////////////////////////////////////////////////////////////////////

#include "mob_prototype.h"

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "lookup.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"

#include "olc/olc.h"

#include "mobile.h"

#include "data/mobile_data.h"
#include "data/race.h"

MobPrototype* mob_proto_hash[MAX_KEY_HASH];
MobPrototype* mob_proto_free;

int mob_proto_count;
int mob_proto_perm_count;
VNUM top_vnum_mob;

MobPrototype* new_mob_prototype()
{
    LIST_ALLOC_PERM(mob_proto, MobPrototype);

    mob_proto->name = str_dup("no name");
    mob_proto->short_descr = str_dup("(no short description)");
    mob_proto->long_descr = str_dup("(no long description)\n\r");
    mob_proto->description = &str_empty[0];
    mob_proto->sex = SEX_NEUTRAL;
    mob_proto->act_flags = ACT_IS_NPC;
    mob_proto->race = (int16_t)race_lookup("human");
    mob_proto->material = str_dup("unknown");
    mob_proto->size = SIZE_MEDIUM;
    mob_proto->start_pos = POS_STANDING;
    mob_proto->default_pos = POS_STANDING;

    return mob_proto;
}

void free_mob_prototype(MobPrototype* mob_proto)
{
    free_string(mob_proto->name);
    free_string(mob_proto->short_descr);
    free_string(mob_proto->long_descr);
    free_string(mob_proto->description);
    free_mob_prog(mob_proto->mprogs);
    free_shop_data(mob_proto->pShop);

    LIST_FREE(mob_proto);
}

// Preserved for historical reasons and so that you can see where these values
// come from. -- Halivar
//void convert_mobile(MobPrototype* p_mob_proto)
//{
//    int i;
//    int type, number, bonus;
//    LEVEL level;
//
//    if (!p_mob_proto) return;
//
//    level = p_mob_proto->level;
//
//    p_mob_proto->act_flags |= ACT_WARRIOR;
//
//    /*
//     * Calculate hit dice.  Gives close to the hitpoints
//     * of old format mobs created with create_mobile()  (db.c)
//     * A high number of dice makes for less variance in mobiles
//     * hitpoints.
//     * (might be a good idea to reduce the max number of dice)
//     *
//     * The conversion below gives:
//
//   level:     dice         min         max        diff       mean
//     1:       1d2+6       7(  7)     8(   8)     1(   1)     8(   8)
//     2:       1d3+15     16( 15)    18(  18)     2(   3)    17(  17)
//     3:       1d6+24     25( 24)    30(  30)     5(   6)    27(  27)
//     5:      1d17+42     43( 42)    59(  59)    16(  17)    51(  51)
//    10:      3d22+96     99( 95)   162( 162)    63(  67)   131(    )
//    15:     5d30+161    166(159)   311( 311)   145( 150)   239(    )
//    30:    10d61+416    426(419)  1026(1026)   600( 607)   726(    )
//    50:    10d169+920   930(923)  2610(2610)  1680(1688)  1770(    )
//
//    The values in parenthesis give the values generated in create_mobile.
//        Diff = max - min.  Mean is the arithmetic mean.
//    (hmm.. must be some roundoff error in my calculations.. smurfette got
//     1d6+23 hp at level 3 ? -- anyway.. the values above should be
//     approximately right..)
//     */
//    type = level * level * 27 / 40;
//    number = UMIN(type / 40 + 1, 10); /* how do they get 11 ??? */
//    type = UMAX(2, type / number);
//    bonus = UMAX(0, ((int)(level * (8 + level) * .9) - number * type));
//
//    p_mob_proto->hit[DICE_NUMBER] = (int16_t)number;
//    p_mob_proto->hit[DICE_TYPE] = (int16_t)type;
//    p_mob_proto->hit[DICE_BONUS] = (int16_t)bonus;
//
//    p_mob_proto->mana[DICE_NUMBER] = (int16_t)level;
//    p_mob_proto->mana[DICE_TYPE] = 10;
//    p_mob_proto->mana[DICE_BONUS] = 100;
//
//    /*
//     * Calculate dam dice.  Gives close to the damage
//     * of old format mobs in damage()  (fight.c)
//     */
//    type = level * 7 / 4;
//    number = UMIN(type / 8 + 1, 5);
//    type = UMAX(2, type / number);
//    bonus = UMAX(0, level * 9 / 4 - number * type);
//
//    p_mob_proto->damage[DICE_NUMBER] = (int16_t)number;
//    p_mob_proto->damage[DICE_TYPE] = (int16_t)type;
//    p_mob_proto->damage[DICE_BONUS] = (int16_t)bonus;
//
//    switch (number_range(1, 3)) {
//    case (1): p_mob_proto->dam_type = 3;       break;  /* slash  */
//    case (2): p_mob_proto->dam_type = 7;       break;  /* pound  */
//    case (3): p_mob_proto->dam_type = 11;       break;  /* pierce */
//    }
//
//    for (i = 0; i < 3; i++)
//        p_mob_proto->ac[i] = (int16_t)interpolate(level, 100, -100);
//    p_mob_proto->ac[3] = (int16_t)interpolate(level, 100, 0);    /* exotic */
//
//    p_mob_proto->wealth /= 100;
//    p_mob_proto->size = SIZE_MEDIUM;
//    p_mob_proto->material = str_dup("none");
//
//    ++newmobs;
//
//    return;
//}

// Translates mob virtual number to its mob index struct.
// Hash table lookup.
MobPrototype* get_mob_prototype(VNUM vnum)
{
    MobPrototype* p_mob_proto;

    for (p_mob_proto = mob_proto_hash[vnum % MAX_KEY_HASH]; p_mob_proto != NULL;
        NEXT_LINK(p_mob_proto)) {
        if (p_mob_proto->vnum == vnum)
            return p_mob_proto;
    }

    if (fBootDb) {
        bug("Get_mob_prototype: bad vnum %"PRVNUM".", vnum);
        exit(1);
    }

    return NULL;
}

// Snarf a mob section.  new style
void load_mobiles(FILE* fp)
{
    MobPrototype* p_mob_proto;

    if (!area_data_last)   /* OLC */
    {
        bug("Load_mobiles: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        VNUM vnum;
        char letter;
        int hash;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_mobiles: # not found.", 0);
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0) break;

        fBootDb = false;
        if (get_mob_prototype(vnum) != NULL) {
            bug("Load_mobiles: vnum %"PRVNUM" duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        p_mob_proto = new_mob_prototype();

        p_mob_proto->vnum = vnum;
        p_mob_proto->area = area_data_last;
        p_mob_proto->name = fread_string(fp);
        p_mob_proto->short_descr = fread_string(fp);
        p_mob_proto->long_descr = fread_string(fp);
        p_mob_proto->description = fread_string(fp);
        p_mob_proto->race = (int16_t)race_lookup(fread_string(fp));

        p_mob_proto->long_descr[0] = UPPER(p_mob_proto->long_descr[0]);
        p_mob_proto->description[0] = UPPER(p_mob_proto->description[0]);

        p_mob_proto->act_flags
            = fread_flag(fp) | ACT_IS_NPC | race_table[p_mob_proto->race].act_flags;
        p_mob_proto->affect_flags
            = fread_flag(fp) | race_table[p_mob_proto->race].aff;
        p_mob_proto->pShop = NULL;
        p_mob_proto->alignment = (int16_t)fread_number(fp);
        p_mob_proto->group = (int16_t)fread_number(fp);

        p_mob_proto->level = (int16_t)fread_number(fp);
        p_mob_proto->hitroll = (int16_t)fread_number(fp);

        /* read hit dice */
        p_mob_proto->hit[DICE_NUMBER] = (int16_t)fread_number(fp);
        /* 'd'          */ fread_letter(fp);
        p_mob_proto->hit[DICE_TYPE] = (int16_t)fread_number(fp);
        /* '+'          */ fread_letter(fp);
        p_mob_proto->hit[DICE_BONUS] = (int16_t)fread_number(fp);

        /* read mana dice */
        p_mob_proto->mana[DICE_NUMBER] = (int16_t)fread_number(fp);
        fread_letter(fp);
        p_mob_proto->mana[DICE_TYPE] = (int16_t)fread_number(fp);
        fread_letter(fp);
        p_mob_proto->mana[DICE_BONUS] = (int16_t)fread_number(fp);

        /* read damage dice */
        p_mob_proto->damage[DICE_NUMBER] = (int16_t)fread_number(fp);
        fread_letter(fp);
        p_mob_proto->damage[DICE_TYPE] = (int16_t)fread_number(fp);
        fread_letter(fp);
        p_mob_proto->damage[DICE_BONUS] = (int16_t)fread_number(fp);
        p_mob_proto->dam_type = (int16_t)attack_lookup(fread_word(fp));

        /* read armor class */
        p_mob_proto->ac[AC_PIERCE] = (int16_t)fread_number(fp) * 10;
        p_mob_proto->ac[AC_BASH] = (int16_t)fread_number(fp) * 10;
        p_mob_proto->ac[AC_SLASH] = (int16_t)fread_number(fp) * 10;
        p_mob_proto->ac[AC_EXOTIC] = (int16_t)fread_number(fp) * 10;

        /* read flags and add in data from the race table */
        p_mob_proto->atk_flags = fread_flag(fp) | race_table[p_mob_proto->race].off;
        p_mob_proto->imm_flags = fread_flag(fp) | race_table[p_mob_proto->race].imm;
        p_mob_proto->res_flags = fread_flag(fp) | race_table[p_mob_proto->race].res;
        p_mob_proto->vuln_flags
            = fread_flag(fp) | race_table[p_mob_proto->race].vuln;

        /* vital statistics */
        p_mob_proto->start_pos = (int16_t)position_lookup(fread_word(fp));
        p_mob_proto->default_pos = (int16_t)position_lookup(fread_word(fp));
        p_mob_proto->sex = (Sex)sex_lookup(fread_word(fp));

        p_mob_proto->wealth = fread_number(fp);

        p_mob_proto->form = fread_flag(fp) | race_table[p_mob_proto->race].form;
        p_mob_proto->parts = fread_flag(fp) | race_table[p_mob_proto->race].parts;
        /* size */
        CHECK_POS(p_mob_proto->size, (int16_t)size_lookup(fread_word(fp)), "size");
        /*	p_mob_proto->size			= size_lookup(fread_word(fp)); */
        p_mob_proto->material = str_dup(fread_word(fp));

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'F') {
                char* word;
                long vector;

                word = fread_word(fp);
                vector = fread_flag(fp);

                if (!str_prefix(word, "act"))
                    REMOVE_BIT(p_mob_proto->act_flags, vector);
                else if (!str_prefix(word, "aff"))
                    REMOVE_BIT(p_mob_proto->affect_flags, vector);
                else if (!str_prefix(word, "off"))
                    REMOVE_BIT(p_mob_proto->atk_flags, vector);
                else if (!str_prefix(word, "imm"))
                    REMOVE_BIT(p_mob_proto->imm_flags, vector);
                else if (!str_prefix(word, "res"))
                    REMOVE_BIT(p_mob_proto->res_flags, vector);
                else if (!str_prefix(word, "vul"))
                    REMOVE_BIT(p_mob_proto->vuln_flags, vector);
                else if (!str_prefix(word, "for"))
                    REMOVE_BIT(p_mob_proto->form, vector);
                else if (!str_prefix(word, "par"))
                    REMOVE_BIT(p_mob_proto->parts, vector);
                else {
                    bug("Flag remove: flag not found.", 0);
                    exit(1);
                }
            }
            else if (letter == 'M') {
                MobProg* pMprog;
                char* word;
                int trigger = 0;

                pMprog = new_mob_prog();
                word = fread_word(fp);
                if ((trigger = flag_lookup(word, mprog_flag_table)) == NO_FLAG) {
                    bug("MOBprogs: invalid trigger.", 0);
                    exit(1);
                }
                SET_BIT(p_mob_proto->mprog_flags, trigger);
                pMprog->trig_type = trigger;
                pMprog->vnum = fread_number(fp);
                pMprog->trig_phrase = fread_string(fp);
                pMprog->next = p_mob_proto->mprogs;
                p_mob_proto->mprogs = pMprog;
            }
            else {
                ungetc(letter, fp);
                break;
            }
        }

        hash = vnum % MAX_KEY_HASH;
        p_mob_proto->next = mob_proto_hash[hash];
        mob_proto_hash[hash] = p_mob_proto;
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;
        assign_area_vnum(vnum);
        kill_table[URANGE(0, p_mob_proto->level, MAX_LEVEL - 1)].number++;
    }

    return;
}

void recalc(MobPrototype* pMob)
{
    int hplev, aclev, damlev, hitbonus;
    //int i, cnt = 0;
    //int clase[10] = { 0 };
    float n;

    if (pMob->level == 0)
        return;

    hplev = 0; aclev = 0; damlev = 0; hitbonus = 0;

    if (IS_SET(pMob->act_flags, ACT_WARRIOR)) {
        hplev += 1;
        //clase[cnt++] = ACT_WARRIOR;
    }

    if (IS_SET(pMob->act_flags, ACT_THIEF)) {
        hplev -= 1; aclev -= 1; damlev -= 1;
        //clase[cnt++] = ACT_THIEF;
    }

    if (IS_SET(pMob->act_flags, ACT_CLERIC)) {
        damlev -= 2;
        //clase[cnt++] = ACT_CLERIC;
    }

    if (IS_SET(pMob->act_flags, ACT_MAGE)) {
//		hplev -= 1; aclev -= 1; damlev -= 3;
        hplev -= 2; aclev -= 1; damlev -= 3;
        //clase[cnt++] = ACT_MAGE;
    }

    hplev += pMob->level;
    aclev += pMob->level;
    damlev += pMob->level;

    hplev = URANGE(1, hplev, 60) - 1;
    aclev = URANGE(1, aclev, 60) - 1;
    damlev = URANGE(1, damlev, 60) - 1;

    pMob->hit[DICE_NUMBER] = recval_table[hplev].numhit;
    pMob->hit[DICE_TYPE] = recval_table[hplev].typhit;
    pMob->hit[DICE_BONUS] = recval_table[hplev].bonhit;

    pMob->damage[DICE_NUMBER] = recval_table[damlev].numdam;
    pMob->damage[DICE_TYPE] = recval_table[damlev].typdam;
    pMob->damage[DICE_BONUS] = recval_table[damlev].bondam;

    pMob->mana[DICE_NUMBER] = pMob->level;
    pMob->mana[DICE_TYPE] = 10 + (pMob->level / 8);
    pMob->mana[DICE_BONUS] = 100;

    if (IS_SET(pMob->act_flags, ACT_CLERIC) || IS_SET(pMob->act_flags, ACT_MAGE))
        pMob->mana[DICE_BONUS] *= (1 + pMob->level / 3);

    for (int i = 0; i < 3; i++)
        pMob->ac[i] = recval_table[aclev].ac * 10;

    if (IS_SET(pMob->act_flags, ACT_UNDEAD)
        || IS_SET(pMob->form, FORM_UNDEAD)
        || IS_SET(pMob->form, FORM_MAGICAL))
        n = 0;
    else if (IS_SET(pMob->act_flags, ACT_MAGE))
        n = 1;
    else if (IS_SET(pMob->act_flags, ACT_THIEF)
        || IS_SET(pMob->act_flags, ACT_CLERIC))
        n = 2;
    else
        n = 3;

    aclev = UMAX(0, (int)(aclev - n));

    pMob->ac[3] = recval_table[aclev].ac * 10;

    if (IS_SET(pMob->act_flags, ACT_WARRIOR))
        hitbonus = pMob->level * 3 / 2;
    else if (IS_SET(pMob->act_flags, ACT_THIEF))
        hitbonus = pMob->level * 2 / 3;
    else if (IS_SET(pMob->act_flags, ACT_CLERIC) || IS_SET(pMob->act_flags, ACT_MAGE))
        hitbonus = pMob->level / 2;

    pMob->hitroll = (int16_t)hitbonus;

    return;
}
