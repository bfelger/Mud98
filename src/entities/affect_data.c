////////////////////////////////////////////////////////////////////////////////
// affect_data.c
// Utilities to handle special conditions and status affects
////////////////////////////////////////////////////////////////////////////////

#include "affect_data.h"

#include "comm.h"
#include "db.h"
#include "handler.h"

#include "char_data.h"
#include "object_data.h"

AffectData* affect_free;

// fix object affects when removing one
void affect_check(CharData* ch, int where, int vector)
{
    AffectData* paf;
    ObjectData* obj;

    if (where == TO_OBJECT || where == TO_WEAPON || vector == 0) return;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
        if (paf->where == where && paf->bitvector == vector) {
            switch (where) {
            case TO_AFFECTS:
                SET_BIT(ch->affected_by, vector);
                break;
            case TO_IMMUNE:
                SET_BIT(ch->imm_flags, vector);
                break;
            case TO_RESIST:
                SET_BIT(ch->res_flags, vector);
                break;
            case TO_VULN:
                SET_BIT(ch->vuln_flags, vector);
                break;
            }
            return;
        }

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content) {
        if (obj->wear_loc == -1) continue;

        for (paf = obj->affected; paf != NULL; paf = paf->next)
            if (paf->where == where && paf->bitvector == vector) {
                switch (where) {
                case TO_AFFECTS:
                    SET_BIT(ch->affected_by, vector);
                    break;
                case TO_IMMUNE:
                    SET_BIT(ch->imm_flags, vector);
                    break;
                case TO_RESIST:
                    SET_BIT(ch->res_flags, vector);
                    break;
                case TO_VULN:
                    SET_BIT(ch->vuln_flags, vector);
                }
                return;
            }

        if (obj->enchanted) continue;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
            if (paf->where == where && paf->bitvector == vector) {
                switch (where) {
                case TO_AFFECTS:
                    SET_BIT(ch->affected_by, vector);
                    break;
                case TO_IMMUNE:
                    SET_BIT(ch->imm_flags, vector);
                    break;
                case TO_RESIST:
                    SET_BIT(ch->res_flags, vector);
                    break;
                case TO_VULN:
                    SET_BIT(ch->vuln_flags, vector);
                    break;
                }
                return;
            }
    }
}

// enchanted stuff for eq
void affect_enchant(ObjectData* obj)
{
    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted) {
        AffectData* paf, * af_new;
        obj->enchanted = true;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) {
            af_new = new_affect();

            af_new->next = obj->affected;
            obj->affected = af_new;

            af_new->where = paf->where;
            af_new->type = UMAX(0, paf->type);
            af_new->level = paf->level;
            af_new->duration = paf->duration;
            af_new->location = paf->location;
            af_new->modifier = paf->modifier;
            af_new->bitvector = paf->bitvector;
        }
    }
}

// find an effect in an affect list
AffectData* affect_find(AffectData* paf, SKNUM sn)
{
    AffectData* paf_find;

    for (paf_find = paf; paf_find != NULL; paf_find = paf_find->next) {
        if (paf_find->type == sn) return paf_find;
    }

    return NULL;
}

// Add or enhance an affect.
void affect_join(CharData* ch, AffectData* paf)
{
    AffectData* paf_old;

    for (paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next) {
        if (paf_old->type == paf->type) {
            // TODO: Don't just take the average; figure out how to increase
            //       level the right way.
            paf->level = (paf->level + paf_old->level) / 2;
            paf->duration += paf_old->duration;
            paf->modifier += paf_old->modifier;
            affect_remove(ch, paf_old);
            break;
        }
    }

    affect_to_char(ch, paf);
    return;
}

// Apply or remove an affect to a character.
void affect_modify(CharData* ch, AffectData* paf, bool fAdd)
{
    ObjectData* wield;
    int16_t mod;
    int i;

    mod = paf->modifier;

    if (fAdd) {
        switch (paf->where) {
        case TO_AFFECTS:
            SET_BIT(ch->affected_by, paf->bitvector);
            break;
        case TO_IMMUNE:
            SET_BIT(ch->imm_flags, paf->bitvector);
            break;
        case TO_RESIST:
            SET_BIT(ch->res_flags, paf->bitvector);
            break;
        case TO_VULN:
            SET_BIT(ch->vuln_flags, paf->bitvector);
            break;
        case TO_WEAPON:
        case TO_OBJECT:
        default:
            break;
        }
    }
    else {
        switch (paf->where) {
        case TO_AFFECTS:
            REMOVE_BIT(ch->affected_by, paf->bitvector);
            break;
        case TO_IMMUNE:
            REMOVE_BIT(ch->imm_flags, paf->bitvector);
            break;
        case TO_RESIST:
            REMOVE_BIT(ch->res_flags, paf->bitvector);
            break;
        case TO_VULN:
            REMOVE_BIT(ch->vuln_flags, paf->bitvector);
            break;
        case TO_WEAPON:
        case TO_OBJECT:
        default:
            break;
        }
        mod = 0 - mod;
    }

    switch (paf->location) {
    default:
        bug("Affect_modify: unknown location %d.", paf->location);
        return;

    case APPLY_NONE:
        break;
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
    case APPLY_CLASS:
        break;
    case APPLY_LEVEL:
        break;
    case APPLY_AGE:
        break;
    case APPLY_HEIGHT:
        break;
    case APPLY_WEIGHT:
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
    case APPLY_GOLD:
        break;
    case APPLY_EXP:
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
    case APPLY_SPELL_AFFECT:
        break;
    }

    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */
    if (!IS_NPC(ch) && (wield = get_eq_char(ch, WEAR_WIELD)) != NULL
        && get_obj_weight(wield)
        > (str_app[get_curr_stat(ch, STAT_STR)].wield * 10)) {
        static int depth;

        if (depth == 0) {
            depth++;
            act("You drop $p.", ch, wield, NULL, TO_CHAR);
            act("$n drops $p.", ch, wield, NULL, TO_ROOM);
            obj_from_char(wield);
            obj_to_room(wield, ch->in_room);
            depth--;
        }
    }

    return;
}

