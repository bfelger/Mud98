////////////////////////////////////////////////////////////////////////////////
// entities/obj_prototype.c
// Utilities for handling in-game objects
////////////////////////////////////////////////////////////////////////////////

#include "obj_prototype.h"

#include <entities/event.h>

#include <lox/memory.h>
#include <lox/ordered_table.h>

#include <db.h>
#include <handler.h>
#include <lookup.h>

#include <stdlib.h>
#include <string.h>

ObjPrototype* obj_proto_free = NULL;
static OrderedTable obj_protos;

int obj_proto_count;
int obj_proto_perm_count;

VNUM top_vnum_obj;

void init_global_obj_protos(void)
{
    ordered_table_init(&obj_protos);
}

void free_global_obj_protos(void)
{
    ordered_table_free(&obj_protos);
}

ObjPrototype* global_obj_proto_get(VNUM vnum)
{
    Value value;
    if (ordered_table_get_vnum(&obj_protos, vnum, &value) && IS_OBJ_PROTO(value))
        return AS_OBJ_PROTO(value);
    return NULL;
}

bool global_obj_proto_set(ObjPrototype* proto)
{
    if (proto == NULL)
        return false;
    return ordered_table_set_vnum(&obj_protos, VNUM_FIELD(proto), OBJ_VAL(proto));
}

bool global_obj_proto_remove(VNUM vnum)
{
    return ordered_table_delete_vnum(&obj_protos, (int32_t)vnum);
}

int global_obj_proto_count(void)
{
    return ordered_table_count(&obj_protos);
}

GlobalObjProtoIter make_global_obj_proto_iter(void)
{
    GlobalObjProtoIter iter = { ordered_table_iter(&obj_protos) };
    return iter;
}

ObjPrototype* global_obj_proto_iter_next(GlobalObjProtoIter* iter)
{
    if (iter == NULL)
        return NULL;

    Value value;
    while (ordered_table_iter_next(&iter->iter, NULL, &value)) {
        if (IS_OBJ_PROTO(value))
            return AS_OBJ_PROTO(value);
    }
    return NULL;
}

OrderedTable snapshot_global_obj_protos(void)
{
    return obj_protos;
}

void restore_global_obj_protos(OrderedTable snapshot)
{
    obj_protos = snapshot;
}

void mark_global_obj_protos(void)
{
    mark_ordered_table(&obj_protos);
}

ObjPrototype* new_object_prototype()
{
    LIST_ALLOC_PERM(obj_proto, ObjPrototype);

    gc_protect(OBJ_VAL(obj_proto));

    init_header(&obj_proto->header, OBJ_OBJ_PROTO);

    SET_NATIVE_FIELD(&obj_proto->header, obj_proto->header.vnum, vnum, I32);

    SET_NAME(obj_proto, lox_string("no name"));
    
    obj_proto->short_descr = fBootDb ? boot_intern_string("(no short description)")
        : str_dup("(no short description)");
    obj_proto->description = fBootDb ? boot_intern_string("(no description)")
        : str_dup("(no description)");
    obj_proto->item_type = ITEM_TRASH;
    obj_proto->condition = 100;
    obj_proto->salvage_mats = NULL;
    obj_proto->salvage_mat_count = 0;

    return obj_proto;
}

void free_object_prototype(ObjPrototype* obj_proto)
{
    ExtraDesc* pExtra;
    Affect* pAf;

    SET_NAME(obj_proto, lox_empty_string);
    free_string(obj_proto->short_descr);
    free_string(obj_proto->description);

    FOR_EACH(pAf, obj_proto->affected) {
        free_affect(pAf);
    }

    FOR_EACH(pExtra, obj_proto->extra_desc) {
        free_extra_desc(pExtra);
    }

    if (obj_proto->salvage_mats != NULL) {
        free(obj_proto->salvage_mats);
        obj_proto->salvage_mats = NULL;
        obj_proto->salvage_mat_count = 0;
    }

    LIST_FREE(obj_proto);
}

