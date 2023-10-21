////////////////////////////////////////////////////////////////////////////////
// object_data.c
// Utilities for handling in-game objects
////////////////////////////////////////////////////////////////////////////////

#include "object_data.h"

#include "db.h"
#include "handler.h"
#include "lookup.h"
#include "magic.h"
#include "recycle.h"

ObjectPrototype* object_prototype_hash[MAX_KEY_HASH];
int obj_proto_count;
int obj_count;
VNUM top_vnum_obj;
ObjectPrototype* object_prototype_free;
ObjectData* object_free;
ObjectData* object_list;

void clone_object(ObjectData* parent, ObjectData* clone)
{
    int i;
    AffectData* paf;
    ExtraDesc* ed, * ed_new;

    if (parent == NULL || clone == NULL)
        return;

    /* start fixing the object */
    clone->name = str_dup(parent->name);
    clone->short_descr = str_dup(parent->short_descr);
    clone->description = str_dup(parent->description);
    clone->item_type = parent->item_type;
    clone->extra_flags = parent->extra_flags;
    clone->wear_flags = parent->wear_flags;
    clone->weight = parent->weight;
    clone->cost = parent->cost;
    clone->level = parent->level;
    clone->condition = parent->condition;
    clone->material = str_dup(parent->material);
    clone->timer = parent->timer;

    for (i = 0; i < 5; i++) 
        clone->value[i] = parent->value[i];

    /* affects */
    clone->enchanted = parent->enchanted;

    FOR_EACH(paf, parent->affected)
        affect_to_obj(clone, paf);

    /* extended desc */
    FOR_EACH(ed, parent->extra_desc) {
        ed_new = new_extra_desc();
        ed_new->keyword = str_dup(ed->keyword);
        ed_new->description = str_dup(ed->description);
        ed_new->next = clone->extra_desc;
        clone->extra_desc = ed_new;
    }
}


// Preserved for historical reasons, and to demonstrate how current values were
// arrived at. -- Halivar
//void convert_object(ObjectPrototype* obj_proto)
//{
//    LEVEL level;
//    int number, type;  /* for dice-conversion */
//
//    if (!obj_proto || obj_proto->new_format) return;
//
//    level = obj_proto->level;
//
//    obj_proto->level = UMAX(0, obj_proto->level); /* just to be sure */
//    obj_proto->cost = 10 * level;
//
//    switch (obj_proto->item_type) {
//    case ITEM_WAND:
//    case ITEM_STAFF:
//        obj_proto->value[2] = obj_proto->value[1];
//        break;
//
//    case ITEM_WEAPON:
//
//        /*
//         * The conversion below is based on the values generated
//         * in one_hit() (fight.c).  Since I don't want a lvl 50
//         * weapon to do 15d3 damage, the min value will be below
//         * the one in one_hit, and to make up for it, I've made
//         * the max value higher.
//         * (I don't want 15d2 because this will hardly ever roll
//         * 15 or 30, it will only roll damage close to 23.
//         * I can't do 4d8+11, because one_hit there is no dice-
//         * bounus value to set...)
//         *
//         * The conversion below gives:
//
//         level:   dice      min      max      mean
//           1:     1d8      1( 2)    8( 7)     5( 5)
//           2:     2d5      2( 3)   10( 8)     6( 6)
//           3:     2d5      2( 3)   10( 8)     6( 6)
//           5:     2d6      2( 3)   12(10)     7( 7)
//          10:     4d5      4( 5)   20(14)    12(10)
//          20:     5d5      5( 7)   25(21)    15(14)
//          30:     5d7      5(10)   35(29)    20(20)
//          50:     5d11     5(15)   55(44)    30(30)
//
//         */
//
//        number = UMIN(level / 4 + 1, 5);
//        type = (level + 7) / number;
//
//        obj_proto->value[1] = number;
//        obj_proto->value[2] = type;
//        break;
//
//    case ITEM_ARMOR:
//        obj_proto->value[0] = level / 5 + 3;
//        obj_proto->value[1] = obj_proto->value[0];
//        obj_proto->value[2] = obj_proto->value[0];
//        break;
//
//    case ITEM_MONEY:
//        obj_proto->value[0] = obj_proto->cost;
//        break;
//
//    default:
//        bug("Obj_convert: vnum %"PRVNUM" bad type.", obj_proto->item_type);
//        break;
//    }
//
//    obj_proto->new_format = true;
//
//    return;
//}

