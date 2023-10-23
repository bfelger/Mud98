////////////////////////////////////////////////////////////////////////////////
// obj_prototype.c
// Utilities for handling in-game objects
////////////////////////////////////////////////////////////////////////////////

#include "obj_prototype.h"

#include "db.h"
#include "handler.h"
#include "lookup.h"

ObjPrototype* obj_proto_free;
ObjPrototype* obj_proto_hash[MAX_KEY_HASH];

int obj_proto_count;
VNUM top_vnum_obj;

void free_object_prototype(ObjPrototype* obj_proto)
{
    ExtraDesc* pExtra;
    Affect* pAf;

    free_string(obj_proto->name);
    free_string(obj_proto->short_descr);
    free_string(obj_proto->description);

    FOR_EACH(pAf, obj_proto->affected) {
        free_affect(pAf);
    }

    FOR_EACH(pExtra, obj_proto->extra_desc) {
        free_extra_desc(pExtra);
    }

    obj_proto->next = obj_proto_free;
    obj_proto_free = obj_proto;
}

// Translates mob virtual number to its obj index struct.
// Hash table lookup.
ObjPrototype* get_object_prototype(VNUM vnum)
{
    ObjPrototype* obj_proto;

    for (obj_proto = obj_proto_hash[vnum % MAX_KEY_HASH]; 
        obj_proto != NULL;
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
    ObjPrototype* obj_proto;

    if (!area_last) {
        bug("Load_objects: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        VNUM vnum;
        char letter;
        int hash;

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
                Affect* affect;
                if ((affect = alloc_perm(sizeof(*affect))) == NULL) {
                    bug("Ran out of memory allocating Affect.");
                    exit(1);
                }
                affect->where = TO_OBJECT;
                affect->type = -1;
                affect->level = obj_proto->level;
                affect->duration = -1;
                affect->location = (int16_t)fread_number(fp);
                affect->modifier = (int16_t)fread_number(fp);
                affect->bitvector = 0;
                ADD_AFFECT(obj_proto, affect)
                    affect_count++;
            }

            else if (letter == 'F') {
                Affect* affect;
                if ((affect = alloc_perm(sizeof(*affect))) == NULL) {
                    bug("Ran out of memory allocating Affect.");
                    exit(1);
                }
                letter = fread_letter(fp);
                switch (letter) {
                case 'A':
                    affect->where = TO_AFFECTS;
                    break;
                case 'I':
                    affect->where = TO_IMMUNE;
                    break;
                case 'R':
                    affect->where = TO_RESIST;
                    break;
                case 'V':
                    affect->where = TO_VULN;
                    break;
                default:
                    bug("Load_objects: Bad where on flag set.", 0);
                    exit(1);
                }
                affect->type = -1;
                affect->level = obj_proto->level;
                affect->duration = -1;
                affect->location = (int16_t)fread_number(fp);
                affect->modifier = (int16_t)fread_number(fp);
                affect->bitvector = fread_flag(fp);
                ADD_AFFECT(obj_proto, affect)
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

        hash = vnum % MAX_KEY_HASH;
        obj_proto->next = obj_proto_hash[hash];
        obj_proto_hash[hash] = obj_proto;
        obj_proto_count++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;
        assign_area_vnum(vnum);
    }
}

ObjPrototype* new_object_prototype()
{
    static ObjPrototype zero = { 0 };
    ObjPrototype* obj_proto;

    if (!obj_proto_free) {
        obj_proto = alloc_perm(sizeof(*obj_proto));
        obj_proto_count++;
    }
    else {
        obj_proto = obj_proto_free;
        NEXT_LINK(obj_proto_free);
    }

    *obj_proto = zero;

    obj_proto->name = str_dup("no name");
    obj_proto->short_descr = str_dup("(no short description)");
    obj_proto->description = str_dup("(no description)");
    obj_proto->item_type = ITEM_TRASH;
    obj_proto->condition = 100;

    return obj_proto;
}