// Remove an affect from a char.
void affect_remove(CharData* ch, AffectData* paf)
{
    int where;
    int vector;

    if (ch->affected == NULL) {
        bug("Affect_remove: no affect.", 0);
        return;
    }

    affect_modify(ch, paf, false);
    where = paf->where;
    vector = paf->bitvector;

    if (paf == ch->affected) { ch->affected = paf->next; }
    else {
        AffectData* prev;

        for (prev = ch->affected; prev != NULL; prev = prev->next) {
            if (prev->next == paf) {
                prev->next = paf->next;
                break;
            }
        }

        if (prev == NULL) {
            bug("Affect_remove: cannot find paf.", 0);
            return;
        }
    }

    free_affect(paf);

    affect_check(ch, where, vector);
    return;
}

void affect_remove_obj(ObjectData* obj, AffectData* paf)
{
    int where, vector;
    if (obj->affected == NULL) {
        bug("Affect_remove_object: no affect.", 0);
        return;
    }

    if (obj->carried_by != NULL && obj->wear_loc != -1)
        affect_modify(obj->carried_by, paf, false);

    where = paf->where;
    vector = paf->bitvector;

    /* remove flags from the object if needed */
    if (paf->bitvector) switch (paf->where) {
    case TO_OBJECT:
        REMOVE_BIT(obj->extra_flags, paf->bitvector);
        break;
    case TO_WEAPON:
        if (obj->item_type == ITEM_WEAPON)
            REMOVE_BIT(obj->value[4], paf->bitvector);
        break;
    case TO_VULN:
    case TO_RESIST:
    case TO_IMMUNE:
    case TO_AFFECTS:
    default:
        break;
    }

    if (paf == obj->affected) { obj->affected = paf->next; }
    else {
        AffectData* prev;

        for (prev = obj->affected; prev != NULL; prev = prev->next) {
            if (prev->next == paf) {
                prev->next = paf->next;
                break;
            }
        }

        if (prev == NULL) {
            bug("Affect_remove_object: cannot find paf.", 0);
            return;
        }
    }

    free_affect(paf);

    if (obj->carried_by != NULL && obj->wear_loc != -1)
        affect_check(obj->carried_by, where, vector);
    return;
}

// Strip all affects of a given sn.
void affect_strip(CharData* ch, SKNUM sn)
{
    AffectData* paf;
    AffectData* paf_next = NULL;

    for (paf = ch->affected; paf != NULL; paf = paf_next) {
        paf_next = paf->next;
        if (paf->type == sn) affect_remove(ch, paf);
    }

    return;
}

// Give an affect to a char.
void affect_to_char(CharData* ch, AffectData* paf)
{
    AffectData* paf_new;

    paf_new = new_affect();

    *paf_new = *paf;

    VALIDATE(paf); /* in case we missed it when we set up paf */
    paf_new->next = ch->affected;
    ch->affected = paf_new;

    affect_modify(ch, paf_new, true);
    return;
}

// give an affect to an object 
void affect_to_obj(ObjectData* obj, AffectData* paf)
{
    AffectData* paf_new;

    paf_new = new_affect();

    *paf_new = *paf;

    VALIDATE(paf); /* in case we missed it when we set up paf */
    paf_new->next = obj->affected;
    obj->affected = paf_new;

    /* apply any affect vectors to the object's extra_flags */
    if (paf->bitvector) switch (paf->where) {
    case TO_OBJECT:
        SET_BIT(obj->extra_flags, paf->bitvector);
        break;
    case TO_WEAPON:
        if (obj->item_type == ITEM_WEAPON)
            SET_BIT(obj->value[4], paf->bitvector);
        break;
    case TO_IMMUNE:
    case TO_AFFECTS:
    case TO_VULN:
    case TO_RESIST:
    default:
        break;
    }

    return;
}

void free_affect(AffectData* af)
{
    if (!IS_VALID(af)) return;

    INVALIDATE(af);
    af->next = affect_free;
    affect_free = af;
}

bool is_affected(CharData* ch, SKNUM sn)
{
    AffectData* paf;

    for (paf = ch->affected; paf != NULL; paf = paf->next) {
        if (paf->type == sn) return true;
    }

    return false;
}

AffectData* new_affect()
{
    static AffectData af_zero;
    AffectData* af;

    if (affect_free == NULL)
        af = alloc_perm(sizeof(*af));
    else {
        af = affect_free;
        affect_free = affect_free->next;
    }

    *af = af_zero;

    VALIDATE(af);
    return af;
}