ObjectData* create_object(ObjectPrototype* obj_proto, LEVEL level)
{
    AffectData* paf;
    ObjectData* obj;
    int i;

    if (obj_proto == NULL) {
        bug("Create_object: NULL obj_proto.", 0);
        exit(1);
    }

    obj = new_object();

    obj->prototype = obj_proto;
    obj->in_room = NULL;
    obj->enchanted = false;

    obj->level = obj_proto->level;
    obj->wear_loc = -1;

    obj->name = str_dup(obj_proto->name);           /* OLC */
    obj->short_descr = str_dup(obj_proto->short_descr);    /* OLC */
    obj->description = str_dup(obj_proto->description);    /* OLC */
    obj->material = str_dup(obj_proto->material);
    obj->item_type = obj_proto->item_type;
    obj->extra_flags = obj_proto->extra_flags;
    obj->wear_flags = obj_proto->wear_flags;
    obj->value[0] = obj_proto->value[0];
    obj->value[1] = obj_proto->value[1];
    obj->value[2] = obj_proto->value[2];
    obj->value[3] = obj_proto->value[3];
    obj->value[4] = obj_proto->value[4];
    obj->weight = obj_proto->weight;

    if (level == -1)
        obj->cost = obj_proto->cost;
    else
        obj->cost = number_fuzzy(10) * number_fuzzy(level) * number_fuzzy(level);

// Mess with object properties.
    switch (obj->item_type) {
    case ITEM_LIGHT:
        if (obj->value[2] == 999) obj->value[2] = -1;
        break;

    case ITEM_JUKEBOX:
        for (i = 0; i < 5; i++) obj->value[i] = -1;
        break;

    default:
        break;
    }

    FOR_EACH(paf, obj_proto->affected)
        if (paf->location == APPLY_SPELL_AFFECT) 
            affect_to_obj(obj, paf);

    obj->next = object_list;
    object_list = obj;
    obj_proto->count++;
    obj_count++;

    return obj;
}

void free_object(ObjectData* obj)
{
    AffectData* paf;
    AffectData* paf_next = NULL;
    ExtraDesc* ed;
    ExtraDesc* ed_next = NULL;

    if (!IS_VALID(obj)) 
        return;

    for (paf = obj->affected; paf != NULL; paf = paf_next) {
        paf_next = paf->next;
        free_affect(paf);
    }
    obj->affected = NULL;

    for (ed = obj->extra_desc; ed != NULL; ed = ed_next) {
        ed_next = ed->next;
        free_extra_desc(ed);
    }
    obj->extra_desc = NULL;

    free_string(obj->name);
    free_string(obj->description);
    free_string(obj->short_descr);
    free_string(obj->owner);
    INVALIDATE(obj);

    obj_count--;
    obj->next = object_free;
    object_free = obj;
}

void free_object_prototype(ObjectPrototype* pObj)
{
    ExtraDesc* pExtra;
    AffectData* pAf;

    free_string(pObj->name);
    free_string(pObj->short_descr);
    free_string(pObj->description);

    FOR_EACH(pAf, pObj->affected) {
        free_affect(pAf);
    }

    FOR_EACH(pExtra, pObj->extra_desc) {
        free_extra_desc(pExtra);
    }

    pObj->next = object_prototype_free;
    object_prototype_free = pObj;
    return;
}

