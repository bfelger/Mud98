////////////////////////////////////////////////////////////////////////////////
// object.c
// Utilities for handling in-game objects
////////////////////////////////////////////////////////////////////////////////

#include "object.h"

#include <db.h>
#include <handler.h>
#include <lookup.h>
#include <magic.h>
#include <recycle.h>

#include <lox/vm.h>

List obj_free;
List obj_list;

Object* new_object()
{
    ENTITY_ALLOC_PERM(obj, Object);

    push(OBJ_VAL(obj));

    init_header(&obj->header, OBJ_OBJ);

    SET_NAME(obj, lox_empty_string);

    SET_NATIVE_FIELD(&obj->header, obj->header.vnum, vnum, I32);

    init_list(&obj->objects);

    SET_NATIVE_FIELD(&obj->header, obj->short_descr, short_desc, STR);
    SET_NATIVE_FIELD(&obj->header, obj->in_room, in_room, OBJ);

    pop();

    VALIDATE(obj);

    return obj;
}

void free_object(Object* obj)
{
    Affect* affect;
    Affect* paf_next = NULL;
    ExtraDesc* ed;
    ExtraDesc* ed_next = NULL;

    if (!IS_VALID(obj))
        return;

    for (affect = obj->affected; affect != NULL; affect = paf_next) {
        paf_next = affect->next;
        free_affect(affect);
    }
    obj->affected = NULL;

    for (ed = obj->extra_desc; ed != NULL; ed = ed_next) {
        ed_next = ed->next;
        free_extra_desc(ed);
    }
    obj->extra_desc = NULL;

    free_string(obj->description);
    free_string(obj->short_descr);
    obj->owner = NULL;
    INVALIDATE(obj);

    ENTITY_FREE(obj);
}

void clone_object(Object* parent, Object* clone)
{
    int i;
    Affect* affect;
    ExtraDesc* ed, * ed_new;

    if (parent == NULL || clone == NULL)
        return;

    /* start fixing the object */
    SET_NAME(clone, NAME_FIELD(parent));
    VNUM_FIELD(clone) = VNUM_FIELD(parent);

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

    FOR_EACH(affect, parent->affected)
        affect_to_obj(clone, affect);

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
//void convert_object(ObjPrototype* obj_proto)
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

Object* create_object(ObjPrototype* obj_proto, LEVEL level)
{
    Affect* affect;
    Object* obj;
    int i;

    if (obj_proto == NULL) {
        bug("Create_object: NULL obj_proto.", 0);
        exit(1);
    }

    obj = new_object();

    push(OBJ_VAL(obj));
    obj->obj_list_node = list_push_back(&obj_list, OBJ_VAL(obj));
    pop(); // obj

    obj->prototype = obj_proto;

    SET_NAME(obj, NAME_FIELD(obj_proto));
    VNUM_FIELD(obj) = VNUM_FIELD(obj_proto);

    if (obj_proto->header.klass != NULL) {
        obj->header.klass = obj_proto->header.klass;
        init_entity_class((Entity*)obj);
    }

    obj->header.events = obj_proto->header.events;
    obj->header.event_triggers = obj_proto->header.event_triggers;

    obj->in_room = NULL;
    obj->enchanted = false;

    obj->level = obj_proto->level;
    obj->wear_loc = -1;
    obj->short_descr = str_dup(obj_proto->short_descr);     // OLC
    obj->description = str_dup(obj_proto->description);     // OLC
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
        if (obj->value[2] == 999) 
            obj->value[2] = -1;
        break;

    case ITEM_JUKEBOX:
        for (i = 0; i < 5; i++) 
            obj->value[i] = -1;
        break;

    default:
        break;
    }

    FOR_EACH(affect, obj_proto->affected)
        if (affect->location == APPLY_SPELL_AFFECT) 
            affect_to_obj(obj, affect);

    obj_proto->count++;

    return obj;
}

////////////////////////////////////////////////////////////////////////////////
// Lox representation
////////////////////////////////////////////////////////////////////////////////

//static ObjClass* object_class = NULL;
//
//void init_object_class()
//{
//    char* source =
//        "class Object { "
//        "   short_desc() { return marshal(this._short_desc); } "
//        "   in_room() { return get_room(this._in_room); } "
//        "}";
//
//    InterpretResult result = interpret_code(source);
//
//    if (result == INTERPRET_COMPILE_ERROR) exit(65);
//    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
//
//    object_class = find_class("Object");
//}
//
//Value create_object_value(Object* object)
//{
//    if (!object_class)
//        runtime_error("create_object_value: 'Object' class not defined.");
//
//    if (!object || !object_class)
//        return NIL_VAL;
//
//    ObjInstance* inst = new_instance(object_class);
//    push(OBJ_VAL(inst));
//
//    SET_NATIVE_FIELD(inst, object, base, OBJ);
//    SET_NATIVE_FIELD(inst, VNUM_FIELD(object->prototype), vnum, I32);
//    SET_NATIVE_FIELD(inst, object->short_descr, short_desc, STR);
//    SET_NATIVE_FIELD(inst, object->in_room, in_room, OBJ);
//
//    SET_LOX_FIELD(inst, object->header.name, name);
//
//    pop(); // instance
//
//    return OBJ_VAL(inst);
//}