// Translates mob virtual number to its obj index struct.
// Hash table lookup.
ObjPrototype* get_object_prototype(VNUM vnum)
{
    ObjPrototype* proto = global_obj_proto_get(vnum);
    if (proto)
        return proto;

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

    if (global_areas.count == 0) {
        bug("Load_objects: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;) {
        VNUM vnum;
        char letter;

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

        obj_proto = new_object_prototype();
        VNUM_FIELD(obj_proto) = vnum;
        obj_proto->area = LAST_AREA_DATA;
        SET_NAME(obj_proto, fread_lox_string(fp));
        obj_proto->short_descr = fread_string(fp);
        obj_proto->description = fread_string(fp);
        obj_proto->material = fread_string(fp);

        CHECK_POS(obj_proto->item_type, (int16_t)item_lookup(fread_word(fp)), "item_type");
        obj_proto->extra_flags = fread_flag(fp);
        obj_proto->wear_flags = fread_flag(fp);
        switch (obj_proto->item_type) {
        case ITEM_WEAPON:
            obj_proto->weapon.weapon_type = weapon_lookup(fread_word(fp));
            obj_proto->weapon.num_dice = fread_number(fp);
            obj_proto->weapon.size_dice = fread_number(fp);
            obj_proto->weapon.damage_type = attack_lookup(fread_word(fp));
            obj_proto->weapon.flags = fread_flag(fp);
            break;
        case ITEM_ARMOR:
            obj_proto->armor.ac_pierce = fread_number(fp);
            obj_proto->armor.ac_bash = fread_number(fp);
            obj_proto->armor.ac_slash = fread_number(fp);
            obj_proto->armor.ac_exotic = fread_number(fp);
            obj_proto->armor.armor_type = fread_number(fp);
            obj_proto->armor.armor_type = armor_type_from_value(obj_proto->armor.armor_type);
            break;
        case ITEM_CONTAINER:
            obj_proto->container.capacity = fread_number(fp);
            obj_proto->container.flags = fread_flag(fp);
            obj_proto->container.key_vnum = fread_number(fp);
            obj_proto->container.max_item_weight = fread_number(fp);
            obj_proto->container.weight_mult = fread_number(fp);
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
        case ITEM_MAT:
            // Crafting material: mat_type amount quality unused unused
            obj_proto->craft_mat.mat_type = fread_number(fp);
            obj_proto->craft_mat.amount = fread_number(fp);
            obj_proto->craft_mat.quality = fread_number(fp);
            obj_proto->craft_mat.unused3 = fread_number(fp);
            obj_proto->craft_mat.unused4 = fread_number(fp);
            break;
        case ITEM_WORKSTATION:
            // Workstation: station_flags bonus unused unused unused
            obj_proto->workstation.station_flags = fread_flag(fp);
            obj_proto->workstation.bonus = fread_number(fp);
            obj_proto->workstation.unused2 = fread_number(fp);
            obj_proto->workstation.unused3 = fread_number(fp);
            obj_proto->workstation.unused4 = fread_number(fp);
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
                if ((affect = new_affect()) == NULL) {
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
            }

            else if (letter == 'F') {
                Affect* affect;
                if ((affect = new_affect()) == NULL) {
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
            }

            else if (letter == 'E') {
                ExtraDesc* ed = new_extra_desc();
                if (ed == NULL) {
                    bug("Ran out of memory allocating ExtraDesc.");
                    exit(1);
                }
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ADD_EXTRA_DESC(obj_proto, ed)
            }
            else if (letter == 'V') {
                load_event(fp, &obj_proto->header);
            }
            else if (letter == 'L') {
                if (!load_lox_class(fp, "obj", &obj_proto->header)) {
                    bug("Load_objects: vnum %"PRVNUM" has malformed Lox script.", vnum);
                    exit(1);
                }
            }
            else if (letter == 'S') {
                // SalvageMats format: SalvageMats <vnum> [<vnum>...] 0
                char* word = fread_word(fp);
                if (!str_cmp(word, "alvageMats")) {
                    // Read VNUMs until we hit 0
                    VNUM temp_mats[64];  // Temporary buffer for reading
                    int count = 0;
                    VNUM mat_vnum;
                    while ((mat_vnum = fread_number(fp)) != 0 && count < 64) {
                        temp_mats[count++] = mat_vnum;
                    }
                    if (count > 0) {
                        obj_proto->salvage_mats = malloc(sizeof(VNUM) * (size_t)count);
                        if (obj_proto->salvage_mats != NULL) {
                            memcpy(obj_proto->salvage_mats, temp_mats, sizeof(VNUM) * (size_t)count);
                            obj_proto->salvage_mat_count = count;
                        }
                    }
                }
                else {
                    bug("Load_objects: vnum %"PRVNUM" expected SalvageMats, got S%s.", vnum, word);
                }
            }
            else {
                ungetc(letter, fp);
                break;
            }
        }

        global_obj_proto_set(obj_proto);

        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;
        assign_area_vnum(vnum);
    }
}
