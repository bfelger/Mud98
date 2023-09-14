////////////////////////////////////////////////////////////////////////////////
// mob_prototype.c
// Prototype data for mobile (NPC) CharData
////////////////////////////////////////////////////////////////////////////////

#include "mob_prototype.h"

#include "db.h"
#include "handler.h"
#include "lookup.h"
#include "magic.h"
#include "mem.h"
#include "olc.h"
#include "recycle.h"
#include "tables.h"

#include "char_data.h"

#include "data/mobile.h"

MobPrototype* mob_prototype_hash[MAX_KEY_HASH];
MobPrototype* mob_prototype_free;

int last_mob_id = 0;
int newmobs = 0;
int top_mob_prototype;
VNUM top_vnum_mob;      // OLC

/*****************************************************************************
 Name:		convert_mobile
 Purpose:	Converts an old_format mob into new_format
 Called by:	load_old_mob (db.c).
 Note:          Dug out of create_mobile (db.c)
 Author:        Hugin
 ****************************************************************************/
void convert_mobile(MobPrototype* p_mob_proto)
{
    int i;
    int type, number, bonus;
    LEVEL level;

    if (!p_mob_proto || p_mob_proto->new_format) return;

    level = p_mob_proto->level;

    p_mob_proto->act_flags |= ACT_WARRIOR;

    /*
     * Calculate hit dice.  Gives close to the hitpoints
     * of old format mobs created with create_mobile()  (db.c)
     * A high number of dice makes for less variance in mobiles
     * hitpoints.
     * (might be a good idea to reduce the max number of dice)
     *
     * The conversion below gives:

   level:     dice         min         max        diff       mean
     1:       1d2+6       7(  7)     8(   8)     1(   1)     8(   8)
     2:       1d3+15     16( 15)    18(  18)     2(   3)    17(  17)
     3:       1d6+24     25( 24)    30(  30)     5(   6)    27(  27)
     5:      1d17+42     43( 42)    59(  59)    16(  17)    51(  51)
    10:      3d22+96     99( 95)   162( 162)    63(  67)   131(    )
    15:     5d30+161    166(159)   311( 311)   145( 150)   239(    )
    30:    10d61+416    426(419)  1026(1026)   600( 607)   726(    )
    50:    10d169+920   930(923)  2610(2610)  1680(1688)  1770(    )

    The values in parenthesis give the values generated in create_mobile.
        Diff = max - min.  Mean is the arithmetic mean.
    (hmm.. must be some roundoff error in my calculations.. smurfette got
     1d6+23 hp at level 3 ? -- anyway.. the values above should be
     approximately right..)
     */
    type = level * level * 27 / 40;
    number = UMIN(type / 40 + 1, 10); /* how do they get 11 ??? */
    type = UMAX(2, type / number);
    bonus = UMAX(0, ((int)(level * (8 + level) * .9) - number * type));

    p_mob_proto->hit[DICE_NUMBER] = (int16_t)number;
    p_mob_proto->hit[DICE_TYPE] = (int16_t)type;
    p_mob_proto->hit[DICE_BONUS] = (int16_t)bonus;

    p_mob_proto->mana[DICE_NUMBER] = (int16_t)level;
    p_mob_proto->mana[DICE_TYPE] = 10;
    p_mob_proto->mana[DICE_BONUS] = 100;

    /*
     * Calculate dam dice.  Gives close to the damage
     * of old format mobs in damage()  (fight.c)
     */
    type = level * 7 / 4;
    number = UMIN(type / 8 + 1, 5);
    type = UMAX(2, type / number);
    bonus = UMAX(0, level * 9 / 4 - number * type);

    p_mob_proto->damage[DICE_NUMBER] = (int16_t)number;
    p_mob_proto->damage[DICE_TYPE] = (int16_t)type;
    p_mob_proto->damage[DICE_BONUS] = (int16_t)bonus;

    switch (number_range(1, 3)) {
    case (1): p_mob_proto->dam_type = 3;       break;  /* slash  */
    case (2): p_mob_proto->dam_type = 7;       break;  /* pound  */
    case (3): p_mob_proto->dam_type = 11;       break;  /* pierce */
    }

    for (i = 0; i < 3; i++)
        p_mob_proto->ac[i] = (int16_t)interpolate(level, 100, -100);
    p_mob_proto->ac[3] = (int16_t)interpolate(level, 100, 0);    /* exotic */

    p_mob_proto->wealth /= 100;
    p_mob_proto->size = SIZE_MEDIUM;
    p_mob_proto->material = str_dup("none");

    p_mob_proto->new_format = true;
    ++newmobs;

    return;
}

