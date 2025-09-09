////////////////////////////////////////////////////////////////////////////////
// native_methods.h
// Enumerate native methods for Mud98 Entities in Lox
////////////////////////////////////////////////////////////////////////////////

#include "native.h"

#include <act_comm.h>
#include <comm.h>

#include <entities/area.h>
#include <entities/mobile.h>
#include <entities/object.h>
#include <entities/room.h>

Table native_methods;

static Value is_obj_native(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 0) {
        runtime_error("is_obj() takes no arguments.");
        return FALSE_VAL;
    }

    if (IS_OBJECT(receiver))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_obj_proto_native(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 0) {
        runtime_error("is_obj_proto() takes no arguments.");
        return FALSE_VAL;
    }

    if (IS_OBJ_PROTO(receiver))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_mob_native(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 0) {
        runtime_error("is_mob() takes no arguments.");
        return FALSE_VAL;
    }

    if (IS_MOBILE(receiver))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_mob_proto_native(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 0) {
        runtime_error("is_mob_proto() takes no arguments.");
        return FALSE_VAL;
    }

    if (IS_MOB_PROTO(receiver))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_room_native(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 0) {
        runtime_error("is_room() takes no arguments.");
        return FALSE_VAL;
    }

    if (IS_ROOM(receiver))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_room_data_native(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 0) {
        runtime_error("is_room_data() takes no arguments.");
        return FALSE_VAL;
    }

    if (IS_ROOM_DATA(receiver))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_area_native(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 0) {
        runtime_error("is_area() takes no arguments.");
        return FALSE_VAL;
    }

    if (IS_AREA(receiver))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value is_area_data_native(Value receiver, int arg_count, Value* args)
{
    if (arg_count != 0) {
        runtime_error("is_area_data() takes no arguments.");
        return FALSE_VAL;
    }

    if (IS_AREA_DATA(receiver))
        return TRUE_VAL;

    return FALSE_VAL;
}

static Value send_native(Value receiver, int arg_count, Value* args)
{
    if (arg_count == 0) {
        runtime_error("send() requires at least one argument.");
        return FALSE_VAL;
    }

    if (!IS_ENTITY(receiver)) {
        runtime_error("send() only works for game entities.");
        return FALSE_VAL;
    }

    if (IS_MOBILE(receiver)) {
        Mobile* mob = AS_MOBILE(receiver);

        // Skip NPCs (for now)
        if (mob->pcdata == NULL)
            return TRUE_VAL;

        for (int i = 0; i < arg_count; i++) {
            char* msg = string_value(args[i]);
            act(msg, mob, NULL, NULL, TO_CHAR);
        }
    }

    return TRUE_VAL;
}

const NativeMethodEntry native_method_entries[] = {
    { "is_area",        is_area_native              },
    { "is_area_data",   is_area_data_native         },
    { "is_mob",         is_mob_native               },
    { "is_mob_proto",   is_mob_proto_native         },
    { "is_obj",         is_obj_native               },
    { "is_obj_proto",   is_obj_proto_native         },
    { "is_room",        is_room_native              },
    { "is_room_data",   is_room_data_native         },
    { "send",           send_native                 },
    { NULL,             NULL                        },
};