// Translates mob virtual number to its obj index struct.
// Hash table lookup.
ObjectPrototype* get_object_prototype(VNUM vnum)
{
    ObjectPrototype* obj_proto;

    for (obj_proto = object_prototype_hash[vnum % MAX_KEY_HASH]; obj_proto != NULL;
        NEXT_LINK(obj_proto)) {
        if (obj_proto->vnum == vnum) 
            return obj_proto;
    }

    if (fBootDb) {
        bug("Get_object_prototype: bad vnum %"PRVNUM".", vnum);
        exit(1);
    }

    return NULL;
}

// Snarf an obj section. new style
void load_objects(FILE* fp)
{
    ObjectPrototype* obj_proto;

    if (!area_last)   /* OLC */
    {
        bug("Load_objects: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        VNUM vnum;
        char letter;
        int iHash;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_objects: # not found.", 0);
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0) 
            break;

        fBootDb = false;
        if (get_object_prototype(vnum) != NULL) {
            bug("Load_objects: vnum %"PRVNUM" duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        obj_proto = alloc_perm(sizeof(*obj_proto));
        obj_proto->vnum = vnum;
        obj_proto->area = area_last;
        obj_proto->reset_num = 0;
        obj_proto->name = fread_string(fp);
        obj_proto->short_descr = fread_string(fp);
        obj_proto->description = fread_string(fp);
        obj_proto->material = fread_string(fp);

        CHECK_POS(obj_proto->item_type, (int16_t)item_lookup(fread_word(fp)), "item_type");
        obj_proto->extra_flags = fread_flag(fp);
        obj_proto->wear_flags = fread_flag(fp);
        switch (obj_proto->item_type) {
        case ITEM_WEAPON:
            obj_proto->value[0] = weapon_lookup(fread_word(fp));
            obj_proto->value[1] = fread_number(fp);
            obj_proto->value[2] = fread_number(fp);
            obj_proto->value[3] = attack_lookup(fread_word(fp));
            obj_proto->value[4] = fread_flag(fp);
            break;
        case ITEM_CONTAINER:
            obj_proto->value[0] = fread_number(fp);
            obj_proto->value[1] = fread_flag(fp);
            obj_proto->value[2] = fread_number(fp);
            obj_proto->value[3] = fread_number(fp);
            obj_proto->value[4] = fread_number(fp);
            break;
        case ITEM_DRINK_CON:
        case ITEM_FOUNTAIN:
            obj_proto->value[0] = fread_number(fp);
            obj_proto->value[1] = fread_number(fp);
            CHECK_POS(obj_proto->value[2], liquid_lookup(fread_word(fp)), "liquid_lookup");
            obj_proto->value[3] = fread_number(fp);
            obj_proto->value[4] = fread_number(fp);
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            obj_proto->value[0] = fread_number(fp);
            obj_proto->value[1] = fread_number(fp);
            obj_proto->value[2] = fread_number(fp);
            obj_proto->value[3] = skill_lookup(fread_word(fp));
            obj_proto->value[4] = fread_number(fp);
            break;
        case ITEM_POTION:
        case ITEM_PILL:
        case ITEM_SCROLL:
            obj_proto->value[0] = fread_number(fp);
            obj_proto->value[1] = skill_lookup(fread_word(fp));
            obj_proto->value[2] = skill_lookup(fread_word(fp));
            obj_proto->value[3] = skill_lookup(fread_word(fp));
            obj_proto->value[4] = skill_lookup(fread_word(fp));
            break;
        default:
            obj_proto->value[0] = fread_flag(fp);
            obj_proto->value[1] = fread_flag(fp);
            obj_proto->value[2] = fread_flag(fp);
            obj_proto->value[3] = fread_flag(fp);
            obj_proto->value[4] = fread_flag(fp);
            break;
        }
        obj_proto->level = (int16_t)fread_number(fp);
        obj_proto->weight = (int16_t)fread_number(fp);
        obj_proto->cost = fread_number(fp);

        /* condition */
        letter = fread_letter(fp);
        switch (letter) {
        case ('P'):
            obj_proto->condition = 100;
            break;
        case ('G'):
            obj_proto->condition = 90;
            break;
        case ('A'):
            obj_proto->condition = 75;
            break;
        case ('W'):
            obj_proto->condition = 50;
            break;
        case ('D'):
            obj_proto->condition = 25;
            break;
        case ('B'):
            obj_proto->condition = 10;
            break;
        case ('R'):
            obj_proto->condition = 0;
            break;
        default:
            obj_proto->condition = 100;
            break;
        }

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'A') {
                AffectData* paf;

                paf = alloc_perm(sizeof(AffectData));
                if (paf == NULL)
                    exit(1);
                paf->where = TO_OBJECT;
                paf->type = -1;
                paf->level = obj_proto->level;
                paf->duration = -1;
                paf->location = (int16_t)fread_number(fp);
                paf->modifier = (int16_t)fread_number(fp);
                paf->bitvector = 0;
                ADD_AFF_DATA(obj_proto, paf)
                affect_count++;
            }

            else if (letter == 'F') {
                AffectData* paf;

                paf = alloc_perm(sizeof(AffectData));
                if (paf == NULL)
                    exit(1);
                letter = fread_letter(fp);
                switch (letter) {
                case 'A':
                    paf->where = TO_AFFECTS;
                    break;
                case 'I':
                    paf->where = TO_IMMUNE;
                    break;
                case 'R':
                    paf->where = TO_RESIST;
                    break;
                case 'V':
                    paf->where = TO_VULN;
                    break;
                default:
                    bug("Load_objects: Bad where on flag set.", 0);
                    exit(1);
                }
                paf->type = -1;
                paf->level = obj_proto->level;
                paf->duration = -1;
                paf->location = (int16_t)fread_number(fp);
                paf->modifier = (int16_t)fread_number(fp);
                paf->bitvector = fread_flag(fp);
                ADD_AFF_DATA(obj_proto, paf)
                affect_count++;
            }

            else if (letter == 'E') {
                ExtraDesc* ed;

                ed = alloc_perm(sizeof(ExtraDesc));
                if (ed == NULL)
                    exit(1);
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ADD_EXTRA_DESC(obj_proto, ed)
                extra_desc_count++;
            }

            else {
                ungetc(letter, fp);
                break;
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        obj_proto->next = object_prototype_hash[iHash];
        object_prototype_hash[iHash] = obj_proto;
        obj_proto_count++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;   // OLC
        assign_area_vnum(vnum);                                     // OLC
    }

    return;
}

ObjectData* new_object()
{
    static ObjectData obj_zero;
    ObjectData* obj;

    if (object_free == NULL)
        obj = alloc_perm(sizeof(*obj));
    else {
        obj = object_free;
        NEXT_LINK(object_free);
    }
    *obj = obj_zero;
    VALIDATE(obj);

    return obj;
}

ObjectPrototype* new_object_prototype()
{
    ObjectPrototype* pObj;
    int value;

    if (!object_prototype_free) {
        pObj = alloc_perm(sizeof(*pObj));
        obj_proto_count++;
    }
    else {
        pObj = object_prototype_free;
        NEXT_LINK(object_prototype_free);
    }

    pObj->next = NULL;
    pObj->extra_desc = NULL;
    pObj->affected = NULL;
    pObj->area = NULL;
    pObj->name = str_dup("no name");
    pObj->short_descr = str_dup("(no short description)");
    pObj->description = str_dup("(no description)");
    pObj->vnum = 0;
    pObj->item_type = ITEM_TRASH;
    pObj->extra_flags = 0;
    pObj->wear_flags = 0;
    pObj->count = 0;
    pObj->weight = 0;
    pObj->cost = 0;
    pObj->condition = 100;
    for (value = 0; value < 5; value++)
        pObj->value[value] = 0;

    return pObj;
}