CharData* create_mobile(MobPrototype* p_mob_proto)
{
    CharData* mob;
    int i;
    AffectData af = { 0 };

    mobile_count++;

    if (p_mob_proto == NULL) {
        bug("Create_mobile: NULL p_mob_proto.", 0);
        exit(1);
    }

    mob = new_char_data();

    mob->prototype = p_mob_proto;

    mob->name = str_dup(p_mob_proto->name);    /* OLC */
    mob->short_descr = str_dup(p_mob_proto->short_descr);    /* OLC */
    mob->long_descr = str_dup(p_mob_proto->long_descr);     /* OLC */
    mob->description = str_dup(p_mob_proto->description);    /* OLC */
    mob->id = get_mob_id();
    mob->spec_fun = p_mob_proto->spec_fun;
    mob->prompt = NULL;
    mob->mprog_target = NULL;

    if (p_mob_proto->wealth == 0) {
        mob->silver = 0;
        mob->gold = 0;
    }
    else {
        int16_t wealth = (int16_t)number_range(p_mob_proto->wealth / 2,
            3 * p_mob_proto->wealth / 2);
        mob->gold = (int16_t)number_range(wealth / 200, wealth / 100);
        mob->silver = wealth - (mob->gold * 100);
    }

    if (p_mob_proto->new_format)
    /* load in new style */
    {
        /* read from prototype */
        mob->group = p_mob_proto->group;
        mob->act_flags = p_mob_proto->act_flags;
        mob->comm = COMM_NOCHANNELS | COMM_NOSHOUT | COMM_NOTELL;
        mob->affect_flags = p_mob_proto->affect_flags;
        mob->alignment = p_mob_proto->alignment;
        mob->level = p_mob_proto->level;
        mob->hitroll = p_mob_proto->hitroll;
        mob->damroll = p_mob_proto->damage[DICE_BONUS];
        mob->max_hit = (int16_t)dice(p_mob_proto->hit[DICE_NUMBER], 
            p_mob_proto->hit[DICE_TYPE]) + p_mob_proto->hit[DICE_BONUS];
        mob->hit = mob->max_hit;
        mob->max_mana = (int16_t)dice(p_mob_proto->mana[DICE_NUMBER], 
            p_mob_proto->mana[DICE_TYPE]) + p_mob_proto->mana[DICE_BONUS];
        mob->mana = mob->max_mana;
        mob->damage[DICE_NUMBER] = p_mob_proto->damage[DICE_NUMBER];
        mob->damage[DICE_TYPE] = p_mob_proto->damage[DICE_TYPE];
        mob->dam_type = p_mob_proto->dam_type;
        if (mob->dam_type == 0) {
            switch (number_range(1, 3)) {
            case (1):
                mob->dam_type = 3;
                break; /* slash */
            case (2):
                mob->dam_type = 7;
                break; /* pound */
            case (3):
                mob->dam_type = 11;
                break; /* pierce */
            }
        }
        for (i = 0; i < 4; i++) 
            mob->armor[i] = p_mob_proto->ac[i];
        mob->atk_flags = p_mob_proto->atk_flags;
        mob->imm_flags = p_mob_proto->imm_flags;
        mob->res_flags = p_mob_proto->res_flags;
        mob->vuln_flags = p_mob_proto->vuln_flags;
        mob->start_pos = p_mob_proto->start_pos;
        mob->default_pos = p_mob_proto->default_pos;
        mob->sex = p_mob_proto->sex;
        if (mob->sex == SEX_EITHER) /* random sex */
            mob->sex = (Sex)number_range(1, 2);
        mob->race = p_mob_proto->race;
        mob->form = p_mob_proto->form;
        mob->parts = p_mob_proto->parts;
        mob->size = p_mob_proto->size;
        mob->material = str_dup(p_mob_proto->material);

        /* computed on the spot */

        for (i = 0; i < MAX_STATS; i++)
            mob->perm_stat[i] = UMIN(25, 11 + mob->level / 4);

        if (IS_SET(mob->act_flags, ACT_WARRIOR)) {
            mob->perm_stat[STAT_STR] += 3;
            mob->perm_stat[STAT_INT] -= 1;
            mob->perm_stat[STAT_CON] += 2;
        }

        if (IS_SET(mob->act_flags, ACT_THIEF)) {
            mob->perm_stat[STAT_DEX] += 3;
            mob->perm_stat[STAT_INT] += 1;
            mob->perm_stat[STAT_WIS] -= 1;
        }

        if (IS_SET(mob->act_flags, ACT_CLERIC)) {
            mob->perm_stat[STAT_WIS] += 3;
            mob->perm_stat[STAT_DEX] -= 1;
            mob->perm_stat[STAT_STR] += 1;
        }

        if (IS_SET(mob->act_flags, ACT_MAGE)) {
            mob->perm_stat[STAT_INT] += 3;
            mob->perm_stat[STAT_STR] -= 1;
            mob->perm_stat[STAT_DEX] += 1;
        }

        if (IS_SET(mob->atk_flags, ATK_FAST))
            mob->perm_stat[STAT_DEX] += 2;

        mob->perm_stat[STAT_STR] += (int16_t)(mob->size - SIZE_MEDIUM);
        mob->perm_stat[STAT_CON] += (int16_t)(mob->size - SIZE_MEDIUM) / 2;

        /* let's get some spell action */
        if (IS_AFFECTED(mob, AFF_SANCTUARY)) {
            af.where = TO_AFFECTS;
            af.type = skill_lookup("sanctuary");
            af.level = mob->level;
            af.duration = -1;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = AFF_SANCTUARY;
            affect_to_char(mob, &af);
        }

        if (IS_AFFECTED(mob, AFF_HASTE)) {
            af.where = TO_AFFECTS;
            af.type = skill_lookup("haste");
            af.level = mob->level;
            af.duration = -1;
            af.location = APPLY_DEX;
            af.modifier = 1 + (mob->level >= 18) + (mob->level >= 25)
                + (mob->level >= 32);
            af.bitvector = AFF_HASTE;
            affect_to_char(mob, &af);
        }

        if (IS_AFFECTED(mob, AFF_PROTECT_EVIL)) {
            af.where = TO_AFFECTS;
            af.type = skill_lookup("protection evil");
            af.level = mob->level;
            af.duration = -1;
            af.location = APPLY_SAVES;
            af.modifier = -1;
            af.bitvector = AFF_PROTECT_EVIL;
            affect_to_char(mob, &af);
        }

        if (IS_AFFECTED(mob, AFF_PROTECT_GOOD)) {
            af.where = TO_AFFECTS;
            af.type = skill_lookup("protection good");
            af.level = mob->level;
            af.duration = -1;
            af.location = APPLY_SAVES;
            af.modifier = -1;
            af.bitvector = AFF_PROTECT_GOOD;
            affect_to_char(mob, &af);
        }
    }
    else /* read in old format and convert */
    {
        mob->act_flags = p_mob_proto->act_flags;
        mob->affect_flags = p_mob_proto->affect_flags;
        mob->alignment = p_mob_proto->alignment;
        mob->level = p_mob_proto->level;
        mob->hitroll = p_mob_proto->hitroll;
        mob->damroll = 0;
        mob->max_hit = mob->level * 8
            + (int16_t)number_range(mob->level * mob->level / 4,
                mob->level * mob->level);
        mob->max_hit = (int16_t)((double)mob->max_hit * 0.9);
        mob->hit = mob->max_hit;
        mob->max_mana = 100 + (int16_t)dice(mob->level, 10);
        mob->mana = mob->max_mana;
        switch (number_range(1, 3)) {
        case (1):
            mob->dam_type = 3;
            break; /* slash */
        case (2):
            mob->dam_type = 7;
            break; /* pound */
        case (3):
            mob->dam_type = 11;
            break; /* pierce */
        }
        for (i = 0; i < 3; i++)
            mob->armor[i] = (int16_t)interpolate(mob->level, 100, -100);
        mob->armor[3] = (int16_t)interpolate(mob->level, 100, 0);
        mob->race = p_mob_proto->race;
        mob->atk_flags = p_mob_proto->atk_flags;
        mob->imm_flags = p_mob_proto->imm_flags;
        mob->res_flags = p_mob_proto->res_flags;
        mob->vuln_flags = p_mob_proto->vuln_flags;
        mob->start_pos = p_mob_proto->start_pos;
        mob->default_pos = p_mob_proto->default_pos;
        mob->sex = p_mob_proto->sex;
        mob->form = p_mob_proto->form;
        mob->parts = p_mob_proto->parts;
        mob->size = SIZE_MEDIUM;
        mob->material = "";

        for (i = 0; i < MAX_STATS; i++) mob->perm_stat[i] = 11 + mob->level / 4;
    }

    mob->position = mob->start_pos;

    /* link the mob to the world list */
    mob->next = char_list;
    char_list = mob;
    p_mob_proto->count++;
    return mob;
}

