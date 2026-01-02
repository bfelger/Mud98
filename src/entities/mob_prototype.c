////////////////////////////////////////////////////////////////////////////////
// entities/mob_prototype.c
// Prototype data for mobile (NPC) Mobile
////////////////////////////////////////////////////////////////////////////////

#include "mob_prototype.h"

#include "mobile.h"

#include <comm.h>
#include <db.h>
#include <handler.h>
#include <lookup.h>
#include <magic.h>
#include <recycle.h>
#include <tables.h>

#include <olc/olc.h>

#include <data/mobile_data.h>
#include <data/race.h>

#include <entities/event.h>

#include <lox/memory.h>
#include <lox/ordered_table.h>

static OrderedTable mob_protos;
MobPrototype* mob_proto_free = NULL;

int mob_proto_count;
int mob_proto_perm_count;
VNUM top_vnum_mob;

void init_global_mob_protos(void)
{
    ordered_table_init(&mob_protos);
}

void free_global_mob_protos(void)
{
    ordered_table_free(&mob_protos);
}

MobPrototype* global_mob_proto_get(VNUM vnum)
{
    Value value = NIL_VAL;
    if (ordered_table_get_vnum(&mob_protos, vnum, &value) && IS_MOB_PROTO(value))
        return AS_MOB_PROTO(value);
    return NULL;
}

bool global_mob_proto_set(MobPrototype* proto)
{
    if (proto == NULL)
        return false;
    return ordered_table_set_vnum(&mob_protos, VNUM_FIELD(proto), OBJ_VAL(proto));
}

bool global_mob_proto_remove(VNUM vnum)
{
    return ordered_table_delete_vnum(&mob_protos, (int32_t)vnum);
}

int global_mob_proto_count(void)
{
    return ordered_table_count(&mob_protos);
}

GlobalMobProtoIter make_global_mob_proto_iter(void)
{
    GlobalMobProtoIter iter = { ordered_table_iter(&mob_protos) };
    return iter;
}

MobPrototype* global_mob_proto_iter_next(GlobalMobProtoIter* iter)
{
    if (iter == NULL)
        return NULL;

    Value value;
    while (ordered_table_iter_next(&iter->iter, NULL, &value)) {
        if (IS_MOB_PROTO(value))
            return AS_MOB_PROTO(value);
    }

    return NULL;
}

OrderedTable snapshot_global_mob_protos(void)
{
    return mob_protos;
}

void restore_global_mob_protos(OrderedTable snapshot)
{
    mob_protos = snapshot;
}

void mark_global_mob_protos(void)
{
    mark_ordered_table(&mob_protos);
}

MobPrototype* new_mob_prototype()
{
    LIST_ALLOC_PERM(mob_proto, MobPrototype);

    gc_protect(OBJ_VAL(mob_proto));

    init_header(&mob_proto->header, OBJ_MOB_PROTO);

    if (fBootDb) {
        mob_proto->short_descr = boot_intern_string("(no short description)");
        mob_proto->long_descr = boot_intern_string("(no long description)\n\r");
        mob_proto->material = boot_intern_string("unknown");
    }
    else {
        mob_proto->short_descr = str_dup("(no short description)");
        mob_proto->long_descr = str_dup("(no long description)\n\r");
        mob_proto->material = str_dup("unknown");
    }
    mob_proto->description = &str_empty[0];
    mob_proto->sex = SEX_NEUTRAL;
    mob_proto->act_flags = ACT_IS_NPC;
    mob_proto->race = (int16_t)race_lookup("human");
    mob_proto->size = SIZE_MEDIUM;
    mob_proto->start_pos = POS_STANDING;
    mob_proto->default_pos = POS_STANDING;
    mob_proto->faction_vnum = 0;

    return mob_proto;
}

void free_mob_prototype(MobPrototype* mob_proto)
{
    SET_NAME(mob_proto, lox_empty_string);

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
    MobPrototype* proto = global_mob_proto_get(vnum);
    if (proto)
        return proto;

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

    if (global_areas.count == 0)   /* OLC */
    {
        bug("Load_mobiles: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        VNUM vnum;
        char letter;

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

        VNUM_FIELD(p_mob_proto) = vnum;
        p_mob_proto->area = LAST_AREA_DATA;
        SET_NAME(p_mob_proto, fread_lox_string(fp));
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

        p_mob_proto->material = fread_word(fp);

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'F') {
                char* word;

                word = fread_word(fp);
                // Woah-ho! This is super hacky.
                if (!str_prefix(word, "action")) {
                    p_mob_proto->faction_vnum = fread_number(fp);
                    continue;
                }
                
                long vector = fread_flag(fp);

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
            else if (letter == 'V') {
                load_event(fp, &p_mob_proto->header);
            }
            else if (letter == 'L') {
                int next = getc(fp);
                if (next == '\n' || next == '\r') {
                    // Lox script format: L\n<script>~
                    // Newline consumed, now read the script
                    if (!load_lox_class(fp, "mob", &p_mob_proto->header)) {
                        bug("Load_mobiles: vnum %"PRVNUM" has malformed Lox script.", vnum);
                        exit(1);
                    }
                }
                else if (next == 'o') {
                    // LootTable format: LootTable <name>~
                    // 'L' and 'o' consumed, skip rest of "otTable"
                    char* word = fread_word(fp);
                    if (!str_cmp(word, "otTable")) {
                        p_mob_proto->loot_table = fread_string(fp);
                    }
                    else {
                        bug("Load_mobiles: vnum %"PRVNUM" expected LootTable, got Lo%s.", vnum, word);
                    }
                }
                else {
                    bug("Load_mobiles: vnum %"PRVNUM" unexpected char after L: '%c'.", vnum, (char)next);
                    ungetc(next, fp);
                }
            }
            else {
                ungetc(letter, fp);
                break;
            }
        }

        global_mob_proto_set(p_mob_proto);

        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;
        assign_area_vnum(vnum);
        kill_table[URANGE(0, p_mob_proto->level, MAX_LEVEL - 1)].number++;
    }

    return;
}

void recalc(MobPrototype* pMob)
{
    int hplev, aclev, damlev, hitbonus;
    float n;

    if (pMob->level == 0)
        return;

    hplev = 0; aclev = 0; damlev = 0; hitbonus = 0;

    if (IS_SET(pMob->act_flags, ACT_WARRIOR)) {
        hplev += 1;
    }

    if (IS_SET(pMob->act_flags, ACT_THIEF)) {
        hplev -= 1; aclev -= 1; damlev -= 1;
    }

    if (IS_SET(pMob->act_flags, ACT_CLERIC)) {
        damlev -= 2;
    }

    if (IS_SET(pMob->act_flags, ACT_MAGE)) {
        hplev -= 2; aclev -= 1; damlev -= 3;
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