void free_mob_prototype(MobPrototype* p_mob_proto)
{
    free_string(p_mob_proto->name);
    free_string(p_mob_proto->short_descr);
    free_string(p_mob_proto->long_descr);
    free_string(p_mob_proto->description);
    free_mprog(p_mob_proto->mprogs);

    free_shop(p_mob_proto->pShop);

    p_mob_proto->next = mob_prototype_free;
    mob_prototype_free = p_mob_proto;
    return;
}

long get_mob_id()
{
    last_mob_id++;
    return last_mob_id;
}

// Translates mob virtual number to its mob index struct.
// Hash table lookup.
MobPrototype* get_mob_prototype(VNUM vnum)
{
    MobPrototype* p_mob_proto;

    for (p_mob_proto = mob_prototype_hash[vnum % MAX_KEY_HASH]; p_mob_proto != NULL;
        p_mob_proto = p_mob_proto->next) {
        if (p_mob_proto->vnum == vnum) return p_mob_proto;
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

    if (!area_last)   /* OLC */
    {
        bug("Load_mobiles: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        VNUM vnum;
        char letter;
        int iHash;

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

        p_mob_proto = alloc_perm(sizeof(*p_mob_proto));
        p_mob_proto->vnum = vnum;
        p_mob_proto->area = area_last;    // OLC
        p_mob_proto->new_format = true;
        newmobs++;
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
                MPROG_LIST* pMprog;
                char* word;
                int trigger = 0;

                pMprog = alloc_perm(sizeof(*pMprog));
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

        iHash = vnum % MAX_KEY_HASH;
        p_mob_proto->next = mob_prototype_hash[iHash];
        mob_prototype_hash[iHash] = p_mob_proto;
        top_mob_prototype++;
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;   // OLC
        assign_area_vnum(vnum);                                     // OLC
        kill_table[URANGE(0, p_mob_proto->level, MAX_LEVEL - 1)].number++;
    }

    return;
}

// Snarf a mob section. old style
void load_old_mob(FILE* fp)
{
    MobPrototype* p_mob_proto;
    /* for race updating */
    int race;
    char name[MAX_STRING_LENGTH];

    if (!area_last) {
        bug("Load_mobiles: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        VNUM vnum;
        char letter;
        int iHash;

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

        p_mob_proto = alloc_perm(sizeof(*p_mob_proto));
        p_mob_proto->vnum = vnum;
        p_mob_proto->area = area_last;    // OLC
        p_mob_proto->new_format = false;
        p_mob_proto->name = fread_string(fp);
        p_mob_proto->short_descr = fread_string(fp);
        p_mob_proto->long_descr = fread_string(fp);
        p_mob_proto->description = fread_string(fp);

        p_mob_proto->long_descr[0] = UPPER(p_mob_proto->long_descr[0]);
        p_mob_proto->description[0] = UPPER(p_mob_proto->description[0]);

        p_mob_proto->act_flags = fread_flag(fp) | ACT_IS_NPC;
        p_mob_proto->affect_flags = fread_flag(fp);
        p_mob_proto->pShop = NULL;
        p_mob_proto->alignment = (int16_t)fread_number(fp);
        letter = fread_letter(fp);
        p_mob_proto->level = (LEVEL)fread_number(fp);

        /*
         * The unused stuff is for imps who want to use the old-style
         * stats-in-files method.
         */
        fread_number(fp); /* Unused */
        fread_number(fp); /* Unused */
        fread_number(fp); /* Unused */
        fread_letter(fp); /* d */
        fread_number(fp); /* Unused */
        fread_letter(fp); /* + */
        fread_number(fp); /* Unused */
        fread_number(fp); /* Unused */
        fread_letter(fp); /* d */
        fread_number(fp); /* Unused */
        fread_letter(fp); /* + */
        fread_number(fp); /* Unused */
        p_mob_proto->wealth = fread_number(fp) / 20;
        /* xp can't be used! */
        fread_number(fp); /* Unused */
        p_mob_proto->start_pos = (int16_t)fread_number(fp); /* Unused */
        p_mob_proto->default_pos = (int16_t)fread_number(fp); /* Unused */

        if (p_mob_proto->start_pos < POS_SLEEPING)
            p_mob_proto->start_pos = POS_STANDING;
        if (p_mob_proto->default_pos < POS_SLEEPING)
            p_mob_proto->default_pos = POS_STANDING;

        /*
         * Back to meaningful values.
         */
        p_mob_proto->sex = (Sex)fread_number(fp);

        /* compute the race BS */
        one_argument(p_mob_proto->name, name);

        if (name[0] == '\0' || (race = race_lookup(name)) == 0) {
            /* fill in with blanks */
            p_mob_proto->race = (int16_t)race_lookup("human");
            p_mob_proto->atk_flags
                = ATK_DODGE | ATK_DISARM | ATK_TRIP | ASSIST_VNUM;
            p_mob_proto->imm_flags = 0;
            p_mob_proto->res_flags = 0;
            p_mob_proto->vuln_flags = 0;
            p_mob_proto->form
                = FORM_EDIBLE | FORM_SENTIENT | FORM_BIPED | FORM_MAMMAL;
            p_mob_proto->parts = PART_HEAD | PART_ARMS | PART_LEGS | PART_HEART
                | PART_BRAINS | PART_GUTS;
        }
        else {
            p_mob_proto->race = (int16_t)race;
            p_mob_proto->atk_flags = ATK_DODGE | ATK_DISARM | ATK_TRIP
                | ASSIST_RACE | race_table[race].off;
            p_mob_proto->imm_flags = race_table[race].imm;
            p_mob_proto->res_flags = race_table[race].res;
            p_mob_proto->vuln_flags = race_table[race].vuln;
            p_mob_proto->form = race_table[race].form;
            p_mob_proto->parts = race_table[race].parts;
        }

        if (letter != 'S') {
            bug("Load_mobiles: vnum %"PRVNUM" non-S.", vnum);
            exit(1);
        }

        convert_mobile(p_mob_proto);  // OLC

        iHash = vnum % MAX_KEY_HASH;
        p_mob_proto->next = mob_prototype_hash[iHash];
        mob_prototype_hash[iHash] = p_mob_proto;
        top_mob_prototype++;
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;   // OLC
        assign_area_vnum(vnum);                                     // OLC
        kill_table[URANGE(0, p_mob_proto->level, MAX_LEVEL - 1)].number++;
    }

    return;
}

MobPrototype* new_mob_prototype()
{
    MobPrototype* pMob;

    if (!mob_prototype_free) {
        pMob = alloc_perm(sizeof(*pMob));
        top_mob_prototype++;
    }
    else {
        pMob = mob_prototype_free;
        mob_prototype_free = mob_prototype_free->next;
    }

    pMob->next = NULL;
    pMob->spec_fun = NULL;
    pMob->pShop = NULL;
    pMob->area = NULL;
    pMob->name = str_dup("no name");
    pMob->short_descr = str_dup("(no short description)");
    pMob->long_descr = str_dup("(no long description)\n\r");
    pMob->description = &str_empty[0];
    pMob->vnum = 0;
    pMob->count = 0;
    pMob->killed = 0;
    pMob->sex = SEX_NEUTRAL;
    pMob->level = 0;
    pMob->act_flags = ACT_IS_NPC;
    pMob->affect_flags = 0;
    pMob->alignment = 0;
    pMob->hitroll = 0;
    pMob->race = (int16_t)race_lookup("human"); /* - Hugin */
    pMob->form = 0;           /* ROM patch -- Hugin */
    pMob->parts = 0;           /* ROM patch -- Hugin */
    pMob->imm_flags = 0;           /* ROM patch -- Hugin */
    pMob->res_flags = 0;           /* ROM patch -- Hugin */
    pMob->vuln_flags = 0;           /* ROM patch -- Hugin */
    pMob->material = str_dup("unknown"); /* -- Hugin */
    pMob->atk_flags = 0;           /* ROM patch -- Hugin */
    pMob->size = SIZE_MEDIUM; /* ROM patch -- Hugin */
    pMob->ac[AC_PIERCE] = 0;           /* ROM patch -- Hugin */
    pMob->ac[AC_BASH] = 0;           /* ROM patch -- Hugin */
    pMob->ac[AC_SLASH] = 0;           /* ROM patch -- Hugin */
    pMob->ac[AC_EXOTIC] = 0;           /* ROM patch -- Hugin */
    pMob->hit[DICE_NUMBER] = 0;   /* ROM patch -- Hugin */
    pMob->hit[DICE_TYPE] = 0;   /* ROM patch -- Hugin */
    pMob->hit[DICE_BONUS] = 0;   /* ROM patch -- Hugin */
    pMob->mana[DICE_NUMBER] = 0;   /* ROM patch -- Hugin */
    pMob->mana[DICE_TYPE] = 0;   /* ROM patch -- Hugin */
    pMob->mana[DICE_BONUS] = 0;   /* ROM patch -- Hugin */
    pMob->damage[DICE_NUMBER] = 0;   /* ROM patch -- Hugin */
    pMob->damage[DICE_TYPE] = 0;   /* ROM patch -- Hugin */
    pMob->damage[DICE_NUMBER] = 0;   /* ROM patch -- Hugin */
    pMob->start_pos = POS_STANDING; /*  -- Hugin */
    pMob->default_pos = POS_STANDING; /*  -- Hugin */
    pMob->wealth = 0;
    pMob->mprogs = NULL;
    pMob->mprog_flags = 0;

    pMob->new_format = true;  /* ROM */

    return pMob;
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
